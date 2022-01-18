#ifndef EVI_COMMON_H
#define EVI_COMMON_H

#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cwalk.hpp>

#include <iostream>

using namespace std;

#include "tools.hpp"

// compiler name
#ifndef COMPILER
#define COMPILER "gcc"
#endif

// OS name
#ifdef _WIN32
#define OS "Windows 32-bit"
#elif _WIN64
#define OS "Windows 64-bit"
#elif __APPLE__ || __MACH__
#define OS "Mac OSX"
#elif __linux__
#define OS "Linux"
#elif __FreeBSD__
#define OS "FreeBSD"
#elif __unix || __unix__
#define OS "Unix"
#else
#define OS "Other"
#endif

// app info
#define APP_NAME "evi"
#define APP_VERSION "0.0.1"
// #define APP_DOC APP_NAME \
// " -- The Evi interpreter.\n\
// Written by Sjoerd Vermeulen ("EMAIL")\v\
// More info at https://github.com/SjVer/Evi.\n\
// Build: " __DATE__ " " __TIME__ " on " OS " (" COMPILER ")."
#define APP_DOC "%s -- The Evi interpreter.\nWritten by Sjoerd Vermeulen (%s)\v\
More info at %s.\nBuild: %s %s on %s (%s)."
// format: APP_NAME, EMAIL, LINK, __DATE__, __TIME__, OS, COMPILER
#define EMAIL "sjoerd@marsenaar.com"
#define LINK "https://github.com/SjVer/Evi"

// version info
#define EVI_VERSION_0 0
#define EVI_VERSION_1 0
#define EVI_VERSION_2 1

#define WELCOME_MESSAGE \
"Evi %d.%d.%d (%s, %s)\n\
[%s %s] on %s\n\
Type \"clear()\" to clear the screen, \
\"exit()\" to quit \
or \"help()\" for more information.\n"

// help message
#define HELP_MESSAGE \
"This is the Evi repl. Here you can run \
commands and test your snippets.\n\
Type \"clear()\" to clear the screen or \"exit()\" to quit.\n\
For more information go to \
%s.\n" // format: LINK

// prompts
#define PROMPT_NORM "evi> "
#define PROMPT_IND  "...  "

#define UINT8_COUNT (UINT8_MAX + 1)

typedef enum
{
	INTERPRET_SUCCESS = 0,
	INTERPRET_COMPILE_ERROR = 65,
	INTERPRET_RUNTIME_ERROR = 70
} Status;

typedef struct
{
	int exitCode;
	Status status;
} InterpretResult;

#endif