#ifndef SYMTAB_H__
#define SYMTAB_H__

#include <stdio.h>

typedef struct SymTable_ SymTable;
typedef struct Type_* Type;
typedef struct FieldList_* FieldList;

/* implement symtable by hash table with list */
struct SymTable_ {
    char* name;
    Type type;
    SymTable next;
};

SymTable symtable[0x3fff + 1];

struct Type_ {
    enum { BASIC,
        ARRAY,
        STRUCT } kind;
    union {
        int basic;
        struct {
            Type elem;
            int size;
        } array;
        FieldList structure;
    };
};

struct FieldList_ {
    char* name;
    Type type;
    FieldList next;
};

#endif