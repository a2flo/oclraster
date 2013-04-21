/*
 *  Flexible OpenCL Rasterizer (oclraster_support)
 *  Copyright (C) 2004 - 2013 Florian Ziesche
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

#include "gfx2d.h"

pipeline* gfx2d::oclr_pipeline = nullptr;

// size = #floats
static constexpr size_t primitive_buffer_size = 1024 * 16;
opencl::buffer_object* gfx2d::primitives_buffer = nullptr;
opencl::buffer_object* gfx2d::primitives_indices = nullptr;

//
void gfx2d::init(pipeline* p_) {
	oclr_pipeline = p_;
	
	//
	primitives_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
										   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
										   sizeof(float) * primitive_buffer_size);
	
	// indices are constant for all primitive types
	unsigned int* indices = new unsigned int[primitive_buffer_size];
	for(unsigned int i = 0; i < primitive_buffer_size; i++) {
		indices[i] = i;
	}
	primitives_indices = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
											opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
											opencl::BUFFER_FLAG::INITIAL_COPY,
											sizeof(unsigned int) * primitive_buffer_size,
											indices);
	delete [] indices;
}

void gfx2d::destroy() {
	if(primitives_buffer != nullptr) {
		ocl->delete_buffer(primitives_buffer);
	}
	if(primitives_indices != nullptr) {
		ocl->delete_buffer(primitives_indices);
	}
}

void gfx2d::compute_ellipsoid_points(vector<primitive_properties::primitive_point>& dst_points,
									 const float& radius_lr, const float& radius_tb,
									 const float& start_angle, const float& end_angle) {
	//
	const float angle_size = (end_angle >= start_angle ? (end_angle - start_angle) : (360.0f + end_angle - start_angle)) / 360.0f;
	const float steps_per_quadrant_lr = ceilf(radius_lr); // "per 90° or 0.25 angle size"
	const float steps_per_quadrant_tb = ceilf(radius_tb);
	const float angle_offset = (start_angle / 360.0f) * (2.0f * PI);
	const size_t steps = (size_t)(std::max(steps_per_quadrant_lr, steps_per_quadrant_tb) * (angle_size * 4.0f));
	const float fsteps = steps-1;
	
	dst_points.reserve(dst_points.size() + steps);
	for(size_t i = 0; i < steps; i++) {
		const float fstep = ((float(i) / fsteps) * angle_size) * (2.0f * PI);
		const float sin_step = sinf(angle_offset + fstep);
		const float cos_step = cosf(angle_offset + fstep);
		dst_points.emplace_back(float2(sin_step * radius_lr, cos_step * radius_tb));
	}
}

void gfx2d::upload_points_and_draw(const primitive_properties& props) {
	// TODO: fix this
	if(props.points.size()*4 > primitive_buffer_size) {
		oclr_error("too many primitives!");
		return;
	}
	/*cout << "points: " << endl;
	for(const auto& point : props.points) {
		cout << point << " ";
	}
	cout << endl;*/
	
	//
	const unsigned int vertex_count = (unsigned int)props.points.size();
	unsigned int primitive_count = 0;
	switch(props.primitive_type) {
		case PRIMITIVE_TYPE::TRIANGLE:
			primitive_count = vertex_count / 3;
			break;
		case PRIMITIVE_TYPE::TRIANGLE_STRIP:
		case PRIMITIVE_TYPE::TRIANGLE_FAN:
			primitive_count = vertex_count - 2;
			break;
	}
	
	ocl->write_buffer(primitives_buffer, &props.points[0],
					  0, sizeof(decltype(props.points)::value_type) * props.points.size());
	
	oclr_pipeline->bind_buffer("index_buffer", *primitives_indices);
	oclr_pipeline->bind_buffer("input_attributes", *primitives_buffer);
	
	oclr_pipeline->draw(props.primitive_type, vertex_count, { 0, primitive_count });
}

/*! returns true if point is in rectangle
 *  @param rectangle the rectangle
 *  @param point the point we want to test
 */
bool gfx2d::is_pnt_in_rectangle(const rect& rectangle, const pnt& point) {
	if(point.x >= rectangle.x1 && point.x <= rectangle.x2 &&
	   point.y >= rectangle.y1 && point.y <= rectangle.y2) {
		return true;
	}
	return false;
}

/*! returns true if point is in rectangle
 *  @param rectangle the rectangle
 *  @param point the point we want to test
 */
bool gfx2d::is_pnt_in_rectangle(const rect& rectangle, const ipnt& point) {
	if(point.x < 0 || point.y < 0) return false;
	
	if(point.x >= (int)rectangle.x1 && point.x <= (int)rectangle.x2 &&
	   point.y >= (int)rectangle.y1 && point.y <= (int)rectangle.y2) {
		return true;
	}
	return false;
}
