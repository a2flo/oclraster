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

#include "image.hpp"
#include "oclraster.hpp"

// 2d array: [IMAGE_TYPE][IMAGE_CHANNEL] -> cl::ImageFormat (-> will be (0, 0) if not supported)
array<array<cl::ImageFormat, (size_t)IMAGE_CHANNEL::__MAX_CHANNEL>, (size_t)IMAGE_TYPE::__MAX_TYPE> image::internal_image_format_mapping;

//
bool is_correct_format(const SDL_PixelFormat& format, const IMAGE_CHANNEL& channel_order);
bool is_correct_format(const SDL_PixelFormat& format, const IMAGE_CHANNEL& channel_order) {
	switch(channel_order) {
#if defined(SDL_LIL_ENDIAN)
		case IMAGE_CHANNEL::R:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0 ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case IMAGE_CHANNEL::RG:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case IMAGE_CHANNEL::RGB:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0xFF0000 ||
			   format.Amask != 0) return false;
			break;
		case IMAGE_CHANNEL::RGBA:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0xFF0000 ||
			   format.Amask != 0xFF000000) return false;
			break;
#elif defined(SDL_BIG_ENDIAN)
		case IMAGE_CHANNEL::R:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0 ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case IMAGE_CHANNEL::RG:
			if(format.Rmask != 0xFF00 ||
			   format.Gmask != 0xFF ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case IMAGE_CHANNEL::RGB:
			if(format.Rmask != 0xFF0000 ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0xFF ||
			   format.Amask != 0) return false;
			break;
		case IMAGE_CHANNEL::RGBA:
			if(format.Rmask != 0xFF000000 ||
			   format.Gmask != 0xFF0000 ||
			   format.Bmask != 0xFF00 ||
			   format.Amask != 0xFF) return false;
			break;
#else
#error "unknown endianness"
#endif
		case IMAGE_CHANNEL::NONE:
		case IMAGE_CHANNEL::__MAX_CHANNEL:
			floor_unreachable();
	}
	return true;
}

image image::from_file(const string& filename, const BACKING& backing,
					   const IMAGE_TYPE& type, const IMAGE_CHANNEL& channel_order) {
	const auto fail_return = [&filename, &backing](const string& error_msg) -> image {
		log_error("%s (\"%s\"): %s!", error_msg, filename, SDL_GetError());
		const unsigned int fail_pixel = 0xDEADBEEF;
		auto img = image(1, 1, backing, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA, &fail_pixel);
		img.invalidate();
		return img;
	};
	if(type >= IMAGE_TYPE::__MAX_TYPE) return fail_return("invalid image type");
	if(channel_order >= IMAGE_CHANNEL::__MAX_CHANNEL) return fail_return("invalid channel type");
	
	//
	SDL_Surface* surface = IMG_Load(filename.c_str());
	if(surface == nullptr) {
		return fail_return("failed to load image");
	}
	
	// check if the loaded surface must be converted to match the requested channel order (format)
	// note that this will only work for INT_8 and UINT_8 images (SDL only supports these directly)
	const size_t pixel_size = image_type(type, channel_order).pixel_size();
	SDL_PixelFormat* format = surface->format;
	if((type == IMAGE_TYPE::INT_8 || type == IMAGE_TYPE::UINT_8) &&
	   (format->BytesPerPixel != pixel_size ||
		!is_correct_format(*format, channel_order))) {
		SDL_PixelFormat correct_format;
		memcpy(&correct_format, format, sizeof(SDL_PixelFormat));
		
		//
		correct_format.BytesPerPixel = pixel_size;
		correct_format.BitsPerPixel = pixel_size * 8;
		switch(channel_order) {
#if defined(SDL_LIL_ENDIAN)
			case IMAGE_CHANNEL::R:
				correct_format.Gshift = 0;
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Gmask = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				break;
			case IMAGE_CHANNEL::RG:
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				correct_format.Gmask = 0xFF00;
				correct_format.Gshift = 8;
				break;
			case IMAGE_CHANNEL::RGB:
				correct_format.Ashift = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				correct_format.Gmask = 0xFF00;
				correct_format.Gshift = 8;
				correct_format.Bmask = 0xFF0000;
				correct_format.Bshift = 16;
				break;
			case IMAGE_CHANNEL::RGBA:
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				correct_format.Gmask = 0xFF00;
				correct_format.Gshift = 8;
				correct_format.Bmask = 0xFF0000;
				correct_format.Bshift = 16;
				correct_format.Amask = 0xFF000000;
				correct_format.Ashift = 24;
				break;
#elif defined(SDL_BIG_ENDIAN)
			case IMAGE_CHANNEL::R:
				correct_format.Gshift = 0;
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Gmask = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				break;
			case IMAGE_CHANNEL::RG:
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF00;
				correct_format.Rshift = 8;
				correct_format.Gmask = 0xFF;
				correct_format.Gshift = 0;
				break;
			case IMAGE_CHANNEL::RGB:
				correct_format.Ashift = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF0000;
				correct_format.Rshift = 16;
				correct_format.Gmask = 0xFF00;
				correct_format.Gshift = 8;
				correct_format.Bmask = 0xFF;
				correct_format.Bshift = 0;
				break;
			case IMAGE_CHANNEL::RGBA:
				correct_format.Rmask = 0xFF000000;
				correct_format.Rshift = 24;
				correct_format.Gmask = 0xFF0000;
				correct_format.Gshift = 16;
				correct_format.Bmask = 0xFF00;
				correct_format.Bshift = 8;
				correct_format.Amask = 0xFF;
				correct_format.Ashift = 0;
				break;
#else
#error "unknown endianness"
#endif
			case IMAGE_CHANNEL::NONE:
			case IMAGE_CHANNEL::__MAX_CHANNEL:
				floor_unreachable();
		}
		
		SDL_Surface* converted_surface = SDL_ConvertSurface(surface, &correct_format, 0);
		if(converted_surface == nullptr) {
			return fail_return("failed to convert image to correct format");
		}
		SDL_FreeSurface(surface);
		surface = converted_surface;
	}
	else if(type != IMAGE_TYPE::INT_8 && type != IMAGE_TYPE::UINT_8) {
		return fail_return("automatic conversion to image types != INT_8 or UINT_8 not supported");
	}
	
	image img(surface->w, surface->h, backing, type, channel_order, surface->pixels);
	SDL_FreeSurface(surface);
	return img;
}

