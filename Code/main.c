#include <stdio.h>
extern int yyparse();
extern void yyrestart(FILE* s);

int main(int argc, char** argv)
{
    if (argc <= 1) {
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        printf("FILE: %s\n", argv[i]);
        FILE* f = fopen(argv[i], "r");
        if (!f) {
            perror(argv[1]);
            return 1;
        }
        yyrestart(f);
        yyparse();
        printf("\n\n");
    }
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