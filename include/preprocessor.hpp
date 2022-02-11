#ifndef EVI_PREPROCESSOR_H
#define EVI_PREPROCESSOR_H

#include "common.hpp"
#include "error.hpp"
#include "scanner.hpp"

// ==== ============= ====

class Preprocessor
{
public:
	Preprocessor():
		_current_file() {}
	Status preprocess(string infile, const char* source);

private:
	// types
	typedef enum
	{
		DIR_APPLY,
		
		DIR_FLAG,
		DIR_UNFLAG,

		DIR_IFSET,
		DIR_IFNSET,
		DIR_ELSE,
		DIR_ENDIF,

		DIR_INVALID,
		DIR_NONE
	} DirectiveType;

	typedef void(Preprocessor::*DirectiveHandler)(string, uint);

	// methods
	#define ERR_PROMPT "Preprocessing Error"
	#define ERROR(line, msg) { \
		_error_dispatcher.dispatch_error_at_ln(\
		line, ERR_PROMPT, msg); _had_error = true; }
	#define ERROR_F(line, format, ...) { _error_dispatcher.dispatch_error_at_ln(\
		line, ERR_PROMPT, tools::fstr(format, __VA_ARGS__).c_str()); _had_error = true; }

	string strip_start(string str);
	bool consume_identifier(string* str, string* dest, uint line);

	DirectiveType get_directive_type(string str);
	DirectiveHandler get_directive_handler(DirectiveType type);
	void handle_directive(string line);
	void append_line_marker(uint line);

	#define HANDLER(name) void handle_directive_##name(string line, uint line_idx)
		HANDLER(apply);
		HANDLER(flag);
	#undef HANDLER

	// members
	vector<string> _lines;
	string _current_file;

	vector<string> _flags;

	bool _had_error;
	ErrorDispatcher _error_dispatcher;
};

#endif