#include "debug.hpp"
#include "tools.hpp"

#define ADD_NODE(name) (_stream << \
	tools::fstr("\tnode%d [label=\"%s\"]\n", _nodecount, name), _nodecount++)

#define CONNECT_NODES(node1, node2) (_stream << \
	tools::fstr("\tnode%d -> node%d\n", node1, node2))

#define CONNECT_NODES_LABELED(node1, node2, label) (_stream << \
	tools::fstr("\tnode%d -> node%d [label=" #label ", fontsize=10, fontname=\"Courier\"]\n", node1, node2))

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
	else system(tools::fstr("eog %s", path.c_str()).c_str());
	remove(path.c_str());
}

// =========================================
// All visit methods MUST invoke ADD_NODE() at least once!
#define VISIT(_node) void ASTVisualizer::visit(_node* node)

// === Statements ===

VISIT(FuncDeclNode)
{
	string namelabel = tools::fstr("@ %s", node->_identifier.c_str());
	int thisnode = ADD_NODE(namelabel.c_str());

	// ret type	and params
	string infolabel;
	int c = 0;
	if(node->_params.size() > 0) for(auto& p : node->_params)
		{ infolabel += tools::fstr("%%%d ", c) + p._name + "\\n"; c++; }
	infolabel += "~ " + node->_ret_type._name;

	CONNECT_NODES(thisnode, ADD_NODE(infolabel.c_str()));

	// body
	if(node->_body)
	{
		CONNECT_NODES(thisnode, _nodecount);
		node->_body->accept(this);
	}
}

VISIT(VarDeclNode)
{
	string namelabel = tools::fstr("%% %s", node->_identifier.c_str());
	int thisnode = ADD_NODE(namelabel.c_str());
	
	CONNECT_NODES(thisnode, ADD_NODE(node->_type._name.c_str()));

	if(node->_expr)
	{
		CONNECT_NODES(thisnode, _nodecount);
		node->_expr->accept(this);
	}
}

VISIT(AssignNode)
{
	// string lable = "= " + node->_ident;
	int thisnode = ADD_NODE(("= " + node->_ident).c_str());
	CONNECT_NODES(thisnode, _nodecount);
	node->_expr->accept(this);
}

VISIT(IfNode)
{
	int thisnode = ADD_NODE("??");
	CONNECT_NODES(thisnode, _nodecount);
	node->_cond->accept(this);
	CONNECT_NODES(thisnode, _nodecount);
	node->_if->accept(this);
	if(node->_else)
	{
		CONNECT_NODES(thisnode, _nodecount);
		node->_else->accept(this);
	}
}

VISIT(LoopNode)
{
	int thisnode = ADD_NODE("(;;)");;
	int anchor = _nodecount; _nodecount++;
	_stream << tools::fstr("\tnode%d [label=\"\", shape=\"none\", width=0, height=0]\n", anchor);
	CONNECT_NODES(thisnode, anchor);

	if(node->_first)
	{
		CONNECT_NODES_LABELED(anchor, _nodecount, "init");
		node->_first->accept(this);
	}

	CONNECT_NODES_LABELED(anchor, _nodecount, "cond");
	node->_second->accept(this);

	if(node->_third) // must be for-loop
	{
		CONNECT_NODES_LABELED(anchor, _nodecount, "incr");
		node->_third->accept(this);
	}

	CONNECT_NODES(thisnode, _nodecount);
	node->_body->accept(this);
}

VISIT(ReturnNode)
{
	int thisnode = ADD_NODE("~");
	if(node->_expr)
	{
		CONNECT_NODES(thisnode, _nodecount);
		node->_expr->accept(this);
	}
}

VISIT(BlockNode)
{
	int thisnode;
	if(node->_secret)
	{
		_stream << tools::fstr("\tnode%d [label=\"\", shape=\"none\", width=0, height=0]\n", _nodecount);
		thisnode = _nodecount++;
	}
	else thisnode = ADD_NODE("{}");

	for(auto& subnode : node->_statements)
	{
		// node id will be _nodecount
		CONNECT_NODES(thisnode, _nodecount);
		subnode->accept(this);
	}
}

// === Expressions ===

VISIT(LogicalNode)
{
	int thisnode;
	switch(node->_optype)
	{
		case TOKEN_PIPE_PIPE: thisnode = ADD_NODE("||"); break;
		case TOKEN_AND_AND:   thisnode = ADD_NODE("&&"); break;
		case TOKEN_QUESTION:  thisnode = ADD_NODE("?:"); break;
		default: assert(false);
	}

	CONNECT_NODES(thisnode, _nodecount);
	node->_left->accept(this);
	if(node->_middle)
	{
		CONNECT_NODES(thisnode, _nodecount);
		node->_middle->accept(this);
	}
	CONNECT_NODES(thisnode, _nodecount);
	node->_right->accept(this);
}

VISIT(BinaryNode)
{
	int thisnode;
	switch(node->_optype)
	{
		case TOKEN_PIPE: 	  		thisnode = ADD_NODE("|"); break;
		case TOKEN_CARET: 	  		thisnode = ADD_NODE("^"); break;
		case TOKEN_AND:       		thisnode = ADD_NODE("&"); break;

		case TOKEN_EQUAL_EQUAL:		thisnode = ADD_NODE("=="); break;
		case TOKEN_SLASH_EQUAL:		thisnode = ADD_NODE("/="); break;

		case TOKEN_GREATER_EQUAL:	thisnode = ADD_NODE(">="); break;
		case TOKEN_LESS_EQUAL:		thisnode = ADD_NODE("<="); break;
		case TOKEN_GREATER:			thisnode = ADD_NODE(">"); break;
		case TOKEN_LESS:			thisnode = ADD_NODE("<"); break;

		case TOKEN_GREATER_GREATER: thisnode = ADD_NODE(">>"); break;
		case TOKEN_LESS_LESS: 		thisnode = ADD_NODE("<<"); break;

		case TOKEN_PLUS:  			thisnode = ADD_NODE("+"); break;
		case TOKEN_MINUS: 			thisnode = ADD_NODE("-"); break;
		case TOKEN_STAR:  			thisnode = ADD_NODE("*"); break;
		case TOKEN_SLASH:			thisnode = ADD_NODE("/"); break;
		default: assert(false);
	}

	CONNECT_NODES(thisnode, _nodecount);
	node->_left->accept(this);
	CONNECT_NODES(thisnode, _nodecount);
	node->_right->accept(this);
}

VISIT(UnaryNode)
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

VISIT(GroupingNode)
{
	int thisnode = ADD_NODE("()");
	// node id will be _nodecount
	CONNECT_NODES(thisnode, _nodecount);
	node->_expr->accept(this);
}


VISIT(LiteralNode)
{
	ADD_NODE(tools::unescstr(node->_tokenstr, true, false).c_str());
}

VISIT(ReferenceNode)
{
	if(node->_type == TOKEN_VARIABLE_REF)
		ADD_NODE(tools::fstr("$ %s", node->_variable.c_str()).c_str());
	else if(node->_type == TOKEN_PARAMETER_REF)
		ADD_NODE(tools::fstr("$ %d", node->_parameter).c_str());
	else assert(false);
}

VISIT(CallNode)
{
	int thisnode = ADD_NODE(tools::fstr("%s ()", node->_ident.c_str()).c_str());

	for(auto& subnode : node->_arguments)
	{
		// node id will be _nodecount
		CONNECT_NODES(thisnode, _nodecount);
		subnode->accept(this);
	}
}

#undef ADD_NODE
#undef CONNECT_NODES
#undef VISIT