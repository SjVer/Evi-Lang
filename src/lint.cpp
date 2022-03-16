#include "lint.hpp"
#include "tools.hpp"

lint_args_t lint_args = { LINT_NONE, -1, {0, 0} };
std::string lint_output;

void lint_output_error_object(Token* token, string message, const char* type)
{
	LINT_OUTPUT_START_PLAIN_OBJECT();

	LINT_OUTPUT_PAIR("file", *token->file);
	LINT_OUTPUT_PAIR_F("line", token->line, %d);
	// LINT_OUTPUT_PAIR_F("column", get_token_col(token, lint_args.tab_width), %d);
	LINT_OUTPUT_PAIR_F("column", get_token_col(token), %d);
	LINT_OUTPUT_PAIR_F("length", token->length, %d);
	LINT_OUTPUT_PAIR("message", tools::replacestr(message, "\"", "\\\""));
	LINT_OUTPUT_PAIR("type", type);

	LINT_OUTPUT_ARRAY_START("related");
}

void lint_output_error_object_end()
{
	LINT_OUTPUT_ARRAY_END();
	LINT_OUTPUT_OBJECT_END();
}