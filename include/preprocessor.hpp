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
		_result_source(),
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

	typedef void(Preprocessor::*DirectiveHandler)(string);

	// methods
	string strip_start(string str);

	DirectiveType get_directive_type(string str);
	DirectiveFn get_directive_fn(DirectiveType type);
	void handle_directive(string line);

	// members
	vector<string> _lines;
	string _result_source;
	string _current_file;

	ErrorDispatcher _error_dispatcher;
};

#endif