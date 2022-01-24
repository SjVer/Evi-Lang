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
	class AssignNode;
	class LoopNode;
	class ReturnNode;

	class ExprNode;
		class LogicalNode;
		class BinaryNode;
		class UnaryNode;
		class GroupingNode;
		class PrimaryNode;
			class LiteralNode;
			class ReferenceNode;
			class CallNode;

// visitor class
class Visitor
{
	public:
	#define VISIT(_node) virtual void visit(_node* node) = 0
	VISIT(FuncDeclNode);
	VISIT(VarDeclNode);
	VISIT(AssignNode);
	VISIT(LoopNode);
	VISIT(ReturnNode);
	VISIT(BlockNode);
	
		VISIT(LogicalNode);
		VISIT(BinaryNode);
		VISIT(UnaryNode);
		VISIT(GroupingNode);
			VISIT(LiteralNode);
			VISIT(ReferenceNode);
			VISIT(CallNode);
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

		// body = nullptr if only declaration
		FuncDeclNode(string identifier, EviType ret_type, vector<EviType> params, StmtNode* body):
			_identifier(identifier), _ret_type(ret_type), _params(params), _body(body) {}
		ACCEPT

		string _identifier;
		EviType _ret_type;
		vector<EviType> _params;
		StmtNode* _body; // nullptr if only declared
	};

	class VarDeclNode: public StmtNode
	{
		public:

		// expr = nullptr if only declaration
		VarDeclNode(string identifier, EviType type, ExprNode* expr):
			_identifier(identifier), _type(type), _expr(expr) {}
		ACCEPT

		string _identifier;
		EviType _type;
		ExprNode* _expr;
	};

	class AssignNode: public StmtNode
	{
		public:

		AssignNode(string ident, ExprNode* expr):
			_ident(ident), _expr(expr) {}
		ACCEPT

		string _ident;
		ExprNode* _expr;
	};

	class LoopNode: public StmtNode
	{
		public:

		LoopNode(StmtNode* first, ExprNode* second,
				 StmtNode* third, StmtNode* body):
			_first(first), _second(second),
			_third(third), _body(body) {}
		ACCEPT

		StmtNode* _first;
		ExprNode* _second;
		StmtNode* _third;
		StmtNode* _body;
	};

	class ReturnNode: public StmtNode
	{
		public:

		ReturnNode(ExprNode* expr): _expr(expr) {}
		ACCEPT

		ExprNode* _expr;
	};

	class BlockNode: public StmtNode
	{
		public:
		
		// if a block is "secret" it's basically
		// just a collection of statements that belong
		// to the same context rather than a new one
		BlockNode(AST statements, bool secret = false):
			 _statements(statements), _secret(secret) {}
		ACCEPT

		AST _statements;
		bool _secret;
	};

	class ExprNode: public StmtNode { public: VIRTUAL_ACCEPT };

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

		class GroupingNode: public ExprNode
		{
			public:

			GroupingNode(ExprNode* expr): _expr(expr) {}
			ACCEPT

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

			class ReferenceNode: public PrimaryNode
			{
				public:

				ReferenceNode(string var, int par, TokenType type):
					_variable(var), _parameter(par), _type(type) {}
				ACCEPT

				string _variable;
				int _parameter;
				TokenType _type;
			};

			class CallNode: public PrimaryNode
			{
				public:

				CallNode(string ident, vector<ExprNode*> arguments):
					_ident(ident), _arguments(arguments) {}
				ACCEPT

				string _ident;
				vector<ExprNode*> _arguments;

			};


#undef ACCEPT
#endif