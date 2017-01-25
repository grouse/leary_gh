/**
 * @file:   platform_vulkan.h
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

#ifndef LEARY_PLATFORM_VULKAN_H
#define LEARY_PLATFORM_VULKAN_H

#if defined(__linux__)
	// TODO: support wayland and mir
	#define VK_USE_PLATFORM_XCB_KHR
#elif defined(_WIN32)
	#define VK_USE_PLATFORM_WIN32_KHR
#else
	#error "unsupported platform"
#endif

#include <vulkan/vulkan.h>

#include "platform_main.h"

VkResult
vulkan_create_surface(VkInstance instance,
                      VkSurfaceKHR *surface,
                      PlatformState platform_state);

bool
platform_vulkan_enable_instance_extension(VkExtensionProperties &property);

bool
platform_vulkan_enable_instance_layer(VkLayerProperties layer);


#endif // LEARY_PLATFORM_VULKAN_H
