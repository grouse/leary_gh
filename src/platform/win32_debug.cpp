/**
 * @file:   win32_debug.cpp
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
**/

#include "debug.h"

#include <stdarg.h>
#include <cstdio>

#define DEBUG_BUFFER_SIZE (512)

namespace debug
{
	void print(LogType    type,
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
		OutputDebugString(buffer);
	}

	void printf(LogType    type,
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

		debug::print(type, func, line, file, msg);
	}
}

