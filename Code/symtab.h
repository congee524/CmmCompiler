#ifndef SYMTAB_H__
#define SYMTAB_H__

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INFO(msg)                                         \
    do {                                                  \
        fprintf(stderr, "info: %s:", __FILE__);           \
        fprintf(stderr, "[%s] %d: ", __func__, __LINE__); \
        fprintf(stderr, "%s\n", msg);                     \
    } while (0)

#define TODO()                                            \
    do {                                                  \
        fprintf(stderr, "unfinished at %s:", __FILE__);   \
        fprintf(stderr, "[%s] %d: ", __func__, __LINE__); \
        assert(0);                                        \
    } while (0);

typedef struct SymTable_* SymTable;
typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct FuncTable_* FuncTable;
typedef struct SymTableNode_* SymTableNode;

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

struct FuncTable_ {
    char* name;
    int lineno;
    Type ret_type;
    FieldList para;
    int isDefined;
    FuncTable next;
};

/* use list instead of hash map, since we must check all the function whether been defined */
extern FuncTable FuncHead;

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
    } u;
};

struct FieldList_ {
    char* name;
    Type type;
    FieldList next;
};

struct SymTableNode_ {
    SymTable var;
    SymTableNode next;
};

struct SymTabStack_ {
    int depth;
    SymTableNode StructHead;
    SymTableNode var_stack[256];
};

extern struct SymTabStack_ symtabstack;

void SemanticError(int error_num, int lineno, char* errtext);

void SemanticAnalysis(Node* root);

void ExtDefAnalysis(Node* root);

void ExtDecListAnalysis(Node* root, Type type);

Type SpecAnalysis(Node* spec);

Type StructSpecAnalysis(Node* struct_spec);

char* TraceVarDec(Node* var_dec, int* dim, int* size);

Type ConstArray(Type fund, int dim, int* size, int level);

FuncTable FunDecAnalysis(Node* root, Type type);

FieldList VarListAnalysis(Node* var_list);

FieldList ParamDecAnalysis(Node* param);

int AddSymTab(char* type_name, Type type, int lineno);

int AddFuncTab(FuncTable func, int isDefined);

int CheckSymTab(char* type_name, Type type, int lineno);

int CheckFuncTab(FuncTable func, int isDefined);

void CheckFuncDefined();

int CheckTypeEql(Type t1, Type t2);

int CheckFieldEql(FieldList f1, FieldList f2);

void InitProg();

#endif