image::image(const unsigned int& width, const unsigned int& height,
			 const BACKING& backing_,
			 const IMAGE_TYPE& type,
			 const IMAGE_CHANNEL& channel_order_,
			 const void* pixels) :
backing(backing_), img_type(type, channel_order_), data_type(type), channel_order(channel_order_), size(width, height), native_format(0, 0) {
	create_buffer(pixels);
}

image::image(const unsigned int& width, const unsigned int& height,
			 const IMAGE_TYPE& type,
			 const IMAGE_CHANNEL& channel_order_,
			 const cl::ImageFormat& native_format_,
			 const void* pixels) :
backing(BACKING::IMAGE), img_type(type, channel_order_), data_type(type), channel_order(channel_order_), size(width, height), native_format(native_format_) {
	create_buffer(pixels);
}

void image::create_buffer(const void* pixels) {
#if defined(OCLRASTER_DEBUG)
	if(data_type >= IMAGE_TYPE::__MAX_TYPE) {
		log_error("invalid image type: %u!", data_type);
		invalidate();
		return;
	}
	if(channel_order >= IMAGE_CHANNEL::__MAX_CHANNEL) {
		log_error("invalid image channel order type: %u!", channel_order);
		invalidate();
		return;
	}
#endif
	
	if(backing == BACKING::IMAGE) {
		// if constructed with a native format, check if the specified format is supported
		if(native_format.image_channel_data_type != 0 ||
		   native_format.image_channel_order != 0) {
			bool found = false;
			for(const auto& format : ocl->get_image_formats()) {
				if(format.image_channel_order == native_format.image_channel_order &&
				   format.image_channel_data_type == native_format.image_channel_data_type) {
					found = true;
					break;
				}
			}
			if(!found) {
				// not supported, reset and look for a compatible one
				log_error("specified native image format (%X %X) not supported - checking for compatible image format ...",
						   native_format.image_channel_data_type, native_format.image_channel_order);
				native_format.image_channel_data_type = 0;
				native_format.image_channel_order = 0;
			}
		}
		
		// look for a supported/compatible image format
		if(native_format.image_channel_data_type == 0 ||
		   native_format.image_channel_order == 0) {
			native_format = get_image_format(data_type, channel_order);
		}
		if(native_format.image_channel_data_type == 0 ||
		   native_format.image_channel_order == 0) {
			log_error("image format \"%s\" is not natively supported - falling back to buffer based image backing!",
					   image_type { data_type, channel_order }.to_string());
			backing = BACKING::BUFFER;
		}
	}
	
	if(backing == BACKING::BUFFER) {
		img_type.native = false;
		const size_t pixel_size = img_type.pixel_size();
		const size_t data_size = size.x * size.y * pixel_size;
		const size_t buffer_size = header_size() + data_size;
		
		auto buffer_ptrs = ocl->create_and_map_buffer(opencl::BUFFER_FLAG::READ_WRITE |
													  opencl::BUFFER_FLAG::BLOCK_ON_READ |
													  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
													  buffer_size,
													  nullptr,
													  opencl::MAP_BUFFER_FLAG::WRITE_INVALIDATE |
													  opencl::MAP_BUFFER_FLAG::BLOCK,
													  0,
													  pixels == nullptr ? header_size() : buffer_size);
		buffer = buffer_ptrs.first;
		auto mapped_ptr = buffer_ptrs.second;
		
		// init buffer
		header* header_ptr = (header*)mapped_ptr;
		header_ptr->type = data_type;
		header_ptr->channel_order = channel_order;
		header_ptr->width = size.x;
		header_ptr->height = size.y;
		
		// fill buffer with the specified pixels (otherwise, leave it uninitialized)
		if(pixels != nullptr) {
			unsigned char* data_ptr = (unsigned char*)mapped_ptr + header_size();
			copy_n((const unsigned char*)pixels, data_size, data_ptr);
		}
		ocl->unmap_buffer(buffer, mapped_ptr);
		
		data_buffer = ocl->create_sub_buffer(buffer,
											 opencl::BUFFER_FLAG::READ_WRITE |
											 opencl::BUFFER_FLAG::BLOCK_ON_READ |
											 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
											 header_size(), data_size);
	}
	else {
		img_type.native = true;
		buffer = ocl->create_image2d_buffer(opencl::BUFFER_FLAG::READ_WRITE |
											opencl::BUFFER_FLAG::BLOCK_ON_READ |
											opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
											(pixels != nullptr ? opencl::BUFFER_FLAG::INITIAL_COPY : opencl::BUFFER_FLAG::NONE),
											native_format.image_channel_order, native_format.image_channel_data_type,
											size.x, size.y, (void*)pixels);
		if(buffer->image_buffer == nullptr) {
			log_error("image buffer creation failed!");
			invalidate();
			return;
		}
	}
	
	// image creation was successful -> set valid state
	valid = true;
}

