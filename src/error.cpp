#include "error.hpp"

void ErrorDispatcher::set_filename(const char* filename)
{
	// just change it lmao
	this->_infile = strdup(filename);
}

void ErrorDispatcher::dispatch_token_marked(Token *token)
{
	/*
		To find out what lines to print and where those lines
		start we first find the total offset from the start
		of the token, then we go back char by char to find the
		first newline, which would be the start of the line of
		the token. Then we do the same thing but in the opposite
		direction to find the end of the line.
	*/

	string before1 = "";
	string tokenline = "";
	string after1 = "";

	// get offset of token
	ptrdiff_t token_offset = token->start - _source;

	// find first newline before token
	ptrdiff_t tok_ln_begin;
	if(token->line > 1)
	{
		tok_ln_begin = token_offset;
		while(tok_ln_begin > 0 && _source[tok_ln_begin] != '\n') tok_ln_begin--;
		tok_ln_begin++; // skip newline itself
	}
	else tok_ln_begin = 0;

	// get line before token's line if possible
	if (token->line > 1)
	{
		ptrdiff_t bef_ln_begin;
		if(token->line - 1 > 0)
		{
			bef_ln_begin = tok_ln_begin - 2;
			while(bef_ln_begin > 0 && _source[bef_ln_begin] != '\n') bef_ln_begin--;
			bef_ln_begin++; // skip newline itself
		}
		else bef_ln_begin = 0;
		
		string bef_ln = string(_source + bef_ln_begin, tok_ln_begin - bef_ln_begin - 1);
		before1 = tools::fstr("       %2d| %s", token->line - 1, bef_ln.c_str());
	}

	// find first newline after token
	ptrdiff_t tok_ln_end = token_offset + token->length;
	while(_source[tok_ln_end] != '\n' && _source[tok_ln_end] != '\0') tok_ln_end++;

	// get line with marked token
	{
		/*
			Inserting the escape codes for coloring won't work, so instead we find
			length A and B as seen below:

				this is an example line with a WRONG token in it.
				<------------- A ------------->     <---- B ---->

			And use those and the info we have of the token to merge them,
			and the escape codes together in a new string.
		*/

		// find string on the token's line before and after the token itself
		ptrdiff_t tok_ln_before_tok_len = token_offset - tok_ln_begin;
		string tok_ln_before_tok = string(_source + tok_ln_begin, tok_ln_before_tok_len);

		ptrdiff_t tok_ln_after_tok_len = tok_ln_end - token_offset - token->length;
		string tok_ln_after_tok = string(_source + token_offset + token->length, tok_ln_after_tok_len);

		// get token and its full line
		string tok = string(token->start, token->length);
		string tok_ln = tok_ln_before_tok + COLOR_RED + tok + COLOR_NONE + tok_ln_after_tok;
		tokenline = tools::fstr(COLOR_RED "    ->" COLOR_NONE " %2d| %s", token->line, tok_ln.c_str());
	}

	// get line after token's line if and possible
	if(_source[tok_ln_end] != '\0')
	{
		// find first newline after end of token's line;
		ptrdiff_t af_ln_begin = tok_ln_end + 1;                 // skip prev. line's newline;
		ptrdiff_t af_ln_end   = af_ln_begin;                    // skip prev. line's newline;
		while(_source[af_ln_end] != '\n' && _source[af_ln_end] != '\0') af_ln_end++;

		int af_ln_len = af_ln_end - tok_ln_end - 1; // minus trailing newline
		string af_ln = string(_source + af_ln_begin, af_ln_len);
		after1 = tools::fstr("       %2d| %s", token->line + 1, af_ln.c_str());
	}

	// print it all out
	if(before1 	 != "") cerr << before1 << endl;
	if(tokenline != "") cerr << tokenline << endl;
	if(after1 	 != "") cerr << after1 << endl;
}

void ErrorDispatcher::__dispatch(bool at, Token* t, const char* c,
								 const char* p, const char* m, uint line)
{
	if(at) 		  fprintf(stderr, "[%s:%d] %s%s" COLOR_NONE, _infile, t->line, c, p);
	else if(line) fprintf(stderr, "[%s:%d] %s%s" COLOR_NONE, _infile, line, c, p);
	else 		  fprintf(stderr, "[%s] %s%s" COLOR_NONE, _infile, c, p);

    if (at && t->type == TOKEN_EOF) 
		fprintf(stderr, " at end");
	else if (at && t->type != TOKEN_ERROR) 
		fprintf(stderr, " at " COLOR_BOLD "'%.*s'" COLOR_NONE, t->length, t->start);

    fprintf(stderr, ": %s\n", m);
}

void ErrorDispatcher::dispatch_error(const char* prompt, const char* message)
{
    __dispatch(false, nullptr, COLOR_RED, prompt, message);
}

void ErrorDispatcher::dispatch_error_at(Token *token, const char* prompt, const char* message)
{
    __dispatch(true, token, COLOR_RED, prompt, message);
}

void ErrorDispatcher::dispatch_error_at_ln(uint line, const char* prompt, const char* message)
{
    __dispatch(false, nullptr, COLOR_RED, prompt, message, line);
}

void ErrorDispatcher::dispatch_warning(const char* prompt, const char* message)
{
    __dispatch(false, nullptr, COLOR_PURPLE, prompt, message);
}

void ErrorDispatcher::dispatch_warning_at(Token *token, const char* prompt, const char* message)
{
    __dispatch(true, token, COLOR_PURPLE, prompt, message);
}

