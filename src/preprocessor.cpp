#include "preprocessor.hpp"
#include "tools.hpp"

#define SUBMIT_LINE(line) _lines.push_back(line)
#define SUBMIT_LINE_F(format, ...) _lines.push_back(tools::fstr(format, __VA_ARGS__))
#define IN_FALSE_BRANCH (_branches->size() && !_branches->top())
#define LINE_MARKER(line) SUBMIT_LINE_F("# %d \"%s\"", line, _current_file.c_str())

Status Preprocessor::preprocess(string infile, const char** source)
{
	// prepare sum shit
	_lines = vector<string>();
	_current_file = infile;
	_branches = new stack<bool>();
	_apply_depth = 0;
	_error_dispatcher = ErrorDispatcher(*source, infile.c_str());
	_had_error = false;

	vector<string> lines = tools::split_string(string(*source), "\n");

	// do the actual preprocessing
	LINE_MARKER(0);
	process_lines(lines);

	// finish up
	DEBUG_PRINT_MSG("Preprocessor done!");

	string result_source; for(string& ln : _lines) result_source += ln + "\n";
	*source = strdup(result_source.c_str());

	// DEBUG_PRINT_F_MSG("Preprocessed source:\n%s", *source);
	return _had_error ? STATUS_PREPROCESS_ERROR : STATUS_SUCCESS;
}

// ===============================================================

void Preprocessor::process_lines(vector<string> lines)
{
	for(int line_idx = 0; line_idx < lines.size(); line_idx++)
	{
		string line_str = strip_start(lines[line_idx]);
		
		if(line_str[0] == '#') // handle directives
		{
			// DEBUG_PRINT_F_MSG("processing line %2d: %s", line_idx + 1, line_str.c_str());
			handle_directive(strip_start(line_str.erase(0, 1)), line_idx);
		}
		
		else if(IN_FALSE_BRANCH)
		{
			// empty line or commented source line?
			SUBMIT_LINE("");
			// SUBMIT_LINE("\\ " + line_str);
		}
		
		else SUBMIT_LINE(lines[line_idx]);
	}
}

