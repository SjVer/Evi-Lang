#ifndef EVI_PARSER_H
#define EVI_PARSER_H

#include "scanner.hpp"
#include "ast.hpp"
#include "common.hpp"

#include "phc.h"

using namespace std;

// ==== ============= ====

class Parser
{
public:
	Parser(): _scope_stack() {}
	Status parse(string infile, AST* astree);

private:

	// types

	typedef enum
	{
		PREC_NONE,
		PREC_TERNARY,	 // ?:
		PREC_OR,		 // ||
		PREC_AND,		 // &&
		PREC_B_OR,		 // |
		PREC_B_XOR,		 // ^
		PREC_B_AND,		 // &
		PREC_EQUALITY,	 // == !=
		PREC_COMPARISON, // < > <= >=
		PREC_SHIFT, 	 // << >>
		PREC_TERM,	 	 // + -
		PREC_FACTOR,	 // * /
		PREC_UNARY,		 // ! - $ ++ --
		PREC_PRIMARY	 // literals n shit
	} Precedence;

	typedef void (Parser::*ParseFn)();

	typedef struct
	{
		ParseFn prefix;
		ParseFn infix;
		Precedence precedence;
	} ParseRule;

	typedef struct
	{
		int _depth;
		vector<string> _locals;
	} Scope;

	// methods

	void error_at(Token *token, string message);
	void error(string message);
	void error_at_current(string message);

	void advance();
	bool check(TokenType type);
	void consume(TokenType type, string message);
	bool match(TokenType type);

	void add_local(Token *identtoken);

	StmtNode* declaration();
	StmtNode* variable_declaration();

	ExprNode* expression();
	ExprNode* primary();

	void scope_up();
	Scope scope_down();

	// members

	bool _had_error;
	bool _panic_mode;

	Scanner _scanner;

	Token _current;
	Token _previous;

	AST* _astree;

	vector<Scope> _scope_stack;
	Scope _current_scope;

	string _infile, _source;

	#define PREV_TOKEN_STR std::string(_previous.start, _previous.length)
};

#endif