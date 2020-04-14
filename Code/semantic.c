#include "symtab.h"

SymTable symtable[0x3fff + 1];
FuncTable FuncHead;
struct SymTabStack_ symtabstack;

void SemanticError(int error_num, int lineno, char* errtext)
{
    fprintf(stderr, "Error type %d at Line %d: %s.\n", error_num, lineno, errtext);
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
    if (root->n_child == 2) {
        /* ExtDefList := ExtDef ExtDefList */
        INFO("ExtDefList");
        ExtDefAnalysis(root->child[0]);
        SemanticAnalysis(root->child[1]);
    } else if (root->n_child == 1) {
        /* Program := ExtDefList */
        INFO("Program");
        InitProg();
        SemanticAnalysis(root->child[0]);
        CheckFuncDefined();
    }
}

void ExtDefAnalysis(Node* root)
{
    assert(root != NULL);
    assert(root->n_child >= 2);

    INFO(root->symbol);
    Node* tmp = root->child[1];
    Type type = SpecAnalysis(root->child[0]);

    if (!strcmp(tmp->symbol, "ExtDecList")) {
        /* Specifier ExtDecList SEMI */
        ExtDecListAnalysis(tmp, type);
    } else if (!strcmp(tmp->symbol, "SEMI")) {
        /* Specifier SEMI
           has define structure in SpecAnalysis;
         */
        return;
    } else if (!strcmp(tmp->symbol, "FunDec")) {
        FuncTable func = FunDecAnalysis(tmp, type);
        if (!strcmp(root->child[2]->symbol, "CompSt")) {
            /* function definition */
            AddFuncTab(func, 1);
            /*  analyze the CompSt
                return_type;
             */
            TODO()
        } else if (!strcmp(root->child[2]->symbol, "SEMI")) {
            /* function declarion */
            AddFuncTab(func, 0);
        } else {
            assert(0);
        }
    } else {
        assert(0);
    }
}

void CompSTAnalysis(Node* root, FuncTable func)
{
    /* analyze the compst [return_type, definition, exp etc.] */
    /* stack */
    CreateLocalVar();
    TODO()
    DeleteLocalVar();
}

FuncTable FunDecAnalysis(Node* root, Type type)
{
    /* get Func here */
    assert(root != NULL);
    FuncTable ret = (FuncTable)malloc(sizeof(struct FuncTable_));
    ret->name = strdup(root->child[0]->ident);
    ret->lineno = root->child[0]->line;
    ret->ret_type = type;
    ret->isDefined = 0;
    ret->next = NULL;

    /* check parameter */
    if (root->n_child == 3) {
        ret->para = NULL;
    } else if (root->n_child == 4) {
        ret->para = VarListAnalysis(root->child[2]);
    } else {
        assert(0);
    }

    return ret;
}

FieldList VarListAnalysis(Node* var_list)
{
    /* get the parameter list of a function */
    FieldList ret = ParamDecAnalysis(var_list->child[0]);

    if (var_list->n_child == 3) {
        ret->next = VarListAnalysis(var_list->child[2]);
    }
    return ret;
}

FieldList ParamDecAnalysis(Node* param)
{
    assert(param->n_child == 2);
    FieldList ret = malloc(sizeof(struct FieldList_));

    Type basic_type = SpecAnalysis(param->child[0]);
    int dim = 0, size[256];
    char* para_name = TraceVarDec(param->child[1], &dim, size);
    Type para_type = ConstArray(basic_type, dim, size, 0);

    ret->name = strdup(para_name);
    ret->type = para_type;
    ret->next = NULL;
    return ret;
}

