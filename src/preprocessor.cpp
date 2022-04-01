#include "preprocessor.hpp"
#include "tools.hpp"
#include <regex>

int include_paths_count = 0;
char* include_paths[MAX_INCLUDE_PATHS] = {};

#define SUBMIT_LINE(line) _lines.push_back(line)
#define SUBMIT_LINE_F(format, ...) _lines.push_back(tools::fstr(format, __VA_ARGS__))
#define IN_FALSE_BRANCH (_branches->size() && !_branches->top())
#define LINE_MARKER(line) SUBMIT_LINE_F("# %d \"%s\"", line, _current_file.c_str())
#define CHECK_MACRO(macro) (_macros->find(macro) != _macros->end())

Status Preprocessor::preprocess(string infile, const char** source)
{
	// prepare sum shit
	_source = *source;
	_lines = vector<string>();
	_current_file = infile;
	_current_line_no = 0;
	_branches = new stack<bool>();
	_macros = new map<string, MacroProperties>();
	_apply_depth = 0;
	_error_dispatcher = ErrorDispatcher();
	_had_error = false;

	initialize_state_singleton(this);
	initialize_builtin_macros();

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

		_current_original_line = lines[line_idx];
		string line_str = strip_start(lines[line_idx]);
		
		if(line_str[0] == '#') // handle directives
			handle_directive(strip_start(line_str.erase(0, 1)), _current_line_no);
		else if(IN_FALSE_BRANCH)
			SUBMIT_LINE("");
		
		else SUBMIT_LINE(handle_plain_line(lines[line_idx]));
	}

	if(_branches->size()) error_at_line(_current_line_no, "Expected #endif.", "");
}

string Preprocessor::handle_plain_line(string line)
{
	cmatch match; // index 0 is whole match
	regex regexp(MACRO_INVOKE_REGEX);

	while(regex_search(line.c_str(), match, regexp))
	{
		string macro = match[1].str();

		// check if macro exists
		if(!CHECK_MACRO(macro))
		{
			Token tok = generate_token(line, macro + '#');
			const char* msg = strdup(tools::fstr("Macro '%s' is not defined.", macro.c_str()).c_str());

			_error_dispatcher.error_at_token(&tok, "Preprocessing Error", msg);
			cerr << endl;
			_error_dispatcher.print_token_marked(&tok, COLOR_RED);

			_had_error = true;
			return line;
		}

		// get macro properties
		MacroProperties props = _macros->at(macro);
		string format = props.has_getter ? (props.getter)() : props.format;

		// replace 
		line = regex_replace(line, regexp, format);
	}
	return line;
}

vector<string> Preprocessor::remove_comments(vector<string> lines)
{
	bool in_block_comment = false;
	bool in_string = false;
	bool in_character = false;

	for(auto& line : lines)
	{
		for(int i = 0; i < line.length(); i++)
		{
			if(in_block_comment)
			{
				if(i + 1 < line.length() && line[i] == ':' && line[i + 1] == '\\')
				{
					in_block_comment = false;
					line[i] = ' ';
					line[++i] = ' ';
				}
				else line[i] = ' ';
			}
			else if(in_string)
			{
				// check for end of string
				if(line[i] == '"' && line[i - 1] != '\\') in_string = false;
			}
			else if(in_character)
			{
				// check for end of character
				if(line[i] == '\'' && line[i - 1] != '\\') in_character = false;
			}
			else if(i + 1 < line.length() && line[i] == '\\' && line[i + 1] == ':') // start block comment
			{
				in_block_comment = true;
				line[i] = ' ';
				line[++i] = ' ';
			}
			else if(line[i] == '\\')  // line comment
			{
				// just rest with whitespaces
				while(i < line.length()) line[i++] = ' ';
			}
			else if(line[i] == '"') in_string = true; // start string 
			else if(line[i] == '\'') in_character = true; // start char 
		}
	}

	return lines;
}