image::~image() {
	if(buffer != nullptr && ocl != nullptr) {
		if(data_buffer != nullptr) {
			ocl->delete_buffer(data_buffer);
		}
		ocl->delete_buffer(buffer);
	}
}

image::image(image&& img) noexcept :
backing(img.backing), img_type(img.img_type), data_type(img.data_type), channel_order(img.channel_order),
size(img.size), buffer(img.buffer), native_format(img.native_format) {
	img.invalidate();
	img.buffer = nullptr;
}

image::BACKING image::get_backing() const {
	return backing;
}

const opencl::buffer_object* image::get_buffer() const {
	return buffer;
}

opencl::buffer_object* image::get_buffer() {
	return buffer;
}

const opencl::buffer_object* image::get_data_buffer() const {
	return data_buffer;
}

opencl::buffer_object* image::get_data_buffer() {
	return data_buffer;
}

image_type image::get_image_type() const {
	return img_type;
}

IMAGE_TYPE image::get_data_type() const {
	return data_type;
}

IMAGE_CHANNEL image::get_channel_order() const {
	return channel_order;
}

const uint2& image::get_size() const {
	return size;
}

const cl::ImageFormat& image::get_native_format() const {
	return native_format;
}

void image::write(const void* src, const uint2 offset, const uint2 size_) {
	const size3 write_size {
		(size_.x == ~0u ? size.x : std::min(size.x, size_.x)),
		(size_.y == ~0u ? size.y : std::min(size.y, size_.y)),
		1
	};
	if(backing == BACKING::BUFFER) {
		ocl->write_buffer_rect(data_buffer, src, size3(offset.x, offset.y, 0), size3(0, 0, 0), write_size);
	}
	else {
		ocl->write_image(buffer, src, size3(offset.x, offset.y, 0), write_size);
	}
}

