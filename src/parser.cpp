#include "parser.hpp"
#include "tools.hpp"

#include <cstdlib>

// ====================== errors =======================

void Parser::error_at(Token *token, string message)
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

// returns true and advances if the current token is of the given type
bool Parser::match(TokenType type)
{
	if (!check(type))
		return false;
	advance();
	return true;
}

// ======================= state =======================

// adds a local variable to the current scope
// if the variable already exists an error is thrown
void Parser::add_local(Token *identtoken)
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

// ===================== statements ====================

// parses a declaration
StmtNode Parser::declaration()
{
	if (match(TOKEN_MODULO))
		return variable_declaration();
	else
		advance();
	// 	errorAtCurrent(":(");
}

// parses a variable declaration
StmtNode Parser::variable_declaration()
{
	// get name
	consume(TOKEN_IDENTIFIER, "Expected identifier after '%'.");
	string name = PREV_TOKEN_STR;
	add_local(&_previous);

	// get type
	consume(TOKEN_TYPE, tools::fstr("Expected type after '%%%s'.", name.c_str()));
	string typestr = PREV_TOKEN_STR;
	if (!IS_EVI_TYPE(typestr)) error(tools::fstr("Invalid type: '%s'.", typestr.c_str()));
	EviType type = GET_EVI_TYPE(typestr);

	return VarDeclNode(name, type);
}

// ======================= misc. =======================

Status Parser::parse(string infile, AST* astree)
{
	_astree = astree;

	// set members
	_infile = infile;
	_source = tools::readf(_infile);
	_scanner = Scanner(_source.c_str());
	_hadError = false;
	_panicMode = false;

	_scope_stack = vector<Scope>();
	_current_scope = (Scope){0, vector<string>()};

	// printTokensFromSrc(_source.c_str());

	advance();
	while (!match(TOKEN_EOF))
		_astree->push_back(declaration());

	return STATUS_SUCCESS;
}