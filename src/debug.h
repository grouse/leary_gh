#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <stdio.h>
#include <stdarg.h>

/**
 * enabled debug modes, #undef to disable output and check for that debug type
 * overwritten by #define NDEBUG which disables all types of debug
 */
#define DEBUG_FATAL
#define DEBUG_ERROR
#define DEBUG_WARNING
#define DEBUG_MESSAGE

#ifndef DEBUG_BUFFER_SIZE
#define DEBUG_BUFFER_SIZE (100)
#endif

#define DebugPrint(type, code, msg) Debug::print(type, code, __FILE__, __func__, __LINE__, msg)
#define DebugPrintf(type, code, format, ...) Debug::printf(type, code, __FILE__, __func__, __LINE__, format, __VA_ARGS__)

enum EDebugType {
	FATAL 		= 0,
	ERROR 		= 1,
	WARNING 	= 2,
	MESSAGE 	= 3
};

enum EDebugCode {
	SDL_INIT 			= 0,
	SDL_INIT_WINDOW,
	OPENGL_INIT,
	UTIL_OPEN_FILE,
	OGL_SHADER_COMPILE,
	OGL_SHADER_LINK,
	OGL_LOAD_OBJ_FILE,
	OGL_TEXTURE_UNSUPPORTED_COMPONENTS,
	SETTINGS_UNSUPPORTED_KEY
};

namespace Debug {
	// don't use these functions directly, instead use the helper debug macros defined above.
	void print(EDebugType, EDebugCode, const char* file, const char* func, int line, const char* msg);	
	void printf(EDebugType, EDebugCode, const char* file, const char* func, int line, const char* format, ...);	
}

#endif
