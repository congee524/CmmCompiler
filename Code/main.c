#include <stdio.h>

FILE *fin, *fout;
extern int yylineno;
extern int yylex_destroy(void);
extern int yyparse();
extern void yyrestart(FILE* s);

int main(int argc, char** argv)
{
    if (argc != 3) {
        fprintf(stdout, "input format: ./parser test1 out1.ir\n");
        return 1;
    }
    fin = fopen(argv[1], "r"), fout = fopen(argv[2], "w");
    if (!fin) {
        perror(argv[1]);
        return 1;
    }
    if (!fout) {
        perror(argv[2]);
        return 1;
    }
    yylineno = 1;
    yyrestart(fin);
    yyparse();
    yylex_destroy();
    return 0;
}