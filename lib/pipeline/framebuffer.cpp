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

#include "framebuffer.h"
#include "oclraster.h"
#include "oclraster_program.h"
#include "pipeline.h"

//
static constexpr char template_framebuffer_program[] { u8R"OCLRASTER_RAWSTR(
	#include "oclr_global.h"
	#include "oclr_framebuffer_clear.h"
	
	//
	void OCLRASTER_FUNC clear_depth(global float* depth_image, const uint offset, const float clear_depth) {
		depth_image[offset] = clear_depth;
	}
	
	void FUNC_OVERLOAD OCLRASTER_FUNC clear_stencil(global uchar* stencil_image, const uint offset, const ulong clear_stencil) {
		stencil_image[offset] = clear_stencil;
	}
	void FUNC_OVERLOAD OCLRASTER_FUNC clear_stencil(global ushort* stencil_image, const uint offset, const ulong clear_stencil) {
		stencil_image[offset] = clear_stencil;
	}
	void FUNC_OVERLOAD OCLRASTER_FUNC clear_stencil(global uint* stencil_image, const uint offset, const ulong clear_stencil) {
		stencil_image[offset] = clear_stencil;
	}
	void FUNC_OVERLOAD OCLRASTER_FUNC clear_stencil(global ulong* stencil_image, const uint offset, const ulong clear_stencil) {
		stencil_image[offset] = clear_stencil;
	}
	
	//
	kernel void clear_framebuffer(//###OCLRASTER_FRAMEBUFFER_IMAGES###
								  //###OCLRASTER_FRAMEBUFFER_CLEAR_COLORS###
								  const uint2 framebuffer_size,
#if defined(CLEAR_DEPTH)
								  const float clear_depth_value,
#endif
#if defined(CLEAR_STENCIL)
								  const ulong clear_stencil_value,
#endif
								  const uint4 scissor_rectangle) { // note: scissor_rectangle contains absolute coordinates
		const unsigned int x = get_global_id(0) + scissor_rectangle.x;
		const unsigned int y = get_global_id(1) + scissor_rectangle.y;
		if(x >= framebuffer_size.x || y >= framebuffer_size.y ||
		   x >= scissor_rectangle.z || y >= scissor_rectangle.w) {
			return;
		}
		const unsigned int offset = y * framebuffer_size.x + x;
		
		//###OCLRASTER_FRAMEBUFFER_CLEAR_CALLS###
#if defined(CLEAR_DEPTH)
		clear_depth(depth_image, offset, clear_depth_value);
#endif
#if defined(CLEAR_STENCIL)
		clear_stencil(stencil_image, offset, clear_stencil_value);
#endif
	}
)OCLRASTER_RAWSTR"};

// static, usable across all framebuffers
class framebuffer_program {
public:
	framebuffer_program() = delete;
	~framebuffer_program() = delete;
	framebuffer_program(framebuffer_program& prog) = delete;
	framebuffer_program& operator=(framebuffer_program& prog) = delete;
	
	struct image_spec {
		vector<image_type> images;
		image_type depth_image;
		image_type stencil_image;
	};
	struct clear_kernel {
		weak_ptr<opencl::kernel_object> kernel;
		vector<IMAGE_TYPE> clear_image_types; // unique set of the images data types
	};
	static vector<image_spec*> compiled_image_kernels;
	static unordered_map<image_spec*, clear_kernel> kernels;
	static const clear_kernel& get_clear_kernel(const image_spec& spec);
	static const clear_kernel& build_kernel(const image_spec& spec);
	
	static void delete_kernels();
	
};
vector<framebuffer_program::image_spec*> framebuffer_program::compiled_image_kernels;
unordered_map<framebuffer_program::image_spec*, framebuffer_program::clear_kernel> framebuffer_program::kernels;

