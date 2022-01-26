#include "typechecker.hpp"
#include "common.hpp"

Status TypeChecker::check(string path, const char* source, AST* astree)
{
	_infile = path;
	_error_dispatcher = ErrorDispatcher(source, path.c_str());
	_type_stack = stack<LexicalType>();

	for(auto& node : *astree)
	{
		node->accept(this);
		pop();
	}

	DEBUG_PRINT_MSG("Type check done!");
	return STATUS_SUCCESS;
}

void TypeChecker::error_at(Token *token, string message)
{
	_error_dispatcher.dispatch_error_at(token, "Type Error", message.c_str());

	// print token
	cerr << endl;
	_error_dispatcher.dispatch_token_marked(token);
	cerr << endl;
	
	exit(STATUS_TYPE_ERROR);
}

void TypeChecker::warning_at(Token *token, string message)
{
	// just a lil warnign
	_error_dispatcher.dispatch_warning_at(token, "Type Inference Warning", message.c_str());
}

void TypeChecker::push(LexicalType type)
{
	// just a lil push
	_type_stack.push(type);
}

LexicalType TypeChecker::pop()
{
	assert(!_type_stack.empty());
	LexicalType type = _type_stack.top();
	_type_stack.pop();
	return type;
}

// check if right is compatible with left and return
// "compromise" decided by left
// returns __TYPE_NONE if invalid
LexicalType TypeChecker::resolve_types(LexicalType left, LexicalType right)
{
	switch(left)
	{
		case TYPE_INTEGER: switch(right)
		{
			case TYPE_INTEGER: return TYPE_INTEGER;
			case TYPE_FLOAT: return TYPE_FLOAT;
			case TYPE_CHARACTER: return TYPE_INTEGER;
			default: return __TYPE_NONE;
		}
		case TYPE_FLOAT: switch(right)
		{
			case TYPE_INTEGER: return TYPE_FLOAT;
			case TYPE_FLOAT: return TYPE_FLOAT;
			case TYPE_CHARACTER: return TYPE_FLOAT;
			default: return __TYPE_NONE;
		}
		case TYPE_CHARACTER: switch(right)
		{
			case TYPE_INTEGER: return TYPE_INTEGER;
			case TYPE_FLOAT: return TYPE_FLOAT;
			case TYPE_CHARACTER: return TYPE_CHARACTER;
			default: return __TYPE_NONE;
		}
		case TYPE_STRING: switch(right)
		{
			default: return __TYPE_NONE;
		}
		
		case __TYPE_NONE: return __TYPE_NONE;
	}
}

// =========================================
#define VISIT(_node) void TypeChecker::visit(_node* node)

#define ERROR_AT(token, format, ...) error_at(token, tools::fstr(format, __VA_ARGS__))
#define CANNOT_CONVERT_ERROR_AT(token, ltype, rtype) \
	ERROR_AT(token, "Cannot convert from type " COLOR_BOLD "'%s'" \
	COLOR_NONE " to type " COLOR_BOLD "'%s'" COLOR_NONE ".", \
	GET_LEX_TYPE_STR(rtype), GET_LEX_TYPE_STR(ltype))
#define CONVERSION_WARNING_AT(token, original, result) \
	warning_at(token, tools::fstr("Implicit conversion from type " COLOR_BOLD "'%s'" \
	COLOR_NONE " to type " COLOR_BOLD "'%s'" COLOR_NONE ".", \
	GET_LEX_TYPE_STR(original), GET_LEX_TYPE_STR(result)))

// === Statements ===
// all nodes should have a stack effect of 1

VISIT(FuncDeclNode)
{
	if(node->_body)
	{
		node->_body->accept(this);
		pop();
	}
	
	push(__TYPE_NONE);
}