void image::read(void* dst, const uint2 offset, const uint2 size_) {
	const size3 read_size {
		(size_.x == ~0u ? size.x : std::min(size.x, size_.x)),
		(size_.y == ~0u ? size.y : std::min(size.y, size_.y)),
		1
	};
	if(backing == BACKING::BUFFER) {
		ocl->read_buffer_rect(dst, data_buffer, size3(offset.x, offset.y, 0), size3(0, 0, 0), read_size);
	}
	else {
		ocl->read_image(dst, buffer, size3(offset.x, offset.y, 0), read_size);
	}
}

void image::copy(const image& src_img, const uint2 src_offset, const uint2 dst_offset, const uint2 size_) {
	const size3 copy_src_offset(src_offset.x, src_offset.y, 0);
	const size3 copy_dst_offset(dst_offset.x, dst_offset.y, 0);
	const size3 copy_region(size_.x == ~0u ? size.x : std::min(size.x, size_.x),
							size_.y == ~0u ? size.y : std::min(size.y, size_.y),
							1);
	const size_t pixel_size = img_type.pixel_size();
	if(backing == BACKING::BUFFER) {
		if(src_img.get_backing() == BACKING::BUFFER) {
			// buffer -> buffer
			ocl->copy_buffer_rect(src_img.get_data_buffer(), data_buffer, copy_src_offset, copy_dst_offset, copy_region);
		}
		else {
			// image -> buffer
			// since opencl has no clEnqueueCopyImageToBufferRect function, the data needs to copied to a temporary buffer object
			// and then copied to this buffer object, if the dst x-offset is any other than 0, or the copy x-region is not equal to the image width
			if(dst_offset.x == 0 && copy_region.x == size.x) {
				ocl->copy_image_to_buffer(src_img.get_buffer(), data_buffer,
										  copy_src_offset, copy_region,
										  pixel_size * dst_offset.y * size.x);
			}
			else {
				opencl::buffer_object* copy_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
																		opencl::BUFFER_FLAG::BLOCK_ON_READ |
																		opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																		pixel_size * copy_region.x * copy_region.y, nullptr);
				ocl->copy_image_to_buffer(src_img.get_buffer(), copy_buffer, copy_src_offset, copy_region, 0);
				ocl->copy_buffer_rect(copy_buffer, data_buffer, size3(0, 0, 0), copy_dst_offset, copy_region);
				ocl->delete_buffer(copy_buffer);
			}
		}
	}
	else {
		if(src_img.get_backing() == BACKING::IMAGE) {
			// image -> image
			ocl->copy_image(src_img.get_buffer(), buffer, copy_src_offset, copy_dst_offset, copy_region);
		}
		else {
			// buffer -> image
			// since opencl has no clEnqueueCopyBufferRectToImage function, the data needs to copied to a temporary buffer object
			// and then copied to this buffer object, if the src x-offset is any other than 0, or the copy x-region is not equal to the image width
			if(src_offset.x == 0 && copy_region.x == size.x) {
				ocl->copy_buffer_to_image(src_img.get_buffer(), data_buffer,
										  pixel_size * src_offset.y * size.x,
										  copy_dst_offset, copy_region);
			}
			else {
				opencl::buffer_object* copy_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
																		opencl::BUFFER_FLAG::BLOCK_ON_READ |
																		opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																		pixel_size * copy_region.x * copy_region.y, nullptr);
				ocl->copy_buffer_rect(src_img.get_data_buffer(), copy_buffer, copy_src_offset, size3(0, 0, 0), copy_region);
				ocl->copy_buffer_to_image(copy_buffer, buffer, 0, copy_dst_offset, copy_region);
				ocl->delete_buffer(copy_buffer);
			}
		}
	}
}

void* __attribute__((aligned(128))) image::map(const opencl::MAP_BUFFER_FLAG access_type) {
	if(backing == BACKING::BUFFER) {
		return ocl->map_buffer(data_buffer, access_type);
	}
	else {
		return ocl->map_image(buffer, access_type);
	}
}

