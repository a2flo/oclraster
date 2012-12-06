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

#include "rtt.h"
#include "oclraster.h"

//
vector<rtt::fbo*> rtt::buffers;
rtt::fbo* rtt::current_buffer = nullptr;
unsigned int rtt::max_anisotropic_filtering = 1;
unsigned int rtt::max_texture_size = 1024;
unsigned int rtt::max_samples = 1;

const char* rtt::TEXTURE_ANTI_ALIASING_STR[] = {
	"NONE",
	"MSAA2",
	"MSAA4",
	"MSAA8",
	"MSAA16",
	"MSAA32",
	"MSAA64",
	"CSAA8",
	"CSAA8Q",
	"CSAA16",
	"CSAA16Q",
	"CSAA32",
	"CSAA32Q",
	"2xSSAA",
	"4xSSAA",
	"FXAA",
	"4/3xSSAA+FXAA",
	"2xSSAA+FXAA",
};

void rtt::init() {
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint*)(&max_anisotropic_filtering));
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&max_texture_size);
	glGetIntegerv(GL_MAX_SAMPLES, (GLint*)&max_samples);
}

void rtt::destroy() {
	glBindFramebuffer(GL_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);

	const vector<fbo*> buffer_copy(buffers); // copy, b/c delete_buffer will operate on buffers
	for(const auto& buffer : buffer_copy) {
		delete_buffer(buffer);
	}
	buffers.clear();
}

rtt::fbo* rtt::add_buffer(unsigned int width, unsigned int height, GLenum target, TEXTURE_FILTERING filtering, TEXTURE_ANTI_ALIASING taa, GLint wrap_s, GLint wrap_t, GLint internal_format, GLenum format, GLenum type, unsigned int attachment_count, DEPTH_TYPE depth_type, STENCIL_TYPE stencil_type) {
	GLenum* targets = new GLenum[attachment_count];
	TEXTURE_FILTERING* filterings = new TEXTURE_FILTERING[attachment_count];
	TEXTURE_ANTI_ALIASING* taas = new TEXTURE_ANTI_ALIASING[attachment_count];
	GLint* wraps_s = new GLint[attachment_count];
	GLint* wraps_t = new GLint[attachment_count];
	GLint* internal_formats = new GLint[attachment_count];
	GLenum* formats = new GLenum[attachment_count];
	GLenum* types = new GLenum[attachment_count];

	for(unsigned int i = 0; i < attachment_count; i++) {
		targets[i] = target;
		filterings[i] = filtering;
		taas[i] = taa;
		wraps_s[i] = wrap_s;
		wraps_t[i] = wrap_t;
		internal_formats[i] = internal_format;
		formats[i] = format;
		types[i] = type;
	}

	rtt::fbo* ret_buffer = add_buffer(width, height, targets, filterings, taas, wraps_s, wraps_t, internal_formats, formats, types, attachment_count, depth_type, stencil_type);

	delete [] targets;
	delete [] filterings;
	delete [] taas;
	delete [] wraps_s;
	delete [] wraps_t;
	delete [] internal_formats;
	delete [] formats;
	delete [] types;

	return ret_buffer;
}

