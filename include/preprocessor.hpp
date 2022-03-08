#ifndef EVI_PREPROCESSOR_H
#define EVI_PREPROCESSOR_H

#include "common.hpp"
#include "error.hpp"
#include "scanner.hpp"

extern int include_paths_count;
extern char* include_paths[MAX_INCLUDE_PATHS];

// ==== ============= ====

class Preprocessor
{
public:
	Preprocessor():
		_current_file(),
		_had_error(false),
		_error_dispatcher() {}
	Status preprocess(string infile, const char** source);

private:
	// types
	typedef enum
	{
		DIR_APPLY,
		DIR_INFO,
		DIR_FILE,
		DIR_LINE,
		
		DIR_FLAG,
		DIR_UNFLAG,

		DIR_IFSET,
		DIR_IFNSET,
		DIR_ELSE,
		DIR_ENDIF,

		DIR_INVALID
	} DirectiveType;

	typedef void(Preprocessor::*DirectiveHandler)(string, uint);

	// methods
	#define ERR_PROMPT "Preprocessing Error"
	#define ERROR(line, msg) { \
		_error_dispatcher.error_at_line(\
		line, _current_file.c_str(), ERR_PROMPT, msg); _had_error = true; }
	#define ERROR_F(line, format, ...) { _error_dispatcher.error_at_line(\
		line, _current_file.c_str(), ERR_PROMPT, tools::fstr(format, __VA_ARGS__).c_str()); _had_error = true; }

	void process_lines(vector<string> lines);
	string remove_comments(string source);
	string find_header(string name);

	string strip_start(string str);
	bool consume_identifier(string* str, string* dest, uint line);
	bool consume_string(string* str, string* dest, uint line);
	bool consume_integer(string* str, uint* dest, uint line);

	DirectiveType get_directive_type(string str);
	DirectiveHandler get_directive_handler(DirectiveType type);
	void handle_directive(string line, uint line_idx);

	bool handle_pragma(vector<string> args, uint line_idx);

	#define HANDLER(name) void handle_directive_##name(string line, uint line_no)
		HANDLER(apply);
		HANDLER(info);
		HANDLER(file);
		HANDLER(line);

		HANDLER(flag);
		HANDLER(unflag);

		HANDLER(ifset);
		HANDLER(ifnset);
		HANDLER(else);
		HANDLER(endif);
	#undef HANDLER

	// members
	vector<string> _lines;
	string _current_file;
	uint _current_line_no;

	vector<string> _flags;
	stack<bool>* _branches;
	
	vector<string> _blocked_files;
	uint _apply_depth;

	bool _had_error;
	ErrorDispatcher _error_dispatcher;
};

#endif