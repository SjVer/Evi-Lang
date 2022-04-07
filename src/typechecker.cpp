#include "typechecker.hpp"
#include "common.hpp"

Status TypeChecker::check(string path, ccp source, AST* astree)
{
	_infile = path;
	_error_dispatcher = ErrorDispatcher();
	_type_stack = stack<ParsedType*>();

	_panic_mode = false;
	_had_error = false;

	for(auto& node : *astree)
	{
		if(!node) continue;
		node->accept(this);
		pop();
		_panic_mode = false;
	}

	DEBUG_PRINT_MSG("Type check done!");
	return _had_error ? STATUS_TYPE_ERROR : STATUS_SUCCESS;
}

void TypeChecker::error_at(Token *token, string message)
{
	if(_panic_mode) return;

	_panic_mode = true;
	_had_error = true;

	if(lint_args.type == LINT_GET_DIAGNOSTICS)
	{
		lint_output_diagnostic_object(token, message, "error");
		
		// TODO:? array and surrounding object ended in synchronize()
		lint_output_diagnostic_object_end();
	}
	else if(lint_args.type == LINT_NONE)
	{
		_error_dispatcher.error_at_token(token, "Type Inference Error", message.c_str());

		// print token
		cerr << endl;
		_error_dispatcher.print_token_marked(token, COLOR_RED);
		
		ABORT(STATUS_TYPE_ERROR);
		// synchronize?
	}
}

void TypeChecker::warning_at(Token *token, string message, bool print_token)
{
	if(_panic_mode) return;
	else if(lint_args.type == LINT_GET_DIAGNOSTICS)
	{
		lint_output_diagnostic_object(token, message, "warning");

		// TODO:? array and surrounding object ended in synchronize()
		lint_output_diagnostic_object_end();
	}
	else if(lint_args.type == LINT_NONE)
	{
		_error_dispatcher.warning_at_token(token, "Type Inference Warning", message.c_str());
		if(print_token)
		{
			cerr << endl;
			_error_dispatcher.print_token_marked(token, COLOR_PURPLE);
		}
	}
}

void TypeChecker::push(ParsedType* type)
{
	// just a lil push
	_type_stack.push(type);
}

ParsedType* TypeChecker::pop()
{
	ASSERT_OR_THROW_INTERNAL_ERROR(!_type_stack.empty(), "during type checking");
	ParsedType* type = _type_stack.top();
	_type_stack.pop();
	return type;
}

// check if right is compatible with from and return
// "compromise" decided by original
// returns a nullptr if invalid
ParsedType* TypeChecker::resolve_types(ParsedType* original, ParsedType* adapted)
{
	// DEBUG_PRINT_F_MSG("resolve_types(%s, %s) (%s)", original->to_c_string(), adapted->to_c_string(),
	// 												original->eq(adapted) ? "same" : "different");

	if(original->eq(adapted)) return original->copy();

	if(original->get_depth() == adapted->get_depth())
	{
		if(original->_lexical_type == adapted->_lexical_type) return original->copy();

		// just check their types
		switch(AS_LEX(original))
		{
			case TYPE_BOOL: switch(AS_LEX(adapted))
			{
				case TYPE_CHARACTER:
				case TYPE_INTEGER:
					return original->copy_change_lex(TYPE_INTEGER);
				case TYPE_FLOAT:
					return original->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
			}
			case TYPE_CHARACTER: switch(AS_LEX(adapted))
			{
				case TYPE_BOOL:
				case TYPE_INTEGER:
					return original->copy_change_lex(TYPE_INTEGER);
				case TYPE_FLOAT:
					return original->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
				
			}
			case TYPE_INTEGER: switch(AS_LEX(adapted))
			{
				case TYPE_BOOL:
				case TYPE_CHARACTER:
					return original->copy_change_lex(TYPE_INTEGER);
				case TYPE_FLOAT:
					return original->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
				
			}
			case TYPE_FLOAT: switch(AS_LEX(adapted))
			{
				case TYPE_BOOL:
				case TYPE_INTEGER:
				case TYPE_CHARACTER:
					return original->copy_change_lex(TYPE_FLOAT);
				default: return nullptr;
			}
			default: return nullptr;
		}
	}
	else if(original->is_pointer())
	{
		// both ptrs, compare elements
		if(adapted->is_pointer()) return resolve_types(
			original->copy_element_of(), adapted->copy_element_of())->copy_pointer_to();

		// original = ptr, adapted = value
		switch (adapted->_lexical_type)
		{
			case TYPE_BOOL:
			case TYPE_INTEGER:
			case TYPE_CHARACTER:
				return original->copy_change_lex(TYPE_INTEGER);

			default: return nullptr;
		}

	 	THROW_INTERNAL_ERROR("during type checking");
	}
	else if(adapted->is_pointer())
	{
		// e.g. "int** a = int* b" is basically "int* a = int b"
		// so relative to right, left is a pointer
		switch (adapted->_lexical_type)
		{
			case TYPE_BOOL:
			case TYPE_INTEGER:
			case TYPE_CHARACTER:
			case TYPE_FLOAT:
				return adapted->copy_change_lex(TYPE_INTEGER);

			default: return nullptr;
		}
	}

 	THROW_INTERNAL_ERROR("during type checking");
	return nullptr;
}

