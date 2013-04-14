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

#ifndef __OCLRASTER_IMAGE_TYPES_H__
#define __OCLRASTER_IMAGE_TYPES_H__

// this directly maps to opencl types (6.1.1)
enum class IMAGE_TYPE : unsigned short int {
	NONE,		//!< only used internally!
	INT_8,		//!< char
	INT_16,		//!< short
	INT_32,		//!< int
	INT_64,		//!< long
	UINT_8,		//!< uchar
	UINT_16,	//!< ushort
	UINT_32,	//!< uint
	UINT_64,	//!< ulong
	FLOAT_16,	//!< half
	FLOAT_32,	//!< float
	FLOAT_64,	//!< double (note: must be supported by the device)
	__MAX_TYPE
};

enum class IMAGE_CHANNEL : unsigned short int {
	NONE,		//!< only used internally!
	R,
	RG,
	RGB,
	RGBA,
	__MAX_CHANNEL
};

//
struct __attribute__((packed)) image_type {
	IMAGE_TYPE data_type;
	IMAGE_CHANNEL channel_type;
	bool native;
	
	constexpr image_type() noexcept :
	data_type(IMAGE_TYPE::NONE), channel_type(IMAGE_CHANNEL::NONE), native(false) {}
	
	constexpr image_type(const IMAGE_TYPE& data_type_, const IMAGE_CHANNEL& channel_type_, const bool native_ = false) noexcept :
	data_type(data_type_), channel_type(channel_type_), native(native_) {}
	
	bool is_valid() const {
		return (data_type != IMAGE_TYPE::NONE && channel_type != IMAGE_CHANNEL::NONE);
	}
	
	bool operator==(const image_type& img_type) const {
		return (data_type == img_type.data_type &&
				channel_type == img_type.channel_type &&
				native == img_type.native);
	}
	bool operator!=(const image_type& img_type) const {
		return !(*this == img_type);
	}
	
	size_t pixel_size() const;
	
	string to_string(const bool print_native = true) const;
	friend ostream& operator<<(ostream& output, const image_type& img_type) {
		output << img_type.to_string();
		return output;
	}
};
extern string image_data_type_to_string(const IMAGE_TYPE& img_data_type);
extern string image_channel_type_to_string(const IMAGE_CHANNEL& img_channel_type);

// TODO: user defined types

#endif
