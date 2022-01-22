#include "parser.hpp"
#include "tools.hpp"

#include <cstdlib>

// ====================== errors =======================

void Parser::error_at(Token *token, string message)
{
	// already in panicmode. swallow error.
	if (_panic_mode)
		return;

	_panic_mode = true;

	fprintf(stderr, "[%s:%d] Error", _infile.c_str(), token->line);

	if (token->type == TOKEN_EOF)
	{
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR)
	{
		// Nothing.
	}
	else
	{
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message.c_str());
	_had_error = true;
	exit(1);
}

// displays an error at the previous token with the given message
void Parser::error(string message)
{
	//
	error_at(&_previous, message);
}

// displays an error at the current token with the given message
void Parser::error_at_current(string message)
{
	//
	error_at(&_current, message);
}

// ====================== scanner ======================

// advances to the next token
void Parser::advance()
{
	_previous = _current;

	for (;;)
	{
		_current = _scanner.scanToken();
		if (_current.type != TOKEN_ERROR)
			break;

		error_at_current(_current.start);
	}
}

// checks if the current token is of the given type
bool Parser::check(TokenType type)
{
	//
	return _current.type == type;
}

// consume the next token if it is of the correct type,
// otherwise throw an error with the given message
void Parser::consume(TokenType type, string message)
{
	if (_current.type == type)
	{
		advance();
		return;
	}

	error_at_current(message);
}

// consumes a terminator. this can be a newline or EOF
// in normal cases, but also a '}' when in a block.
// braces won't be consumed but checked.
void Parser::consume_terminator(string after)
{
	if(!match(TOKEN_NEWLINE) && !match(TOKEN_EOF) && 
	!(_current_scope.depth > 0 || check(TOKEN_RIGHT_BRACE)))
		error_at_current(tools::fstr("Expected newline%s after %s.",
			_current_scope.depth > 0 ? ", EOF or '}'" : " or EOF", after.c_str()));
}

// returns true and advances if the current token is of the given type
bool Parser::match(TokenType type)
{
	if (!check(type))
		return false;
	advance();
	return true;
}

// checks if EOF reached
bool Parser::is_at_end()
{
	//
	return _current.type == TOKEN_EOF;
}

// ======================= state =======================

// adds a local variable to the current scope
// if the variable already exists an error is thrown
void Parser::add_symbol(Token *identtoken)
{
	#define LOCALS _current_scope.symbols

	string name = string(identtoken->start, identtoken->length);

	// try to find local
	bool found = false;

	if (find(LOCALS.begin(), LOCALS.end(), name) != LOCALS.end())
		found = true;

	for (auto scope = _scope_stack.rbegin(); !found && scope != _scope_stack.rend(); scope++)
	{
		if (find((*scope).symbols.begin(), (*scope).symbols.end(), name) != (*scope).symbols.end())
		{
			found = true;
			break;
		}
	}

	if (found)
		error_at(identtoken, "Symbol already exists in current scope.");
	else
		_current_scope.symbols.push_back(name);

	#undef LOCALS
}

void Parser::scope_up()
{
	int depth = _current_scope.depth + 1;
	_scope_stack.push_back(_current_scope);
	_current_scope = (Scope){depth, vector<string>()};
}

void Parser::scope_down()
{
	Scope topscope = _scope_stack[_scope_stack.size()];
	DEBUG_PRINT_VAR(topscope.symbols.size(), %d);
	_current_scope = _scope_stack[_scope_stack.size()];
	_scope_stack.pop_back();
}

// ===================== declarations ===================

StmtNode* Parser::declaration()
{
	// declaration		: func_decl
	//					| var_decl
	//					| statement

	while(match(TOKEN_NEWLINE)) {}

	if(match(TOKEN_MODULO))
		return variable_declaration();
	else if(match(TOKEN_AT))
		return function_declaration();
	
	else
	{
		DEBUG_PRINT_F_MSG("declaration falling through to statement. (%d)", _current.type);
		return statement();
	}
}

