#include "typechecker.hpp"
#include "common.hpp"

Status TypeChecker::check(string path, AST* astree)
{
	_infile = path;

	for(auto& node : *astree)
	{
		node->accept(this);
	}

	DEBUG_PRINT_MSG("Type check done!");
	return STATUS_SUCCESS;
}

// =========================================
// All visit methods MUST invoke ADD_NODE() at least once!
#define VISIT(_node) void TypeChecker::visit(_node* node)

// === Statements ===

VISIT(FuncDeclNode)
{
}

VISIT(VarDeclNode)
{
}

VISIT(AssignNode)
{
}

VISIT(IfNode)
{
}

VISIT(LoopNode)
{
}

VISIT(ReturnNode)
{
}

VISIT(BlockNode)
{
}

// === Expressions ===

VISIT(LogicalNode)
{
}

VISIT(BinaryNode)
{
}

VISIT(UnaryNode)
{
}

VISIT(GroupingNode)
{
}


VISIT(LiteralNode)
{
}

VISIT(ReferenceNode)
{
}

VISIT(CallNode)
{
}

#undef VISIT