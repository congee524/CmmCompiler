#ifndef SYMTAB_H__
#define SYMTAB_H__

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DEBUG

#ifdef DEBUG
#define INFO(msg)                                         \
    do {                                                  \
        fprintf(stderr, "info: %s:", __FILE__);           \
        fprintf(stderr, "[%s] %d: ", __func__, __LINE__); \
        fprintf(stderr, "%s\n", msg);                     \
    } while (0)
#else
#define INFO(msg) \
    do {          \
    } while (0)
#endif

#define TODO()                                            \
    do {                                                  \
        fprintf(stderr, "unfinished at %s:", __FILE__);   \
        fprintf(stderr, "[%s] %d: ", __func__, __LINE__); \
        assert(0);                                        \
    } while (0);

typedef struct ExpNode_* ExpNode;
typedef struct SymTable_* SymTable;
typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct FuncTable_* FuncTable;
typedef struct SymTableNode_* SymTableNode;

struct ExpNode_ {
    int isRight;
    double val;
    Type type;
};

typedef struct Node {
    int token;
    char* symbol;
    int line;
    int n_child;
    struct Node** child;
    union {
        int ival;
        float fval;
        char* ident;
        ExpNode eval;
    };
} Node;

/* implement symtable by hash table with list */
struct SymTable_ {
    char* name;
    Type type;
    SymTable next;
};

struct FuncTable_ {
    char* name;
    int lineno;
    Type ret_type;
    FieldList para;
    int isDefined;
    FuncTable next;
};

/* use list instead of hash map, since we must check all the function whether been defined */

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
        struct {
            FieldList structure;
            char* struct_name;
        };
    } u;
};

struct FieldList_ {
    char* name;
    int lineno;
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

extern SymTable symtable[0x3fff + 1];
extern FuncTable FuncHead;
extern struct SymTabStack_ symtabstack;

void SemanticAnalysis(Node* root);

void ExtDefAnalysis(Node* root);

void CompSTAnalysis(Node* root, FuncTable func);

void DefListAnalysis(Node* def_list);

void DefAnalysis(Node* def);

void DecListAnalysis(Node* root, Type type);

void StmtListAnalysis(Node* stmt_list, FuncTable func);

void StmtAnalysis(Node* stmt, FuncTable func);

void ExpAnalysis(Node* exp);

void ExtDecListAnalysis(Node* root, Type type);

FuncTable FunDecAnalysis(Node* root, Type type);

FieldList VarListAnalysis(Node* var_list);

FieldList ParamDecAnalysis(Node* param);

FieldList ArgsAnalysis(Node* args);

Type SpecAnalysis(Node* spec);

Type StructSpecAnalysis(Node* struct_spec);

char* TraceVarDec(Node* var_dec, int* dim, int* size, int* m_size);

Type ConstArray(Type fund, int dim, int* size, int level);

FieldList FetchStructField(Node* def_list);

FieldList ConstFieldFromDecList(Node* dec_List, Type type);

void CreateLocalVar();

void DeleteLocalVar();

int AddSymTab(char* type_name, Type type, int lineno);

FuncTable AddFuncTab(FuncTable func, int isDefined);

Type LookupTab(char* name);

int CheckSymTab(char* sym_name, Type type, int lineno);

int CheckFuncTab(FuncTable func, int isDefined);

Type CheckStructField(FieldList structure, char* name);

void CheckFuncDefined();

int CheckTypeEql(Type t1, Type t2);

int CheckFieldEql(FieldList f1, FieldList f2);

Type CheckFuncCall(char* func_name, FieldList para, int lineno);

int CheckLogicOPE(Node* exp);

int CheckArithOPE(Node* obj1, Node* obj2);

void InitSA();

#endif