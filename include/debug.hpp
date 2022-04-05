#ifndef EVI_DEBUG_H
#define EVI_DEBUG_H

#include "common.hpp"
#include "types.hpp"
#include "pch.h"

class DebugInfoBuilder
{
	unique_ptr<llvm::DIBuilder> _builder;
	llvm::DICompileUnit* _cunit;

public:

	DebugInfoBuilder(llvm::Module*, string, bool);
	void finish();

	void emit_location(uint line);

	llvm::DIType* get_type(ParsedType* type);
};

#endif