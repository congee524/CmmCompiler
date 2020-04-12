#ifndef SEMANTIC_H__
#define SEMANTIC_H__
#include "ptypes.h"
#include <assert.h>
#include <stdio.h>

void SemanticError(int error_num, int lineno, char* errtext);

void SemanticAnalysis(Node* root);

void ExtDefAnalysis(Node* root);

#endif