const framebuffer_program::clear_kernel& framebuffer_program::get_clear_kernel(const image_spec& spec) {
	const size_t spec_size = spec.images.size();
	for(const auto& kernel : kernels) {
		if(kernel.first->images.size() != spec_size) continue;
		if(kernel.first->depth_image != spec.depth_image) continue;
		if(kernel.first->stencil_image != spec.stencil_image) continue;
		
		bool spec_found = true;
		for(size_t i = 0; i < spec_size; i++) {
			if(spec.images[i] != kernel.first->images[i]) {
				spec_found = false;
				break;
			}
		}
		if(!spec_found) continue;
		
		return kernel.second;
	}
	// new kernel image spec -> compile new kernel
	return build_kernel(spec);
}

const framebuffer_program::clear_kernel& framebuffer_program::build_kernel(const image_spec& spec) {
	image_spec* new_spec = new image_spec(spec);
	compiled_image_kernels.emplace_back(new_spec);
	
	//
	string program_code { template_framebuffer_program };
	string kernel_image_parameters = "", clear_colors = "", clear_calls = "", img_spec_str = "";
	unordered_set<IMAGE_TYPE> clear_image_types;
	vector<IMAGE_TYPE> clear_image_types_vec;
	size_t img_idx = 0;
	for(const auto& type : spec.images) {
		clear_image_types.emplace(type.data_type);
		const auto type_str = type.to_string();
		img_spec_str += "." + type_str;
		kernel_image_parameters += "global " + type_str + "* image_" + size_t2string(img_idx) + ",\n";
		clear_calls += "clear_image(image_" + size_t2string(img_idx) + ", offset, clear_color_value_" + image_data_type_to_string(type.data_type) + ");\n";
		img_idx++;
	}
	
	for(const auto& type : clear_image_types) {
		// note: we need a set to get all image data types once (unique set), but we need a vector for consistency/ordering
		clear_image_types_vec.emplace_back(type);
		
		const string data_type_str = image_data_type_to_string(type);
		clear_colors += "const " + data_type_str + "4 clear_color_value_" + data_type_str + ",\n";
	}
	
	string build_options = "";
	if(spec.depth_image.is_valid()) {
		build_options += " -DCLEAR_DEPTH";
		kernel_image_parameters += "global float* depth_image,\n";
		clear_calls += "clear_depth(depth_image, offset, clear_depth_value);\n";
	}
	if(spec.stencil_image.is_valid()) {
		build_options += " -DCLEAR_STENCIL";
		kernel_image_parameters += "global " + spec.stencil_image.to_string() + "* stencil_image,\n";
		clear_calls += "clear_stencil(stencil_image, offset, clear_stencil_value);\n";
	}
	img_spec_str += ".depth_"+spec.depth_image.to_string();
	img_spec_str += ".stencil_"+spec.stencil_image.to_string();
	
	core::find_and_replace(program_code, "//###OCLRASTER_FRAMEBUFFER_IMAGES###", kernel_image_parameters);
	core::find_and_replace(program_code, "//###OCLRASTER_FRAMEBUFFER_CLEAR_COLORS###", clear_colors);
	core::find_and_replace(program_code, "//###OCLRASTER_FRAMEBUFFER_CLEAR_CALLS###", clear_calls);
	
	//
	const string identifier = "FRAMEBUFFER_CLEAR"+img_spec_str;
	weak_ptr<opencl::kernel_object> kernel = ocl->add_kernel_src(identifier, program_code, "clear_framebuffer", build_options);
#if defined(OCLRASTER_DEBUG)
	if(kernel.use_count() == 0) {
		log_debug("kernel source: %s", program_code);
	}
#endif
	kernels.emplace(new_spec, framebuffer_program::clear_kernel { kernel, std::move(clear_image_types_vec) });
	return kernels.at(new_spec);
}

void framebuffer_program::delete_kernels() {
	for(auto& spec : compiled_image_kernels) {
		delete spec;
	}
	compiled_image_kernels.clear();
	kernels.clear();
}
	
void delete_clear_kernels() {
	framebuffer_program::delete_kernels();
}

