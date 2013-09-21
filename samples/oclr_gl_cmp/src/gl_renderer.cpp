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

#include "gl_renderer.hpp"
#include <oclraster/oclraster.hpp>
#include <floor/core/gl_support.hpp>
#include <oclraster/core/camera.hpp>
#include <regex>

//
struct shader_object {
	struct internal_shader_object {
		unsigned int program;
		unsigned int vertex_shader;
		unsigned int fragment_shader;
		
		struct shader_variable {
			size_t location;
			size_t size;
			size_t type;
			shader_variable(size_t location_, size_t size_, GLenum type_) : location(location_), size(size_), type(type_) {}
		};
		unordered_map<string, shader_variable> uniforms;
		unordered_map<string, shader_variable> attributes;
		unordered_map<string, size_t> samplers;
		
		internal_shader_object() : program(0), vertex_shader(0), fragment_shader(0), uniforms(), attributes(), samplers() {}
		~internal_shader_object() {
			uniforms.clear();
			attributes.clear();
			samplers.clear();
		}
	};
	const string name;
	internal_shader_object program;
	
	shader_object(const string& shd_name) : name(shd_name) {}
	~shader_object() {}
};
static void compile_shaders();
static shader_object* get_shader(const string& name);

static unordered_map<string, shader_object*> shader_objects;
static GLuint index_buffer = 0;
static GLuint vertex_buffer = 0;
static unsigned int triangle_count = 0;
static GLuint copy_fbo_id = 0;
static GLuint copy_fbo_tex_id = 0;
static GLuint depth_buffer = 0;

//
void init_gl_renderer() {
	compile_shaders();
	
	// create a fbo for copying the color framebuffer every frame and displaying it
	glGenFramebuffers(1, &copy_fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, copy_fbo_id);
	glGenTextures(1, &copy_fbo_tex_id);
	glBindTexture(GL_TEXTURE_2D, copy_fbo_tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, floor::get_width(), floor::get_height(),
				 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copy_fbo_tex_id, 0);
	
	glGenRenderbuffers(1, &depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, floor::get_width(), floor::get_height());
	
	glBindFramebuffer(GL_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void destroy_gl_renderer() {
	if(index_buffer != 0) {
		glDeleteBuffers(1, &index_buffer);
		index_buffer = 0;
	}
	if(vertex_buffer != 0) {
		glDeleteBuffers(1, &vertex_buffer);
		vertex_buffer = 0;
	}
}

void create_gl_buffers(const size_t& indices_size, const void* indices,
					   const size_t& vertex_data_size, const void* vertex_data) {
	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertex_data_size, vertex_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	triangle_count = (unsigned int)(indices_size / (sizeof(unsigned int)));
}

void gl_render(camera* cam) {
	// draw
	glBindFramebuffer(GL_FRAMEBUFFER, copy_fbo_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copy_fbo_tex_id, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	const float aspect_ratio = float(floor::get_width()) / float(floor::get_height());
	const float angle_ratio = tanf(DEG2RAD(floor::get_fov() * 0.5f)) * 2.0f;
	
	constexpr float z_near = 0.01f, z_far = 1000.0f;
	matrix4f pm {
		1.0f / (angle_ratio * aspect_ratio), 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f / (angle_ratio), 0.0f, 0.0f,
		0.0f, 0.0f, (z_far + z_near) / (z_near - z_far), -1.0f,
		0.0f, 0.0f, (2.0f * z_far * z_near) / (z_near - z_far), 0.0f
	};
	
	matrix4f mvm = (matrix4f().translate(-cam->get_position().x, -cam->get_position().y, -cam->get_position().z) *
					(matrix4f().rotate_y(cam->get_rotation().y) * matrix4f().rotate_x(360.0f - cam->get_rotation().x)));
	matrix4f mvpm = mvm * pm;
	
	shader_object* shd = get_shader("GL_CMP");
	glUseProgram(shd->program.program);
	glUniformMatrix4fv((GLint)shd->program.uniforms.find("mvpm")->second.location, 1, false, (GLfloat*)&mvpm);
	
	glFrontFace(GL_CCW);
	glDisable(GL_CULL_FACE);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer((GLint)shd->program.attributes.find("in_vertex")->second.location, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glDrawElements(GL_TRIANGLES, triangle_count, GL_UNSIGNED_INT, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glUseProgram(0);
	
	// blit
	glBindFramebuffer(GL_READ_FRAMEBUFFER, copy_fbo_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
	glBlitFramebuffer(0, 0, floor::get_width(), floor::get_height(),
					  0, 0, floor::get_width(), floor::get_height(),
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void log_pretty_print(const char* log, const char* code floor_unused) {
	log_undecorated("%s", log);
}

static bool is_gl_sampler_type(const GLenum& type) {
	switch(type) {
		case GL_SAMPLER_2D: return true;
		case GL_SAMPLER_CUBE: return true;
		default: break;
	}
	return false;
}

#define OCLRASTER_SHADER_LOG_SIZE (65535)
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

void compile_shaders() {
	static constexpr char blit_vs_text[] { u8R"OCLRASTER_RAWSTR(
		uniform mat4 mvpm;
		attribute vec4 in_vertex;
		varying vec4 out_vertex;
		varying vec4 color;
		void main() {
			vec4 mv_vertex = mvpm * vec4(in_vertex.x, in_vertex.y,
										 -0.375, // -1 / (z * aspect * 2) ... damn camera setup
										 1.0);
			out_vertex = mv_vertex;
			gl_Position = mv_vertex;
			color = vec4(in_vertex.w == 0.0 ? 1.0 : 0.0,
						 in_vertex.w == 2.0 ? 1.0 : 0.0,
						 in_vertex.w == 1.0 ? 1.0 : 0.0,
						 1.0);
		}
	)OCLRASTER_RAWSTR"};
	static constexpr char blit_fs_text[] { u8R"OCLRASTER_RAWSTR(
		varying vec4 out_vertex;
		varying vec4 color;
		void main() {
			gl_FragColor = color;
		}
	)OCLRASTER_RAWSTR"};
	
	shader_object* shd = new shader_object("GL_CMP");
	compile_shader(*shd, blit_vs_text, blit_fs_text);
	shader_objects.emplace("GL_CMP", shd);
}

shader_object* get_shader(const string& name) {
	const auto iter = shader_objects.find(name);
	if(iter == shader_objects.end()) return nullptr;
	return iter->second;
}
