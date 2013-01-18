/*  Flexible OpenCL Rasterizer (oclraster)
 *  Copyright (C) 2012 - 2013 Florian Ziesche
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "oclraster/oclraster.h"
#include "oclraster/oclraster_version.h"
#include "cl/opencl.h"
#include "core/rtt.h"
#include "core/camera.h"
#include "core/gl_support.h"

#if defined(__APPLE__)
#include "osx/osx_helper.h"
#endif

// init statics
event* oclraster::evt = nullptr;
xml* oclraster::x = nullptr;
opencl_base* ocl = nullptr;
camera* oclraster::cam = nullptr;

struct oclraster::oclraster_config oclraster::config;
xml::xml_doc oclraster::config_doc;

string oclraster::datapath = "";
string oclraster::rel_datapath = "";
string oclraster::callpath = "";
string oclraster::kernelpath = "";

struct oclraster::camera_setup oclraster::cam_setup;
float3 oclraster::position;
float3 oclraster::rotation;
matrix4f oclraster::projection_matrix;
matrix4f oclraster::modelview_matrix;
matrix4f oclraster::mvp_matrix;
matrix4f oclraster::translation_matrix;
matrix4f oclraster::rotation_matrix;
deque<matrix4f*> oclraster::projm_stack;
deque<matrix4f*> oclraster::mvm_stack;

unsigned int oclraster::fps = 0;
unsigned int oclraster::fps_counter = 0;
unsigned int oclraster::fps_time = 0;
float oclraster::frame_time = 0.0f;
unsigned int oclraster::frame_time_sum = 0;
unsigned int oclraster::frame_time_counter = 0;
bool oclraster::new_fps_count = false;

bool oclraster::cursor_visible = true;

event::handler* oclraster::window_handler = nullptr;

atomic<bool> oclraster::reload_kernels_flag { false };

// dll main for windows dll export
#if defined(__WINDOWS__)
BOOL APIENTRY DllMain(HANDLE hModule oclr_unused, DWORD ul_reason_for_call, LPVOID lpReserved oclr_unused);
BOOL APIENTRY DllMain(HANDLE hModule oclr_unused, DWORD ul_reason_for_call, LPVOID lpReserved oclr_unused) {
	switch(ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
#endif // __WINDOWS__

/*! this is used to set an absolute data path depending on call path (path from where the binary is called/started),
 *! which is mostly needed when the binary is opened via finder under os x or any file manager under linux
 */
