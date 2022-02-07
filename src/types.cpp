#include "types.hpp"

bool EviType::operator==(const EviType& rhs)
{
	// just checking the name is ok
	return this->_name == rhs._name;
}

llvm::Type* EviType::get_llvm_type()
{
	llvm::Type* ret = _llvm_type;
	for(int i =0; i < _pointer_depth; i++) ret = ret->getPointerTo();
	return ret;
}

string EviType::to_string()
{
	string ret = _name;
	for(int i = 0; i < _pointer_depth; i++) ret += "*";
	return ret;
}

const char* lexical_type_strings[TYPE_NONE];

map<string, EviType*> __evi_types;
llvm::LLVMContext __context;