rtt::fbo* rtt::add_buffer(unsigned int width, unsigned int height, GLenum* target, TEXTURE_FILTERING* filtering, TEXTURE_ANTI_ALIASING* taa, GLint* wrap_s, GLint* wrap_t, GLint* internal_format, GLenum* format, GLenum* type, unsigned int attachment_count, DEPTH_TYPE depth_type, STENCIL_TYPE stencil_type) {
	rtt::fbo* buffer = new rtt::fbo(attachment_count);
	buffers.push_back(buffer);
	buffer->width = width;
	buffer->height = height;
	buffer->draw_width = width;
	buffer->draw_height = height;
	buffer->depth_type = depth_type;
	buffer->stencil_type = stencil_type;
	buffer->samples = 0;
	
	//
	const size_t max_tex_size = max_texture_size;
	const float fmax_tex_size = max_tex_size;
	size2 orig_resolution = size2(width, height);
	float ssaa = 0.0f;
	for(unsigned int i = 0; i < buffer->attachment_count; i++) {
		buffer->anti_aliasing[i] = taa[i];
		
		float ssaa_factor = get_anti_aliasing_scale(buffer->anti_aliasing[i]);
		if(ssaa_factor <= 1.0f) continue;
		
		//
		// try lower ssaa setting
		float cur_ssaa_factor = ssaa_factor;
		while(cur_ssaa_factor >= 2.0f) {
			if(float(orig_resolution.x) * ssaa_factor > fmax_tex_size ||
			   float(orig_resolution.y) * ssaa_factor > fmax_tex_size) {
				cur_ssaa_factor -= 2.0f;
				continue;
			}
			else break;
		}
		
		if(cur_ssaa_factor <= 0.0f) {
			oclr_error("couldn't create a SSAA%u buffer (nor using a smaller SSAA setting)!", ssaa_factor);
			break; // break, since this won't work with any setting
		}
		
		if(cur_ssaa_factor < ssaa_factor) {
			oclr_error("couldn't create a SSAA%u buffer - using SSAA%u instead!", ssaa_factor, cur_ssaa_factor);
		}
		
		ssaa = std::max(ssaa, cur_ssaa_factor);
	}
	
	// apply ssaa
	if(ssaa > 0.0f) {
		const float2 ssaa_res = get_resolution_for_scale(ssaa, size2(width, height));
		width = (unsigned int)ssaa_res.x;
		height = (unsigned int)ssaa_res.y;
		buffer->width = width;
		buffer->height = height;
		buffer->draw_width = width;
		buffer->draw_height = height;
	}
	
	//
	glGenFramebuffers(1, &buffer->fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->fbo_id);
	
	glGenTextures(attachment_count, &buffer->tex[0]);
	for(unsigned int i = 0; i < buffer->attachment_count; i++) {
#if defined(OCLRASTER_IOS)
		if(i > 0) {
			oclr_error("too many FBO attachments - only one is allowed on iOS!");
			break;
		}
#endif
		
		buffer->target[i] = target[i];
		glBindTexture(buffer->target[i], buffer->tex[i]);
		
		glTexParameteri(buffer->target[i], GL_TEXTURE_MAG_FILTER,
						(filtering[i] == TEXTURE_FILTERING::POINT ? GL_NEAREST : GL_LINEAR));
		glTexParameteri(buffer->target[i], GL_TEXTURE_MIN_FILTER, select_filter(filtering[i]));
		glTexParameteri(buffer->target[i], GL_TEXTURE_WRAP_S, wrap_s[i]);
		glTexParameteri(buffer->target[i], GL_TEXTURE_WRAP_T, wrap_t[i]);
		if(filtering[i] >= TEXTURE_FILTERING::BILINEAR) {
			glTexParameteri(buffer->target[i], GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropic_filtering);
		}
		
		switch(buffer->target[i]) {
#if !defined(OCLRASTER_IOS)
			case GL_TEXTURE_1D:
				glTexImage1D(buffer->target[i], 0, convert_internal_format(internal_format[i]), width, 0, format[i], type[i], nullptr);
				break;
#endif
			case GL_TEXTURE_2D:
				glTexImage2D(buffer->target[i], 0, convert_internal_format(internal_format[i]), width, height, 0, format[i], type[i], nullptr);
				break;
#if !defined(OCLRASTER_IOS)
			case GL_TEXTURE_2D_MULTISAMPLE:
				glTexImage2DMultisample(buffer->target[i], (GLsizei)get_sample_count(buffer->anti_aliasing[0]), convert_internal_format(internal_format[i]), width, height, false);
				break;
#endif
			default:
				glTexImage2D(buffer->target[i], 0, convert_internal_format(internal_format[i]), width, height, 0, format[i], type[i], nullptr);
				break;
		}
		
		if(filtering[i] > TEXTURE_FILTERING::LINEAR) {
			buffer->auto_mipmap[i] = true;
			//glGenerateMipmap(buffer->target[i]);
		}
		else buffer->auto_mipmap[i] = false;
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, buffer->target[i], buffer->tex[i], 0);
		
#if !defined(OCLRASTER_IOS)
#if defined(oclr_debug)
		// TODO: fbo/texture checking
		GLint check_internal_format = 0, check_type = 0, check_size = 0;
		glGetTexLevelParameteriv(buffer->target[i], 0, GL_TEXTURE_INTERNAL_FORMAT, &check_internal_format);
		glGetTexLevelParameteriv(buffer->target[i], 0, GL_TEXTURE_RED_TYPE, &check_type);
		glGetTexLevelParameteriv(buffer->target[i], 0, GL_TEXTURE_RED_SIZE, &check_size);
		//oclr_debug("FBO: iformat: %X, type: %X, size: %d", check_internal_format, check_type, check_size);
#endif
#endif
	}
	
	current_buffer = buffer;
	check_fbo(current_buffer);
	
	// check if a depth attachment should be created
	if(depth_type != DEPTH_TYPE::NONE) {
		// apparently opencl/opengl depth texture sharing only works with a float format
#if !defined(OCLRASTER_INFERRED_RENDERING_CL)
		GLenum depth_internel_format = GL_DEPTH_COMPONENT24;
		GLenum depth_storage_type = GL_UNSIGNED_INT;
#else
		GLenum depth_internel_format = GL_DEPTH_COMPONENT32F;
		GLenum depth_storage_type = GL_FLOAT;
#endif
		GLenum depth_format = GL_DEPTH_COMPONENT;
		GLenum depth_attachment_type = GL_DEPTH_ATTACHMENT;
		if(stencil_type == STENCIL_TYPE::STENCIL_8) {
#if !defined(OCLRASTER_INFERRED_RENDERING_CL)
			depth_internel_format = GL_DEPTH24_STENCIL8;
			depth_storage_type = GL_UNSIGNED_INT_24_8;
#else
			depth_internel_format = GL_DEPTH32F_STENCIL8;
			depth_storage_type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
#endif
			depth_format = GL_DEPTH_STENCIL;
			depth_attachment_type = GL_DEPTH_STENCIL_ATTACHMENT;
		}
		buffer->depth_attachment_type = depth_attachment_type;
		
		switch(buffer->anti_aliasing[0]) {
			case TEXTURE_ANTI_ALIASING::NONE:
			case TEXTURE_ANTI_ALIASING::SSAA_2:
			case TEXTURE_ANTI_ALIASING::SSAA_4:
			case TEXTURE_ANTI_ALIASING::FXAA:
			case TEXTURE_ANTI_ALIASING::SSAA_4_3_FXAA:
			case TEXTURE_ANTI_ALIASING::SSAA_2_FXAA:
				if(depth_type == DEPTH_TYPE::RENDERBUFFER) {
					glGenRenderbuffers(1, &buffer->depth_buffer);
					glBindRenderbuffer(GL_RENDERBUFFER, buffer->depth_buffer);
					glRenderbufferStorage(GL_RENDERBUFFER, depth_internel_format, width, height);
				}
				else if(depth_type == DEPTH_TYPE::TEXTURE_2D) {
					glGenTextures(1, &buffer->depth_buffer);
					glBindTexture(GL_TEXTURE_2D, buffer->depth_buffer);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
					
					glTexImage2D(GL_TEXTURE_2D, 0, convert_internal_format(depth_internel_format), width, height, 0, depth_format, depth_storage_type, nullptr);
					glFramebufferTexture2D(GL_FRAMEBUFFER, depth_attachment_type, GL_TEXTURE_2D, buffer->depth_buffer, 0);
				}
				
				check_fbo(current_buffer);
				break;
			case TEXTURE_ANTI_ALIASING::MSAA_2:
			case TEXTURE_ANTI_ALIASING::MSAA_4:
			case TEXTURE_ANTI_ALIASING::MSAA_8:
			case TEXTURE_ANTI_ALIASING::MSAA_16:
			case TEXTURE_ANTI_ALIASING::MSAA_32:
			case TEXTURE_ANTI_ALIASING::MSAA_64: {
				buffer->samples = (GLsizei)get_sample_count(buffer->anti_aliasing[0]);
				
				glGenFramebuffers(attachment_count, &buffer->resolve_buffer[0]);
				for(size_t i = 0; i < attachment_count; i++) {
					glBindFramebuffer(GL_FRAMEBUFFER, buffer->resolve_buffer[i]);
					glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0+i), target[i], buffer->tex[i], 0);
				}
				check_fbo(current_buffer);
				
				glBindFramebuffer(GL_FRAMEBUFFER, buffer->fbo_id);
				
				glGenRenderbuffers(1, &buffer->color_buffer);
				glBindRenderbuffer(GL_RENDERBUFFER, buffer->color_buffer);
				glRenderbufferStorageMultisample(GL_RENDERBUFFER, buffer->samples, format[0], buffer->width, buffer->height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, buffer->color_buffer);
				check_fbo(current_buffer);
				
				if(depth_type == DEPTH_TYPE::RENDERBUFFER) {
					glGenRenderbuffers(1, &buffer->depth_buffer);
					glBindRenderbuffer(GL_RENDERBUFFER, buffer->depth_buffer);
					glRenderbufferStorageMultisample(GL_RENDERBUFFER, buffer->samples, depth_internel_format, buffer->width, buffer->height);
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, depth_attachment_type, GL_RENDERBUFFER, buffer->depth_buffer);
				}
