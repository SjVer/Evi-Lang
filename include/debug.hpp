#ifndef EVI_DEBUG_H
#define EVI_DEBUG_H

#include "common.hpp"
#include "types.hpp"
#include "ast.hpp"
#include "pch.h"

class DebugInfoBuilder
{
public:
	unique_ptr<llvm::DIBuilder> _debug_builder;
private:
	llvm::DICompileUnit* _cunit;
	vector<llvm::DIScope*> _scopes;

	#ifdef DEBUG_NO_FOLD
	llvm::IRBuilder<llvm::NoFolder>* _ir_builder;
	#define IRBUILDER llvm::IRBuilder<llvm::NoFolder>*
	#else
	llvm::IRBuilder<>* _ir_builder;
	#define IRBUILDER llvm::IRBuilder<>*
	#endif

	string _directory;

public:

	DebugInfoBuilder(llvm::Module*, IRBUILDER, string, bool);
	void finish();

	llvm::DIFile* create_file_unit(string filename);
	llvm::DISubprogram* create_subprogram(FuncDeclNode* node, llvm::DIFile* file_unit);
	llvm::DILocalVariable* create_parameter(int index, int line_no, ParsedType* type);

	void emit_location(uint line, uint col);
	void insert_declare(llvm::AllocaInst*, ...);
	void push_subprogram(llvm::DISubprogram* sub_program);
	void pop_subprogram();

	llvm::DIType* get_type(ParsedType* type);
	llvm::DISubroutineType* get_function_type(FuncDeclNode* node, llvm::DIFile* file_unit);
};

#endif