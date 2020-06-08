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
#define INFO(msg)                                     \
  do {                                                \
    fprintf(stderr, "info: %s:", __FILE__);           \
    fprintf(stderr, "[%s] %d: ", __func__, __LINE__); \
    fprintf(stderr, "%s\n", msg);                     \
  } while (0)
#else
#define INFO(msg) \
  do {            \
  } while (0)
#endif

#define TODO()                                        \
  do {                                                \
    fprintf(stderr, "unfinished at %s:", __FILE__);   \
    fprintf(stderr, "[%s] %d: ", __func__, __LINE__); \
    assert(0);                                        \
  } while (0);

typedef struct ExpNode_ *ExpNode;
typedef struct SymTable_ *SymTable;
typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;
typedef struct FuncTable_ *FuncTable;
typedef struct SymTableNode_ *SymTableNode;

typedef struct Operand_ *Operand;
typedef struct ArgList_ *ArgList;
typedef struct InterCode_ *InterCode;
typedef struct InterCodes_ *InterCodes;

struct ExpNode_ {
  int isRight;
  double val;
  Type type;
};

typedef struct Node {
  int token;
  char *symbol;
  int line;
  int n_child;
  struct Node **child;
  union {
    int ival;
    float fval;
    char *ident;
    ExpNode eval;
  };
} Node;

/* implement symtable by hash table with list */
struct SymTable_ {
  char *name;
  Type type;
  int isParam;
  Operand op_var;
  SymTable next;
};

struct FuncTable_ {
  char *name;
  int lineno;
  Type ret_type;
  FieldList para;
  Operand op_func;
  int isDefined;
  FuncTable next;
};

/* use list instead of hash map, since we must check all the function whether
 * been defined */

struct Type_ {
  enum { BASIC, ARRAY, STRUCTURE } kind;
  union {
    int basic; /* 0-int 1-float */
    struct {
      Type elem;
      int size;
    } array;
    struct {
      FieldList structure;
      char *struct_name;
    };
  } u;
};

