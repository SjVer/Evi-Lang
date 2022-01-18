#ifndef EVI_EXPRESSION_H
#define EVI_EXPRESSION_H

#include <cstdarg>

#include "common.hpp"
#include "darray.hpp"
#include "value.hpp"

class Expression
{
public:
	string name;
	string path;

	DArray<Value> variables;
	DArray<Value> constants;

	DArray<Expression> expressions;

	int ip;
	DArray<uint8_t> bytes;
	DArray<int> lines;

	bool invalid;

	Expression()
	{
		name = "";
		path = "";
		variables.clear();
		constants.clear();
		expressions.clear();
		ip = 0;
		bytes.clear();
		lines.clear();
		invalid = false;
	}

	Expression(const string _name, const string _path)
	{
		name = _name;
		path = _path;
		variables.clear();
		constants.clear();
		expressions.clear();
		ip = 0;
		bytes.clear();
		lines.clear();
		invalid = false;
	}

	int addConstant(Value value)
	{
		constants.append(value);
		return constants.length() - 1;
	}

	void emit(int line, uint8_t byte)
	{
		// printf("emitting %d on line %d\n", byte, line);
		bytes.append(byte);
		lines.append(line);
	}
	void emit(int line, uint8_t byte1, uint8_t byte2)
	{
		emit(line, byte1);
		emit(line, byte2);
	}
	void emit(int line, uint8_t byte1, uint8_t byte2, uint8_t byte3)
	{
		emit(line, byte1);
		emit(line, byte2, byte3);
	}
	void emit(int line, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4)
	{
		emit(line, byte1);
		emit(line, byte2, byte3, byte4);
	}
};

#endif