#include "types.hpp"

ParsedType::ParsedType(LexicalType lexical_type, EviType* evi_type,
			   		   bool is_reference, ParsedType* subtype)
{
	_lexical_type = lexical_type;

	if(evi_type) _evi_type = evi_type;
	else switch(lexical_type)
	{
		case TYPE_BOOL: 	 _evi_type = GET_EVI_TYPE("bln"); break;
		case TYPE_CHARACTER: _evi_type = GET_EVI_TYPE("chr"); break;
		case TYPE_INTEGER: 	 _evi_type = GET_EVI_TYPE("i32"); break;
		case TYPE_FLOAT: 	 _evi_type = GET_EVI_TYPE("dbl"); break;
		case TYPE_VOID: 	 _evi_type = GET_EVI_TYPE("nll"); break;
		case TYPE_NONE:		 // break;
		default: THROW_INTERNAL_ERROR("in type construction");
	}

	_is_reference = is_reference;
	_keep_as_reference = false;
	_invalid = false;
	_subtype = subtype;
}

ParsedType* ParsedType::new_invalid()
{
	ParsedType* type = new ParsedType();
	type->_invalid = true;
	return type;
}

ParsedType* ParsedType::copy()
{
	ASSERT_OR_THROW_INTERNAL_ERROR(!is_invalid(), "during llvm type generation");

	ParsedType* ret = new ParsedType(_lexical_type, _evi_type, false, nullptr);
	ret->_is_constant = _is_constant;
	if(_subtype) ret->_subtype = _subtype->copy();
	return ret;
}

ParsedType* ParsedType::copy_change_lex(LexicalType type)
{
	ParsedType* ret = this->copy();
	set_lex_type(type);
	return ret;
}

ParsedType* ParsedType::copy_pointer_to()
{
	ParsedType* ret = this->copy();
	ret->_subtype = this->copy();
	return ret;
}

ParsedType* ParsedType::copy_element_of()
{
	ASSERT_OR_THROW_INTERNAL_ERROR(_subtype, "in type clone");
	return _subtype->copy();
}

void ParsedType::set_lex_type(LexicalType type)
{
	_lexical_type = type;
	if(_subtype) _subtype->set_lex_type(type);
}


string ParsedType::to_string(bool __first)
{
	if(is_invalid()) return "???";

	string str = _subtype ? _subtype->to_string(false) + '*' : _evi_type->_name;
	if(_is_constant && __first) return '!' + str;
	else return str;
}

ccp ParsedType::to_c_string()
{
	// ez
	return strdup(to_string().c_str());
}

llvm::Type* ParsedType::get_llvm_type()
{
	ASSERT_OR_THROW_INTERNAL_ERROR(!is_invalid(), "during llvm type generation");

	if(_subtype) 
	{
		// for llvm void* is invalid
		if(!_subtype->is_pointer() && _subtype->_lexical_type == TYPE_VOID)
			return llvm::IntegerType::getInt8PtrTy(__context);
		return _subtype->get_llvm_type()->getPointerTo();
	}
	else return _evi_type->_llvm_type;
}


bool ParsedType::eq(ParsedType* rhs, bool simple)
{
	if(simple && (!get_depth() && _lexical_type == TYPE_BOOL && !rhs->get_depth() && rhs->_lexical_type != TYPE_VOID))
		return true;

	if(_subtype) return get_depth() == rhs->get_depth() && _subtype->eq(rhs->_subtype);

	return _lexical_type == rhs->_lexical_type
		&& (simple || _evi_type->eq(rhs->_evi_type));
}

uint ParsedType::get_alignment()
{
	if(_subtype) return POINTER_ALIGNMENT;
	else return _evi_type->_alignment;
}

uint ParsedType::get_depth()
{
	if(_subtype) return _subtype->get_depth() + 1;
	else return 0;
}

bool ParsedType::is_pointer()
{
	// ez
	return (bool)_subtype;
}

bool ParsedType::is_signed()
{
	if(_subtype) return false;
	return _evi_type->_is_signed;
}

bool ParsedType::is_constant()
{
	return _is_constant;
}


bool ParsedType::is_invalid()
{
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wundefined-bool-conversion"
	#pragma clang diagnostic ignored "-Wtautological-undefined-compare"
	return !this || _invalid;
	#pragma clang diagnostic pop
}


bool EviType::eq(EviType* rhs)
{
	ASSERT_OR_THROW_INTERNAL_ERROR(rhs, "in type comparison");
	// name is enough
	return _name == rhs->_name;
}


ccp lexical_type_strings[TYPE_NONE];

map<string, EviType*> __evi_types;
llvm::LLVMContext __context;