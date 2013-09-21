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

#ifndef __OCLRASTER_SAMPLE_GL_CMP_HPP__
#define __OCLRASTER_SAMPLE_GL_CMP_HPP__

#include <oclraster/oclraster.hpp>
#include <oclraster/pipeline/pipeline.hpp>
#include <oclraster/pipeline/transform_stage.hpp>
#include <oclraster/pipeline/image.hpp>
#include <oclraster/core/a2m.hpp>
#include <oclraster/core/camera.hpp>
#include <oclraster/program/oclraster_program.hpp>
#include <oclraster/program/transform_program.hpp>
#include <oclraster/program/rasterization_program.hpp>

#define APPLICATION_NAME "oclraster opengl compare"

// prototypes
bool key_handler(EVENT_TYPE type, shared_ptr<event_object> obj);
bool mouse_handler(EVENT_TYPE type, shared_ptr<event_object> obj);
bool quit_handler(EVENT_TYPE type, shared_ptr<event_object> obj);
bool kernel_reload_handler(EVENT_TYPE type, shared_ptr<event_object> obj);

bool load_programs();

#endif
