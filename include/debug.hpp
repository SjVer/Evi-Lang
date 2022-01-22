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
	VISIT(BlockNode);
		VISIT(LogicalNode);
		VISIT(BinaryNode);
		VISIT(UnaryNode);
		VISIT(LiteralNode);
	#undef VISIT

	private:
	stringstream _stream;
	int _nodecount;
};

#define HEADER "digraph astgraph {\n\
	node [shape=none, fontsize=12, fontname=\"Courier\", height=.1];\n\
	ranksep=.3;\n\
	edge [arrowsize=.5]\n\
	\n\
	node0 [label=\"Program\"]\n\
	\n\
"
#define FOOTER "}"

#endif