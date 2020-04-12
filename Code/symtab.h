#ifndef SYMTAB_H__
#define SYMTAB_H__

#include "ptypes.h"
#include <assert.h>
#include <stdio.h>

typedef struct SymTable_ SymTable;
typedef struct Type_* Type;
typedef struct FieldList_* FieldList;

/* implement symtable by hash table with list */
struct SymTable_ {
    char* name;
    Type type;
    SymTable next;
};

SymTable symtable[0x3fff + 1];

struct Type_ {
    enum { BASIC,
        ARRAY,
        STRUCTURE } kind;
    union {
        enum { INT,
            FLOAT } basic;
        struct {
            Type elem;
            int size;
        } array;
        FieldList structure;
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

Type GetType(Node* spec);

FieldList StructSpecAnalysis(Node* struct_spec);

char* TraceVarDec(Node* var_dec, int* dim, int* size);

Type ConstArray(Type fund, int dim, int* size, int level);

#endif