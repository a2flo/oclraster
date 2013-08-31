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

#if defined(__clang__)
// someone decided to recursively define boolean values ...
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif

#include "ios_helper.h"
#include "core/logger.h"
#include "core/core.h"
#include <regex>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <OpenGLES/EAGL.h>

static unordered_map<string, shader_object*> shader_objects;

void* ios_helper::get_eagl_sharegroup() {
	return [[EAGLContext currentContext] sharegroup];
}

static void log_pretty_print(const char* log, const char* code) {
	static const regex rx_log_line("\\w+: 0:(\\d+):.*");
	smatch regex_result;
	
	const vector<string> lines { core::tokenize(string(log), '\n') };
	const vector<string> code_lines { core::tokenize(string(code), '\n') };
	for(const string& line : lines) {
		if(line.size() == 0) continue;
		log_undecorated("## \033[31m%s\033[m", line);
		
		// find code line and print it (+/- 1 line)
		if(regex_match(line, regex_result, rx_log_line)) {
			const size_t src_line_num = string2size_t(regex_result[1]) - 1;
			if(src_line_num < code_lines.size()) {
				if(src_line_num != 0) {
					log_undecorated("\033[37m%s\033[m", code_lines[src_line_num-1]);
				}
				log_undecorated("\033[31m%s\033[m", code_lines[src_line_num]);
				if(src_line_num+1 < code_lines.size()) {
					log_undecorated("\033[37m%s\033[m", code_lines[src_line_num+1]);
				}
			}
			log_undecorated("");
		}
	}
}

static bool is_gl_sampler_type(const GLenum& type) {
	switch(type) {
		case GL_SAMPLER_2D: return true;
		case GL_SAMPLER_CUBE: return true;
		default: break;
	}
	return false;
}

