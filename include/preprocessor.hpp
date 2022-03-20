#ifndef EVI_PREPROCESSOR_H
#define EVI_PREPROCESSOR_H

#include "common.hpp"
#include "error.hpp"
#include "lint.hpp"
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

	typedef void(Preprocessor::*DirectiveHandler)(string);

	// methods
	#define ERR_PROMPT "Preprocessing Error"
	#define ERROR(line, msg) { error_at_line(line, msg); }
	#define ERROR_F(line, format, ...) { error_at_line(line, tools::fstr(format, __VA_ARGS__).c_str()); }

	void process_lines(vector<string> lines);
	vector<string> remove_comments(vector<string> lines);
	string find_header(string name);

	void error_at_line(uint line, const char* message);

	string strip_start(string str);
	bool consume_identifier(string* str, string* dest, uint line);
	bool consume_string(string* str, string* dest, uint line);
	bool consume_integer(string* str, uint* dest, uint line);

	DirectiveType get_directive_type(string str);
	DirectiveHandler get_directive_handler(DirectiveType type);
	void handle_directive(string line, uint line_no);

	bool handle_pragma(vector<string> args, uint line_no);

	#define HANDLER(name) void handle_directive_##name(string line)
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
	const char* _source;
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