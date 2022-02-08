#include "parser.hpp"
#include "tools.hpp"

#include <cstdlib>
#include <exception>

// ====================== errors =======================

void Parser::error_at(Token *token, string message)
{
	_error_dispatcher.dispatch_error_at(token, "Syntax Error", message.c_str());

	// print token
	if(token->type != TOKEN_ERROR)
	{
		cerr << endl;
		_error_dispatcher.dispatch_token_marked(token);
		cerr << endl;
	}

	exit(STATUS_PARSE_ERROR);
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

EviType* Parser::consume_type(string msg)
{
	// get base type
	consume(TOKEN_TYPE, msg);
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	EviType* type =  GET_EVI_TYPE(typestr);

	// get as pointer if applicable
	type->_parsed_type->_pointer_depth = 0;
	while(match(TOKEN_STAR)) type->_parsed_type->_pointer_depth++;

	return type;
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

// returns nullptr if not found
ParsedType* Parser::get_variable_type(string name)
{
	if(_current_scope.variables.find(name) != _current_scope.variables.end())
		return _current_scope.variables.at(name);

	for (auto scope = _scope_stack.rbegin(); scope != _scope_stack.rend(); scope++)
	{
		if (scope->variables.find(name) != scope->variables.end())
			return scope->variables.at(name);
	}
	return nullptr;
}

// returns with invalid = true if not found
Parser::FuncProperties Parser::get_function_props(string name)
{
	if(_current_scope.functions.find(name) != _current_scope.functions.end())
		return _current_scope.functions.at(name);

	for (auto scope = _scope_stack.rbegin(); scope != _scope_stack.rend(); scope++)
	{
		if (scope->functions.find(name) != scope->functions.end())
			return scope->functions.at(name);
	}
	return {.invalid = true};
}

// checks if the given variable already exists
bool Parser::check_variable(string name)
{
	if(get_variable_type(name) == nullptr)
		return false;
	return true;
}

// checks if the given function already exists
bool Parser::check_function(string name)
{
	if(get_function_props(name).invalid)
		return false;
	return true;
}

void Parser::add_variable(Token* identtoken, ParsedType* type)
{
	string name = string(identtoken->start, identtoken->length);

	if(check_function(name)) error_at(identtoken, "Function with identical name already exists in current scope.");
	else if (check_variable(name)) error_at(identtoken, "Variable already exists in current scope.");
	else _current_scope.variables.insert(pair<string, ParsedType*>(name, type));
}

void Parser::add_function(Token* identtoken, FuncProperties properties)
{
	string name = string(identtoken->start, identtoken->length);

	if (check_variable(name)) error_at(identtoken, "Variable with identical name already exists in current scope.");
	else if(check_function(name))
	{
		FuncProperties props = get_function_props(name);
		if(props.defined) error_at(identtoken, "Function already defined in current scope.");
		else if(!properties.defined) error_at(identtoken, "Function already declared in current scope.");
		else if(props.ret_type->_llvm_type != properties.ret_type->_llvm_type || props.params != properties.params)
			error_at(identtoken, "Function signature doesn't match declaration.");

		_current_scope.functions.at(name).defined = true;
	}
	else _current_scope.functions.insert(pair<string, FuncProperties>(name, properties));
}

void Parser::scope_up()
{
	int depth = _current_scope.depth + 1;
	FuncProperties func_props = _current_scope.func_props;
	
	_scope_stack.push_back(_current_scope);
	_current_scope = Scope{depth, map<string, ParsedType*>(), func_props, map<string, FuncProperties>()};
}

void Parser::scope_down()
{
	_current_scope = _scope_stack[_scope_stack.size() - 1];
	_scope_stack.pop_back();
}

// ===================== declarations ===================

StmtNode* Parser::declaration()
{
	// declaration		: var_decl
	//					| statement

	// if(match(TOKEN_AT)) return function_declaration();
	if(match(TOKEN_AT)) error("Function declaration is not allowed here.");
	else if(match(TOKEN_MODULO))  return variable_declaration();
	else return statement();

	return nullptr;
}

StmtNode* Parser::function_declaration()
{
	// func_decl	: "@" IDENT TYPE "(" TYPE* ")" (statement | ";")

	Token tok = _previous;

	// get name
	consume(TOKEN_IDENTIFIER, "Expected identifier after '@'.");
	string name = PREV_TOKEN_STR;
	Token nametok = _previous;

	// get type
	EviType* ret_type = consume_type(tools::fstr("Expected type after '@%s'.", name.c_str()));

	consume(TOKEN_LEFT_PAREN, "Expect '(' after return type.");

	// get parameters
	vector<EviType*> params;
	while(!check(TOKEN_RIGHT_PAREN)) do
	{
		if (params.size() >= 255) error_at_current("Parameter count exceeded limit of 255.");
		params.push_back(consume_type("Expected parameter."));

	} while (check(TOKEN_TYPE));

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

	// get body?
	if(match(TOKEN_SEMICOLON))
	{
		// declaration
		add_function(&nametok, {ret_type, params, false});
		return new FuncDeclNode(tok, name, ret_type, params, nullptr);
	}
	else
	{
		// definition
		add_function(&nametok, {ret_type, params, true});

		scope_up();
		_current_scope.func_props = FuncProperties{ret_type, params, true};
		
		StmtNode* body = statement();

		scope_down();
		return new FuncDeclNode(tok, name, ret_type, params, body);
	}
}

StmtNode* Parser::variable_declaration()
{
	// var_decl		: "%" IDENT ("," IDENT)* TYPE (expression ("," expression)*)? ";"

	Token tok = _previous;

	// get name(s)
	vector<Token> nametokens;
	do
	{
		consume(TOKEN_IDENTIFIER, nametokens.size() > 0 ? 
			"Expected identifier after ','." :
			"Expected identifier after '%'.");
		nametokens.push_back(_previous);

	} while(match(TOKEN_COMMA));

	// get type
	EviType* type = consume_type();
	// consume(TOKEN_TYPE, "Expected type.");
	// string typestr = PREV_TOKEN_STR;
	// if (!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	// EviType* type = GET_EVI_TYPE(typestr);
	
	// add to locals for parser to use
	for(Token& tok : nametokens) add_variable(&tok, type->_parsed_type);

	vector<VarDeclNode*> decls;
	// get initializers(s)?
	if(match(TOKEN_SEMICOLON))
	{
		// declarations
		for(Token& name : nametokens) decls.push_back(
			new VarDeclNode(tok, string(name.start, name.length), type, nullptr, _current_scope.depth == 0));
	}
	else
	{
		// definitions
		for(int i = 0; i < nametokens.size(); i++)
		{
			ExprNode* expr = expression();
			if(i + 1 < nametokens.size()) consume(TOKEN_COMMA, "Expected ',' after expression.");
			string name = string(nametokens[i].start, nametokens[i].length);
			decls.push_back(new VarDeclNode(tok, name, type, expr, _current_scope.depth == 0));
		}
		consume(TOKEN_SEMICOLON, "Expected ';' after variable defenition.");
	}

	assert(nametokens.size() == decls.size());
	if(nametokens.size() == 1) return decls[0];
	else
	{
		AST stmts;
		for(VarDeclNode*& decl : decls) stmts.push_back(decl);
		return new BlockNode(tok, stmts, true);
	}
}

// ===================== statements ====================

StmtNode* Parser::statement()
{
	// statement	: assignment
	//				| if_branch
	// 				| loop
	//		 		| return
	// 				| block
	// 				| expression ";"

	if	   (match(TOKEN_EQUAL			 ))	return assign_statement();
	else if(match(TOKEN_QUESTION_QUESTION)) return if_statement();
	else if(match(TOKEN_TILDE			 ))	return return_statement();
	else if(match(TOKEN_BANG_BANG	 	 ))	return loop_statement();
	else if(match(TOKEN_LEFT_BRACE		 ))	return block_statement();
	else									return expression_statement();
}

StmtNode* Parser::assign_statement()
{
	// assignment	: "=" IDENT expression ";"
	Token tok = _previous;

	consume(TOKEN_IDENTIFIER, "Expected identifier after '='.");
	string ident = PREV_TOKEN_STR;
	if(!check_variable(ident)) error("Variable doesn't exist in current scope.");

	ExprNode* expr = expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression.");

	return new AssignNode(tok, ident, expr, get_variable_type(ident));
}

StmtNode* Parser::if_statement()
{
	Token tok = _previous;

	consume(TOKEN_LEFT_PAREN, "Expect '(' after '\?\?'.");
	ExprNode* cond = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	StmtNode* if_branch = statement();
	StmtNode* else_branch = match(TOKEN_COLON_COLON) ? statement() : nullptr;
	return new IfNode(tok, cond, if_branch, else_branch);
}

StmtNode* Parser::loop_statement()
{
	// loop		: "!!" "(" (declaration | ";") expression ";" (declaration | ";") ")" statement
	Token tok = _previous;

	scope_up();
	consume(TOKEN_LEFT_PAREN, "Expected '(' after '!!'.");

	StmtNode* init = match(TOKEN_SEMICOLON) ? nullptr : declaration(); 

	ExprNode* cond = expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after condition.");

	StmtNode* incr = match(TOKEN_SEMICOLON) ? nullptr : declaration(); 
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after incrementor statement.");

	StmtNode* body = statement();

	scope_down();
	return new LoopNode(tok, init, cond, incr, body);
}

StmtNode* Parser::return_statement()
{
	// return 	: "~" expression? ";"
	Token tok = _previous;
	if(match(TOKEN_SEMICOLON)) return new ReturnNode(tok, nullptr, _current_scope.func_props.ret_type);
	else
	{
		ExprNode* expr = expression();
		consume(TOKEN_SEMICOLON, "Expected ';' after return statement.");
		return new ReturnNode(tok, expr, _current_scope.func_props.ret_type);
	}
}

StmtNode* Parser::block_statement()
{
	// block		: "{" declaration* "}"

	AST statements = AST();
	Token tok = _previous;
	scope_up();

	while(!check(TOKEN_RIGHT_BRACE) && !is_at_end()) statements.push_back(declaration());
	consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
	
	scope_down();
	return (StmtNode*)(new BlockNode(tok, statements));
}

StmtNode* Parser::expression_statement()
{
	ExprNode* expr = expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
	return expr;	
}

// ===================== expressions ===================

ExprNode* Parser::expression()
{
	// expression 	: ternary
	return ternary();
}

ExprNode* Parser::ternary()
{
	// ternary		: logical_or ("?" expression ":" ternary)?

	ExprNode* expr = logical_or();

	while(match(TOKEN_QUESTION))
	{
		Token tok = _previous;
		ExprNode* middle = expression();
	
		consume(TOKEN_COLON, "Expect ':' after if-expression.");
		ExprNode* right = ternary();
	
		expr = new LogicalNode(tok, expr, right, middle);
	}

	return expr;
}

ExprNode* Parser::logical_or()
{
	// logical_or	: logical_and ("||" logical_and)*
	ExprNode* expr = logical_and();

	while(match(TOKEN_PIPE_PIPE))
	{
		Token tok = _previous;
		ExprNode* right = logical_and();
		expr = new LogicalNode(tok, expr, right);
	}

	return expr;
}

ExprNode* Parser::logical_and()
{
	// logical_and	: bitwise_or ("&&" bitwise_or)*
	ExprNode* expr = bitwise_or();

	while(match(TOKEN_AND_AND))
	{
		Token tok = _previous;
		ExprNode* right = bitwise_or();
		expr = new LogicalNode(tok, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_or()
{
	// bitwise_or	: bitwise_xor ("|" bitwise_xor)*
	ExprNode* expr = bitwise_xor();

	while(match(TOKEN_PIPE))
	{
		Token tok = _previous;
		ExprNode* right = bitwise_xor();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_xor()
{
	// bitwise_xor	: bitwise_and ("^" bitwise_and)*
	ExprNode* expr = bitwise_and();

	while(match(TOKEN_CARET))
	{
		Token tok = _previous;
		ExprNode* right = bitwise_and();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_and()
{
	// bitwise_and	: equality ("&" equality)*
	ExprNode* expr = equality();

	while(match(TOKEN_AND))
	{
		Token tok = _previous;
		ExprNode* right = equality();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::equality()
{
	// equality		: comparison (("/=" | "==") comparison)*
	ExprNode* expr = comparison();

	while(match(TOKEN_SLASH_EQUAL) || match(TOKEN_EQUAL_EQUAL))
	{
		Token tok = _previous;
		ExprNode* right = comparison();
		expr = new BinaryNode(tok, tok.type, expr, right);
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
		Token tok = _previous;
		ExprNode* right = bitwise_shift();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::bitwise_shift()
{
	// bitwise_shift	: term (("<<" | ">>") term)*
	ExprNode* expr = term();

	while(match(TOKEN_LESS_LESS) || match(TOKEN_GREATER_GREATER))
	{
		Token tok = _previous;
		ExprNode* right = term();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::term()
{
	// term		: factor (("-" | "+") factor)*

	ExprNode* expr = factor();
	
	while(match(TOKEN_PLUS) || match(TOKEN_MINUS))
	{
		Token tok = _previous;
		ExprNode* right = factor();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::factor()
{
	// factor		: unary (("/" | "*") unary)*

	ExprNode* expr = unary();
	
	while(match(TOKEN_STAR) || match(TOKEN_SLASH))
	{
		Token tok = _previous;
		ExprNode* right = unary();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::unary()
{
	// unary		: ("*" | "&" | "!" | "-" | "++" | "--") unary | primary

	if(match(TOKEN_BANG) || match(TOKEN_MINUS)     || match(TOKEN_STAR) 
	|| match(TOKEN_AND)  || match(TOKEN_PLUS_PLUS) || match(TOKEN_MINUS_MINUS))
	{
		Token tok = _previous;
		ExprNode* expr = unary();
		return new UnaryNode(tok, tok.type, expr);
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
	if(match(TOKEN_INTEGER))
	{
		int base = 0;
		string tok = PREV_TOKEN_STR;
		
			 if(tok.size() > 2 && tolower(tok[1]) == 'b') base = 2;
		else if(tok.size() > 2 && tolower(tok[1]) == 'c') base = 8;
		else if(tok.size() > 2 && tolower(tok[1]) == 'x') base = 16;
		else base = 10;

		long intval = strtol(tok.c_str() + (base != 10 ? 2 : 0), NULL, base);
		return new LiteralNode(_previous, intval);
	}
	else if(match(TOKEN_FLOAT))
	{
		string tok = PREV_TOKEN_STR;

		double doubleval = strtod(tok.c_str(), NULL);
		return new LiteralNode(_previous, doubleval);
	}
	else if(match(TOKEN_CHARACTER))
	{
		// DEBUG_PRINT_VAR(PREV_TOKEN_STR.c_str(), %s);
		string tok = PREV_TOKEN_STR;
		char ch;

		if(tok[1] == '\\') ch = tools::escchr(tok[2]);
		else ch = tok[1];

		return new LiteralNode(_previous, ch);
	}
	else if(match(TOKEN_STRING))
	{
		return new LiteralNode(_previous, string(_previous.start + 1, _previous.length - 2));
	}

	// grouping
	else if(match(TOKEN_LEFT_PAREN))
	{
		

		Token tok = _previous;
		ExprNode* expr = expression();
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after parenthesized expression.");
		return new GroupingNode(tok, expr);
	}

	// references
	else if(match(TOKEN_VARIABLE_REF))
	{
		string name = PREV_TOKEN_STR.erase(0, 1);
		if(!check_variable(name)) error("Variable doesn't exist in current scope.");
		return new ReferenceNode(_previous, name, -1, get_variable_type(name));
	}
	else if(match(TOKEN_PARAMETER_REF))
	{
		int intval = strtol(PREV_TOKEN_STR.erase(0, 1).c_str(), NULL, 10);
		
		int arity = _current_scope.func_props.params.size();
		if(intval >= arity) error(tools::fstr("Parameter reference exceeds arity of %d.", arity));

		ParsedType* type = _current_scope.func_props.params[intval]->_parsed_type;
		return new ReferenceNode(_previous, "", intval, type);
	}

	// calls
	else if (match(TOKEN_IDENTIFIER))
	{
		Token tok = _previous;
		string name = PREV_TOKEN_STR;
		if(!check_function(name)) error("Function does not exist in current scope.");

		vector<ExprNode*> args;
		FuncProperties funcprops = get_function_props(name);
		
		consume(TOKEN_LEFT_PAREN, "Expected '(' after identifier.");
		if(!check(TOKEN_RIGHT_PAREN)) do
		{
			args.push_back(expression());
			if(args.size() > funcprops.params.size()) break;
		
		} while(match(TOKEN_COMMA));

		if(args.size() != funcprops.params.size())
			error(tools::fstr("Expected %d argument%s, not %d.",
				funcprops.params.size(), funcprops.params.size() == 1 ? "" : "s", args.size()));
		
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");

		vector<ParsedType*> lexparams; for(EviType*& p : funcprops.params) lexparams.push_back(p->_parsed_type);
		return new CallNode(tok, name, args, funcprops.ret_type->_parsed_type, lexparams);
	}

	error_at_current("Expected expression.");
	return nullptr;
}

// ======================= misc. =======================

Status Parser::parse(string infile, const char* source, AST* astree)
{
	// printTokensFromSrc(tools::readf(infile).c_str());
	_astree = astree;

	// set members
	_scanner = Scanner(source);

	_scope_stack = vector<Scope>();
	_current_scope = Scope{0, map<string, ParsedType*>(), {}, map<string, FuncProperties>()};

	_error_dispatcher = ErrorDispatcher(source, infile.c_str());

	advance();
	while (!match(TOKEN_EOF))
	{
		if(match(TOKEN_MODULO))  _astree->push_back(variable_declaration());
		else if(match(TOKEN_AT)) _astree->push_back(function_declaration());
		else error_at_current("Expected declaration at top-level code.");
	}

	DEBUG_PRINT_MSG("Parsing complete!");
	return STATUS_SUCCESS;
}