/*
 *  Flexible OpenCL Rasterizer (oclraster)
 *  Copyright (C) 2012 - 2013 Florian Ziesche
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "volume.hpp"

#define MIN_VOLUME_SIZE 32
#define MAX_VOLUME_SIZE 128
#define MAX_TEXTURE_SIZE 4096u
#define LAYER_SPACING ((LAYER_SIZE * 2.0f) / (float)MAX_VOLUME_SIZE)

volume* volume::from_file(const string& filename) {
	// reads the header data
	fstream file;
	string value;
	string raw_path;
	string format;
	size3 resolution;
	
	// open and read info/dat file
	file.open(filename, fstream::in);
	if(!file.is_open()) {
		log_error("couldn't open %s!", filename);
		file.close();
		return nullptr;
	}
	
	while(file.good()) {
		file >> value;
		if(value == "ObjectFileName:") {
			if(file >> value && file.good()) raw_path = value;
		}
		else if(value == "Resolution:") {
			if(file >> value && file.good()) resolution.x = strtoul(value.c_str(), nullptr, 10);
			if(file >> value && file.good()) resolution.y = strtoul(value.c_str(), nullptr, 10);
			if(file >> value && file.good()) resolution.z = strtoul(value.c_str(), nullptr, 10);
		}
		else if(value == "Format:") {
			if(file >> value && file.good()) format = value;
		}
	}
	
	file.close();
	file.clear();
	
	// open and read volume data file
	string subfilename = filename;
	subfilename = subfilename.substr(0, subfilename.rfind("/") + 1) + raw_path;

	file.open(subfilename.c_str(), fstream::in | fstream::binary);
	if(!file.is_open()) {
		log_error("couldn't open %s!", subfilename);
		file.close();
		return nullptr;
	}
	
	file.seekg(0, ios::end);
	size_t size = (size_t)file.tellg();
	file.seekg(0, ios::beg);
	unsigned char* read_volume_data = new unsigned char[size];
	file.read((char*)read_volume_data, (streamsize)size);
	
	file.close();
	
	// convert 16-bit data to 8-bit (note: this is more of a test right now; values are simply divided by 256)
	if(format == "USHORT") {
		unsigned char* rd_volume_data = new unsigned char[resolution.x*resolution.y*resolution.z];
		size_t index;
		for(size_t x = 0; x < resolution.x; x++) {
			for(size_t y = 0; y < resolution.y; y++) {
				for(size_t z = 0; z < resolution.z; z++) {
					index = (z*resolution.x*resolution.y + y*resolution.x + x);
					float val = (unsigned short)read_volume_data[index*2+1] + (((unsigned short)read_volume_data[index*2]) << 8);
					rd_volume_data[index] = val/256.0f;
				}
			}
		}
		delete [] read_volume_data;
		read_volume_data = rd_volume_data;
	}
	
	// restructure data
	unsigned char* volume_data = new unsigned char[size];
	for(size_t x = 0; x < resolution.x; x++) {
		for(size_t y = 0; y < resolution.y; y++) {
			for(size_t z = 0; z < resolution.z; z++) {
				volume_data[x*resolution.y*resolution.z + y*resolution.z + z] = read_volume_data[z*resolution.x*resolution.y + y*resolution.x + x];
			}
		}
	}
	delete [] read_volume_data;
	
	// create and set volume
	log_msg("creating volume \"%s\" with resolution %v!", filename, resolution);
	volume* new_volume = new volume(volume_data, resolution);
	delete [] volume_data;
	return new_volume;
}

//
volume::volume(const unsigned char* volume_data, const uint3& volume_size) : size(volume_size) {
	const unsigned char* data = volume_data;
	if(volume_size.min_element() < MIN_VOLUME_SIZE ||
	   volume_size.max_element() > MAX_VOLUME_SIZE) {
		data = resize_volume(data);
	}
	construct_texture(data);
	construct_buffers();
	if(data != volume_data) delete [] data;
}

volume::~volume() {
	for(size_t i = 0; i < 3; i++) {
		if(vertex_buffer[i] != nullptr) {
			ocl->delete_buffer(vertex_buffer[i]);
		}
		if(index_buffer[i*2] != nullptr) {
			ocl->delete_buffer(index_buffer[i*2]);
		}
		if(index_buffer[i*2 + 1] != nullptr) {
			ocl->delete_buffer(index_buffer[i*2 + 1]);
		}
		if(stacks[i] != nullptr) delete stacks[i];
	}
}

void volume::construct_volume_texture(unsigned char* volume_tex, const unsigned char* volume_data, const size_t& axis, const size_t data_offset) {
	const size_t max_texture_size = MAX_TEXTURE_SIZE;
	const size_t tex_w = max_texture_size; // TODO: compute this dependent on volume size
	const size3 max_slices = size3(max_texture_size)/size; // max slices per row, = 16
	
	size2 offset;
	for(size_t x = 0; x < size.x; x++) {
		for(size_t y = 0; y < size.y; y++) {
			for(size_t z = 0; z < size.z; z++) {
				size_t tex_index = 0;
				switch(axis) {
					case 0: // x axis
						// Y * Z
						// x = slice%max_slices, y = slice/max_slices
						offset.x = (x%max_slices.y) * size.y;
						offset.y = (x/max_slices.y) * tex_w*size.z;
						tex_index = offset.x + offset.y + z*tex_w + y;
						break;
					case 2: // z axis
						// X * Y
						offset.x = (z%max_slices.x) * size.x;
						offset.y = (z/max_slices.x) * tex_w*size.y;
						tex_index = offset.x + offset.y + y*tex_w + x;
						break;
					case 1: // y axis
						// X * Z
						offset.x = (y%max_slices.x) * size.x;
						offset.y = (y/max_slices.x) * tex_w*size.z;
						tex_index = offset.x + offset.y + z*tex_w + x;
						break;
					default:
						break;
				}
				volume_tex[data_offset + tex_index] = volume_data[x*size.y*size.z + y*size.z + z];
			}
		}
	}
}

void volume::construct_texture(const unsigned char* volume_data) {
	// conversion to 2d texture
	const size_t max_texture_size = MAX_TEXTURE_SIZE;
	const size_t tex_w = max_texture_size; // TODO: compute this dependent on volume size
	const size_t tex_h = max_texture_size;
	
	// for each axis
	size_t volume_textures = 3; // 1: single-stack, 3: 1 stack per axis
	for(size_t axis = 0; axis < volume_textures; axis++) {
		unsigned char* volume_tex = new unsigned char[tex_w*tex_h];
		construct_volume_texture(volume_tex, volume_data, axis, 0);
		stacks[axis] = new image(tex_w, tex_h, image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::R, &volume_tex[0]);
		delete [] volume_tex;
	}
}

void volume::construct_buffers() {
	const size_t max_texture_size = MAX_TEXTURE_SIZE;
	const size3 max_slices_row(max_texture_size/size.y, max_texture_size/size.x, max_texture_size/size.x); // max slices per row
	const float2 slice_size[3] = {
		float2((float)size.y / (float)max_texture_size, (float)size.z / (float)max_texture_size),
		float2((float)size.x / (float)max_texture_size, (float)size.z / (float)max_texture_size),
		float2((float)size.x / (float)max_texture_size, (float)size.y / (float)max_texture_size),
	};
	
	const float3 min_vertex((-float3(size) * LAYER_SPACING) * 0.5f);
	const float3 max_vertex((float3(size) * LAYER_SPACING) * 0.5f);
	
	bool single_stack = false; // don't use single-stack for now
	for(size_t axis = 0; axis < 3; axis++) {
		const unsigned int layers = size[axis];
		const float half_layers = float(layers) * 0.5f;
		
		vertex_data* vdata = new vertex_data[layers*4];
		index3* volume_indices_1 = new index3[layers*2];
		index3* volume_indices_2 = new index3[layers*2];
		for(unsigned int layer = 0; layer < layers; layer++) {
			const unsigned int layer_offset = layer * 4u;
			const float layer_pos = (float(layer) - half_layers) * LAYER_SPACING;
			switch(axis) {
				case 0:
					vdata[layer_offset + 0].vertex = float4(layer_pos, min_vertex.y, max_vertex.z, 1.0f);
					vdata[layer_offset + 1].vertex = float4(layer_pos, max_vertex.y, max_vertex.z, 1.0f);
					vdata[layer_offset + 2].vertex = float4(layer_pos, max_vertex.y, min_vertex.z, 1.0f);
					vdata[layer_offset + 3].vertex = float4(layer_pos, min_vertex.y, min_vertex.z, 1.0f);
					break;
				case 1:
					vdata[layer_offset + 0].vertex = float4(min_vertex.x, layer_pos, min_vertex.z, 1.0f);
					vdata[layer_offset + 1].vertex = float4(max_vertex.x, layer_pos, min_vertex.z, 1.0f);
					vdata[layer_offset + 2].vertex = float4(max_vertex.x, layer_pos, max_vertex.z, 1.0f);
					vdata[layer_offset + 3].vertex = float4(min_vertex.x, layer_pos, max_vertex.z, 1.0f);
					break;
				case 2:
					vdata[layer_offset + 0].vertex = float4(min_vertex.x, min_vertex.y, layer_pos, 1.0f);
					vdata[layer_offset + 1].vertex = float4(min_vertex.x, max_vertex.y, layer_pos, 1.0f);
					vdata[layer_offset + 2].vertex = float4(max_vertex.x, max_vertex.y, layer_pos, 1.0f);
					vdata[layer_offset + 3].vertex = float4(max_vertex.x, min_vertex.y, layer_pos, 1.0f);
					break;
				default:
					break;
			}
			
			if(!single_stack) {
				float2 offset(layer%max_slices_row[axis], layer/max_slices_row[axis]);
				float2 tc_min { offset.x * slice_size[axis].x, offset.y * slice_size[axis].y };
				float2 tc_max = tc_min + float2 { slice_size[axis].x, slice_size[axis].y };
				switch(axis) {
					case 0:
						vdata[layer_offset + 0].tex_coord = float2(tc_min.x, tc_max.y);
						vdata[layer_offset + 1].tex_coord = float2(tc_max.x, tc_max.y);
						vdata[layer_offset + 2].tex_coord = float2(tc_max.x, tc_min.y);
						vdata[layer_offset + 3].tex_coord = float2(tc_min.x, tc_min.y);
						break;
					case 1:
						vdata[layer_offset + 0].tex_coord = float2(tc_min.x, tc_min.y);
						vdata[layer_offset + 1].tex_coord = float2(tc_max.x, tc_min.y);
						vdata[layer_offset + 2].tex_coord = float2(tc_max.x, tc_max.y);
						vdata[layer_offset + 3].tex_coord = float2(tc_min.x, tc_max.y);
						break;
					case 2:
						vdata[layer_offset + 0].tex_coord = float2(tc_min.x, tc_min.y);
						vdata[layer_offset + 1].tex_coord = float2(tc_min.x, tc_max.y);
						vdata[layer_offset + 2].tex_coord = float2(tc_max.x, tc_max.y);
						vdata[layer_offset + 3].tex_coord = float2(tc_max.x, tc_min.y);
						break;
					default: break;
				}
			}
			else {
				vdata[layer_offset + 0].tex_coord = float2((float)layer, 0.0f);
				vdata[layer_offset + 1].tex_coord = float2((float)layer, 1.0f);
				vdata[layer_offset + 2].tex_coord = float2((float)layer, 2.0f);
				vdata[layer_offset + 3].tex_coord = float2((float)layer, 3.0f);
			}
			
			volume_indices_1[layer*2 + 0] = index3(layer_offset + 0, layer_offset + 2, layer_offset + 1);
			volume_indices_1[layer*2 + 1] = index3(layer_offset + 0, layer_offset + 3, layer_offset + 2);
			volume_indices_2[layers*2 - layer*2 - 2] = index3(layer_offset + 0, layer_offset + 1, layer_offset + 2);
			volume_indices_2[layers*2 - layer*2 - 1] = index3(layer_offset + 0, layer_offset + 2, layer_offset + 3);
		}
		
		vertex_buffer[axis] = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
												 opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
												 opencl::BUFFER_FLAG::INITIAL_COPY,
												 (layers * 4) * sizeof(vertex_data),
												 vdata);
		
		index_buffer[axis*2] = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
												  opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
												  opencl::BUFFER_FLAG::INITIAL_COPY,
												  (layers * 2) * sizeof(index3),
												  volume_indices_1);
		
		index_buffer[axis*2 + 1] = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
													  opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
													  opencl::BUFFER_FLAG::INITIAL_COPY,
													  (layers * 2) * sizeof(index3),
													  volume_indices_2);
		
		delete [] vdata;
		delete [] volume_indices_1;
		delete [] volume_indices_2;
		
		index_count[axis] = layers*2;
	}
}

const unsigned char* volume::resize_volume(const unsigned char* volume_data) {
	if(size.min_element() < MIN_VOLUME_SIZE && size.max_element() == MAX_VOLUME_SIZE) {
		// one side is below the minimum size, but another is already at the max size -> can't do anything here
		return volume_data;
	}
	
	// TODO: resized size computation, this may not be larger than max size and should! not be smaller than min size
	// also: rather use an algorithm that checks if the size fits into the texture, instead of using hard max/min values
	
	// determine if we have to shrink or scale up the volume
	unsigned char* resized_volume = nullptr;
	if(size.max_element() > MAX_VOLUME_SIZE) {
		// shrink
		float scale_factor = (float)size.max_element() / (float)MAX_VOLUME_SIZE;
		
		// first try: weighted interpolation
		size3 resized_size = size3((float3(size) / scale_factor).ceil());
		resized_volume = new unsigned char[resized_size.x*resized_size.y*resized_size.z];
		
		float3 pos(0.0f);
		for(size_t x = 0; x < resized_size.x; x++) {
			pos.y = 0.0f;
			for(size_t y = 0; y < resized_size.y; y++) {
				pos.z = 0.0f;
				for(size_t z = 0; z < resized_size.z; z++) {
					//
					float value = 0.0f;
					size_t count = 0;
					float3 weight;
					
					float3 next_value = pos + scale_factor;
					float3 rnd_next_value = next_value.ceiled();
					size3 steps = float3(rnd_next_value - pos).round();
					
					float3 lower_weight = pos.ceiled() - pos;
					float3 upper_weight = float3(1.0f) - rnd_next_value - next_value;
					
					for(size_t sx = 0; sx < steps.x; sx++) {
						weight.x = (sx == 0 ? lower_weight.x : (sx == steps.x ? upper_weight.x : 1.0f));
						
						for(size_t sy = 0; sy < steps.y; sy++) {
							weight.y = (sy == 0 ? lower_weight.y : (sy == steps.y ? upper_weight.y : 1.0f));
							
							for(size_t sz = 0; sz < steps.z; sz++) {
								weight.z = (sz == 0 ? lower_weight.z : (sz == steps.z ? upper_weight.z : 1.0f));
								
								//
								size3 min(float3(pos + float3(sx, sy, sz)).floor());
								unsigned char val = volume_data[min.x*size.y*size.z + min.y*size.z + min.z];
								
								float avg_weight = (weight.x+weight.y+weight.z)/3.0f;
								avg_weight = 1.0f;
								value += avg_weight * (float)val;
								count++;
							}
						}
					}
					
					value /= (float)count;
					resized_volume[x*resized_size.y*resized_size.z + y*resized_size.z + z] = (unsigned char)rintf(value);
					
					pos.z += scale_factor;
				}
				pos.y += scale_factor;
			}
			pos.x += scale_factor;
		}
		
		size = resized_size;
	}
	else {
		// scale up
		float scale_factor = ceilf((float)MIN_VOLUME_SIZE / (float)size.min_element());
		unsigned int cscale_factor = (unsigned int)scale_factor;
		uint3 resized_size = size * cscale_factor;
		resized_volume = new unsigned char[resized_size.x*resized_size.y*resized_size.z];
		
#define VOLUME_INTERPOLATION 1
#if !defined(VOLUME_INTERPOLATION)
		// no interpolation whatsoever
		size_t steps = cscale_factor;
		for(size_t x = 0; x < size.x; x++) {
			for(size_t y = 0; y < size.y; y++) {
				for(size_t z = 0; z < size.z; z++) {
					unsigned char value = volume_data[x*size.y*size.z + y*size.z + z];
					
					for(size_t sx = 0; sx < steps; sx++) {
						for(size_t sy = 0; sy < steps; sy++) {
							for(size_t sz = 0; sz < steps; sz++) {
								size_t index = ((x*steps)+sx)*resized_size.y*resized_size.z + ((y*steps)+sy)*resized_size.z + ((z*steps)+sz);
								resized_volume[index] = value;
							}
						}
					}
				}
			}
		}
#else
		// simple trilinear interpolation
		size_t steps = cscale_factor;
		for(size_t x = 0; x < size.x; x++) {
			for(size_t y = 0; y < size.y; y++) {
				for(size_t z = 0; z < size.z; z++) {
					// to get the color for each of the eight corners of the voxel, we have to sample the eight
					// surrounding/corner voxels of each corner and take the average
					
					// take care of border cases
					ssize3 x_vals = ssize3(x-1, x, x+1).clamped(0, size.x-1);
					ssize3 y_vals = ssize3(y-1, y, y+1).clamped(0, size.y-1);
					ssize3 z_vals = ssize3(z-1, z, z+1).clamped(0, size.z-1);
					
					unsigned int values[27];
					for(size_t vx = 0; vx < 3; vx++) {
						for(size_t vy = 0; vy < 3; vy++) {
							for(size_t vz = 0; vz < 3; vz++) {
								values[vx*3*3 + vy*3 + vz] = volume_data[x_vals[vx]*size.y*size.z + y_vals[vy]*size.z + z_vals[vz]];
							}
						}
					}
					
					float corners[8];
					for(size_t cx = 0; cx < 2; cx++) {
						for(size_t cy = 0; cy < 2; cy++) {
							for(size_t cz = 0; cz < 2; cz++) {
								corners[cx*2*2 + cy*2 + cz] = float(values[cx*3*3 + cy*3 + cz] +
																	values[cx*3*3 + cy*3 + cz+1] +
																	values[cx*3*3 + (cy+1)*3 + cz] +
																	values[cx*3*3 + (cy+1)*3 + cz+1] +
																	values[(cx+1)*3*3 + cy*3 + cz] +
																	values[(cx+1)*3*3 + cy*3 + cz+1] +
																	values[(cx+1)*3*3 + (cy+1)*3 + cz] +
																	values[(cx+1)*3*3 + (cy+1)*3 + cz+1]) / 8.0f;
							}
						}
					}
					
					float value = 0.0f;
					for(size_t sx = 0; sx < steps; sx++) {
						for(size_t sy = 0; sy < steps; sy++) {
							for(size_t sz = 0; sz < steps; sz++) {
								// interpolation
								float3 icoeff = float3(sx, sy, sz) / float3(steps-1);
								float3 coeff = float3(1.0f) - icoeff;
								
								value = (((corners[0]*coeff.z + corners[1]*icoeff.z)*coeff.y +
										  (corners[2]*coeff.z + corners[3]*icoeff.z)*icoeff.y)*coeff.x +
										 ((corners[4]*coeff.z + corners[5]*icoeff.z)*coeff.y +
										  (corners[6]*coeff.z + corners[7]*icoeff.z)*icoeff.y)*icoeff.x);
								
								size_t index = ((x*steps)+sx)*resized_size.y*resized_size.z + ((y*steps)+sy)*resized_size.z + ((z*steps)+sz);
								resized_volume[index] = (unsigned char)rintf(value);
							}
						}
					}
				}
			}
		}
#endif
		
		size = resized_size;
	}
	
	return (resized_volume != nullptr ? resized_volume : volume_data);
}