void ExtDecListAnalysis(Node* root, Type type)
{
    /*  ExtDef: Specifier ExtDecList SEMI
        type is the Type of Specifier
        ExtDecList: 
            VarDec
        |   VarDec COMMA ExtDecList
        ;
    */
    assert(root != NULL);

    if (root->n_child == 3) {
        ExtDecListAnalysis(root->child[2], type);
    }

    Type new_var = (Type)malloc(sizeof(struct Type_));
    int dim = 0, size[256];
    char* name = TraceVarDec(root->child[0], &dim, size);

    new_var = ConstArray(type, dim, size, 0);
    AddSymTab(name, new_var, root->line);
}

void ExpAnalysis(Node* exp)
{
    assert(strcmp(exp->symbol, "Exp") == 0);
    if (exp->eval != NULL) {
        /* the analysis has completed */
        return;
    }

    exp->eval = (ExpNode)malloc(sizeof(struct ExpNode_));

    switch (exp->n_child) {
    case 1: {
        /* 1 nodes */
        Node* obj = exp->child[0];
        if (!strcmp(obj->symbol, "ID")) {
            /* variable */
            ExpNode eval = exp->eval;
            eval->isRight = 0;
            eval->type = LookupTab(obj->ident);
            if (eval->type == NULL) {
                char msg[128];
                sprintf(msg, "Variable \"%s\" has not been defined", obj->ident);
                SemanticError(1, obj->line, msg);
            }
        } else if (!strcmp(obj->symbol, "INT")) {
            ExpNode eval = exp->eval;
            eval->isRight = 1;
            eval->val = obj->ival;
            eval->type = (Type)malloc(sizeof(struct Type_));
            eval->type->kind = BASIC;
            eval->type->u.basic = 0;
        } else if (!strcmp(obj->symbol, "FLOAT")) {
            ExpNode eval = exp->eval;
            eval->isRight = 1;
            eval->val = obj->fval;
            eval->type = (Type)malloc(sizeof(struct Type_));
            eval->type->kind = BASIC;
            eval->type->u.basic = 1;
        } else {
            assert(0);
        }
        break;
    }
    case 2: {
        /* 2 nodes */
        Node *ope = exp->child[0], *obj = exp->child[1];
        ExpAnalysis(obj);
        if (!strcmp(ope->symbol, "MINUS")) {
            ExpNode eval = exp->eval;
            eval->isRight = obj->eval->isRight;
            eval->type = obj->eval->type;
            if (eval->type->kind != BASIC) {
                char msg[128];
                sprintf(msg, "Wrong operand object type");
                SemanticError(7, obj->line, msg);
            } else {
                eval->val = -(obj->eval->val);
            }
        } else if (!strcmp(ope->symbol, "NOT")) {
            memcpy(exp->eval, obj->eval, sizeof(struct ExpNode_));
            if (CheckLogicOPE(obj)) {
                exp->eval->val = !((int)obj->eval->val);
            }
        } else {
            assert(0);
        }
        break;
    }
    case 3: {
        /* 3 nodes */
        if (!strcmp(exp->child[2]->symbol, "Exp")) {
            Node *obj1 = exp->child[0], *obj2 = exp->child[2], *ope = exp->child[1];
            ExpAnalysis(obj1);
            ExpAnalysis(obj2);
            if (!strcmp(ope->symbol, "ASSIGNOP")) {
                if (obj1->eval->isRight == 1) {
                    char msg[128];
                    sprintf(msg, "Assign to left value");
                    SemanticError(6, obj1->line, msg);
                } else {
                    if (!CheckTypeEql(obj1->eval->type, obj2->eval->type)) {
                        char msg[128];
                        sprintf(msg, "Mismatched types on the sides of the assignment");
                        SemanticError(5, ope->line, msg);
                    }
                }
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
            } else if (!strcmp(ope->symbol, "AND")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckLogicOPE(obj1) && CheckLogicOPE(obj2)) {
                    exp->eval->isRight = 1;
                    exp->eval->val = (int)obj1->eval->val && (int)obj2->eval->val;
                    exp->eval->type = obj1->type;
                }
            } else if (!strcmp(ope->symbol, "OR")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckLogicOPE(obj1) && CheckLogicOPE(obj2)) {
                    exp->eval->isRight = 1;
                    exp->eval->val = (int)obj1->eval->val || (int)obj2->eval->val;
                    exp->eval->type = obj1->type;
                }
            } else if (!strcmp(ope->symbol, "RELOP")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckLogicOPE(obj1) && CheckLogicOPE(obj2)) {
                    exp->eval->isRight = 1;
                    switch (ope->ival) {
                    case 0:
                        exp->eval->val = (int)obj1->eval->val > (int)obj2->eval->val;
                        break;
                    case 1:
                        exp->eval->val = (int)obj1->eval->val < (int)obj2->eval->val;
                        break;
                    case 2:
                        exp->eval->val = (int)obj1->eval->val >= (int)obj2->eval->val;
                        break;
                    case 3:
                        exp->eval->val = (int)obj1->eval->val <= (int)obj2->eval->val;
                        break;
                    case 4:
                        exp->eval->val = (int)obj1->eval->val == (int)obj2->eval->val;
                        break;
                    case 5:
                        exp->eval->val = (int)obj1->eval->val != (int)obj2->eval->val;
                        break;
                    default:
                        assert(0);
                    }
                    exp->eval->type = obj1->type;
                }
            } else if (!strcmp(ope->symbol, "PLUS")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->isRight = 1;
                    exp->eval->val = obj1->eval->val + obj2->eval->val;
                }
            } else if (!strcmp(ope->symbol, "MINUS")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->isRight = 1;
                    exp->eval->val = obj1->eval->val - obj2->eval->val;
                }
            } else if (!strcmp(ope->symbol, "STAR")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->isRight = 1;
                    exp->eval->val = obj1->eval->val * obj2->eval->val;
                }
            } else if (!strcmp(ope->symbol, "DIV")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->isRight = 1;
                    exp->eval->val = obj1->eval->val / obj2->eval->val;
                }
            } else {
                assert(0);
            }
        } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
            /* Exp DOT ID : structure */
            Node *obj1 = exp->child[0], *obj2 = exp->child[2];
            ExpAnalysis(obj1);
            memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
            if (obj1->eval->type->kind != STRUCTURE) {
                char msg[128];
                sprintf(msg, "Apply DOT operand on nonstructure variable");
                SemanticError(13, exp->child[0]->line, msg);
            } else {
                Type exp_type = CheckStructField(obj1->eval->type->u.structure, obj2->ident);
                if (exp_type == NULL) {
                    char msg[128];
                    sprintf(msg, "Access the undefined field \"%s\" in structure", obj2->ident);
                    SemanticError(14, exp->child[0]->line, msg);
                } else {
                    /* access field succeed */
                    exp->eval->isRight = 1;
                    exp->eval->type = exp_type;
                }
            }
        } else {
            /* ID LP RP | LP EXP RP */
            if (!strcmp(exp->child[0]->symbol, "LP")) {
                /* LP EXP RP */
                ExpAnalysis(exp->child[1]);
                memcpy(exp->eval, exp->child[1]->eval, sizeof(struct ExpNode_));
            } else if (!strcmp(exp->child[0]->symbol, "ID")) {
                /* ID LP RP  function */
                ExpNode eval = exp->eval;
                /* there is no pointer */
                eval->isRight = 1;
                eval->type = CheckFuncCall(exp->child[0]->ident, NULL, exp->line);
            } else {
                assert(0);
            }
        }
        break;
    }
    case 4: {
        if (!strcmp(exp->child[0], "ID")) {
            ExpNode eval = exp->eval;
            /* there is no pointer */
            FieldList func_para = ArgsAnalysis(exp->child[2]);
            eval->isRight = 1;
            eval->type = CheckFuncCall(exp->child[0]->ident, func_para, exp->line);
        } else if (!strcmp(exp->child[1], "Exp")) {
            Node *obj = exp->child[0], *coord = exp->child[2];
            ExpAnalysis(obj);
            ExpAnalysis(coord);
            memcpy(exp->eval, obj->eval, sizeof(struct ExpNode_));
            Type temp_type = (Type)malloc(sizeof(struct Type_));
            temp_type->kind = BASIC, temp_type->u.basic = 0;
            if (!CheckTypeEql(coord->eval->type, temp_type)) {
                SemanticError(12, coord->line, "NonInteger in array access operator");
            }
            temp_type->kind = ARRAY;
            if (!CheckTypeEql(obj->eval->type, temp_type)) {
                SemanticError(10, obj->line, "Apply array access operator on nonArray");
            } else {
                exp->eval->type = obj->eval->type->u.array.elem;
                if (exp->eval->type->kind == BASIC) {
                    exp->isRight = 1;
                } else {
                    exp->isRight = 0;
                }
            }
            free(temp_type);
        } else {
            assert(0);
        }
        break;
    }
    default:
        assert(0);
    }
}

