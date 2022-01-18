#include "compiler.hpp"
#include "tools.hpp"

#include <cstdlib>

llvm::LLVMContext __context;
map<string, EviType> __evi_types;

void Compiler::configure(string infile, string outfile)
{
	_infile = infile;
	_outfile = outfile;
	_configured = true;
}

// ====================== errors =======================

void Compiler::error_at(Token *token, string message)
{
	// already in panicmode. swallow error.
	if (_panicMode)
		return;

	_panicMode = true;

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
	_hadError = true;
	exit(1);
}

// displays an error at the previous token with the given message
void Compiler::error(string message)
{
	//
	error_at(&_previous, message);
}

// displays an error at the current token with the given message
void Compiler::error_at_current(string message)
{
	//
	error_at(&_current, message);
}

// ====================== scanner ======================

// advances to the next token
void Compiler::advance()
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
bool Compiler::check(TokenType type)
{
	//
	return _current.type == type;
}

// consume the next token if it is of the correct type,
// otherwise throw an error with the given message
void Compiler::consume(TokenType type, string message)
{
	if (_current.type == type)
	{
		advance();
		return;
	}

	error_at_current(message);
}

// consumes a token corresponding with the given EviType.
// if it doesn't match an error is thrown using the given
// message as a format and the type as parameter
llvm::Constant *Compiler::consume_type(EviType type, string message_format, string typestr)
{
// consumes and defines string 'token'
#define CONSUME_AND_GET_STR_PART(toktype)                   \
	consume(toktype, tools::fstr(message_format, typestr)); \
	string token = string(_previous.start, _previous.length)

	// get value
	switch (type._initializer_type)
	{
	case INIT_INTEGER:
	{
		CONSUME_AND_GET_STR_PART(TOKEN_INTEGER);

		// get base
		int base = token.length() < 3 ? 10
				   : token[1] == 'b'  ? 2
				   : token[1] == 'c'  ? 8
				   : token[1] == 'x'  ? 16
									  : 10;

		// scanner made sure that the token is valid
		// so no strol() should just work (if not decimal cut off the first two chars)
		int intval = strtol(token.c_str() + (base == 10 ? 0 : 2), NULL, base);

		// DEBUG_PRINT_VAR(base, %d);
		// DEBUG_PRINT_VAR(token.c_str() + (base == 10 ? 0 : 2), %s);
		// DEBUG_PRINT_VAR(intval, %d);
		return llvm::ConstantInt::get(type._llvm_type, intval, true);
	}
	default:
		return nullptr;
	}

#undef CONSUME_AND_GET_STR_PART
}

// returns true and advances if the comp.current token is of the given type
bool Compiler::match(TokenType type)
{
	if (!check(type))
		return false;
	advance();
	return true;
}

// ======================= state =======================

// adds a local variable to the current scope
// if the variable already exists an error is thrown
void Compiler::add_local(Token *identtoken)
{
#define LOCALS _current_scope._locals

	string name = string(identtoken->start, identtoken->length);

	// try to find local
	bool found = false;

	if (find(LOCALS.begin(), LOCALS.end(), name) != LOCALS.end())
		found = true;

	for (auto scope = end(_scope_stack); !found && scope != begin(_scope_stack); --scope)
	{
		if (find((*scope)._locals.begin(), (*scope)._locals.end(), name) != (*scope)._locals.end())
		{
			found = true;
			break;
		}
	}

	if (found)
		error_at(identtoken, "Variable already exists in current scope.");
	else
		_current_scope._locals.push_back(name);

#undef LOCALS
}

// ====================== grammar ======================