#if !defined(OCLRASTER_IOS)
				else if(depth_type == DEPTH_TYPE::TEXTURE_2D) {
					glGenTextures(1, &buffer->depth_buffer);
					glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, buffer->depth_buffer);
					glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					
					glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, buffer->samples, convert_internal_format(depth_internel_format), width, height, false);
					glFramebufferTexture2D(GL_FRAMEBUFFER, depth_attachment_type, GL_TEXTURE_2D_MULTISAMPLE, buffer->depth_buffer, 0);
				}
#endif
				
				check_fbo(current_buffer);
			}
			break;
		}
	}
	
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
	
	return buffer;
}

void rtt::delete_buffer(rtt::fbo* buffer) {
	for(size_t i = 0; i < buffer->attachment_count; i++) {
		if(buffer->tex[i] == 0) continue;
		glDeleteTextures(1, &buffer->tex[i]);
	}
	for(size_t i = 0; i < buffer->attachment_count; i++) {
		if(buffer->resolve_buffer[i] == 0) break;
		glDeleteFramebuffers(1, &buffer->resolve_buffer[i]);
	}
	
	// depending on how these were created, they can either be renderbuffers or textures
	if(glIsRenderbuffer(buffer->color_buffer)) {
		glDeleteRenderbuffers(1, &buffer->color_buffer);
		buffer->color_buffer = 0;
	}
	if(glIsRenderbuffer(buffer->depth_buffer)) {
		glDeleteRenderbuffers(1, &buffer->depth_buffer);
		buffer->depth_buffer = 0;
	}
	if(glIsTexture(buffer->color_buffer)) {
		glDeleteTextures(1, &buffer->color_buffer);
		buffer->color_buffer = 0;
	}
	if(glIsTexture(buffer->depth_buffer)) {
		glDeleteTextures(1, &buffer->depth_buffer);
		buffer->depth_buffer = 0;
	}
	
	//
	glDeleteFramebuffers(1, &buffer->fbo_id);
	const auto iter = find(begin(buffers), end(buffers), buffer);
	if(iter != end(buffers)) {
		buffers.erase(iter);
		delete buffer;
	}
}

