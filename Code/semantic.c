#include "semantic.h"

#define TODO()                       \
    {                                \
        printf("unfinished part\n"); \
        assert(0);                   \
    }

void SemanticError(int error_num, int lineno, char* errtext)
{
    fprintf(stderr, "Error type %d at Line %d: %s.", error_num, lineno, errtext);
}

void SemanticAnalysis(Node* root)
{
    assert(root != NULL);
    Node* tmp = NULL;
    for (int i = 0; i < root->n_child; i++) {
        tmp = root->child[i];
        if (!strcmp(tmp->symbol, "ExtDefList")) {
            SemanticAnalysis(tmp);
        } else if (!strcmp(tmp->symbol, "ExtDef")) {
            ExtDefAnalysis(tmp);
        } else {
            assert(0);
        }
    }
}

void ExtDefAnalysis(Node* root)
{
    assert(root != NULL);
    assert(root->n_child >= 2);
    Node* tmp = root->child[1];
    if (!strcmp(tmp->symbol, "ExtDecList")) {
        /* Specifier ExtDecList SEMI */
        TODO()
    } else if (!strcmp(tmp->symbol, "SEMI")) {
        TODO()
    } else if (!strcmp(tmp->symbol, "FunDec")) {
        if (!strcmp(root->child[2]->symbol, "CompSt")) {
            TODO()
        } else if (!strcmp(root->child[2]->symbol, "SEMI")) {
            TODO()
        } else {
            assert(0);
        }
    } else {
        assert(0);
    }
}