string Preprocessor::find_header(string name)
{
	name += ".evi";

	// first look in include path
	if(ifstream(name).is_open())
		return name;

	// otherwise look in stdlib
	if(ifstream(STDLIB_DIR + name).is_open())
		return STDLIB_DIR + name;

	return "";
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

// errors handled in the function
bool Preprocessor::consume_string(string* str, string* dest, uint line)
{
	*str = strip_start(*str);

	// consume first "
	if((*str)[0] != '"')
	{
		ERROR_F(line, "Expected string, not '%s'.", str->substr(0, str->find(' ')).c_str());
		return false;
	}

	// find index of second "
	int i;
	for(i = 1; i < str->length() && (*str)[i] != '"'; i++) {}

	// check if correct
	if(i == str->length())
	{
		ERROR_F(line, "Expected string, not '%s'.", str->substr(0, str->find(' ')).c_str());
		return false;
	}

	// get identifier and finish
	*dest = string(*str, 1, --i);
	return true;
}

// ===============================================================

Preprocessor::DirectiveType Preprocessor::get_directive_type(string str)
{
		 if(str == "apply")  return DIR_APPLY;
	else if(str == "info")	 return DIR_INFO;

	else if(str == "flag")	 return DIR_FLAG;
	else if(str == "unflag") return DIR_UNFLAG;

	else if(str == "ifset")	 return DIR_IFSET;
	else if(str == "ifnset") return DIR_IFNSET;
	else if(str == "else")	 return DIR_ELSE;
	else if(str == "endif")	 return DIR_ENDIF;

	else return DIR_INVALID;
}

Preprocessor::DirectiveHandler Preprocessor::get_directive_handler(DirectiveType type)
{
	#define CASE(TYPE, name) case TYPE: return &Preprocessor::handle_directive_##name
	switch(type)
	{
		CASE(DIR_APPLY, apply);
		CASE(DIR_INFO, info);

		CASE(DIR_FLAG, flag);
		CASE(DIR_UNFLAG, unflag);

		CASE(DIR_IFSET, ifset);
		CASE(DIR_IFNSET, ifnset);
		CASE(DIR_ELSE, else);
		CASE(DIR_ENDIF, endif);

		default: assert(false);
	}
	#undef CASE
}

void Preprocessor::handle_directive(string line, uint line_idx)
{
	// get first word and make sure it's a valid directive
	string first_word = line.substr(0, line.find(' '));
	DirectiveType dirtype = get_directive_type(first_word);

 	if(dirtype == DIR_INVALID)
	{
		if(!IN_FALSE_BRANCH)
			ERROR_F(line_idx + 1, "Invalid preprocessor directive: '#%s'.", first_word.c_str())
		else SUBMIT_LINE("");
		return; // report the error but keep on chuckin'
	}

	// directive is valid, so handle it
	DirectiveHandler handler = get_directive_handler(dirtype);
	(this->*handler)(strip_start(line.erase(0, first_word.length())), line_idx);
}

// ===============================================================

bool Preprocessor::handle_pragma(vector<string> args)
{
	string cmd = args[0];
	args.erase(args.begin());

	if(cmd == "apply_once")
	{
		// file already applied?
		if(find(_applied_files.begin(), _applied_files.end(), _current_file) != _applied_files.end())
		{
			// TODO: this
		}
	}
}

// ===============================================================

#define HANDLER(name) void Preprocessor::handle_directive_##name(string line, uint line_idx)
#define TRY_TO(code) { if(!(code)) return; }
#define CHECK_FLAG(flag) (std::find(_flags.begin(), _flags.end(), flag) != _flags.end())

HANDLER(apply)
{
	if(!IN_FALSE_BRANCH)
	{
		string oldfile = _current_file;
		bool old_apply_once = _apply_once;
		_apply_depth++;

		// get filename
		string header;
		TRY_TO(consume_string(&line, &header, line_idx + 1));

		// find and read file
		string path = find_header(header);

		if(!path.length()) // find_header failed
		{
			ERROR_F(line_idx + 1, "Could not find header '%s'.", header.c_str());
			return;
		}
		else if(_apply_depth > MAX_APPLY_DEPTH) // too deep
		{
			ERROR_F(line_idx + 1, "Inclusion depth surpassed limit of %d.", MAX_APPLY_DEPTH);
			return;
		}
		
		// DEBUG_PRINT_F_MSG("Found header '%s' at '%s'.", header.c_str(), path.c_str());
		string source = tools::readf(path);

		// process text
		_current_file = path;
		_error_dispatcher.set_filename(path.c_str());
		vector<string> lines = tools::split_string(source, "\n");
		LINE_MARKER(0);
		process_lines(lines);
		_applied_files.push_back(path);

		// continue current file
		_current_file = oldfile;
		_apply_once = old_apply_once;
		_error_dispatcher.set_filename(oldfile.c_str());
		LINE_MARKER(line_idx + 1);
	}
	else SUBMIT_LINE("");
}

HANDLER(info)
{
	if(!IN_FALSE_BRANCH)
	{
		// get arguments and check if there are any
		vector<string> args = tools::split_string(line, " ");
		if(!args.size() || !args[0].length())
		{
			ERROR(line_idx + 1, "Expected arguments after #info.");
			return;
		}
		else if(!handle_pragma(args))
		{
			ERROR(line_idx + 1, "Invalid #info arguments.");
			return;
		}
	}
	SUBMIT_LINE("");
}


HANDLER(flag)
{
	if(!IN_FALSE_BRANCH)
	{
		string flag;
		TRY_TO(consume_identifier(&line, &flag, line_idx + 1));

		if(CHECK_FLAG(flag))
		{
			ERROR_F(line_idx + 1, "Flag '%s' is already set.", flag.c_str());
			return;
		}
		
		_flags.push_back(flag);
		// DEBUG_PRINT_F_MSG("Set flag '%s'", flag.c_str());
	}
	SUBMIT_LINE("");
}

HANDLER(unflag)
{
	if(!IN_FALSE_BRANCH)
	{
		string flag;
		TRY_TO(consume_identifier(&line, &flag, line_idx + 1));

		if(!CHECK_FLAG(flag))
		{
			ERROR_F(line_idx + 1, "Flag '%s' is not set.", flag.c_str());
			return;
		}
		
		_flags.erase(find(_flags.begin(), _flags.end(), flag));
		// DEBUG_PRINT_F_MSG("Unset flag '%s'", flag.c_str());
	}
	SUBMIT_LINE("");
}


HANDLER(ifset)
{
	if(!IN_FALSE_BRANCH)
	{
		string flag;
		TRY_TO(consume_identifier(&line, &flag, line_idx + 1));
		// DEBUG_PRINT_F_MSG("Started if-branch with '%s'", flag.c_str());
		_branches->push(CHECK_FLAG(flag));
	}
	else _branches->push(false);
	SUBMIT_LINE("");
}

HANDLER(ifnset)
{
	if(!IN_FALSE_BRANCH)
	{
		string flag;
		TRY_TO(consume_identifier(&line, &flag, line_idx + 1));
		// DEBUG_PRINT_F_MSG("Started if-not-branch with '%s'", flag.c_str());
		_branches->push(!CHECK_FLAG(flag));
	}
	else _branches->push(false);
	SUBMIT_LINE("");
}

HANDLER(else)
{
	if(!_branches->size())
	{
		ERROR(line_idx + 1, "Stray '#else'.");
		return;
	}
	_branches->top() = !_branches->top();
	SUBMIT_LINE("");
}

HANDLER(endif)
{
	if(!_branches->size())
	{
		ERROR(line_idx + 1, "Stray '#endif'.");
		return;
	}
	_branches->pop();
	SUBMIT_LINE("");
}

#undef SUBMIT_LINE
#undef SUBMIT_LINE_F
#undef IN_FALSE_BRANCH
#undef HANDLER
#undef TRY_TO
#undef CHECK_FLAG