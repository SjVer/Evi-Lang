#ifndef EVI_DEBUG_H
#define EVI_DEBUG_H

#include "ast.hpp"
#include <sstream>
#include <string>

using namespace std;

class ASTVisualizer: public Visitor
{
	public:
	void visualize(string path, AST* astree);
	
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
		VISIT(CastNode);
		VISIT(UnaryNode);
		VISIT(GroupingNode);
		VISIT(SubscriptNode);
			VISIT(LiteralNode);
			VISIT(ArrayNode);
			VISIT(ReferenceNode);
			VISIT(CallNode);
	#undef VISIT

	private:
	stringstream _stream;
	int _nodecount;
};

#define HEADER "digraph astgraph {\n\
	node [shape=rect, fontsize=12, fontname=\"Courier\", height=.1];\n\
	ranksep=.4;\n\
	edge [arrowsize=.5, arrowhead=\"none\"]\n\
	rankdir=\"UD\"\n\
	node0 [label=\"Program\"]\n\
	\n\
"
#define FOOTER "}"

#endif