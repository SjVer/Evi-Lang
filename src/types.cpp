#include "types.hpp"

bool ParsedType::operator==(const ParsedType& rhs)
{
	// DEBUG_PRINT_MSG("=======================");
	// DEBUG_PRINT_VAR(_lexical_type, %d);
	// DEBUG_PRINT_VAR(rhs._lexical_type, %d);
	// DEBUG_PRINT_VAR(_pointer_depth, %d);
	// DEBUG_PRINT_VAR(rhs._pointer_depth, %d);
	// DEBUG_PRINT_VAR(_is_reference, %d);
	// DEBUG_PRINT_VAR(rhs._is_reference, %d);

	return _lexical_type == rhs._lexical_type
		&& _pointer_depth == rhs._pointer_depth;
}

ParsedType* ParsedType::copy()
{
	ParsedType* ret = new ParsedType(_lexical_type, _pointer_depth, _is_reference);
	return ret;
}

ParsedType* ParsedType::copy_change_lex(LexicalType type)
{
	ParsedType* ret = this->copy();
	ret->_lexical_type = type;
	return ret;
}

ParsedType* ParsedType::copy_inc_depth()
{
	ParsedType* ret = this->copy();
	ret->_pointer_depth++;
	return ret;
}

ParsedType* ParsedType::copy_dec_depth()
{
	ParsedType* ret = this->copy();
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