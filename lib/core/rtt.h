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

#ifndef __OCLRASTER_RENDER_TO_TEXTURE_H__
#define __OCLRASTER_RENDER_TO_TEXTURE_H__

#include "oclraster/global.h"
#include "core/core.h"
#include "core/gl_support.h"

enum class TEXTURE_FILTERING : unsigned int {
	POINT,
	LINEAR,
	BILINEAR,
	TRILINEAR,
	AUTOMATIC,
};

class OCLRASTER_API rtt {
public:
	rtt() = delete;
	rtt& operator=(const rtt&) = delete;
	~rtt() = delete;
	
	static void init();
	static void destroy();

	enum class TEXTURE_ANTI_ALIASING {
		NONE,
		MSAA_2,
		MSAA_4,
		MSAA_8,
		MSAA_16,
		MSAA_32,
		MSAA_64,
		SSAA_2,
		SSAA_4,
		FXAA,
		SSAA_4_3_FXAA,
		SSAA_2_FXAA,
	};
	static const char* TEXTURE_ANTI_ALIASING_STR[];
	static size_t get_sample_count(const TEXTURE_ANTI_ALIASING& taa);
	static float get_anti_aliasing_scale(const TEXTURE_ANTI_ALIASING& taa);
	static float2 get_resolution_for_scale(const float& scale, const size2& res);
	
	enum class DEPTH_TYPE {
		NONE,
		RENDERBUFFER,
		TEXTURE_2D
	};
	enum class STENCIL_TYPE {
		NONE,
		STENCIL_8
	};

	struct fbo {
		const unsigned int attachment_count = 0;
		unsigned int fbo_id = 0;
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int draw_width = 0;
		unsigned int draw_height = 0;
		unsigned int color_buffer = 0;
		unsigned int depth_buffer = 0;
		DEPTH_TYPE depth_type = DEPTH_TYPE::NONE;
		STENCIL_TYPE stencil_type = STENCIL_TYPE::NONE;
		GLenum depth_attachment_type = GL_DEPTH_ATTACHMENT;
		unsigned int samples = 0;
		
		vector<unsigned int> tex;
		vector<unsigned int> resolve_buffer;
		vector<bool> auto_mipmap;
		vector<GLenum> target;
		vector<TEXTURE_ANTI_ALIASING> anti_aliasing;

		fbo(const unsigned int& count) : attachment_count(count) {
			tex.resize(count);
			resolve_buffer.resize(count);
			auto_mipmap.resize(count);
			target.resize(count);
			anti_aliasing.resize(count);
			
			tex.assign(count, 0);
			resolve_buffer.assign(count, 0);
			auto_mipmap.assign(count, false);
			target.assign(count, GL_TEXTURE_2D);
			anti_aliasing.assign(count, TEXTURE_ANTI_ALIASING::NONE);
		}
	};

	static rtt::fbo* add_buffer(unsigned int width, unsigned int height, GLenum target = GL_TEXTURE_2D, TEXTURE_FILTERING filtering = TEXTURE_FILTERING::POINT, TEXTURE_ANTI_ALIASING taa = TEXTURE_ANTI_ALIASING::NONE, GLint wrap_s = GL_REPEAT, GLint wrap_t = GL_REPEAT, GLint internal_format = GL_RGBA8, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE, unsigned int attachment_count = 1, rtt::DEPTH_TYPE depth_type = DEPTH_TYPE::NONE, rtt::STENCIL_TYPE stencil_type = STENCIL_TYPE::NONE);
	static rtt::fbo* add_buffer(unsigned int width, unsigned int height, GLenum* target, TEXTURE_FILTERING* filtering, TEXTURE_ANTI_ALIASING* taa, GLint* wrap_s, GLint* wrap_t, GLint* internal_format, GLenum* format, GLenum* type, unsigned int attachment_count = 1, rtt::DEPTH_TYPE depth_type = DEPTH_TYPE::NONE, rtt::STENCIL_TYPE stencil_type = STENCIL_TYPE::NONE);
	static void delete_buffer(rtt::fbo* buffer);
	static void start_draw(rtt::fbo* buffer);
	static void stop_draw();
	static void start_2d_draw();
	static void stop_2d_draw();
	static void clear(const unsigned int and_mask = 0xFFFFFFFF);
	static void check_fbo(rtt::fbo* buffer);
	static const fbo* get_current_buffer();

	static void mipmap();

protected:
	static vector<fbo*> buffers;
	static fbo* current_buffer;
	
	//
	static GLint convert_internal_format(const GLint& internal_format) {
#if !defined(OCLRASTER_IOS)
		return internal_format;
#else
		switch(internal_format) {
			case GL_RGB8: return GL_RGB;
			case GL_RGBA4: return GL_RGBA;
			case GL_RGB5_A1: return GL_RGBA;
			case GL_RGBA8: return GL_RGBA;
			case GL_RGBA16F: return GL_RGBA;
			case GL_DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT;
			case GL_DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT;
				
			case GL_RED:
			case GL_RG:
			case GL_RGB:
			case GL_RGBA:
			case GL_BGRA:
			case GL_ALPHA:
			case GL_LUMINANCE:
			case GL_LUMINANCE_ALPHA:
			default: break;
		}
		return internal_format;
#endif
	}
	static GLenum select_filter(const TEXTURE_FILTERING& filter) {
		switch(filter) {
			case TEXTURE_FILTERING::POINT: return GL_NEAREST;
			case TEXTURE_FILTERING::LINEAR: return GL_LINEAR;
			case TEXTURE_FILTERING::BILINEAR: return GL_LINEAR_MIPMAP_NEAREST;
			case TEXTURE_FILTERING::TRILINEAR: return GL_LINEAR_MIPMAP_LINEAR;
			default: break;
		}
		return GL_NEAREST;
	}

	//
	static unsigned int max_anisotropic_filtering;
	static unsigned int max_texture_size;
	static unsigned int max_samples;
};

#endif
