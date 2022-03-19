#ifndef EVI_PARSER_H
#define EVI_PARSER_H

#include "scanner.hpp"
#include "ast.hpp"
#include "error.hpp"
#include "common.hpp"
#include "lint.hpp"

#include "pch.h"

using namespace std;

// ==== ============= ====

class Parser
{
public:
	Parser(): _scope_stack() {}
	Status parse(string infile, const char* source, AST* astree);

private:

	// types

	typedef struct
	{
		ParsedType* ret_type;
		vector<ParsedType*> params;
		bool defined;
		bool invalid = false;
		Token token;
	} FuncProperties;

	typedef struct
	{
		ParsedType* type;
		Token token;
	} VarProperties;

	typedef struct
	{
		int depth;
		map<string, VarProperties> variables;
		FuncProperties func_props;
		map<string, FuncProperties> functions;
	} Scope;

	// methods

	void error_at(Token *token, string message);
	void error(string message);
	void error_at_current(string message);
	void note_declaration(string type, string name, Token* token);

	void advance(bool can_trigger_lint = true);
	bool check(TokenType type);
	void consume(TokenType type, string message);
	ParsedType* consume_type(string msg = "Expected type.");
	bool match(TokenType type);
	bool is_at_end();

	void generate_lint();

	VarProperties get_variable_props(string name);
	FuncProperties get_function_props(string name);
	bool check_variable(string name);
	bool check_function(string name);
	void add_variable(Token* identtoken, ParsedType* type);
	void add_function(Token* identtoken, FuncProperties properties);
	void scope_up();
	void scope_down();
	void synchronize(bool toplevel);

	StmtNode* statement();
		StmtNode* declaration();
			StmtNode* function_declaration();
			StmtNode* variable_declaration();
		StmtNode* assign_statement();
		StmtNode* if_statement();
		StmtNode* loop_statement();
		StmtNode* return_statement();
		StmtNode* block_statement();
		StmtNode* expression_statement();
			ExprNode* expression();
			ExprNode* ternary();
			ExprNode* logical_or();
			ExprNode* logical_xor();
			ExprNode* logical_and();
			ExprNode* bitwise_or();
			ExprNode* bitwise_xor();
			ExprNode* bitwise_and();
			ExprNode* equality();
			ExprNode* comparison();
			ExprNode* bitwise_shift();
			ExprNode* term();
			ExprNode* factor();
			ExprNode* cast();
			ExprNode* unary();
			ExprNode* subscript();
			ExprNode* primary();

	LiteralNode* literal();
	ArrayNode* array();
	ReferenceNode* reference();
	CallNode* call();

	// members

	Scanner _scanner;
	Token _current;
	Token _previous;

	AST* _astree;
	vector<Scope> _scope_stack;
	Scope _current_scope;

	bool _had_error;
	bool _panic_mode;
	ErrorDispatcher _error_dispatcher;

	#define HOLD_PANIC() bool _old_panic_mode_from_macro_hold_panic = _panic_mode
	#define PANIC_HELD (_old_panic_mode_from_macro_hold_panic)

	string _main_file;

	#define PREV_TOKEN_STR std::string(_previous.start, _previous.length)
};

#endif