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

EviType Parser::get_prev_as_type()
{
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	return GET_EVI_TYPE(typestr);
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

// checks if the given variable already exists
bool Parser::check_variable(string name)
{
	if (find(_current_scope.symbols.begin(), _current_scope.symbols.end(), name) != _current_scope.symbols.end())
		return true;

	for (auto scope = _scope_stack.rbegin(); scope != _scope_stack.rend(); scope++)
	{
		if (find((*scope).symbols.begin(), (*scope).symbols.end(), name) != (*scope).symbols.end())
			return true;
	}
	return false;
}

// checks if the given function already exists
bool Parser::check_function(string name)
{
	if(_current_scope.functions.find(name) != _current_scope.functions.end())
		return true;

	for (auto scope = _scope_stack.rbegin(); scope != _scope_stack.rend(); scope++)
	{
		if (scope->functions.find(name) != scope->functions.end())
			return true;
	}
	return false;
}

void Parser::add_variable(Token* identtoken)
{
	string name = string(identtoken->start, identtoken->length);

	if(check_function(name)) error_at(identtoken, "Function with identical name already exists in current scope.");
	else if (check_variable(name)) error_at(identtoken, "Variable already exists in current scope.");
	else _current_scope.symbols.push_back(name);
}

void Parser::add_function(Token* identtoken, int arity)
{
	string name = string(identtoken->start, identtoken->length);

	if(check_function(name)) error_at(identtoken, "Function already exists in current scope.");
	else if (check_variable(name)) error_at(identtoken, "Variable with identical name already exists in current scope.");
	else _current_scope.functions.insert(pair<string, int>(name, arity));
}

void Parser::scope_up()
{
	int depth = _current_scope.depth + 1;
	int param_count = _current_scope.param_count;
	_scope_stack.push_back(_current_scope);
	_current_scope = (Scope){depth, param_count, vector<string>(), map<string, int>()};
}

void Parser::scope_down()
{
	_current_scope = _scope_stack[_scope_stack.size() - 1];
	_scope_stack.pop_back();
}

// ===================== declarations ===================

StmtNode* Parser::declaration()
{
	// declaration		: func_decl
	//					| var_decl
	//					| statement

	if(match(TOKEN_MODULO))
		return variable_declaration();
	else if(match(TOKEN_AT))
		return function_declaration();

	else
	// {
		// DEBUG_PRINT_F_MSG("declaration falling through to statement. (%d)", _current.type);
		return statement();
	// }
}

StmtNode* Parser::function_declaration()
{
	// func_decl	: "@" IDENT TYPE "(" TYPE* ")"
	//				| "@" IDENT TYPE "(" TYPE* ")" "{" (func_decl | var_decl | statement)* "}"

	// get name
	consume(TOKEN_IDENTIFIER, "Expected identifier after '@'.");
	string name = PREV_TOKEN_STR;
	Token tok = _previous;

	// get type
	consume(TOKEN_TYPE, tools::fstr("Expected type after '@%s'.", name.c_str()));
	EviType ret_type = get_prev_as_type();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after return type.");

	// get parameters
	vector<EviType> params;
	while(!check(TOKEN_RIGHT_PAREN)) do
	{
		if (params.size() >= 255) error_at_current("Parameter count exceeded limit of 255.");
		consume(TOKEN_TYPE, "Expected parameter.");
		params.push_back(get_prev_as_type());

	} while (match(TOKEN_COMMA));

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	add_function(&tok, params.size());

	// get body?
	if(match(TOKEN_SEMICOLON))
	{
		// declaration
		// DEBUG_PRINT_F_MSG("Declared function (\"%s\").", name.c_str());
		return new FuncDeclNode(name, ret_type, params, nullptr);
	}
	else
	{
		// definition
		// DEBUG_PRINT_F_MSG("Start function body (\"%s\").", name.c_str());
		scope_up();
		_current_scope.param_count = params.size();
		StmtNode* body = declaration();
		scope_down();
		// DEBUG_PRINT_F_MSG("End function body (\"%s\").", name.c_str());
		return new FuncDeclNode(name, ret_type, params, body);
	}
}

StmtNode* Parser::variable_declaration()
{
	// var_decl		: "%" IDENT ("," IDENT)* TYPE (expression ("," expression)*)?

	// get name(s)
	vector<string> names;
	do
	{
		consume(TOKEN_IDENTIFIER, names.size() > 0 ? 
			"Expected identifier after ','." :
			"Expected identifier after '%'.");
		names.push_back(PREV_TOKEN_STR);
		add_variable(&_previous);

	} while(match(TOKEN_COMMA));

	// get type
	consume(TOKEN_TYPE, "Expected type.");
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	EviType type = GET_EVI_TYPE(typestr);

	vector<VarDeclNode*> decls;
	// get initializers(s)?
	if(match(TOKEN_SEMICOLON))
	{
		// declarations
		for(string& name : names) decls.push_back(new VarDeclNode(name, type, nullptr));
	}
	else
	{
		// definitions
		
		for(int i = 0; i < names.size(); i++)
		{
			ExprNode* expr = expression();
			if(i + 1 < names.size()) consume(TOKEN_COMMA, "Expected ',' after expression.");
			decls.push_back(new VarDeclNode(string(names[i]), type, expr));
		}
		consume(TOKEN_SEMICOLON, "Expected ';' after variable defenition.");
	}

	assert(names.size() == decls.size());
	if(names.size() == 1) return decls[0];
	else
	{
		AST stmts;
		for(VarDeclNode*& decl : decls) stmts.push_back(decl);
		return new BlockNode(stmts, true);
	}
}

// ===================== statements ====================

StmtNode* Parser::statement()
{
	// statement	: assignment
	// 				| loop
	//		 		| return
	// 				| block
	// 				| expression


	if(match(TOKEN_EQUAL)) 			 return assign_statement();
	else if(match(TOKEN_TILDE)) 	 return return_statement();
	else if(match(TOKEN_LEFT_PAREN)) return loop_statement();
	else if(match(TOKEN_LEFT_BRACE)) return block_statement();
	else
	// {
	// 	DEBUG_PRINT_F_MSG("statement falling through to expression. (%d)", _current.type);
		return expression_statement();
	// }
}

StmtNode* Parser::assign_statement()
{
	// assignment	: "=" IDENT expression	
	consume(TOKEN_IDENTIFIER, "Expected identifier after '='.");
	string ident = PREV_TOKEN_STR;
	if(!check_variable(ident)) error("Variable doesn't exist in current scope.");

	ExprNode* expr = expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression.");

	return new AssignNode(ident, expr);
}

StmtNode* Parser::loop_statement()
{
	// loop		: "(" expression ")" declaration
	//			| "(" declaration ";" expression ";)" declaration
	//			| "(" declaration ";" expression ";" declaration ";)" declaration

	// 1st version: while-loop
	// 2nd version: while-loop with initializer
	// 3rd version: for-loop

	// we can't look ahead so we check later when we know 
	// how many expressions or statements we have what they are

	scope_up();

	Token firsttoken = _current;
	StmtNode* first = declaration();
	
	if(!check(TOKEN_RIGHT_PAREN)) // 2nd or 3rd version
	{
		// 2nd is always expression
		ExprNode* second = expression();
		consume(TOKEN_SEMICOLON, "Expected ';' after condition.");
		
		if(!check(TOKEN_RIGHT_PAREN)) // 3rd version
		{
			StmtNode* third = declaration();
			consume(TOKEN_RIGHT_PAREN, "Expected ')' after incrementer.");
			StmtNode* body = declaration();
			scope_down();
			
			return new LoopNode(first, second, third, body);
		}
		
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after ';' after condition.");
		StmtNode* body = declaration();
		scope_down();

		return new LoopNode(first, second, nullptr, body);
	}

	consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");
	StmtNode* body = declaration();
	scope_down();
	
	ExprNode* newfirst = dynamic_cast<ExprNode*>(first);
	if(newfirst == nullptr) error_at(&firsttoken, "Expected expression, not statement.");
		
	return new LoopNode(nullptr, newfirst, nullptr, body);
}

StmtNode* Parser::return_statement()
{
	// return 	: "~" expression?
	if(match(TOKEN_SEMICOLON)) return new ReturnNode(nullptr);
	else
	{
		ExprNode* expr = expression();
		consume(TOKEN_SEMICOLON, "Expected ';' after return statement.");
		return new ReturnNode(expr);
	}
}

StmtNode* Parser::block_statement()
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
	consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
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
	// primary		: NUMBER | CHAR | STRING | "(" expression ")"
	//				| reference | call

	// reference	: "$" (IDENT | INTEGER)
	// call			: IDENT "(" (expression ("," expression)*)? ")"
	
	// literals
	if(match(TOKEN_INTEGER) || match(TOKEN_FLOAT)
	|| match(TOKEN_CHARACTER) || match(TOKEN_STRING))
		return (ExprNode*)(new LiteralNode(PREV_TOKEN_STR, _previous.type));

	// grouping
	else if(match(TOKEN_LEFT_PAREN))
	{
		ExprNode* expr = expression();
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after parenthesized expression.");
		return new GroupingNode(expr);
	}

	// references
	else if(match(TOKEN_VARIABLE_REF))
	{
		string name = PREV_TOKEN_STR.erase(0, 1);
		if(!check_variable(name)) error("Variable doesn't exist in current scope.");
		return new ReferenceNode(name, -1, TOKEN_VARIABLE_REF);
	}
	else if(match(TOKEN_PARAMETER_REF))
	{
		int intval = strtol(PREV_TOKEN_STR.erase(0, 1).c_str(), NULL, 10);
		if(intval >= _current_scope.param_count) error(tools::fstr(
			"Parameter reference exceeds arity of %d.", intval));
		return new ReferenceNode("", intval, TOKEN_PARAMETER_REF);
	}

	// calls
	else if (match(TOKEN_IDENTIFIER))
	{
		string name = PREV_TOKEN_STR;
		vector<ExprNode*> args;
		
		consume(TOKEN_LEFT_PAREN, "Expected '(' after identifier.");
		if(!check(TOKEN_RIGHT_PAREN)) do
		{
			args.push_back(expression());
			if(args.size() >= 255) error("Argument count exceeded limit of 255.");
		
		} while(match(TOKEN_COMMA));
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
		
		return new CallNode(name, args);
	}

	error_at_current("Expected expression.");
	return nullptr;
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
	_current_scope = (Scope){0, 0, vector<string>(), map<string, int>()};

	// printTokensFromSrc(_source.c_str());

	advance();
	while (!match(TOKEN_EOF))
	{
		if(match(TOKEN_MODULO))
			_astree->push_back(variable_declaration());
		else if(match(TOKEN_AT))
			_astree->push_back(function_declaration());
	
		else error("Expected declaration.");
	}

	DEBUG_PRINT_MSG("Parsing complete!");
	return STATUS_SUCCESS;
}