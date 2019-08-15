/**
 * file:    win32_vulkan.cpp
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016-2018 - all rights reserved
 */

extern PlatformState *g_platform;

VkResult gfx_platform_create_surface(
    VkInstance instance,
    VkSurfaceKHR *surface)
{
    VkWin32SurfaceCreateInfoKHR create_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr,
        0,
        g_platform->native.hinstance,
        g_platform->native.hwnd
    };

    return vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, surface);
}

void gfx_platform_required_extensions(Array<const char*> *extensions)
{
    array_add(extensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}
