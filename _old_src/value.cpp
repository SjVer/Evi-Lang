#include "value.hpp"

string Value::toString(bool debug)
{
	if (debug)
		switch(type)
		{
		case VAL_BLN: return as.boolean ? "true" : "false";
		case VAL_CHR: return fstr("'%c'", as.character);
		// case VAL_FLT: return fstr("%f", as.float_);
		// case VAL_INT: return fstr("%d", as.integer);
		case VAL_NUM: return fstr("%f", as.number);
		case VAL_OBJ: return "<obj>";
		}
	else
		switch(type)
		{
		case VAL_BLN: return as.boolean ? "true" : "false";
		case VAL_CHR: return string(1, as.character);
		// case VAL_FLT: return fstr("%f", as.float_);
		// case VAL_INT: return fstr("%d", as.integer);
		case VAL_NUM: return fstr("%g", as.number);
		case VAL_OBJ: return "<obj>";
		}
	return "<VALUE-TO-STRING_ERROR>";
}

bool Value::equals(Value value)
{
	if (type != value.type) return false;

	switch(type)
	{
		case VAL_BLN: return as.boolean == AS_BLN(value);
		case VAL_CHR: return as.character == AS_CHR(value);
		case VAL_NUM: return as.number == AS_NUM(value);
		case VAL_OBJ: return false;
	}
	return false;
}