//
framebuffer framebuffer::create_with_images(const unsigned int& width, const unsigned int& height,
											initializer_list<pair<IMAGE_TYPE, IMAGE_CHANNEL>> image_types,
											pair<IMAGE_TYPE, IMAGE_CHANNEL> depth_type,
											pair<IMAGE_TYPE, IMAGE_CHANNEL> stencil_type) {
	//
	framebuffer ret_fb(width, height);
	
	// check validity
	for(const auto& img_type : image_types) {
		if(img_type.first == IMAGE_TYPE::NONE ||
		   img_type.second == IMAGE_CHANNEL::NONE) {
			log_error("framebuffer images without a type are not allowed!");
			return ret_fb;
		}
	}
	if(!((depth_type.first == IMAGE_TYPE::NONE && depth_type.second == IMAGE_CHANNEL::NONE) ||
		 (depth_type.first == IMAGE_TYPE::FLOAT_32 && depth_type.second == IMAGE_CHANNEL::R))) {
		log_error("framebuffer depth type must either be NONE or FLOAT_32/R");
		return ret_fb;
	}
	if(!((stencil_type.first == IMAGE_TYPE::NONE && stencil_type.second == IMAGE_CHANNEL::NONE) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_8 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_16 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_32 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_64 && stencil_type.second == IMAGE_CHANNEL::R))) {
		log_error("framebuffer stencil type must either be NONE or UINT_*/R");
		return ret_fb;
	}
	
	// create all framebuffer images
	size_t img_idx = 0;
	for(const auto& img_type : image_types) {
		ret_fb.attach(img_idx, *new image(width, height, image::BACKING::BUFFER, img_type.first, img_type.second));
		img_idx++;
	}
	if(depth_type.first != IMAGE_TYPE::NONE) {
		ret_fb.attach_depth_buffer(*new image(width, height, image::BACKING::BUFFER, depth_type.first, depth_type.second));
	}
	if(stencil_type.first != IMAGE_TYPE::NONE) {
		ret_fb.attach_stencil_buffer(*new image(width, height, image::BACKING::BUFFER, stencil_type.first, stencil_type.second));
	}
	
	return ret_fb;
}

void framebuffer::destroy_images(framebuffer& fb) {
	for(const auto& img : fb.images) {
		if(img != nullptr) delete img;
	}
	fb.images.clear();
	if(fb.depth_buffer != nullptr) {
		delete fb.depth_buffer;
		fb.depth_buffer = nullptr;
	}
	if(fb.stencil_buffer != nullptr) {
		delete fb.stencil_buffer;
		fb.stencil_buffer = nullptr;
	}
}

framebuffer::framebuffer(const unsigned int& width, const unsigned int& height) : size(width, height) {
}

framebuffer::~framebuffer() {
}

framebuffer::framebuffer(framebuffer&& fb) noexcept : size(fb.size), images(fb.images), depth_buffer(fb.depth_buffer), stencil_buffer(fb.stencil_buffer) {
	fb.images.clear();
	fb.depth_buffer = nullptr;
	fb.stencil_buffer = nullptr;
}

framebuffer& framebuffer::operator=(framebuffer&& fb) noexcept {
	this->size = fb.size;
	this->images = std::move(fb.images);
	this->depth_buffer = fb.depth_buffer;
	this->stencil_buffer = fb.stencil_buffer;
	fb.depth_buffer = nullptr;
	fb.stencil_buffer = nullptr;
	return *this;
}

void framebuffer::set_size(const uint2& size_) {
	size = size_;
}

const uint2& framebuffer::get_size() const {
	return size;
}

const image* framebuffer::get_image(const size_t& index) const {
	return (index >= images.size() ? nullptr : images[index]);
}

image* framebuffer::get_image(const size_t& index) {
	return (index >= images.size() ? nullptr : images[index]);
}

const image* framebuffer::get_depth_buffer() const {
	return depth_buffer;
}

image* framebuffer::get_depth_buffer() {
	return depth_buffer;
}

const image* framebuffer::get_stencil_buffer() const {
	return stencil_buffer;
}

image* framebuffer::get_stencil_buffer() {
	return stencil_buffer;
}

