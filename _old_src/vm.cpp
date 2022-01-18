#include "common.hpp"
#include "expression.hpp"
#include "compiler.hpp"
#include "value.hpp"
#include "vm.hpp"
#include "instructions.hpp"
#include "debug.hpp"

#include <unistd.h>
#include <cmath>

// ======== VM stuff ========

VM *_vm;

VM::VM(){ reset(); }
VM::VM(const string path, const string source, bool repl_mode)
{
	reset();

	this->path = path;
	this->source = source;
	this->repl_mode = repl_mode;	
}

void VM::reset()
{
	stack.clear();
	frames.clear();
}

Value VM::stackTop()
{
	// returns value at top of stack
	return stack.back();
}

void VM::push(Value value)
{
	//
	stack.push_back(value);
}

Value VM::pop()
{
	Value popped = stack[stack.size() - 1];
	stack.pop_back();
	return popped;
}

Value VM::peek(int distance)
{
	//
	return stack[stack.size() - 1 - distance];
}

void VM::prevFrame()
{
	if (frames.size() == 1) return;

	frame = frames[frames.size() - 1];
	frames.pop_back();
}

void VM::newFrame(Expression expression)
{
	frames.push_back(expression);
	frame = frames[frames.size() - 1];
}

Value VM::run(Expression expression)
{
	frames.clear();
	newFrame(expression);

	for (;;)
	{
		#ifdef DEBUG
			cout << "STACK: ";
			for (Value value : stack)
			{
				cout << "[";
				cout << value.toString(true);
				cout << "]";
			}
			if (stack.empty()) cout << "(empty)";

			cout << "\nFRAMES: ";
			for (vector<Expression>::size_type i = 0; i < frames.size(); i++)
			{
				cout << frames[i].name;
				if (i != frames.size() - 1) cout << "->";
			}

			// printf("\nINSTRUCTION %d: ", (int)(frame.instructions.bytes[frame.ip]))
			cout << "\nINSTRUCTION: ";
			disassembleInstruction(&frame, frame.ip);
			cout << ">>> ";
		#endif

		cout << "\n\n";

		#define READ_BYTE() (frame.bytes[frame.ip++])
		#define READ_CONSTANT() (frame.constants[READ_BYTE()])
		#define BINARY_OP(castType, op)                        	\
			do                                                  \
			{                                                   \
				if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) 		\
				{                                               \
					runtimeError("Operands must be numbers.");  \
					return INTERPRET_RUNTIME_ERROR;             \
				}                                               \
				double b = AS_NUM(pop());                    	\
				double a = AS_NUM(pop());                   	\
				push(Value((castType)(a op b)));                \
			} while (false)

		uint8_t instruction;
		switch(instruction = READ_BYTE())
		{
			case OP_CONSTANT:
			{
				Value constant = READ_CONSTANT();
				push(constant);
				break;
			}
			case OP_TRUE:
			{
				push(Value(true));
				break;
			}
			case OP_FALSE:
			{
				push(Value(false));
				break;
			}
			case OP_POP:
			{
				pop();
				break;
			}

			case OP_NOT:
			{
				break;
			}

			case OP_ADD:
			{
				BINARY_OP(double, +);
				break;
			}
			case OP_SUBTRACT:
			{
				BINARY_OP(double, -);
				break;
			}
			case OP_MULTIPLY:
			{
				BINARY_OP(double, *);
				break;
			}
			case OP_DIVIDE:
			{
				BINARY_OP(double, /);
				break;
			}
			case OP_MODULO:
			{
				Value b = pop();
				Value a = pop();
				if (!(IS_NUM(a) && IS_NUM(b)))
				{
					runtimeError("Operands must be numbers");
					return INTERPRET_RUNTIME_ERROR;
				}
				push(Value((double)fmod(AS_NUM(a), AS_NUM(b))));
				break;
			}

			case OP_EQUAL:
			{
				Value b = pop();
				Value a = pop();
				push(Value(a.equals(b)));
				break;
			}
			case OP_GREATER:
			{
				BINARY_OP(bool, >);
				break;
			}
			case OP_LESS:
			{
				BINARY_OP(bool, <);
				break;
			}

			case OP_END_REPL:
			{
				
			}
		}

		sleep(1);
	}

	return Value((double)0);
}

InterpretResult VM::interpret()
{
	reset();
	_vm = this;

	InterpretResult result = {0, INTERPRET_SUCCESS};

	Expression expr = compile(path.c_str(), source.c_str());

	if (repl_mode)
	{
		expr.emit(-1, OP_END_REPL);
	}

	if (expr.invalid)
	{
		result.status = INTERPRET_COMPILE_ERROR;
		return result;
	}

	#ifdef DEBUG
		disassembleExpression(&expr);
	#endif

	Value exitCode = VM::run(expr);

	#ifdef DEBUG
		printf("\n");
	#endif
	
	result.exitCode = (double)exitCode;
	return result;
}

// =========================

// display a runtime error
void runtimeError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (int i = _vm->frames.size() - 1; i >= 0; i--)
	{
		Expression *frame = &_vm->frames[i];

		fprintf(stderr, "[%s:%d] in ", frame->path.c_str(), frame->lines[frame->ip]);
		fprintf(stderr, "%s\n", frame->name.c_str());
	}
}


bool isTrue(Value value)
{
	switch(value.type)
	{
		case VAL_BLN: return AS_BLN(value);
		case VAL_CHR: return AS_CHR(value) != '\0';
		case VAL_NUM: return AS_NUM(value) != 0.0;
		case VAL_OBJ: return true;
	}

	runtimeError("<VALUE-IS-TRUE-ERROR>");
	return false;
}