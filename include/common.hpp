#ifndef EVI_COMMON_H
#define EVI_COMMON_H

#pragma once

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

#define COLOR_RED "\x1b[1;31m"
#define COLOR_PURPLE "\x1b[1;35m"
#define COLOR_NONE "\x1b[0m"
#define COLOR_BOLD "\x1b[1m"

// app info
#define APP_NAME "evi"
#define APP_VERSION "0.0.1"
#define APP_DOC "%s -- The Evi compiler.\nWritten by Sjoerd Vermeulen (%s)\v\
More information at %s.\nBuild: %s %s on %s (%s)."
// format: APP_NAME, EMAIL, LINK, __DATE__, __TIME__, OS_NAME, COMPILER
#define EMAIL "sjoerd@marsenaar.com"
#define LINK "https://github.com/SjVer/Evi-Lang"

// version info
#define EVI_VERSION_0 0
#define EVI_VERSION_1 0
#define EVI_VERSION_2 1

// usefull macros
#define STRINGIFY(value) #value

#ifdef DEBUG
#define __DEBUG_MARKER(file, line) "[debug:" file ":" STRINGIFY(line) "]"
#define DEBUG_MARKER __DEBUG_MARKER(__FILE__, __LINE__)
#define DEBUG_PRINT_LINE() cout << tools::fstr(\
	DEBUG_MARKER " line %d passed!",__LINE__) << endl
#define DEBUG_PRINT_VAR(value, formatspec) cout << tools::fstr(\
	DEBUG_MARKER " var %s = " #formatspec, #value, value) << endl
#define DEBUG_PRINT_MSG(msg) cout << DEBUG_MARKER " " msg << endl;
#define DEBUG_PRINT_F_MSG(format, ...) cout <<  tools::fstr( \
	DEBUG_MARKER " " format, __VA_ARGS__) << endl
#else
#define DEBUG_MARKER {}
#define DEBUG_PRINT_LINE() {}
#define DEBUG_PRINT_VAR(value, formatspec) {}
#define DEBUG_PRINT_MSG(msg) {}
#define DEBUG_PRINT_F_MSG(format, ...) {}
#endif

// llvm stuff
#define LLVM_MODULE_TOP_NAME "top"
#define TEMP_OBJ_FILE_TEMPLATE "%s.%d.o" // format: sourcefile name and timestamp

#ifndef CC_PATH
#pragma error "CC_PATH must be defined!"
#endif
#ifndef STDLIB_DIR
#pragma error "STDLIB_DIR must be defined! (e.g. \"/usr/lib/\")"
#endif
#define CC_ARGS CC_PATH, infile, "-o", outfile, "-L" STDLIB_DIR, "-levi"
#define CC_ARGC 6

#define POINTER_ALIGNMENT 8

// status enum
typedef enum
{
	STATUS_SUCCESS = 0,
	STATUS_CLI_ERROR = 64,
	STATUS_PARSE_ERROR = 65,
	STATUS_TYPE_ERROR = 65,
	STATUS_CODEGEN_ERROR = 65,
	STATUS_OUTPUT_ERROR = 65
} Status;

#endif