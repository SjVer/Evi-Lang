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

	typedef struct
	{
		int depth;
		vector<string> symbols;
	} Scope;

	// methods

	void error_at(Token *token, string message);
	void error(string message);
	void error_at_current(string message);

	void advance();
	bool check(TokenType type);
	void consume(TokenType type, string message);
	bool match(TokenType type);
	bool is_at_end();

	void add_symbol(Token *identtoken);
	void scope_up();
	void scope_down();

	StmtNode* declaration();
		StmtNode* function_declaration();
		StmtNode* variable_declaration();
		StmtNode* statement();
			StmtNode* block();
			StmtNode* expression_statement();
				ExprNode* expression();
				// ExprNode* ternary();
				ExprNode* logical_or();
				ExprNode* logical_and();
				ExprNode* bitwise_or();
				ExprNode* bitwise_xor();
				ExprNode* bitwise_and();
				ExprNode* equality();
				ExprNode* comparison();
				ExprNode* bitwise_shift();
				ExprNode* term();
				ExprNode* factor();
				ExprNode* unary();
				ExprNode* primary();

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