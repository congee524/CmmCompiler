#ifndef SYMTAB_H__
#define SYMTAB_H__

#include <stdio.h>

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;

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