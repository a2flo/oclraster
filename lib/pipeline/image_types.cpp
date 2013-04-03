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

#include "global.h"
#include "image_types.h"

static constexpr array<const char*, (size_t)IMAGE_TYPE::__MAX_TYPE> data_type_str_table {
	{
		"none",
		"char", "short", "int", "long",
		"uchar", "ushort", "uint", "ulong",
		"half", "float", "double"
	}
};

static constexpr array<const char*, (size_t)IMAGE_CHANNEL::__MAX_CHANNEL> channel_type_str_table {
	{ "", "", "2", "3", "4" }
};

string image_type::to_string() const {
	return ((native ? string("native_") : string("")) +
			data_type_str_table[(size_t)data_type] +
			channel_type_str_table[(size_t)channel_type]);
}

size_t image_type::pixel_size() const {
	const size_t channel_size = (size_t)channel_type;
	switch(data_type) {
		case IMAGE_TYPE::INT_8:
		case IMAGE_TYPE::UINT_8:
			return 1 * channel_size;
		case IMAGE_TYPE::INT_16:
		case IMAGE_TYPE::UINT_16:
		case IMAGE_TYPE::FLOAT_16:
			return 2 * channel_size;
		case IMAGE_TYPE::INT_32:
		case IMAGE_TYPE::UINT_32:
		case IMAGE_TYPE::FLOAT_32:
			return 4 * channel_size;
		case IMAGE_TYPE::INT_64:
		case IMAGE_TYPE::UINT_64:
		case IMAGE_TYPE::FLOAT_64:
			return 8 * channel_size;
		case IMAGE_TYPE::NONE:
		case IMAGE_TYPE::__MAX_TYPE:
			return 0;
	}
	oclr_unreachable();
}

string image_data_type_to_string(const IMAGE_TYPE& img_data_type) {
	return data_type_str_table[(size_t)img_data_type];
}

string image_channel_type_to_string(const IMAGE_CHANNEL& img_channel_type) {
	return channel_type_str_table[(size_t)img_channel_type];
}
