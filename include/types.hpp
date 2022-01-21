#ifndef EVI_TYPES_H
#define EVI_TYPES_H

#include "common.hpp"
#include "phc.h"

#include <algorithm>
#include <map>
#include <string>

// ================================

typedef enum
{
	INIT_INTEGER,
	INIT_FLOAT,
	INIT_CHARACTER,
	INIT_STRING
} InitializerType;

typedef struct
{
	llvm::Type* _llvm_type;
	InitializerType _initializer_type;
	string _name;
	int _alignment;
} EviType;

#define EVI_INT_TYPE(name, bitsnum) \
	((EviType){llvm::IntegerType::get(__context, bitsnum), INIT_INTEGER, name, 4})

// ================================

extern llvm::LLVMContext __context;
extern map<string, EviType> __evi_types;
static bool __evi_builtin_types_initialized = false;

// ================================

#define ADD_EVI_TYPE(name, type) __evi_types.insert(pair<string, EviType>(name, type))
#define IS_EVI_TYPE(name) (__evi_types.find(name) != __evi_types.end())
#define GET_EVI_TYPE(name) (assert(IS_EVI_TYPE(name)), __evi_types.at(name))

// initialize built-in types
static void init_builtin_evi_types()
{
	// prevent multiple initializations
	if(__evi_builtin_types_initialized) return;
	__evi_builtin_types_initialized = true;

	ADD_EVI_TYPE(string("i1"),  EVI_INT_TYPE("i1", 1));
	ADD_EVI_TYPE(string("i4"),  EVI_INT_TYPE("i4", 4));
	ADD_EVI_TYPE(string("i8"),  EVI_INT_TYPE("i8", 8));
	ADD_EVI_TYPE(string("i16"), EVI_INT_TYPE("i16", 16));
	ADD_EVI_TYPE(string("i32"), EVI_INT_TYPE("i32", 32));
}

#endif