#ifndef EVI_COMMON_H
#define EVI_COMMON_H

#pragma once

#include <cstring>
#include <cctype>
#include <cassert>
#include <csignal>
#include "tools.hpp"

using namespace std;

// c++ compiler name
#ifndef COMPILER
#define COMPILER "unknown"
#endif

// OS_NAME specific
#pragma region
#ifdef _WIN32
	#define OS_NAME "Windows 32-bit"
	#define PATH_SEPARATOR "\\"
#elif _WIN64
	#define OS_NAME "Windows 64-bit"
	#define PATH_SEPARATOR "\\"
#elif __APPLE__ || __MACH__
	#define OS_NAME "Mac OS_NAMEX"
	#define PATH_SEPARATOR "/"
#elif __linux__
	#define OS_NAME "Linux"
	#define PATH_SEPARATOR "/"
#elif __unix || __unix__
	#define OS_NAME "Unix"
	#define PATH_SEPARATOR "/"
#else
	#define OS_NAME "Unknown OS"
	#define PATH_SEPARATOR "/"
#endif
#pragma endregion

#define COLOR_RED "\x1b[1;31m"
#define COLOR_GREEN "\x1b[0;32m"
#define COLOR_PURPLE "\x1b[1;35m"
#define COLOR_NONE "\x1b[0m"
#define COLOR_BOLD "\x1b[1m"

// app info
#pragma region
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

// build target
#ifndef TARGET
#pragma warning "TARGET not set! Defaulting to \"unkown\"."
#define TARGET "unkown"
#endif

#pragma endregion

// macros
#pragma region
#define STRINGIFY(value) #value
#define LINE_MARKER_REGEX " ([0-9]+) \"(.+)\""
// #define FLAG_MARKER_REGEX " f ([0-9]+)"

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

#define ABORT(status) { cerr << tools::fstr("[evi] Aborted with code %d.\n", status); exit(status); }
#define THROW_INTERNAL_ERROR(where) { \
		const char* bn = basename(__FILE__); \
		const char ext = strrchr(bn, '.')[1]; \
		cerr << tools::fstr("[evi:!!!] Internal error %c%c%d%c occurred " where ".", \
							toupper(bn[0]), toupper(bn[1]), __LINE__, toupper(ext)) << endl; \
		cerr << "[evi:!!!] If this error occurs repeatedly please report it on github" << endl; \
		cerr << "[evi:!!!] at https://github.com/SjVer/Evi-Lang/issues/new." << endl; \
		raise(SIGINT); \
	}
#define ASSERT_OR_THROW_INTERNAL_ERROR(condition, where) { if(!(condition)) THROW_INTERNAL_ERROR(where) }

#pragma endregion

// llvm stuff
#define LLVM_MODULE_TOP_NAME "top"
#define TEMP_OBJ_FILE_TEMPLATE "%s.%d.o" // format: sourcefile name and timestamp

// internal shit
#pragma region
// path of c compiler used for linking
#ifndef LD_PATH
#error "LD_PATH must be defined! (e.g. \"/usr/bin/ld\")"
#endif
// directory of evi static library
#ifndef STATICLIB_DIR
#error "STATICLIB_DIR must be defined! (e.g. \"/usr/lib/\")"
#endif
// ld args
#include "ld_args.h"
// stdlib headers directory
#ifndef STDLIB_DIR
#error "STDLIB_DIR must be defined! (e.g. \"/usr/share/evi/\")"
#endif

#pragma endregion

#define POINTER_ALIGNMENT 16
#define MAX_APPLY_DEPTH 255

#define MAX_INCLUDE_PATHS 0xff

// status enum
typedef enum
{
	STATUS_SUCCESS = 0,
	STATUS_CLI_ERROR = 64,
	STATUS_PREPROCESS_ERROR = 65,
	STATUS_PARSE_ERROR = 66,
	STATUS_TYPE_ERROR = 67,
	STATUS_CODEGEN_ERROR = 68,
	STATUS_OUTPUT_ERROR = 69,

	STATUS_INTERNAL_ERROR = -1
} Status;

#endif