FieldList ArgsAnalysis(Node* args)
{
    /* cannot get the values of parameters */
    FieldList ret = (FieldList)malloc(sizeof(struct FieldList_));
    ExpAnalysis(args->child[0]);
    ret->type = args->child[0]->eval->type;
    ret->next = ArgsAnalysis(args->child[2]);
    return ret;
}

Type CheckFuncCall(char* func_name, FieldList para, int lineno)
{
    Type ret = NULL;
    FuncTable temp = FuncHead;
    while (temp->next) {
        temp->next;
        if (!strcmp(func_name, temp->name)) {
            ret = ret_type;
            if (!CheckFieldEql(para, temp->para)) {
                char msg[128];
                sprintf(msg, "Mismatched parameters on calling function \"%s\"", func_name);
                SemanticError(9, lineno, msg);
            }
            break;
        }
    }
    if (ret == NULL) {
        if (LookupTab(func_name) == NULL) {
            char msg[128];
            sprintf(msg, "Call undefined function \"%s\"", func_name);
            SemanticError(2, lineno, msg);
        } else {
            char msg[128];
            sprintf(msg, "Apply Call operation on variable \"%s\"", func_name);
            SemanticError(11, lineno, msg);
        }
    }
    return ret;
}

Type CheckStructField(FieldList structure, char* name)
{
    Type ret = ret;
    FieldList temp = structure;
    while (temp) {
        if (!strcmp(temp->name, name)) {
            ret = temp->type;
            break;
        }
    }
    return ret;
}