void rtt::start_draw(rtt::fbo* buffer) {
	current_buffer = buffer;
	if(buffer->anti_aliasing[0] == TEXTURE_ANTI_ALIASING::NONE ||
	   buffer->anti_aliasing[0] == TEXTURE_ANTI_ALIASING::SSAA_2 ||
	   buffer->anti_aliasing[0] == TEXTURE_ANTI_ALIASING::SSAA_4 ||
	   buffer->anti_aliasing[0] == TEXTURE_ANTI_ALIASING::FXAA ||
	   buffer->anti_aliasing[0] == TEXTURE_ANTI_ALIASING::SSAA_4_3_FXAA ||
	   buffer->anti_aliasing[0] == TEXTURE_ANTI_ALIASING::SSAA_2_FXAA) {
		glBindFramebuffer(GL_FRAMEBUFFER, buffer->fbo_id);
		for(unsigned int i = 0; i < buffer->attachment_count; i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, buffer->target[i], buffer->tex[i], 0);
		}
		
		if(buffer->depth_type == DEPTH_TYPE::RENDERBUFFER) {
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, buffer->depth_attachment_type, GL_RENDERBUFFER, buffer->depth_buffer);
		}
		else if(buffer->depth_type == DEPTH_TYPE::TEXTURE_2D) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, buffer->depth_attachment_type,
