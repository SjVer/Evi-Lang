#include "types.hpp"

bool operator==(const EviType& lhs, const EviType& rhs)
{
	return lhs.llvm_type == rhs.llvm_type
		&& lhs.lexical_type == rhs.lexical_type
		&& lhs.name == rhs.name
		&& lhs.alignment == rhs.alignment
		&& lhs.issigned == rhs.issigned;
}

const char* lexical_type_strings[TYPE_NONE];

map<string, EviType> __evi_types;
llvm::LLVMContext __context;