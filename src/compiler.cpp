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

void Compiler::errorAt(Token *token, string message)
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
	errorAt(&_previous, message);
}

// displays an error at the current token with the given message
void Compiler::errorAtCurrent(string message)
{
	//
	errorAt(&_current, message);
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

		errorAtCurrent(_current.start);
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

	errorAtCurrent(message);
}

// consumes a token corresponding with the given EviType.
// if it doesn't match an error is thrown using the given
// message as a format and the type as parameter
llvm::Constant* Compiler::consume_type(EviType type, string message_format, string typestr)
{
	// consumes and defines string 'token'
	#define CONSUME_AND_GET_STR_PART(toktype) \
		consume(toktype, tools::fstr(message_format, typestr)); \
		string token = string(_previous.start, _previous.length)

	// get value
	switch(type._initializer_type)
	{
		case INIT_INTEGER:
		{
			CONSUME_AND_GET_STR_PART(TOKEN_INTEGER);

			// get base
			int base = token.length() < 3 ? 10
				: token[1] == 'b' ? 2
				: token[1] == 'c' ? 8
				: token[1] == 'x' ? 16
				: 10;

			// scanner made sure that the token is valid
			// so no strol() should just work (if not decimal cut off the first two chars)
			int intval = strtol(token.c_str() + (base == 10 ? 0 : 2), NULL, base);

			// DEBUG_PRINT_VAR(base, %d);
			// DEBUG_PRINT_VAR(token.c_str() + (base == 10 ? 0 : 2), %s);
			// DEBUG_PRINT_VAR(intval, %d);
			return llvm::ConstantInt::get(type._llvm_type, intval, true);
		}
		default: return nullptr;
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

// ====================== grammar ======================



// ====================== parsing ======================

void Compiler::declaration()
{
	if (match(TOKEN_MODULO))
		variable_declaration();
	else
		advance();
	// 	errorAtCurrent(":(");
}

void Compiler::variable_declaration()
{
	// get name
	consume(TOKEN_IDENTIFIER, "Expected identifier after '%'.");
	string name = PREV_TOKEN_STR;

	// get type
	consume(TOKEN_TYPE, tools::fstr("Expected type after '%%%s'.", name.c_str()));
	string typestr = PREV_TOKEN_STR;
	if(!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	EviType type = GET_EVI_TYPE(typestr);

	// get initializer
	llvm::Constant* init_const = consume_type(type, "Invalid initializer for type %s.", typestr);

	// codegen
	if(_current_module == _top_module) // global
	{
		_top_module->getOrInsertGlobal(name, type._llvm_type);
		llvm::GlobalVariable* global_var = _top_module->getNamedGlobal(name);
		global_var->setLinkage(llvm::GlobalValue::CommonLinkage);
		global_var->setAlignment(llvm::MaybeAlign(type._alignment));
		global_var->setInitializer(init_const);
	}
	else // local
	{
		
	}
}

// ======================= misc. =======================

void Compiler::start()
{
	// init. top module
	_top_module = make_unique<llvm::Module>(LLVM_MODULE_TOP_NAME, __context);
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

	// printTokensFromSrc(_source.c_str());

	start();

	advance();
	while(!match(TOKEN_EOF))
		declaration();

	finish();

	write_to_file();
	return STATUS_SUCCESS;
}