int CheckLogicOPE(Node* exp)
{
    assert(strcmp(exp->symbol, "Exp") == 0);
    if (exp->eval->type->kind != BASIC || exp->eval->type->u.basic != 0) {
        SemanticError(7, exp->line, "Wrong operand object type");
        return 0;
    }
    return 1;
}

int CheckArithOPE(Node* obj1, Node* obj2)
{
    assert(strcmp(obj1->symbol, "Exp") == 0 && strcmp(obj2->symbol, "Exp") == 0);
    if (obj1->eval->type->kind == BASIC && CheckTypeEql(obj1->eval->type, obj2->eval->type)) {
        return 1;
    }
    SemanticError(7, obj1->line, "Wrong operand object type");
    return 0;
}

Type SpecAnalysis(Node* spec)
{
    assert(spec != NULL);
    assert(strcmp(spec->symbol, "Specifier") == 0);
    assert(spec->n_child == 1);

    INFO("sepc analysis");
    Type ret;
    Node* tmp = spec->child[0];

    if (!strcmp(tmp->symbol, "TYPE")) {
        INFO("int a");
        ret = (Type)malloc(sizeof(struct Type_));
        ret->kind = BASIC;
        if (!strcmp(tmp->ident, "int")) {
            ret->u.basic = 0;
        } else if (!strcmp(tmp->ident, "float")) {
            ret->u.basic = 1;
        } else {
            assert(0);
        }
        INFO("finish");
    } else if (!strcmp(tmp->symbol, "StructSpecifier")) {
        INFO("STRUCT");
        ret = StructSpecAnalysis(tmp);
    } else {
        assert(0);
    }
    return ret;
}

