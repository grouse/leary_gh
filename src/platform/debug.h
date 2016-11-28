/**
 * @file:   debug.h
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

#ifndef LEARY_DEBUG_H
#define LEARY_DEBUG_H

#include <cstring>
#include <cstdint>
#include <cstdarg>

#if defined(_WIN32)
    #define DEBUG_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define DEBUG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define DEBUG_LOGF(type, format, ...)                                                              \
	debug_printf(type, __FUNCTION__, __LINE__, DEBUG_FILENAME, format, __VA_ARGS__)

#define DEBUG_LOG(type, msg)                                                                       \
	debug_print(type, __FUNCTION__, __LINE__, DEBUG_FILENAME, msg)

#define VAR_UNUSED(var) (void)(var)

#if defined(_MSC_VER)
	#define DEBUG_BREAK()       __debugbreak()
#else
	#define DEBUG_BREAK()       asm("int $3")
#endif

#define DEBUG_ASSERT(condition) if (!(condition)) DEBUG_BREAK()
#define DEBUG_UNIMPLEMENTED()   DEBUG_LOG(LogType::unimplemented, "fixme! stub");

enum class LogType {
	error,
	warning,
	info,
	assert,
	unimplemented
};

#define DEBUG_BUFFER_SIZE (512)

void platform_debug_output(const char *msg);

void debug_print(LogType    type,
                 const char *func,
                 uint32_t   line,
                 const char *file,
                 const char *msg);

void debug_printf(LogType    type,
                  const char *func,
                  uint32_t   line,
                  const char *file,
                  const char *fmt, ...);

#endif // LEARY_DEBUG_H

#ifdef DEBUG_PRINT_IMPL
void debug_print(LogType    type,
                 const char *func,
                 uint32_t   line,
                 const char *file,
                 const char *msg)
{
	const char *type_str;

	switch (type) {
	case LogType::info:
		type_str = "info";
		break;
	case LogType::error:
		type_str = "error";
		break;
	case LogType::warning:
		type_str = "warning";
		break;
	case LogType::assert:
		type_str = "assert";
		break;
	case LogType::unimplemented:
		type_str = "unimplemented";
		break;
	default:
		type_str = "";
		break;
	}

	// TODO(jesper): add log to file if enabled
	char buffer[DEBUG_BUFFER_SIZE];
	std::sprintf(buffer, "%s:%d: %s in %s: %s\n", file, line, type_str, func, msg);
	platform_debug_output(buffer);
}

void debug_printf(LogType    type,
                  const char *func,
                  uint32_t   line,
                  const char *file,
                  const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char msg[DEBUG_BUFFER_SIZE];
	std::vsprintf(msg, fmt, args);

	va_end(args);

	debug_print(type, func, line, file, msg);
}

#undef DEBUG_PRINT_IMPL
#endif // DEBUG_PRINT_IMPL

