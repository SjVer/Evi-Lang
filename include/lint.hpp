#ifndef EVI_LINT_H
#define EVI_LINT_H

#include <string.h>

typedef enum
{
	LINT_GET_FUNCTIONS,
	LINT_GET_PARAMETERS,
	LINT_GET_VARIABLES,

	LINT_NONE
} LintType;

#define LINT_GET_FUNCTIONS_STR "get-functions"
#define LINT_GET_PARAMETERS_STR "get-parameters"
#define LINT_GET_VARIABLES_STR "get-variables"

static const char* get_lint_type_string(LintType type)
{
	#define CASE(type) case type: return type##_STR
	switch(type)
	{
		CASE(LINT_GET_FUNCTIONS);
		CASE(LINT_GET_PARAMETERS);
		CASE(LINT_GET_VARIABLES);

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
	bool lint = false;
	bool pos_given = false;
	LintType type = LINT_NONE;
	uint pos[2] = {0, 0};
} lint_args_t;

#undef LINT_GET_FUNCTIONS_STR
#undef LINT_GET_PARAMETERS_STR
#undef LINT_GET_VARIABLES_STR

string lint_output = "";

#define LINT_OUTPUT_START() { lint_output += "{ "; }
#define LINT_OUTPUT_OBJECT_START(key) { lint_output += '"' + key + "\": { "; }
#define LINT_OUTPUT_PAIR(key, value) { lint_output += '"' + key + "\": \"" + value + "\", "; }
#define LINT_OUTPUT_OBJECT_END() { lint_output.erase(lint_output.end() - 2); lint_output += " }, "; }
#define LINT_OUTPUT_END() { lint_output.erase(lint_output.end() - 2); lint_output += " }"; }

#endif