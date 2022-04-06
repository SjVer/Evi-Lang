#ifndef EVI_DEBUG_H
#define EVI_DEBUG_H

#include "common.hpp"
#include "types.hpp"
#include "ast.hpp"
#include "pch.h"

class DebugInfoBuilder
{
	unique_ptr<llvm::DIBuilder> _debug_builder;
	llvm::DICompileUnit* _cunit;
	vector<llvm::DIScope*> _lexical_blocks;

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
	
	void emit_location(uint line, uint col);
	void push_subprogram(llvm::DISubprogram* sub_program);

	llvm::DIType* get_type(ParsedType* type);
	llvm::DISubroutineType* get_function_type(FuncDeclNode* node, llvm::DIFile* file_unit);
};

#endif