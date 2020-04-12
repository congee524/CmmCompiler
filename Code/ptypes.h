#ifndef _PTYPES_H__
#define _PTYPES_H__

typedef struct Node {
    int token;
    char* symbol;
    int line;
    int n_child;
    struct Node** child;
    union {
        int ival;
        float fval;
        double dval;
        char* ident;
    };
} Node;

// struct YYSTYPE {
//     int token;
//     char* symbol;
//     int line;
//     int n_child;
//     struct YYSTYPE** child;
//     union {
//         int ival;
//         float fval;
//         double dval;
//         char* ident;
//     };
// };

#endif