%{
// #include "symtab.h"
#include "lex.yy.c"

int errors;

int yylex();
void yyerror(const char *s);
struct Node* make_yylval(char* sname, int line, int num, ...);
void PrintParserTree(struct Node* node, int level);

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
%right NEG NOT
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
    ExtDefList { 
        if ($1->n_child == 0) {
            $$ = make_yylval("Program", yylineno, 1, $1);
        } else {
            $$ = make_yylval("Program", @$.first_line, 1, $1);
        }
        if (!errors) {
            /* CP1 */
            #ifdef DEBUG
            PrintParserTree($$, 0);
            #endif
            /* CP2 */
            SemanticAnalysis($$);
            /* CP3 */
            Translate($$);
        }
    }
;

ExtDefList:
    ExtDef ExtDefList {
        $$ = make_yylval("ExtDefList", @$.first_line, 2, $1, $2);
    }
|   %empty {
        $$ = make_yylval("ExtDefList", @$.first_line, 0); 
    }
;

ExtDef:
    Specifier ExtDecList SEMI {
        $$ = make_yylval("ExtDef", @$.first_line, 3, $1, $2, $3);
    }
|   Specifier SEMI {
        $$ = make_yylval("ExtDef", @$.first_line, 2, $1, $2); 
    }
|   Specifier FunDec CompSt {
        $$ = make_yylval("ExtDef", @$.first_line, 3, $1, $2, $3);
    }
|   Specifier FunDec SEMI {
        /* declare funciton in global field */
        $$ = make_yylval("ExtDef", @$.first_line, 3, $1, $2, $3);
    }
;

ExtDecList:
    VarDec {
        $$ = make_yylval("ExtDecList", @$.first_line, 1, $1);
    }
|   VarDec COMMA ExtDecList {
        $$ = make_yylval("ExtDecList", @$.first_line, 3, $1, $2, $3);
    }
|   error COMMA ExtDecList {
        $$ = make_yylval("ExtDecList", @$.first_line, 3, $1, $2, $3);
        yyerrok; errors++;
    }
;

// Specifiers
Specifier:
    TYPE {
        $$ = make_yylval("Specifier", @$.first_line, 1, $1);
    }
|   StructSpecifier {
        $$ = make_yylval("Specifier", @$.first_line, 1, $1);
    }
;

StructSpecifier:
    STRUCT OptTag LC DefList RC {
        $$ = make_yylval("StructSpecifier", @$.first_line, 5, $1, $2, $3, $4, $5);
    }
|   STRUCT OptTag LC error RC {
        $$ = make_yylval("StructSpecifier", @$.first_line, 5, $1, $2, $3, $4, $5);
        yyerrok; errors++;
    }
|   STRUCT Tag {
        $$ = make_yylval("StructSpecifier", @$.first_line, 2, $1, $2);
    }
;

OptTag:
    ID {
        $$ = make_yylval("OptTag", @$.first_line, 1, $1);
    }
|   %empty {
        $$ = make_yylval("OptTag", @$.first_line, 0);
    }
;

Tag:
    ID {
        $$ = make_yylval("Tag", @$.first_line, 1, $1);
    }
;

// Declarators
VarDec:
    ID {
        $$ = make_yylval("VarDec", @$.first_line, 1, $1);
    }
|   VarDec LB INT RB {
        $$ = make_yylval("VarDec", @$.first_line, 4, $1, $2, $3, $4);
    }
|   VarDec LB error RB {
        $$ = make_yylval("VarDec", @$.first_line, 4, $1, $2, $3, $4);
        yyerrok; errors++;
    }
;

FunDec:
    ID LP VarList RP {
        $$ = make_yylval("FunDec", @$.first_line, 4, $1, $2, $3, $4);
    }
|   ID LP error RP {
        $$ = make_yylval("FunDec", @$.first_line, 4, $1, $2, $3, $4); 
        yyerrok; errors++;
    }
|   ID LP RP {
        $$ = make_yylval("FunDec", @$.first_line, 3, $1, $2, $3);
    }
;

VarList:
    ParamDec COMMA VarList {
        $$ = make_yylval("VarList", @$.first_line, 3, $1, $2, $3);
    }
|   ParamDec {
        $$ = make_yylval("VarList", @$.first_line, 1, $1);
    }
;

ParamDec:
    Specifier VarDec {
        $$ = make_yylval("ParamDec", @$.first_line, 2, $1, $2);
    }
;

// Statements
CompSt:
    LC DefList StmtList RC {
        $$ = make_yylval("CompSt", @$.first_line, 4, $1, $2, $3, $4);
    }
|   LC DefList error RC { 
        $$ = make_yylval("CompSt", @$.first_line, 4, $1, $2, $3, $4);
        yyerrok; errors;
    }
;

StmtList:
    Stmt StmtList {
        $$ = make_yylval("StmtList", @$.first_line, 2, $1, $2);
    }
|   %empty {
        $$ = make_yylval("StmtList", @$.first_line, 0);
    }
;

Stmt:
    Exp SEMI {
        $$ = make_yylval("Stmt", @$.first_line, 2, $1, $2);
    }
|   CompSt {
        $$ = make_yylval("Stmt", @$.first_line, 1, $1);
    }
|   RETURN Exp SEMI {
        $$ = make_yylval("Stmt", @$.first_line, 3, $1, $2, $3);
    }
|   IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
        $$ = make_yylval("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5);
    }
|   IF LP Exp RP Stmt ELSE Stmt {
        $$ = make_yylval("Stmt", @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7);
    }
|   WHILE LP Exp RP Stmt {
        $$ = make_yylval("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5);
    }
|   IF LP error RP Stmt %prec LOWER_THAN_ELSE { 
        $$ = make_yylval("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); 
        yyerrok; errors++;
    }
|   IF LP error RP Stmt ELSE Stmt { 
        $$ = make_yylval("Stmt", @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); 
        yyerrok; errors++;
    }
|   WHILE LP error RP Stmt { 
        $$ = make_yylval("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); 
        yyerrok; errors++;
    }
|   error SEMI {
        $$ = make_yylval("Stmt", @$.first_line, 0);
        yyerrok; errors++;
    }
;

// Local Definitions
DefList:
    Def DefList {
        $$ = make_yylval("DefList", @$.first_line, 2, $1, $2);
    }
|   %empty {
        $$ = make_yylval("DefList", @$.first_line, 0);
    }
;

Def:
    Specifier DecList SEMI {
        $$ = make_yylval("Def", @$.first_line, 3, $1, $2, $3);
    }
;

DecList:
    Dec {
        $$ = make_yylval("DecList", @$.first_line, 1, $1);
    }
|   Dec COMMA DecList {
        $$ = make_yylval("DecList", @$.first_line, 3, $1, $2, $3);
    }
;

Dec:
    VarDec {
        $$ = make_yylval("Dec", @$.first_line, 1, $1);
    }
|   VarDec ASSIGNOP Exp {
        $$ = make_yylval("Dec", @$.first_line, 3, $1, $2, $3);
    }
;

// Expressions
Exp:
    /* 3 nodes */
    Exp ASSIGNOP Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp AND Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp OR Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp RELOP Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp PLUS Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp MINUS Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp STAR Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp DIV Exp {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   Exp DOT ID {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   LP Exp RP {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
|   ID LP RP {
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3);
    }
    /* 4 nodes */
|   ID LP Args RP {
        $$ = make_yylval("Exp", @$.first_line, 4, $1, $2, $3, $4);
    }
|   Exp LB Exp RB {
        $$ = make_yylval("Exp", @$.first_line, 4, $1, $2, $3, $4);
    }
    /* 2 nodes */
|   MINUS Exp %prec NEG {
        $$ = make_yylval("Exp", @$.first_line, 2, $1, $2);
    }
|   NOT Exp {
        $$ = make_yylval("Exp", @$.first_line, 2, $1, $2);
    }
    /* 1 nodes */
|   ID {
        $$ = make_yylval("Exp", @$.first_line, 1, $1);
    }
|   INT {
        $$ = make_yylval("Exp", @$.first_line, 1, $1);
    }
|   FLOAT {
        $$ = make_yylval("Exp", @$.first_line, 1, $1);
    }
    /* error production */
|   LP error RP{ 
        $$ = make_yylval("Exp", @$.first_line, 3, $1, $2, $3); 
        yyerrok; errors++;
    }
|   Exp LB error RB {
        $$ = make_yylval("Exp", @$.first_line, 4, $1, $2, $3, $4); 
        yyerrok; errors++;
    }
|   ID LP error RP { 
        $$ = make_yylval("Exp", @$.first_line, 4, $1, $2, $3, $4);
        yyerrok; errors++;
    }
;

Args:
    Exp COMMA Args {
        $$ = make_yylval("Args", @$.first_line, 3, $1, $2, $3);
    }
|   Exp {
        $$ = make_yylval("Args", @$.first_line, 1, $1);
    }      
;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error type B at Line %d: %s.\n", yylineno, s);
}

struct Node* make_yylval(char *sname, int line, int num, ...) {
    assert(num >= 0);

    struct Node* ret = malloc(sizeof(struct Node));
    ret->token = 0;
    ret->symbol = strdup(sname);
    ret->line = line;
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
    }
    va_end(valist);
    return ret;
}

void PrintParserTree(struct Node* node, int level)
{
    if (node->token == 0 && node->n_child == 0) return;
    printf("%*s", 2 * level, "");
    if (node->token == 0) {
        printf("%s", node->symbol);
        printf(" (%d)", node->line);
    } else {
        printf("%s", node->symbol);
        switch (node->token) {
        case ID:
        case TYPE:
            printf(": %s", node->ident);
            break;
        case INT:
            printf(": %d", node->ival);
            break;
        case FLOAT:
            printf(": %f", node->fval);
            break;
        default:
            break;
        }
    }
    printf("\n");
    for (int i = 0; i < node->n_child; i++) {
        // printf("%d %x %X %X\n", i, node, node->child[i], node->child[i]->child[i]);
        PrintParserTree(node->child[i], level + 1);
    }
}