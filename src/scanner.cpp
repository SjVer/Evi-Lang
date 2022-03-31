#include "common.hpp"
#include "scanner.hpp"
#include "tools.hpp"

#include <cstdio>
#include <cctype>
#include <cstring>
#include <regex>

Scanner::Scanner() {}

Scanner::Scanner(const char *source)
{
	_src_start = source;
	_start = source;
	_current = source;
	_line = 1;
}

int Scanner::getScannedLength()
{
	//
	return (int)(_current - _src_start);
}

bool Scanner::isAtEnd()
{
	//
	return *_current == '\0';
}

bool Scanner::isDigit(char c)
{
	//
	return c >= '0' && c <= '9';
}

bool Scanner::isAlpha(char c)
{
	return (c >= 'a' && c <= 'z') ||
		   (c >= 'A' && c <= 'Z') ||
		    c == '_';
}

bool Scanner::match(char expected)
{
	if (isAtEnd())
		return false;
	if (*_current != expected)
		return false;
	_current++;
	return true;
}

char Scanner::advance()
{
	_current++;
	return _current[-1];
}

char Scanner::peek()
{
	//
	return *_current;
}

char Scanner::peekNext()
{
	if (isAtEnd())
		return '\0';
	return _current[1];
}

Token Scanner::makeToken(TokenType type)
{
	return Token{
		/*type*/ type,
		/*source*/ _src_start,
		/*start*/ _start,
		/*length*/ (int)(_current - _start),
		/*line*/ _line,
		/*file*/ _filename,
	};
}

Token Scanner::errorToken(const char *message)
{
	return Token{
		/*type*/ TOKEN_ERROR,
		/*source*/ _src_start,
		/*start*/ message,
		/*length*/ (int)strlen(message),
		/*line*/ _line,
		/*file*/ _filename
	};
}

Token Scanner::string()
{
	while (peek() != '"' && !isAtEnd())
	{
		if (peek() == '\n')
			_line++;
		else if (peek() == '\\')
			advance();
		advance();
	}

	if (isAtEnd())
		return errorToken("Unterminated string.");

	// The closing quote.
	advance();
	return makeToken(TOKEN_STRING);
}

Token Scanner::character()
{
	if(peek() == '\'') return errorToken("Invalid character of length 0.");
	
	bool esc = peek() == '\\' ? (advance(), true) : false;

	if (isAtEnd()) return errorToken("Unterminated character.");
	
	char ch = advance();

	if(esc) ch = tools::escchr(ch);
	if(ch == -1) return errorToken("Invalid escape character.");
	
	if (peek() != '\'') return errorToken("Unterminated character.");

	advance();
	return makeToken(TOKEN_CHARACTER);
}

Token Scanner::number()
{
	if(isAlpha(peek())) // other notation
	{
		// assert proper notation
		char last = tolower(advance());
		if(last != 'b' && last != 'c' && last != 'x') return errorToken("Invalid numerical notation.");

		// set max char
		char max = last == 'b' ? '1'
				 : last == 'c' ? '7'
				 : /*last=='x'*/ 'f';

		while(isDigit(peek()) || isAlpha(peek()))
			if(advance() > max) return errorToken("Invalid numerical notation.");

		return makeToken(TOKEN_INTEGER);
	}
	else // decimal (possibly float)
	{
		while (isDigit(peek())) advance();

		// Look for a fractional part.
		if (peek() == '.' && isDigit(peekNext()))
		{
			// Consume the ".".
			advance();

			while (isDigit(peek())) advance();
			return makeToken(TOKEN_FLOAT);
		}

		return makeToken(TOKEN_INTEGER);
	}
}

Token Scanner::type_or_identifier()
{
	while (isAlpha(peek()) || isDigit(peek()))
		advance();
	
	// check if token is identifier
	std::string token = std::string(_start, _current - _start);
	if(IS_EVI_TYPE(token)) return makeToken(TOKEN_TYPE);

	return makeToken(TOKEN_IDENTIFIER);
}

Token Scanner::reference()
{
	if(isAlpha(peek()))
	{
		// variable
		advance();
		while (isAlpha(peek()) || isDigit(peek())) advance();
		return makeToken(TOKEN_VARIABLE_REF);
	}
	else if(isDigit(peek()))
	{
		// parameter
		advance();
		while (isDigit(peek())) advance();
		return makeToken(TOKEN_PARAMETER_REF);
	}
	else return errorToken("Expected identifier or integer.");
}

