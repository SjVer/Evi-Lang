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
abstract class Visitor
{
public:
	abstract void visit(VarDeclNode*);
	// abstract void visit(ExprNode*);
	// abstract void visit(LiteralNode*);
}

// astnode class (visited by visitor)
abstract class ASTNode
{
public: 
	// accpet visitor
	void accept(Visitor *visitor) { v->visit(this); }
};

// abstract syntax tree
typedef AST vector<StmtNode>;

// =================================================

class StmtNode: public ASTNode;

	class VarDeclNode: public StmtNode
	{
		VarDeclNode(string ident, EviType type):
			_identifier(ident), _type(type) {}; 
		string _identifier;
		EviType _type;
		// ExprNode initializer;
	}

#endif