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

#include "image.h"
#include "oclraster.h"

static constexpr array<size_t, (size_t)image::TYPE::__MAX_TYPE> image_type_sizes {
	{
		1, 2, 4, 8, // INT*
		1, 2, 4, 8, // UINT*
		2, 4, 8, // FLOAT*
	}
};
static constexpr array<size_t, (size_t)image::CHANNEL::__MAX_CHANNEL> image_channel_sizes {
	{
		1, 2, 3, 4, // R/RG/RGB/RGBA
	}
};

bool is_correct_format(const SDL_PixelFormat& format, const image::CHANNEL& channel_order);
bool is_correct_format(const SDL_PixelFormat& format, const image::CHANNEL& channel_order) {
	switch(channel_order) {
#if defined(SDL_LIL_ENDIAN)
		case image::CHANNEL::R:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0 ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case image::CHANNEL::RG:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case image::CHANNEL::RGB:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0xFF0000 ||
			   format.Amask != 0) return false;
			break;
		case image::CHANNEL::RGBA:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0xFF0000 ||
			   format.Amask != 0xFF000000) return false;
			break;
#elif defined(SDL_BIG_ENDIAN)
		case image::CHANNEL::R:
			if(format.Rmask != 0xFF ||
			   format.Gmask != 0 ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case image::CHANNEL::RG:
			if(format.Rmask != 0xFF00 ||
			   format.Gmask != 0xFF ||
			   format.Bmask != 0 ||
			   format.Amask != 0) return false;
			break;
		case image::CHANNEL::RGB:
			if(format.Rmask != 0xFF0000 ||
			   format.Gmask != 0xFF00 ||
			   format.Bmask != 0xFF ||
			   format.Amask != 0) return false;
			break;
		case image::CHANNEL::RGBA:
			if(format.Rmask != 0xFF000000 ||
			   format.Gmask != 0xFF0000 ||
			   format.Bmask != 0xFF00 ||
			   format.Amask != 0xFF) return false;
			break;
#else
#error "unknown endianness"
#endif
		case image::CHANNEL::__MAX_CHANNEL:
			oclr_unreachable();
	}
	return true;
}

