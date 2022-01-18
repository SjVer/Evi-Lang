#ifndef EVI_COMPILER_H
#define EVI_COMPILER_H

#include "scanner.hpp"
#include "ir_object.hpp"
#include "common.hpp"

#include "phc.h"

#include <stack>

using namespace std;

// ==== ============= ====

class Compiler
{
public:
	Compiler():  _configured(false),
				_scope_stack(), _builder(__context) {}

	void configure(string infile, string outfile);
	Status compile();

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

	typedef void (Compiler::*ParseFn)();

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
	llvm::Constant* consume_type(EviType type, string message_format, string typestr);
	bool match(TokenType type);

	void add_local(Token *identtoken);

	ParseRule get_parse_rule(TokenType type);
	void parse_precedence(Precedence precedence);

	void expression();
	void binary();
	void literal();

	void declaration();
	void variable_declaration();

	void scope_up();
	Scope scope_down();
	void start();
	void finish();
	void write_to_file();

	// members

	bool _configured;
	bool _hadError;
	bool _panicMode;

	stack<Scope> _scope_stack;
	Scope _current_scope;

	// See ir_object.hpp for info
	stack<EviIRObject> _ir_object_stack;

	string _infile, _outfile, _source;

	Scanner _scanner;

	Token _current;
	Token _previous;

	#define PREV_TOKEN_STR std::string(_previous.start, _previous.length)

	// llvm-specific

	llvm::IRBuilder<> _builder;
	shared_ptr<llvm::Module> _top_module;
	// shared_ptr<llvm::Module> _current_module;
	llvm::BasicBlock* _global_init_func_block;

	#define LLVM_MODULE_TOP_NAME "top"
};

#endif