#if !defined(OCLRASTER_IOS)
								   (buffer->samples == 0 ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE),
#else
								   GL_TEXTURE_2D,
#endif
								   buffer->depth_buffer, 0);
		}
		else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, buffer->depth_attachment_type, GL_TEXTURE_2D, 0, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, buffer->depth_attachment_type, GL_RENDERBUFFER, 0);
		}
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, buffer->fbo_id);
	}
	glViewport(0, 0, buffer->draw_width, buffer->draw_height);
}

void rtt::stop_draw() {
#if defined(OCLRASTER_DEBUG)
	check_fbo(current_buffer);
#endif
	
	if(current_buffer->anti_aliasing[0] != TEXTURE_ANTI_ALIASING::NONE &&
	   current_buffer->anti_aliasing[0] != TEXTURE_ANTI_ALIASING::SSAA_2 &&
	   current_buffer->anti_aliasing[0] != TEXTURE_ANTI_ALIASING::SSAA_4 &&
	   current_buffer->anti_aliasing[0] != TEXTURE_ANTI_ALIASING::FXAA &&
	   current_buffer->anti_aliasing[0] != TEXTURE_ANTI_ALIASING::SSAA_4_3_FXAA &&
	   current_buffer->anti_aliasing[0] != TEXTURE_ANTI_ALIASING::SSAA_2_FXAA) {
		if(current_buffer->depth_type == DEPTH_TYPE::RENDERBUFFER ||
		   current_buffer->depth_type == DEPTH_TYPE::TEXTURE_2D) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, current_buffer->fbo_id);
			for(size_t i = 0; i < current_buffer->attachment_count; i++) {
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, current_buffer->resolve_buffer[i]);
#if !defined(OCLRASTER_IOS)
				glBlitFramebuffer(0, 0, current_buffer->draw_width, current_buffer->draw_height, 0, 0, current_buffer->draw_width, current_buffer->draw_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
				// TODO: implement framebuffer blitting on iOS
#endif
			}
		}
	}
	
	for(unsigned int i = 0; i < current_buffer->attachment_count; i++) {
		if(current_buffer->auto_mipmap[i]) {
			// TODO: fix this
			//glBindTexture(current_buffer->target[i], current_buffer->tex[i]);
			//glGenerateMipmap(current_buffer->target[i]);
		}
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
}

void rtt::start_2d_draw() {
	oclraster::start_2d_draw(current_buffer->draw_width, current_buffer->draw_height);
}

void rtt::stop_2d_draw() {
	oclraster::stop_2d_draw();
}

