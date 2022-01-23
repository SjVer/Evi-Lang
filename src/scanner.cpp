#include "common.hpp"
#include "scanner.hpp"
#include "tools.hpp"

#include <cstdio>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

Scanner::Scanner() { init_builtin_evi_types(); }

Scanner::Scanner(const char *source)
{
	init_builtin_evi_types();
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
	Token token;
	token.type = type;
	token.start = _start;
	token.length = (int)(_current - _start);
	token.line = _line;
	return token;
}

Token Scanner::errorToken(const char *message)
{
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = _line;
	return token;
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
	while (peek() != '\'' && !isAtEnd())
		advance();

	if (isAtEnd())
		return errorToken("Unterminated character.");

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
		// case '#':
		// 	while(peek() != '\n') advance();
		// 	break;
		case '\\':
			if (peekNext() == ':')
			{
				for (;;)
				{
					if (peek() == ':' && peekNext() == '\\')
					{
						advance();
						advance();
						break;
					}
					else if (isAtEnd()) break;
					else if (peek() == '\n') _line++;
					advance();
				}
			}
			else
			{
				while (peek() != '\n' && !isAtEnd())
					advance();
			}
			break;
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
		// case '[': return makeToken(TOKEN_LEFT_B_BRACE);
		// case ']': return makeToken(TOKEN_RIGHT_B_BRACE);
		case '?': return makeToken(TOKEN_QUESTION);
		case '!': return makeToken(TOKEN_BANG);
		case ':': return makeToken(TOKEN_COLON);
		case ',': return makeToken(TOKEN_COMMA);
		case '*': return makeToken(TOKEN_STAR);
		case '%': return makeToken(TOKEN_MODULO);
		case '$': return makeToken(TOKEN_DOLLAR);
		case '~': return makeToken(TOKEN_TILDE);
		case '^': return makeToken(TOKEN_CARET);
		case '@': return makeToken(TOKEN_AT);
		case ';': return makeToken(TOKEN_SEMICOLON);

		// two-character
		// case '+': return makeToken(match('+') ? TOKEN_PLUS_PLUS : TOKEN_PLUS);
		case '+': return makeToken(match('+') ? TOKEN_PLUS_PLUS   	: TOKEN_PLUS);
		case '-': return makeToken(match('-') ? TOKEN_MINUS_MINUS  	: TOKEN_MINUS);
		case '/': return makeToken(match('=') ? TOKEN_SLASH_EQUAL	: TOKEN_SLASH);
		case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL   : TOKEN_EQUAL);
		case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL    : match('<') ? TOKEN_LESS_LESS 		 : TOKEN_LESS);
		case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : match('>') ? TOKEN_GREATER_GREATER : TOKEN_GREATER);
		
		case '|': return match('|') ? makeToken(TOKEN_PIPE_PIPE) : makeToken(TOKEN_PIPE);
		case '&': return match('&') ? makeToken(TOKEN_AND_AND) : makeToken(TOKEN_AND);

		// literals
		case '"': return string();
		case '\'': return character();

		case '\n':
		{
			while (peek() == '\n')
			{
				advance();
				_line++;
			}
			Token token = makeToken(TOKEN_NEWLINE);
			_line++;
			return token;
		}

		case '#':
		{
			while(peek() != '\n') advance();

			// TODO: implement line markers (after preprocessor)

			// if (peek() == 'l')
			// {
			// 	advance();
			// 	char c = advance();
			// 	if (!isDigit(c)) return errorToken(
			// 		"Preprocessor failed to correctly process directive.");

			// 	current_--;
			// 	int len = 0;
			// 	while(isDigit(*current_)) { current_++, len++; }
			// 	std::string numstr(current_ - len, len);
			// 	line_ = std::stoi(numstr);

			// 	// while(advance() != '\n') {}
			// }

			// else return errorToken("Preprocessor failed to correctly process directive.");
			// return makeToken(TOKEN_IGNORE);
		}
	}

	return errorToken("Unexpected character.");
}

// =========================

char *getTokenStr(TokenType type)
{
	switch(type)
	{
		// Single-character tokens.
		case TOKEN_LEFT_PAREN: return "LEFT_PAREN";
		case TOKEN_RIGHT_PAREN: return "RIGHT_PAREN";
		case TOKEN_LEFT_BRACE: return "LEFT_BRACE";
		case TOKEN_RIGHT_BRACE: return "RIGHT_BRACE";
		case TOKEN_SLASH: return "SLASH";
		case TOKEN_GREATER: return "GREATER";
		case TOKEN_LESS: return "LESS";
		case TOKEN_COMMA: return "COMMA";
		case TOKEN_QUESTION: return "QUESTION";
		case TOKEN_COLON: return "COLON";
		case TOKEN_NEWLINE: return "NEWLINE";
		case TOKEN_STAR: return "STAR";
		case TOKEN_MODULO: return "MODULO";
		case TOKEN_DOLLAR: return "DOLLAR";
		case TOKEN_BANG: return "BANG";
		case TOKEN_TILDE: return "TILDE";
		case TOKEN_AT: return "AT";
		case TOKEN_HASHTAG: return "HASHTAG";

		// One or two character tokens.
		case TOKEN_PLUS: return "PLUS";
		case TOKEN_PLUS_PLUS: return "PLUS_PLUS";
		case TOKEN_MINUS: return "MINUS";
		case TOKEN_MINUS_MINUS: return "MINUS_MINUS";
		case TOKEN_EQUAL: return "EQUAL";
		case TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
		case TOKEN_AND: return "AND";
		case TOKEN_AND_AND: return "AND_AND";
		case TOKEN_PIPE: return "PIPE";
		case TOKEN_PIPE_PIPE: return "PIPE_PIPE";

		// Multi-character tokens
		case TOKEN_SLASH_EQUAL: return "SLASH_EQUAL";
		case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
		case TOKEN_LESS_EQUAL: return "LESS_EQUAL";

		// Literals.
		case TOKEN_IDENTIFIER: return "IDENTIFIER";
		case TOKEN_INTEGER: return "INTEGER";
		case TOKEN_FLOAT: return "FLOAT";
		case TOKEN_CHARACTER: return "CHARACTER";
		case TOKEN_STRING: return "STRING";
		case TOKEN_TYPE: return "TYPE";

		// misc.
		case TOKEN_ERROR: return "ERROR";
		case TOKEN_EOF: return "EOF";
		default: return "<TOKEN_NOT_FOUND>";
	}
}

void printTokensFromSrc(const char *src)
{
	Scanner scanner(src);
	
	Token token;
	do {
		token = scanner.scanToken();
		printf("%d: \"%.*s\" \t\t-> %s\n\n",
			token.line, token.length, token.start, getTokenStr(token.type));

	} while (token.type != TOKEN_EOF);
}