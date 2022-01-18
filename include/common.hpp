#ifndef EVI_COMMON_H
#define EVI_COMMON_H

#include <cstring>
#include <assert.h>

#include "tools.hpp"

using namespace std;

// compiler name
#ifndef COMPILER
#define COMPILER "unknown"
#endif

// OS_NAME name
#ifdef _WIN32
#define OS_NAME "Windows 32-bit"
#elif _WIN64
#define OS_NAME "Windows 64-bit"
#elif __APPLE__ || __MACH__
#define OS_NAME "Mac OS_NAMEX"
#elif __linux__
#define OS_NAME "Linux"
#elif __FreeBSD__
#define OS_NAME "FreeBSD"
#elif __unix || __unix__
#define OS_NAME "Unix"
#else
#define OS_NAME "Other"
#endif

// app info
#define APP_NAME "evi"
#define APP_VERSION "0.0.1"
#define APP_DOC "%s -- The Evi compiler.\nWritten by Sjoerd Vermeulen (%s)\v\
More info at %s.\nBuild: %s %s on %s (%s)."
// format: APP_NAME, EMAIL, LINK, __DATE__, __TIME__, OS_NAME, COMPILER
#define EMAIL "sjoerd@marsenaar.com"
#define LINK "https://github.com/SjVer/Evi-Lang"

// version info
#define EVI_VERSION_0 0
#define EVI_VERSION_1 0
#define EVI_VERSION_2 1

// usefull macros
#define STRINGIFY(value) #value

#define __DEBUG_MARKER(file, line) "[debug:" file ":" STRINGIFY(line) "]"
#define DEBUG_MARKER __DEBUG_MARKER(__FILE__, __LINE__)
#define DEBUG_PRINT_LINE() cout << tools::fstr(\
	DEBUG_MARKER " line %d passed!",__LINE__) << endl
#define DEBUG_PRINT_VAR(value, formatspec) cout << tools::fstr(\
	DEBUG_MARKER " var %s = " #formatspec, #value, value) << endl
#define DEBUG_PRINT_MSG(format, ...) cout << tools::fstr( \
	DEBUG_MARKER " " format, __VA_ARGS__) << endl

// status enum
typedef enum
{
	STATUS_SUCCESS = 0,
	STATUS_COMPILE_ERROR = 65,
} Status;

#endif