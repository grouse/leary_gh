/**
 * @file:   settings.h
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef LEARY_SETTINGS_H
#define LEARY_SETTINGS_H

#include <cstdint>

#ifndef INTROSPECT
#define INTROSPECT
#endif

struct PlatformState;

INTROSPECT struct Resolution
{
	int32_t width  = 1280;
	int32_t height = 720;
};

INTROSPECT struct VideoSettings
{
	Resolution resolution;

	// NOTE: these are integers to later support different fullscreen and vsync techniques
	int16_t fullscreen = 0;
	int16_t vsync      = 1;
};

INTROSPECT struct Settings
{
	VideoSettings video;
};

Settings load_settings(const char* filename, PlatformState &platform_state);
void save_settings(Settings &settings, const char* filename, PlatformState &platform_state);


#endif // LEARY_SETTINGS_H
