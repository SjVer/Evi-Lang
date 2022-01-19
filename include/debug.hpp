#ifndef EVI_DEBUG_H
#define EVI_DEBUG_H

#include "ast.hpp"
#include <string>

using namespace std;

class ASTVisualizer: public Visitor
{
	sstream _stream;
	int _nodecount;
	void visualize(string path, AST* astree);
}

#define HEADER "digraph astgraph {\n\
	node [shape=box, fontsize=12, fontname=\"Courier\", height=.1];\n\
	ranksep=.3;\n\
	edge [arrowsize=.5]\n\
	\n\
	"
#define FOOTER "}"

#endif