#ifndef EVI_TYPECHECKER_H
#define EVI_TYPECHECKER_H

#include "common.hpp"
#include "error.hpp"
#include "ast.hpp"

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
		VISIT(LogicalNode);
		VISIT(BinaryNode);
		VISIT(UnaryNode);
		VISIT(GroupingNode);
			VISIT(LiteralNode);
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
	stack<LexicalType> _type_stack;
	void push(LexicalType type);
	LexicalType pop();

	LexicalType resolve_types(LexicalType left, LexicalType right);
};

#endif