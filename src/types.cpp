#include "types.hpp"

bool EviType::operator==(const EviType& rhs)
{
	return lhs.llvm_type == rhs.llvm_type
		&& lhs.lexical_type == rhs.lexical_type
		&& lhs.name == rhs.name
		&& lhs.alignment == rhs.alignment;
}

const char* lexical_type_strings[TYPE_NONE];

map<string, EviType> __evi_types;
llvm::LLVMContext __context;