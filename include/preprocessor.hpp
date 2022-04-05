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
	Status preprocess(string infile, ccp* source);

private:

	friend class State;

	// types
	#pragma region types

	typedef enum
	{
		DIR_APPLY,
		DIR_INFO,

		DIR_FILE,
		DIR_LINE,
		
		DIR_MACRO,
		DIR_UNDEF,

		DIR_FLAG,
		DIR_UNSET,

		DIR_IFSET,
		DIR_IFNSET,
		DIR_IFDEF,
		DIR_IFNDEF,
		DIR_ELSE,
		DIR_ENDIF,

		DIR_INVALID
	} DirectiveType;
	typedef void (Preprocessor::*DirectiveHandler)(string);

	typedef string (*BuiltinMacroGetter)();
	typedef struct
	{
		bool has_getter = false;
		string format;
		BuiltinMacroGetter getter;
	} MacroProperties;

	typedef enum
	{
		PRAGMA_SUCCESS,
		PRAGMA_ERROR_HANDLED,
		PRAGMA_NONEXISTENT,
		PRAGMA_INVALID_ARGS,
		PRAGMA_NO_NEWLINE,
	} PragmaStatus;

	#pragma endregion

	// macros
	#pragma region macros
	#define ERROR(lineno, line, msg) { error_at_line(lineno, msg, line); }
	#define ERROR_F(lineno, line, format, ...) { error_at_line(lineno, tools::fstr(format, __VA_ARGS__).c_str(), line); }
	#define WARNING(lineno, line, msg) { warning_at_line(lineno, msg, line); }
	#define WARNING_F(lineno, line, format, ...) { warning_at_line(lineno, tools::fstr(format, __VA_ARGS__).c_str(), line); }
	#define MACRO_INVOKE_REGEX "([a-zA-Z_][a-zA-Z0-9_]*)#"
	#pragma endregion

	// methods
	#pragma region methods
	void initialize_builtin_macros();

	void process_lines(vector<string> lines);
	string handle_plain_line(string line);
	vector<string> remove_comments(vector<string> lines);

	void error_at_line(uint line, ccp message, string whole_line = "");
	void error_at_token(Token* token, ccp message);
	void warning_at_line(uint line, ccp message, string whole_line = "");
	void warning_at_token(Token* token, ccp message);
	Token generate_token(string line, string token);

	string strip_start(string str);
	bool consume_identifier(string* str, string* dest, uint line,
							ccp errformat = "Expected identifier, not '%s'.");
	bool consume_string(string* str, string* dest, uint line);
	bool consume_integer(string* str, uint* dest, uint line);

	DirectiveType get_directive_type(string str);
	DirectiveHandler get_directive_handler(DirectiveType type);
	void handle_directive(string line, uint line_no);

	string find_header(string name);
	PragmaStatus handle_pragma(string pragma, string args, uint line_no);
	#pragma endregion

	// directive handlers
	#pragma region handlers
	#define HANDLER(name) void handle_directive_##name(string line)
		HANDLER(apply);
		HANDLER(info);

		HANDLER(file);
		HANDLER(line);

		HANDLER(macro);
		HANDLER(undef);

		HANDLER(flag);
		HANDLER(unset);

		HANDLER(ifset);
		HANDLER(ifnset);
		HANDLER(ifdef);
		HANDLER(ifndef);
		HANDLER(else);
		HANDLER(endif);
	#undef HANDLER
	#pragma endregion

	// members
	#pragma region members
	ccp _source;
	vector<string> _lines;
	string _current_file;
	uint _current_line_no;
	string _current_original_line;

	vector<string> _flags;
	map<string, MacroProperties>* _macros;
	stack<bool>* _branches;
	
	vector<string> _blocked_files;
	uint _apply_depth;

	bool _had_error;
	ErrorDispatcher _error_dispatcher;
	#pragma endregion
};

// ==== ============= ====

extern void initialize_state_singleton(Preprocessor* p);

#endif