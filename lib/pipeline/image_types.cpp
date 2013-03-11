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
		"",
		"char", "short", "int", "long",
		"uchar", "ushort", "uint", "ulong",
		"half", "float", "double"
	}
};

static constexpr array<const char*, (size_t)IMAGE_CHANNEL::__MAX_CHANNEL> channel_type_str_table {
	{ "", "", "2", "3", "4" }
};

string image_type_to_string(const image_type& img_type) {
	return (string(data_type_str_table[(size_t)get_image_data_type(img_type)]) +
			channel_type_str_table[(size_t)get_image_channel_type(img_type)]);
}

string image_data_type_to_string(const IMAGE_TYPE& img_data_type) {
	return data_type_str_table[(size_t)img_data_type];
}

string image_channel_type_to_string(const IMAGE_CHANNEL& img_channel_type) {
	return channel_type_str_table[(size_t)img_channel_type];
}
