#include "debug.hpp"
#include "tools.hpp"

#define ADD_NODE(name) (_stream << \
	tools::fstr("\tnode%d [label=\"%s\"]\n", _nodecount++, name), _nodecount - 1)

#define CONNECT_NODES(node1, node2) (_stream <<\
	tools::fstr("\tnode%d -> node%d\n", node1, node2))

void ASTVisualizer::visualize(string path, AST* astree)
{
	_stream = stringstream();
	_nodecount = 1;

	_stream << HEADER;

	for(auto& node : *astree) if(node) 
	{
		// node id will be _nodecount
		CONNECT_NODES(0 /* root */, _nodecount);
		_stream << "// root connected" << endl;
		node->accept(this);
	}

	_stream << FOOTER << endl;

	// write to file
	system(tools::fstr("echo '%s' | dot -Tsvg > %s", _stream.str().c_str(), path.c_str()).c_str());
}

// =========================================

// VarDeclNode
void ASTVisualizer::visit(VarDeclNode* node)
{
	int thisnode = ADD_NODE("Var Decl");
	CONNECT_NODES(thisnode, ADD_NODE(node->_identifier.c_str()));
	CONNECT_NODES(thisnode, ADD_NODE("type?"));
}

#undef ADD_NODE
#undef CONNECT_NODES