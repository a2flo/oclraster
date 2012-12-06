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

#include <Cocoa/Cocoa.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "core/vector2.h"
#include "osx_helper.h"

size_t osx_helper::get_dpi(SDL_Window* wnd) {
	float2 display_res(float2(CGDisplayPixelsWide(CGMainDisplayID()),
							  CGDisplayPixelsHigh(CGMainDisplayID())) * get_scale_factor(wnd));
	const CGSize display_phys_size(CGDisplayScreenSize(CGMainDisplayID()));
	const float2 display_dpi((display_res.x / display_phys_size.width) * 25.4f,
							 (display_res.y / display_phys_size.height) * 25.4f);
	return floorf(std::max(display_dpi.x, display_dpi.y));
}

float osx_helper::get_scale_factor(SDL_Window* wnd) {
	SDL_SysWMinfo wm_info;
	SDL_VERSION(&wm_info.version);
	if(SDL_GetWindowWMInfo(wnd, &wm_info) == 1) {
		return [wm_info.info.cocoa.window backingScaleFactor];
	}
	return 1.0f;
}

float osx_helper::get_menu_bar_height() {
	return [[[NSApplication sharedApplication] mainMenu] menuBarHeight];
}
