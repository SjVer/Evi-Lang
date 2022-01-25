#ifndef EVI_TYPECHECKER_H
#define EVI_TYPECHECKER_H

#include "common.hpp"
#include "error.hpp"
#include "ast.hpp"

class TypeChecker: public Visitor
{
	public:
	Status check(string path, AST* astree);

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
	ErrorDispatcher _dispatcher;
};

#endif