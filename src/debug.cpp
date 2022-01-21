#include "debug.hpp"
#include "tools.hpp"

#define ADD_NODE(name) (_stream << \
	tools::fstr("\tnode%d [label=\"%s\"]\n", _nodecount++, name), _nodecount - 1)

#define CONNECT_NODES(node1, node2) (_stream << \
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
	DEBUG_PRINT_MSG("Generating AST image...");
	system(tools::fstr("echo '%s' | dot -Tsvg > %s", _stream.str().c_str(), path.c_str()).c_str());
}

// =========================================
// All visit methods MUST invoke ADD_NODE() at least once!

// === Statements ===

// VarDeclNode
void ASTVisualizer::visit(VarDeclNode* node)
{
	string label1 = tools::fstr("Declare \\\"%s\\\"", node->_identifier.c_str());
	int thisnode = ADD_NODE(label1.c_str());
	
	string label2 = tools::fstr("type: %s", node->_type._name.c_str());
	CONNECT_NODES(thisnode, ADD_NODE(label2.c_str()));
}

// === Expressions ===

// LiteralNode
void ASTVisualizer::visit(LiteralNode* node)
{
	ADD_NODE("Some literal");
}

#undef ADD_NODE
#undef CONNECT_NODES