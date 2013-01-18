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

#ifndef __OCLRASTER_GL_SUPPORT_H__
#define __OCLRASTER_GL_SUPPORT_H__

#if defined(__APPLE__)

#if !defined(OCLRASTER_IOS)
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif

// we only need to get opengl functions pointers on windows, linux, *bsd, ...
#else
#include "cpp_headers.h"

#if defined(MINGW)
#define GL3_PROTOTYPES
#endif
#include <GL/gl3.h>

//
void init_gl_funcs();

//
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

//
OGL_API extern PFNGLISRENDERBUFFERPROC _glIsRenderbuffer_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLBINDRENDERBUFFERPROC _glBindRenderbuffer_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLDELETERENDERBUFFERSPROC _glDeleteRenderbuffers_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLGENRENDERBUFFERSPROC _glGenRenderbuffers_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLRENDERBUFFERSTORAGEPROC _glRenderbufferStorage_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLGETRENDERBUFFERPARAMETERIVPROC _glGetRenderbufferParameteriv_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLISFRAMEBUFFERPROC _glIsFramebuffer_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLBINDFRAMEBUFFERPROC _glBindFramebuffer_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLDELETEFRAMEBUFFERSPROC _glDeleteFramebuffers_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLGENFRAMEBUFFERSPROC _glGenFramebuffers_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLCHECKFRAMEBUFFERSTATUSPROC _glCheckFramebufferStatus_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLFRAMEBUFFERTEXTURE1DPROC _glFramebufferTexture1D_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLFRAMEBUFFERTEXTURE2DPROC _glFramebufferTexture2D_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLFRAMEBUFFERTEXTURE3DPROC _glFramebufferTexture3D_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLFRAMEBUFFERRENDERBUFFERPROC _glFramebufferRenderbuffer_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC _glGetFramebufferAttachmentParameteriv_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLGENERATEMIPMAPPROC _glGenerateMipmap_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLBLITFRAMEBUFFERPROC _glBlitFramebuffer_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC _glRenderbufferStorageMultisample_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLFRAMEBUFFERTEXTURELAYERPROC _glFramebufferTextureLayer_ptr; // ARB_framebuffer_object
OGL_API extern PFNGLTEXIMAGE2DMULTISAMPLEPROC _glTexImage2DMultisample_ptr; // ARB_texture_multisample
OGL_API extern PFNGLTEXIMAGE3DMULTISAMPLEPROC _glTexImage3DMultisample_ptr; // ARB_texture_multisample
OGL_API extern PFNGLGETMULTISAMPLEFVPROC _glGetMultisamplefv_ptr; // ARB_texture_multisample
OGL_API extern PFNGLSAMPLEMASKIPROC _glSampleMaski_ptr; // ARB_texture_multisample

#define glIsRenderbuffer ((PFNGLISRENDERBUFFERPROC)_glIsRenderbuffer_ptr)
#define glBindRenderbuffer ((PFNGLBINDRENDERBUFFERPROC)_glBindRenderbuffer_ptr)
#define glDeleteRenderbuffers ((PFNGLDELETERENDERBUFFERSPROC)_glDeleteRenderbuffers_ptr)
#define glGenRenderbuffers ((PFNGLGENRENDERBUFFERSPROC)_glGenRenderbuffers_ptr)
#define glRenderbufferStorage ((PFNGLRENDERBUFFERSTORAGEPROC)_glRenderbufferStorage_ptr)
#define glGetRenderbufferParameteriv ((PFNGLGETRENDERBUFFERPARAMETERIVPROC)_glGetRenderbufferParameteriv_ptr)
#define glIsFramebuffer ((PFNGLISFRAMEBUFFERPROC)_glIsFramebuffer_ptr)
#define glBindFramebuffer ((PFNGLBINDFRAMEBUFFERPROC)_glBindFramebuffer_ptr)
#define glDeleteFramebuffers ((PFNGLDELETEFRAMEBUFFERSPROC)_glDeleteFramebuffers_ptr)
#define glGenFramebuffers ((PFNGLGENFRAMEBUFFERSPROC)_glGenFramebuffers_ptr)
#define glCheckFramebufferStatus ((PFNGLCHECKFRAMEBUFFERSTATUSPROC)_glCheckFramebufferStatus_ptr)
#define glFramebufferTexture1D ((PFNGLFRAMEBUFFERTEXTURE1DPROC)_glFramebufferTexture1D_ptr)
#define glFramebufferTexture2D ((PFNGLFRAMEBUFFERTEXTURE2DPROC)_glFramebufferTexture2D_ptr)
#define glFramebufferTexture3D ((PFNGLFRAMEBUFFERTEXTURE3DPROC)_glFramebufferTexture3D_ptr)
#define glFramebufferRenderbuffer ((PFNGLFRAMEBUFFERRENDERBUFFERPROC)_glFramebufferRenderbuffer_ptr)
#define glGetFramebufferAttachmentParameteriv ((PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)_glGetFramebufferAttachmentParameteriv_ptr)
#define glGenerateMipmap ((PFNGLGENERATEMIPMAPPROC)_glGenerateMipmap_ptr)
#define glBlitFramebuffer ((PFNGLBLITFRAMEBUFFERPROC)_glBlitFramebuffer_ptr)
#define glRenderbufferStorageMultisample ((PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)_glRenderbufferStorageMultisample_ptr)
#define glFramebufferTextureLayer ((PFNGLFRAMEBUFFERTEXTURELAYERPROC)_glFramebufferTextureLayer_ptr)
#define glTexImage2DMultisample ((PFNGLTEXIMAGE2DMULTISAMPLEPROC)_glTexImage2DMultisample_ptr)
#define glTexImage3DMultisample ((PFNGLTEXIMAGE3DMULTISAMPLEPROC)_glTexImage3DMultisample_ptr)
#define glGetMultisamplefv ((PFNGLGETMULTISAMPLEFVPROC)_glGetMultisamplefv_ptr)
#define glSampleMaski ((PFNGLSAMPLEMASKIPROC)_glSampleMaski_ptr)

#endif

#endif
