// utility.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Various little utility functions, macros & typedefs.

#include "base/utility.h"

#ifdef _WIN32
#ifndef NDEBUG

//int	tu_testbed_assert_break(const char* filename, int linenum, const char* expression)
//{
	// @@ TODO output print error message
//	__asm { int 3 }
//	return 0;
//}

#endif // not NDEBUG
#endif // _WIN32

// check existense of a file
bool exist(const char* path)
{
	FILE* fi = fopen(path, "r");
	if (fi)
	{
		fclose(fi);
		return true;
	}
	return false;
}



