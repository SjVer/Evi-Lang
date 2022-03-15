#include "error.hpp"

/* void ErrorDispatcher::dispatch_token_marked(Token *token)
{
		To find out what lines to print and where those lines
		start we first find the total offset from the start
		of the token, then we go back char by char to find the
		first newline, which would be the start of the line of
		the token. Then we do the same thing but in the opposite
		direction to find the end of the line.

	string before1 = "";
	string tokenline = "";
	string after1 = "";

	// get offset of token
	ptrdiff_t token_offset = token->start - token->source;

	// find first newline before token
	ptrdiff_t tok_ln_begin;
	if(token->line > 1)
	{
		tok_ln_begin = token_offset;
		while(tok_ln_begin > 0 && token->source[tok_ln_begin] != '\n') tok_ln_begin--;
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
			while(bef_ln_begin > 0 && token->source[bef_ln_begin] != '\n') bef_ln_begin--;
			bef_ln_begin++; // skip newline itself
		}
		else bef_ln_begin = 0;
		
		string bef_ln = string(token->source + bef_ln_begin, tok_ln_begin - bef_ln_begin - 1);
		before1 = tools::fstr("       %2d| %s", token->line - 1, bef_ln.c_str());
	}

	// find first newline after token
	ptrdiff_t tok_ln_end = token_offset + token->length;
	while(token->source[tok_ln_end] != '\n' && token->source[tok_ln_end] != '\0') tok_ln_end++;

	// get line with marked token
	{
		
			Inserting the escape codes for coloring won't work, so instead we find
			length A and B as seen below:

				this is an example line with a WRONG token in it.
				<------------- A ------------->     <---- B ---->

			And use those and the info we have of the token to merge them,
			and the escape codes together in a new string.
		

		// find string on the token's line before and after the token itself
		ptrdiff_t tok_ln_before_tok_len = token_offset - tok_ln_begin;
		string tok_ln_before_tok = string(token->source + tok_ln_begin, tok_ln_before_tok_len);

		ptrdiff_t tok_ln_after_tok_len = tok_ln_end - token_offset - token->length;
		string tok_ln_after_tok = string(token->source + token_offset + token->length, tok_ln_after_tok_len);

		// get token and its full line
		string tok = string(token->start, token->length);
		string tok_ln = tok_ln_before_tok + COLOR_RED + tok + COLOR_NONE + tok_ln_after_tok;
		tokenline = tools::fstr(COLOR_RED "    ->" COLOR_NONE " %2d| %s", token->line, tok_ln.c_str());
	}

	// get line after token's line if and possible
	if(token->source[tok_ln_end] != '\0')
	{
		// find first newline after end of token's line;
		ptrdiff_t af_ln_begin = tok_ln_end + 1;                 // skip prev. line's newline;
		ptrdiff_t af_ln_end   = af_ln_begin;                    // skip prev. line's newline;
		while(token->source[af_ln_end] != '\n' && token->source[af_ln_end] != '\0') af_ln_end++;

		int af_ln_len = af_ln_end - tok_ln_end - 1; // minus trailing newline
		string af_ln = string(token->source + af_ln_begin, af_ln_len);
		after1 = tools::fstr("       %2d| %s", token->line + 1, af_ln.c_str());
	}

	// print it all out
	if(before1 	 != "") cerr << before1 << endl;
	if(tokenline != "") cerr << tokenline << endl;
	if(after1 	 != "") cerr << after1 << endl;
} */