void oclraster::init(const char* callpath_, const char* datapath_) {
	logger::init();
	
	oclraster::callpath = callpath_;
	oclraster::datapath = callpath_;
	oclraster::rel_datapath = datapath_;
	
#if !defined(__WINDOWS__)
	const char dir_slash = '/';
#else
	const char dir_slash = '\\';
#endif
	
#if defined(OCLRASTER_IOS)
	// strip one "../"
	const size_t cdup_pos = rel_datapath.find("../");
	if(cdup_pos != string::npos) {
		rel_datapath = (rel_datapath.substr(0, cdup_pos) +
						rel_datapath.substr(cdup_pos+3, rel_datapath.length()-cdup_pos-3));
	}
#endif
	
	// no '/' -> relative path
	if(rel_datapath[0] != '/') {
		oclraster::datapath = datapath.substr(0, datapath.rfind(dir_slash)+1) + rel_datapath;
	}
	// absolute path
	else oclraster::datapath = rel_datapath;
	
#if defined(CYGWIN)
	oclraster::callpath = "./";
	oclraster::datapath = callpath_;
	oclraster::datapath = datapath.substr(0, datapath.rfind("/")+1) + rel_datapath;
#endif
	
	// create
#if !defined(__WINDOWS__) && !defined(CYGWIN)
	if(datapath.size() > 0 && datapath[0] == '.') {
		// strip leading '.' from datapath if there is one
		datapath.erase(0, 1);
		
		char working_dir[8192];
		memset(working_dir, 0, 8192);
		getcwd(working_dir, 8192);
		
		datapath = working_dir + datapath;
	}
#elif defined(CYGWIN)
	// do nothing
#else
	char working_dir[8192];
	memset(working_dir, 0, 8192);
	getcwd(working_dir, 8192);
	
	size_t strip_pos = datapath.find("\\.\\");
	if(strip_pos != string::npos) {
		datapath.erase(strip_pos, 3);
	}
	
	bool add_bin_path = (working_dir == datapath.substr(0, datapath.length()-1)) ? false : true;
	if(!add_bin_path) datapath = working_dir + string("\\") + (add_bin_path ? datapath : "");
	else {
		if(datapath[datapath.length()-1] == '/') {
			datapath = datapath.substr(0, datapath.length()-1);
		}
		datapath += string("\\");
	}
	
#endif
	
#if defined(__APPLE__)
	// check if datapath contains a 'MacOS' string (indicates that the binary is called from within an OS X .app or via complete path from the shell)
	if(datapath.find("MacOS") != string::npos) {
		// if so, add "../../../" to the datapath, since we have to relocate the datapath if the binary is inside an .app
		datapath.insert(datapath.find("MacOS")+6, "../../../");
	}
#endif
	
	// condense datapath
	datapath = core::strip_path(datapath);
	
	kernelpath = "kernels/";
	cursor_visible = true;
	
	fps = 0;
	fps_counter = 0;
	fps_time = 0;
	frame_time = 0.0f;
	frame_time_sum = 0;
	frame_time_counter = 0;
	new_fps_count = false;
	
	x = new xml();
	evt = new event();
	
	window_handler = new event::handler(&oclraster::window_event_handler);
	evt->add_internal_event_handler(*window_handler, EVENT_TYPE::WINDOW_RESIZE);
	
	// print out oclraster info
	oclr_debug("%s", (OCLRASTER_VERSION_STRING).c_str());
	
	// load config
	const string config_filename(string("config.xml") +
								 (file_io::is_file(data_path("config.xml.local")) ? ".local" : ""));
	config_doc = x->process_file(data_path(config_filename));
	if(config_doc.valid) {
		config.width = config_doc.get<size_t>("config.screen.width", 1280);
		config.height = config_doc.get<size_t>("config.screen.height", 720);
		config.fullscreen = config_doc.get<bool>("config.screen.fullscreen", false);
		config.vsync = config_doc.get<bool>("config.screen.vsync", false);
		config.stereo = config_doc.get<bool>("config.screen.stereo", false);
		
		config.fov = config_doc.get<float>("config.projection.fov", 72.0f);
		config.near_far_plane.x = config_doc.get<float>("config.projection.near", 1.0f);
		config.near_far_plane.y = config_doc.get<float>("config.projection.far", 1000.0f);
		config.upscaling = config_doc.get<float>("config.projection.upscaling", 1.0f);
		
		config.key_repeat = config_doc.get<size_t>("config.input.key_repeat", 200);
		config.ldouble_click_time = config_doc.get<size_t>("config.input.ldouble_click_time", 200);
		config.mdouble_click_time = config_doc.get<size_t>("config.input.mdouble_click_time", 200);
		config.rdouble_click_time = config_doc.get<size_t>("config.input.rdouble_click_time", 200);
		
		config.opencl_platform = config_doc.get<string>("config.opencl.platform", "0");
		config.clear_cache = config_doc.get<bool>("config.opencl.clear_cache", false);
		const auto cl_dev_tokens(core::tokenize(config_doc.get<string>("config.opencl.restrict", ""), ','));
		for(const auto& dev_token : cl_dev_tokens) {
			if(dev_token == "") continue;
			config.cl_device_restriction.insert(dev_token);
		}
	}
	
	//
	init_internal();
}

void oclraster::destroy() {
	oclr_debug("destroying oclraster ...");
	
	acquire_context();
	
	evt->remove_event_handler(*window_handler);
	delete window_handler;
	
	rtt::destroy();
	if(x != nullptr) delete x;
	if(ocl != nullptr) {
		delete ocl;
		ocl = nullptr;
	}
	
	// delete this at the end, b/c other classes will remove event handlers
	if(evt != nullptr) delete evt;
	
	release_context();

	oclr_debug("oclraster destroyed!");
	
	SDL_GL_DeleteContext(config.ctx);
	SDL_DestroyWindow(config.wnd);
	SDL_Quit();
	
	logger::destroy();
}