Token Scanner::directive()
{
	// get line
	// ptrdiff_t start = (ptrdiff_t)_start;
	while(peek() != '\n' && !isAtEnd()) advance();
	std::string line = std::string(_start + 1, _current);

	// check it
	cmatch match; // index 0 is whole match
	if(regex_match(line.c_str(), match, regex(LINE_MARKER_REGEX)))
	{
		// marker
		// get lineno and filename
		int lineno = stoi(match[1].str(), 0, 10);
		_line = lineno;
		_filename = new std::string(match[2].str());

		// // clear line marker for neatness' sake
		// std::string new_line = std::string(' ', line.length() + 1);
		// _src_start[start] = ' '; // remove '\0'

		// return token
		return Token{
			/*type  */ TOKEN_LINE_MARKER,
			/*source */ _src_start,
			/*start */ 0,
			/*length*/ 0,
			/*line  */ lineno,
			/*file  */ _filename,
		};
	}
	else return errorToken("Preprocessed code corrupted. (Line or flag marker invalid.)");
}

void Scanner::skipWhitespaces()
{
	for (;;)
	{
		char c = peek();
		switch (c)
		{
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '\n':
			_line++;
			advance();
			break;
		// case '\\':
		// 	if (peekNext() == ':')
		// 	{
		// 		for (;;)
		// 		{
		// 			if (peek() == ':' && peekNext() == '\\')
		// 			{
		// 				advance();
		// 				advance();
		// 				break;
		// 			}
		// 			else if (isAtEnd()) break;
		// 			else if (peek() == '\n') _line++;
		// 			advance();
		// 		}
		// 	}
		// 	else
		// 	{
		// 		while (peek() != '\n' && !isAtEnd())
		// 			advance();
		// 	}
		// 	break;
		default:
			return;
		}
	}
}

Token Scanner::scanToken()
{
	skipWhitespaces();

	_start = _current;

	if (isAtEnd())
		return makeToken(TOKEN_EOF);

	char c = advance();

	// check for idents
	if (isAlpha(c))
		return type_or_identifier();
	// check for digits
	if (isDigit(c))
		return number();

	switch (c)
	{
		// single-character

		case '(': return makeToken(TOKEN_LEFT_PAREN);
		case ')': return makeToken(TOKEN_RIGHT_PAREN);
		case '{': return makeToken(TOKEN_LEFT_BRACE);
		case '}': return makeToken(TOKEN_RIGHT_BRACE);
		case '[': return makeToken(TOKEN_LEFT_B_BRACE);
		case ']': return makeToken(TOKEN_RIGHT_B_BRACE);
		case ',': return makeToken(TOKEN_COMMA);
		case '*': return makeToken(TOKEN_STAR);
		case '%': return makeToken(TOKEN_MODULO);
		case '~': return makeToken(TOKEN_TILDE);
		case '@': return makeToken(TOKEN_AT);
		case ';': return makeToken(TOKEN_SEMICOLON);

		// two-character
		case '+': return makeToken(match('+') ? TOKEN_PLUS_PLUS   	: TOKEN_PLUS);
		case '-': return makeToken(match('-') ? TOKEN_MINUS_MINUS  	: match('>') ? TOKEN_ARROW : TOKEN_MINUS);
		case '/': return makeToken(match('=') ? TOKEN_SLASH_EQUAL	: TOKEN_SLASH);
		case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL   : TOKEN_EQUAL);
		case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL    : match('<') ? TOKEN_LESS_LESS 		 : TOKEN_LESS);
		case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : match('>') ? TOKEN_GREATER_GREATER : TOKEN_GREATER);
		
		case '|': return match('|') ? makeToken(TOKEN_PIPE_PIPE) : makeToken(TOKEN_PIPE);
		case '^': return match('^') ? makeToken(TOKEN_CARET_CARET) : makeToken(TOKEN_CARET);
		case '&': return match('&') ? makeToken(TOKEN_AND_AND) : makeToken(TOKEN_AND);
		case '?': return match('?') ? makeToken(TOKEN_QUESTION_QUESTION) : makeToken(TOKEN_QUESTION);
		case ':': return match(':') ? makeToken(TOKEN_COLON_COLON) : makeToken(TOKEN_COLON);
		case '!': return match('!') ? makeToken(TOKEN_BANG_BANG) : makeToken(TOKEN_BANG);

		case '.': return match('.') ? match('.') ? makeToken(TOKEN_ELIPSES)
												 : errorToken("Unexpected character '.'.")
									: makeToken(TOKEN_DOT);

		// literals
		case '$': return reference();
		case '"': return string();
		case '\'': return character();

		case '#': return directive();
	}

	char* errstr = new char[25]; // just above what's needed
	strcpy(errstr, "Unexpected character 'X'.");
	errstr[strlen(errstr) - 3] = c;
	return errorToken(errstr);
}

