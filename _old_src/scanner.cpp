#include "scanner.hpp"
#include "instructions.hpp"

#include <cstring>

Scanner::Scanner(const char *source)
{
	start = source;
	current = source;
	line = 1;
}

bool Scanner::isAtEnd()
{
	//
	return *current == '\0';
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
	if (*current != expected)
		return false;
	current++;
	return true;
}

char Scanner::advance()
{
	current++;
	return current[-1];
}

char Scanner::peek()
{
	//
	return *current;
}

char Scanner::peekNext()
{
	if (isAtEnd())
		return '\0';
	return current[1];
}

Token Scanner::makeToken(TokenType type)
{
	Token token;
	token.type = type;
	token.start = start;
	token.length = (int)(current - start);
	token.line = line;
	return token;
}

Token Scanner::errorToken(const char *message)
{
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = line;
	return token;
}

Token Scanner::string()
{
	while (peek() != '"' && !isAtEnd())
	{
		if (peek() == '\n')
			line++;
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
	if (peek() == 'x' || peek() == 'X' ||
		peek() == 'b' || peek() == 'B' ||
		peek() == 'c' || peek() == 'C')
		advance();
		
	while (isDigit(peek()))
		advance();

	// Look for a fractional part.
	if (peek() == '.' && isDigit(peekNext()))
	{
		// Consume the ".".
		advance();

		while (isDigit(peek()))
			advance();
	}

	return makeToken(TOKEN_NUMBER);
}

Token Scanner::identifier()
{
	while (isAlpha(peek()) || isDigit(peek()))
		advance();
	// return makeToken(identifierType());
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
		// case '\n':
		// 	scanner.line++;
		// 	advance();
		// 	break;
		case ';':
			if (peekNext() == ';')
			{
				// while (peek() != '*' && peekNext() != '#' && !isAtEnd())
				for (;;)
				{
					if (peek() == ';' && peekNext() == ';')
					{
						advance();
						advance();
						break;
					}
					else if (isAtEnd()) break;
					else if (peek() == '\n') line++;
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

	start = current;

	if (isAtEnd())
		return makeToken(TOKEN_EOF);

	char c = advance();

	// check for idents
	if (isAlpha(c))
		return identifier();
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
		case '?': return makeToken(TOKEN_QUESTION);
		case ':': return makeToken(TOKEN_COLON);
		case ';': return makeToken(TOKEN_SEMICOLON);
		case ',': return makeToken(TOKEN_COMMA);
		case '.': return makeToken(TOKEN_DOT);
		case '*': return makeToken(TOKEN_STAR);
		case '%': return makeToken(TOKEN_MODULO);
		case '$': return makeToken(TOKEN_DOLLAR);
		case '~': return makeToken(TOKEN_TILDE);
		case '@': return makeToken(TOKEN_AT);
		case '#': return makeToken(TOKEN_HASHTAG);

		// two-character
		// case '+': return makeToken(match('+') ? TOKEN_PLUS_PLUS : TOKEN_PLUS);
		case '+': return makeToken(match('+') ? TOKEN_PLUS_PLUS   	: match('=') ? TOKEN_PLUS_EQUAL	: TOKEN_PLUS);
		case '-': return makeToken(match('=') ? TOKEN_MINUS_EQUAL 	: match('>') ? TOKEN_R_ARROW 	: TOKEN_MINUS);
		case '!': return makeToken(match('!') ? TOKEN_BANG_BANG   	: TOKEN_BANG);
		case '/': return makeToken(match('=') ? TOKEN_SLASH_EQUAL	: TOKEN_SLASH);
		case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL   : TOKEN_EQUAL);
		case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL    : match('-') ? TOKEN_L_ARROW 	: TOKEN_LESS);
		case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
		
		case '|': return match('|') ? makeToken(TOKEN_OR) : makeToken(TOKEN_PIPE);
		case '&': return match('&') ? makeToken(TOKEN_AND) : errorToken("Expected '&' after '&'.");

		// literals
		case '"': return string();
		case '\'': return character();

		case '\n':
		{
			while (peek() == '\n')
			{
				advance();
				line++;
			}
			line++;
			return makeToken(TOKEN_NEWLINE);
		}
	}

	return errorToken("Unexpected character.");
}