#ifndef UTIL_H
#define UTIL_H

#include "debug.h"

struct ID {
	unsigned int id; 	// user/editor defined identifier number
	unsigned int index;	// index in related array
};

namespace Util {
	char* readfile(const char*);
}



#endif
