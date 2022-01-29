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
// returns TYPE_NONE if invalid
LexicalType TypeChecker::resolve_types(LexicalType left, LexicalType right)
{
	switch(left)
	{
		case TYPE_INTEGER: switch(right)
		{
			case TYPE_INTEGER: return TYPE_INTEGER;
			case TYPE_FLOAT: return TYPE_FLOAT;
			case TYPE_CHARACTER: return TYPE_INTEGER;
			default: return TYPE_NONE;
		}
		case TYPE_FLOAT: switch(right)
		{
			case TYPE_INTEGER: return TYPE_FLOAT;
			case TYPE_FLOAT: return TYPE_FLOAT;
			case TYPE_CHARACTER: return TYPE_FLOAT;
			default: return TYPE_NONE;
		}
		case TYPE_CHARACTER: switch(right)
		{
			case TYPE_INTEGER: return TYPE_INTEGER;
			case TYPE_FLOAT: return TYPE_FLOAT;
			case TYPE_CHARACTER: return TYPE_CHARACTER;
			default: return TYPE_NONE;
		}
		case TYPE_STRING: switch(right)
		{
			case TYPE_STRING: return TYPE_STRING;
			default: return TYPE_NONE;
		}
		case TYPE_VOID: switch(right)
		{
			case TYPE_VOID: return TYPE_VOID;
			default: return TYPE_NONE;
		}
		
		case TYPE_NONE: return TYPE_NONE;
	}
}

