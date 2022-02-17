#include "typechecker.hpp"
#include "common.hpp"

Status TypeChecker::check(string path, const char* source, AST* astree)
{
	_infile = path;
	_error_dispatcher = ErrorDispatcher(source, path.c_str());
	_type_stack = stack<ParsedType*>();

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

void TypeChecker::push(ParsedType* type)
{
	// just a lil push
	_type_stack.push(type);
}

ParsedType* TypeChecker::pop()
{
	assert(!_type_stack.empty());
	ParsedType* type = _type_stack.top();
	_type_stack.pop();
	return type;
}

// check if right is compatible with left and return
// "compromise" decided by left
// returns a nullptr if invalid
ParsedType* TypeChecker::resolve_types(ParsedType* left, ParsedType* right)
{
	// DEBUG_PRINT_F_MSG("resolve_types(%s, %s) (%s)", left->to_c_string(), right->to_c_string(),
	// 												left->eq(right) ? "same" : "different");

	if(left->eq(right)) return left->copy();

	if(left->_pointer_depth == right->_pointer_depth)
	{
		if(left->_lexical_type == right->_lexical_type) return left->copy();

		// just check their types
		switch(AS_LEX(left))
		{
			case TYPE_BOOL: switch(AS_LEX(right))
			{
				case TYPE_CHARACTER:
				case TYPE_INTEGER:
					return left->copy_change_lex(TYPE_INTEGER);
				case TYPE_FLOAT:
					return left->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
			}
			case TYPE_CHARACTER: switch(AS_LEX(right))
			{
				case TYPE_BOOL:
				case TYPE_INTEGER:
					return left->copy_change_lex(TYPE_INTEGER);
				case TYPE_FLOAT:
					return left->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
				
			}
			case TYPE_INTEGER: switch(AS_LEX(right))
			{
				case TYPE_BOOL:
				case TYPE_CHARACTER:
					return left->copy_change_lex(TYPE_INTEGER);
				case TYPE_FLOAT:
					return left->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
				
			}
			case TYPE_FLOAT: switch(AS_LEX(right))
			{
				case TYPE_BOOL:
				case TYPE_INTEGER:
				case TYPE_CHARACTER:
					return left->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
			}
			default: return nullptr;
		}
	}
	else if(left->_pointer_depth < right->_pointer_depth)
	{
		// e.g. "int* a = int** b" is basically "int a = int* b"
		// so relative to left, right is a pointer
		switch (left->_lexical_type)
		{
			case TYPE_BOOL:
			case TYPE_INTEGER:
			case TYPE_CHARACTER:
			case TYPE_FLOAT:
				return left->copy_change_lex(TYPE_INTEGER);

			default: return nullptr;
		}
	}
	else if(left->_pointer_depth > right->_pointer_depth)
	{
		// e.g. "int** a = int* b" is basically "int* a = int b"
		// so relative to right, left is a pointer
		switch (right->_lexical_type)
		{
			case TYPE_BOOL:
			case TYPE_INTEGER:
			case TYPE_CHARACTER:
			case TYPE_FLOAT:
				return right->copy_change_lex(TYPE_INTEGER);

			default: return nullptr;
		}
	}

	assert(false);
}

bool TypeChecker::can_cast_types(ParsedType* from, ParsedType* to)
{
	if(!from) return false;
	if(from->eq(to)) return true;

	if(from->_pointer_depth == to->_pointer_depth)
	{
		// just check their types
		switch(AS_LEX(from))
		{
			case TYPE_BOOL: switch(AS_LEX(to))
			{
				case TYPE_CHARACTER:
				case TYPE_INTEGER:
				case TYPE_FLOAT:
					return true;
				default: return false;
			}
			case TYPE_CHARACTER: switch(AS_LEX(to))
			{
				case TYPE_BOOL:
				case TYPE_INTEGER:
				case TYPE_FLOAT:
					return true;
				default: return false;
				
			}
			case TYPE_INTEGER: switch(AS_LEX(to))
			{
				case TYPE_BOOL:
				case TYPE_CHARACTER:
				case TYPE_FLOAT:
					return true;
				default: return false;
				
			}
			case TYPE_FLOAT: switch(AS_LEX(to))
			{
				case TYPE_BOOL:
				case TYPE_INTEGER:
				case TYPE_CHARACTER:
					return true;
				default: return false;
			}
			default: return false;
		}
	}
	else if(from->_pointer_depth < to->_pointer_depth)
	{
		// to = ptr relative to from
		switch(from->_lexical_type)
		{
			case TYPE_BOOL:
			case TYPE_CHARACTER:
			case TYPE_INTEGER:
				return true;
			default: return false;
		}
	}

	assert(false);
}

// =========================================
#define VISIT(_node) void TypeChecker::visit(_node* node)

#define ERROR_AT(token, format, ...) error_at(token, tools::fstr(format, __VA_ARGS__))
#define CANNOT_CONVERT_ERROR_AT(token, ltype, rtype) \
	ERROR_AT(token, "Cannot convert from type " COLOR_BOLD "'%s'" \
	COLOR_NONE " to type " COLOR_BOLD "'%s'" COLOR_NONE ".", \
	rtype->to_c_string(), ltype->to_c_string())
#define CONVERSION_WARNING_AT(token, original, result) \
	warning_at(token, tools::fstr("Implicit conversion from type " COLOR_BOLD "'%s'" \
	COLOR_NONE " to type " COLOR_BOLD "'%s'" COLOR_NONE ".", \
	original->to_c_string(), result->to_c_string()))

// === Statements ===
// all nodes should have a stack effect of 1

VISIT(FuncDeclNode)
{
	if(node->_body)
	{
		node->_body->accept(this);
		pop();
	}
	
	push(nullptr);
}

VISIT(VarDeclNode)
{
	ParsedType* vartype = node->_type;

	if(node->_expr)
	{
		node->_expr->accept(this);
		ParsedType* exprtype = pop();
		ParsedType* result = resolve_types(vartype, exprtype);

		if(!can_cast_types(result ? result : exprtype, vartype))
			ERROR_AT(&node->_token, "Cannot initialize variable of type " COLOR_BOLD \
			"'%s'" COLOR_NONE " with expression of type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			vartype->to_c_string(), exprtype->to_c_string());

		node->_expr->_cast_to = vartype;
	}
	
	push(nullptr);
}

VISIT(AssignNode)
{
	node->_expr->accept(this);
	ParsedType* exprtype = pop();
	ParsedType* vartype = node->_expected_type;

	ParsedType* result = resolve_types(vartype, exprtype);

	if(!can_cast_types(result, vartype)) 
		ERROR_AT(&node->_token, "Cannot implicitly convert expression of type " COLOR_BOLD \
		"'%s'" COLOR_NONE " to variable's type " COLOR_BOLD "'%s'" COLOR_NONE ".",
		exprtype->to_c_string(), vartype->to_c_string());

	push(nullptr);
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

	push(nullptr);
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

	push(nullptr);
}

VISIT(ReturnNode)
{
	if(node->_expr)
	{
		node->_expr->accept(this);
		ParsedType* exprtype = pop();
		ParsedType* functype = node->_expected_type;

		ParsedType* result = resolve_types(functype, exprtype);

		if(!can_cast_types(result, functype)) 
			ERROR_AT(&node->_token, "Cannot implicitly convert return type " COLOR_BOLD \
			"'%s'" COLOR_NONE " to function's return type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			exprtype->to_c_string(), functype->to_c_string());
		
		node->_expr->_cast_to = functype;
	}

	push(nullptr);
}

VISIT(BlockNode)
{
	for(auto& subnode : node->_statements)
	{
		subnode->accept(this);
		pop();
	}
	push(nullptr);
}

// === Expressions ===

VISIT(LogicalNode)
{
	// _left && _right
	// _left ? _middle : _right

	ParsedType* booltype = PTYPE(TYPE_BOOL);

	if(node->_token.type == TOKEN_QUESTION)
	{
		node->_left->accept(this);
		ParsedType* cond = pop();
		if(!can_cast_types(cond, booltype))	CANNOT_CONVERT_ERROR_AT(&node->_left->_token, cond, booltype);
		node->_left->_cast_to = booltype;

		node->_middle->accept(this);
		node->_right->accept(this);
		ParsedType* right = pop();
		ParsedType* left = pop();
		ParsedType* result = resolve_types(left, right);

		if(result == nullptr) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
		else if(left != result) CONVERSION_WARNING_AT(&node->_middle->_token, left, result);
		else if(right != result) CONVERSION_WARNING_AT(&node->_right->_token, right, result);
		
		node->_middle->_cast_to = result;
		node->_right->_cast_to = result;

		push(result);
	}
	else
	{
		node->_left->accept(this);
		node->_right->accept(this);
		
		ParsedType* right = pop();
		ParsedType* left = pop();

		// if(result == TYPE_NONE) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
		if(!can_cast_types(left, booltype)) CANNOT_CONVERT_ERROR_AT(&node->_left->_token, left, booltype);
		if(!can_cast_types(right, booltype)) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, right, booltype);

		if(left != booltype) CONVERSION_WARNING_AT(&node->_left->_token, left, booltype);
		if(right != booltype) CONVERSION_WARNING_AT(&node->_right->_token, right, booltype);
		
		node->_left->_cast_to = booltype;
		node->_right->_cast_to = booltype;

		push(booltype);
	}
}

VISIT(BinaryNode)
{
	node->_left->accept(this);
	node->_right->accept(this);

	ParsedType* right = pop();
	ParsedType* left = pop();

	if(node->_optype == TOKEN_AND
	|| node->_optype == TOKEN_PIPE
	|| node->_optype == TOKEN_CARET) // binary op requires ints
	{
		ParsedType* inttype = PTYPE(TYPE_INTEGER);

		if(!can_cast_types(left, inttype))
		{
			ERROR_AT(&node->_left->_token, "Cannot implicitly convert expression of type " COLOR_BOLD "'%s'" COLOR_NONE
			" to type " COLOR_BOLD "'integer'" COLOR_NONE " required by binary operator '%.*s'.",
			left->to_c_string(), node->_token.length, node->_token.start);
		}
		if(!can_cast_types(right, inttype))
		{
			ERROR_AT(&node->_right->_token, "Cannot implicitly convert expression of type " COLOR_BOLD "'%s'" COLOR_NONE
			" to type " COLOR_BOLD "'integer'" COLOR_NONE " required by binary operator '%.*s'.",
			right->to_c_string(), node->_token.length, node->_token.start);
		}
		if(AS_LEX(left) != TYPE_INTEGER) CONVERSION_WARNING_AT(&node->_left->_token, left, inttype);
		if(AS_LEX(right) != TYPE_INTEGER) CONVERSION_WARNING_AT(&node->_right->_token, right, inttype);

		node->_left->_cast_to = inttype;
		node->_right->_cast_to = inttype;
		node->_cast_to = inttype; // in case nothing else sets it
		push(inttype);
	}
	else
	{
		ParsedType* result = resolve_types(left, right);

		if(result == nullptr) 
		{
			if(right != left) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
			else ERROR_AT(&node->_token, "Cannot peform binary operation on expressions of type " \
				COLOR_BOLD "'%s'" COLOR_NONE ".", right->to_c_string());
		}
		if(!left->eq(result)) CONVERSION_WARNING_AT(&node->_left->_token, left, result);
		if(!right->eq(result)) CONVERSION_WARNING_AT(&node->_right->_token, right, result);
		
		node->_left->_cast_to = result;
		node->_right->_cast_to = result;
		node->_cast_to = result; // in case nothing else sets it
		push(result);
	}
}

VISIT(UnaryNode)
{
	node->_expr->accept(this);
	ParsedType* type = pop();

	if(type->_pointer_depth == 0) switch(node->_optype)
	{
		case TOKEN_STAR:
		{
			// can only dereference if pointer depth > 0
			error_at(&node->_token, "Cannot dereference non-pointer value.");
			break;
		}
		case TOKEN_AND:
		{
			// can only get address of "lvalue"
			if(!type->_is_reference)
			{
				error_at(&node->_token, "Cannot get address of non-reference value.");
				break;
			}

			node->_expr->_cast_to = type->copy_inc_ptr_depth();
			node->_expr->_cast_to->_keep_as_reference = true;
			push(node->_expr->_cast_to);
			break;
		}
		case TOKEN_BANG:
		{
			node->_expr->_cast_to = PTYPE(TYPE_BOOL);
			push(node->_expr->_cast_to);
			break;
		}

		case TOKEN_MINUS:
		case TOKEN_PLUS_PLUS:
		case TOKEN_MINUS_MINUS:
		{
			node->_expr->_cast_to = type;
			push(type);
			break;
		}

		default: assert(false);
	}
	else switch(node->_optype) // we're dealing with a pointer
	{
		case TOKEN_STAR:
		{
			node->_expr->_cast_to = type->copy_dec_ptr_depth();
			node->_expr->_cast_to->_keep_as_reference = true;
			push(node->_expr->_cast_to);
			break;
		}
		case TOKEN_AND:
		{
			node->_expr->_cast_to = type->copy_inc_ptr_depth();
			node->_expr->_cast_to->_keep_as_reference = true;
			push(node->_expr->_cast_to);
			break;
		}
		case TOKEN_BANG:
		{
			node->_expr->_cast_to = PTYPE(TYPE_BOOL);
			push(node->_expr->_cast_to);
			break;
		}

		case TOKEN_MINUS:
		case TOKEN_PLUS_PLUS:
		case TOKEN_MINUS_MINUS:
		{
			// pointer arithmetic :(
			node->_expr->_cast_to = PTYPE(TYPE_BOOL);
			push(node->_expr->_cast_to);
			CONVERSION_WARNING_AT(&node->_token, type, node->_expr->_cast_to);
			break;
		}

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
		case TOKEN_INTEGER: 	node->_cast_to = PTYPE(TYPE_INTEGER);      push(node->_cast_to); break;
		case TOKEN_FLOAT: 		node->_cast_to = PTYPE(TYPE_FLOAT); 	   push(node->_cast_to); break;
		case TOKEN_CHARACTER: 	node->_cast_to = PTYPE(TYPE_CHARACTER);    push(node->_cast_to); break;
		case TOKEN_STRING: 		node->_cast_to = new ParsedType(TYPE_CHARACTER, nullptr, 1); push(node->_cast_to); break;
		default: assert(false);
	}
}

VISIT(ArrayNode)
{
	// get first expr
	ParsedType* firsttype = nullptr;
	if(node->_elements.size() >= 1)
	{
		node->_elements[0]->accept(this);
		firsttype = pop();
	}

	for(int i = 1; i < node->_elements.size(); i++)
	{
		node->_elements[i]->accept(this);
		ParsedType* exprtype = pop();

		ParsedType* result = resolve_types(firsttype, exprtype);
		node->_elements[i]->_cast_to = firsttype;

		if(!can_cast_types(result ? result : exprtype, firsttype))
			ERROR_AT(&node->_elements[i]->_token, "Cannot implicitly convert argument of type" COLOR_BOLD \
			"'%s'" COLOR_NONE " to first element's type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			exprtype->to_c_string(), firsttype->to_c_string());
	}

	ParsedType* arrtype = firsttype->copy();
	arrtype->_array_sizes.push_back(node->_elements.size());
	push(arrtype);
}

VISIT(ReferenceNode)
{
	node->_cast_to = node->_type;
	push(node->_type);
}

VISIT(CallNode)
{
	for(int i = 0; i < node->_arguments.size(); i++)
	{
		node->_arguments[i]->accept(this);
		ParsedType* exprtype = pop();
		ParsedType* argtype = node->_expected_arg_types[i];

		ParsedType* result = resolve_types(argtype, exprtype);
		node->_arguments[i]->_cast_to = argtype;

		if(!can_cast_types(result ? result : exprtype, argtype)) 
			ERROR_AT(&node->_arguments[i]->_token, "Cannot implicitly convert argument of type " COLOR_BOLD \
			"'%s'" COLOR_NONE " to parameter's type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			exprtype->to_c_string(), argtype->to_c_string());
	}

	push(node->_ret_type);
}

#undef VISIT
#undef ERROR_AT
#undef CANNOT_CONVERT_ERROR_AT
#undef CONVERSION_WARNING_AT