void oclraster::init_internal() {
	oclr_debug("initializing oclraster");

	// in order to use multi-threaded opengl with x11/xlib, we have to tell it to actually be thread safe
#if !defined(__APPLE__) && !defined(__WINDOWS__)
	if(XInitThreads() == 0) {
		oclr_error("XInitThreads failed!");
		exit(1);
	}
#endif

	// initialize sdl
	if(SDL_Init(SDL_INIT_VIDEO) == -1) {
		oclr_error("can't init SDL: %s", SDL_GetError());
		exit(1);
	}
	else {
		oclr_debug("sdl initialized");
	}
	atexit(SDL_Quit);

	// set some flags
	config.flags |= SDL_WINDOW_OPENGL;
	config.flags |= SDL_WINDOW_SHOWN;
	
#if !defined(OCLRASTER_IOS)
	config.flags |= SDL_WINDOW_INPUT_FOCUS;
	config.flags |= SDL_WINDOW_MOUSE_FOCUS;
	config.flags |= SDL_WINDOW_RESIZABLE;

	int2 windows_pos(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	if(config.fullscreen) {
		config.flags |= SDL_WINDOW_FULLSCREEN;
		config.flags |= SDL_WINDOW_BORDERLESS;
		windows_pos.set(0, 0);
		oclr_debug("fullscreen enabled");
	}
	else {
		oclr_debug("fullscreen disabled");
	}
#else
	config.flags |= SDL_WINDOW_FULLSCREEN;
	config.flags |= SDL_WINDOW_RESIZABLE;
	config.flags |= SDL_WINDOW_BORDERLESS;
#endif

	oclr_debug("vsync %s", config.vsync ? "enabled" : "disabled");
	
	// gl attributes
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	
#if !defined(OCLRASTER_IOS)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	
	//
	SDL_DisplayMode fullscreen_mode;
	SDL_zero(fullscreen_mode);
	fullscreen_mode.format = SDL_PIXELFORMAT_RGBA8888;
	fullscreen_mode.w = config.width;
	fullscreen_mode.h = config.height;
#endif

	// create screen
#if !defined(OCLRASTER_IOS)
	config.wnd = SDL_CreateWindow("oclraster", windows_pos.x, windows_pos.y, (unsigned int)config.width, (unsigned int)config.height, config.flags);
#else
	config.wnd = SDL_CreateWindow("oclraster", 0, 0, (unsigned int)config.width, (unsigned int)config.height, config.flags);
#endif
	if(config.wnd == nullptr) {
		oclr_error("can't create window: %s", SDL_GetError());
		exit(1);
	}
	else {
		SDL_GetWindowSize(config.wnd, (int*)&config.width, (int*)&config.height);
		oclr_debug("video mode set: w%u h%u", config.width, config.height);
	}
	
#if defined(OCLRASTER_IOS)
	if(SDL_SetWindowDisplayMode(config.wnd, &fullscreen_mode) < 0) {
		oclr_error("can't set up fullscreen display mode: %s", SDL_GetError());
		exit(1);
	}
	SDL_GetWindowSize(config.wnd, (int*)&config.width, (int*)&config.height);
	oclr_debug("fullscreen mode set: w%u h%u", config.width, config.height);
	SDL_ShowWindow(config.wnd);
#endif
	
	config.ctx = SDL_GL_CreateContext(config.wnd);
	if(config.ctx == nullptr) {
		oclr_error("can't create opengl context: %s", SDL_GetError());
		exit(1);
	}
#if !defined(OCLRASTER_IOS)
	SDL_GL_SetSwapInterval(config.vsync ? 1 : 0); // has to be set after context creation
	
	// enable multi-threaded opengl context when on os x
#if defined(__APPLE__) && 0
	CGLContextObj cgl_ctx = CGLGetCurrentContext();
	CGLError cgl_err = CGLEnable(cgl_ctx, kCGLCEMPEngine);
	if(cgl_err != kCGLNoError) {
		oclr_error("unable to set multi-threaded opengl context (%X: %X): %s!",
				  (size_t)cgl_ctx, cgl_err, CGLErrorString(cgl_err));
	}
	else {
		oclr_debug("multi-threaded opengl context enabled!");
	}
#endif
#endif
	
	acquire_context();
	
	// initialize opengl functions (get function pointers) on non-apple platforms
#if !defined(__APPLE__)
	init_gl_funcs();
#endif
	
	// check if a cudacl or pure opencl context should be created
	// use absolute path
#if defined(OCLRASTER_CUDA_CL)
	if(config.opencl_platform == "cuda") {
		ocl = new cudacl(core::strip_path(string(datapath + kernelpath)).c_str(), config.wnd, config.clear_cache);
	}
	else {
#else
		if(config.opencl_platform == "cuda") {
			oclr_error("CUDA support is not enabled!");
		}
#endif
		ocl = new opencl(core::strip_path(string(datapath + kernelpath)).c_str(), config.wnd, config.clear_cache);
#if defined(OCLRASTER_CUDA_CL)
	}
#endif
	
	// make an early clear
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	swap();
	evt->handle_events(); // this will effectively create/open the window on some platforms
	
	//
	int tmp = 0;
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &tmp);
	oclr_debug("double buffering %s", tmp == 1 ? "enabled" : "disabled");

	// print out some opengl informations
	oclr_debug("vendor: %s", glGetString(GL_VENDOR));
	oclr_debug("renderer: %s", glGetString(GL_RENDERER));
	oclr_debug("version: %s", glGetString(GL_VERSION));
	
	if(SDL_GetCurrentVideoDriver() == nullptr) {
		oclr_error("couldn't get video driver: %s!", SDL_GetError());
	}
	else oclr_debug("video driver: %s", SDL_GetCurrentVideoDriver());
	
	evt->set_ldouble_click_time((unsigned int)config.ldouble_click_time);
	evt->set_rdouble_click_time((unsigned int)config.rdouble_click_time);
	evt->set_mdouble_click_time((unsigned int)config.mdouble_click_time);
	
	// initialize ogl
	init_gl();
	oclr_debug("opengl initialized");

	// resize stuff
	resize_window();

	// seed
	const unsigned int rseed = ((unsigned int)time(nullptr)/SDL_GetTicks())*((unsigned int)time(nullptr)%SDL_GetTicks());
	srand(rseed >> 1);
	
	// draw the loading screen/image
	start_draw();
	start_2d_draw();
	/*oclr_texture load_tex = t->add_texture(data_path("loading.png"), TEXTURE_FILTERING::LINEAR, 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	const uint2 load_tex_draw_size(load_tex->width/2, load_tex->height/2);
	const uint2 img_offset((unsigned int)config.width/2 - load_tex_draw_size.x/2,
						   (unsigned int)config.height/2 - load_tex_draw_size.y/2);
	gfx2d::set_blend_mode(gfx2d::BLEND_MODE::PRE_MUL);
	gfx2d::draw_rectangle_texture(rect(img_offset.x, img_offset.y,
									   img_offset.x + load_tex_draw_size.x,
									   img_offset.y + load_tex_draw_size.y),
								  load_tex->tex(),
								  coord(0.0f, 1.0f), coord(1.0f, 0.0f));*/
	stop_2d_draw();
	stop_draw();
	
	// retrieve dpi info
	if(config.dpi == 0) {
#if defined(__APPLE__)
#if !defined(OCLRASTER_IOS)
		config.dpi = osx_helper::get_dpi(config.wnd);
#else
		// TODO: iOS
		config.dpi = 326;
#endif
#elif defined(__WINDOWS__)
		HDC hdc = wglGetCurrentDC();
		const size2 display_res(GetDeviceCaps(hdc, HORZRES), GetDeviceCaps(hdc, VERTRES));
		const float2 display_phys_size(GetDeviceCaps(hdc, HORZSIZE), GetDeviceCaps(hdc, VERTSIZE));
		const float2 display_dpi((float(display_res.x) / display_phys_size.x) * 25.4f,
								 (float(display_res.y) / display_phys_size.y) * 25.4f);
		config.dpi = floorf(std::max(display_dpi.x, display_dpi.y));
#else // x11
		SDL_SysWMinfo wm_info;
		SDL_VERSION(&wm_info.version);
		if(SDL_GetWindowWMInfo(config.wnd, &wm_info) == 1) {
			Display* display = wm_info.info.x11.display;
			const size2 display_res(DisplayWidth(display, 0), DisplayHeight(display, 0));
			const float2 display_phys_size(DisplayWidthMM(display, 0), DisplayHeightMM(display, 0));
			const float2 display_dpi((float(display_res.x) / display_phys_size.x) * 25.4f,
									 (float(display_res.y) / display_phys_size.y) * 25.4f);
			config.dpi = floorf(std::max(display_dpi.x, display_dpi.y));
		}
#endif
	}
	
	// init rtt
	rtt::init();
	
	// init opencl
	ocl->init(false,
			  config.opencl_platform == "cuda" ? 0 : string2size_t(config.opencl_platform),
			  config.cl_device_restriction);
	
	// set dpi lower bound to 72
	if(config.dpi < 72) config.dpi = 72;
	
	release_context();
}

