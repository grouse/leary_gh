/**
 * file:    linux_vulkan.cpp
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2015-2018 - all rights reserved
*/

extern PlatformState *g_platform;

VkResult gfx_platform_create_surface(
    VkInstance instance,
    VkSurfaceKHR *surface)
{
    VkXlibSurfaceCreateInfoKHR info = {};
    info.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.pNext  = nullptr;
    info.flags  = 0;
    info.dpy    = g_platform->native.display;
    info.window = g_platform->native.window;

    return vkCreateXlibSurfaceKHR(instance, &info, nullptr, surface);
}

void gfx_platform_required_extensions(Array<const char*> *extensions)
{
    array_add(extensions, VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
}
