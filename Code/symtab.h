#ifndef SYMTAB_H__
#define SYMTAB_H__

#define _POSIX_C_SOURCE 200809L
#include "ptypes.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SymTable_* SymTable;
typedef struct Type_* Type;
typedef struct FieldList_* FieldList;

typedef struct Node {
    int token;
    char* symbol;
    int line;
    int n_child;
    struct Node** child;
    union {
        int ival;
        float fval;
        double dval;
        char* ident;
    };
} Node;

/* implement symtable by hash table with list */
struct SymTable_ {
    char* name;
    Type type;
    SymTable next;
};

extern SymTable symtable[0x3fff + 1];

struct Type_ {
    enum { BASIC,
        ARRAY,
        STRUCTURE } kind;
    union {
        int basic; /* 0-int 1-float */
        struct {
            Type elem;
            int size;
        } array;
        FieldList structure;
        // struct {
        //     char* struct_name;
        //     FieldList structure;
        // };
    } u;
};

struct FieldList_ {
    char* name;
    Type type;
    FieldList next;
};

void SemanticError(int error_num, int lineno, char* errtext);

void SemanticAnalysis(Node* root);

void ExtDefAnalysis(Node* root);

Type SpecAnalysis(Node* spec);

Type StructSpecAnalysis(Node* struct_spec);

char* TraceVarDec(Node* var_dec, int* dim, int* size);

Type ConstArray(Type fund, int dim, int* size, int level);

SymTable InsertSymTab(char* type_name, Type type);

#endif