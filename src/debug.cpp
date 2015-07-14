#include "debug.h"

namespace Debug {
	
	void printf(EDebugType type, EDebugCode code, const char* file, const char* func, int line, const char* format, ...) {
		#ifdef NDEBUG
		return;	
		#endif
		
		char buffer[DEBUG_BUFFER_SIZE];
		
		va_list argptr;
		va_start(argptr, format);
		vsprintf(buffer, format, argptr);
		va_end(argptr);

		Debug::print(type, code, file, func, line, buffer);
	}	
	
	void print(EDebugType type, EDebugCode code, const char* file, const char* func, int line, const char* msg) {
		#ifdef NDEBUG
		return;	
		#endif

		const char* errtype;

		switch (type) {
			#ifdef DEBUG_FATAL
			case EDebugType::FATAL:
				errtype = "FATAL";
				break;
			#endif
			
			#ifdef DEBUG_ERROR
			case EDebugType::ERROR:
				errtype = "ERROR";
				break;
			#endif
			
			#ifdef DEBUG_WARNING
			case EDebugType::WARNING:	errtype = "WARNING";
				errtype = "WARNING";
				break;
			#endif
			
			#ifdef DEBUG_MESSAGE
			case EDebugType::MESSAGE:
				errtype = "MESSAGE";
				break;
			#endif

			default: return;
		}

		fprintf(stderr, "%s:%d %s (%d): in function %s\n", file, line, errtype, code, func);
		fprintf(stderr, "--- %s\n", msg);
	
		if (type == EDebugType::FATAL)
			exit(1);
	}	
}

