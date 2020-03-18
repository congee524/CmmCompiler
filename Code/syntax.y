%{
#include <stdarg.h>
#include <assert.h>
#include "lex.yy.c"
#include "ptypes.h"

struct Node* prog_root;

int yylex();
void yyerror(const char *s);
struct Node* make_yylval(char* sname, int num, ...);

void printParserTree(struct Node* node, int level);

%}

%locations

%define parse.error verbose
%define api.value.type {struct Node *}

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%token STRUCT TYPE IF ELSE WHILE RETURN
%token SEMI COMMA LC RC

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT

%token INT
%token FLOAT
%token ID

// %token <ival> INT
// %token <fval> FLOAT
// %token <ident> ID
// %type <dval> Exp
%start Program

%%

// High-level Definitions
Program:
    ExtDefList
        { $$ = make_yylval("Program", 1, $1); prog_root = $$;}
;

ExtDefList:
    ExtDef ExtDefList
        { $$ = make_yylval("ExtDefList", 2, $1, $2); }
|   /* empty */
        { $$ = make_yylval("ExtDefList", 0); }
;

ExtDef: 
    Specifier ExtDecList SEMI
        { $$ = make_yylval("ExtDef", 3, $1, $2, $3); }
|   Specifier SEMI
        { $$ = make_yylval("ExtDef", 2, $1, $2); }
|   Specifier FunDec CompSt
        { $$ = make_yylval("ExtDef", 3, $1, $2, $3); }
;

ExtDecList:
    VarDec
        { $$ = make_yylval("ExtDecList", 1, $1); }
|   VarDec COMMA ExtDecList
        { $$ = make_yylval("ExtDecList", 3, $1, $2, $3); }
;

// Specifiers
Specifier:
    TYPE
        { $$ = make_yylval("Specifier", 1, $1); }
|   StructSpecifier
        { $$ = make_yylval("Specifier", 1, $1); }
;

StructSpecifier:
    STRUCT OptTag LC DefList RC
        { $$ = make_yylval("StructSpecifier", 5, $1, $2, $3, $4, $5); }
|   STRUCT Tag
        { $$ = make_yylval("StructSpecifier", 2, $1, $2); }
;

OptTag:
    ID
        { $$ = make_yylval("OptTag", 1, $1); }
|   /* empty */
        { $$ = make_yylval("OptTag", 0); }
;

Tag:
    ID
        { $$ = make_yylval("Tag", 1, $1); }
;

// Declarators
VarDec:
    ID
        { $$ = make_yylval("VarDec", 1, $1); }
|   VarDec LB INT RB
        { $$ = make_yylval("VarDec", 4, $1, $2, $3, $4); }
;

FunDec:
    ID LP VarList RP
        { $$ = make_yylval("FunDec", 4, $1, $2, $3, $4); }
|   ID LP RP
        { $$ = make_yylval("FunDec", 3, $1, $2, $3); }
;

VarList:
    ParamDec COMMA VarList
        { $$ = make_yylval("VarList", 3, $1, $2, $3); }
|   ParamDec
        { $$ = make_yylval("VarList", 1, $1); }
;

ParamDec:
    Specifier VarDec
        { $$ = make_yylval("ParamDec", 2, $1, $2); }
;

CompSt:
    LC DefList StmtList RC
        { $$ = make_yylval("ParamDec", 4, $1, $2, $3, $4); }
;

StmtList:
    Stmt StmtList
        { $$ = make_yylval("StmtList", 2, $1, $2); }
|   /* empty */
        { $$ = make_yylval("StmtList", 0); }
;

Stmt:
    Exp SEMI
        { $$ = make_yylval("Stmt", 2, $1, $2); }
|   CompSt
        { $$ = make_yylval("Stmt", 1, $1); }
|   RETURN Exp SEMI
        { $$ = make_yylval("Stmt", 3, $1, $2, $3); }
|   IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
        { $$ = make_yylval("Stmt", 5, $1, $2, $3, $4, $5); }
|   IF LP Exp RP Stmt ELSE Stmt
        { $$ = make_yylval("Stmt", 7, $1, $2, $3, $4, $5, $6, $7); }
|   WHILE LP Exp RP Stmt
        { $$ = make_yylval("Stmt", 5, $1, $2, $3, $4, $5); }
;

// Local Definitions
DefList:
    Def DefList
        { $$ = make_yylval("DefList", 2, $1, $2); }
|   /* empty */
        { $$ = make_yylval("DefList", 0); }
;

Def:
    Specifier DecList SEMI
        { $$ = make_yylval("Def", 3, $1, $2, $3); }
;

DecList:
    Dec
        { $$ = make_yylval("DecList", 1, $1); }
|   Dec COMMA DecList
        { $$ = make_yylval("DecList", 3, $1, $2, $3); }
;

Dec:
    VarDec
        { $$ = make_yylval("Dec", 1, $1); }
|   VarDec ASSIGNOP Exp
        { $$ = make_yylval("Dec", 3, $1, $2, $3); }
;

// Expressions
Exp:
    Exp ASSIGNOP Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp AND Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp OR Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp RELOP Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp PLUS Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp MINUS Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp STAR Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp DIV Exp
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   LP Exp RP
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   MINUS Exp
        { $$ = make_yylval("Exp", 2, $1, $2); }
|   NOT Exp
        { $$ = make_yylval("Exp", 2, $1, $2); }
|   ID LP Args RP
        { $$ = make_yylval("Exp", 4, $1, $2, $3, $4); }
|   ID LP RP
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   Exp LB Exp RB
        { $$ = make_yylval("Exp", 4, $1, $2, $3, $4); }
|   Exp DOT ID
        { $$ = make_yylval("Exp", 3, $1, $2, $3); }
|   ID
        { $$ = make_yylval("Exp", 1, $1); }
|   INT
        { $$ = make_yylval("Exp", 1, $1); }
|   FLOAT
        { $$ = make_yylval("Exp", 1, $1); }
;

Args:
    Exp COMMA Args
        { $$ = make_yylval("Args", 3, $1, $2, $3); }
|   Exp
        { $$ = make_yylval("Args", 1, $1); }      
;

%%

void yyerror(const char *s) {
    fprintf(stderr, "\033[31mError \033[0mtype \033[34mB \033[0mat line \033[34m%d\033[0m: %s.\n", yylineno, s);
}

struct Node* make_yylval(char *sname, int num, ...) {
    assert(num >= 0);

    struct Node* ret = malloc(sizeof(struct Node));
    ret->token = 0;
    ret->symbol = strdup(sname);
    ret->line = yylineno;
    ret->n_child = num;
    if (num == 0) {
        ret->child = NULL;
        return ret;
    }

    ret->child = (struct Node**)malloc(num * sizeof(struct Node*));

    va_list valist;
    va_start(valist, num);
    for(int i = 0; i < num; i++) {
        ret->child[i] = va_arg(valist, struct Node*);
        // printf("%s check address %X %X\n", obj->symbol, obj, child[i]);
    }
    va_end(valist);
    return ret;
}

void printParserTree(struct Node* node, int level) {
    printf("%*s", 2 * level, "");
    if (node->token == 0) {
         printf("%s", node->symbol);
    } else {
        // TODO()
    }
    printf(" (%d)\n", node->line);
    for (int i = 0; i < node->n_child; i++) {
        // printf("%d %x %X %X\n", i, node, node->child[i], node->child[i]->child[i]);
        printParserTree(node->child[i], level + 1);
    }
}