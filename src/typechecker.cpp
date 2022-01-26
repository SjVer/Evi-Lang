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
	_error_dispatcher.dispatch_error_at(token, "Type Inference Error", message.c_str());

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
#define CONVERSION_WARNING_AT(token, ltype, rtype) \
	warning_at(token, tools::fstr("Implicit conversion from type " COLOR_BOLD "'%s'" \
	COLOR_NONE " to type " COLOR_BOLD "'%s'" COLOR_NONE ".", \
	GET_LEX_TYPE_STR(rtype), GET_LEX_TYPE_STR(ltype)))

// === Statements ===
// all nodes should have a stack effect of 1

VISIT(FuncDeclNode)
{
	// Push type for ReturnNode to check
	push(node->_ret_type._lexical_type);

	node->_body->accept(this);
	pop();
	
	pop();
	push(__TYPE_NONE);
}

VISIT(VarDeclNode)
{
	LexicalType vartype = node->_type._lexical_type;

	node->_expr->accept(this);
	LexicalType exprtype = pop();

	if(resolve_types(vartype, exprtype) != vartype) 
		ERROR_AT(&node->_token, "Cannot initialize variable of type " COLOR_BOLD \
		"'%s'" COLOR_NONE " with expression of type " COLOR_BOLD "'%s'" COLOR_NONE ".",
		GET_LEX_TYPE_STR(vartype), GET_LEX_TYPE_STR(exprtype));
	
	push(__TYPE_NONE);
}

VISIT(AssignNode)
{
	// TODO
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
	// FuncDeclNode pushed type so we can use that
	LexicalType functype = _type_stack.top();

	node->_expr->accept(this);
	LexicalType exprtype = pop();

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
	push(__TYPE_NONE);
}

VISIT(BinaryNode)
{
	node->_left->accept(this);
	node->_right->accept(this);

	LexicalType right = pop();
	LexicalType left = pop();
	LexicalType result = resolve_types(left, right);

	if(result == __TYPE_NONE) CANNOT_CONVERT_ERROR_AT(&node->_token, left, right);
	else if(right != left) CONVERSION_WARNING_AT(&node->_token, left, right);
	push(result);
}

VISIT(UnaryNode)
{
	push(__TYPE_NONE);
}

VISIT(GroupingNode)
{
	node->_expr->accept(this);
}


VISIT(LiteralNode)
{
	switch(node->_tokentype)
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
	push(__TYPE_NONE);
}

VISIT(CallNode)
{
	push(__TYPE_NONE);
}

#undef VISIT
#undef ERROR_AT
#undef CANNOT_CONVERT_ERROR_AT
#undef CONVERSION_WARNING_AT