/*! sets the windows width
 *  @param width the window width
 */
void oclraster::set_width(const unsigned int& width) {
	if(width == config.width) return;
	config.width = width;
	SDL_SetWindowSize(config.wnd, (int)config.width, (int)config.height);
	// TODO: make this work:
	/*SDL_SetWindowPosition(config.wnd,
						  config.fullscreen ? 0 : SDL_WINDOWPOS_CENTERED,
						  config.fullscreen ? 0 : SDL_WINDOWPOS_CENTERED);*/
}

/*! sets the window height
 *  @param height the window height
 */
void oclraster::set_height(const unsigned int& height) {
	if(height == config.height) return;
	config.height = height;
	SDL_SetWindowSize(config.wnd, (int)config.width, (int)config.height);
	// TODO: make this work:
	/*SDL_SetWindowPosition(config.wnd,
						  config.fullscreen ? 0 : SDL_WINDOWPOS_CENTERED,
						  config.fullscreen ? 0 : SDL_WINDOWPOS_CENTERED);*/
}

void oclraster::set_screen_size(const uint2& screen_size) {
	if(screen_size.x == config.width && screen_size.y == config.height) return;
	config.width = screen_size.x;
	config.height = screen_size.y;
	SDL_SetWindowSize(config.wnd, (int)config.width, (int)config.height);
	
	SDL_Rect bounds;
	SDL_GetDisplayBounds(0, &bounds);
	SDL_SetWindowPosition(config.wnd,
						  bounds.x + (bounds.w - int(config.width)) / 2,
						  bounds.y + (bounds.h - int(config.height)) / 2);
	// TODO: make this work:
	/*SDL_SetWindowPosition(config.wnd,
						  config.fullscreen ? 0 : SDL_WINDOWPOS_CENTERED,
						  config.fullscreen ? 0 : SDL_WINDOWPOS_CENTERED);*/
}

