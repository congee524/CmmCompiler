#include "symtab.h"

void semanticError(int error_num, int lineno, char* errtext)
{
    fprintf(stderr, "Error type %d at Line %d: %s.", error_num, lineno, errtext);
}