// =========================

char *get_tokentype_str(TokenType type)
{
	switch(type)
	{
		// Single-character tokens.
		case TOKEN_LEFT_PAREN: return "LEFT_PAREN";
		case TOKEN_RIGHT_PAREN: return "RIGHT_PAREN";
		case TOKEN_LEFT_BRACE: return "LEFT_BRACE";
		case TOKEN_RIGHT_BRACE: return "RIGHT_BRACE";
		case TOKEN_LEFT_B_BRACE: return "LEFT_B_BRACE";
		case TOKEN_RIGHT_B_BRACE: return "RIGHT_B_BRACE";
		case TOKEN_SLASH: return "SLASH";
		case TOKEN_COMMA: return "COMMA";
		case TOKEN_SEMICOLON: return "SEMICOLON";
		case TOKEN_STAR: return "STAR";
		case TOKEN_MODULO: return "MODULO";
		case TOKEN_TILDE: return "TILDE";
		case TOKEN_AT: return "AT";
		case TOKEN_HASHTAG: return "HASHTAG";
		case TOKEN_DOT: return "DOT";

		// One or two character tokens.
		case TOKEN_PLUS: return "PLUS";
		case TOKEN_PLUS_PLUS: return "PLUS_PLUS";
		case TOKEN_MINUS: return "MINUS";
		case TOKEN_MINUS_MINUS: return "MINUS_MINUS";
		case TOKEN_GREATER: return "GREATER";
		case TOKEN_GREATER_GREATER: return "GREATER_GREATER";
		case TOKEN_LESS: return "LESS";
		case TOKEN_LESS_LESS: return "LESS_LESS";
		case TOKEN_EQUAL: return "EQUAL";
		case TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
		case TOKEN_PIPE: return "PIPE";
		case TOKEN_PIPE_PIPE: return "PIPE_PIPE";
		case TOKEN_CARET: return "CARET";
		case TOKEN_CARET_CARET: return "CARET_CARET";
		case TOKEN_AND: return "AND";
		case TOKEN_AND_AND: return "AND_AND";
		case TOKEN_QUESTION: return "QUESTION";
		case TOKEN_QUESTION_QUESTION: return "QUESTION_QUESTION";
		case TOKEN_COLON: return "COLON";
		case TOKEN_COLON_COLON: return "COLON_COLON";
		case TOKEN_BANG: return "BANG";
		case TOKEN_BANG_BANG: return "BANG_BANG";

		// Multi-character tokens
		case TOKEN_SLASH_EQUAL: return "SLASH_EQUAL";
		case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
		case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
		case TOKEN_ARROW: return "ARROW";
		case TOKEN_ELIPSES: return "ELIPSES";

		// Literals.
		case TOKEN_IDENTIFIER: return "IDENTIFIER";
		case TOKEN_VARIABLE_REF: return "VARIABLE_REF";
		case TOKEN_PARAMETER_REF: return "PARAMETER_REF";
		case TOKEN_INTEGER: return "INTEGER";
		case TOKEN_FLOAT: return "FLOAT";
		case TOKEN_CHARACTER: return "CHARACTER";
		case TOKEN_STRING: return "STRING";
		case TOKEN_TYPE: return "TYPE";

		// misc.
		case TOKEN_LINE_MARKER: return "LINE_MARKER";
		case TOKEN_ERROR: return "ERROR";
		case TOKEN_EOF: return "EOF";
	}
}

void print_tokens_from_src(const char *src)
{
	Scanner scanner(src);
	
	Token token;
	do {
		token = scanner.scanToken();
		printf("%d: \"%.*s\" \t\t-> %s\n\n",
			token.line, token.length, token.start, get_tokentype_str(token.type));

	} while (token.type != TOKEN_EOF);
}