bool TypeChecker::can_cast_types(ParsedType* from, ParsedType* to)
{
	// DEBUG_PRINT_F_MSG("can_cast_types(%s, %s) (%s)", from->to_c_string(), to->to_c_string(),
	// 												 from->eq(to) ? "same" : "different");

	if(!from || from->is_invalid() || !to || to->is_invalid()) return false;
	else if(from->eq(to)) return true;

	else if(from->get_depth() == to->get_depth())
	{
		// just check their types
		if(AS_LEX(to) == AS_LEX(from) || AS_LEX(to) == TYPE_VOID) return true;
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
			case TYPE_VOID:
				// nll* -> idk* is ok
				return from->get_depth();
			default: return false;
		}
	}
	else if(from->is_pointer())
	{
		// pointer/array -> plain type

		switch(to->_lexical_type)
		{
			case TYPE_BOOL:
			case TYPE_CHARACTER:
			case TYPE_INTEGER:
			case TYPE_FLOAT:
			case TYPE_VOID:
				return true;
			default:
				return false;
		}
	}
	else if(to->get_depth())
	{
		// plain type -> pointer/array

		switch(from->_lexical_type)
		{
			case TYPE_BOOL:
			case TYPE_CHARACTER:
			case TYPE_INTEGER:
				return true;
			default:
				return false;
		}
	}

 	THROW_INTERNAL_ERROR("during type checking");
	return false;
}

// =========================================
#define VISIT(_node) void TypeChecker::visit(_node* node)
#define ACCEPT_AND_POP(node) { if(node) { node->accept(this); pop(); } }

#define ERROR_AT(token, format, ...) error_at(token, tools::fstr(format, __VA_ARGS__))
#define CANNOT_CONVERT_ERROR_AT(token, from, to) \
	ERROR_AT(token, "Cannot convert from type " COLOR_BOLD "'%s'" \
	COLOR_NONE " to type " COLOR_BOLD "'%s'" COLOR_NONE ".", \
	from->to_c_string(), to->to_c_string())
#define CONVERSION_WARNING_AT(token, original, result) \
	warning_at(token, tools::fstr("Implicit conversion from type " COLOR_BOLD "'%s'" \
	COLOR_NONE " to type " COLOR_BOLD "'%s'" COLOR_NONE ".", \
	original->to_c_string(), result->to_c_string()), true)

// === Statements ===
// all nodes should have a stack effect of 1

VISIT(FuncDeclNode)
{
	ACCEPT_AND_POP(node->_body)
	_panic_mode = false;
	
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
		else if(!exprtype->eq(vartype, true))
			CONVERSION_WARNING_AT(&node->_token, exprtype, vartype);			

		node->_expr->_cast_to = vartype;
	}
	
	push(nullptr);
}

VISIT(AssignNode)
{
	node->_expr->accept(this);
	ParsedType* exprtype = pop();
	ParsedType* vartype = node->_expected_type;

	// handle subscript
	for(auto& subscript : node->_subscripts)
	{
		if(!vartype->is_pointer())
		{
			ERROR_AT(&subscript->_token, "Subscripted target is not a pointer.", 0);
			return;
		}
		else vartype = vartype->copy_element_of();
	}

	ParsedType* result = resolve_types(vartype, exprtype);

	if(!can_cast_types(result, vartype)) 
		ERROR_AT(&node->_token, "Cannot implicitly convert expression of type " COLOR_BOLD \
		"'%s'" COLOR_NONE " to target's type " COLOR_BOLD "'%s'" COLOR_NONE ".",
		exprtype->to_c_string(), vartype->to_c_string());
	else if(!exprtype->eq(vartype, true))
		CONVERSION_WARNING_AT(&node->_token, exprtype, vartype);

	push(nullptr);
}