void oclraster::set_fullscreen(const bool& state) {
	if(state == config.fullscreen) return;
	config.fullscreen = state;
	if(SDL_SetWindowFullscreen(config.wnd, (SDL_bool)state) != 0) {
		oclr_error("failed to %s fullscreen: %s!",
				  (state ? "enable" : "disable"), SDL_GetError());
	}
	evt->add_event(EVENT_TYPE::WINDOW_RESIZE,
				   make_shared<window_resize_event>(SDL_GetTicks(),
													size2(config.width, config.height)));
	// TODO: border?
}

void oclraster::set_vsync(const bool& state) {
	if(state == config.vsync) return;
	config.vsync = state;
#if !defined(OCLRASTER_IOS)
	SDL_GL_SetSwapInterval(config.vsync ? 1 : 0);
#endif
}

/*! starts drawing the window
 */
void oclraster::start_draw() {
	acquire_context();
	
	// draws ogl stuff
	glBindFramebuffer(GL_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
	glViewport(0, 0, (unsigned int)config.width, (unsigned int)config.height);
	
	// clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// reset model view matrix
	modelview_matrix.identity();
	translation_matrix.identity();
	rotation_matrix.identity();
	mvp_matrix = projection_matrix;
	
	// run/step camera
	run_camera();
}

/*! stops drawing the window
 */
void oclraster::stop_draw() {
	// TODO: draw scene
	
	//
	swap();
	
	GLenum error = glGetError();
	switch(error) {
		case GL_NO_ERROR:
			break;
		case GL_INVALID_ENUM:
			oclr_error("OpenGL error: invalid enum!");
			break;
		case GL_INVALID_VALUE:
			oclr_error("OpenGL error: invalid value!");
			break;
		case GL_INVALID_OPERATION:
			oclr_error("OpenGL error: invalid operation!");
			break;
		case GL_OUT_OF_MEMORY:
			oclr_error("OpenGL error: out of memory!");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			oclr_error("OpenGL error: invalid framebuffer operation!");
			break;
		default:
			oclr_error("unknown OpenGL error: %u!");
			break;
	}
	
	frame_time_sum += SDL_GetTicks() - frame_time_counter;

	// handle fps count
	fps_counter++;
	if(SDL_GetTicks() - fps_time > 1000) {
		fps = fps_counter;
		new_fps_count = true;
		fps_counter = 0;
		fps_time = SDL_GetTicks();
		
		frame_time = (float)frame_time_sum / (float)fps;
		frame_time_sum = 0;
	}
	frame_time_counter = SDL_GetTicks();
	
	// check for kernel reload (this is safe to do here)
	if(reload_kernels_flag) {
		reload_kernels_flag = false;
		ocl->flush();
		ocl->finish();
		ocl->reload_kernels();
	}
	
	release_context();
}

/*! sets the window caption
 *  @param caption the window caption
 */
void oclraster::set_caption(const char* caption) {
	SDL_SetWindowTitle(config.wnd, caption);
}

/*! returns the window caption
 */
const char* oclraster::get_caption() {
	return SDL_GetWindowTitle(config.wnd);
}

/*! opengl initialization function
 */
void oclraster::init_gl() {
	// set clear color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// depth buffer setup
	glClearDepth(1.0f);
	// enable depth testing
	glEnable(GL_DEPTH_TEST);
	// less/equal depth test
	glDepthFunc(GL_LESS);
	// enable backface culling
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glDisable(GL_STENCIL_TEST);
}

/* function to reset our viewport after a window resize
 */
void oclraster::resize_window() {
	// set the viewport
	glViewport(0, 0, (GLsizei)config.width, (GLsizei)config.height);

	// projection matrix
	// set perspective with fov (default = 72) and near/far plane value (default = 1.0f/1000.0f)
	projection_matrix.perspective(config.fov, float(config.width) / float(config.height),
								  config.near_far_plane.x, config.near_far_plane.y);

	// model view matrix
	modelview_matrix.identity();
	translation_matrix.identity();
	rotation_matrix.identity();
	mvp_matrix = projection_matrix;
}

/*! sets the position of the user/viewer
 *  @param xpos x coordinate
 *  @param ypos y coordinate
 *  @param zpos z coordinate
 */
void oclraster::set_position(const float& xpos, const float& ypos, const float& zpos) {
	position.set(xpos, ypos, zpos);
	
	translation_matrix = matrix4f().translate(xpos, ypos, zpos);
	modelview_matrix = translation_matrix * rotation_matrix;
	mvp_matrix = modelview_matrix * projection_matrix;
}

/*! sets the rotation of the user/viewer
 *  @param xrot x rotation
 *  @param yrot y rotation
 */
void oclraster::set_rotation(const float& xrot, const float& yrot) {
	rotation.x = xrot;
	rotation.y = yrot;
	
	rotation_matrix = matrix4f().rotate_y(yrot) * matrix4f().rotate_x(xrot);
	modelview_matrix = translation_matrix * rotation_matrix;
	mvp_matrix = modelview_matrix * projection_matrix;
}

/*! returns the position of the user
 */
float3& oclraster::get_position() {
	return position;
}

/*! returns the rotation of the user
 */
float3& oclraster::get_rotation() {
	return rotation;
}

/*! starts drawing the 2d elements and initializes the opengl functions for that
 */
void oclraster::start_2d_draw() {
	start_2d_draw((unsigned int)config.width, (unsigned int)config.height);
}

void oclraster::start_2d_draw(const unsigned int width, const unsigned int height) {
	glViewport(0, 0, width, height);

	// we need an orthogonal view (2d) for drawing 2d elements
	push_projection_matrix();
	projection_matrix.ortho(0, width, 0, height, -1.0, 1.0);

	push_modelview_matrix();
	modelview_matrix.identity();
	
	glFrontFace(GL_CW);
	mvp_matrix = projection_matrix;
	glDisable(GL_CULL_FACE); // TODO: GL3, remove again
	
	// shaders are using pre-multiplied alpha
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

/*! stops drawing the 2d elements
 */
void oclraster::stop_2d_draw() {
	pop_projection_matrix();
	pop_modelview_matrix();
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE); // TODO: GL3, remove again
}

/*! sets the cursors visibility to state
 *  @param state the cursor visibility state
 */
void oclraster::set_cursor_visible(const bool& state) {
	oclraster::cursor_visible = state;
	SDL_ShowCursor(oclraster::cursor_visible);
}

/*! returns the cursor visibility stateo
 */
bool oclraster::get_cursor_visible() {
	return oclraster::cursor_visible;
}

/*! returns a pointer to the event class
 */
event* oclraster::get_event() {
	return oclraster::evt;
}

/*! returns the xml class
 */
xml* oclraster::get_xml() {
	return oclraster::x;
}

/*! sets the data path
 *  @param data_path the data path
 */
void oclraster::set_data_path(const char* data_path) {
	oclraster::datapath = data_path;
}

/*! returns the data path
 */
string oclraster::get_data_path() {
	return datapath;
}

/*! returns the call path
 */
string oclraster::get_call_path() {
	return callpath;
}

/*! returns the kernel path
 */
string oclraster::get_kernel_path() {
	return kernelpath;
}

/*! returns data path + str
 *  @param str str we want to "add" to the data path
 */
string oclraster::data_path(const string& str) {
	if(str.length() == 0) return datapath;
	return datapath + str;
}

/*! returns data path + kernel path + str
 *  @param str str we want to "add" to the data + kernel path
 */
string oclraster::kernel_path(const string& str) {
	if(str.length() == 0) return datapath + kernelpath;
	return datapath + kernelpath + str;
}

/*! strips the data path from a string
 *  @param str str we want strip the data path from
 */
string oclraster::strip_data_path(const string& str) {
	if(str.length() == 0) return "";
	return core::find_and_replace(str, datapath, "");
}

matrix4f& oclraster::get_projection_matrix() {
	return oclraster::projection_matrix;
}

matrix4f& oclraster::get_modelview_matrix() {
	return oclraster::modelview_matrix;
}

matrix4f& oclraster::get_mvp_matrix() {
	return oclraster::mvp_matrix;
}

matrix4f& oclraster::get_translation_matrix() {
	return oclraster::translation_matrix;
}

matrix4f& oclraster::get_rotation_matrix() {
	return oclraster::rotation_matrix;
}

unsigned int oclraster::get_fps() {
	new_fps_count = false;
	return oclraster::fps;
}

float oclraster::get_frame_time() {
	return oclraster::frame_time;
}

bool oclraster::is_new_fps_count() {
	return oclraster::new_fps_count;
}

bool oclraster::get_fullscreen() {
	return config.fullscreen;
}

bool oclraster::get_vsync() {
	return config.vsync;
}

bool oclraster::get_stereo() {
	return config.stereo;
}

unsigned int oclraster::get_width() {
	return (unsigned int)config.width;
}

unsigned int oclraster::get_height() {
	return (unsigned int)config.height;
}

uint2 oclraster::get_screen_size() {
	return uint2((unsigned int)config.width, (unsigned int)config.height);
}

unsigned int oclraster::get_key_repeat() {
	return (unsigned int)config.key_repeat;
}

unsigned int oclraster::get_ldouble_click_time() {
	return (unsigned int)config.ldouble_click_time;
}

unsigned int oclraster::get_mdouble_click_time() {
	return (unsigned int)config.mdouble_click_time;
}

unsigned int oclraster::get_rdouble_click_time() {
	return (unsigned int)config.rdouble_click_time;
}

SDL_Window* oclraster::get_window() {
	return config.wnd;
}

const string oclraster::get_version() {
	return OCLRASTER_VERSION_STRING;
}

void oclraster::swap() {
	SDL_GL_SwapWindow(config.wnd);
}

void oclraster::push_projection_matrix() {
	projm_stack.push_back(new matrix4f(projection_matrix));
}

void oclraster::pop_projection_matrix() {
	matrix4f* pmat = projm_stack.back();
	projection_matrix = *pmat;
	delete pmat;
	projm_stack.pop_back();
}

void oclraster::push_modelview_matrix() {
	mvm_stack.push_back(new matrix4f(modelview_matrix));
}

void oclraster::pop_modelview_matrix() {
	matrix4f* pmat = mvm_stack.back();
	modelview_matrix = *pmat;
	mvp_matrix = modelview_matrix * projection_matrix;
	delete pmat;
	mvm_stack.pop_back();
}

void oclraster::reload_kernels() {
	reload_kernels_flag = true;
}

const float& oclraster::get_fov() {
	return config.fov;
}

void oclraster::set_fov(const float& fov) {
	if(config.fov == fov) return;
	config.fov = fov;
	evt->add_event(EVENT_TYPE::WINDOW_RESIZE,
				   make_shared<window_resize_event>(SDL_GetTicks(),
													size2(config.width, config.height)));
}

const float2& oclraster::get_near_far_plane() {
	return config.near_far_plane;
}

const size_t& oclraster::get_dpi() {
	return config.dpi;
}

xml::xml_doc& oclraster::get_config_doc() {
	return config_doc;
}

void oclraster::acquire_context() {
	// note: the context lock is recursive, so one thread can lock
	// it multiple times. however, SDL_GL_MakeCurrent should only
	// be called once (this is the purpose of ctx_active_locks).
	config.ctx_lock.lock();
	// note: not a race, since there can only be one active gl thread
	const unsigned int cur_active_locks = config.ctx_active_locks++;
	if(cur_active_locks == 0) {
		if(SDL_GL_MakeCurrent(config.wnd, config.ctx) != 0) {
			oclr_error("couldn't make gl context current: %s!", SDL_GetError());
			return;
		}
		if(ocl != nullptr && ocl->is_supported()) {
			ocl->activate_context();
		}
	}
#if defined(OCLRASTER_IOS)
	glBindFramebuffer(GL_FRAMEBUFFER, 1);
#endif
}

void oclraster::release_context() {
	// only call SDL_GL_MakeCurrent with nullptr, when this is the last lock
	const unsigned int cur_active_locks = --config.ctx_active_locks;
	if(cur_active_locks == 0) {
		if(ocl != nullptr && ocl->is_supported()) {
			ocl->finish();
			ocl->deactivate_context();
		}
		if(SDL_GL_MakeCurrent(config.wnd, nullptr) != 0) {
			oclr_error("couldn't release current gl context: %s!", SDL_GetError());
			return;
		}
	}
	config.ctx_lock.unlock();
}

bool oclraster::window_event_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(type == EVENT_TYPE::WINDOW_RESIZE) {
		const window_resize_event& wnd_evt = (const window_resize_event&)*obj;
		config.width = wnd_evt.size.x;
		config.height = wnd_evt.size.y;
		resize_window();
	}
	return true;
}