void rtt::clear(const unsigned int and_mask) {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLbitfield bits = GL_COLOR_BUFFER_BIT;
	bits |= (current_buffer->depth_type != DEPTH_TYPE::NONE ? GL_DEPTH_BUFFER_BIT : 0);
	bits |= (current_buffer->stencil_type != STENCIL_TYPE::NONE ? GL_STENCIL_BUFFER_BIT : 0);
	glClear(bits & and_mask);
}

void rtt::check_fbo(rtt::fbo* buffer) {
	GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE) {
		oclr_error("framebuffer (size: %u*%upx; depth: %i; stencil: %i; tex id: %u; fbo id: %u) didn't pass status check!",
				  buffer->width, buffer->height, (unsigned int)buffer->depth_type, (unsigned int)buffer->stencil_type, buffer->tex[0], buffer->fbo_id);
	}
	
	switch(status) {
		case GL_FRAMEBUFFER_COMPLETE:
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			oclr_error("unsupported framebuffer (%u)!", status);
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			oclr_error("incomplete framebuffer attachement (%u)!", status);
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			oclr_error("missing framebuffer attachement (%u)!", status);
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			oclr_error("incomplete framebuffer multisample (%u)!", status);
			break;
#if !defined(OCLRASTER_IOS)
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			oclr_error("incomplete framebuffer draw buffer (%u)!", status);
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			oclr_error("incomplete framebuffer read buffer (%u)!", status);
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			oclr_error("incomplete framebuffer layer targets (%u)!", status);
			break;
#endif
		default:
			oclr_error("unknown framebuffer error (%u)!", status);
			break;
	}
}

size_t rtt::get_sample_count(const TEXTURE_ANTI_ALIASING& taa) {
	switch(taa) {
		case TEXTURE_ANTI_ALIASING::NONE: return 0;
		case TEXTURE_ANTI_ALIASING::MSAA_2: return 2;
		case TEXTURE_ANTI_ALIASING::MSAA_4: return 4;
		case TEXTURE_ANTI_ALIASING::MSAA_8: return 8;
		case TEXTURE_ANTI_ALIASING::MSAA_16: return 16;
		case TEXTURE_ANTI_ALIASING::MSAA_32: return 32;
		case TEXTURE_ANTI_ALIASING::MSAA_64: return 64;
		case TEXTURE_ANTI_ALIASING::SSAA_2: return 0;
		case TEXTURE_ANTI_ALIASING::SSAA_4: return 0;
		case TEXTURE_ANTI_ALIASING::FXAA: return 0;
		case TEXTURE_ANTI_ALIASING::SSAA_4_3_FXAA: return 0;
		case TEXTURE_ANTI_ALIASING::SSAA_2_FXAA: return 0;
	}
	assert(false && "invalid anti-aliasing mode");
	return 0;
}

const rtt::fbo* rtt::get_current_buffer() {
	return current_buffer;
}

float rtt::get_anti_aliasing_scale(const TEXTURE_ANTI_ALIASING& taa) {
	switch(taa) {
		case TEXTURE_ANTI_ALIASING::SSAA_2: return 2.0f;
		case TEXTURE_ANTI_ALIASING::SSAA_4: return 4.0f;
		case TEXTURE_ANTI_ALIASING::SSAA_4_3_FXAA: return 4.0f/3.0f;
		case TEXTURE_ANTI_ALIASING::SSAA_2_FXAA: return 2.0f;
		default: break;
	}
	return 1.0f;
}

float2 rtt::get_resolution_for_scale(const float& scale, const size2& res) {
	if(scale == 1.0f) return float2(res);
	
	float2 ret_res(ceilf(scale * float(res.x)), ceilf(scale * float(res.y)));
	
	// always return a resolution that is dividable by 2
	ret_res.x += (fmod(ret_res.x, 2.0f) != 0.0f ? 1.0f : 0.0f);
	ret_res.y += (fmod(ret_res.y, 2.0f) != 0.0f ? 1.0f : 0.0f);
	return ret_res;
}
