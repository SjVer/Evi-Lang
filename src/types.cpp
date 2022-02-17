#include "types.hpp"

ParsedType::ParsedType(LexicalType lexical_type, EviType* evi_type,
			   		   bool is_reference, uint pointer_depth, vector<int> array_sizes)
{
	_lexical_type = lexical_type;

	if(evi_type) _evi_type = evi_type;
	else switch(lexical_type)
	{
		case TYPE_BOOL: 	 _evi_type = GET_EVI_TYPE("bln"); break;
		case TYPE_CHARACTER: _evi_type = GET_EVI_TYPE("chr"); break;
		case TYPE_INTEGER: 	 _evi_type = GET_EVI_TYPE("i32"); break;
		case TYPE_FLOAT: 	 _evi_type = GET_EVI_TYPE("flt"); break;
		case TYPE_VOID: 	 _evi_type = GET_EVI_TYPE("nll"); break;
		default: assert(false);
	}

	_is_reference = is_reference;
	_pointer_depth = pointer_depth;
	_array_sizes = array_sizes;
}

// bool ParsedType::operator==(const ParsedType& rhs)
bool ParsedType::eq(ParsedType* rhs)
{
	return _lexical_type == rhs->_lexical_type
		&& _pointer_depth == rhs->_pointer_depth
		&& _array_sizes == rhs->_array_sizes
		&& _evi_type == rhs->_evi_type;
}

ParsedType* ParsedType::copy()
{
	ParsedType* ret = new ParsedType(_lexical_type, _evi_type, _is_reference, _pointer_depth, _array_sizes);
	return ret;
}

ParsedType* ParsedType::copy_change_lex(LexicalType type)
{
	ParsedType* ret = this->copy();
	ret->_lexical_type = type;
	return ret;
}

ParsedType* ParsedType::copy_inc_ptr_depth()
{
	ParsedType* ret = this->copy();
	ret->_pointer_depth++;
	return ret;
}

ParsedType* ParsedType::copy_dec_ptr_depth()
{
	ParsedType* ret = this->copy();
	ret->_pointer_depth--;
	return ret;
}

string ParsedType::to_string()
{
	string ret = _evi_type->_name;
	for(int i = 0; i < _array_sizes.size(); i++)
		ret += _array_sizes[i] >= 0 ? tools::fstr("|%u", _array_sizes[i]) : "|?";
	for(int i = 0; i < _pointer_depth; i++) ret += '*';
	return ret;
}

const char* ParsedType::to_c_string()
{
	// ez
	return strdup(to_string().c_str());
}

llvm::Type* ParsedType::get_llvm_type()
{
	llvm::Type* ret = _evi_type->_llvm_type;
	for(int i = 0; i < _array_sizes.size(); i++) ret = ret->getPointerTo(); // TODO: change?
	for(int i = 0; i < _pointer_depth; i++) ret = ret->getPointerTo();
	return ret;
}

bool ParsedType::is_signed()
{
	if(_pointer_depth || _array_sizes.size()) return false;
	return _evi_type->_is_signed;
}

uint ParsedType::get_alignment()
{
	if(_pointer_depth || _array_sizes.size()) return POINTER_ALIGNMENT;
	else return _evi_type->_alignment;
}



bool EviType::operator==(const EviType& rhs)
{
	// name is enough
	return _name == rhs._name;
}


const char* lexical_type_strings[TYPE_NONE];

map<string, EviType*> __evi_types;
llvm::LLVMContext __context;