%{
#include "syntax.tab.h"
#include <stdlib.h>
#include <string.h>
#include "ptypes.h"
// #include "token.h"

// #define DEBUG

#define TOKEN(t) (yylval->token = t)

int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

#define TO_NUM(x) \
    do { \
        yylval->ival = (int)strtol(yytext, &yytext, x); \
    } while(0)

void init_yylval(char* sname);

void token_error(char* err_text);
%}

%x MUL_COMMENT
%option yylineno

_INT8               (0[0-7]+)
_INT10              ([1-9][0-9]*)|0
_INT16              (0[x|X][0-9a-fA-F]+)
_W_INT8             (0[0-9]+)
_W_INT16            (0[x|X][0-9a-zA-Z]*)
_FLOAT              ([0-9]+\.[0-9]+([eE][-+]?[0-9]+)?)
_W_FLOAT            ([0-9]+\.[0-9]+([eE][-+]?[0-9]?)?)

_ID                 ([_a-zA-Z][_a-zA-Z0-9]*)
_W_ID               ([_a-zA-Z0-9]+)
_RELOP              (>=|<=|==|!=|>|<)

_DELIM              ([ \t\r])
_SINGLE_COMMENT     ("//"[^\n]*)

%%
[\n]                { yycolumn = 1;}
{_DELIM}            { /* ignore the extra delimiters */}
{_SINGLE_COMMENT}   { /* skip the comment */ }
"/*"                { BEGIN(MUL_COMMENT); }
<MUL_COMMENT>"*/"   { BEGIN(INITIAL); }
<MUL_COMMENT>\n     { /* yylineno will auto increase */ }
<MUL_COMMENT>.      { /* skip the comment */ }

{_RELOP}            {
    init_yylval("RELOP");
    return TOKEN(RELOP); 
}
"int"|"float"       {
    init_yylval("TYPE");
    yylval->ident = strdup(yytext);
    return TOKEN(TYPE); 
}
"struct"            {
    init_yylval("STRUCT");
    return TOKEN(STRUCT);
}
"return"            {
    init_yylval("RETURN");
    return TOKEN(RETURN); 
}
"if"                {
    init_yylval("IF");
    return TOKEN(IF);
}
"else"              {
    init_yylval("ELSE");
    return TOKEN(ELSE);
}
"while"             {
    init_yylval("WHILE");
    return TOKEN(WHILE); 
}
";"                 {
    init_yylval("SEMI");
    return TOKEN(SEMI);
}
","                 {
    init_yylval("COMMA");
    return TOKEN(COMMA);
}
"="                 {
    init_yylval("ASSIGNOP");
    return TOKEN(ASSIGNOP);
}
"+"                 {
    init_yylval("PLUS");
    return TOKEN(PLUS);
}
"-"                 {
    init_yylval("MINUS");
    return TOKEN(MINUS);
}
"*"                 {
    init_yylval("STAR");
    return TOKEN(STAR);
}
"/"                 {
    init_yylval("DIV");
    return TOKEN(DIV);
}
"&&"                {
    init_yylval("AND");
    return TOKEN(AND);
}
"||"                {
    init_yylval("OR");
    return TOKEN(OR);
}
"\."                {
    init_yylval("DOT");
    return TOKEN(DOT);
}
"!"                 {
    init_yylval("NOT");
    return TOKEN(NOT);
}
"("                 {
    init_yylval("LP");
    return TOKEN(LP);
}
")"                 {
    init_yylval("RP");
    return TOKEN(RP);
}
"["                 {
    init_yylval("LB");
    return TOKEN(LB);
}
"]"                 {
    init_yylval("RB");
    return TOKEN(RB);
}
"{"                 {
    init_yylval("LC");
    return TOKEN(LC);
}
"}"                 {
    init_yylval("RC");
    return TOKEN(RC);
}

{_INT10}            {
    init_yylval("INT");
    TO_NUM(10);
    return TOKEN(INT);
}
{_INT8}             {
    init_yylval("INT");
    TO_NUM(8);
    return TOKEN(INT);
}
{_W_INT8}           {
    token_error("Illegal octal number");
}
{_INT16}            {
    init_yylval("INT");
    TO_NUM(16);
    return TOKEN(INT);
}
{_W_INT16}          {
    token_error("Illegal hexadecimal number");
}
{_FLOAT}            {
    init_yylval("FLOAT");
    yylval->fval = atof(yytext);
    return TOKEN(FLOAT);
}
{_W_FLOAT}          {
    token_error("Illegal floating point number");
}
{_ID}               {
    init_yylval("ID");
    yylval->ident = (char *) strdup(yytext);
    return TOKEN(ID);
}
{_W_ID}             {
    token_error("Illegal identifier");
}

.                   {
    token_error("Mysterious character");
}

%%

void init_yylval(char* sname) {
    yylval = malloc(sizeof(struct Node));
    yylval->symbol = strdup(sname);
    yylval->line = yylineno;
    yylval->n_child = 0;
    yylval->child = NULL;
#ifdef DEBUG
    printf("%s %s\n", sname, yytext);
#endif
}

void token_error(char* err_text)
{
    fprintf(stderr, "\033[31mError \033[0mtype \033[34mA \033[0mat line \033[34m%d\033[0m: %s \'\033[35m%s\033[0m\'.\n", yylineno, err_text, yytext);
}