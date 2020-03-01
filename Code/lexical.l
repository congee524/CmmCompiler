%{
#include "token.h"
// #define TOKEN(t) (yylval.token = t)
// TODO() return value before with yyac
#define TOKEN(t) (t)

/* recording the current line number to report errors*/
int lines = 1;
void token_error(char* err_text, int line);
%}

_INT10              (([-+]?[1-9][0-9]*) | ([-+]?0))
_INT8               ([-+]?0[0-7]+)
_INT16              ([-+]?0[x|X][0-9a-fA-F]+)
_INT                ({_INT8}|{_INT10}|{_INT16})
_FLOAT              ([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)
_ID                 ([_a-zA-Z][_a-zA-Z0-9]*)
_RELOP              (>=|<=|==|!=|>|<)
_DELIM              ([ \t\r])

%%
[\n]                {lines++;}
{_DELIM}            {/*ignore the extra delimiters*/}
{_INT}              {return TOKEN(INT)}
{_FLOAT}            {return TOKEN(FLOAT)}
";"                 {return TOKEN(SEMI);}
","                 {return TOKEN(COMMA);}
{_RELOP}            {return TOKEN(RELOP);}
"="                 {return TOKEN(ASSIGNOP);}
"+"                 {return TOKEN(PLUS);}
"-"                 {return TOKEN(MINUS);}
"*"                 {return TOKEN(STAR);}
"/"                 {return TOKEN(DIV);}
"&&"                {return TOKEN(AND);}
"||"                {return TOKEN(OR);}
"\."                {return TOKEN(DOT);}
"!"                 {return TOKEN(NOT);}
"("                 {return TOKEN(LP);}
")"                 {return TOKEN(RP);}
"["                 {return TOKEN(LB);}
"]"                 {return TOKEN(RB);}
"{"                 {return TOKEN(LC);}
"}"                 {return TOKEN(RC);}
"int"|"float"       {return TOKEN(TYPE);}
"struct"            {return TOKEN(STRUCT);}
"return"            {return TOKEN(RETURN);}
"if"                {return TOKEN(IF);}
"else"              {return TOKEN(ELSE);}
"while"             {return TOKEN(WHILE);}
{_ID}               {return TOKEN(ID);}
.                   {token_error(yytext, lines);}

%%

void token_error(char* err_text, int line)
{
    printf("\033[31mError \033[0mtype \033[34mA \033[0mat line \033[34m%d\033[0m: Mysterious character \'\033[35m%s\033[0m\'\n", line, err_text);
}