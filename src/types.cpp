#include "types.hpp"

bool EviType::operator==(const EviType& rhs)
{
	//
	return lhs.name == rhs.name;
}

llvm::Type* EviType::generate_type()
{
	llvm::Type* ret = this->llvm_type;
	
}

const char* lexical_type_strings[TYPE_NONE];

map<string, EviType> __evi_types;
llvm::LLVMContext __context;