#define OCLRASTER_SHADER_LOG_SIZE 65535
static void compile_shader(shader_object& shd, const char* vs_text, const char* fs_text) {
	// success flag (if it's 1 (true), we successfully created a shader object)
	GLint success = 0;
	GLchar info_log[OCLRASTER_SHADER_LOG_SIZE+1];
	info_log[OCLRASTER_SHADER_LOG_SIZE] = 0;
	
	// add a new program object to this shader
	shader_object::internal_shader_object& shd_obj = shd.program;
	
	// create the vertex shader object
	shd_obj.vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shd_obj.vertex_shader, 1, (GLchar const**)&vs_text, nullptr);
	glCompileShader(shd_obj.vertex_shader);
	glGetShaderiv(shd_obj.vertex_shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(shd_obj.vertex_shader, OCLRASTER_SHADER_LOG_SIZE, nullptr, info_log);
		log_error("error in vertex shader \"%s\" compilation!", shd.name);
		log_pretty_print(info_log, vs_text);
		return;
	}
	
	// create the fragment shader object
	shd_obj.fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shd_obj.fragment_shader, 1, (GLchar const**)&fs_text, nullptr);
	glCompileShader(shd_obj.fragment_shader);
	glGetShaderiv(shd_obj.fragment_shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(shd_obj.fragment_shader, OCLRASTER_SHADER_LOG_SIZE, nullptr, info_log);
		log_error("error in fragment shader \"%s\" compilation!", shd.name);
		log_pretty_print(info_log, fs_text);
		return;
	}
	
	// create the program object
	shd_obj.program = glCreateProgram();
	// attach the vertex and fragment shader progam to it
	glAttachShader(shd_obj.program, shd_obj.vertex_shader);
	glAttachShader(shd_obj.program, shd_obj.fragment_shader);
	
	// now link the program object
	glLinkProgram(shd_obj.program);
	glGetProgramiv(shd_obj.program, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(shd_obj.program, OCLRASTER_SHADER_LOG_SIZE, nullptr, info_log);
		log_error("error in program \"%s\" linkage!\nInfo log: %s", shd.name, info_log);
		return;
	}
	glUseProgram(shd_obj.program);
	
	// grab number and names of all attributes and uniforms and get their locations (needs to be done before validation, b/c we have to set sampler locations)
	GLint attr_count = 0, uni_count = 0, max_attr_len = 0, max_uni_len = 0;
	GLint var_location = 0;
	GLint var_size = 0;
	GLenum var_type = 0;
	
	glGetProgramiv(shd_obj.program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_attr_len);
	glGetProgramiv(shd_obj.program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_uni_len);
	glGetProgramiv(shd_obj.program, GL_ACTIVE_ATTRIBUTES, &attr_count);
	glGetProgramiv(shd_obj.program, GL_ACTIVE_UNIFORMS, &uni_count);
	max_attr_len+=2;
	max_uni_len+=2;
	
	// note: this may report weird attribute/uniform names (and locations), if uniforms/attributes are optimized away by the compiler
	GLchar* attr_name = new GLchar[max_attr_len];
	for(GLint attr = 0; attr < attr_count; attr++) {
		memset(attr_name, 0, max_attr_len);
		glGetActiveAttrib(shd_obj.program, attr, max_attr_len-1, nullptr, &var_size, &var_type, attr_name);
		var_location = glGetAttribLocation(shd_obj.program, attr_name);
		if(var_location < 0) {
			continue;
		}
		string attribute_name = attr_name;
		if(attribute_name.find("[") != string::npos) attribute_name = attribute_name.substr(0, attribute_name.find("["));
		shd_obj.attributes.insert(make_pair(attribute_name,
											shader_object::internal_shader_object::shader_variable(var_location, var_size, var_type)));
	}
	delete [] attr_name;
	
	GLchar* uni_name = new GLchar[max_uni_len];
	for(GLint uniform = 0; uniform < uni_count; uniform++) {
		memset(uni_name, 0, max_uni_len);
		glGetActiveUniform(shd_obj.program, uniform, max_uni_len-1, nullptr, &var_size, &var_type, uni_name);
		var_location = glGetUniformLocation(shd_obj.program, uni_name);
		if(var_location < 0) {
			continue;
		}
		string uniform_name = uni_name;
		if(uniform_name.find("[") != string::npos) uniform_name = uniform_name.substr(0, uniform_name.find("["));
		shd_obj.uniforms.insert(make_pair(uniform_name,
										  shader_object::internal_shader_object::shader_variable(var_location, var_size, var_type)));
		
		// if the uniform is a sampler, add it to the sampler mapping (with increasing id/num)
		// also: use shader_gl3 here, because we can't use shader_base directly w/o instantiating it
		if(is_gl_sampler_type(var_type)) {
			shd_obj.samplers.insert(make_pair(uniform_name, shd_obj.samplers.size()));
			
			// while we are at it, also set the sampler location to a dummy value (this has to be done to satisfy program validation)
			glUniform1i(var_location, (GLint)shd_obj.samplers.size()-1);
		}
	}
	delete [] uni_name;
	
	// validate the program object
	glValidateProgram(shd_obj.program);
	glGetProgramiv(shd_obj.program, GL_VALIDATE_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(shd_obj.program, OCLRASTER_SHADER_LOG_SIZE, nullptr, info_log);
		log_error("error in program \"%s\" validation!\nInfo log: %s", shd.name, info_log);
		return;
	}
	else {
		glGetProgramInfoLog(shd_obj.program, OCLRASTER_SHADER_LOG_SIZE, nullptr, info_log);
		
		// check if shader will run in software (if so, print out a debug message)
		if(strstr((const char*)info_log, (const char*)"software") != nullptr) {
			log_debug("program \"%s\" validation: %s", shd.name, info_log);
		}
	}
	
	//
	glUseProgram(0);
}

void ios_helper::compile_shaders() {
	static constexpr char blit_vs_text[] { u8R"OCLRASTER_RAWSTR(
		attribute vec2 in_vertex;
		varying lowp vec2 tex_coord;
		void main() {
			gl_Position = vec4(in_vertex.x, in_vertex.y, 0.0, 1.0);
			tex_coord = in_vertex * 0.5 + 0.5;
		}
	)OCLRASTER_RAWSTR"};
	static constexpr char blit_fs_text[] { u8R"OCLRASTER_RAWSTR(
		uniform sampler2D tex;
		varying lowp vec2 tex_coord;
		void main() {
			gl_FragColor = texture2D(tex, tex_coord);
		}
	)OCLRASTER_RAWSTR"};
		
	shader_object* shd = new shader_object("BLIT");
	compile_shader(*shd, blit_vs_text, blit_fs_text);
	shader_objects.emplace("BLIT", shd);
}

shader_object* ios_helper::get_shader(const string& name) {
	const auto iter = shader_objects.find(name);
	if(iter == shader_objects.end()) return nullptr;
	return iter->second;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
