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

// joined <IMAGE_CHANNEL, IMAGE_TYPE>
// note: a value of 0 is reserved for "no/unspecified type"
typedef unsigned int image_type;
constexpr image_type make_image_type(const IMAGE_TYPE& type, const IMAGE_CHANNEL& channel) {
	return (((unsigned int)channel) << 16u) + ((unsigned int)type);
}
constexpr IMAGE_TYPE get_image_data_type(const image_type& img_type) {
	return (IMAGE_TYPE)(img_type & 0xFFFFu);
}
constexpr IMAGE_CHANNEL get_image_channel_type(const image_type& img_type) {
	return (IMAGE_CHANNEL)((img_type >> 16u) & 0xFFFFu);
}
extern string image_type_to_string(const image_type& img_type);

// TODO: user defined types

#endif