const void* __attribute__((aligned(128))) image::map(const opencl::MAP_BUFFER_FLAG access_type) const {
#if defined(OCLRASTER_DEBUG)
	if((access_type & opencl::MAP_BUFFER_FLAG::READ_WRITE) == opencl::MAP_BUFFER_FLAG::READ_WRITE ||
	   (access_type & opencl::MAP_BUFFER_FLAG::WRITE) == opencl::MAP_BUFFER_FLAG::WRITE ||
	   (access_type & opencl::MAP_BUFFER_FLAG::WRITE_INVALIDATE) == opencl::MAP_BUFFER_FLAG::WRITE_INVALIDATE) {
		log_error("access-type must be opencl::MAP_BUFFER_FLAG::READ when using the const map function!");
		return nullptr;
	}
#endif
	
	if(backing == BACKING::BUFFER) {
		return ocl->map_buffer(data_buffer, access_type);
	}
	else {
		return ocl->map_image(buffer, access_type);
	}
}

void* __attribute__((aligned(128))) image::map_region(const uint2 offset, const uint2 size_, const opencl::MAP_BUFFER_FLAG access_type) {
	const size3 map_size {
		(size_.x == ~0u ? size.x : std::min(size.x, size_.x)),
		(size_.y == ~0u ? size.y : std::min(size.y, size_.y)),
		1
	};
	if(backing == BACKING::BUFFER) {
		// mapping a region with a width other than the images width is not possible when buffer based backing is used,
		// because there is no way to specify a row offset when mapping a buffer (+then, there wouldn't be a way to offset by the header size)
		if(offset.x != 0) {
			log_error("map x-offset must be 0 when BUFFER backing is used!");
			return nullptr;
		}
		if(map_size.x != size.x) {
			log_error("map x-size must match image x-size when BUFFER backing is used!");
			return nullptr;
		}
		
		return ocl->map_buffer(data_buffer, access_type,
							   img_type.pixel_size() * offset.y * size.x,
							   map_size.y * size.x);
	}
	else {
		return ocl->map_image(buffer, access_type, size3(offset.x, offset.y, 0), map_size);
	}
}

void image::unmap(const void* mapped_ptr) const {
	if(backing == BACKING::BUFFER) {
		ocl->unmap_buffer(data_buffer, (void*)mapped_ptr);
	}
	else {
		ocl->unmap_buffer(buffer, (void*)mapped_ptr);
	}
}

bool image::modify_backing(const BACKING& new_backing) {
	if(backing == new_backing) return true;
	
	const BACKING old_backing = backing;
	opencl::buffer_object* old_buffer = buffer;
	opencl::buffer_object* old_data_buffer = data_buffer;
	
	backing = new_backing;
	create_buffer(nullptr); // init pixel == nullptr, because we'll do a device-side copy later
	
	// this can only happen, if IMAGE backing should be used, but it's falling back to BUFFER backing
	if(old_backing == backing) {
		log_error("failed to modify image backing!");
		return false;
	}
	
	// copy image data from the old buffer to the new buffer
	if(old_backing == BACKING::BUFFER) {
		ocl->copy_buffer_to_image(old_data_buffer, buffer);
	}
	else {
		ocl->copy_image_to_buffer(old_buffer, data_buffer);
	}
	
	// cleanup
	if(old_backing == BACKING::BUFFER) {
		ocl->delete_buffer(old_data_buffer); // no longer needed
		data_buffer = nullptr;
	}
	ocl->delete_buffer(old_buffer);
	
	return true;
}

void image::invalidate() {
	valid = false;
}

bool image::is_valid() const {
	return valid;
}

cl::ImageFormat image::get_image_format(const IMAGE_TYPE& img_data_type, const IMAGE_CHANNEL img_channel_type) const {
	const auto data_idx = (typename underlying_type<IMAGE_TYPE>::type)img_data_type;
	const auto channel_idx = (typename underlying_type<IMAGE_CHANNEL>::type)img_channel_type;
#if defined(OCLRASTER_DEBUG)
	if(data_idx >= internal_image_format_mapping.size()) {
		log_error("invalid data_type: %u!", data_idx);
		return cl::ImageFormat(0, 0);
	}
	if(channel_idx >= internal_image_format_mapping[data_idx].size()) {
		log_error("invalid channel_type: %u!", channel_idx);
		return cl::ImageFormat(0, 0);
	}
#endif
	return internal_image_format_mapping[data_idx][channel_idx];
}
