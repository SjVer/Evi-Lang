#ifndef EVI_AST_H
#define EVI_AST_H

#include "common.hpp"
#include "phc.h"
#include "types.hpp"
#include "scanner.hpp"

// =================================================

// forward declarate nodes
class StmtNode;
	class VarDeclNode;
class ExprNode;
	class LiteralNode;

// visitor class
class Visitor
{
	public:
	#define VISIT(node) virtual void visit(node*) = 0
	VISIT(VarDeclNode);
	VISIT(LiteralNode);
	#undef VISIT
};

// astnode class (visited by visitor)
class ASTNode
{
	public: 
	// virtual ~ASTNode() = 0;
	virtual void accept(Visitor* v) = 0;
};

// abstract syntax tree
typedef vector<StmtNode*> AST;

// =================================================

#define ACCEPT void accept(Visitor *v) { v->visit(this); }
#define VIRTUAL_ACCEPT virtual void accept(Visitor *v) = 0;

class StmtNode: public ASTNode { public: VIRTUAL_ACCEPT };

	class VarDeclNode: public StmtNode
	{
		public:

		VarDeclNode(string identifier, EviType type, ExprNode* expr):
			_identifier(identifier), _type(type), _expr(expr) {}
		ACCEPT

		string _identifier;
		EviType _type;
		ExprNode* _expr;
	};

class ExprNode: public ASTNode { public: VIRTUAL_ACCEPT };

	class PrimaryNode: public StmtNode { public: VIRTUAL_ACCEPT };
	
		// numbers and strings n shit
		class LiteralNode: public PrimaryNode
		{
			public:

			LiteralNode(string token, TokenType tokentype):
				_token(token), _tokentype(tokentype) {}
			ACCEPT

			string _token;
			TokenType _tokentype;
		};

#undef ACCEPT
#endif