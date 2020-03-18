#include "ptypes.h"
#include <stdio.h>
extern int yylineno, yycolumn;
extern struct Node* prog_root;
extern int yylex_destroy(void);
extern int yyparse();
extern void yyrestart(FILE* s);
extern void printParserTree(struct Node* node, int level);

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
        yylineno = 1, yycolumn = 1;
        yyparse();
        printParserTree(prog_root, 0);
        yylex_destroy();
        printf("\n\n");
    }
    return 0;
}