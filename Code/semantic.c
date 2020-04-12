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

Type GetType(Node* spec)
{
    assert(spec != NULL);
    assert(strcmp(spec->symbol, "Specifier") == 0);
    assert(spec->n_child == 1);

    Node* tmp = spec->child[0];
    Type ret = (Type)malloc(sizeof(struct Type_));

    if (!strcmp(tmp->symbol, "TYPE")) {
        ret->kind = BASIC;
        if (!strcmp(tmp->ident, "int")) {
            ret->u.basic = INT;
        } else if (!strcmp(tmp->ident, "float")) {
            ret->u.basic = FLOAT;
        } else {
            assert(0);
        }
    } else if (!strcmp(tmp->symbol, "StructSpecifier")) {
        ret->kind = STRUCT;
        ret->u.structure = StructSpecAnalysis(tmp);
    } else {
        assert(0);
    }
    return ret;
}

FieldList StructSpecAnalysis(Node* struct_spec)
{
    /* if return NULL then error happen else succeed */
    assert(struct_spec != NULL);
    assert(structure->n_child >= 2);
    Node* tag = struct_spec->child[1];
    FieldList ret = NULL;
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
                    if (ret != NULL) {
                        /* shouldn't arrive here 
                           it means there are multiple the same structure
                           but the error should be found in def struture!
                           Note that structure shouldn't be defined in funciton 
                        */
                        assert(0);
                    }
                    ret = temp->type->u.structure;
                }
                temp = temp->next;
            }
        } else {
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
        ret = (FieldList)malloc(sizeof(struct FieldList_));
        ret->next = NULL;
        FieldList cur = ret;
        Node* def_list = struct_spec->child[3];
        while (def_list->n_child == 2) {
            Type cur_type = GetType(def_list->child[0]->child[0]);
            Node* dec_list = def_list->child[1];
            while (dec_list->n_child == 3) {
                Node* dec = dec_list->child[0];
                if (dec->n_child == 3) {
                    /* initial the variable in struture */
                    char msg[128];
                    sprintf(msg, "Initialize the domain of the structure");
                    SemanticError(15, dec->lin, msg);
                }
                /* add the domain regardless of initialization */
                TODO() /* check same name */

                Node* var_dec = dec->child[0];
                int dim = 0,
                    if (var_dec->n_child == 4)
                {
                    /* array */
                    TODO()
                }
                else if (var_dec->n_child == 1)
                {
                    TODO()
                }
                else
                {
                    assert(0);
                }
                dec_list = dec_list->child[2];
            }
            cur = cur->next;
            def_list = def_list->child[1];
        }
    } else {
        assert(0);
    }
    return ret;
}

char* RecurDecList(Node* root, int* dimen, int* size)
{
}