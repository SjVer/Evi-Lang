#ifndef EVI_LINT_H
#define EVI_LINT_H

#include <string.h>

typedef enum
{
	LINT_GET_FUNCTIONS,
	LINT_GET_ERRORS,
	LINT_GET_VARIABLES,

	LINT_NONE
} LintType;

static const char* get_lint_type_string(LintType type)
{
	#define CASE(type, str) case type: return str
	switch(type)
	{
		CASE(LINT_GET_FUNCTIONS, "functions");
		CASE(LINT_GET_ERRORS, "errors");
		CASE(LINT_GET_VARIABLES, "variables");

		default: return nullptr;
	}
	#undef CASE
}

static LintType get_lint_type(const char* str)
{
	for(int i = 0; i < LINT_NONE; i++)
		if(!strcmp(get_lint_type_string((LintType)i), str))
			return (LintType)i;
	return LINT_NONE;
}

#define LINT_POS_REGEX "^([0-9]+):([0-9]+)$"

typedef struct
{
	LintType type = LINT_NONE;
	int tab_width = -1;
	uint pos[2] = {0, 0};
} lint_args_t;

extern std::string lint_output;

#define WRONG_END (lint_output.length() >= 2 && lint_output.substr(lint_output.length() - 2) == ", ")

#define LINT_OUTPUT_START_PLAIN_OBJECT() { lint_output += "{ "; }
#define LINT_OUTPUT_END_PLAIN_OBJECT() { if(WRONG_END) lint_output.erase(lint_output.end() - 2); lint_output += "}"; }
#define LINT_OUTPUT_OBJECT_START(key) { lint_output += '"' + key + "\": { "; }
#define LINT_OUTPUT_OBJECT_END() { if(WRONG_END) lint_output.erase(lint_output.end() - 2); lint_output += "}, "; }
#define LINT_OUTPUT_PAIR(key, value) { lint_output += '"' + key + "\": \"" + value + "\", "; }

#define LINT_OUTPUT_START_PLAIN_ARRAY() { lint_output += "[ "; }
#define LINT_OUTPUT_END_PLAIN_ARRAY() { if(WRONG_END) lint_output.erase(lint_output.end() - 2); lint_output += "]"; }
#define LINT_OUTPUT_ARRAY_START(key) { lint_output += '"' + key + "\": [ "; }
#define LINT_OUTPUT_ARRAY_END() { if(WRONG_END) lint_output.erase(lint_output.end() - 2); lint_output += "], "; }
#define LINT_OUTPUT_ARRAY_ITEM(value) { lint_output += '"' + value + "\", "; }

#endif