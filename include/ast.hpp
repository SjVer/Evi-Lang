#ifndef EVI_AST_H
#define EVI_AST_H

#include "common.hpp"
#include "phc.h"
#include "types.hpp"

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

class StmtNode: public ASTNode 
	{ public: virtual void accept(Visitor* v) = 0; };

	class VarDeclNode: public StmtNode
	{
		public:

		VarDeclNode(string identifier, EviType type):
			_identifier(identifier), _type(type) {};
		void accept(Visitor *v) { v->visit(this); }

		string _identifier;
		EviType _type;
	};

class ExprNode: public ASTNode 
	{ public: virtual void accept(Visitor* v) = 0; };

	class LiteralNode: public StmtNode
	{
		public:

		LiteralNode() {};
		void accept(Visitor *v) { v->visit(this); }
	};

#endif