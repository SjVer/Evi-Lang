#include "preprocessor.hpp"
#include "tools.hpp"

Status Preprocessor::preprocess(string infile, const char* source)
{
	// prepare sum shit
	_error_dispatcher = ErrorDispatcher(source, infile.c_str());
	_lines = vector<string>();
	vector<string> lines = tools::split_string(string(source), "\n");

	// do the actual preprocessing
	for(int line_idx = 0; line_idx < lines.size(); line_idx++)
	{
		string line_str = strip_start(lines[line_idx]);
		if(line_str[0] == '#') // handle directives
		{
			DEBUG_PRINT_F_MSG("processing line %2d: %s", line_idx + 1, line_str.c_str());
			handle_directive(strip_start(line_str.erase(0, 1)));
		}
		else _lines.push_back(line_str);
	}

	// finish up
	DEBUG_PRINT_MSG("Preprocessor done!");

	string result_source; for(string& ln : _lines) result_source += ln + "\n";
	source = strdup(result_source.c_str());
	DEBUG_PRINT_F_MSG("Preprocessed source:\n%s", source);
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

// errors handled in the function
bool Preprocessor::consume_identifier(string* str, string* dest, uint line)
{
	*str = strip_start(*str);

	// find index of first char that doesn't belong to the identifier
	int i;
	for(i = 0; (i ? isalnum((*str)[i]) : isalpha((*str)[i])) || (*str)[i] == '_'; i++) {}

	// check if correct
	if(i == 0)
	{
		ERROR_F(line, "Expected identifier, not '%s'.", str->substr(0, str->find(' ')).c_str());
		return false;
	}

	// get identifier and finish
	*dest = string(*str, 0, i);
	return true;
}

// ===============================================================

Preprocessor::DirectiveType Preprocessor::get_directive_type(string str)
{
		 if(str == "apply") return DIR_APPLY;
	else if(str == "flag")	return DIR_FLAG;
	else return DIR_INVALID;
}

Preprocessor::DirectiveHandler Preprocessor::get_directive_handler(DirectiveType type)
{
	#define CASE(TYPE, name) case TYPE: return &Preprocessor::handle_directive_##name
	switch(type)
	{
		CASE(DIR_APPLY, apply);
		CASE(DIR_FLAG, flag);

		default: assert(false);
	}
	#undef CASE
}

void Preprocessor::handle_directive(string line)
{
	// get first word and make sure it's a valid directive
	string first_word = line.substr(0, line.find(' '));
	DirectiveType dirtype = get_directive_type(first_word);

	if(dirtype == DIR_INVALID)
	{
		ERROR_F("Invalid preprocessor directive: '#%s'.", first_word.c_str());
		return; // report the error but keep on chuckin'
	}

	// directive is valid, so handle it
	DirectiveHandler handler = get_directive_handler(dirtype);
	(this->*handler)(strip_start(line.erase(0, first_word.length())));
}

// ===============================================================

#define HANDLER(name) void Preprocessor::handle_directive_##name(string line, uint line_idx)
#define SUBMIT(line) _lines.push_back(line)

HANDLER(apply)
{
}

HANDLER(flag)
{
	string flag;
	if(consume_identifier(&line, &flag, line_idx + 1))
		SUBMIT(tools::fstr("\\ defined '%s'", flag.c_str()));
}

#undef HANDLER
#undef SUBMIT