#ifndef EVI_CODEGEN_H
#define EVI_CODEGEN_H

#include "common.hpp"
#include "error.hpp"
#include "ast.hpp"
#include "pch.h"

#include <stack>

class CodeGenerator: public Visitor
{
	public:
	CodeGenerator();
	Status generate(string infile, string outfile, 
					const char* source, AST* astree);

	#define VISIT(_node) void visit(_node* node)
	VISIT(FuncDeclNode);
	VISIT(VarDeclNode);
	VISIT(AssignNode);
	VISIT(IfNode);
	VISIT(LoopNode);
	VISIT(ReturnNode);
	VISIT(BlockNode);
		VISIT(LogicalNode);
		VISIT(BinaryNode);
		VISIT(UnaryNode);
		VISIT(GroupingNode);
			VISIT(LiteralNode);
			VISIT(ReferenceNode);
			VISIT(CallNode);
	#undef VISIT

	private:


	string _outfile;
	ErrorDispatcher _error_dispatcher;

	#ifdef DEBUG_NO_FOLD
	unique_ptr<llvm::IRBuilder<llvm::NoFolder>> _builder;
	#else
	unique_ptr<llvm::IRBuilder<>> _builder;
	#endif
	
	unique_ptr<llvm::Module> _top_module;
	llvm::BasicBlock* _global_init_func_block;

	stack<llvm::Value*> _value_stack;
	map<string, llvm::Function*> _functions;
	map<string, pair<llvm::AllocaInst*, EviType>> _named_values;
	map<string, EviType> _named_globals;


	void error_at(Token *token, string message);
	void warning_at(Token *token, string message);

	void push(llvm::Value* value);
	llvm::Value* pop();

	llvm::AllocaInst* create_entry_block_alloca(
		llvm::Function *function, llvm::Argument& arg);
	llvm::Value* create_cast(llvm::Value* srcval, bool srcsigned, 
							 llvm::Type* desttype, bool destsigned);
	llvm::Type* lexical_type_to_llvm(LexicalType type);
	llvm::Type* lexical_type_to_llvm(TokenType type);
};

#endif