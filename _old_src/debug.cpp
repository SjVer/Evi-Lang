#include "tools.hpp"
#include "debug.hpp"
#include "instructions.hpp"

static int simpleInstruction(const char *name, int offset)
{
	cout << name << endl;
	return offset + 1;
}

static int constantInstruction(const char *name, Expression *expr, int offset)
{
    uint8_t constant = expr->bytes[offset + 1];
    printf("%-16s %4d = %s\n", name, constant, expr->constants[constant].toString(true).c_str());
    return offset + 2;
}

// =========================

int disassembleInstruction(Expression *expr, int offset)
{
	printf("%04d ", offset);

	// if line is same as previous print pipe, else print line
    if (offset > 0 && expr->lines[offset] == expr->lines[offset - 1])
    	printf("   |  ");
    else
    	printf("%4d  ", expr->lines[offset]);

    uint8_t instruction = expr->bytes[offset];
    switch(instruction)
    {
		case OP_CONSTANT:	return constantInstruction("OP_CONSTANT", expr, offset);
		case OP_TRUE:		return simpleInstruction("OP_TRUE", offset);
		case OP_FALSE:		return simpleInstruction("OP_FALSE", offset);
		case OP_POP:		return simpleInstruction("OP_POP", offset);

		case OP_NOT:		return simpleInstruction("OP_NOT", offset);

		case OP_ADD:		return simpleInstruction("OP_ADD", offset);
		case OP_SUBTRACT:	return simpleInstruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:	return simpleInstruction("OP_MULTIPLY", offset);
		case OP_DIVIDE:		return simpleInstruction("OP_DIVIDE", offset);
		case OP_MODULO:		return simpleInstruction("OP_MODULO", offset);

		case OP_EQUAL:		return simpleInstruction("OP_EQUAL", offset);
		case OP_GREATER:	return simpleInstruction("OP_GREATER", offset);
		case OP_LESS:		return simpleInstruction("OP_LESS", offset);

    	default:
	        printf("Unknown opcode %d\n", instruction);
	        return offset + 1;
    }
}


void disassembleExpression(Expression *expr)
{
	// printf("=== %s ===\n", expr->name);
	cout << "=== " << expr->name << " ===" << endl;

	for (int offset = 0; offset < expr->bytes.length();)
		offset = disassembleInstruction(expr, offset);

	// printf("=== ===== ===\n");
	cout << "=== ===== ===" << endl;
}