VISIT(VarDeclNode)
{
	LexicalType vartype = node->_type._lexical_type;

	if(node->_expr)
	{
		node->_expr->accept(this);
		LexicalType exprtype = pop();

		if(resolve_types(vartype, exprtype) != vartype) 
			ERROR_AT(&node->_token, "Cannot initialize variable of type " COLOR_BOLD \
			"'%s'" COLOR_NONE " with expression of type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			GET_LEX_TYPE_STR(vartype), GET_LEX_TYPE_STR(exprtype));
	}
	
	push(__TYPE_NONE);
}

VISIT(AssignNode)
{
	node->_expr->accept(this);
	LexicalType exprtype = pop();
	LexicalType vartype = node->_expected_type;

	if(resolve_types(vartype, exprtype) != vartype) 
		ERROR_AT(&node->_token, "Cannot assign expression of type " COLOR_BOLD \
		"'%s'" COLOR_NONE " to variable with type " COLOR_BOLD "'%s'" COLOR_NONE ".",
		GET_LEX_TYPE_STR(exprtype), GET_LEX_TYPE_STR(vartype));

	push(__TYPE_NONE);
}

VISIT(IfNode)
{
	node->_cond->accept(this);
	pop();

	node->_if->accept(this);
	pop();

	if(node->_else)
	{
		node->_else->accept(this);
		pop();
	}

	push(__TYPE_NONE);
}

VISIT(LoopNode)
{
	assert(false);
}

VISIT(ReturnNode)
{
	node->_expr->accept(this);
	LexicalType exprtype = pop();
	LexicalType functype = node->_expected_type;

	if(resolve_types(functype, exprtype) != functype) 
		ERROR_AT(&node->_token, "Cannot return type " COLOR_BOLD \
		"'%s'" COLOR_NONE " from function with return type " COLOR_BOLD "'%s'" COLOR_NONE ".",
		GET_LEX_TYPE_STR(exprtype), GET_LEX_TYPE_STR(functype));
	
	push(__TYPE_NONE);
}

VISIT(BlockNode)
{
	for(auto& subnode : node->_statements)
	{
		subnode->accept(this);
		pop();
	}
	push(__TYPE_NONE);
}

// === Expressions ===

VISIT(LogicalNode)
{
	// _left && _right
	// _left ? _middle : _right

	ExprNode* leftnode = node->_token.type == TOKEN_QUESTION ? node->_middle : node->_left;

	leftnode->accept(this);
	node->_right->accept(this);
	
	LexicalType right = pop();
	LexicalType left = pop();
	LexicalType result = resolve_types(left, right);

	if(result == __TYPE_NONE) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
	else if(left != result) CONVERSION_WARNING_AT(&node->_left->_token, left, result);
	else if(right != result) CONVERSION_WARNING_AT(&node->_right->_token, right, result);
	push(result);
}

VISIT(BinaryNode)
{
	node->_left->accept(this);
	node->_right->accept(this);

	LexicalType right = pop();
	LexicalType left = pop();
	LexicalType result = resolve_types(left, right);

	if(result == __TYPE_NONE) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
	else if(left != result) CONVERSION_WARNING_AT(&node->_left->_token, left, result);
	else if(right != result) CONVERSION_WARNING_AT(&node->_right->_token, right, result);
	push(result);
}

VISIT(UnaryNode)
{
	node->_expr->accept(this);
	LexicalType type = pop();

	switch(type)
	{
		case TYPE_INTEGER:
			push(TYPE_INTEGER);
			break;

		case TYPE_CHARACTER:
			CONVERSION_WARNING_AT(&node->_token, TYPE_CHARACTER, TYPE_INTEGER);
			push(TYPE_INTEGER);
			break;

		case TYPE_FLOAT: case TYPE_STRING:
			ERROR_AT(&node->_token, "Cannot apply not-operator to expression of type " \
					 COLOR_BOLD "'%s'" COLOR_NONE ".", GET_LEX_TYPE_STR(type));
			break;

		default: assert(false);
	}

	assert(false);
}

VISIT(GroupingNode)
{
	node->_expr->accept(this);
}


VISIT(LiteralNode)
{
	switch(node->_token.type)
	{
		case TOKEN_INTEGER: 	push(TYPE_INTEGER); break;
		case TOKEN_FLOAT: 		push(TYPE_FLOAT); break;
		case TOKEN_CHARACTER: 	push(TYPE_CHARACTER); break;
		case TOKEN_STRING: 		push(TYPE_STRING); break;
		default: assert(false);
	}
}

VISIT(ReferenceNode)
{
	push(node->_type);
}

VISIT(CallNode)
{
	for(int i = 0; i < node->_arguments.size(); i++)
	{
		node->_arguments[i]->accept(this);
		LexicalType exprtype = pop();
		LexicalType argtype = node->_expected_arg_types[i];

		if(resolve_types(argtype, exprtype) != argtype) 
			ERROR_AT(&node->_token, "Argument of type " COLOR_BOLD \
			"'%s'" COLOR_NONE " does not match parameter of type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			GET_LEX_TYPE_STR(exprtype), GET_LEX_TYPE_STR(argtype));
	}

	push(node->_ret_type);
}

#undef VISIT
#undef ERROR_AT
#undef CANNOT_CONVERT_ERROR_AT
#undef CONVERSION_WARNING_AT