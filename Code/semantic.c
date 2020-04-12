#include "symtab.h"

#define TODO()                       \
    {                                \
        printf("unfinished part\n"); \
        assert(0);                   \
    }

static void SemanticError(int error_num, int lineno, char* errtext)
{
    fprintf(stderr, "Error type %d at Line %d: %s.", error_num, lineno, errtext);
}

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

void SemanticAnalysis(Node* root)
{
    assert(root != NULL);
    Node* tmp = NULL;
    for (int i = 0; i < root->n_child; i++) {
        tmp = root->child[i];
        if (!strcmp(tmp->symbol, "ExtDefList")) {
            SemanticAnalysis(tmp);
        } else if (!strcmp(tmp->symbol, "ExtDef")) {
            ExtDefAnalysis(tmp);
        } else {
            assert(0);
        }
    }
}

void ExtDefAnalysis(Node* root)
{
    assert(root != NULL);
    assert(root->n_child >= 2);
    Node* tmp = root->child[1];
    if (!strcmp(tmp->symbol, "ExtDecList")) {
        /* Specifier ExtDecList SEMI */
        TODO()
    } else if (!strcmp(tmp->symbol, "SEMI")) {
        TODO()
    } else if (!strcmp(tmp->symbol, "FunDec")) {
        if (!strcmp(root->child[2]->symbol, "CompSt")) {
            TODO()
        } else if (!strcmp(root->child[2]->symbol, "SEMI")) {
            TODO()
        } else {
            assert(0);
        }
    } else {
        assert(0);
    }
}

Type SpecAnalysis(Node* spec)
{
    assert(spec != NULL);
    assert(strcmp(spec->symbol, "Specifier") == 0);
    assert(spec->n_child == 1);

    Type ret;
    Node* tmp = spec->child[0];

    if (!strcmp(tmp->symbol, "TYPE")) {
        ret = (Type)malloc(sizeof(struct Type_));
        ret->kind = BASIC;
        if (!strcmp(tmp->ident, "int")) {
            ret->u.basic = INT;
        } else if (!strcmp(tmp->ident, "float")) {
            ret->u.basic = FLOAT;
        } else {
            assert(0);
        }
    } else if (!strcmp(tmp->symbol, "StructSpecifier")) {
        ret = StructSpecAnalysis(tmp);
        // ret->kind = STRUCTURE;
        // int isDefine = 0;
        // ret->u.structure = StructSpecAnalysis(tmp, &isDefine);
        /* allow for empty structure */
    } else {
        assert(0);
    }
    return ret;
}

