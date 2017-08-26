/**
 * @file:   win32_vulkan.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016 Jesper Stefansson
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

extern PlatformState *g_platform;

VkResult
platform_vulkan_create_surface(VkInstance instance,
                               VkSurfaceKHR *surface)
{
    Win32State *native = (Win32State*)g_platform->native;
    VkWin32SurfaceCreateInfoKHR create_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr,
        0,
        native->hinstance,
        native->hwnd
    };

    return vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, surface);
}

bool
platform_vulkan_enable_instance_extension(VkExtensionProperties &extension)
{
    bool enable = false;

    enable |= strcmp(extension.extensionName, VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0;

    return enable;
}

bool
platform_vulkan_enable_instance_layer(VkLayerProperties layer)
{
    (void)layer;
    bool enable = false;
    return enable;
}



