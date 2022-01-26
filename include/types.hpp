#ifndef EVI_TYPES_H
#define EVI_TYPES_H

#include "common.hpp"
#include "pch.h"

#include <algorithm>
#include <map>
#include <string>

// ================================

typedef enum
{
	TYPE_INTEGER,
	TYPE_FLOAT,
	TYPE_CHARACTER,
	TYPE_STRING,

	__TYPE_NONE
} LexicalType;

extern const char* lexical_type_strings[__TYPE_NONE];

#define GET_LEX_TYPE_STR(type) (lexical_type_strings[type])

typedef struct
{
	llvm::Type* _llvm_type;
	LexicalType _lexical_type;
	string _name;
	int _alignment;
} EviType;

#define EVI_INT_TYPE(name, bitsnum) \
	((EviType){llvm::IntegerType::get(__context, bitsnum), TYPE_INTEGER, name, 4})

// ================================

extern llvm::LLVMContext __context;
extern map<string, EviType> __evi_types;
static bool __evi_builtin_types_initialized = false;

// ================================

#define ADD_EVI_TYPE(name, type) __evi_types.insert(pair<string, EviType>(name, type))
#define IS_EVI_TYPE(name) (__evi_types.find(name) != __evi_types.end())
#define GET_EVI_TYPE(name) (assert(IS_EVI_TYPE(name)), __evi_types.at(name))

static void init_builtin_evi_types()
{
	// prevent multiple initializations
	if(__evi_builtin_types_initialized) return;
	__evi_builtin_types_initialized = true;

	lexical_type_strings[TYPE_INTEGER] = "integer";
	lexical_type_strings[TYPE_FLOAT] = "float";
	lexical_type_strings[TYPE_CHARACTER] = "character";
	lexical_type_strings[TYPE_STRING] = "string";

	ADD_EVI_TYPE(string("i1"),  EVI_INT_TYPE("i1", 1));
	ADD_EVI_TYPE(string("i4"),  EVI_INT_TYPE("i4", 4));
	ADD_EVI_TYPE(string("i8"),  EVI_INT_TYPE("i8", 8));
	ADD_EVI_TYPE(string("i16"), EVI_INT_TYPE("i16", 16));
	ADD_EVI_TYPE(string("i32"), EVI_INT_TYPE("i32", 32));
}

#endif