#ifndef EVI_DEBUG_H
#define EVI_DEBUG_H

#include "common.hpp"
#include "types.h"
#include "pch.h"

class DebugInfo
{
	unique_ptr<llvm::DIBuilder<>> _builder;
	unique_ptr<llvm::DICompileUnit> _cunit;

public:

	DebugInfo(llvm::Module* module, const char* filename);
	void finish();

	llvm::DIType* get_type(ParsedType* type);
};

#endif