void ErrorDispatcher::print_token_marked(Token *token, ccp color)
{
	string tokenline = "";
	string markerline = "";

	// get offset of token (first char)
	ptrdiff_t token_offset = token->start - token->source;

	// find first newline before token
	ptrdiff_t tok_ln_begin;
	{
		tok_ln_begin = token_offset;
		while(tok_ln_begin > 0 && token->source[tok_ln_begin] != '\n') tok_ln_begin--;
		tok_ln_begin++; // skip newline itself
	}

	// find first newline after token
	ptrdiff_t tok_ln_end = token_offset + token->length;
	while(token->source[tok_ln_end] != '\n' && token->source[tok_ln_end] != '\0') tok_ln_end++;

	ptrdiff_t tok_ln_before_tok_len; // keep for later use

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
		tok_ln_before_tok_len = token_offset - tok_ln_begin;
		string tok_ln_before_tok = string(token->source + tok_ln_begin, tok_ln_before_tok_len);

		ptrdiff_t tok_ln_after_tok_len = tok_ln_end - token_offset - token->length;
		string tok_ln_after_tok = string(token->source + token_offset + token->length, tok_ln_after_tok_len);

		// get token and its full line
		string tok = string(token->start, token->length);
		string tok_ln = tok_ln_before_tok + color + tok + COLOR_NONE + tok_ln_after_tok;
		tokenline = tools::fstr(" %3d| %s", token->line, tok_ln.c_str());
	}

	// get the marker line
	{
		/*
			Example:

			 69| @this i32 WRONG ();
			               ^~~~~
		*/

		int prefixlen = tools::fstr(" %3d| ", token->line).length();
		for(int i = 0; i < prefixlen; i++)
			markerline +=  ' ';

		for(int i = 0; i < tok_ln_before_tok_len; i++)
			markerline +=  token->source[tok_ln_begin + i] == '\t' ? '\t' : ' ';


		markerline += string(color) + "^";
		for(int i = 0; i < token->length - 1; i++)
			markerline += '~';

		markerline += COLOR_NONE;
	}

	// print it all out
	cerr << tokenline << endl;
	cerr << markerline << endl;
}

void ErrorDispatcher::__dispatch(ccp color, ccp prompt, ccp message)
{
	cerr << tools::fstr("[evi] %s%s" COLOR_NONE ": %s",
						color, prompt, message) << endl;
}

void ErrorDispatcher::__dispatch_at_token(ccp color, Token* token, ccp prompt, ccp message)
{
	fprintf(stderr, "[%s:%d] %s%s" COLOR_NONE ": %s\n",
			token->file->c_str(), token->line, color, prompt, message);
}

// if line == 0 lineno is omitted. likewise with filename
void ErrorDispatcher::__dispatch_at_line(ccp color, uint line, ccp filename, ccp prompt, ccp message)
{
	if(line) fprintf(stderr, "[%s:%d] %s%s" COLOR_NONE ": %s\n",
				filename ? filename : "???", line, color, prompt, message);

	else fprintf(stderr, "[%s] %s%s" COLOR_NONE ": %s\n",
				filename ? filename : "???", color, prompt, message);
}


void ErrorDispatcher::error(ccp prompt, ccp message)
	{ __dispatch(COLOR_RED, prompt, message); }

void ErrorDispatcher::error_at_line(uint line, ccp filename, ccp prompt, ccp message)
	{ __dispatch_at_line(COLOR_RED, line, filename, prompt, message); }

void ErrorDispatcher::error_at_token(Token* token, ccp prompt, ccp message)
	{ __dispatch_at_token(COLOR_RED, token, prompt, message); }

void ErrorDispatcher::warning(ccp prompt, ccp message)
	{ __dispatch(COLOR_PURPLE, prompt, message); }

void ErrorDispatcher::warning_at_line(uint line, ccp filename, ccp prompt, ccp message)
	{ __dispatch_at_line(COLOR_PURPLE, line, filename, prompt, message); }

void ErrorDispatcher::warning_at_token(Token* token, ccp prompt, ccp message)
	{ __dispatch_at_token(COLOR_PURPLE, token, prompt, message); }

void ErrorDispatcher::note(ccp message)
	{ __dispatch(COLOR_GREEN, "Note", message); }

void ErrorDispatcher::note_at_line(uint line, ccp filename, ccp message)
	{ __dispatch_at_line(COLOR_GREEN, line, filename, "Note", message); }

void ErrorDispatcher::note_at_token(Token* token, ccp message)
	{ __dispatch_at_token(COLOR_GREEN, token, "Note", message); }