void framebuffer::attach(const size_t& index, image& img) {
	if(index >= images.size()) {
		images.resize(index+1, nullptr);
	}
	images[index] = &img;
}

void framebuffer::detach(const size_t& index) {
	const size_t attachment_count = images.size();
#if defined(OCLRASTER_DEBUG)
	if(index >= attachment_count) {
		log_error("invalid index: %u - current framebuffer attachment count is only %u!", index, attachment_count);
		return;
	}
#endif
	images[index] = nullptr;
	
	// cleanup
	if(index+1 == attachment_count) {
		size_t new_count = 0;
		for(ssize_t idx = index-1; idx >= 0; idx--) {
			if(images[idx] != nullptr) {
				new_count = idx+1;
			}
		}
		images.resize(new_count);
		images.shrink_to_fit();
	}
}

size_t framebuffer::get_attachment_count() const {
	return images.size();
}

void framebuffer::attach_depth_buffer(image& img) {
	if(!((img.get_data_type() == IMAGE_TYPE::NONE && img.get_channel_order() == IMAGE_CHANNEL::NONE) ||
		 (img.get_data_type() == IMAGE_TYPE::FLOAT_32 && img.get_channel_order() == IMAGE_CHANNEL::R))) {
		log_error("framebuffer depth type must either be NONE or FLOAT_32/R");
		return;
	}
	depth_buffer = &img;
}
void framebuffer::detach_depth_buffer() {
	depth_buffer = nullptr;
}

void framebuffer::attach_stencil_buffer(image& img) {
	if(!((img.get_data_type() == IMAGE_TYPE::NONE && img.get_channel_order() == IMAGE_CHANNEL::NONE) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_8 && img.get_channel_order() == IMAGE_CHANNEL::R) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_16 && img.get_channel_order() == IMAGE_CHANNEL::R) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_32 && img.get_channel_order() == IMAGE_CHANNEL::R) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_64 && img.get_channel_order() == IMAGE_CHANNEL::R))) {
		log_error("framebuffer stencil type must either be NONE or UINT_*/R");
		return;
	}
	stencil_buffer = &img;
}
void framebuffer::detach_stencil_buffer() {
	stencil_buffer = nullptr;
}

