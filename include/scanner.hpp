#ifndef EVI_SCANNER_H
#define EVI_SCANNER_H

#include "types.hpp"

#include <vector>
#include <string>

typedef enum
{
	// Single-character tokens.
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	// TOKEN_LEFT_B_BRACE,
	// TOKEN_RIGHT_B_BRACE,
	TOKEN_SLASH,
	TOKEN_COMMA,
	TOKEN_BANG,
	TOKEN_QUESTION,
	TOKEN_COLON,
	TOKEN_STAR,
	TOKEN_MODULO,
	TOKEN_DOLLAR,
	TOKEN_TILDE,
	TOKEN_CARET,
	TOKEN_AT,
	TOKEN_HASHTAG,

	// One or two character tokens.
	TOKEN_PLUS,
	TOKEN_PLUS_PLUS,
	TOKEN_MINUS,
	TOKEN_MINUS_MINUS,
	TOKEN_GREATER,
	TOKEN_GREATER_GREATER,
	TOKEN_LESS,
	TOKEN_LESS_LESS,
	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_AND,
	TOKEN_AND_AND,
	TOKEN_PIPE,
	TOKEN_PIPE_PIPE,

	// Multi-character tokens
	TOKEN_SLASH_EQUAL,
	TOKEN_GREATER_EQUAL,
	TOKEN_LESS_EQUAL,

	// Literals.
	TOKEN_IDENTIFIER,
	TOKEN_INTEGER,
	TOKEN_FLOAT,
	TOKEN_CHARACTER,
	TOKEN_STRING,
	TOKEN_TYPE,

	// misc.
	TOKEN_NEWLINE,
	TOKEN_ERROR,
	TOKEN_EOF
} TokenType;

char *getTokenStr(TokenType type);

typedef struct
{
	TokenType type;
	const char *start;
	int length;
	int line;
} Token;

class Scanner
{
public:
	Scanner();
	Scanner(const char *source);
	Token scanToken();
	int getScannedLength();

private:
	const char *_src_start;	
	const char *_start;
	const char *_current;
	int _line;

	bool isAtEnd();
	bool isDigit(char c);
	bool isAlpha(char c);
	bool match(char expected);
	char advance();
	char peek();
	char peekNext();
	Token makeToken(TokenType type);
	Token errorToken(const char *message);
	Token string();
	Token character();
	Token number();
	Token type_or_identifier();
	void skipWhitespaces();
};

void printTokensFromSrc(const char *src);

#endif