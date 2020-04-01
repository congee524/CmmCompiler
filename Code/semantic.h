#ifndef SEMANTIC_H__
#define SEMANTIC_H__
#include "ptypes.h"

void semanticError(int error_num, int lineno, char* errtext);

void semanticAnalysis(Node* root);

#endif