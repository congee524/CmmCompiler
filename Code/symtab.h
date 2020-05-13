#ifndef SYMTAB_H__
#define SYMTAB_H__

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG
// #define LOCAL_SYM

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

typedef struct Operand_* Operand;
typedef struct ArgList_* ArgList;
typedef struct InterCode_* InterCode;
typedef struct InterCodes_* InterCodes;

unsigned int hash(char* name);

/*============= semantic =============*/

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
    Operand op_var;
    int reg_no;
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
    FieldList StructHead;
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

int AddStructList(Type structure, int lineno);

Type LookupTab(char* name);

Type GetStruct(char* name);

int GetSize(Type type);

int CheckSymTab(char* sym_name, Type type, int lineno);

int CheckFuncTab(FuncTable func, int isDefined);

int CheckStructName(char* name);

Type CheckStructField(FieldList structure, char* name);

void CheckFuncDefined();

int CheckTypeEql(Type t1, Type t2);

int CheckFieldEql(FieldList f1, FieldList f2);

Type CheckFuncCall(char* func_name, FieldList para, int lineno);

int CheckLogicOPE(Node* exp);

int CheckArithOPE(Node* obj1, Node* obj2);

void InitSA();

/*============= translate =============*/

struct Operand_ {
    enum {
        OP_TEMP = 0,
        OP_VAR,
        OP_CONST_INT,
        OP_CONST_FLOAT,
        op_ADDR,
        OP_LABEL,
        OP_FUNC
    } kind;
    union {
        int var_no;
        int value;
        float fval;
        char* id;
    } u;
};

struct ArgList_ {
    Operand arg;
    struct ArgList_* next;
    struct ArgList_* prev;
};

typedef enum RELOP_TYPE {
    GEQ = 0, /* >= */
    LEQ, /* <= */
    GE, /* > */
    LE, /* < */
    EQ, /* == */
    NEQ /* != */
} RELOP_TYPE;

struct InterCode_ {
    enum {
        I_LABEL = 0, /* LABEL x: */
        I_FUNC, /* FUNCTION f: */
        I_ASSIGN, /* x := y */
        I_ADD, /* x := y + z */
        I_SUB, /* x := y - z */
        I_MUL, /* x := y * z */
        I_DIV, /* x := y / z */
        I_ADDR, /* x := &y */
        I_DEREF_R, /* x := *y */
        I_DEREF_L, /* *x := y */
        I_JMP, /* GOTO x */
        I_JMP_IF, /* IF x [relop] y GOTO z */
        I_RETURN, /* RETURN x */
        I_DEC, /* DEC x [size] */
        I_ARG, /* ARG x */
        I_CALL, /* x := CALL f */
        I_PARAM, /* PARAM x */
        I_READ, /* READ x */
        I_WRITE /* WRITE */
    } kind;
    union {
        struct {
            Operand x;
        } label, jmp, ret, arg, param, read, write;
        struct {
            Operand f;
        } func;
        struct {
            Operand x, y;
        } assign, addr, defer_r, defer_l;
        struct {
            Operand x, f;
        } call;
        struct {
            Operand x, y, z;
        } add, sub, mul, div;
        struct {
            int size;
            Operand x;
        } dec;
        struct {
            RELOP_TYPE relop;
            Operand x, y, z;
        } jmp_if;
    } u;
};

struct InterCodes_ {
    InterCode data;
    struct InterCodes_* prev;
    struct InterCodes_* next;
};

extern InterCodes CodeHead;

InterCodes Translate(Node* root);

InterCodes TranslateExtDef(Node* ext_def);

InterCodes TranslateFunDec(Node* fun_dec);

InterCodes TranslateVarList(Node* var_list);

InterCodes TranslateParamDec(Node* param_dec);

InterCodes TranslateCompSt(Node* comp_st);

InterCodes TranslateDefList(Node* def_list);

InterCodes TranslateDecList(Node* dec_list);

InterCodes TranslateDec(Node* dec);

InterCodes TranslateStmtList(Node* stmt_list);

InterCodes TranslateStmt(Node* stmt);

InterCodes TranslateExp(Node* exp, Operand place);

InterCodes TranslateCond(Node* exp, Operand label_true, Operand label_false);

Operand LookupOpe(char* name);

InterCodes JointCodes(InterCodes code1, InterCodes code2);

InterCodes MakeInterCodesNode();

RELOP_TYPE GetRelop(Node* relop);

InterCodes GotoCode(Operand label);

InterCodes LabelCode(Operand label);

Operand NewTemp();

Operand NewLabel()();

#endif