image image::from_file(const string& filename,
					   const TYPE& type, const CHANNEL& channel_order) {
	const auto fail_return = [&filename](const string& error_msg) -> image {
		oclr_error("%s (\"%s\"): %s!", error_msg, filename, SDL_GetError());
		const unsigned int fail_pixel = 0xDEADBEEF;
		return image(1, 1, image::TYPE::UINT_8, image::CHANNEL::RGBA, &fail_pixel);
	};
	if(type >= TYPE::__MAX_TYPE) return fail_return("invalid image type");
	if(channel_order >= CHANNEL::__MAX_CHANNEL) return fail_return("invalid channel type");
	
	//
	SDL_Surface* surface = IMG_Load(filename.c_str());
	if(surface == nullptr) {
		return fail_return("failed to load image");
	}
	
	// check if the loaded surface must be converted to match the requested channel order (format)
	// note that this will only work for INT_8 and UINT_8 images (SDL only supports these directly)
	SDL_PixelFormat* format = surface->format;
	const size_t channel_count = image_channel_sizes[static_cast<underlying_type<image::CHANNEL>::type>(channel_order)];
	const size_t type_size = image_type_sizes[static_cast<underlying_type<image::TYPE>::type>(type)];
	if((type == TYPE::INT_8 || type == TYPE::UINT_8) &&
	   (format->BytesPerPixel != (channel_count * type_size) ||
		!is_correct_format(*format, channel_order))) {
		SDL_PixelFormat correct_format;
		memcpy(&correct_format, format, sizeof(SDL_PixelFormat));
		
		//
		correct_format.BytesPerPixel = channel_count * type_size;
		correct_format.BitsPerPixel = channel_count * type_size * 8;
		switch(channel_order) {
#if defined(SDL_LIL_ENDIAN)
			case CHANNEL::R:
				correct_format.Gshift = 0;
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Gmask = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				break;
			case CHANNEL::RG:
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				correct_format.Gmask = 0xFF00;
				correct_format.Gshift = 8;
				break;
			case CHANNEL::RGB:
				correct_format.Ashift = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				correct_format.Gmask = 0xFF00;
				correct_format.Gshift = 8;
				correct_format.Bmask = 0xFF0000;
				correct_format.Bshift = 16;
				break;
			case CHANNEL::RGBA:
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
			case CHANNEL::R:
				correct_format.Gshift = 0;
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Gmask = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF;
				correct_format.Rshift = 0;
				break;
			case CHANNEL::RG:
				correct_format.Bshift = 0;
				correct_format.Ashift = 0;
				correct_format.Bmask = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF00;
				correct_format.Rshift = 8;
				correct_format.Gmask = 0xFF;
				correct_format.Gshift = 0;
				break;
			case CHANNEL::RGB:
				correct_format.Ashift = 0;
				correct_format.Amask = 0;
				
				correct_format.Rmask = 0xFF0000;
				correct_format.Rshift = 16;
				correct_format.Gmask = 0xFF00;
				correct_format.Gshift = 8;
				correct_format.Bmask = 0xFF;
				correct_format.Bshift = 0;
				break;
			case CHANNEL::RGBA:
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
			case CHANNEL::__MAX_CHANNEL:
				oclr_unreachable();
		}
		
		SDL_Surface* converted_surface = SDL_ConvertSurface(surface, &correct_format, 0);
		if(converted_surface == nullptr) {
			return fail_return("failed to convert image to correct format");
		}
		SDL_FreeSurface(surface);
		surface = converted_surface;
	}
	else if(type != TYPE::INT_8 && type != TYPE::UINT_8) {
		return fail_return("automatic conversion to image types != INT_8 or UINT_8 not supported");
	}
	
	image img(surface->w, surface->h, type, channel_order, surface->pixels);
	SDL_FreeSurface(surface);
	return img;
}

image::image(const unsigned int& width, const unsigned int& height,
			 const TYPE& type, const CHANNEL& channel_order,
			 const void* pixels) {
#if defined(OCLRASTER_DEBUG)
	if(type >= TYPE::__MAX_TYPE) {
		oclr_error("invalid image type: %u!", type);
		return;
	}
	if(channel_order >= CHANNEL::__MAX_CHANNEL) {
		oclr_error("invalid image channel order type: %u!", channel_order);
		return;
	}
#endif
	
	const size_t data_size = (width * height *
							  image_type_sizes[static_cast<underlying_type<image::TYPE>::type>(type)] *
							  image_channel_sizes[static_cast<underlying_type<image::CHANNEL>::type>(channel_order)]);
	const size_t size = header_size() + data_size;
	buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
								opencl::BUFFER_FLAG::BLOCK_ON_READ |
								opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
								size);
	
	// init buffer ...
	auto mapped_ptr = ocl->map_buffer(buffer,
									  opencl::MAP_BUFFER_FLAG::WRITE_INVALIDATE |
									  opencl::MAP_BUFFER_FLAG::BLOCK);
	
	header* header_ptr = (header*)mapped_ptr;
	header_ptr->type = type;
	header_ptr->channel_order = channel_order;
	header_ptr->width = width;
	header_ptr->height = height;
	
	unsigned char* data_ptr = (unsigned char*)mapped_ptr + header_size();
	if(pixels == nullptr) {
		// ... with 0s
		fill_n(data_ptr, data_size, 0);
	}
	else {
		// ... with the specified pixels
		copy_n((const unsigned char*)pixels, data_size, data_ptr);
	}
	ocl->unmap_buffer(buffer, mapped_ptr);
}

image::image(image&& img) : backing(img.backing), buffer(img.buffer) {
	img.buffer = nullptr;
}

image::~image() {
	if(buffer != nullptr && ocl != nullptr) {
		ocl->delete_buffer(buffer);
	}
}

image::BACKING image::get_backing() const {
	return backing;
}

const opencl::buffer_object* image::get_buffer() const {
	return buffer;
}