Type StructSpecAnalysis(Node* struct_spec)
{
    /* if return NULL then error happen or empty struture */
    assert(struct_spec != NULL);
    assert(structure->n_child >= 2);

    Type ret = (Type)malloc(sizeof(struct Type_));
    Node* tag = struct_spec->child[1];
    ret->kind = STRUCTURE;
    ret->u.structure = NULL;
    unsigned int tag_idx;

    if (!strcmp(tag->symbol, "Tag")) {
        /* declare structure variable */
        tag_idx = hash(tag->child[0]->ident);
        if (symtable[tag_idx] != NULL) {
            SymTable temp = symtable[tag_idx];
            while (temp) {
                if (!strcmp(temp->name, tag->child[0]->ident && temp->type->kind != STRUCTURE)) {
                    /* duplicated structure name */
                    char msg[128]; /* the length of ID is less than 32 */
                    sprintf(msg, "Duplicated structure name \"%s\"", tag->child[0]->ident);
                    SemanticError(16, tag->line, msg);
                    return ret;
                } else if (!strcmp(temp->name, tag->child[0]->ident) {
                    if (ret->u.structure != NULL) {
                        /* shouldn't arrive here 
                           it means there are multiple the same structure
                           but the error should be found in def struture!
                           Note that structure shouldn't be defined in funciton 
                        */
                        assert(0);
                    }
                    ret->u.structure = temp->type->u.structure;
                    // ret->u.struct_name = strdup(tag->child[0]->ident);
                }
                temp = temp->next;
            }
        }
        if (ret->u.structure == NULL) {
            /* hasn't been defined */
            char msg[128];
            sprintf(msg, "Undefined structure \"%s\"", tag->child[0]->ident);
            SemanticError(17, tag->line, msg);
        }
    } else if (!strcmp(tag->symbol, "OptTag")) {
        /* define structure */
        char struct_name[32];
        if (tag->n_child == 0) {
            /* Anonymous structure */
            sprintf(struct_name, "anon_struture@%d_", tag->line);
        } else {
            strcpy(struct_name, tag->child[0]->ident);
        }
        tag_idx = hash(struct_name);

        if (symtable[tag_idx] != NULL) {
            SymTable temp = symtable[tag_idx];
            while (temp) {
                if (!strcmp(temp->name, struct_name)) {
                    /* duplicated structure name */
                    char msg[128]; /* the length of ID is less than 32 */
                    sprintf(msg, "Duplicated structure name \"%s\"", struct_name);
                    SemanticError(16, tag->line, msg);
                    return ret;
                }
                temp = temp->next;
            }
        }

        /* there not exist duplicated name, begin define */
        // ret->u.struct_name = strdup(struct_name);
        FieldList cur;
        Node* def_list = struct_spec->child[3];
        if (def_list->n_child == 2) {
            /* not empty structure */
            ret->u.structure = (FieldList)malloc(sizeof(struct FieldList_));
            ret->u.structure->next = NULL;
            FieldList cur = ret->u.structure;
        }
        while (def_list->n_child == 2) {
            Node* def = def_list->child[0];
            Type cur_type = SpecAnalysis(def->child[0]);
            Node* dec_list = def->child[1];
            do {
                Node* dec = dec_list->child[0];
                if (dec->n_child == 3) {
                    /* initial the variable in struture */
                    char msg[128];
                    sprintf(msg, "Initialize the domain of the structure");
                    SemanticError(15, dec->lin, msg);
                }
                /* add the domain regardless of initialization */

                Node* var_dec = dec->child[0];
                int dim = 0, size[256];
                char* var_name = TraceVarDec(dec->child[0], &dim, size);
                /* check whether duplicated field exist */
                FieldList check_field = ret->u.structure;
                int flag = 1;
                while (check_field) {
                    if (!strcmp(check_field, var_name)) {
                        flag = 0;
                        break;
                    }
                    check_field = check_field->next;
                }

                if (!flag) {
                    char msg[128];
                    sprintf(msg, "Redefined field \"%s\"", var_name);
                    SemanticError(15, dec->child[0]->line, msg);
                } else {
                    cur->name = strdup(var_name);
                    cur->type = ConstArray(cur_type, dim, size, 0);
                }
                if (dec_list->n_child == 3) {
                    dec_list = dec_list->child[2];
                    cur->next = (FieldList)malloc(sizeof(struct FieldList_));
                    cur->next->next = NULL;
                    cur = cur->next;
                } else {
                    dec_list = NULL;
                }
            } while (dec_list);

            def_list = def_list->child[1];
            if (def_list->n_child == 2) {
                cur->next = (FieldList)malloc(sizeof(struct FieldList_));
                cur = cur->next;
                cur->next = NULL;
            } else {
                break;
            }
        }
        /* define struct, need to add to symtab */
        InsertSymTab(struct_name, ret);
    } else {
        assert(0);
    }
    return ret;
}

char* TraceVarDec(Node* var_dec, int* dim, int* size)
{
    /* to get array or ID */
    char* ret;
    if (var_dec->n_child == 1) {
        ret = var_dec->child[0]->ident;
    } else if (var_dec->n_child == 4) {
        /* support max 256 dimension */
        if (dim >= 256) {
            fprintf(stderrr, "support maximum 256 dimension!\n");
            return NULL;
        }
        size[dim] = var_dec->child[2]->ival;
        *dim = *dim + 1;
        ret = TraceVarDec(var_dec->child[0], dim, size);
    } else {
        assert(0);
    }
    return ret;
}

Type ConstArray(Type fund, int dim, int* size, int level)
{
    if (level == dim) {
        return fund;
    }
    Type ret = (Type)malloc(sizeof(struct Type_));
    ret->kind = ARRAY;
    ret->u.array.elem = ConstArray(fund, dim, size, level + 1);
    ret->u.array.size = size[dim - level - 1];
    return ret;
}

SymTable InsertSymTab(char* type_name, Type type)
{
    unsigned int idx = hash(type_name);

    SymTable ret = NULL;

    // don't allow duplicated name TODO()
    ret = (SymTable)malloc(sizeof(struct SymTable_));
    ret->name = strdup(type_name);
    ret->Type = type;
    ret->next = symtable[tag_idx];
    symtable[idx] = ret;

    return ret;
}

// FieldList StructSpecAnalysis(Node* struct_spec, int* isDefine)
// {
//     /* if return NULL then error happen or empty struture */
//     assert(struct_spec != NULL);
//     assert(structure->n_child >= 2);
//     Node* tag = struct_spec->child[1];
//     FieldList ret = NULL;
//     unsigned int tag_idx;

//     if (!strcmp(tag->symbol, "Tag")) {
//         /* declare structure variable */
//         tag_idx = hash(tag->child[0]->ident);
//         if (symtable[tag_idx] != NULL) {
//             SymTable temp = symtable[tag_idx];
//             while (temp) {
//                 if (!strcmp(temp->name, tag->child[0]->ident && temp->type->kind != STRUCTURE)) {
//                     /* duplicated structure name */
//                     char msg[128]; /* the length of ID is less than 32 */
//                     sprintf(msg, "Duplicated structure name \"%s\"", tag->child[0]->ident);
//                     SemanticError(16, tag->line, msg);
//                     return ret;
//                 } else if (!strcmp(temp->name, tag->child[0]->ident) {
//                     if (ret != NULL) {
//                         /* shouldn't arrive here
//                            it means there are multiple the same structure
//                            but the error should be found in def struture!
//                            Note that structure shouldn't be defined in funciton
//                         */
//                         assert(0);
//                     }
//                     ret = temp->type->u.structure;
//                 }
//                 temp = temp->next;
//             }
//         }
//         if (ret == NULL) {
//             /* hasn't been defined */
//             char msg[128];
//             sprintf(msg, "Undefined structure \"%s\"", tag->child[0]->ident);
//             SemanticError(17, tag->line, msg);
//         }
//     } else if (!strcmp(tag->symbol, "OptTag")) {
//         /* define structure */
//         char struct_name[32];
//         if (tag->n_child == 0) {
//             /* Anonymous structure */
//             sprintf(struct_name, "anon_struture@%d_", tag->line);
//         } else {
//             strcpy(struct_name, tag->child[0]->ident);
//         }
//         tag_idx = hash(struct_name);

//         if (symtable[tag_idx] != NULL) {
//             SymTable temp = symtable[tag_idx];
//             while (temp) {
//                 if (!strcmp(temp->name, struct_name)) {
//                     /* duplicated structure name */
//                     char msg[128]; /* the length of ID is less than 32 */
//                     sprintf(msg, "Duplicated structure name \"%s\"", struct_name);
//                     SemanticError(16, tag->line, msg);
//                     return ret;
//                 }
//                 temp = temp->next;
//             }
//         }

//         /* there not exist duplicated name, begin define */
//         FieldList cur;
//         Node* def_list = struct_spec->child[3];
//         if (def_list->n_child == 2) {
//             /* not empty structure */
//             ret = (FieldList)malloc(sizeof(struct FieldList_));
//             ret->next = NULL;
//             FieldList cur = ret;
//         }
//         while (def_list->n_child == 2) {
//             Node* def = def_list->child[0];
//             Type cur_type = SpecAnalysis(def->child[0]);
//             Node* dec_list = def->child[1];
//             do {
//                 Node* dec = dec_list->child[0];
//                 if (dec->n_child == 3) {
//                     /* initial the variable in struture */
//                     char msg[128];
//                     sprintf(msg, "Initialize the domain of the structure");
//                     SemanticError(15, dec->lin, msg);
//                 }
//                 /* add the domain regardless of initialization */

//                 Node* var_dec = dec->child[0];
//                 int dim = 0, size[256];
//                 char* var_name = TraceVarDec(dec->child[0], &dim, size);
//                 /* check whether duplicated field exist */
//                 FieldList check_field = ret;
//                 int flag = 1;
//                 while (check_field) {
//                     if (!strcmp(check_field, var_name)) {
//                         flag = 0;
//                         break;
//                     }
//                     check_field = check_field->next;
//                 }

//                 if (!flag) {
//                     char msg[128];
//                     sprintf(msg, "Redefined field \"%s\"", var_name);
//                     SemanticError(15, dec->child[0]->line, msg);
//                 } else {
//                     cur->name = strdup(var_name);
//                     cur->type = ConstArray(cur_type, dim, size, 0);
//                 }
//                 if (dec_list->n_child == 3) {
//                     dec_list = dec_list->child[2];
//                     cur->next = (FieldList)malloc(sizeof(struct FieldList_));
//                     cur->next->next = NULL;
//                     cur = cur->next;
//                 } else {
//                     dec_list = NULL;
//                 }
//             } while (dec_list);

//             def_list = def_list->child[1];
//             if (def_list->n_child == 2) {
//                 cur->next = (FieldList)malloc(sizeof(struct FieldList_));
//                 cur = cur->next;
//                 cur->next = NULL;
//             } else {
//                 break;
//             }
//         }
//         /* define struct, need to add to symtab */
//         *isDefine = 1;
//     } else {
//         assert(0);
//     }
//     return ret;
// }