struct FieldList_ {
  char *name;
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

struct Operand_ {
  enum {
    OP_TEMP = 0,
    OP_VAR,
    OP_CONST_INT,
    OP_CONST_FLOAT,
    OP_ADDR,
    OP_LABEL,
    OP_FUNC
  } kind;
  union {
    int var_no;
    int value;
    float fval;
    char *id;
  } u;
  int ref_cnt;
};

struct ArgList_ {
  Operand arg;
  struct ArgList_ *next;
  struct ArgList_ *prev;
};

typedef enum RELOP_TYPE {
  GEQ = 0, /* >= */
  LEQ,     /* <= */
  GE,      /* > */
  LE,      /* < */
  EQ,      /* == */
  NEQ      /* != */
} RELOP_TYPE;

typedef enum IC_TYPE {
  I_LABEL = 0, /* LABEL x: */
  I_FUNC,      /* FUNCTION f: */
  I_ASSIGN,    /* x := y */
  I_ADD,       /* x := y + z */
  I_SUB,       /* x := y - z */
  I_MUL,       /* x := y * z */
  I_DIV,       /* x := y / z */
  I_ADDR,      /* x := &y */
  I_DEREF_R,   /* x := *y */
  I_DEREF_L,   /* *x := y */
  I_JMP,       /* GOTO x */
  I_JMP_IF,    /* IF x [relop] y GOTO z */
  I_RETURN,    /* RETURN x */
  I_DEC,       /* DEC x [size] */
  I_ARG,       /* ARG x */
  I_CALL,      /* x := CALL f */
  I_PARAM,     /* PARAM x */
  I_READ,      /* READ x */
  I_WRITE,     /* WRITE x */
  I_TEMP
} IC_TYPE;

struct InterCode_ {
  IC_TYPE kind;
  union {
    struct {
      Operand x;
    } label, jmp, ret, arg, param, read, write, single;
    struct {
      Operand f;
    } func;
    struct {
      Operand x, y;
    } assign, addr, deref_r, deref_l, unary;
    struct {
      Operand x, f;
    } call;
    struct {
      Operand x, y, z;
    } add, sub, mul, div, binop;
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
  struct InterCodes_ *prev;
  struct InterCodes_ *next;
};

extern FILE *fin, *fout;
extern SymTable symtable[0x3fff + 1];
extern FuncTable FuncHead;
extern struct SymTabStack_ symtabstack;
extern InterCodes CodeHead;

/*============= semantic =============*/

void SemanticAnalysis(Node *root);

void ExtDefAnalysis(Node *root);

void CompSTAnalysis(Node *root, FuncTable func);

void DefListAnalysis(Node *def_list);

void DefAnalysis(Node *def);

void DecListAnalysis(Node *root, Type type);

void StmtListAnalysis(Node *stmt_list, FuncTable func);

void StmtAnalysis(Node *stmt, FuncTable func);

void ExpAnalysis(Node *exp);

void ExtDecListAnalysis(Node *root, Type type);

FuncTable FunDecAnalysis(Node *root, Type type);

FieldList VarListAnalysis(Node *var_list);

FieldList ParamDecAnalysis(Node *param);

FieldList ArgsAnalysis(Node *args);

Type SpecAnalysis(Node *spec);

Type StructSpecAnalysis(Node *struct_spec);

void InitSA();

/*============= translate =============*/

InterCodes Translate(Node *root);

InterCodes TranslateExtDef(Node *ext_def);

InterCodes TranslateFunDec(Node *fun_dec);

InterCodes TranslateVarList(Node *var_list);

InterCodes TranslateParamDec(Node *param_dec);

InterCodes TranslateCompSt(Node *comp_st);

InterCodes TranslateDefList(Node *def_list);

InterCodes TranslateDecList(Node *dec_list);

InterCodes TranslateDec(Node *dec);

InterCodes TranslateStmtList(Node *stmt_list);

InterCodes TranslateStmt(Node *stmt);

InterCodes TranslateExp(Node *exp, Operand place);

InterCodes TranslateCond(Node *exp, Operand label_true, Operand label_false);

InterCodes TranslateArgs(Node *args, ArgList arg_list);

/*============= helper =============*/
void SemanticError(int error_num, int lineno, char *errtext);

char *TraceVarDec(Node *var_dec, int *dim, int *size, int *m_size);

Type ConstArray(Type fund, int dim, int *size, int level);

FieldList FetchStructField(Node *def_list);

FieldList ConstFieldFromDecList(Node *dec_List, Type type);

void CreateLocalVar();

void DeleteLocalVar();

SymTable AddSymTab(char *type_name, Type type, int lineno);

FuncTable AddFuncTab(FuncTable func, int isDefined);

int AddStructList(Type structure, int lineno);

Type LookupTab(char *name);

Type GetStruct(char *name);

int GetSize(Type type);

int GetStructMemOff(Type type, char *name);

int CheckSymTab(char *sym_name, Type type, int lineno);

int CheckFuncTab(FuncTable func, int isDefined);

int CheckStructName(char *name);

Type CheckStructField(FieldList structure, char *name);

void CheckFuncDefined();

int CheckTypeEql(Type t1, Type t2);

int CheckFieldEql(FieldList f1, FieldList f2);

Type CheckFuncCall(char *func_name, FieldList para, int lineno);

int CheckLogicOPE(Node *exp);

int CheckArithOPE(Node *obj1, Node *obj2);

InterCodes SimplifyPlace(InterCodes code, Operand place, int isVar);

void AddArgs(ArgList arg_list, Operand t1);

Operand LookupOpe(char *name);

Type GetNearestType(Node *exp);

InterCodes GetAddr(Node *exp, Operand addr);

InterCodes JointCodes(InterCodes code1, InterCodes code2);

InterCodes MakeICNode(IC_TYPE kind, ...);

RELOP_TYPE GetRelop(Node *relop);

InterCodes GotoCode(Operand label);

InterCodes LabelCode(Operand label);

Operand NewTemp();

Operand NewLabel();

Operand NewConstInt(int ival);

Operand NewConstFloat(float fval);

char *OpeName(Operand ope);

char *RelopName(RELOP_TYPE relop);

void DeleteIRNode(InterCodes to_del);

InterCodes RemoveNull(InterCodes root);

void CalRefCnt(InterCodes root);

InterCodes SimplifyAssign(InterCodes to_simp);

InterCodes RemoveTempVar(InterCodes root);

InterCodes OptimIRCode(InterCodes root);

void IRGen(InterCodes root);

#endif