#include "parser.hpp"
#include "tools.hpp"

// ====================== errors =======================

void Parser::error_at(Token *token, string message)
{
	if(_panic_mode) return;

	_had_error = true;
	_panic_mode = true;

	if(lint_args.type == LINT_GET_ERRORS) lint_output_error_object(token, message, "error");
	else if(lint_args.type == LINT_NONE)
	{
		_error_dispatcher.error_at_token(token, "Syntax Error", message.c_str());

		// print token
		if(token->type != TOKEN_ERROR)
		{
			cerr << endl;
			_error_dispatcher.print_token_marked(token, COLOR_RED);
		}
	}
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

// displays note of declaration of token and line of token
void Parser::note_declaration(string type, string name, Token* token)
{
	const char* msg = strdup((type + " '" + name + "' declared here:").c_str());

	if(lint_args.type == LINT_GET_ERRORS)
	{
		// // remove "], }, " at end of error object
		// lint_output.erase(lint_output.end() - 6);

		LINT_OUTPUT_START_PLAIN_OBJECT();
		
		LINT_OUTPUT_PAIR("file", *token->file);
		LINT_OUTPUT_PAIR_F("line", token->line, %d);
		// LINT_OUTPUT_PAIR_F("column", get_token_col(token, lint_args.tab_width), %d);
		LINT_OUTPUT_PAIR_F("column", get_token_col(token), %d);
		LINT_OUTPUT_PAIR_F("length", token->length, %d);
		LINT_OUTPUT_PAIR("message", msg);

		LINT_OUTPUT_OBJECT_END();
	}
	else if(lint_args.type == LINT_NONE)
	{
		_error_dispatcher.note_at_token(token, msg);
		cerr << endl;
		_error_dispatcher.print_token_marked(token, COLOR_GREEN);
	}	
}

// ====================== scanner ======================

// advances to the next token
void Parser::advance(bool can_trigger_lint)
{
	_previous = _current;

	for (;;)
	{
		_current = _scanner.scanToken();

		if(can_trigger_lint && (*_current.file == _main_file)
		&& (lint_args.type == LINT_GET_FUNCTIONS || lint_args.type == LINT_GET_VARIABLES || lint_args.type == LINT_GET_DECLARATION)
		&& ((_current.line == lint_args.pos[0] && get_token_col(&_current, lint_args.tab_width) >= lint_args.pos[1]) || _current.line > lint_args.pos[0]))
			generate_lint();

		else if (_current.type == TOKEN_ERROR)
			error_at_current(_current.start);

		else if(_current.type == TOKEN_LINE_MARKER)
			// _error_dispatcher.set_filename(strdup(_current.start));
			{}

		else break;
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
bool Parser::consume(TokenType type, string message)
{
	if (_current.type == type)
	{
		advance();
		return true;
	}
	error_at_current(message);
	return false;
}

ParsedType* Parser::consume_type(string msg)
{
	// get base type
	CONSUME_OR_RET_NULL(TOKEN_TYPE, msg);
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr))
	{
		error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
		return ParsedType::new_invalid();
	}

	ParsedType* type = PTYPE(
		GET_EVI_TYPE(typestr)->_default_type,
		GET_EVI_TYPE(typestr));

	// can be pointer
	while(match(TOKEN_STAR)) type = type->copy_pointer_to();

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

// ======================= lint ========================

void Parser::generate_lint()
{
	// cout << tools::fstr("lint at: %s:%d:%d (%s)", _current.file->c_str(), _current.line, 
	// 	get_token_col(&_current, lint_args.tab_width), getTokenStr(_current.type)) << endl;

	switch(lint_args.type)
	{
		case LINT_GET_FUNCTIONS:
		{
			LINT_OUTPUT_START_PLAIN_OBJECT();

			// output props of functions in current scope
			for(auto const& func : _current_scope.functions)
			{
				LINT_OUTPUT_OBJECT_START(func.first);
				LINT_OUTPUT_PAIR("return type", func.second.ret_type->to_string());

				LINT_OUTPUT_ARRAY_START("parameters");
				for(auto const& param : func.second.params)
					LINT_OUTPUT_ARRAY_ITEM(param->to_string());
				LINT_OUTPUT_ARRAY_END();
				LINT_OUTPUT_PAIR_F("variadic", func.second.variadic ? "true" : "false", %s);

				LINT_OUTPUT_OBJECT_END();
			}

			// and now of all other scopes
			for(auto scope = _scope_stack.rbegin(); scope != _scope_stack.rend(); scope++)
				for(auto const& func : scope->functions)
				{
					LINT_OUTPUT_OBJECT_START(func.first);
					LINT_OUTPUT_PAIR("return type", func.second.ret_type->to_string());

					LINT_OUTPUT_ARRAY_START("parameters");
					for(auto const& param : func.second.params)
						LINT_OUTPUT_ARRAY_ITEM(param->to_string());
					LINT_OUTPUT_ARRAY_END();
					LINT_OUTPUT_PAIR_F("variadic", func.second.variadic ? "true" : "false", %s);

					LINT_OUTPUT_OBJECT_END();
				}						
				

			LINT_OUTPUT_END_PLAIN_OBJECT();
			break;
		}
		case LINT_GET_VARIABLES:
		{
			LINT_OUTPUT_START_PLAIN_OBJECT();

			for(int i = 0; i < _current_scope.func_props.params.size(); i++)
				LINT_OUTPUT_PAIR(tools::fstr("%d", i), _current_scope.func_props.params[i]->to_string());

			for (auto const& var : _current_scope.variables)
				LINT_OUTPUT_PAIR(var.first, var.second.type->to_string());

			for (auto scope = _scope_stack.rbegin(); scope != _scope_stack.rend(); scope++)
				for(auto const& var : scope->variables)
					LINT_OUTPUT_PAIR(var.first, var.second.type->to_string());

			LINT_OUTPUT_END_PLAIN_OBJECT();
			break;
		}	
		case LINT_GET_DECLARATION:
		{
			#pragma region get token
			#define IS_WANTED_TOKEN(token) \
				(token.type == TOKEN_IDENTIFIER || token.type == TOKEN_VARIABLE_REF || token.type == TOKEN_PARAMETER_REF)
			
			// if cursor is after identifier use previous instead
			Token token = IS_WANTED_TOKEN(_current) ? _current :
						  IS_WANTED_TOKEN(_previous) ? _previous :
						  (advance(false), _current);
						//   IS_WANTED_TOKEN((advance(false), _current)) ? _current
						//   : _current_call_token.file ? _current_call_token : _current;
			
			#undef IS_WANTED_TOKEN
			#pragma endregion
			string name(token.start, token.length);
			Token decltok;

			#define INVALID() { LINT_OUTPUT_PAIR_F("invalid", "true", %s); LINT_OUTPUT_END_PLAIN_OBJECT(); break; }

			LINT_OUTPUT_START_PLAIN_OBJECT();

			if(token.type == TOKEN_VARIABLE_REF)
			{
				name.erase(0, 1);
				if(!check_variable(name)) INVALID()
				else decltok = get_variable_props(name).token;
			}
			else if(token.type == TOKEN_PARAMETER_REF)
			{
				decltok = _current_scope.func_props.token;
			}
			else if(token.type == TOKEN_IDENTIFIER)
			{
				if(!check_function(name)) INVALID()
				else decltok = get_function_props(name).token;
			}
			else INVALID()

			LINT_OUTPUT_PAIR("file", *decltok.file);
			LINT_OUTPUT_PAIR_F("line", decltok.line, %d);
			LINT_OUTPUT_PAIR_F("column", get_token_col(&decltok), %d);
			LINT_OUTPUT_PAIR_F("length", decltok.length, %d);

			LINT_OUTPUT_END_PLAIN_OBJECT();
			#undef INVALID
			break;
		}
		default: assert(false && "invalid lint type?");
	}

	cout << lint_output;
	exit(0);
}

// ======================= state =======================

// returns nullptr if not found
Parser::VarProperties Parser::get_variable_props(string name)
{
	if(_current_scope.variables.find(name) != _current_scope.variables.end())
		return _current_scope.variables.at(name);

	for (auto scope = _scope_stack.rbegin(); scope != _scope_stack.rend(); scope++)
	{
		if (scope->variables.find(name) != scope->variables.end())
			return scope->variables.at(name);
	}
	return { nullptr };
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
	if(get_variable_props(name).type == nullptr)
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
	else _current_scope.variables.insert(pair<string, VarProperties>(name, { type->copy(), *identtoken }));
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
		else if(props.ret_type != properties.ret_type || props.params != properties.params)
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
	_current_scope = Scope{
		depth,
		map<string, VarProperties>(),
		func_props,
		map<string, FuncProperties>()};
}

void Parser::scope_down()
{
	_current_scope = _scope_stack[_scope_stack.size() - 1];
	_scope_stack.pop_back();
}

void Parser::synchronize(bool toplevel)
{
	_panic_mode = false;

	if(lint_args.type == LINT_GET_ERRORS) lint_output_error_object_end();

	if(_previous.type == TOKEN_SEMICOLON)
	{
		advance();
		// return;
	}

	if(toplevel) while(!is_at_end())
	{
		if(match(TOKEN_LEFT_BRACE)) while(!match(TOKEN_RIGHT_BRACE)) { advance(); }
		if(match(TOKEN_LEFT_B_BRACE)) while(!match(TOKEN_RIGHT_B_BRACE)) { advance(); }
		if(match(TOKEN_LEFT_PAREN)) while(!match(TOKEN_RIGHT_PAREN)) { advance(); }

		if(_current.type == TOKEN_AT || _current.type == TOKEN_MODULO)
			return;

		advance();
	}
	else while(!is_at_end())
	{
		if(_previous.type == TOKEN_SEMICOLON) return;
		//  { cerr << "---\n"; return; }

		switch(_current.type)
		{
			case TOKEN_AT:					// func
			case TOKEN_MODULO:				// var

			case TOKEN_EQUAL:				// assign
			case TOKEN_QUESTION_QUESTION: 	// if
			case TOKEN_TILDE:				// return
			case TOKEN_BANG_BANG:			// for
			case TOKEN_LEFT_BRACE:			// block

			case TOKEN_RIGHT_BRACE:
			// case TOKEN_RIGHT_PAREN:
				return;

			default:
				;
		}

		advance();
	}
}

// ===================== declarations ===================

StmtNode* Parser::declaration()
{
	// declaration		: var_decl
	//					| statement

	StmtNode* stmt = nullptr;

	// if(match(TOKEN_AT)) return function_declaration();
	if(match(TOKEN_AT)) error("Function declaration is not allowed here.");
	else if(match(TOKEN_MODULO)) stmt = variable_declaration();
	else stmt = statement();

	if(_panic_mode) synchronize(false);

	return stmt;
}

StmtNode* Parser::function_declaration()
{
	// func_decl	: "@" IDENT TYPE "(" TYPE* ")" (statement | ";")

	Token tok = _previous;

	// get name
	CONSUME_OR_RET_NULL(TOKEN_IDENTIFIER, "Expected identifier after '@'.");
	string name = PREV_TOKEN_STR;
	Token nametok = _previous;

	// get type
	ParsedType* ret_type = consume_type(tools::fstr("Expected type after '@%s'.", name.c_str()));

	CONSUME_OR_RET_NULL(TOKEN_LEFT_PAREN, "Expect '(' after return type.");

	// get parameters
	vector<ParsedType*> params;
	bool is_variadic = false;
	if(!check(TOKEN_RIGHT_PAREN)) do
	{
		if (params.size() >= 255) error_at_current("Parameter count exceeded limit of 255.");
		else if(match(TOKEN_ELIPSES))
		{
			is_variadic = true;
			break;
		}
		else
		{
			ParsedType* type = consume_type("Expected type as parameter.");
			params.push_back(type);
		}
	} while (check(TOKEN_TYPE) || check(TOKEN_ELIPSES));

	CONSUME_OR_RET_NULL(TOKEN_RIGHT_PAREN, is_variadic ? "Expect ')' after '...'." : "Expect ')' after parameters.");

	// get body?
	if(match(TOKEN_SEMICOLON))
	{
		// declaration
		add_function(&nametok, {ret_type, params, is_variadic, false, false, tok});
		return new FuncDeclNode(tok, name, ret_type, params, is_variadic, nullptr);
	}
	else
	{
		// definition
		FuncProperties props = {ret_type, params, is_variadic, true, false, tok};
		add_function(&nametok, props);

		scope_up();
		_current_scope.func_props = props;
		
		StmtNode* body = statement();

		scope_down();
		return new FuncDeclNode(tok, name, ret_type, params, is_variadic, body);
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
		CONSUME_OR_RET_NULL(TOKEN_IDENTIFIER, nametokens.size() > 0 ? 
			"Expected identifier after ','." :
			"Expected identifier after '%'.");
		nametokens.push_back(_previous);

	} while(match(TOKEN_COMMA));

	// get type
	ParsedType* type = consume_type(nametokens.size() > 1 ? "Expected type after identifiers." : "Expected type after identifier.");
	if(!type || type->_invalid) return nullptr;
	type->_is_reference = true;

	// add to locals for parser to use
	for(Token& tok : nametokens) add_variable(&tok, type);

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
			if(i + 1 < nametokens.size()) CONSUME_OR_RET_NULL(TOKEN_COMMA, "Expected ',' after expression.");
			string name = string(nametokens[i].start, nametokens[i].length);
			decls.push_back(new VarDeclNode(tok, name, type, expr, _current_scope.depth == 0));
		}
		CONSUME_OR_RET_NULL(TOKEN_SEMICOLON, "Expected ';' after variable defenition.");
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
	//				| ";"

	if	   (match(TOKEN_EQUAL			 ))	return assign_statement();
	else if(match(TOKEN_QUESTION_QUESTION)) return if_statement();
	else if(match(TOKEN_TILDE			 ))	return return_statement();
	else if(match(TOKEN_BANG_BANG	 	 ))	return loop_statement();
	else if(match(TOKEN_SEMICOLON	 	 ))	return nullptr;
	else if(match(TOKEN_LEFT_BRACE		 ))	return block_statement();
	else									return expression_statement();
}

StmtNode* Parser::assign_statement()
{
	// assignment	: "=" IDENT ("[" expression "]")* expression ";"
	Token tok = _previous;

	CONSUME_OR_RET_NULL(TOKEN_IDENTIFIER, "Expected identifier after '='.");
	string ident = PREV_TOKEN_STR;
	if(!check_variable(ident)) error("Variable doesn't exist in current scope.");

	// allow subscript
	vector<ExprNode*> subs = vector<ExprNode*>();
	while(match(TOKEN_LEFT_B_BRACE))
	{
		subs.push_back(expression());
		consume(TOKEN_RIGHT_B_BRACE, "Expected ']' after subscript index.");
	}

	ExprNode* expr = expression();
	CONSUME_OR_RET_NULL(TOKEN_SEMICOLON, "Expected ';' after expression.");

	return new AssignNode(tok, ident, subs, expr, get_variable_props(ident).type);
}

StmtNode* Parser::if_statement()
{
	Token tok = _previous;

	CONSUME_OR_RET_NULL(TOKEN_LEFT_PAREN, "Expect '(' after '\?\?'.");
	ExprNode* cond = expression();
	CONSUME_OR_RET_NULL(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	StmtNode* if_branch = statement();
	StmtNode* else_branch = match(TOKEN_COLON_COLON) ? statement() : nullptr;
	return new IfNode(tok, cond, if_branch, else_branch);
}

StmtNode* Parser::loop_statement()
{
	// loop		: "!!" "(" (declaration | ";") expression ";" (declaration | ";") ")" statement
	Token tok = _previous;

	scope_up();
	CONSUME_OR_RET_NULL(TOKEN_LEFT_PAREN, "Expected '(' after '!!'.");

	StmtNode* init = match(TOKEN_SEMICOLON) ? nullptr : declaration(); 

	ExprNode* cond = expression();
	CONSUME_OR_RET_NULL(TOKEN_SEMICOLON, "Expect ';' after condition.");

	StmtNode* incr = match(TOKEN_SEMICOLON) ? nullptr : declaration(); 
	CONSUME_OR_RET_NULL(TOKEN_RIGHT_PAREN, "Expect ')' after incrementor statement.");

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
		CONSUME_OR_RET_NULL(TOKEN_SEMICOLON, "Expected ';' after return statement.");
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
	CONSUME_OR_RET_NULL(TOKEN_RIGHT_BRACE, "Expected '}' after block.");

	scope_down();
	return (StmtNode*)(new BlockNode(tok, statements));
}

StmtNode* Parser::expression_statement()
{
	ExprNode* expr = expression();
	CONSUME_OR_RET_NULL(TOKEN_SEMICOLON, "Expected ';' after expression.");
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
	
		CONSUME_OR_RET_NULL(TOKEN_COLON, "Expect ':' after if-expression.");
		ExprNode* right = ternary();
	
		expr = new LogicalNode(tok, expr, right, middle);
	}

	return expr;
}

ExprNode* Parser::logical_or()
{
	// logical_or		: logical_xor ("||" logical_xor)*
	ExprNode* expr = logical_xor();

	while(match(TOKEN_PIPE_PIPE))
	{
		Token tok = _previous;
		ExprNode* right = logical_xor();
		expr = new LogicalNode(tok, expr, right);
	}

	return expr;
}

ExprNode* Parser::logical_xor()
{
	// logical_or		: logical_and ("^^" logical_and)*
	ExprNode* expr = logical_and();

	while(match(TOKEN_CARET_CARET))
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
	// factor		: cast (("/" | "*") cast)*

	ExprNode* expr = cast();
	
	while(match(TOKEN_STAR) || match(TOKEN_SLASH))
	{
		Token tok = _previous;
		ExprNode* right = cast();
		expr = new BinaryNode(tok, tok.type, expr, right);
	}

	return expr;
}

ExprNode* Parser::cast()
{
	// cast		: unary ("->" TYPE)*

	ExprNode* expr = unary();

	while(match(TOKEN_ARROW))
	{
		Token tok = _previous;
		ParsedType* type = consume_type("Expected type after '->'.");
		expr = new CastNode(tok, expr, type);
	}

	return expr;
}

ExprNode* Parser::unary()
{
	// unary		: ("*" | "&" | "!" | "-" | "++" | "--") unary | subscript

	if(match(TOKEN_STAR)  || match(TOKEN_AND) 		|| match(TOKEN_BANG)
	|| match(TOKEN_MINUS) || match(TOKEN_PLUS_PLUS) || match(TOKEN_MINUS_MINUS))
	{
		if(_previous.type == TOKEN_AND) _error_dispatcher.warning_at_token(&_previous, "Syntax Warning", 
			"Use of unary '&' operator not officially supported, behaviour may be undefined.");
		if(_previous.type == TOKEN_STAR) _error_dispatcher.warning_at_token(&_previous, "Syntax Warning", 
			"Use of unary '*' operator not officially supported, behaviour may be undefined.");

		Token tok = _previous;
		ExprNode* expr = unary();
		return new UnaryNode(tok, tok.type, expr);
	}

	return subscript();
}

ExprNode* Parser::subscript()
{
	// subscript	: primary ("[" expression "]")*

	ExprNode* expr = primary();

	while(match(TOKEN_LEFT_B_BRACE))
	{
		Token tok = _previous;
		ExprNode* index = expression();
		consume(TOKEN_RIGHT_B_BRACE, "Expected ']' after subscript index.");
		expr = new SubscriptNode(tok, expr, index);
	}

	return expr;
}

ExprNode* Parser::primary()
{
	// primary		: NUMBER | CHAR | STRING | "(" expression ")"
	// 				| array | size_of | reference | call

	// array		: "{" (expression ("," expression)*)? "}"
	// size_of		: "?" ( "(" TYPE ")" | TYPE )
	// reference	: "$" (IDENT | INTEGER)
	// call			: IDENT "(" (expression ("," expression)*)? ")"
	

	// literals
	if(match(TOKEN_INTEGER) || match(TOKEN_FLOAT)
	|| match(TOKEN_CHARACTER) || match(TOKEN_STRING))
		return literal();

	// grouping
	else if(match(TOKEN_LEFT_PAREN))
	{
		Token tok = _previous;
		ExprNode* expr = expression();
		CONSUME_OR_RET_NULL(TOKEN_RIGHT_PAREN, "Expected ')' after parenthesized expression.");
		return new GroupingNode(tok, expr);
	}


	// arrays
	else if(match(TOKEN_LEFT_BRACE)) return array();

	// size of
	else if(match(TOKEN_QUESTION)) return size_of();

	// references
	else if(match(TOKEN_VARIABLE_REF) || match(TOKEN_PARAMETER_REF)) return reference();

	// calls
	else if (match(TOKEN_IDENTIFIER)) return call();

	error_at_current("Expected expression.");
	return nullptr;
}

// ===================== primaries =====================

LiteralNode* Parser::literal()
{
	switch(_previous.type)
	{
		case TOKEN_INTEGER:
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
		case TOKEN_FLOAT:
		{
			string tok = PREV_TOKEN_STR;
			double doubleval = strtod(tok.c_str(), NULL);
			return new LiteralNode(_previous, doubleval);
		}
		case TOKEN_CHARACTER:
		{
			string tok = PREV_TOKEN_STR;
			char ch;

			if(tok[1] == '\\') ch = tools::escchr(tok[2]);
			else ch = tok[1];

			return new LiteralNode(_previous, ch);
		}
		case TOKEN_STRING:
		{
			//
			return new LiteralNode(_previous, string(_previous.start + 1, _previous.length - 2));
		}
		default: assert(false);
	}
}

ArrayNode* Parser::array()
{
	Token tok = _previous;

	vector<ExprNode*> elements;
	if(!check(TOKEN_RIGHT_BRACE)) do
	{
		// simple expression
		elements.push_back(expression());
	} while(match(TOKEN_COMMA));

	CONSUME_OR_RET_NULL(TOKEN_RIGHT_BRACE, "Expected '}' after array elements.");
	return new ArrayNode(tok, elements);
}

SizeOfNode* Parser::size_of()
{
	Token tok = _previous;

	bool has_parens = match(TOKEN_LEFT_PAREN);
	ParsedType* type = consume_type(has_parens ?
		"Expected type after '?' and '('." : "Expected type or '(' after '?'.");
	if(has_parens) consume(TOKEN_RIGHT_PAREN, "Expected ')' after type.");

	return new SizeOfNode(tok, type);
}

ReferenceNode* Parser::reference()
{
	if(_previous.type == TOKEN_VARIABLE_REF)
	{
		string name = PREV_TOKEN_STR.erase(0, 1);
		if(!check_variable(name)) error("Variable doesn't exist in current scope.");
		return new ReferenceNode(_previous, name, -1, get_variable_props(name).type);
	}
	else if(_previous.type == TOKEN_PARAMETER_REF)
	{
		int intval = strtol(PREV_TOKEN_STR.erase(0, 1).c_str(), NULL, 10);
		
		int arity = _current_scope.func_props.params.size();
		if(intval >= arity)
		{
			HOLD_PANIC();
			error(tools::fstr("Parameter reference exceeds arity of %d.", arity));
			if(!PANIC_HELD)
			{
				string name(_current_scope.func_props.token.start, _current_scope.func_props.token.length);
				note_declaration("Surrounding function", name, &_current_scope.func_props.token);
			}
			return nullptr;
		}
		ParsedType* type = _current_scope.func_props.params[intval];
		return new ReferenceNode(_previous, "", intval, type);
	}
	assert(false);
}

CallNode* Parser::call()
{
	Token tok = _previous;
	// _current_call_token = tok;

	CONSUME_OR_RET_NULL(TOKEN_LEFT_PAREN, "Expected '(' after identifier.");

	string name = PREV_TOKEN_STR;
	if(!check_function(name)) error_at(&tok, "Function does not exist in current scope.");

	vector<ExprNode*> args;
	FuncProperties funcprops = get_function_props(name);
	int paramscount = funcprops.params.size();
	
	if(!check(TOKEN_RIGHT_PAREN)) do
	{
		args.push_back(expression());
		if(!funcprops.variadic && args.size() > paramscount) break;
	
	} while(match(TOKEN_COMMA));

	if((funcprops.variadic && args.size() < paramscount)
	||(!funcprops.variadic && args.size() != paramscount))
	{
		HOLD_PANIC();
		error_at(&tok, tools::fstr("Expected %s%d argument%s, but %d were given.",
			funcprops.variadic ? "at least " : "", paramscount, paramscount == 1 ? "" : "s", args.size()));
		if(!PANIC_HELD) note_declaration("Function", name, &funcprops.token);
		return nullptr;
	}

	CONSUME_OR_RET_NULL(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");

	// _current_call_token = { TOKEN_ERROR, nullptr, nullptr, 0, 0, nullptr };

	vector<ParsedType*> lexparams; for(ParsedType*& p : funcprops.params) lexparams.push_back(p);
	return funcprops.invalid ? nullptr : new CallNode(tok, name, args, funcprops.ret_type, lexparams, paramscount);
}

// ======================= misc. =======================

Status Parser::parse(string infile, const char* source, AST* astree)
{
	// printTokensFromSrc(tools::readf(infile).c_str());
	_astree = astree;

	// set members
	_scanner = Scanner(source);

	_scope_stack = vector<Scope>();
	_current_scope = Scope{0, map<string, VarProperties>(), {}, map<string, FuncProperties>()};

	_had_error = false;
	_panic_mode = false;
	_error_dispatcher = ErrorDispatcher();

	// _current_call_token = { TOKEN_ERROR, nullptr, nullptr, 0, 0, nullptr };
	_main_file = infile;

	advance();
	while (!is_at_end())
	{
		if(match(TOKEN_MODULO))  _astree->push_back(variable_declaration());
		else if(match(TOKEN_AT)) _astree->push_back(function_declaration());
		else error_at_current("Expected declaration at top-level code.");

		if(_panic_mode) synchronize(true);
	}

	DEBUG_PRINT_MSG("Parsing complete!");
	return _had_error ? STATUS_PARSE_ERROR : STATUS_SUCCESS;
}