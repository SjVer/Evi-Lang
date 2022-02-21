#include "types.hpp"

ParsedType::ParsedType(LexicalType lexical_type, EviType* evi_type,
			   		   bool is_reference, SubType subtypetype, ParsedType* subtype)
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
		case TYPE_NONE:		 break;
		default: assert(false);
	}

	_is_reference = is_reference;
	_subtypetype = subtypetype;
	_subtype = subtype;
}

ParsedType* ParsedType::copy()
{
	ParsedType* ret = new ParsedType(
		_lexical_type, _evi_type, _is_reference,
		_subtypetype, _subtype);
	return ret;
}

ParsedType* ParsedType::copy_change_lex(LexicalType type)
{
	ParsedType* ret = this->copy();
	ret->_lexical_type = type;
	return ret;
}

ParsedType* ParsedType::copy_pointer_to()
{
	ParsedType* ret = this->copy();
	ret->_subtype = this->copy();
	ret->_subtypetype = SUBTYPE_POINTER;
	return ret;
}

ParsedType* ParsedType::copy_array_of(int len)
{
	ParsedType* ret = this->copy();
	ret->_subtype = this->copy();
	ret->_subtypetype = SUBTYPE_ARRAY;
	ret->_evi_type = (EviType*)((long)len);
	return ret;
}

ParsedType* ParsedType::copy_element_of()
{
	assert(_subtypetype != SUBTYPE_NONE);
	return _subtype->copy();
}


string ParsedType::to_string()
{
	if(_subtypetype == SUBTYPE_ARRAY && (long)_evi_type < 0)
		return _subtype->to_string() + "|?|";
	else if(_subtypetype == SUBTYPE_ARRAY)
		return _subtype->to_string() + tools::fstr("|%u|", (long)_evi_type);
	else if(_subtypetype == SUBTYPE_POINTER)
		return _subtype->to_string() + '*';
	else
		return _evi_type->_name;
}

const char* ParsedType::to_c_string()
{
	// ez
	return strdup(to_string().c_str());
}

llvm::Type* ParsedType::get_llvm_type()
{
	if(_subtypetype == SUBTYPE_ARRAY)
		return llvm::ArrayType::get(_subtype->get_llvm_type(), (long)_evi_type);
	else if(_subtypetype == SUBTYPE_POINTER)
		return _subtype->get_llvm_type()->getPointerTo();
	else return _evi_type->_llvm_type;
}


bool ParsedType::eq(ParsedType* rhs)
{
	if(_subtypetype != rhs->_subtypetype) return false;

	if(_subtypetype != SUBTYPE_NONE)
	{
		return _subtypetype == rhs->_subtypetype
			&& get_depth() == rhs->get_depth()
			&& _subtype->eq(rhs->_subtype);
	}

	return _lexical_type == rhs->_lexical_type
		&& _evi_type->eq(rhs->_evi_type);
}

uint ParsedType::get_alignment()
{
	if(_subtypetype != SUBTYPE_NONE) return POINTER_ALIGNMENT;
	else return _evi_type->_alignment;
}

uint ParsedType::get_depth()
{
	if(_subtypetype != SUBTYPE_NONE)
		return _subtype->get_depth() + 1;
	else return 0;
}

int ParsedType::get_array_size()
{
	assert(_subtypetype == SUBTYPE_ARRAY);
	return (long)_evi_type;
}

void ParsedType::set_array_size(int size)
{
	assert(_subtypetype == SUBTYPE_ARRAY);
	_evi_type = (EviType*)((long)size);
}

bool ParsedType::is_array()
{
	// ez
	return _subtypetype == SUBTYPE_ARRAY;
}

bool ParsedType::is_pointer()
{
	// ez
	return _subtypetype == SUBTYPE_POINTER;
}

bool ParsedType::is_signed()
{
	if(_subtypetype != SUBTYPE_NONE) return false;
	return _evi_type->_is_signed;
}



bool EviType::eq(EviType* rhs)
{
	assert(rhs);
	// name is enough
	return _name == rhs->_name;
}


const char* lexical_type_strings[TYPE_NONE];

map<string, EviType*> __evi_types;
llvm::LLVMContext __context;