void oclraster::run_camera() {
	if(cam == nullptr) return;
	cam->run();
	
	const float aspect_ratio = float(config.width) / float(config.height);
	const float angle_ratio = tanf(DEG2RAD(config.fov * 0.5f)) * 2.0f;
	
	const float3 forward_(cam->get_forward());
	const float3 up_(cam->get_up());
	
	const float3 right { (up_ ^ forward_).normalized() };
	const float3 up { (forward_ ^ right).normalized() };
	const float3 forward { forward_.normalized() };
	//oclr_debug("right: %v, up: %v, forward: %v", right, up, forward);
	
	const float3 width_vec { right * angle_ratio * aspect_ratio };
	const float3 height_vec { up * angle_ratio };
	//oclr_debug("w/h: %v %v, %f %f", width_vec, height_vec, aspect_ratio, angle_ratio);
	
	cam_setup.position = cam->get_position();
	// TODO: general upscaling support
#if !defined(__APPLE__)
	cam_setup.x_vec = width_vec / float(config.width);
	cam_setup.y_vec = height_vec / float(config.height);
#else
	const float scale_factor = osx_helper::get_scale_factor(config.wnd);
	cam_setup.x_vec = (width_vec * scale_factor) / float(config.width);
	cam_setup.y_vec = (height_vec * scale_factor) / float(config.height);
#endif
	cam_setup.origin = forward - ((width_vec + height_vec) * 0.5f);
	cam_setup.origin.normalized();
	cam_setup.forward = forward;
}

const oclraster::camera_setup& oclraster::get_camera_setup() {
	return cam_setup;
}

void oclraster::set_camera(camera* cam_) {
	cam = cam_;
}

camera* oclraster::get_camera() {
	return cam;
}

void oclraster::set_upscaling(const float& upscaling_) {
	config.upscaling = upscaling_;
}

const float& oclraster::get_upscaling() {
	return config.upscaling;
}