// ===============================================================

void Preprocessor::error_at_line(uint line, const char* message, string whole_line)
{
	if(lint_args.type == LINT_GET_DIAGNOSTICS)
	{
		LINT_OUTPUT_START_PLAIN_OBJECT();

		LINT_OUTPUT_PAIR("file", _current_file);
		LINT_OUTPUT_PAIR_F("line", line, %d);
		LINT_OUTPUT_PAIR_F("column", 0, %d);
		LINT_OUTPUT_PAIR_F("length", 0, %d);
		LINT_OUTPUT_PAIR("message", tools::replacestr(message, "\"", "\\\""));
		LINT_OUTPUT_PAIR("type", "error");

		LINT_OUTPUT_ARRAY_START("related");
		lint_output_diagnostic_object_end();
	}
	else if(lint_args.type == LINT_NONE)
	{
		_error_dispatcher.error_at_line(line, _current_file.c_str(), "Preprocessing Error", message);
		if(whole_line.length())
		{
			cerr << endl;
			_error_dispatcher.print_line_marked(line, whole_line, COLOR_RED);
		}
		_had_error = true;
	}
}

void Preprocessor::error_at_token(Token* token, const char* message)
{
	_error_dispatcher.error_at_token(token, "Preprocessing Error", message);
	cerr << endl;
	_error_dispatcher.print_token_marked(token, COLOR_RED);

	_had_error = true;
}

void Preprocessor::warning_at_line(uint line, const char* message, string whole_line)
{
	if(lint_args.type == LINT_GET_DIAGNOSTICS)
	{
		LINT_OUTPUT_START_PLAIN_OBJECT();

		LINT_OUTPUT_PAIR("file", _current_file);
		LINT_OUTPUT_PAIR_F("line", line, %d);
		LINT_OUTPUT_PAIR_F("column", 0, %d);
		LINT_OUTPUT_PAIR_F("length", 0, %d);
		LINT_OUTPUT_PAIR("message", tools::replacestr(message, "\"", "\\\""));
		LINT_OUTPUT_PAIR("type", "warning");

		LINT_OUTPUT_ARRAY_START("related");
		lint_output_diagnostic_object_end();
	}
	else if(lint_args.type == LINT_NONE)
	{
		_error_dispatcher.warning_at_line(line, _current_file.c_str(), "Preprocessing Warning", message);
		if(whole_line.length())
		{
			cerr << endl;
			_error_dispatcher.print_line_marked(line, whole_line, COLOR_PURPLE);
		}
	}
}

void Preprocessor::warning_at_token(Token* token, const char* message)
{
	_error_dispatcher.warning_at_token(token, "Preprocessing Warning", message);
	cerr << endl;
	_error_dispatcher.print_token_marked(token, COLOR_PURPLE);
}

