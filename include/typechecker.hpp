#ifndef EVI_TYPECHECKER_H
#define EVI_TYPECHECKER_H

#include "common.hpp"
#include "error.hpp"
#include "ast.hpp"
#include "lint.hpp"

#include <stack>

class TypeChecker: public Visitor
{
	public:
	Status check(string path, const char* source, AST* astree);

	#define VISIT(_node) void visit(_node* node)
	VISIT(FuncDeclNode);
	VISIT(VarDeclNode);
	VISIT(AssignNode);
	VISIT(IfNode);
	VISIT(LoopNode);
	VISIT(ReturnNode);
	VISIT(BlockNode);
		VISIT(SubscriptNode);
		VISIT(LogicalNode);
		VISIT(BinaryNode);
		VISIT(CastNode);
		VISIT(UnaryNode);
		VISIT(GroupingNode);
			VISIT(LiteralNode);
			VISIT(ArrayNode);
			VISIT(ReferenceNode);
			VISIT(CallNode);
	#undef VISIT

	private:
	string _infile;
	ErrorDispatcher _error_dispatcher;

	void error_at(Token *token, string message);
	void warning_at(Token *token, string message);

	// bc the visitor methods return void we instead
	// just use a stack for the stuff
	stack<ParsedType*> _type_stack;
	void push(ParsedType* type);
	ParsedType* pop();

	ParsedType* resolve_types(ParsedType* left, ParsedType* right);
	bool can_cast_types(ParsedType* from, ParsedType* to);

	bool _panic_mode;
	bool _had_error;
};

#endif