#ifndef EVI_AST_H
#define EVI_AST_H

#include "common.hpp"
#include "phc.h"
#include "types.hpp"
#include "scanner.hpp"

// =================================================

// forward declarate nodes
class StmtNode;
	class FuncDeclNode;
	class VarDeclNode;
	class BlockNode;

	class ExprNode;
		class LogicalNode;
		class BinaryNode;
		class UnaryNode;
		class PrimaryNode;
			class LiteralNode;

// visitor class
class Visitor
{
	public:
	#define VISIT(_node) virtual void visit(_node* node) = 0
	VISIT(FuncDeclNode);
	VISIT(VarDeclNode);
	VISIT(BlockNode);
		VISIT(LogicalNode);
		VISIT(BinaryNode);
		VISIT(UnaryNode);
		VISIT(LiteralNode);
	#undef VISIT
};

// astnode class (visited by visitor)
class ASTNode
{
	public: 
	virtual void accept(Visitor* v) = 0;
};

// abstract syntax tree
typedef vector<StmtNode*> AST;

// =================================================

#define ACCEPT void accept(Visitor *v) { v->visit(this); }
#define VIRTUAL_ACCEPT virtual void accept(Visitor *v) = 0;

class StmtNode: public ASTNode { public: VIRTUAL_ACCEPT };

	class FuncDeclNode: public StmtNode
	{
		public:

		FuncDeclNode(string identifier, EviType type, StmtNode* body):
			_identifier(identifier), _type(type), _body(body) {}
		ACCEPT

		string _identifier;
		EviType _type;
		StmtNode* _body;
	};

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

	class BlockNode: public StmtNode
	{
		public:
		
		BlockNode(AST statements): _statements(statements) {}
		ACCEPT

		AST _statements;
	};

	class ExprNode: public ASTNode { public: VIRTUAL_ACCEPT };

		class LogicalNode: public ExprNode
		{
			public:

			LogicalNode(TokenType optype, ExprNode* left, ExprNode* right):
				_optype(optype), _left(left), _right(right) {}
			ACCEPT

			TokenType _optype;
			ExprNode* _left;
			ExprNode* _right;
		};

		class BinaryNode: public ExprNode
		{
			public:

			BinaryNode(TokenType optype, ExprNode* left, ExprNode* right):
				_optype(optype), _left(left), _right(right) {}
			ACCEPT

			TokenType _optype;
			ExprNode* _left;
			ExprNode* _right;
		};

		class UnaryNode: public ExprNode
		{
			public:

			UnaryNode(TokenType optype, ExprNode* expr):
				_optype(optype), _expr(expr) {}
			ACCEPT

			TokenType _optype;
			ExprNode* _expr;
		};

		class PrimaryNode: public ExprNode { public: VIRTUAL_ACCEPT };
		
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