Type StructSpecAnalysis(Node* struct_spec)
{
    /* if return NULL then error happen or empty struture */
    /* ret leak when define error happen */
    assert(struct_spec != NULL);
    assert(struct_spec->n_child >= 2);

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
                if (!strcmp(temp->name, tag->child[0]->ident) && temp->type->kind != STRUCTURE) {
                    /* duplicated structure name */
                    char msg[128]; /* the length of ID is less than 32 */
                    sprintf(msg, "Duplicated structure name \"%s\"", tag->child[0]->ident);
                    SemanticError(16, tag->line, msg);
                    return ret;
                } else if (!strcmp(temp->name, tag->child[0]->ident)) {
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
        INFO("opttag");
        char struct_name[32];
        if (tag->n_child == 0) {
            /* Anonymous structure */
            sprintf(struct_name, "anon_struture@%d_", tag->line);
        } else {
            strcpy(struct_name, tag->child[0]->ident);
        }

        // ret->u.struct_name = strdup(struct_name);
        FieldList cur;
        Node* def_list = struct_spec->child[3];
        if (def_list->n_child == 2) {
            /* not empty structure */
            ret->u.structure = (FieldList)malloc(sizeof(struct FieldList_));
            ret->u.structure->next = NULL;
            cur = ret->u.structure;
        }
        while (def_list->n_child == 2) {
            INFO("travel deflist of STRUCT OptTag LC DefList RC");
            Node* def = def_list->child[0];
            INFO("def_list!");
            Type cur_type = SpecAnalysis(def->child[0]);

            Node* dec_list = def->child[1];
            do {
                INFO("travel dec list");
                Node* dec = dec_list->child[0];
                if (dec->n_child == 3) {
                    /* initial the variable in struture */
                    char msg[128];
                    sprintf(msg, "Initialize the domain of the structure");
                    SemanticError(15, dec->line, msg);
                }
                /* add the domain regardless of initialization */

                Node* var_dec = dec->child[0];
                INFO("travel var list");
                int dim = 0, size[256];
                char* var_name = TraceVarDec(var_dec, &dim, size);
                /* check whether duplicated field exist */
                FieldList check_field = ret->u.structure;
                INFO("check whether duplicated name");
                int flag = 1;
                while (check_field && check_field->name) {
                    INFO(check_field->name);
                    INFO("check");
                    if (check_field->name && !strcmp(check_field->name, var_name)) {
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
                    INFO("print var name");
                    cur->name = strdup(var_name);
                    INFO("to const array");
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
        AddSymTab(struct_name, ret, def_list->line);
    } else {
        assert(0);
    }
    return ret;
}

char* TraceVarDec(Node* var_dec, int* dim, int* size)
{
    /* to get array or ID */
    INFO("here");
    char* ret;
    if (var_dec->n_child == 1) {
        INFO(var_dec->child[0]->ident);
        ret = var_dec->child[0]->ident;
    } else if (var_dec->n_child == 4) {
        /* support max 256 dimension */
        if (*dim >= 256) {
            fprintf(stderr, "support maximum 256 dimension!\n");
            return NULL;
        }
        size[*dim] = var_dec->child[2]->ival;
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

int AddSymTab(char* type_name, Type type, int lineno)
{
    /*
        symtab store the global variables and structure
    */
    INFO("add symtab");

    if (CheckSymTab(type_name, type, lineno)) {
        unsigned int idx = hash(type_name);
        SymTable new_sym = (SymTable)malloc(sizeof(struct SymTable_));
        new_sym->name = strdup(type_name);
        new_sym->type = type;
        new_sym->next = symtable[idx];
        symtable[idx] = new_sym;
        if (type->kind == STRUCTURE) {
            SymTableNode temp = symtabstack.StructHead;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = (SymTableNode)malloc(sizeof(struct SymTableNode_));
            temp = temp->next;
            temp->var = new_sym;
            temp->next = NULL;
        } else if (symtabstack.depth > 0) {
            /* add to local var list */
            SymTableNode temp = symtabstack.var_stack[symtabstack.depth - 1];
            while (temp->next) {
                temp = temp->next;
            }
            temp->next = (SymTableNode)malloc(sizeof(struct SymTableNode_));
            temp = temp->next;
            temp->var = new_sym;
            temp->next = NULL;
        }
        return 1;
    }
    return 0;
}

int AddFuncTab(FuncTable func, int isDefined)
{
    if (CheckFuncTab(func, isDefined)) {
        /* there is not conflict */
        FuncTable_ temp = FuncHead;
        while (temp->next) {
            temp = temp->next;
            if (!strcmp(func->name, temp->name)) {
                /* function declaration */
                assert(func->isDefined == 0);
                func->isDefined = isDefined;
                return 1;
            }
        }
        /* add the new function to the function list */
        temp->next = func;
        return 1;
    }
    return 0;
}

Type LookupTab(char* name)
{
    /* if find symbol failed, return NULL */
    Type ret = NULL;
    unsigned int idx = hash(name);
    SymTable temp = symtable[idx];
    while (temp) {
        if (!strcmp(temp->name, name)) {
            ret = temp->type;
            break;
        }
        temp = temp->next;
    }
    return ret;
}

void CreateLocalVar()
{
    symtabstack.var_stack[symtabstack.depth++] = (SymTableNode)malloc(sizeof(struct SymTableNode_));
    symtabstack.var_stack[symtabstack.depth - 1]->var = NULL;
    symtabstack.var_stack[symtabstack.depth - 1]->next = NULL;
}

void DeleteLocalVar()
{
    assert(symtabstack.depth > 0);
    SymTableNode temp = symtabstack.var_stack[symtabstack.depth - 1];
    unsigned int idx;
    while (temp->next) {
        temp = temp->next;
        idx = hash(temp->var->name);
        SymTable to_del = symtable[idx], last = NULL;
        while (to_del && strcmp(to_del->name, temp->var->name) != 0) {
            last = to_del;
            to_del = to_del->next;
        }
        assert(to_del != NULL);
        if (last == NULL) {
            symtable[idx] = to_del->next;
        } else {
            last->next = to_del->next;
        }
    }

    temp = symtabstack.var_stack[symtabstack.depth - 1];
    SymTableNode to_del;
    while (temp->next) {
        to_del = temp;
        temp = temp->next;
        free(to_del->val);
        free(to_del);
    }
    free(temp->var);
    free(temp);
    if (symtabstack.var_stack[symtabstack.depth - 1] != NULL) {
        free(symtabstack.var_stack[symtabstack.depth - 1]);
        symtabstack.var_stack[--symtabstack.depth] = NULL;
    }
}

int CheckSymTab(char* type_name, Type type, int lineno)
{
    int ret = 1;
    unsigned int idx = hash(type_name);
    if (symtabstack.depth == 0) {
        if (symtable[idx] != NULL) {
            SymTable temp = symtable[idx];
            while (temp) {
                if (!strcmp(temp->name, type_name)) {
                    ret = 0;
                    break;
                }
                temp = temp->next;
            }
        }
    } else {
        SymTableNode temp = symtabstack.StructHead;
        while (temp->next) {
            temp = temp->next;
            if (!strcmp(temp->var->name, type_name)) {
                ret = 0;
                break;
            }
        }
        if (ret) {
            temp = symtabstack.var_stack[symtabstack.depth - 1];
            while (temp->next) {
                temp = temp->next;
                if (!strcmp(temp->var->name, type_name)) {
                    ret = 0;
                    break;
                }
            }
        }
    }
    if (!ret) {
        char msg[128];
        int err_id = -1;
        if (type->kind == STRUCTURE) {
            sprintf(msg, "Duplicated structure name \"%s\"", type_name);
            err_id = 16;
        } else {
            sprintf(msg, "Duplicated variable name \"%s\"", type_name);
            err_id = 3;
        }
        SemanticError(err_id, lineno, msg);
    }
    return ret;
}

int CheckFuncTab(FuncTable func, int isDefined)
{
    int ret = 1;
    FuncTable temp = FuncHead;
    while (temp->next) {
        temp = temp->next;
        if (!strcmp(func->name, temp->name)) {
            if (isDefined & temp->isDefined) {
                /* has been defined */
                char msg[128];
                sprintf(msg, "Multiple defined function \"%s\"", func->name);
                SemanticError(4, func->lineno, msg);
                ret = 0;
            }
            /* check the param and return type */
            if (!CheckTypeEql(func->ret_type, temp->ret_type) || !CheckFieldEql(func->para, temp->para)) {
                char msg[128];
                sprintf(msg, "Conflicting function definitions or declarations at function \"%s\"", func->name);
                SemanticError(19, func->lineno, msg);
                ret = 0;
            }
            break;
        }
    }
    return ret;
}

int CheckTypeEql(Type t1, Type t2)
{
    if ((t1 == NULL && t2 != NULL) || (t1 != NULL && t2 == NULL)) {
        INFO("NULL type");
        return 0;
    }
    if (t1->kind != t2->kind) {
        return 0;
    }
    switch (kind) {
    case BASIC:
        return t1->u.basic == t2->u.basic;
    case ARRAY:
        return CheckTypeEql(t1->u.elem, t2->u.elem);
    case STRUCTURE:
        return CheckFieldEql(t1->u.structure, t2->u.structure);
    default:
        assert(0);
    }
}

int CheckFieldEql(FieldList f1, FieldList f2)
{
    if ((f1 == NULL && f2 != NULL) || (f1 != NULL && f2 == NULL)) {
        return 0;
    }
    if (!CheckTypeEql(f1->type, f2->type)) {
        return 0;
    }
    return CheckFieldEql(f1->next, f2->next);
}

void InitProg()
{
    for (int i = 0; i <= 0x3fff; i++) {
        if (symtable[i]) {
            SymTable temp = symtable[i];
            SymTable to_del;
            while (temp->next) {
                to_del = temp;
                temp = temp->next;
                free(to_del);
            }
            free(temp);
        }
        symtable[i] = NULL;
    }

    if (FuncHead) {
        SymTable temp = FuncHead;
        SymTable to_del;
        while (temp->next) {
            to_del = temp;
            temp = temp->next;
            free(to_del);
        }
        free(temp);
    }
    FuncHead = (FuncTable)malloc(sizeof(struct FuncTable_));
    FuncHead->next = NULL;

    symtabstack.depth = 0;
    if (symtabstack.StructHead) {
        SymTableNode temp = symtabstack.StructHead;
        SymTableNode to_del;
        while (temp->next) {
            to_del = temp;
            temp = temp->next;
            free(to_del->var);
            free(to_del);
        }
        free(temp->var);
        free(temp);
        if (symtabstack.StructHead != NULL) {
            free(symtabstack.StructHead);
        }
    }
    symtabstack.StructHead = (SymTableNode)malloc(sizeof(struct SymTableNode_));
    symtabstack.StructHead->var = NULL;
    symtabstack.StructHead->next = NULL;
}

void CheckFuncDefined()
{
    FuncTable func = FuncHead;
    while (func->next) {
        func = func->next;
        if (func->isDefined == 0) {
            char msg[128];
            sprintf(msg, "Function [%s] is declared but not defined!", func->name);
            SemanticError(18, func->lineno, msg);
        }
    }
}