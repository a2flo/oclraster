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

#include "a2m.h"
#include "file_io.h"
#include "core.h"
#include "oclraster.h"

static constexpr unsigned int A2M_VERSION = 2u;

a2m::a2m(const string& filename) {
	load(filename);
}

a2m::~a2m() {
	if(cl_vertex_buffer.buffer != nullptr) {
		ocl->delete_buffer(cl_vertex_buffer.buffer);
	}
	for(const auto& ib : cl_index_buffers) {
		if(ib.buffer != nullptr) {
			ocl->delete_buffer(ib.buffer);
		}
	}
	if(vertices != nullptr) delete [] vertices;
	if(normals != nullptr) delete [] normals;
	if(binormals != nullptr) delete [] binormals;
	if(tangents != nullptr) delete [] tangents;
	if(tex_coords != nullptr) delete [] tex_coords;
	if(index_count != nullptr) delete [] index_count;
	if(indices != nullptr) {
		for(unsigned int i = 0; i < object_count; i++) {
			// note: indices and tex_indices point to the same location now
			delete [] indices[i];
		}
		delete [] indices;
	}
}

void a2m::load(const string& filename) {
	file_io file(filename, file_io::OPEN_TYPE::READ_BINARY);
	if(!file.is_open()) {
		return;
	}
	
	// get type and name
	char file_type[8];
	file.get_block(file_type, 8);
	if(strncmp(file_type, "A2EMODEL", 8) != 0) {
		oclr_error("non supported file type for %s: %s!", filename, file_type);
		file.close();
		return;
	}
	
	// get model version
	const unsigned int version = file.get_uint();
	if(version != A2M_VERSION) {
		oclr_error("wrong model file version %u - should be %u!", version, A2M_VERSION);
		file.close();
		return;
	}
	
	// get model type and abort if it's not 0x00 (note: 0x02 is not supported any more)
	const char mtype = file.get_char();
	if(mtype != 0x00) {
		oclr_error("non supported model type: %u!", (unsigned int)(mtype & 0xFF));
		file.close();
		return;
	}
	
	// get the actual model data
	// start off with the global data (vertices + texture coordinates):
	vertex_count = file.get_uint();
	tex_coord_count = file.get_uint();
	vertices = new float3[vertex_count];
	normals = new float3[vertex_count];
	binormals = new float3[vertex_count];
	tangents = new float3[vertex_count];
	tex_coords = new float2[tex_coord_count];
	
	for(unsigned int i = 0; i < vertex_count; i++) {
		vertices[i].x = file.get_float();
		vertices[i].y = file.get_float();
		vertices[i].z = file.get_float();
	}
	for(unsigned int i = 0; i < tex_coord_count; i++) {
		tex_coords[i].u = file.get_float();
		tex_coords[i].v = 1.0f - file.get_float();
	}
	
	// load the sub-object specific data (indices, names):
	object_count = file.get_uint();
	for(unsigned int i = 0; i < object_count; i++) {
		string object_name = "";
		file.get_terminated_block(object_name, (char)0xFF);
		object_names.emplace_back(object_name);
	}
	
	indices = new index3*[object_count];
	tex_indices = new index3*[object_count];
	index_count = new unsigned int[object_count];
	for(unsigned int i = 0; i < object_count; i++) {
		index_count[i] = file.get_uint();
		indices[i] = new index3[index_count[i]];
		tex_indices[i] = new index3[index_count[i]];
		for(unsigned int j = 0; j < index_count[i]; j++) {
			indices[i][j].x = file.get_uint();
			indices[i][j].y = file.get_uint();
			indices[i][j].z = file.get_uint();
		}
		for(unsigned int j = 0; j < index_count[i]; j++) {
			tex_indices[i][j].x = file.get_uint();
			tex_indices[i][j].y = file.get_uint();
			tex_indices[i][j].z = file.get_uint();
		}
	}
	
	file.close();
	
	// finally: generate normals (using the loaded vertex/tex coord/index info) and reorganize the model
	generate_normals();
	reorganize_model_data();
	
	// create opencl buffers
	vertex_data* vdata = new vertex_data[vertex_count];
	for(unsigned int i = 0; i < vertex_count; i++) {
		vdata[i].vertex = vertices[i];
		vdata[i].normal = normals[i];
		vdata[i].binormal = binormals[i];
		vdata[i].tangent = tangents[i];
		vdata[i].vertex.w = 1.0f;
		vdata[i].normal.w = 1.0f;
		vdata[i].binormal.w = 1.0f;
		vdata[i].tangent.w = 1.0f;
		vdata[i].tex_coord = tex_coords[i];
	}
	cl_vertex_buffer.buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
												 opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
												 opencl::BUFFER_FLAG::INITIAL_COPY,
												 sizeof(vertex_data) * vertex_count,
												 vdata);
	delete [] vdata;
	
	for(unsigned int i = 0; i < object_count; i++) {
		transform_stage::index_buffer ib;
		ib.index_count = index_count[i] * 3; // 3 vertices/indices per triangle
		ib.buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
									   opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
									   opencl::BUFFER_FLAG::INITIAL_COPY,
									   sizeof(unsigned int) * ib.index_count,
									   indices[i]);
		cl_index_buffers.emplace_back(ib);
	}
}

