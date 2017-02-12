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

#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN32)
	#define DEBUG_BREAK()       __debugbreak()
    #define DEBUG_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
	#define DEBUG_BREAK()       asm("int $3")
    #define DEBUG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define DEBUG_LOG(...)        debug_print(DEBUG_FILENAME, __LINE__, __FUNCTION__, __VA_ARGS__)
#define DEBUG_UNIMPLEMENTED() DEBUG_LOG(Log_unimplemented, "fixme! stub");

#define DEBUG_ASSERT(condition) \
	do { \
		if (!(condition)) { \
			DEBUG_LOG(Log_assert, "assertion failed: %s", #condition); \
			DEBUG_BREAK(); \
		} \
	} while(0)

#define VAR_UNUSED(var) (void)(var)

#if defined(__linux__)
#include <x86intrin.h>
	inline u64 rdtscp()
	{
		u32 dummy;
		u64 rdtscp = __rdtscp(&dummy);
		return rdtscp;
	}
#endif

#define PROFILE_START(name) u64 start_##name = rdtscp();
#define PROFILE_END(name) \
	u64 end_##name = rdtscp();\
	u64 difference_##name = end_##name - start_##name;\
	push_profile_timer(#name, difference_##name)

enum LogChannel {
	Log_error,
	Log_warning,
	Log_info,
	Log_assert,
	Log_unimplemented
};

#define DEBUG_BUFFER_SIZE (1024)

void platform_debug_output(const char *msg);

const char *log_channel_string(LogChannel channel)
{
	switch (channel) {
	case Log_info:          return "info";
	case Log_error:         return "error";
	case Log_warning:       return "warning";
	case Log_assert:        return "assert";
	case Log_unimplemented: return "unimplemented";
	default:                return "";
	}
}

void debug_print(const char *file,
                 u32 line,
                 const char *function,
                 LogChannel channel,
                 const char *fmt, ...)
{
	const char *channel_str = log_channel_string(channel);

	va_list args;
	char message[DEBUG_BUFFER_SIZE];
	char buffer[DEBUG_BUFFER_SIZE];

	va_start(args, fmt);
	i32 result = vsnprintf(message, DEBUG_BUFFER_SIZE, fmt, args);
	va_end(args);
	DEBUG_ASSERT(result < DEBUG_BUFFER_SIZE);

	result = snprintf(buffer, DEBUG_BUFFER_SIZE, "%s:%d: %s: [%s] %s",
	                  file, line, channel_str, function, message);
	DEBUG_ASSERT(result < DEBUG_BUFFER_SIZE);
	platform_debug_output(buffer);
}

void debug_print(const char *file,
                 u32 line,
                 const char *function,
                 const char *fmt, ...)
{
	const char *channel_str = log_channel_string(Log_info);

	va_list args;
	char message[DEBUG_BUFFER_SIZE];
	char buffer[DEBUG_BUFFER_SIZE];

	va_start(args, fmt);
	i32 result = vsnprintf(message, DEBUG_BUFFER_SIZE, fmt, args);
	va_end(args);
	DEBUG_ASSERT(result < DEBUG_BUFFER_SIZE);

	result = snprintf(buffer, DEBUG_BUFFER_SIZE, "%s:%d: %s: [%s] %s",
	                  file, line, channel_str, function, message);
	DEBUG_ASSERT(result < DEBUG_BUFFER_SIZE);
	platform_debug_output(buffer);
}

#endif // LEARY_DEBUG_H
