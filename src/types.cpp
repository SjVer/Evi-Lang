#include "types.hpp"

bool ParsedType::operator==(const ParsedType& rhs)
{
	return _lexical_type == rhs._lexical_type
		&& _pointer_depth == rhs._pointer_depth
		&& _is_reference == rhs._is_reference;
}

ParsedType* ParsedType::copy_change_lex(LexicalType type)
{
	ParsedType* ret = new ParsedType();
	ret->_lexical_type = type;
	ret->_is_reference = _is_reference;
	ret->_pointer_depth = _pointer_depth;
	return ret;
}

ParsedType* ParsedType::copy_inc_depth()
{
	ParsedType* ret = new ParsedType(_lexical_type, _pointer_depth, _is_reference);
	ret->_pointer_depth++;
	return ret;
}

ParsedType* ParsedType::copy_dec_depth()
{
	ParsedType* ret = new ParsedType(_lexical_type, _pointer_depth, _is_reference);
	ret->_pointer_depth--;
	return ret;
}

string ParsedType::to_string()
{
	string ret = string(GET_LEX_TYPE_STR(_lexical_type));
	for(int i = 0; i < _pointer_depth; i++) ret += "*";
	return ret;
}

const char* ParsedType::to_c_string()
{
	// ez
	return strdup(to_string().c_str());
}


bool EviType::operator==(const EviType& rhs)
{
	return _name == rhs._name
		&& _parsed_type == rhs._parsed_type;
}

llvm::Type* EviType::get_llvm_type()
{
	llvm::Type* ret = _llvm_type;
	for(int i =0; i < _parsed_type->_pointer_depth; i++) ret = ret->getPointerTo();
	return ret;
}

string EviType::to_string()
{
	string ret = _name;
	for(int i = 0; i < _parsed_type->_pointer_depth; i++) ret += "*";
	return ret;
}


const char* lexical_type_strings[TYPE_NONE];

map<string, EviType*> __evi_types;
llvm::LLVMContext __context;