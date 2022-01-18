#ifndef EVI_VM_H
#define EVI_VM_H

#include "common.hpp"
#include "darray.hpp"
#include "expression.hpp"
#include "value.hpp"

#include <vector>

using namespace std;

class VM {
public:
	string source, path;
	vector<Expression> frames;
	Expression frame;
	bool repl_mode;

	VM();
	VM(const string path, const string source, bool repl_mode);
	void reset();
	InterpretResult interpret();

private:
	// DArray<Value> stack;
	vector<Value> stack;

	Value stackTop();
	void push(Value value);
	Value pop();
	Value peek(int distance);

	void prevFrame();
	void newFrame(Expression expression);
	Value run(Expression expression);
};

void runtimeError(const char *format, ...);
bool isTrue(Value value);
bool valuesEqual(Value a, Value b);
#endif