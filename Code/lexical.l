%{
#include "syntax.tab.h"
#include <stdlib.h>
#include <string.h>
// #include "token.h"

// TODO() return token id with yyval
// #define TOKEN(t) (yylval.token = t)
#define TOKEN(t) (t)

int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

// #define TO_NUM(x) \
//     do { \
//         yylval.type_int = (int)strtol(yytext, &yytext, x); \
//     } while(0)

#define TO_NUM(x) \
    do { \
        yylval.type_int = yytext; \
    } while(0)

#define DISP_BUG(x) printf("%s %s\n", x, yytext)

void token_error(char* err_text);
%}

%option yylineno

_INT8               ([-+]?0[0-7]+)
_INT10              (([-+]?[1-9][0-9]*)|([-+]?0))
_INT16              ([-+]?0[x|X][0-9a-fA-F]+)
_FLOAT              ([-+]?[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?)
_ID                 ([_a-zA-Z][_a-zA-Z0-9]*)
_RELOP              (>=|<=|==|!=|>|<)
_DELIM              ([ \t\r])
_SINGLE_COMMENT     ("//"[^\n]*)

%%
[\n]                { yycolumn = 1;}
{_DELIM}            { /* ignore the extra delimiters */}
{_SINGLE_COMMENT}   { /* skip the comment */ }

{_RELOP}            {
    DISP_BUG("relop");
    return TOKEN(RELOP); 
}
"int"|"float"       {
    DISP_BUG("type");
    return TOKEN(TYPE); 
}
"struct"            {
    DISP_BUG("_struct");
    return TOKEN(STRUCT);
}
"return"            {
    DISP_BUG("return");
    return TOKEN(RETURN); 
}
"if"                {
    DISP_BUG("if");
    return TOKEN(IF);
}
"else"              {
    DISP_BUG("else");
    return TOKEN(ELSE);
}
"while"             {
    DISP_BUG("while");
    return TOKEN(WHILE); 
}
";"                 {
    DISP_BUG("semi");
    return TOKEN(SEMI);
}
","                 {
    DISP_BUG("comma");
    return TOKEN(COMMA);
}
"="                 {
    DISP_BUG("assignop");
    return TOKEN(ASSIGNOP);
}
"+"                 {
    DISP_BUG("plus");
    return TOKEN(PLUS);
}
"-"                 {
    DISP_BUG("minus");
    return TOKEN(MINUS);
}
"*"                 {
    DISP_BUG("star");
    return TOKEN(STAR);
}
"/"                 {
    DISP_BUG("div");
    return TOKEN(DIV);
}
"&&"                {
    DISP_BUG("and");
    return TOKEN(AND);
}
"||"                {
    DISP_BUG("or");
    return TOKEN(OR);
}
"\."                {
    DISP_BUG("dot");
    return TOKEN(DOT);
}
"!"                 {
    DISP_BUG("not");
    return TOKEN(NOT);
}
"("                 {
    DISP_BUG("lp");
    return TOKEN(LP);
}
")"                 {
    DISP_BUG("rp");
    return TOKEN(RP);
}
"["                 {
    DISP_BUG("lb");
    return TOKEN(LB);
}
"]"                 {
    DISP_BUG("rb");
    return TOKEN(RB);
}
"{"                 {
    DISP_BUG("lc");
    return TOKEN(LC);
}
"}"                 {
    DISP_BUG("rc");
    return TOKEN(RC);
}

{_ID}               {
    DISP_BUG("id");
    yylval.ident = strdup(yytext);
    return TOKEN(ID);
}
{_INT10}            {
    DISP_BUG("int10");
    TO_NUM(10);
    return TOKEN(INT);
}
{_INT8}             {
    DISP_BUG("int8");
    TO_NUM(8);
    return TOKEN(INT);
}
{_INT16}            {
    DISP_BUG("int16");
    TO_NUM(16);
    return TOKEN(INT);
}
{_FLOAT}            {
    DISP_BUG("_float");
    yylval.type_float = atof(yytext);
    return TOKEN(FLOAT);
}

.                   {
    DISP_BUG("other");
    token_error(yytext);
}

%%

void token_error(char* err_text)
{
    printf("\033[31mError \033[0mtype \033[34mA \033[0mat line \033[34m%d\033[0m: Mysterious character \'\033[35m%s\033[0m\'\n", yylineno, err_text);
}