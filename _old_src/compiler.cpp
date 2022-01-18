#include "common.hpp"
#include "compiler.hpp"
#include "scanner.hpp"
#include "instructions.hpp"
// #include "object.h"
#ifdef DEBUG
#include "debug.hpp"
#endif

Compiler comp;
Scanner scanner;

#define EMIT(bytes...) comp.curExpr.emit(comp.previous.line, bytes)

// ======= forward declarations =======

static void expression(bool consumeNewline);

// static void grouping(bool canAssign);
static void number(bool canAssign);
// static void unary(bool canAssign);
static void binary(bool canAssign);
// static void ternary(bool canAssign);
// static void postfix(bool canAssign);
// static void call(bool canAssign);
// static void literal(bool canAssign);
// static void string(bool canAssign);
// static void variable(bool canAssign);
// static void and_(bool canAssign);
// static void or_(bool canAssign);
// static void dot(bool canAssign);
// static void index(bool canAssign);
// static void array(bool canAssign);

// ======= error stuff =======

static void errorAt(Token *token, const char *message)
{
	// already in panicmode. swallow error.
	if (comp.panicMode)
		return;

	comp.panicMode = true;

	fprintf(stderr, "[%s:%d] Error", comp.curExpr.path.c_str(), token->line);

	if (token->type == TOKEN_EOF)
	{
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR)
	{
		// Nothing.
	}
	else
	{
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	comp.hadError = true;
}

// displays an error at the previous token with the given message
static void errorAtPrevious(const char *message)
{
	//
	errorAt(&comp.previous, message);
}

// displays an error at the current token with the given message
static void errorAtCurrent(const char *message)
{
	//
	errorAt(&comp.current, message);
}

// ======= bytecode stuff =======

// create a constant for the stack and return its index
static uint8_t makeConstant(Value value)
{
	int constant = comp.curExpr.addConstant(value);
	if (constant > UINT8_MAX)
	{
		errorAtPrevious("Too many constants in one expression.");
		return 0;
	}

	return (uint8_t)constant;
}

// emit a constant to the comp.current chunk
static void emitConstant(Value value)
{
	//
	EMIT(OP_CONSTANT, makeConstant(value));
}

// ======= token flow stuff =======

// advances to the next token
static void advance()
{
	comp.previous = comp.current;

	for (;;)
	{
		comp.current = scanner.scanToken();
		if (comp.current.type != TOKEN_ERROR)
			break;

		errorAtCurrent(comp.current.start);
	}
}

// checks if the current token is of the given type
static bool check(TokenType type)
{
	//
	return comp.current.type == type;
}

// consume the next token if it is of the correct type,
// otherwise throw an error with the given message
static void consume(TokenType type, const char *message)
{
	if (comp.current.type == type)
	{
		advance();
		return;
	}

	errorAtCurrent(message);
}

// returns true and advances if the comp.current token is of the given type
static bool match(TokenType type)
{
	if (!check(type))
		return false;
	advance();
	return true;
}

static void consumeNewlines(bool acceptEOF, const char *message)
{
	if(check(TOKEN_EOF))
	{
		if (!acceptEOF) errorAtCurrent(message);
		return;
	}

	else if (match(TOKEN_NEWLINE))
	{
		while(match(TOKEN_NEWLINE)) {}
		return;
	}

	else errorAtCurrent(message);
}

static void skipNewlines()
{
	//
	while(match(TOKEN_NEWLINE)) {}
}

// ======= grammar stuff =======

// returns the rule of the given token
static ParseRule getRule(TokenType type)
{
	// return &rules[type];
	switch(type)
	{
		//    token				            prefix,  	infix,  	precedence
		// case TOKEN_LEFT_PAREN:   return {grouping,	call,		PREC_CALL};
		// case TOKEN_RIGHT_PAREN: 	return {NULL, 		NULL,		PREC_NONE};
		// case TOKEN_LEFT_BRACE: 	return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_RIGHT_BRACE: 	return {NULL, 		NULL,		PREC_NONE};
		// case TOKEN_LEFT_B_BRACE: return {array, 		index,  	PREC_CALL};
		// case TOKEN_LEFT_B_BRACE: return {NULL, 		NULL,		PREC_CALL};
		// case TOKEN_RIGHT_B_BRACE:return {NULL, 		NULL,		PREC_NONE};
		// case TOKEN_COMMA: 		return {NULL, 		NULL,		PREC_NONE};
		// case TOKEN_DOT: 			return {NULL, 		NULL,		PREC_CALL};
		case TOKEN_MINUS: 			return {NULL, 		binary,		PREC_TERM};
		//// case TOKEN_MINUS_MINUS:return {NULL, 		postfix,	PREC_CALL};
		//// case TOKEN_MINUS_EQUAL:return {NULL, 		NULL,		PREC_NONE};
		case TOKEN_PLUS: 			return {NULL, 		binary,		PREC_TERM};
		case TOKEN_PLUS_PLUS:		return {NULL,		binary,		PREC_TERM};
		// // case TOKEN_PLUS_EQUAL:return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_QUESTION: 	return {NULL, 		ternary,	PREC_TERNARY};
		// case TOKEN_COLON: 		return {NULL, 		NULL,		PREC_NONE};
		// case TOKEN_SEMICOLON: 	return {NULL, 		NULL,		PREC_NONE};
		case TOKEN_NEWLINE: 		return {NULL, 		NULL,		PREC_NONE};
		// case TOKEN_ARROW: 		return {NULL, 		NULL,		PREC_NONE};
		case TOKEN_SLASH: 			return {NULL, 		binary,		PREC_FACTOR};
		case TOKEN_SLASH_EQUAL: 	return {NULL, 		binary,		PREC_EQUALITY};
		case TOKEN_STAR: 			return {NULL, 		binary,		PREC_FACTOR};
		case TOKEN_MODULO:			return {NULL,		binary,		PREC_FACTOR};
		// case TOKEN_BANG] 		return {unary, 		NULL,		PREC_NONE};
		// case TOKEN_EQUAL: 		return {NULL, 		NULL,		PREC_NONE};
		case TOKEN_EQUAL_EQUAL: 	return {NULL, 		binary,		PREC_EQUALITY};
		case TOKEN_GREATER:			return {NULL, 		binary,		PREC_COMPARISON};
		case TOKEN_GREATER_EQUAL: 	return {NULL, 		binary,		PREC_COMPARISON};
		case TOKEN_LESS: 			return {NULL, 		binary,		PREC_COMPARISON};
		case TOKEN_LESS_EQUAL: 		return {NULL, 		binary,		PREC_COMPARISON};
		// case TOKEN_IDENTIFIER: 	return {variable,	NULL,		PREC_NONE};
		// case TOKEN_STRING: 		return {string, 	NULL,		PREC_NONE};
		case TOKEN_NUMBER: 			return {number, 	NULL,		PREC_NONE};
		// case TOKEN_AND: 			return {NULL, 		and_, 		PREC_AND};
		// case TOKEN_FALSE: 		return {literal,	NULL,		PREC_NONE};
		// case TOKEN_NULL:			return {literal,	NULL,		PREC_NONE};
		// case TOKEN_OR: 			return {NULL, 		or_, 		PREC_OR};
		// case TOKEN_RETURN: 		return {NULL, 		NULL,		PREC_NONE};
		// case TOKEN_TRUE: 		return {literal,	NULL,		PREC_NONE};
		case TOKEN_ERROR: 			return {NULL, 		NULL,		PREC_NONE};
		case TOKEN_EOF:	 			return {NULL, 		NULL,		PREC_NONE};
		
		default:
		{
			cout << "getRule defaulting" << endl;
			return {NULL, NULL, PREC_NONE}; 
		}
	}
}

// parses the comp.current expression with correct precedence
static void parsePrecedence(Precedence precedence)
{
	advance();

	// ParseFn prefixRule = getRule(comp.previous.type)->prefix;
	ParseFn prefixRule = getRule(comp.previous.type).prefix;

	if (prefixRule == NULL)
	{
		// printf("prevtype: %d '%.*s'\n", 
		// 	comp.previous.type, comp.previous.length, comp.previous.start);
		errorAtPrevious("Expect expression.");

		return;
	}

	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	// while (precedence <= getRule(comp.current.type)->precedence)
	while (precedence <= getRule(comp.current.type).precedence)
	{
		advance();

		// ParseFn infixRule = getRule(comp.previous.type)->infix;
		ParseFn infixRule = getRule(comp.previous.type).infix;

		infixRule(canAssign);
	}

	// if (canAssign && matchAssign())
	if (canAssign && match(TOKEN_EQUAL))
	{
		errorAtPrevious("Invalid assignment target.");
		expression(true);
	}
}

// ======= compilation stuff =======

// compile an expression
static void expression(bool consumeNewline)
{
	printf("expression started\n");

	parsePrecedence(PREC_ASSIGNMENT);
	if (consumeNewline)
		consumeNewlines(true, "Expect newline after expression.");

	printf("expression done\n");
}

// compiles a number
static void number(bool canAssign)
{
	char *numstr = strndup(comp.previous.start, comp.previous.length);
	double value = stod(numstr);
	printf("number: '%s' = %f\n", numstr, value);
	// double value = strtod(comp.previous.start, NULL);
	emitConstant(value);
}

// parses a binary expression
static void binary(bool canAssign)
{
	printf("binary: %d '%.*s'\n", comp.previous.type,
		comp.previous.length, comp.previous.start);


	TokenType operatorType = comp.previous.type;
	ParseRule rule = getRule(operatorType);

	skipNewlines();
	parsePrecedence((Precedence)(rule.precedence + 1));

	printf("end of binary\n");

	switch (operatorType)
	{
	case TOKEN_SLASH_EQUAL:
		EMIT(OP_EQUAL, OP_NOT);
		break;
	case TOKEN_EQUAL_EQUAL:
		EMIT(OP_EQUAL);
		break;
	case TOKEN_GREATER:
		EMIT(OP_GREATER);
		break;
	case TOKEN_GREATER_EQUAL:
		EMIT(OP_LESS, OP_NOT);
		break;
	case TOKEN_LESS:
		EMIT(OP_LESS);
		break;
	case TOKEN_LESS_EQUAL:
		EMIT(OP_GREATER, OP_NOT);
		break;
	case TOKEN_PLUS:
		EMIT(OP_ADD);
		break;
	case TOKEN_MINUS:
		EMIT(OP_SUBTRACT);
		break;
	case TOKEN_STAR:
		EMIT(OP_MULTIPLY);
		break;
	case TOKEN_SLASH:
		EMIT(OP_DIVIDE);
		break;
	case TOKEN_MODULO:
		EMIT(OP_MODULO);
		break;
	default:
		return; // Unreachable.
	}
}

// ======= main stuff =======

static void start()
{
	// emit OP__START_UP which the vm will
	// use to determine whether to call
	// main() or main(argv) and pass the args
	// EMIT(OP__START_UP);
}

static void finish()
{
}

Expression compile(const char *path, const char* source)
{
	scanner = Scanner(source);
	comp.hadError = false;
	comp.panicMode = false;

	comp.curExpr = Expression("base", string(path));

	start();

	advance();
	while (!match(TOKEN_EOF))
	{
		expression(true);
	}

	finish();

	if (comp.hadError) comp.curExpr.invalid = true;

	return comp.curExpr;
}