Compiler::ParseRule Compiler::get_parse_rule(TokenType type)
{
	switch(type)
	{
		// Single-character tokens.
		// case TOKEN_LEFT_PAREN:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_RIGHT_PAREN:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_LEFT_BRACE:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_RIGHT_BRACE:		return {NULL,		NULL,		PREC_NONE};
		// TOKEN_LEFT_B_BRACE,
		// TOKEN_RIGHT_B_BRACE,
		// case TOKEN_SLASH:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_COMMA:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_BANG:				return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_QUESTION:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_COLON:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_NEWLINE:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_STAR:				return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_MODULO:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_DOLLAR:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_TILDE:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_CARET:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_AT:				return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_HASHTAG:			return {NULL,		NULL,		PREC_NONE};

		// One or two character tokens.
		case TOKEN_PLUS:			return {NULL,		binary,		PREC_TERM};
		// case TOKEN_PLUS_PLUS:		return {NULL,		NULL,		PREC_NONE};
		case TOKEN_MINUS:			return {NULL,		binary,		PREC_TERM};
		// case TOKEN_MINUS_MINUS:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_GREATER:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_GREATER_GREATER:	return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_LESS:				return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_LESS_LESS:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_EQUAL:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_EQUAL_EQUAL:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_AND:				return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_AND_AND:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_PIPE:				return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_PIPE_PIPE:		return {NULL,		NULL,		PREC_NONE};

		// Multi-character tokens
		// case TOKEN_SLASH_EQUAL:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_GREATER_EQUAL:	return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_LESS_EQUAL:		return {NULL,		NULL,		PREC_NONE};

		// Literals.
		// case TOKEN_IDENTIFIER:		return {NULL,		NULL,		PREC_NONE};
		case TOKEN_INTEGER:			return {literal,	NULL,		PREC_NONE};
		// case TOKEN_FLOAT:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_CHARACTER:		return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_STRING:			return {NULL,		NULL,		PREC_NONE};
		// case TOKEN_TYPE:				return {NULL,		NULL,		PREC_NONE};

		// misc.
		case TOKEN_ERROR:			return {NULL,		NULL,		PREC_NONE};
		case TOKEN_EOF:				return {NULL,		NULL,		PREC_NONE};

		default: return {NULL, NULL, PREC_NONE};
	}
}

void Compiler::parse_precedence(Precedence precedence)
{
	advance();

	ParseFn prefix_rule = get_parse_rule(_previous.type).prefix;

	if(prefix_rule == NULL)
	{
		error("Expected expression.");
		return;
	}

	// run prefix rule
	(this->*prefix_rule)();

	while(precedence <= get_parse_rule(_current.type).precedence)
	{
		ParseFn infix_rule = get_parse_rule(_previous.type).infix;
		(this->*infix_rule)();
	}
}

// ==================== expressions ====================

// parses an expression
void Compiler::expression()
{
	DEBUG_PRINT_MSG("started expression at line %d", _previous.line);

	parse_precedence(PREC_NONE);
	
	DEBUG_PRINT_MSG("expression done at line %d", _previous.line);
}

// parses a binary expression
void Compiler::binary()
{
	TokenType operator_type = _previous.type;
	ParseRule rule = get_parse_rule(operator_type);

	parse_precedence((Precedence)(rule.precedence + 1));


}

// ====================== parsing ======================

// parses a declaration
void Compiler::declaration()
{
	if (match(TOKEN_MODULO))
		variable_declaration();
	else
		advance();
	// 	errorAtCurrent(":(");
}

// parses a variable declaration
void Compiler::variable_declaration()
{
	// get name
	consume(TOKEN_IDENTIFIER, "Expected identifier after '%'.");
	string name = PREV_TOKEN_STR;
	add_local(&_previous);

	// get type
	consume(TOKEN_TYPE, tools::fstr("Expected type after '%%%s'.", name.c_str()));
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr))
		error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	EviType type = GET_EVI_TYPE(typestr);

	// get initializer
	// this can either be nothing, a constant-foldable constant or an expression
	// if it is nothing, we dont set the initializer
	// if it's a constant we set it
	// and if its an expression we add that to the global init. function


	// codegen
	if (_current_scope._depth == 0) // global
	{
		_top_module->getOrInsertGlobal(name, type._llvm_type);
		llvm::GlobalVariable *global_var = _top_module->getNamedGlobal(name);
		global_var->setLinkage(llvm::GlobalValue::CommonLinkage);
		global_var->setAlignment(llvm::MaybeAlign(type._alignment));
	}
	else // local
	{
		// TODO: implement this
	}
}

// ======================= misc. =======================

void Compiler::start()
{
	// init. top module
	_top_module = make_unique<llvm::Module>(LLVM_MODULE_TOP_NAME, __context);
	// _current_module = _top_module;

	// create global initializer function
	llvm::Function *global_init_func = llvm::getOrCreateInitFunction(*_top_module, "_global_var_init");
	_global_init_func_block = llvm::BasicBlock::Create(__context, "entry", global_init_func);
}

void Compiler::finish()
{
	//
	//
}

void Compiler::write_to_file()
{
	// ofstream std_file_stream(_outfile);
	ofstream std_file_stream("/dev/stdout");
	llvm::raw_os_ostream file_stream(std_file_stream);
	_top_module->print(file_stream, nullptr);
}

Status Compiler::compile()
{
	assert(_configured);

	// set members
	_source = tools::readf(_infile);
	_scanner = Scanner(_source.c_str());
	_hadError = false;
	_panicMode = false;

	_scope_stack = vector<Scope>();
	_current_scope = (Scope){0, vector<string>()};

	// printTokensFromSrc(_source.c_str());

	start();

	advance();
	while (!match(TOKEN_EOF))
		declaration();

	finish();

	write_to_file();
	return STATUS_SUCCESS;
}