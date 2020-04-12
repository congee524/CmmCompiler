#include "symtab.h"

static unsigned int hash(char* name)
{
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~0x3fff) {
            val = (val ^ (i >> 22)) & 0x3fff;
        }
    }
    return val;
}

Type GetType(Node* spec)
{
    assert(spec != NULL);
    assert(strcmp(spec->symbol, "Specifier") == 0);
    assert(spec->n_child == 1);

    Node* tmp = spec->child[0];
    if (!strcmp(tmp->symbol, "TYPE")) {
        TODO()
    } else if (!strcmp(tmp->symbol, "StructSpecifier")) {
        TODO()
    } else {
        assert(0);
    }
}