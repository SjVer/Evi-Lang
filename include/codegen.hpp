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
	Status generate(const char* infile, const char* outfile, 
					const char* source, AST* astree);

	Status emit_llvm(const char* filename);
	Status emit_object(const char* filename);
	Status emit_binary(const char* filename);

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
			VISIT(ArrayNode);
			VISIT(ReferenceNode);
			VISIT(CallNode);
	#undef VISIT

	private:

	void prepare();
	void finish();

	char* _infile;
	char* _outfile;
	ErrorDispatcher _error_dispatcher;
	stringstream _errstream;
	llvm::raw_os_ostream* _llvm_errstream;

	#ifdef DEBUG_NO_FOLD
	unique_ptr<llvm::IRBuilder<llvm::NoFolder>> _builder;
	#else
	unique_ptr<llvm::IRBuilder<>> _builder;
	#endif
	
	llvm::TargetMachine* _target_machine;
	string _target_triple;
	
	unique_ptr<llvm::Module> _top_module;
	llvm::BasicBlock* _global_init_func_block;

	stack<llvm::Value*>* _value_stack;
	map<string, llvm::Function*> _functions;
	map<string, pair<llvm::Value*, ParsedType*>> _named_values;

	void error_at(Token *token, string message);
	void warning_at(Token *token, string message);

	void push(llvm::Value* value);
	llvm::Value* pop();

	llvm::AllocaInst* create_entry_block_alloca(
		llvm::Function *function, llvm::Value* value);
	llvm::Value* to_bool(llvm::Value* value);
	llvm::Value* create_cast(llvm::Value* srcval, bool srcsigned, 
							 llvm::Type* desttype, bool destsigned);

	ParsedType* from_token_type(TokenType type);
};

#endif