StmtNode* Parser::function_declaration()
{
	// func_decl	: "@" IDENT TYPE "(" TYPE* ")"
	//				| "@" IDENT TYPE "(" TYPE* ")" "{" (func_decl | var_decl | statement)* "}"

	// get name
	consume(TOKEN_IDENTIFIER, "Expected identifier after '@'.");
	string name = PREV_TOKEN_STR;
	add_symbol(&_previous);

	// get type
	consume(TOKEN_TYPE, tools::fstr("Expected type after '@%s'.", name.c_str()));
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	EviType type = GET_EVI_TYPE(typestr);

	consume(TOKEN_LEFT_PAREN, "Expect '(' after return type.");

	// get parameters
	// TODO: parameters

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

	// get body
	DEBUG_PRINT_MSG("Start function body.");
	StmtNode* body = statement();
	DEBUG_PRINT_MSG("End function body.");
	consume_terminator("function declaration.");
	
	return new FuncDeclNode(name, type, body);
}

StmtNode* Parser::variable_declaration()
{
	// var_decl		: "%" IDENT ("," IDENT)* TYPE
	//				| "%" IDENT ("," IDENT)* TYPE expression ("," expression)*

	// get name
	consume(TOKEN_IDENTIFIER, "Expected identifier after '%'.");
	string name = PREV_TOKEN_STR;
	add_symbol(&_previous);

	// get type
	consume(TOKEN_TYPE, tools::fstr("Expected type after '%%%s'.", name.c_str()));
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	EviType type = GET_EVI_TYPE(typestr);

	// get expression
	ExprNode* expr = expression();
	consume_terminator("variable declaration.");

	return new VarDeclNode(name, type, expr);
}

// ===================== statements ====================

StmtNode* Parser::statement()
{
	// statement	: assignment
	// 				| loop
	//		 		| return
	// 				| block
	// 				| expression

	while(match(TOKEN_NEWLINE)) {}

	if(match(TOKEN_LEFT_BRACE)) return block();
	else
	{
		DEBUG_PRINT_F_MSG("statement falling through to expression. (%d)", _current.type);
		return expression_statement();
	}
}

StmtNode* Parser::block()
{
	// block		: "{" declaration* "}"

	AST statements = AST();
	scope_up();
	DEBUG_PRINT_F_MSG("Entered block! (new depth: %d)", _current_scope.depth);

	while(!check(TOKEN_RIGHT_BRACE) && !is_at_end()) statements.push_back(declaration());
	consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
	
	scope_down();
	DEBUG_PRINT_F_MSG("Exited block! (new depth: %d)", _current_scope.depth);
	return (StmtNode*)(new BlockNode(statements));
}

StmtNode* Parser::expression_statement()
{
	ExprNode* expr = expression();
	consume_terminator("expression");
	return (StmtNode*)expr;	
}

// ===================== expressions ===================

ExprNode* Parser::expression()
{
	// expression 	: ternary
	// return ternary();

	return logical_or();
}

