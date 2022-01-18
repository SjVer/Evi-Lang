#ifndef EVI_DEBUG_H
#define EVI_DEBUG_H

#include "expression.hpp"

void disassembleExpression(Expression *expr);
int disassembleInstruction(Expression *expr, int offset);

#endif