// will assume that the first appearance of token is the correct one
Token Preprocessor::generate_token(string line, string token)
{
	int offset = 0;
	while(line.substr(offset, token.length()) != token) offset++;
	const char* src = strdup(line.c_str());
	return Token{
		TOKEN_ERROR,
		src,
		src + offset,
		(int)token.length(),
		(int)_current_line_no,
		&_current_file
	};
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
bool Preprocessor::consume_identifier(string* str, string* dest, uint line, const char* errformat)
{
	*str = strip_start(*str);

	// find index of first char that doesn't belong to the identifier
	int i;
	for(i = 0; (i ? isalnum((*str)[i]) : isalpha((*str)[i])) || (*str)[i] == '_'; i++) {}

	// check if correct
	if(i == 0)
	{
		ERROR_F(line, errformat, *str, str->substr(0, str->find(' ')).c_str());
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
		Token tok = generate_token(*str, str->substr(0, str->find(' ')));
		error_at_token(&tok, "Expected string, but found no matching '\"'.");
		return false;
	}

	// find index of second "
	int i;
	for(i = 1; i < str->length() && (*str)[i] != '"'; i++) {
		// cout << tools::fstr("in string: '%c'. (%d < %d)\n", (*str)[i], i, str->length());
	}

	// check if correct
	if(i == str->length())
	{
		Token tok = generate_token(*str, str->substr(0, str->find(' ')));
		error_at_token(&tok, "Expected string.");
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
		ERROR_F(line, *str, "Expected integer, not '%s'.", str->substr(0, str->find(' ')).c_str());
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

	else if(str == "macro")	 return DIR_MACRO;
	else if(str == "undef")	 return DIR_UNDEF;

	else if(str == "flag")	 return DIR_FLAG;
	else if(str == "unset")  return DIR_UNSET;

	else if(str == "ifset")	 return DIR_IFSET;
	else if(str == "ifnset") return DIR_IFNSET;
	else if(str == "ifdef")	 return DIR_IFDEF;
	else if(str == "ifndef") return DIR_IFNDEF;
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

		CASE(DIR_MACRO, macro);
		CASE(DIR_UNDEF, undef);

		CASE(DIR_FLAG, flag);
		CASE(DIR_UNSET, unset);

		CASE(DIR_IFSET, ifset);
		CASE(DIR_IFNSET, ifnset);
		CASE(DIR_IFDEF, ifdef);
		CASE(DIR_IFNDEF, ifndef);
		CASE(DIR_ELSE, else);
		CASE(DIR_ENDIF, endif);

		default: THROW_INTERNAL_ERROR("during preprocessing");
	}
	#undef CASE
	return nullptr;
}

void Preprocessor::handle_directive(string line, uint line_no)
{
	// get first word and make sure it's a valid directive
	string first_word = line.substr(0, line.find(' '));
	DirectiveType dirtype = get_directive_type(first_word);

 	if(dirtype == DIR_INVALID)
	{
		if(!IN_FALSE_BRANCH) ERROR_F(line_no, _current_original_line, "Invalid preprocessor directive: '#%s'.", first_word.c_str())
		else SUBMIT_LINE("");
		return; // report the error but keep on chuckin'
	}

	// directive is valid, so handle it
	DirectiveHandler handler = get_directive_handler(dirtype);
	(this->*handler)(strip_start(line.erase(0, first_word.length())));
}

// ===============================================================

string Preprocessor::find_header(string basename)
{
	string hname = basename + ".hevi";
	string name = basename + ".evi";

	// first look in current directory
	if(ifstream(hname).is_open()) return hname;
	else if(ifstream(name).is_open()) return name;

	// look in each include path
	for(int i = 0; i < include_paths_count; i++)
	{
		string hpath = include_paths[i] + (PATH_SEPARATOR + hname);
		string path = include_paths[i] + (PATH_SEPARATOR + name);

		if(ifstream(hpath).is_open()) return hpath;
		else if(ifstream(path).is_open()) return path;
	}

	// otherwise look in stdlib
	string hpath = STDLIB_DIR + (PATH_SEPARATOR + hname);
	string path = STDLIB_DIR + (PATH_SEPARATOR + name);

 	if(ifstream(hpath).is_open()) return hpath;
	else if(ifstream(path).is_open()) return path;

	return "";
}

Preprocessor::PragmaStatus Preprocessor::handle_pragma(string pragma, string args, uint line_no)
{
	Token __err_tok; // used by END_OF_LINE macro
	#define END_OF_LINE (strip_start(args).length() > 0 ? (\
		error_at_token((__err_tok = generate_token(_current_original_line, args), &__err_tok), \
		"Expected newline."), false) : true)

	if(pragma == "apply_once")
	{
		_blocked_files.push_back(_current_file);
		return END_OF_LINE ? PRAGMA_SUCCESS : PRAGMA_NO_NEWLINE;
	}
	else if(pragma == "error")
	{
		string msg;
		if(!consume_string(&args, &msg, line_no)) return PRAGMA_ERROR_HANDLED;
		return END_OF_LINE ? PRAGMA_SUCCESS : PRAGMA_NO_NEWLINE;

		ERROR(line_no, "", msg.c_str());
		return PRAGMA_SUCCESS;
	}
	else if(pragma == "warning")
	{
		string msg;
		if(!consume_string(&args, &msg, line_no)) return PRAGMA_ERROR_HANDLED;
		if(!END_OF_LINE) return PRAGMA_NO_NEWLINE;

		WARNING(line_no, "", msg.c_str());
		return PRAGMA_SUCCESS;
	}
	else if(pragma == "region")
	{
		return END_OF_LINE ? PRAGMA_SUCCESS : PRAGMA_NO_NEWLINE;
	}
	else if(pragma == "endregion")
	{
		return END_OF_LINE ? PRAGMA_SUCCESS : PRAGMA_NO_NEWLINE;
	}

	return PRAGMA_NONEXISTENT;
	#undef END_OF_LINE
}

