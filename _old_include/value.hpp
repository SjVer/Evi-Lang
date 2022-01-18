#ifndef EVI_VALUE_H
#define EVI_VALUE_H

#include "common.hpp"
#include "object.hpp"
#include "tools.hpp"

using namespace std;

#define IS_BLN(value) ((value).type == VAL_BLN)
#define IS_CHR(value) ((value).type == VAL_CHR)
// #define IS_FLT(value) ((value).type == VAL_FLT)
// #define IS_INT(value) ((value).type == VAL_INT)
#define IS_NUM(value) ((value).type == VAL_NUM)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BLN(value) ((value).as.boolean)
#define AS_CHR(value) ((value).as.character)
// #define AS_FLT(value) ((value).as.float_)
// #define AS_INT(value) ((value).as.integer)
#define AS_NUM(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.object)

typedef enum
{
	VAL_BLN,
	VAL_CHR,
	// VAL_FLT,
	// VAL_INT,
	VAL_NUM,
	VAL_OBJ
} ValueType;

class Value
{
public:
	ValueType type;
	union
	{
		bool boolean;
		char character;
		// double float_;
		// int integer;
		double number;
		Object *object;
	} as;

	Value(){}

	Value(bool bln) { type = VAL_BLN; as.boolean = bln; }
	operator bool() const { return as.boolean; }
	
	Value(char chr) { type = VAL_CHR; as.character = chr; }
	operator char() const { return as.character; }

	// Value(double flt) { type = VAL_FLT; as.float_ = flt; }
	// operator double() const { return as.float_; }

	// Value(int _int) { type = VAL_INT; as.integer = _int; }
	// operator int() const { return as.integer; }

	Value(double num) { type = VAL_NUM; as.number = num; }
	Value(int num) { type = VAL_NUM; as.number = num; }
	operator double() const { return as.number; }

	Value(Object *obj) { type = VAL_OBJ; as.object = obj; }
	operator Object*() const { return as.object; }

	string toString() { return toString(false); }

	string toString(bool debug);
	bool equals(Value value);
};

#endif