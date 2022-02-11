#include "preprocessor.hpp"
#include "tools.hpp"

Status Preprocessor::preprocess(string infile, const char* source)
{
	// prepare sum shit
	_error_dispatcher = ErrorDispatcher(source, infile.c_str());
	_lines = tools::split_string(string(source), "\n");

	// do the actual preprocessing
	for(int line_idx = 0; line_idx < _lines.size(); line_idx++)
	{
		string line_str = strip_start(_lines[line_idx]);
		if(line_str[0] == '#')
		{
			DEBUG_PRINT_F_MSG("processing line %2d: %s", line_idx + 1, line_str.c_str());
			handle_directive(line_str.erase(0, 1));
		}
	}


	// finish up
	DEBUG_PRINT_MSG("Preprocessor done!");
	source = strdup(_result_source.c_str());
	/* temp */ exit(0);
	return STATUS_SUCCESS;
}

// ===============================================================

string Preprocessor::strip_start(string str)
{
	while(str[0] == ' '
	   || str[0] == '\t'
	   || str[0] == '\r')
		str.erase(0, 1);
	return str;
}

// ===============================================================

Preprocessor::DirectiveType Preprocessor::get_directive_type(string str)
{
		 if(str == "apply") return DIR_APPLY;
	else if(str == "flag")	return DIR_FLAG;
	else return DIR_INVALID;
}

Preprocessor::DirectiveFn Preprocessor::get_directive_fn(DirectiveType type)
{
	switch(type)
	{
		// case DIR_APPLY: 
	}
}

void Preprocessor::handle_directive(string line)
{
	// get first word and make sure it's a valid directive
	string first_word = line.substr(0, line.find(' '));
	DirectiveType dirtpe = get_directive_type(first_word);

	if(dirtype == DIR_INVALID)
	{
		_error_dispatcher.dispatch_error("Preprocessor Error",
			tools::fstr("Invalid preprocessor directive: '#%s'.", first_word.c_str()).c_str());
		return; // report the error but keep on chuckin'
	}

	// directive is valid, so handle it

}