bool TypeChecker::can_cast_types(LexicalType from, LexicalType to)
{
	switch(from)
	{
		case TYPE_INTEGER: switch(to)
		{
			case TYPE_INTEGER:
			case TYPE_FLOAT:
			case TYPE_CHARACTER: 
				return true;
			default: return false;
		}
		case TYPE_FLOAT: switch(to)
		{
			case TYPE_INTEGER:
			case TYPE_FLOAT:
			case TYPE_CHARACTER:
				return true;
			default: return false;
		}
		case TYPE_CHARACTER: switch(to)
		{
			case TYPE_INTEGER:
			case TYPE_FLOAT:
			case TYPE_CHARACTER:
				return true;
			default: return false;
		}
		case TYPE_STRING: switch(to)
		{
			case TYPE_STRING:
				return true;
			default: return false;
		}
		case TYPE_VOID: switch(to)
		{
			case TYPE_VOID:
				return true;
			default: return false;
		}
		
		case TYPE_NONE: return false;
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
	
	push(TYPE_NONE);
}

VISIT(VarDeclNode)
{
	LexicalType vartype = node->_type.lexical_type;

	if(node->_expr)
	{
		node->_expr->accept(this);
		LexicalType exprtype = pop();
		LexicalType result = resolve_types(vartype, exprtype);
		
		if(!can_cast_types(result, vartype))
			ERROR_AT(&node->_token, "Cannot initialize variable of type " COLOR_BOLD \
			"'%s'" COLOR_NONE " with expression of type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			GET_LEX_TYPE_STR(vartype), GET_LEX_TYPE_STR(exprtype));

		node->_expr->_cast_to = vartype;
	}
	
	push(TYPE_NONE);
}

VISIT(AssignNode)
{
	node->_expr->accept(this);
	LexicalType exprtype = pop();
	LexicalType vartype = node->_expected_type;

	LexicalType result = resolve_types(vartype, exprtype);

	if(!can_cast_types(result, vartype)) 
		ERROR_AT(&node->_token, "Cannot implicitly convert expression of type " COLOR_BOLD \
		"'%s'" COLOR_NONE " to variable's type " COLOR_BOLD "'%s'" COLOR_NONE ".",
		GET_LEX_TYPE_STR(exprtype), GET_LEX_TYPE_STR(vartype));

	push(TYPE_NONE);
}

VISIT(IfNode)
{
	node->_cond->accept(this);
	pop();

	node->_then->accept(this);
	pop();

	if(node->_else)
	{
		node->_else->accept(this);
		pop();
	}

	push(TYPE_NONE);
}

VISIT(LoopNode)
{
	if(node->_init)
	{
		node->_init->accept(this);
		pop();
	}

	node->_cond->accept(this);
	pop();

	if(node->_incr)
	{
		node->_incr->accept(this);
		pop();
	}

	node->_body->accept(this);
	pop();

	push(TYPE_NONE);
}

VISIT(ReturnNode)
{
	if(node->_expr)
	{
		node->_expr->accept(this);
		LexicalType exprtype = pop();
		LexicalType functype = node->_expected_type;

		LexicalType result = resolve_types(functype, exprtype);

		if(!can_cast_types(result, functype)) 
			ERROR_AT(&node->_token, "Cannot implicitly convert return type " COLOR_BOLD \
			"'%s'" COLOR_NONE " to function's return type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			GET_LEX_TYPE_STR(exprtype), GET_LEX_TYPE_STR(functype));
		
		node->_expr->_cast_to = functype;
	}

	push(TYPE_NONE);
}

VISIT(BlockNode)
{
	for(auto& subnode : node->_statements)
	{
		subnode->accept(this);
		pop();
	}
	push(TYPE_NONE);
}

// === Expressions ===

VISIT(LogicalNode)
{
	// _left && _right
	// _left ? _middle : _right

	if(node->_token.type == TOKEN_QUESTION)
	{
		node->_middle->accept(this);
		node->_right->accept(this);
		
		LexicalType right = pop();
		LexicalType left = pop();
		LexicalType result = resolve_types(left, right);

		if(result == TYPE_NONE) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
		else if(left != result) CONVERSION_WARNING_AT(&node->_left->_token, left, result);
		else if(right != result) CONVERSION_WARNING_AT(&node->_right->_token, right, result);
		
		node->_left->_cast_to = result;
		node->_right->_cast_to = result;

		push(result);
	}
	else
	{
		push(TYPE_INTEGER);
	}
}

VISIT(BinaryNode)
{
	node->_left->accept(this);
	node->_right->accept(this);

	LexicalType right = pop();
	LexicalType left = pop();
	LexicalType result = resolve_types(left, right);

	if(result == TYPE_NONE) 
	{
		if(right != left) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
		else ERROR_AT(&node->_token, "Cannot peform binary operation on expressions of type " \
			 COLOR_BOLD "'%s'" COLOR_NONE ".", GET_LEX_TYPE_STR(right));
	}
	else if(left != result) CONVERSION_WARNING_AT(&node->_left->_token, left, result);
	else if(right != result) CONVERSION_WARNING_AT(&node->_right->_token, right, result);
	
	node->_left->_cast_to = result;
	node->_right->_cast_to = result;
	node->_cast_to = result; // in case nothing else sets it
	push(result);
}

VISIT(UnaryNode)
{
	node->_expr->accept(this);
	LexicalType type = pop();

	switch(type)
	{
		case TYPE_INTEGER:
			node->_expr->_cast_to = TYPE_INTEGER;
			push(TYPE_INTEGER);
			break;

		case TYPE_CHARACTER:
			if(node->_optype == TOKEN_BANG) CONVERSION_WARNING_AT(&node->_token, TYPE_CHARACTER, TYPE_INTEGER);
			node->_expr->_cast_to = node->_optype == TOKEN_BANG ? TYPE_INTEGER : TYPE_CHARACTER;
			push(node->_optype == TOKEN_BANG ? TYPE_INTEGER : TYPE_CHARACTER);
			break;

		case TYPE_FLOAT:
			if(node->_optype != TOKEN_BANG)
			{
				node->_expr->_cast_to = TYPE_FLOAT;
				push(TYPE_FLOAT);
				break;
			}

		case TYPE_STRING:
			ERROR_AT(&node->_token, "Cannot apply '%.*s'-operator to expression of type " \
					 COLOR_BOLD "'%s'" COLOR_NONE ".", getTokenStr(node->_optype), GET_LEX_TYPE_STR(type));
			break;

		default: assert(false);
	}
}

VISIT(GroupingNode)
{
	node->_expr->accept(this);
}


VISIT(LiteralNode)
{
	switch(node->_token.type)
	{
		case TOKEN_INTEGER: 	node->_cast_to = TYPE_INTEGER;   push(TYPE_INTEGER); break;
		case TOKEN_FLOAT: 		node->_cast_to = TYPE_FLOAT; 	 push(TYPE_FLOAT); break;
		case TOKEN_CHARACTER: 	node->_cast_to = TYPE_CHARACTER; push(TYPE_CHARACTER); break;
		case TOKEN_STRING: 		node->_cast_to = TYPE_STRING; 	 push(TYPE_STRING); break;
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

		LexicalType result = resolve_types(argtype, exprtype);
		node->_arguments[i]->_cast_to = argtype;

		if(!can_cast_types(result, argtype)) 
			ERROR_AT(&node->_token, "Cannot implicitly convert argument of type " COLOR_BOLD \
			"'%s'" COLOR_NONE " to parameter's type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			GET_LEX_TYPE_STR(exprtype), GET_LEX_TYPE_STR(argtype));
	}

	push(node->_ret_type);
}

#undef VISIT
#undef ERROR_AT
#undef CANNOT_CONVERT_ERROR_AT
#undef CONVERSION_WARNING_AT