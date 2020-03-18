%{
#include "syntax.tab.h"
#include <stdlib.h>
#include <string.h>
#include "ptypes.h"
// #include "token.h"

// #define DEBUG

#define TOKEN(t) (yylval.token = t)

int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

#define TO_NUM(x) \
    do { \
        yylval.ival = (int)strtol(yytext, &yytext, x); \
    } while(0)

#ifdef DEBUG
    #define DISP_BUG(x) printf("%s %s\n", x, yytext)
#else
    #define DISP_BUG(x) {}
#endif

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
{_W_INT8}           {
    DISP_BUG("w_int8");
    token_error("Illegal octal number");
}
{_INT16}            {
    DISP_BUG("int16");
    TO_NUM(16);
    return TOKEN(INT);
}
{_W_INT16}          {
    DISP_BUG("w_int16");
    token_error("Illegal hexadecimal number");
}
{_FLOAT}            {
    DISP_BUG("_float");
    yylval.fval = atof(yytext);
    return TOKEN(FLOAT);
}
{_W_FLOAT}          {
    DISP_BUG("_w_float");
    token_error("Illegal floating point number");
}
{_ID}               {
    DISP_BUG("id");
    yylval.ident = (char *) strdup(yytext);
    return TOKEN(ID);
}
{_W_ID}             {
    DISP_BUG("w_id");
    token_error("Illegal identifier");
}

.                   {
    DISP_BUG("other");
    token_error("Mysterious character");
}

%%

void token_error(char* err_text)
{
    fprintf(stderr, "\033[31mError \033[0mtype \033[34mA \033[0mat line \033[34m%d\033[0m: %s \'\033[35m%s\033[0m\'.\n", yylineno, err_text, yytext);
}