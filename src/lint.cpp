#include "lint.hpp"
#include "tools.hpp"

lint_args_t lint_args = { LINT_NONE, -1, {0, 0} };
std::string lint_output;

void lint_output_diagnostic_object(Token* token, string message, const char* type)
{
	LINT_OUTPUT_START_PLAIN_OBJECT();
	
	if(token->type == TOKEN_ERROR)
	{
		LINT_OUTPUT_PAIR_F("invalid", "true", %s);
		LINT_OUTPUT_ARRAY_START("related");
		return;
	}

	LINT_OUTPUT_PAIR("file", *token->file);
	LINT_OUTPUT_PAIR_F("line", token->line, %d);
	LINT_OUTPUT_PAIR_F("column", get_token_col(token), %u);
	LINT_OUTPUT_PAIR_F("length", token->length, %d);
	LINT_OUTPUT_PAIR("message", tools::replacestr(message, "\"", "\\\""));
	LINT_OUTPUT_PAIR("type", type);

	LINT_OUTPUT_ARRAY_START("related");
}

void lint_output_diagnostic_object_end()
{
	LINT_OUTPUT_ARRAY_END();
	LINT_OUTPUT_OBJECT_END();
}