#include "preprocessor.hpp"
#include "tools.hpp"

int include_paths_count = 0;
char* include_paths[MAX_INCLUDE_PATHS] = {};

#define SUBMIT_LINE(line) _lines.push_back(line)
#define SUBMIT_LINE_F(format, ...) _lines.push_back(tools::fstr(format, __VA_ARGS__))
#define IN_FALSE_BRANCH (_branches->size() && !_branches->top())
#define LINE_MARKER(line) SUBMIT_LINE_F("# %d \"%s\"", line, _current_file.c_str())

Status Preprocessor::preprocess(string infile, const char** source)
{
	// prepare sum shit
	_lines = vector<string>();
	_current_file = infile;
	_current_line_no = 0;
	_branches = new stack<bool>();
	_apply_depth = 0;
	_error_dispatcher = ErrorDispatcher();
	_had_error = false;

	vector<string> lines = tools::split_string(string(*source), "\n");
	lines = remove_comments(lines);

	// do the actual preprocessing
	LINE_MARKER(_current_line_no);
	process_lines(lines);

	// finish up
	string result_source; for(string& ln : _lines) result_source += ln + "\n";
	*source = strdup(result_source.c_str());

	DEBUG_PRINT_MSG("Preprocessor done!");
	// DEBUG_PRINT_F_MSG("Preprocessed source:\n%s", *source);
	return _had_error ? STATUS_PREPROCESS_ERROR : STATUS_SUCCESS;
}

// ===============================================================

void Preprocessor::process_lines(vector<string> lines)
{
	for(int line_idx = 0; line_idx < lines.size(); line_idx++)
	{
		_current_line_no++;

		string line_str = strip_start(lines[line_idx]);
		
		if(line_str[0] == '#') // handle directives
			handle_directive(strip_start(line_str.erase(0, 1)), _current_line_no);
		else if(IN_FALSE_BRANCH)
			SUBMIT_LINE("");
		
		else SUBMIT_LINE(lines[line_idx]);
	}
}

vector<string> Preprocessor::remove_comments(vector<string> lines)
{
	bool block_comment = false;

	for(auto& line : lines)
	{
		for(int i = 0; i < line.length(); i++)
		{
			if(block_comment)
			{
				if(i + 1 < line.length() && line[i] == ':' && line[i + 1] == '\\')
				{
					block_comment = false;
					line[i] = ' ';
					line[++i] = ' ';
				}
				else line[i] = ' ';
			}
			else if(i + 1 < line.length() && line[i] == '\\' && line[i + 1] == ':') // start block comment
			{
				block_comment = true;
				line[i] = ' ';
				line[++i] = ' ';
			}
			else if(line[i] == '\\')  // line comment
				while(i < line.length()) line[i++] = ' ';
		}
	}

	return lines;
}

string Preprocessor::find_header(string name)
{
	name += ".evi";
	string hname = name + ".hevi";

	// first look in current directory
	if(ifstream(name).is_open()) return name;
	else if(ifstream(hname).is_open()) return hname;

	// look in each include path
	for(int i = 0; i < include_paths_count; i++)
	{
		string path = include_paths[i] + (PATH_SEPARATOR + name);
		string hpath = include_paths[i] + (PATH_SEPARATOR + hname);

		if(ifstream(path).is_open()) return path;
		else if(ifstream(hpath).is_open()) return hpath;
	}

	// otherwise look in stdlib
	if(ifstream(STDLIB_DIR + name).is_open()) return STDLIB_DIR + name;
	else if(ifstream(STDLIB_DIR + hname).is_open()) return STDLIB_DIR + hname;

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
	str->erase(0, i);
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
	str->erase(0, i + 2);
	return true;
}

