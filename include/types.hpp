#ifndef EVI_TYPES_H
#define EVI_TYPES_H

#include "common.hpp"
#include "pch.h"
#include "size.h"

#include <algorithm>
#include <map>
#include <string>

// ================================

typedef enum
{
	TYPE_BOOL,
	TYPE_CHARACTER,
	TYPE_INTEGER,
	TYPE_FLOAT,
	TYPE_VOID,

	TYPE_NONE
} LexicalType;

extern ccp lexical_type_strings[TYPE_NONE];
#define GET_LEX_TYPE_STR(type) (lexical_type_strings[type])

// ================================

class EviType
{
public:
	llvm::Type* _llvm_type;
	LexicalType _default_type;

	string _name;
	int _alignment;
	bool _is_signed;

	bool eq(EviType* rhs);
	string to_string();
};

#define EVI_INT_TYPE(name, bitsnum, issigned) \
	(new EviType{llvm::IntegerType::get(__context, bitsnum), \
		TYPE_INTEGER, name, 4, issigned})

// ================================

extern llvm::LLVMContext __context;
extern map<string, EviType*> __evi_types;
static bool __evi_builtin_types_initialized = false;

// ================================

class ParsedType
{
private:
	EviType* _evi_type;
	ParsedType* _subtype = nullptr;

	bool _invalid = false;

public:

	ParsedType() {};
	ParsedType(LexicalType lexical_type, 
			   EviType* evi_type = nullptr,
			   bool is_reference = false,
			   ParsedType* subtype = nullptr);
	static ParsedType* new_invalid();

	ParsedType* copy();
	ParsedType* copy_change_lex(LexicalType type);
	ParsedType* copy_pointer_to();
	ParsedType* copy_element_of();
	void set_lex_type(LexicalType type);

	string to_string(bool __first = true);
	ccp to_c_string();
	llvm::Type* get_llvm_type();

	bool eq(ParsedType* rhs, bool simple = false);
	uint get_alignment();
	uint get_depth();
	bool is_pointer();
	bool is_signed();
	bool is_constant();

	bool is_invalid();

	LexicalType _lexical_type;
	bool _is_constant = false;
	bool _is_reference = false;
	bool _keep_as_reference = false;
};

#define PTYPE(...) (new ParsedType(__VA_ARGS__))
#define AS_LEX(ptype) (ptype->_lexical_type)

// ================================

#define ADD_EVI_TYPE(name, type) __evi_types.insert(pair<string, EviType*>(name, type))
#define IS_EVI_TYPE(name) (__evi_types.find(name) != __evi_types.end())
#define GET_EVI_TYPE(name) (_get_evi_type(name))

static EviType* _get_evi_type(string name)
{
	ASSERT_OR_THROW_INTERNAL_ERROR(IS_EVI_TYPE(name), "in type retrieval");
	return __evi_types.at(name);	
}

static void init_builtin_evi_types()
{
	// prevent multiple initializations
	if(__evi_builtin_types_initialized) return;
	__evi_builtin_types_initialized = true;

	lexical_type_strings[TYPE_INTEGER] = "integer";
	lexical_type_strings[TYPE_FLOAT] = "float";
	lexical_type_strings[TYPE_BOOL] = "boolean";
	lexical_type_strings[TYPE_CHARACTER] = "character";
	lexical_type_strings[TYPE_VOID] = "void";

	// ======================= Integers =======================
	ADD_EVI_TYPE("i1",  EVI_INT_TYPE("i1",  1,  true));

	ADD_EVI_TYPE("i4",  EVI_INT_TYPE("i4",  4,  true));
	ADD_EVI_TYPE("ui4", EVI_INT_TYPE("ui4", 4,  false));

	ADD_EVI_TYPE("i8",  EVI_INT_TYPE("i8",  8,  true));
	ADD_EVI_TYPE("ui8", EVI_INT_TYPE("ui8", 8,  false));

	ADD_EVI_TYPE("i16", EVI_INT_TYPE("i16", 16, true));
	ADD_EVI_TYPE("ui16",EVI_INT_TYPE("ui16",16, false));

	ADD_EVI_TYPE("i32", EVI_INT_TYPE("i32", 32, true));
	ADD_EVI_TYPE("ui32",EVI_INT_TYPE("ui32",32, false));

	ADD_EVI_TYPE("i64", EVI_INT_TYPE("i64", 64, true));
	ADD_EVI_TYPE("ui64",EVI_INT_TYPE("ui64",64, false));

	ADD_EVI_TYPE("i128", EVI_INT_TYPE("i128", 128, true));
	ADD_EVI_TYPE("ui128",EVI_INT_TYPE("ui128",128, false));

	// ======================== Floats ========================
	ADD_EVI_TYPE("flt", (new EviType{llvm::Type::getFloatTy(__context),  TYPE_FLOAT, "flt", 4, true}));
	ADD_EVI_TYPE("dbl", (new EviType{llvm::Type::getDoubleTy(__context), TYPE_FLOAT, "dbl", 4, true}));

	// ======================== Others ========================
	ADD_EVI_TYPE("sze", EVI_INT_TYPE("sze", SIZE_TYPE_SIZE, false));
	ADD_EVI_TYPE("bln", (new EviType{llvm::Type::getInt1Ty(__context), TYPE_BOOL, "bln", 1, false}));
	ADD_EVI_TYPE("chr", (new EviType{llvm::Type::getInt8Ty(__context), TYPE_CHARACTER, "chr", 8, false}));
	ADD_EVI_TYPE("nll", (new EviType{llvm::Type::getVoidTy(__context), TYPE_VOID, "nll"}));
}

#endif