void framebuffer::clear(const vector<size_t> image_indices, const bool depth_clear, const bool stencil_clear) const {
	const vector<size_t>* indices = &image_indices;
	vector<size_t> all_indices;
	if(image_indices.size() == 1 && image_indices[0] == ~0u) {
		// clear all
		const size_t img_count = images.size();
		all_indices.resize(img_count);
		for(size_t i = 0; i < img_count; i++) {
			all_indices[i] = i;
		}
		indices = &all_indices;
	}
	
	//
	framebuffer_program::image_spec spec;
	for(const auto& idx : *indices) {
		spec.images.emplace_back(images[idx]->get_image_type());
	}
	
	if(depth_clear && depth_buffer != nullptr) {
		spec.depth_image = depth_buffer->get_image_type();
	}
	
	if(stencil_clear && stencil_buffer != nullptr) {
		spec.stencil_image = stencil_buffer->get_image_type();
	}
	
	//
	unsigned int argc = 0;
	auto clear_kernel = framebuffer_program::get_clear_kernel(spec);
	ocl->use_kernel(clear_kernel.kernel);
	
	// framebuffer images/attachments
	for(const auto& idx : *indices) {
		ocl->set_kernel_argument(argc++, images[idx]->get_data_buffer());
	}
	if(depth_clear && depth_buffer != nullptr) {
		ocl->set_kernel_argument(argc++, depth_buffer->get_data_buffer());
	}
	if(stencil_clear && stencil_buffer != nullptr) {
		ocl->set_kernel_argument(argc++, stencil_buffer->get_data_buffer());
	}
	
	// type specific clear colors
	for(const auto& clear_type : clear_kernel.clear_image_types) {
		// for integer formats: AND the ulong4 clear color
		// for float formats: just convert/cast
		switch(clear_type) {
			case IMAGE_TYPE::INT_8:
			case IMAGE_TYPE::UINT_8:
				ocl->set_kernel_argument(argc++, uchar4 {
					clear_color_int.x & 0xFF,
					clear_color_int.y & 0xFF,
					clear_color_int.z & 0xFF,
					clear_color_int.w & 0xFF,
				});
				break;
			case IMAGE_TYPE::INT_16:
			case IMAGE_TYPE::UINT_16:
				ocl->set_kernel_argument(argc++, ushort4 {
					clear_color_int.x & 0xFFFF,
					clear_color_int.y & 0xFFFF,
					clear_color_int.z & 0xFFFF,
					clear_color_int.w & 0xFFFF,
				});
				break;
			case IMAGE_TYPE::INT_32:
			case IMAGE_TYPE::UINT_32:
				ocl->set_kernel_argument(argc++, uint4 {
					clear_color_int.x & 0xFFFFFFFF,
					clear_color_int.y & 0xFFFFFFFF,
					clear_color_int.z & 0xFFFFFFFF,
					clear_color_int.w & 0xFFFFFFFF,
				});
				break;
			case IMAGE_TYPE::FLOAT_16:
			case IMAGE_TYPE::FLOAT_32:
				ocl->set_kernel_argument(argc++, float4 { clear_color_float });
				break;
			case IMAGE_TYPE::INT_64:
			case IMAGE_TYPE::UINT_64:
				ocl->set_kernel_argument(argc++, clear_color_int);
				break;
			case IMAGE_TYPE::FLOAT_64:
				ocl->set_kernel_argument(argc++, clear_color_float);
				break;
			case IMAGE_TYPE::NONE:
			case IMAGE_TYPE::__MAX_TYPE:
				floor_unreachable();
		}
	}
	
	//
	ocl->set_kernel_argument(argc++, size);
	if(depth_clear && depth_buffer != nullptr) {
		ocl->set_kernel_argument(argc++, clear_depth);
	}
	if(stencil_clear && stencil_buffer != nullptr) {
		ocl->set_kernel_argument(argc++, clear_stencil);
	}
	
	//
	uint4 scissor_rectangle { 0u, 0u, ~0u, ~0u };
	const auto active_pipeline = oclraster::get_active_pipeline();
	if(active_pipeline != nullptr && active_pipeline->get_scissor_test()) {
		scissor_rectangle = active_pipeline->get_scissor_rectangle();
		const uint2 scissor_size { scissor_rectangle.z, scissor_rectangle.w };
		if(scissor_size.x == 0 || scissor_size.y == 0) return;
		scissor_rectangle.z += scissor_rectangle.x; // make absolute coordinates
		scissor_rectangle.w += scissor_rectangle.y;
		ocl->set_kernel_range(ocl->compute_kernel_ranges(std::min(size.x, scissor_size.x),
														 std::min(size.y, scissor_size.y)));
	}
	else {
		ocl->set_kernel_range(ocl->compute_kernel_ranges(size.x, size.y));
	}
	ocl->set_kernel_argument(argc++, scissor_rectangle);
	ocl->run_kernel();
}

void framebuffer::set_clear_color(const double4 value) {
	clear_color_int.set((unsigned long long int)value.x,
						(unsigned long long int)value.y,
						(unsigned long long int)value.z,
						(unsigned long long int)value.w);
	clear_color_float = value;
}

void framebuffer::set_clear_color_int(const ulong4 value) {
	clear_color_int = value;
}

void framebuffer::set_clear_color_float(const double4 value) {
	clear_color_float = value;
}

const ulong4& framebuffer::get_clear_color_int() const {
	return clear_color_int;
}

const double4& framebuffer::get_clear_color_float() const {
	return clear_color_float;
}

void framebuffer::set_clear_depth(const float value) {
	clear_depth = value;
}

const float& framebuffer::get_clear_depth() const {
	return clear_depth;
}

void framebuffer::set_clear_stencil(const unsigned long long int value) {
	clear_stencil = value;
}

const unsigned long long int& framebuffer::get_clear_stencil() const {
	return clear_stencil;
}
