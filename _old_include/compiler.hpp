#ifndef EVI_COMPILER_H
#define EVI_COMPILER_H

#include "value.hpp"
#include "expression.hpp"
#include "scanner.hpp"

// ==== parsing stuff ====

typedef enum
{
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_TERNARY,	 // ?
	PREC_OR,		 // ||
	PREC_AND,		 // &&
	PREC_EQUALITY,	 // == !=
	PREC_COMPARISON, // < > <= >=
	PREC_TERM,	 // + - @
	PREC_FACTOR,	 // * /
	PREC_UNARY,		 // ! - $
	PREC_CALL,		 // ()
	PREC_PRIMARY	 // literals n shit
} Precedence;

// typedef void (*ParseFn)(bool canAssign);
class Compiler;
typedef void (*ParseFn)(bool canAssign);

typedef struct
{
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

// ==== ============= ====

class Compiler
{
public:
	Compiler() {}
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;

	Expression curExpr;
};

Expression compile(const char *path, const char* source);

#endif