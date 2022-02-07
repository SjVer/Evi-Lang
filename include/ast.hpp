#ifndef EVI_AST_H
#define EVI_AST_H

#include "common.hpp"
#include "pch.h"
#include "types.hpp"
#include "scanner.hpp"

// =================================================

// forward declarate nodes
class StmtNode;
	class FuncDeclNode;
	class VarDeclNode;
	class BlockNode;
	class AssignNode;
	class IfNode;
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
	VISIT(IfNode);
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
	ASTNode(Token token): _token(token) {}
	Token _token;
	LexicalType _cast_to;
	virtual void accept(Visitor* v) = 0;
};

// abstract syntax tree
typedef vector<StmtNode*> AST;

// =================================================

#define ACCEPT void accept(Visitor *v) { v->visit(this); }
#define VIRTUAL_NODE_DECLARATION(name, base) \
	class name : public base \
	{ public: name(Token token): base(token) {} virtual void accept(Visitor *v) = 0; }

VIRTUAL_NODE_DECLARATION(StmtNode, ASTNode);

	class FuncDeclNode: public StmtNode
	{
		public:

		// body = nullptr if only declaration
		FuncDeclNode(Token token, string identifier, EviType* ret_type,
					 vector<EviType*> params, StmtNode* body):
			StmtNode(token), _identifier(identifier), 
			_ret_type(ret_type), _params(params), _body(body) {}
		ACCEPT

		string _identifier;
		EviType* _ret_type;
		vector<EviType*> _params;
		StmtNode* _body; // nullptr if only declared
	};

	class VarDeclNode: public StmtNode
	{
		public:

		// expr = nullptr if only declaration
		VarDeclNode(Token token, string identifier,
					EviType* type, ExprNode* expr, bool is_global):
			StmtNode(token), _identifier(identifier),
			_type(type), _expr(expr), _is_global(is_global) {}
		ACCEPT

		string _identifier;
		EviType* _type;
		ExprNode* _expr;
		bool _is_global;
	};

	class AssignNode: public StmtNode
	{
		public:

		AssignNode(Token token, string ident, ExprNode* expr, LexicalType expected_type):
			StmtNode(token), _ident(ident), _expr(expr), _expected_type(expected_type) {}
		ACCEPT

		string _ident;
		ExprNode* _expr;
		LexicalType _expected_type;
	};

	class IfNode: public StmtNode
	{
		public:

		IfNode(Token token, ExprNode* cond, StmtNode* then, StmtNode* else_):
			StmtNode(token), _cond(cond), 
			_then(then), _else(else_) {}
		ACCEPT

		ExprNode* _cond;
		StmtNode* _then;
		StmtNode* _else;
	};

	class LoopNode: public StmtNode
	{
		public:

		LoopNode(Token token,
				 StmtNode* init, ExprNode* cond,
				 StmtNode* incr, StmtNode* body):
			StmtNode(token), _init(init), _cond(cond),
			_incr(incr), _body(body) {}
		ACCEPT

		StmtNode* _init;
		ExprNode* _cond;
		StmtNode* _incr;
		StmtNode* _body;
	};

	class ReturnNode: public StmtNode
	{
		public:

		ReturnNode(Token token, ExprNode* expr, EviType* expected_type):
			StmtNode(token), _expr(expr), _expected_type(expected_type) {}
		ACCEPT

		ExprNode* _expr;
		EviType* _expected_type;
	};

	class BlockNode: public StmtNode
	{
		public:
		
		// if a block is "secret" it's basically
		// just a collection of statements that belong
		// to the same context rather than a new one
		BlockNode(Token token, AST statements, bool secret = false):
			 StmtNode(token), _statements(statements), _secret(secret) {}
		ACCEPT

		AST _statements;
		bool _secret;
	};

	VIRTUAL_NODE_DECLARATION(ExprNode, StmtNode);

		class LogicalNode: public ExprNode
		{
			public:

			LogicalNode(Token token, ExprNode* left, 
						ExprNode* right, ExprNode* middle = nullptr):
				ExprNode(token), _left(left), 
				_right(right), _middle(middle) {}
			ACCEPT

			ExprNode* _left;
			ExprNode* _right;
			ExprNode* _middle;
		};

		class BinaryNode: public ExprNode
		{
			public:

			BinaryNode(Token token, TokenType optype, ExprNode* left, ExprNode* right):
				ExprNode(token), _optype(optype), _left(left), _right(right) {}
			ACCEPT

			TokenType _optype;
			ExprNode* _left;
			ExprNode* _right;
		};

		class UnaryNode: public ExprNode
		{
			public:

			UnaryNode(Token token, TokenType optype, ExprNode* expr):
				ExprNode(token), _optype(optype), _expr(expr) {}
			ACCEPT

			TokenType _optype;
			ExprNode* _expr;
		};

		class GroupingNode: public ExprNode
		{
			public:

			GroupingNode(Token token, ExprNode* expr): 
				ExprNode(token), _expr(expr) {}
			ACCEPT

			ExprNode* _expr;
		};

		VIRTUAL_NODE_DECLARATION(PrimaryNode, ExprNode);
		
			class LiteralNode: public PrimaryNode
			{
				public:

				LiteralNode(Token token, long value): PrimaryNode(token) { _int_value = value; }
				LiteralNode(Token token, double value): PrimaryNode(token) { _float_value = value; }
				LiteralNode(Token token, char value): PrimaryNode(token) { _char_value = value; }
				LiteralNode(Token token, string value): PrimaryNode(token) { _string_value = value; }
				ACCEPT

				long _int_value;
				double _float_value;
				char _char_value;
				string _string_value;
			};

			class ReferenceNode: public PrimaryNode
			{
				public:

				ReferenceNode(Token token, string var, int par, LexicalType type):
					PrimaryNode(token), _variable(var), _parameter(par), _type(type) {}
				ACCEPT

				string _variable;
				int _parameter;
				LexicalType _type;
			};

			class CallNode: public PrimaryNode
			{
				public:

				CallNode(Token token, string ident, vector<ExprNode*> arguments,
						 LexicalType ret_t_type, vector<LexicalType> expected_arg_types):
					PrimaryNode(token), _ident(ident), _arguments(arguments),
					_ret_type(ret_t_type), _expected_arg_types(expected_arg_types) {}
				ACCEPT

				string _ident;
				vector<ExprNode*> _arguments;
				LexicalType _ret_type;
				vector<LexicalType> _expected_arg_types;
			};

#undef ACCEPT
#endif