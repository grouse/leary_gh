#include "util.h"

#include <stdlib.h>
#include <stdio.h>

namespace Util {
	char* readfile(const char* file) {
		FILE* fileptr;
		long length;
		char* buffer;

		fileptr = fopen(file, "rb");
		if (!fileptr) {
			DebugPrintf(EDebugType::ERROR, EDebugCode::UTIL_OPEN_FILE, "Failed to open file: %s", file);
			return NULL;
		}

		fseek(fileptr, 0, SEEK_END);
		length = ftell(fileptr);
		buffer = (char*) malloc(length+1);

		fseek(fileptr, 0, SEEK_SET);
		fread(buffer, length, 1, fileptr);
		fclose(fileptr);
		buffer[length] = 0;

		return buffer;
	}

}
