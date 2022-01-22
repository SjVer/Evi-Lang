#include "debug.hpp"
#include "tools.hpp"

#define ADD_NODE(name) (_stream << \
	tools::fstr("\tnode%d [label=\"%s\"]\n", _nodecount, name), _nodecount++)

#define CONNECT_NODES(node1, node2) (_stream << \
	tools::fstr("\tnode%d -> node%d\n", node1, node2))

void ASTVisualizer::visualize(string path, AST* astree)
{
	_stream = stringstream();
	_nodecount = 1;

	_stream << HEADER;

	for(auto& node : *astree)
	{
		// node id will be _nodecount
		CONNECT_NODES(0 /* root */, _nodecount);
		node->accept(this);
	}

	_stream << FOOTER << endl;

	// write to file
	DEBUG_PRINT_MSG("Generating AST image...");
	int status = system(tools::fstr("echo '%s' | dot -Tsvg > %s", _stream.str().c_str(), path.c_str()).c_str());
	if(status) cout << _stream.str() << endl;
}

// =========================================
// All visit methods MUST invoke ADD_NODE() at least once!

// === Statements ===

// FuncDeclNode
void ASTVisualizer::visit(FuncDeclNode* node)
{
	string namelabel = tools::fstr("Declare \\\"%s\\\"", node->_identifier.c_str());
	int thisnode = ADD_NODE(namelabel.c_str());
	
	CONNECT_NODES(thisnode, ADD_NODE(node->_type._name.c_str()));

	CONNECT_NODES(thisnode, _nodecount);
	node->_body->accept(this);
}

// VarDeclNode
void ASTVisualizer::visit(VarDeclNode* node)
{
	string namelabel = tools::fstr("Declare \\\"%s\\\"", node->_identifier.c_str());
	int thisnode = ADD_NODE(namelabel.c_str());
	
	CONNECT_NODES(thisnode, ADD_NODE(node->_type._name.c_str()));

	CONNECT_NODES(thisnode, _nodecount);
	node->_expr->accept(this);
}

// BlockNode
void ASTVisualizer::visit(BlockNode* node)
{
	int thisnode = ADD_NODE("Block");
	for(auto& node : node->_statements)
	{
		// node id will be _nodecount
		CONNECT_NODES(thisnode, _nodecount);
		node->accept(this);
	}
}

// === Expressions ===

// Logical
void ASTVisualizer::visit(LogicalNode* node)
{
	int thisnode;
	switch(node->_optype)
	{
		case TOKEN_PIPE_PIPE: thisnode = ADD_NODE("||"); break;
		case TOKEN_AND_AND:   thisnode = ADD_NODE("&&"); break;
		default: assert(false);
	}

	CONNECT_NODES(thisnode, _nodecount);
	node->_left->accept(this);
	CONNECT_NODES(thisnode, _nodecount);
	node->_right->accept(this);
}

// BinaryNode
void ASTVisualizer::visit(BinaryNode* node)
{
	int thisnode;
	switch(node->_optype)
	{
		case TOKEN_PLUS:  			thisnode = ADD_NODE("+"); break;
		case TOKEN_MINUS: 			thisnode = ADD_NODE("-"); break;
		case TOKEN_STAR:  			thisnode = ADD_NODE("*"); break;
		case TOKEN_SLASH:			thisnode = ADD_NODE("/"); break;
		case TOKEN_PIPE: 	  		thisnode = ADD_NODE("|"); break;
		case TOKEN_AND:       		thisnode = ADD_NODE("&"); break;
		case TOKEN_GREATER_GREATER: thisnode = ADD_NODE(">>"); break;
		case TOKEN_LESS_LESS: 		thisnode = ADD_NODE("<<"); break;
		default: assert(false);
	}

	CONNECT_NODES(thisnode, _nodecount);
	node->_left->accept(this);
	CONNECT_NODES(thisnode, _nodecount);
	node->_right->accept(this);
}

// UnaryNode
void ASTVisualizer::visit(UnaryNode* node)
{
	int thisnode;
	switch(node->_optype)
	{
		case TOKEN_BANG:  	  	thisnode = ADD_NODE("!"); break;
		case TOKEN_MINUS:  	  	thisnode = ADD_NODE("-"); break;
		case TOKEN_PLUS_PLUS: 	thisnode = ADD_NODE("++"); break;
		case TOKEN_MINUS_MINUS: thisnode = ADD_NODE("--"); break;
		default: assert(false);
	}

	CONNECT_NODES(thisnode, _nodecount);
	node->_expr->accept(this);
}

// LiteralNode
void ASTVisualizer::visit(LiteralNode* node)
{
	ADD_NODE(tools::unescstr(node->_token, true, false).c_str());
}

#undef ADD_NODE
#undef CONNECT_NODES