// ===============================================================

#define HANDLER(name) void Preprocessor::handle_directive_##name(string line)
#define TRY_TO(code) { if(!(code)) return; }
#define CHECK_FLAG(flag) (std::find(_flags.begin(), _flags.end(), flag) != _flags.end())
#define ASSERT_END_OF_LINE() { if(strip_start(line).length()) ERROR(_current_line_no, line, "Expected newline"); return; }

HANDLER(apply)
{
	if(IN_FALSE_BRANCH) { SUBMIT_LINE(""); return; }
	

	string oldfile = _current_file;
	uint old_lineno = _current_line_no;
	_apply_depth++;

	// get filename
	string header;
	TRY_TO(consume_string(&line, &header, _current_line_no));

	// find and read file
	string path = find_header(header);

	if(!path.length()) // find_header failed
	{
		ERROR_F(_current_line_no, _current_original_line, "Could not find header '%s'.", header.c_str());
		SUBMIT_LINE("");
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
		ERROR_F(_current_line_no, _current_original_line, "Inclusion depth surpassed limit of %d.", MAX_APPLY_DEPTH);
		SUBMIT_LINE("");
		return;
	}
	
	// DEBUG_PRINT_F_MSG("Found header '%s' at '%s'.", header.c_str(), path.c_str());
	string source = tools::readf(path);

	_current_file = path;
	_current_line_no = 0;

	// process text
	vector<string> lines = tools::split_string(source, "\n");
	lines = remove_comments(lines);
	LINE_MARKER(0);
	process_lines(lines);

	// continue current file
	_current_file = oldfile;
	_current_line_no = old_lineno;
	_apply_depth--;

	LINE_MARKER(_current_line_no);
	ASSERT_END_OF_LINE();
}

HANDLER(info)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;

	// get pragma and arguments and check if there are any
	string pragma; if(!consume_identifier(&line, &pragma, _current_line_no))
	{
		ERROR(_current_line_no, _current_original_line, "Expected pragma after '#info'.");
		return;
	}

	PragmaStatus status = handle_pragma(pragma, line, _current_line_no);
	switch(status)
	{
		case PRAGMA_SUCCESS: return;
		case PRAGMA_ERROR_HANDLED: return;
		case PRAGMA_NONEXISTENT:
		{
			Token tok = generate_token(_current_original_line, pragma);
			warning_at_token(&tok, tools::fstr("Nonexistent pragma '%s' ignored.", pragma.c_str()).c_str());
			return;
		}
		case PRAGMA_INVALID_ARGS:
		{
			Token tok = generate_token(_current_original_line, pragma);
			error_at_token(&tok, tools::fstr("Invalid arguments for '#info %s'.", pragma.c_str()).c_str());
			return;
		}
		case PRAGMA_NO_NEWLINE: return;
	}
}


