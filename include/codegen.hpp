#ifndef EVI_CODEGEN_H
#define EVI_CODEGEN_H

#include "common.hpp"
#include "error.hpp"
#include "debug.hpp"
#include "ast.hpp"
#include "pch.h"

#include <stack>

typedef enum
{
	OPTIMIZE_O0 = '0',
	OPTIMIZE_O1 = '1',
	OPTIMIZE_O2 = '2',
	OPTIMIZE_O3 = '3',
	OPTIMIZE_On = 'n',
	OPTIMIZE_Os = 's',
	OPTIMIZE_Oz = 'z',
} OptimizationType;

class CodeGenerator: public Visitor
{
	public:
	CodeGenerator();
	Status generate(const char* infile, const char* outfile, 
					const char* source, AST* astree, OptimizationType opt);

	Status emit_llvm(const char* filename);
	Status emit_object(const char* filename);
	Status emit_binary(const char* filename, const char** linked, int linkedc);

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
		VISIT(CastNode);
		VISIT(UnaryNode);
		VISIT(GroupingNode);
		VISIT(SubscriptNode);
			VISIT(LiteralNode);
			VISIT(ArrayNode);
			VISIT(SizeOfNode);
			VISIT(ReferenceNode);
			VISIT(CallNode);
	#undef VISIT

	private:

	void prepare();
	void optimize(OptimizationType optlevel);
	void finish();

	char* _infile;
	char* _outfile;
	ErrorDispatcher _error_dispatcher;
	llvm::raw_os_ostream* _errstream;

	#ifdef DEBUG_NO_FOLD
	unique_ptr<llvm::IRBuilder<llvm::NoFolder>> _builder;
	#else
	unique_ptr<llvm::IRBuilder<>> _builder;
	#endif
	
	llvm::TargetMachine* _target_machine;
	string _target_triple;
	
	unique_ptr<llvm::Module> _top_module;

	stack<llvm::Value*>* _value_stack;
	map<string, llvm::Function*> _functions;
	map<string, pair<llvm::Value*, ParsedType*>> _named_values;
	uint _string_literal_count;

	void error_at(Token *token, string message);
	void warning_at(Token *token, string message);

	void push(llvm::Value* value);
	llvm::Value* pop();

	llvm::AllocaInst* create_entry_block_alloca(
		llvm::Type* ty, string name);
	llvm::Value* to_bool(llvm::Value* value);
	llvm::Value* create_cast(llvm::Value* srcval, bool srcsigned, 
							 llvm::Type* desttype, bool destsigned);

	ParsedType* from_token_type(TokenType type);
};

#endif