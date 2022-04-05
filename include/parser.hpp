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
	Status parse(string infile, ccp source, AST* astree);

private:

	// types

	typedef struct
	{
		ParsedType* ret_type;
		vector<ParsedType*> params;
		bool variadic;
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
		
		enum ScopeType
		{
			SCOPE_NORMAL,
			SCOPE_FUNCTION,
			SCOPE_LOOP
		} scope_type;
	} Scope;

	// methods

	void error_at(Token *token, string message);
	void error(string message);
	void error_at_current(string message);
	void note_declaration(string type, string name, Token* token);

	void advance(bool can_trigger_lint = true);
	bool check(TokenType type);
	bool consume(TokenType type, string message);
	ParsedType* consume_type(string msg = "Expected type.");
	bool match(TokenType type);
	bool is_at_end();

	#define CONSUME_OR_RET_NULL(type, msg) if(!consume(type, msg)) return nullptr;

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
			ExprNode* expression(bool = false);
			ExprNode* ternary(bool);
			ExprNode* logical_or(bool);
			ExprNode* logical_xor(bool);
			ExprNode* logical_and(bool);
			ExprNode* bitwise_or(bool);
			ExprNode* bitwise_xor(bool);
			ExprNode* bitwise_and(bool);
			ExprNode* equality(bool);
			ExprNode* comparison(bool);
			ExprNode* bitwise_shift(bool);
			ExprNode* term(bool);
			ExprNode* factor(bool);
			ExprNode* cast(bool);
			ExprNode* unary(bool);
			ExprNode* subscript(bool);
			ExprNode* primary(bool);

	LiteralNode* literal();
	ArrayNode* array(bool);
	SizeOfNode* size_of();
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
	// Token _current_call_token;

	#define PREV_TOKEN_STR std::string(_previous.start, _previous.length)
};

#endif