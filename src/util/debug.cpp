/**
 * @file:   debug.cpp
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
**/

#include "debug.h"

#include <stdarg.h>
#include <cstdio>

#define DEBUG_BUFFER_SIZE (256)

namespace debug
{
	static void print_internal(LogType         type,
	                           const char*     func,
	                           const uint32_t& line,
	                           const char*     file,
	                           const char*     msg)
	{
#if LEARY_LOG_ENABLE
		char c;
		switch (type) {
		case LogType::info:
			c = 'I';
			break;
		case LogType::error:
			c = 'E';
			break;
		case LogType::warning:
			c = 'W';
			break;
		case LogType::assert:
			c = 'A';
			break;
		default:
			c = '?';
			break;
		}

		if (c == '?') return;
		std::printf("[%c] %s in %s:%d - %s\n", c, func, file, line, msg);
		std::fflush(stdout);
#endif
	}


	void printf(const char*     func,
	            const uint32_t& line,
	            const char*     file,
	            const char*     fmt, ...)
	{
#if LEARY_LOG_ENABLE
		va_list args;
		va_start(args, fmt);

		char buffer[DEBUG_BUFFER_SIZE];
		vsprintf(buffer, fmt, args);

		va_end(args);

		print_internal(LogType::info, func, line, file, buffer);
#endif
	}

	void printf(LogType          type,
	            const char*      func,
	            const uint32_t&  line,
	            const char*      file,
	            const char*      fmt, ...)
	{
#if LEARY_LOG_ENABLE
		va_list args;
		va_start(args, fmt);

		char buffer[DEBUG_BUFFER_SIZE];
		vsprintf(buffer, fmt, args);

		va_end(args);

		print_internal(type, func, line, file, buffer);
#endif
	}
}
