#ifndef EVI_AST_H
#define EVI_AST_H

#include "phc.h"
#include "types.hpp"

// =================================================

// forward declarate nodes
class StmtNode;
	class VarDeclNode;
// class ExprNode;
// class LiteralNode;

// visitor class
class Visitor
{
	public:
	#define VISIT(node) virtual void visit(node*) const = 0
	VISIT(VarDeclNode);
	#undef VISIT
};

// astnode class (visited by visitor)
class ASTNode
{
	public: 
	// accept visitor
	virtual void accept(Visitor *v) const = 0;
};

// abstract syntax tree
typedef vector<StmtNode> AST;

// =================================================

#define ACCEPT() void accept(Visitor *v) { v->visit(this); }

class StmtNode: public ASTNode { ACCEPT(); };

	class VarDeclNode: public StmtNode
	{
		public:
		ACCEPT();
		VarDeclNode(string ident, EviType type):
			_identifier(ident), _type(type) {}; 
		private:
		string _identifier;
		EviType _type;
		// ExprNode initializer;
	};

#undef ACCEPT
#endif