// errors handled in the function
bool Preprocessor::consume_integer(string* str, uint* dest, uint line)
{
	*str = strip_start(*str);

	int i = 0;

	// get digits
	while(isdigit((*str)[i])) i++;

	// check if correct
	if(i != str->length() && (*str)[i] != ' ')
	{
		ERROR_F(line, "Expected integer, not '%s'.", str->substr(0, str->find(' ')).c_str());
		return false;
	}

	// get integer and finish
	string intstr = string(*str, 0, i);
	*dest = strtol(intstr.c_str(), NULL, 10);
	// DEBUG_PRINT_F_MSG("int consumed: %d", *dest);

	str->erase(0, i);
	return true;
}

// ===============================================================

Preprocessor::DirectiveType Preprocessor::get_directive_type(string str)
{
		 if(str == "apply")  return DIR_APPLY;
	else if(str == "info")	 return DIR_INFO;
	else if(str == "file")	 return DIR_FILE;
	else if(str == "line")	 return DIR_LINE;

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
		CASE(DIR_FILE, file);
		CASE(DIR_LINE, line);

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

void Preprocessor::handle_directive(string line, uint line_no)
{
	// get first word and make sure it's a valid directive
	string first_word = line.substr(0, line.find(' '));
	DirectiveType dirtype = get_directive_type(first_word);

 	if(dirtype == DIR_INVALID)
	{
		if(!IN_FALSE_BRANCH)
			ERROR_F(line_no, "Invalid preprocessor directive: '#%s'.", first_word.c_str())
		else SUBMIT_LINE("");
		return; // report the error but keep on chuckin'
	}

	// directive is valid, so handle it
	DirectiveHandler handler = get_directive_handler(dirtype);
	(this->*handler)(strip_start(line.erase(0, first_word.length())), line_no - 1);
}

// ===============================================================

bool Preprocessor::handle_pragma(vector<string> args, uint line_no)
{
	string cmd = args[0];
	args.erase(args.begin());

	if(cmd == "apply_once")
	{
		_blocked_files.push_back(_current_file);
		return args.size() == 0;
	}
	else if(cmd == "error")
	{
		string msg;
		consume_string(&args[0], &msg, line_no);

		if(args.size() != 1) return false;

		ERROR(line_no, msg.c_str());
		return true;
	}
	else if(cmd == "warning")
	{
		string msg;
		consume_string(&args[0], &msg, line_no);

		if(args.size() != 1) return false;

		_error_dispatcher.warning_at_line(line_no, _current_file.c_str(), "Preprocessor Warning", msg.c_str());
		return true;
	}

	return false;
}

// ===============================================================

#define HANDLER(name) void Preprocessor::handle_directive_##name(string line, uint line_no)
#define TRY_TO(code) { if(!(code)) return; }
#define CHECK_FLAG(flag) (std::find(_flags.begin(), _flags.end(), flag) != _flags.end())
#define ASSERT_END_OF_LINE() { if(strip_start(line).length()) ERROR(line_no, "Expected newline"); return; }

HANDLER(apply)
{
	if(IN_FALSE_BRANCH) { SUBMIT_LINE(""); return; }

	string oldfile = _current_file;
	uint old_lineno = _current_line_no;
	_apply_depth++;

	// get filename
	string header;
	TRY_TO(consume_string(&line, &header, line_no));

	// find and read file
	string path = find_header(header);

	if(!path.length()) // find_header failed
	{
		ERROR_F(line_no, "Could not find header '%s'.", header.c_str());
		return;
	}
	else if(find(_blocked_files.begin(), _blocked_files.end(), path) != _blocked_files.end())
	{
		// file #info apply_once'd
		SUBMIT_LINE("");
		return;
	}
	else if(_apply_depth > MAX_APPLY_DEPTH) // too deep
	{
		ERROR_F(line_no, "Inclusion depth surpassed limit of %d.", MAX_APPLY_DEPTH);
		return;
	}
	
	// DEBUG_PRINT_F_MSG("Found header '%s' at '%s'.", header.c_str(), path.c_str());
	string source = tools::readf(path);

	// process text
	_current_file = path;
	vector<string> lines = tools::split_string(source, "\n");
	lines = remove_comments(lines);
	LINE_MARKER(0);
	process_lines(lines);

	// continue current file
	_current_file = oldfile;
	_current_line_no = old_lineno;
	LINE_MARKER(_current_line_no);

	ASSERT_END_OF_LINE();
}

HANDLER(info)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;

	// get arguments and check if there are any
	vector<string> args = tools::split_string(line, " ");
	if(!args.size() || !args[0].length())
	{
		ERROR(line_no, "Expected arguments after #info.");
		return;
	}
	else if(!handle_pragma(args, line_no))
	{
		ERROR_F(line_no, "Invalid arguments for '#info %s'.", args[0].c_str());
		return;
	}
}

