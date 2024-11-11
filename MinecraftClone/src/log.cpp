#include "log.hpp"

#ifdef _WIN32
#	include <windows.h>

void SetConsoleColor(u32 color) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color); }

#else

void SetConsoleColor(u32 color) {}

#endif