void a2m::generate_normals() {
	float3 normal, binormal, tangent;
	for(unsigned int i = 0; i < object_count; i++) {
		for(unsigned int j = 0; j < index_count[i]; j++) {
			core::compute_normal_tangent_binormal(vertices[indices[i][j].x],
												  vertices[indices[i][j].y],
												  vertices[indices[i][j].z],
												  normal, binormal, tangent,
												  tex_coords[tex_indices[i][j].x],
												  tex_coords[tex_indices[i][j].y],
												  tex_coords[tex_indices[i][j].z]);
			normals[indices[i][j].x] += normal;
			normals[indices[i][j].y] += normal;
			normals[indices[i][j].z] += normal;
			binormals[indices[i][j].x] += binormal;
			binormals[indices[i][j].y] += binormal;
			binormals[indices[i][j].z] += binormal;
			tangents[indices[i][j].x] += tangent;
			tangents[indices[i][j].y] += tangent;
			tangents[indices[i][j].z] += tangent;
		}
	}
	for(unsigned int i = 0; i < vertex_count; i++) {
		normals[i].normalize();
		binormals[i].normalize();
		tangents[i].normalize();
	}
}

/*! reorganizes the model data, so that each texture coordinate has it's own vertex
 *! -> this will generate the same amount of vertices as there are texture coordinates
 *! additionally, all indices have to be "merged" into one index buffer (there simply can't be two)
 */
void a2m::reorganize_model_data() {
	float3* old_vertices = vertices;
	float3* old_normals = normals;
	float3* old_binormals = binormals;
	float3* old_tangents = tangents;
	
	vertex_count = tex_coord_count;
	vertices = new float3[vertex_count];
	normals = new float3[vertex_count];
	binormals = new float3[vertex_count];
	tangents = new float3[vertex_count];
	
	for(unsigned int i = 0; i < object_count; i++) {
		for(unsigned int j = 0; j < index_count[i]; j++) {
			vertices[tex_indices[i][j].x] = old_vertices[indices[i][j].x];
			vertices[tex_indices[i][j].y] = old_vertices[indices[i][j].y];
			vertices[tex_indices[i][j].z] = old_vertices[indices[i][j].z];
			
			normals[tex_indices[i][j].x] = old_normals[indices[i][j].x];
			normals[tex_indices[i][j].y] = old_normals[indices[i][j].y];
			normals[tex_indices[i][j].z] = old_normals[indices[i][j].z];
			
			binormals[tex_indices[i][j].x] = old_binormals[indices[i][j].x];
			binormals[tex_indices[i][j].y] = old_binormals[indices[i][j].y];
			binormals[tex_indices[i][j].z] = old_binormals[indices[i][j].z];
			
			tangents[tex_indices[i][j].x] = old_tangents[indices[i][j].x];
			tangents[tex_indices[i][j].y] = old_tangents[indices[i][j].y];
			tangents[tex_indices[i][j].z] = old_tangents[indices[i][j].z];
		}
	}
	
	delete [] old_vertices;
	delete [] old_normals;
	delete [] old_binormals;
	delete [] old_tangents;
	for(unsigned int i = 0; i < object_count; i++) {
		delete [] indices[i];
	}
	delete [] indices;
	indices = tex_indices;
}

const transform_stage::vertex_buffer& a2m::get_vertex_buffer() const {
	return cl_vertex_buffer;
}

const transform_stage::index_buffer& a2m::get_index_buffer(const size_t& sub_object) const {
	return cl_index_buffers[sub_object];
}

unsigned int a2m::get_index_count(const unsigned int& sub_object) const {
	return index_count[sub_object];
}