HANDLER(file)
{
	if(IN_FALSE_BRANCH) { SUBMIT_LINE(""); return; }

	// get filename
	TRY_TO(consume_string(&line, &_current_file, line_no));
	// DEBUG_PRINT_F_MSG("Set filename to '%s'.", _current_file.c_str());
	LINE_MARKER(line_no);

	ASSERT_END_OF_LINE();
}

HANDLER(line)
{
	if(IN_FALSE_BRANCH) { SUBMIT_LINE(""); return; }

	// get lineno
	TRY_TO(consume_integer(&line, &_current_line_no, line_no));
	LINE_MARKER(_current_line_no);

	ASSERT_END_OF_LINE();
}


HANDLER(flag)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;

	string flag;
	TRY_TO(consume_identifier(&line, &flag, line_no));

	if(CHECK_FLAG(flag))
	{
		ERROR_F(line_no, "Flag '%s' is already set.", flag.c_str());
		return;
	}
	
	_flags.push_back(flag);
	// DEBUG_PRINT_F_MSG("Set flag '%s'", flag.c_str());

	ASSERT_END_OF_LINE();
}

HANDLER(unflag)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;

	string flag;
	TRY_TO(consume_identifier(&line, &flag, line_no));

	if(!CHECK_FLAG(flag))
	{
		ERROR_F(line_no, "Flag '%s' is not set.", flag.c_str());
		return;
	}
	
	_flags.erase(find(_flags.begin(), _flags.end(), flag));
	// DEBUG_PRINT_F_MSG("Unset flag '%s'", flag.c_str());

	ASSERT_END_OF_LINE();
}


HANDLER(ifset)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) { _branches->push(false); return; }

	string flag;
	TRY_TO(consume_identifier(&line, &flag, line_no));
	// DEBUG_PRINT_F_MSG("Started if-branch with '%s'", flag.c_str());
	_branches->push(CHECK_FLAG(flag));
	
	ASSERT_END_OF_LINE();
}

HANDLER(ifnset)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) { _branches->push(false); return; }

	string flag;
	TRY_TO(consume_identifier(&line, &flag, line_no));
	// DEBUG_PRINT_F_MSG("Started if-not-branch with '%s'", flag.c_str());
	_branches->push(!CHECK_FLAG(flag));

	ASSERT_END_OF_LINE();
}

HANDLER(else)
{
	if(!_branches->size())
	{
		ERROR(line_no, "Stray '#else'.");
		return;
	}
	_branches->top() = !_branches->top();
	SUBMIT_LINE("");

	ASSERT_END_OF_LINE();
}

HANDLER(endif)
{
	if(!_branches->size())
	{
		ERROR(line_no, "Stray '#endif'.");
		return;
	}
	_branches->pop();
	SUBMIT_LINE("");

	ASSERT_END_OF_LINE();
}

#undef SUBMIT_LINE
#undef SUBMIT_LINE_F
#undef LINE_MARKER
#undef IN_FALSE_BRANCH
#undef HANDLER
#undef TRY_TO
#undef CHECK_FLAG
#undef ASSERT_END_OF_LINE