ExprNode* Parser::logical_or()
{
	// logical_or	: logical_and ("||" logical_and)*
	ExprNode* expr = logical_and();

	while(match(TOKEN_PIPE_PIPE))
	{
		TokenType op = _previous.type;
		ExprNode* right = logical_and();
		expr = new LogicalNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::logical_and()
{
	// logical_and	: bitwise_or ("&&" bitwise_or)*
	ExprNode* expr = bitwise_or();

	while(match(TOKEN_AND_AND))
	{
		TokenType op = _previous.type;
		ExprNode* right = bitwise_or();
		expr = new LogicalNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_or()
{
	// bitwise_or	: bitwise_xor ("|" bitwise_xor)*
	ExprNode* expr = bitwise_xor();

	while(match(TOKEN_PIPE))
	{
		TokenType op = _previous.type;
		ExprNode* right = bitwise_xor();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_xor()
{
	// bitwise_xor	: bitwise_and ("^" bitwise_and)*
	ExprNode* expr = bitwise_and();

	while(match(TOKEN_CARET))
	{
		TokenType op = _previous.type;
		ExprNode* right = bitwise_and();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_and()
{
	// bitwise_and	: equality ("&" equality)*
	ExprNode* expr = equality();

	while(match(TOKEN_AND))
	{
		TokenType op = _previous.type;
		ExprNode* right = equality();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::equality()
{
	// equality		: comparison (("/=" | "==") comparison)*
	ExprNode* expr = comparison();

	while(match(TOKEN_SLASH_EQUAL) || match(TOKEN_EQUAL_EQUAL))
	{
		TokenType op = _previous.type;
		ExprNode* right = comparison();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::comparison()
{
	// comparison	: bitwise_shift ((">" | ">=" | "<" | "<=") bitwise_shift)*
	ExprNode* expr = bitwise_shift();

	while(match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL)
	   || match(TOKEN_LESS)    || match(TOKEN_LESS_EQUAL))
	{
		TokenType op = _previous.type;
		ExprNode* right = bitwise_shift();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_shift()
{
	// bitwise_shift	: term (("<<" | ">>") term)*
	ExprNode* expr = term();

	while(match(TOKEN_LESS_LESS) || match(TOKEN_GREATER_GREATER))
	{
		TokenType op = _previous.type;
		ExprNode* right = term();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::term()
{
	// term		: factor (("-" | "+") factor)*

	ExprNode* expr = factor();
	
	while(match(TOKEN_PLUS) || match(TOKEN_MINUS))
	{
		TokenType op = _previous.type;
		ExprNode* right = factor();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::factor()
{
	// factor		: unary (("/" | "*") unary)*

	ExprNode* expr = unary();
	
	while(match(TOKEN_STAR) || match(TOKEN_SLASH))
	{
		TokenType op = _previous.type;
		ExprNode* right = unary();
		expr = new BinaryNode(op, expr, right);
	}

	return expr;
}

ExprNode* Parser::unary()
{
	// unary		: ("!" | "-" | "++" | "--") unary | primary

	if(match(TOKEN_BANG) || match(TOKEN_MINUS)
	|| match(TOKEN_PLUS_PLUS) || match(TOKEN_MINUS_MINUS))
	{
		TokenType op = _previous.type;
		ExprNode* expr = unary();
		return new UnaryNode(op, expr);
	}

	return primary();
}

ExprNode* Parser::primary()
{
	// primary	: NUMBER | CHAR | STRING | "(" expression ")"
	// 			| "$" IDENT ("(" arguments? ")")?
	
	if(match(TOKEN_INTEGER) || match(TOKEN_FLOAT)
	|| match(TOKEN_CHARACTER) || match(TOKEN_STRING))
		return (ExprNode*)(new LiteralNode(PREV_TOKEN_STR, _previous.type));
	

	if(match(TOKEN_LEFT_PAREN))
	{
		ExprNode* expr = expression();
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after parenthesized expression.");
		return expr;
	}
	
	error_at_current("Expected expression.");
}

// ======================= misc. =======================

Status Parser::parse(string infile, AST* astree)
{
	// printTokensFromSrc(tools::readf(infile).c_str());
	_astree = astree;

	// set members
	_infile = infile;
	_source = tools::readf(_infile);
	_scanner = Scanner(_source.c_str());
	_had_error = false;
	_panic_mode = false;

	_scope_stack = vector<Scope>();
	_current_scope = (Scope){0, vector<string>()};

	// printTokensFromSrc(_source.c_str());

	advance();
	while (!match(TOKEN_EOF))
	{
		while(match(TOKEN_NEWLINE)) continue;

		if(match(TOKEN_MODULO))
			_astree->push_back(variable_declaration());
		else if(match(TOKEN_AT))
			_astree->push_back(function_declaration());
	
		else error("Expected declaration.");
	}

	DEBUG_PRINT_MSG("Parsing complete!");
	return STATUS_SUCCESS;
}