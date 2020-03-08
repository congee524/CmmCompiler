#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc <= 1) {
        return 1;
    }
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    return 0;
}

/*
#include <stdio.h>

extern FILE* yyin;
extern char* yytext;
#define MY_DEFINE(R) #R
extern enum TokenType token;

static char* tokens[] = { "ID",
    "SEMI",
    "INT",
    "FLOAT",
    "COMMA",
    "RELOP",
    "ASSIGNOP",
    "PLUS",
    "MINUS",
    "STAR",
    "DIV",
    "AND",
    "OR",
    "DOT",
    "NOT",
    "TYPE",
    "LP",
    "RP",
    "LB",
    "RB",
    "LC",
    "RC",
    "STRUCT",
    "RETURN",
    "IF",
    "ELSE",
    "WHILE" };

int main(int argc, char** argv)
{
    int ret;
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }
    while (ret = yylex()) {
        if (ret >= 256) {
            printf("%s ", tokens[ret - 256]);
        } else {
            printf("%d ", ret);
        }
        printf("%s\n", yytext);
    }
    return 0;
}
*/