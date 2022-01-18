#ifndef EVI_SCANNER_H
#define EVI_SCANNER_H

typedef enum
{
	// Single-character tokens.
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_B_BRACE,
	TOKEN_RIGHT_B_BRACE,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_MINUS,
	// TOKEN_MINUS_MINUS,
	TOKEN_PLUS,
	TOKEN_PLUS_PLUS,
	TOKEN_QUESTION,
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	TOKEN_NEWLINE,
	TOKEN_STAR,
	TOKEN_MODULO,
	TOKEN_DOLLAR,
	TOKEN_TILDE,
	TOKEN_AT,
	TOKEN_HASHTAG,
	TOKEN_PIPE,

	// One or two character tokens.
	TOKEN_BANG,
	TOKEN_BANG_BANG,
	TOKEN_SLASH,
	TOKEN_SLASH_EQUAL,
	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER,
	TOKEN_LESS,
	TOKEN_GREATER_EQUAL,
	TOKEN_LESS_EQUAL,
	TOKEN_L_ARROW,
	TOKEN_R_ARROW,
	TOKEN_MINUS_EQUAL,
	TOKEN_PLUS_EQUAL,
	TOKEN_AND,
	TOKEN_OR,

	// Literals.
	TOKEN_IDENTIFIER,
	TOKEN_CHARACTER,
	TOKEN_STRING,
	TOKEN_NUMBER,

	// misc.
	TOKEN_ERROR,
	TOKEN_EOF
} TokenType;

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
	Scanner() {}
	Scanner(const char *source);
	Token scanToken();

private:
	const char *start;
	const char *current;
	int line;
	
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
	Token identifier();
	void skipWhitespaces();
};

// scan the next token
Token scanToken();

#endif