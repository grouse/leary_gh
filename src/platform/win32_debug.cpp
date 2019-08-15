/**
 * file:    win32_debug.cpp
 * created: 2016-09-23
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

void platform_output_debug_string(const char *str)
{
    // TODO(jesper): unicode
    OutputDebugString(str);
}

char* win32_system_error_message(DWORD error)
{
    static char buffer[2048];
    DWORD result = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        error,
        0,
        buffer,
        sizeof buffer,
        NULL);
    ASSERT(result != 0);

    return buffer;
}
