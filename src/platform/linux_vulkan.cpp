/**
 * @file:   linux_vulkan.cpp
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

#include "platform_vulkan.h"

#include <xcb/xcb.h>

VkResult
vulkan_create_surface(VkInstance instance,
                      VkSurfaceKHR *surface,
                      PlatformState platform_state)
{
	VkXcbSurfaceCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	info.pNext = nullptr;
	info.flags = 0;
	info.connection = platform_state.xcb.connection;
	info.window     = platform_state.xcb.window;

	return vkCreateXcbSurfaceKHR(instance, &info, nullptr, surface);
}

bool
platform_vulkan_enable_instance_extension(VkExtensionProperties &extension)
{
	return strcmp(extension.extensionName, VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0;
}

bool
platform_vulkan_enable_instance_layer(VkLayerProperties layer)
{
	VAR_UNUSED(layer);
	return false;
}

