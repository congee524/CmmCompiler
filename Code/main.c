#include "ptypes.h"
#include <stdio.h>

// #define DEBUG
// #define MUL_FILE

extern int yylineno, yycolumn, errors;
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
#ifdef MUL_FILE
        printf("\nFILE: %s\n", argv[i]);
#endif
        FILE* f = fopen(argv[i], "r");
        if (!f) {
            perror(argv[1]);
            return 1;
        }
        yyrestart(f);
        yylineno = 1, yycolumn = 1, errors = 0;
        ;
        yyparse();
        yylex_destroy();
    }
    return 0;
}