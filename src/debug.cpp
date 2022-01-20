#include "debug.hpp"
#include "tools.hpp"

stringstream _stream;

void ASTVisualizer::visualize(string path, AST* astree)
{
	_stream = stringstream();
	_nodecount = -1;

	_stream << HEADER;

	for(auto& node : *astree) node->accept(this);

	_stream << FOOTER << endl;

	cout << _stream.str();
}

#define ADD_NODE(name) (_stream << \
	tools::fstr("\tnode%d [label=\"%s\"]\n", _nodecount++, name), _nodecount)

#define CONNECT_NODES(node1, node2) (_stream <<\
	tools::fstr("\tnode%d -> node%d\n", node1, node2))

// =========================================

// VarDeclNode
void ASTVisualizer::visit(VarDeclNode* node)
{
	int thisnode = ADD_NODE("Variable Declaration");
	CONNECT_NODES(thisnode, ADD_NODE(node->_identifier.c_str()));
	CONNECT_NODES(thisnode, ADD_NODE(STRINGIFY(node->_type)));
}