VISIT(IfNode)
{
	ACCEPT_AND_POP(node->_cond);
	_panic_mode = false;

	ACCEPT_AND_POP(node->_then);
	_panic_mode = false;

	ACCEPT_AND_POP(node->_else);
	_panic_mode = false;

	push(nullptr);
}

VISIT(LoopNode)
{
	ACCEPT_AND_POP(node->_init);
	_panic_mode = false;

	ACCEPT_AND_POP(node->_cond);
	_panic_mode = false;

	ACCEPT_AND_POP(node->_incr);
	_panic_mode = false;

	ACCEPT_AND_POP(node->_body);
	_panic_mode = false;

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
		ACCEPT_AND_POP(subnode);
		_panic_mode = false;
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
		else if(!left->eq(result)) CONVERSION_WARNING_AT(&node->_middle->_token, left, result);
		else if(!right->eq(result)) CONVERSION_WARNING_AT(&node->_right->_token, right, result);
		
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

		if(!left->eq(booltype, true)) CONVERSION_WARNING_AT(&node->_left->_token, left, booltype);
		if(!right->eq(booltype, true)) CONVERSION_WARNING_AT(&node->_right->_token, right, booltype);
		
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
	|| node->_optype == TOKEN_CARET) // bitwise op (requires ints)
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
		if(!left->eq(inttype, true)) CONVERSION_WARNING_AT(&node->_left->_token, left, inttype);
		if(!right->eq(inttype, true)) CONVERSION_WARNING_AT(&node->_right->_token, right, inttype);

		node->_left->_cast_to = inttype;
		node->_right->_cast_to = inttype;
		node->_cast_to = inttype; // in case nothing else sets it

		push(inttype);
		return;
	}
	if(node->_optype == TOKEN_EQUAL_EQUAL
	|| node->_optype == TOKEN_SLASH_EQUAL
	|| node->_optype == TOKEN_GREATER
	|| node->_optype == TOKEN_GREATER_EQUAL
	|| node->_optype == TOKEN_LESS
	|| node->_optype == TOKEN_LESS_EQUAL) // inqualty op (pushes bool)
	{
		ParsedType* booltype = PTYPE(TYPE_BOOL);
		if(!can_cast_types(left, booltype)) CANNOT_CONVERT_ERROR_AT(&node->_left->_token, left, booltype);
		if(!can_cast_types(right, booltype)) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, right, booltype);

		node->_left->_cast_to = booltype;
		node->_right->_cast_to = booltype;
		node->_cast_to = booltype;

		push(booltype);
		return;
	}
	else // normal op
	{
		// handle pointer arithmatic properly
		if(right->is_pointer() && (
			node->_optype == TOKEN_PLUS  ||
			node->_optype == TOKEN_MINUS ||
			node->_optype == TOKEN_STAR  ||
			node->_optype == TOKEN_SLASH ))
			// CONVERSION_WARNING_AT(&node->_right->_token, right, PTYPE(TYPE_INTEGER));
			right = PTYPE(TYPE_INTEGER);
		
		if(left->is_pointer() && (
			node->_optype == TOKEN_PLUS  ||
			node->_optype == TOKEN_MINUS ||
			node->_optype == TOKEN_STAR  ||
			node->_optype == TOKEN_SLASH ))
			// CONVERSION_WARNING_AT(&node->_left->_token, left, PTYPE(TYPE_INTEGER));
			left = PTYPE(TYPE_INTEGER);

		ParsedType* result = resolve_types(left, right);

		if(result == nullptr) 
		{
			if(!right->eq(left)) CANNOT_CONVERT_ERROR_AT(&node->_right->_token, left, right);
			else ERROR_AT(&node->_token, "Cannot peform binary operation on expressions of type " \
				COLOR_BOLD "'%s'" COLOR_NONE ".", right->to_c_string());
		}
		if(!left->eq(result, true)) CONVERSION_WARNING_AT(&node->_left->_token, left, result);
		if(!right->eq(result, true)) CONVERSION_WARNING_AT(&node->_right->_token, right, result);

		node->_left->_cast_to = result;
		node->_right->_cast_to = result;
		node->_cast_to = result; // in case nothing else sets it

		push(result);
		return;
	}
}

VISIT(CastNode)
{
	node->_expr->accept(this);
	ParsedType* srctype = pop();
	ParsedType* desttype = node->_type;

	if(!can_cast_types(srctype, desttype))
		CANNOT_CONVERT_ERROR_AT(&node->_token, srctype, desttype);
	
	push(node->_type);
}