HANDLER(file)
{
	if(IN_FALSE_BRANCH) { SUBMIT_LINE(""); return; }

	// get filename
	TRY_TO(consume_string(&line, &_current_file, _current_line_no));
	LINE_MARKER(_current_line_no);

	ASSERT_END_OF_LINE();
}

HANDLER(line)
{
	if(IN_FALSE_BRANCH) { SUBMIT_LINE(""); return; }

	// get lineno
	TRY_TO(consume_integer(&line, &_current_line_no, _current_line_no));
	LINE_MARKER(_current_line_no);

	ASSERT_END_OF_LINE();
}


HANDLER(macro)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;

	string ident;
	TRY_TO(consume_identifier(&line, &ident, _current_line_no));

	if(CHECK_FLAG(ident))
	{
		ERROR(_current_line_no, _current_original_line, "Flag with identical name already set.");
		return;
	}
	else if(CHECK_MACRO(ident))
	{
		WARNING(_current_line_no, _current_original_line, "Macro with identical name already defined.");
		return;
	}

	_macros->insert(pair<string, MacroProperties>(ident, {false, strip_start(line), nullptr}));
}

HANDLER(undef)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;
	
	string macro;
	TRY_TO(consume_identifier(&line, &macro, _current_line_no));

	if(!CHECK_MACRO(macro))
	{
		ERROR_F(_current_line_no, _current_original_line, "Macro '%s' is not defined.", macro.c_str());
		return;
	}
	
	_macros->erase(macro);

	ASSERT_END_OF_LINE();
}

HANDLER(flag)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;

	string flag;
	TRY_TO(consume_identifier(&line, &flag, _current_line_no));

	if(CHECK_FLAG(flag))
	{
		WARNING_F(_current_line_no, _current_original_line, "Flag '%s' is already set.", flag.c_str());
		return;
	}
	else if(CHECK_MACRO(flag))
	{
		ERROR(_current_line_no, _current_original_line, "Macro with identical name already defined.");
		return;
	}
	
	if(!CHECK_FLAG(flag)) _flags.push_back(flag);

	ASSERT_END_OF_LINE();
}

HANDLER(unset)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) return;
	
	string flag;
	TRY_TO(consume_identifier(&line, &flag, _current_line_no));

	if(!CHECK_FLAG(flag))
	{
		ERROR_F(_current_line_no, _current_original_line, "Flag '%s' is not set.", flag.c_str());
		return;
	}
	
	_flags.erase(find(_flags.begin(), _flags.end(), flag));

	ASSERT_END_OF_LINE();
}


HANDLER(ifset)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) { _branches->push(false); return; }

	string flag;
	TRY_TO(consume_identifier(&line, &flag, _current_line_no));
	_branches->push(CHECK_FLAG(flag));
	
	ASSERT_END_OF_LINE();
}

HANDLER(ifnset)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) { _branches->push(false); return; }

	string flag;
	TRY_TO(consume_identifier(&line, &flag, _current_line_no));
	_branches->push(!CHECK_FLAG(flag));

	ASSERT_END_OF_LINE();
}

HANDLER(ifdef)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) { _branches->push(false); return; }

	string flag;
	TRY_TO(consume_identifier(&line, &flag, _current_line_no));
	_branches->push(CHECK_MACRO(flag));
	
	ASSERT_END_OF_LINE();
}

HANDLER(ifndef)
{
	SUBMIT_LINE("");
	if(IN_FALSE_BRANCH) { _branches->push(false); return; }

	string flag;
	TRY_TO(consume_identifier(&line, &flag, _current_line_no));
	_branches->push(!CHECK_MACRO(flag));

	ASSERT_END_OF_LINE();
}

HANDLER(else)
{
	if(!_branches->size())
	{
		ERROR(_current_line_no, _current_original_line, "Stray '#else'.");
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
		ERROR(_current_line_no, _current_original_line, "Stray '#endif'.");
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
#undef CHECK_MACRO
#undef ASSERT_END_OF_LINE