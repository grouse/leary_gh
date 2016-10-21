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

#if defined(_WIN32)
    #define DEBUG_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define DEBUG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define DEBUG_LOGF(type, format, ...)                                                              \
	debug::printf(type, __FUNCTION__, __LINE__, DEBUG_FILENAME, format, __VA_ARGS__)

#define DEBUG_LOG(type, msg)                                                                       \
	debug::printf(type, __FUNCTION__, __LINE__, DEBUG_FILENAME, "%s", msg)

#define VAR_UNUSED(var) (void)(var)

#if LEARY_DEBUG
	#if defined(_MSC_VER)
		#define DEBUG_BREAK()       __debugbreak()
	#else
		#define DEBUG_BREAK()       asm("int $3") 
	#endif

    #define DEBUG_ASSERT(condition) if (!(condition)) DEBUG_BREAK()
	#define DEBUG_UNIMPLEMENTED()   DEBUG_LOG(LogType::unimplemented, "fixme! stub");
#else
	#define DEBUG_BREAK             do { } while(0)
    #define DEBUG_ASSERT(condition) do { } while(0)
    #define DEBUG_UNIMPLEMENTED     do { } while(0)
#endif // LEARY_DEBUG

enum class LogType {
	error,
	warning,
	info,
	assert,
	unimplemented
};

namespace debug
{
	void printf(LogType type, 
	            const char *func, 
	            uint32_t line, 
	            const char *file, 
	            const char *fmt, ...); 

	void print(LogType type, 
	           const char *func, 
	           uint32_t line, 
	           const char *file, 
	           const char *msg);
}




#endif // LEARY_DEBUG_H