VISIT(UnaryNode)
{
	node->_expr->accept(this);
	ParsedType* type = pop();

	if(type->is_pointer()) switch(node->_optype) // we're dealing with a pointer
	{
		case TOKEN_STAR:
		{
			node->_expr->_cast_to = type->copy_element_of();
			node->_expr->_cast_to->_keep_as_reference = true;
			push(node->_expr->_cast_to);
			break;
		}
		case TOKEN_AND:
		{
			node->_expr->_cast_to = type->copy_pointer_to();
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
			node->_expr->_cast_to = PTYPE(TYPE_INTEGER);
			push(node->_expr->_cast_to);
			CONVERSION_WARNING_AT(&node->_token, type, node->_expr->_cast_to);
			break;
		}

		default: THROW_INTERNAL_ERROR("during type checking");
	}
	else switch(node->_optype) // not a pointer
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
			if(type->is_constant())
			{
				// taking the address of a constant is bad practice
				warning_at(&node->_token, tools::fstr(
					"Unary '&' operator discards constant-modifier from target type '%s'.", type->to_c_string()), true);
			}

			node->_expr->_cast_to = type->copy_pointer_to();
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

		default: THROW_INTERNAL_ERROR("during type checking");
	}
}

VISIT(GroupingNode)
{
	node->_expr->accept(this);
}

VISIT(SubscriptNode)
{
	node->_expr->accept(this);
	ParsedType* exprtype = pop();

	// lhs must be pointer or array
	if(!exprtype->get_depth())
	{
		ERROR_AT(&node->_token, "Subscripted value is not a pointer.", 0);
		push(ParsedType::new_invalid());
		return;
	}

	node->_index->accept(this);
	ParsedType* indextype = pop();

	// try to cast rhs to int
	if(!can_cast_types(indextype, PTYPE(TYPE_INTEGER)))
		ERROR_AT(&node->_token, "Cannot convert from type " COLOR_BOLD "'%s'" \
				COLOR_NONE " to integer type for subscript.", indextype->to_c_string());
	
	push(exprtype->copy_element_of());
}


VISIT(LiteralNode)
{
	switch(node->_token.type)
	{
		case TOKEN_INTEGER: 	node->_cast_to = PTYPE(TYPE_INTEGER);      push(node->_cast_to); break;
		case TOKEN_FLOAT: 		node->_cast_to = PTYPE(TYPE_FLOAT); 	   push(node->_cast_to); break;
		case TOKEN_CHARACTER: 	node->_cast_to = PTYPE(TYPE_CHARACTER);    push(node->_cast_to); break;
		case TOKEN_STRING: 		
			node->_cast_to = PTYPE(TYPE_CHARACTER)->copy_pointer_to(); 
			push(node->_cast_to); break;
		default: THROW_INTERNAL_ERROR("during type checking");
	}
}

VISIT(ArrayNode)
{
	// get first expr
	ParsedType* firsttype = PTYPE(TYPE_NONE);
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

	ParsedType* arrtype = firsttype->copy_pointer_to();
	node->_cast_to = arrtype;
	push(arrtype);
}

VISIT(SizeOfNode)
{
	node->_cast_to = PTYPE(
		GET_EVI_TYPE("sze")->_default_type,
		GET_EVI_TYPE("sze")
	);

	push(node->_cast_to);
}

VISIT(ReferenceNode)
{
	node->_cast_to = node->_type->copy();
	node->_cast_to->_is_reference = true;
	push(node->_cast_to);
}

VISIT(CallNode)
{
	for(int i = 0; i < node->_func_params_count; i++)
	{
		node->_arguments[i]->accept(this);
		ParsedType* exprtype = pop();
		ParsedType* argtype = node->_expected_arg_types[i];

		if(!can_cast_types(exprtype, argtype)) 
			ERROR_AT(&node->_arguments[i]->_token, "Cannot implicitly convert argument of type " COLOR_BOLD \
			"'%s'" COLOR_NONE " to parameter's type " COLOR_BOLD "'%s'" COLOR_NONE ".",
			exprtype->to_c_string(), argtype->to_c_string());
		else if(!exprtype->eq(argtype, true))
			CONVERSION_WARNING_AT(&node->_arguments[i]->_token, exprtype, argtype);

		_panic_mode = false;
	}
	
	// handle left over parameters as well
	for(int i = node->_func_params_count - 1; i < node->_arguments.size(); i++)
	{
		node->_arguments[i]->accept(this);
		pop();
	}

	push(node->_ret_type);
}