#include "symtab.h"

SymTable symtable[0x3fff + 1];
FuncTable FuncHead;
struct SymTabStack_ symtabstack;

unsigned int hash(char* name)
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

static void SemanticError(int error_num, int lineno, char* errtext)
{
    fprintf(stdout, "Error type %d at Line %d: %s.\n", error_num, lineno, errtext);
}

void SemanticAnalysis(Node* root)
{
    assert(root != NULL);
    if (root->n_child == 2) {
        /* ExtDefList := ExtDef ExtDefList */
        ExtDefAnalysis(root->child[0]);
        SemanticAnalysis(root->child[1]);
    } else if (root->n_child == 1) {
        /* Program := ExtDefList */
        InitSA();
        SemanticAnalysis(root->child[0]);
        CheckFuncDefined();
    }
}

void ExtDefAnalysis(Node* root)
{
    assert(root != NULL);
    assert(root->n_child >= 2);
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
            func = AddFuncTab(func, 1);
            /*  analyze the CompSt
                return_type;
             */
            CompSTAnalysis(root->child[2], func);
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
#ifdef LOCAL_SYM
    CreateLocalVar();
#endif
    /* add the parameter to symtable */
    if (func != NULL && func->isDefined == 0) {
        FieldList para = func->para;
        while (para) {
            SymTable new_sym = AddSymTab(para->name, para->type, func->lineno);
            new_sym->isParam = 1;
            para = para->next;
        }
        func->isDefined = 1;
    }
    /* allowing the initialization problem */
    DefListAnalysis(root->child[1]);
    StmtListAnalysis(root->child[2], func);
#ifdef LOCAL_SYM
    DeleteLocalVar();
#endif
}

void DefListAnalysis(Node* def_list)
{
    /* only server for compst */
    if (def_list->n_child == 2) {
        DefAnalysis(def_list->child[0]);
        DefListAnalysis(def_list->child[1]);
    }
}

void DefAnalysis(Node* def)
{
    Type fund_type = SpecAnalysis(def->child[0]);
    DecListAnalysis(def->child[1], fund_type);
}

void DecListAnalysis(Node* root, Type type)
{
    Node* dec = root->child[0];
    Node* var_dec = dec->child[0];
    int dim = 0, *size = (int*)malloc(256 * sizeof(int)), m_size = 256;
    char* name = TraceVarDec(var_dec, &dim, size, &m_size);
    Type ret = ConstArray(type, dim, size, 0);
    AddSymTab(name, ret, dec->line);
    if (dec->n_child == 3) {
        Node* exp = dec->child[2];
        ExpAnalysis(exp);
        if (!CheckTypeEql(ret, exp->eval->type)) {
            SemanticError(5, dec->child[1]->line, "Mismatched types on the sides of the assignment");
        }
    }
    if (root->n_child == 3) {
        DecListAnalysis(root->child[2], type);
    }
}

void StmtListAnalysis(Node* stmt_list, FuncTable func)
{
    if (stmt_list->n_child == 2) {
        StmtAnalysis(stmt_list->child[0], func);
        StmtListAnalysis(stmt_list->child[1], func);
    }
}

void StmtAnalysis(Node* stmt, FuncTable func)
{
    switch (stmt->n_child) {
    case 1: {
        /* CompST */
        CompSTAnalysis(stmt->child[0], func);
        break;
    }
    case 2: {
        /* Exp SEMI */
        ExpAnalysis(stmt->child[0]);
        break;
    }
    case 3: {
        /* RETURN Exp SEMI */
        ExpAnalysis(stmt->child[1]);
        if (!CheckTypeEql(stmt->child[1]->eval->type, func->ret_type)) {
            char msg[128];
            sprintf(msg, "Return type mismatches with the ret_type of Function \"%s\"", func->name);
            SemanticError(8, stmt->child[0]->line, msg);
        }
        break;
    }
    case 5: {
        /* IF LP Exp RP Stmt */
        /* WHILE LP Exp RP Stmt */
        Node *exp = stmt->child[2], *n_stmt = stmt->child[4];
        ExpAnalysis(exp);
        Type temp_type = (Type)malloc(sizeof(struct Type_));
        temp_type->kind = BASIC, temp_type->u.basic = 0;
        if (!CheckTypeEql(exp->eval->type, temp_type)) {
            SemanticError(7, exp->line, "Wrong type of conditional statement");
        }
        free(temp_type);
        StmtAnalysis(n_stmt, func);
        break;
    }
    case 7: {
        /* IF LP Exp RP Stmt ELSE Stmt */
        Node *exp = stmt->child[2], *stmt_1 = stmt->child[4], *stmt_2 = stmt->child[6];
        ExpAnalysis(exp);
        Type temp_type = (Type)malloc(sizeof(struct Type_));
        temp_type->kind = BASIC, temp_type->u.basic = 0;
        if (!CheckTypeEql(exp->eval->type, temp_type)) {
            SemanticError(7, exp->line, "Wrong type of conditional statement");
        }
        free(temp_type);
        StmtAnalysis(stmt_1, func);
        StmtAnalysis(stmt_2, func);
        break;
    }
    default:
        assert(0);
    }
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
    int dim = 0, *size = (int*)malloc(256 * sizeof(int)), m_size = 256;
    char* name = TraceVarDec(root->child[0], &dim, size, &m_size);

    new_var = ConstArray(type, dim, size, 0);
    AddSymTab(name, new_var, root->line);
}

FieldList FetchStructField(Node* def_list)
{
    /* check whether duplicated field happend */
    FieldList ret = NULL;
    if (def_list->n_child == 2) {
        Node* def = def_list->child[0];
        Node* dec_list = def->child[1];
        Type fund_type = SpecAnalysis(def->child[0]);
        ret = ConstFieldFromDecList(dec_list, fund_type);
        FieldList temp = ret;
        while (temp->next) {
            temp = temp->next;
        }
        temp->next = FetchStructField(def_list->child[1]);
        /* Check duplicated field */
        FieldList p1 = ret;
        while (p1 && p1 != temp->next) {
            FieldList p2 = temp;
            while (p2->next) {
                if (!strcmp(p1->name, p2->next->name)) {
                    char msg[128];
                    sprintf(msg, "Redefined field \"%s\"", p1->name);
                    SemanticError(15, p2->next->lineno, msg);
                    /* remove the duplicated field */
                    p2->next = p2->next->next;
                } else {
                    p2 = p2->next;
                }
            }
            p1 = p1->next;
        }
    }
    return ret;
}

FieldList ConstFieldFromDecList(Node* dec_list, Type type)
{
    FieldList ret = (FieldList)malloc(sizeof(struct FieldList_));
    Node* dec = dec_list->child[0];
    Node* var_dec = dec->child[0];
    int dim = 0, *size = (int*)malloc(256 * sizeof(int)), m_size = 256;
    char* name = TraceVarDec(var_dec, &dim, size, &m_size);
    ret->name = strdup(name);
    ret->lineno = var_dec->line;
    ret->type = ConstArray(type, dim, size, 0);
    ret->next = NULL;

    if (dec->n_child == 3) {
        /* VarDec ASSIGNOP Exp */
        SemanticError(15, dec->line, "Initialize the domain of the structure");
        ExpAnalysis(dec->child[2]);
        if (!CheckTypeEql(ret->type, dec->child[2]->eval->type)) {
            SemanticError(5, dec->child[1]->line, "Mismatched types on the sides of the assignment");
        }
    }

    if (dec_list->n_child == 3) {
        ret->next = ConstFieldFromDecList(dec_list->child[2], type);
    }
    return ret;
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
    int dim = 0, *size = (int*)malloc(256 * sizeof(int)), m_size = 256;
    char* para_name = TraceVarDec(param->child[1], &dim, size, &m_size);
    Type para_type = ConstArray(basic_type, dim, size, 0);

    ret->name = strdup(para_name);
    ret->type = para_type;
    ret->next = NULL;
    return ret;
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
            eval->type = LookupTab(obj->ident)->type;
            if (eval->type == NULL || (eval->type->kind == STRUCTURE && !strcmp(obj->ident, eval->type->u.struct_name))) {
                char msg[128];
                sprintf(msg, "Variable \"%s\" has not been defined", obj->ident);
                SemanticError(1, obj->line, msg);
                /* treat it as int */
                eval->type = (Type)malloc(sizeof(struct Type_));
                eval->type->kind = BASIC;
                eval->type->u.basic = 0;
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
                }
                if (!CheckTypeEql(obj1->eval->type, obj2->eval->type)) {
                    char msg[128];
                    sprintf(msg, "Mismatched types on the sides of the assignment");
                    SemanticError(5, ope->line, msg);
                }

                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
            } else if (!strcmp(ope->symbol, "AND")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckLogicOPE(obj1) && CheckLogicOPE(obj2)) {
                    exp->eval->isRight = 1;
                    exp->eval->val = (int)obj1->eval->val && (int)obj2->eval->val;
                    exp->eval->type = obj1->eval->type;
                }
            } else if (!strcmp(ope->symbol, "OR")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckLogicOPE(obj1) && CheckLogicOPE(obj2)) {
                    exp->eval->isRight = 1;
                    exp->eval->val = (int)obj1->eval->val || (int)obj2->eval->val;
                    exp->eval->type = obj1->eval->type;
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
                    exp->eval->type = obj1->eval->type;
                }
            } else if (!strcmp(ope->symbol, "PLUS")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->eval->isRight = 1;
                    exp->eval->val = obj1->eval->val + obj2->eval->val;
                }
            } else if (!strcmp(ope->symbol, "MINUS")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->eval->isRight = 1;
                    exp->eval->val = obj1->eval->val - obj2->eval->val;
                }
            } else if (!strcmp(ope->symbol, "STAR")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->eval->isRight = 1;
                    exp->eval->val = obj1->eval->val * obj2->eval->val;
                }
            } else if (!strcmp(ope->symbol, "DIV")) {
                memcpy(exp->eval, obj1->eval, sizeof(struct ExpNode_));
                if (CheckArithOPE(obj1, obj2)) {
                    exp->eval->isRight = 1;
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
                    exp->eval->isRight = 0;
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
        if (!strcmp(exp->child[0]->symbol, "ID")) {
            /* ID LP Args RP */
            ExpNode eval = exp->eval;
            FieldList func_para = ArgsAnalysis(exp->child[2]);
            eval->isRight = 1;
            eval->type = CheckFuncCall(exp->child[0]->ident, func_para, exp->line);
        } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
            /* Exp LB Exp RB */
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
            // if (!CheckTypeEql(obj->eval->type, temp_type)) {
            if (obj->eval->type->kind != ARRAY) {
                SemanticError(10, obj->line, "Apply array access operator on nonArray");
            } else {
                exp->eval->type = obj->eval->type->u.array.elem;
                exp->eval->isRight = 0;
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
    if (args->n_child == 3) {
        ret->next = ArgsAnalysis(args->child[2]);
    } else {
        ret->next = NULL;
    }
    return ret;
}

Type CheckFuncCall(char* func_name, FieldList para, int lineno)
{
    Type ret = NULL;
    FuncTable temp = FuncHead;
    while (temp->next) {
        temp = temp->next;
        if (!strcmp(func_name, temp->name)) {
            ret = temp->ret_type;
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
    Type ret = NULL;
    FieldList temp = structure;
    while (temp) {
        if (!strcmp(temp->name, name)) {
            ret = temp->type;
            break;
        }
        temp = temp->next;
    }
    return ret;
}

int CheckLogicOPE(Node* exp)
{
    assert(strcmp(exp->symbol, "Exp") == 0);
    if (exp->eval->type == NULL) {
        return 0;
    }
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

    Type ret = NULL;
    Node* tmp = spec->child[0];

    if (!strcmp(tmp->symbol, "TYPE")) {
        ret = (Type)malloc(sizeof(struct Type_));
        ret->kind = BASIC;
        if (!strcmp(tmp->ident, "int")) {
            ret->u.basic = 0;
        } else if (!strcmp(tmp->ident, "float")) {
            ret->u.basic = 1;
        } else {
            assert(0);
        }
    } else if (!strcmp(tmp->symbol, "StructSpecifier")) {
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

    Type ret = NULL;
    Node* tag = struct_spec->child[1];
    unsigned int tag_idx;

    if (!strcmp(tag->symbol, "Tag")) {
        /* declare structure variable */
        ret = GetStruct(tag->child[0]->ident);
        if (ret == NULL) {
            char msg[128];
            sprintf(msg, "Undefined structure \"%s\"", tag->child[0]->ident);
            SemanticError(17, tag->line, msg);
        }
        if (LookupTab(tag->child[0]->ident)) {
            char msg[128];
            sprintf(msg, "Duplicated structure name \"%s\"", tag->child[0]->ident);
            SemanticError(16, tag->child[0]->line, msg);
        }
    } else if (!strcmp(tag->symbol, "OptTag")) {
        /* define structure */
        Node* def_list = struct_spec->child[3];
        ret = (Type)malloc(sizeof(struct Type_));
        ret->kind = STRUCTURE;
        ret->u.structure = FetchStructField(def_list);
        if (tag->n_child == 0) {
            char struct_name[32];
            sprintf(struct_name, "anon_struture@%d_", tag->line);
            ret->u.struct_name = strdup(struct_name);
        } else {
            ret->u.struct_name = strdup(tag->child[0]->ident);
        }
        AddStructList(ret, tag->line);
        // AddSymTab(ret->u.struct_name, ret, tag->line);
    } else {
        assert(0);
    }
    return ret;
}

char* TraceVarDec(Node* var_dec, int* dim, int* size, int* m_size)
{
    /* to get array or ID */
    char* ret;
    if (var_dec->n_child == 1) {
        ret = var_dec->child[0]->ident;
    } else if (var_dec->n_child == 4) {
        if (*dim >= *m_size) {
            *m_size = *m_size + 256;
            size = (int*)realloc(size, (*m_size) * sizeof(int));
            // fprintf(stderr, "support maximum 256 dimension!\n");
            // return NULL;
        }
        size[*dim] = var_dec->child[2]->ival;
        *dim = *dim + 1;
        ret = TraceVarDec(var_dec->child[0], dim, size, m_size);
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

int GetSize(Type type)
{
    int ret = 0;
    switch (type->kind) {
    case BASIC: {
        ret = 4;
    } break;
    case ARRAY: {
        ret = GetSize(type->u.array.elem) * type->u.array.size;
    } break;
    case STRUCTURE: {
        FieldList member = type->u.structure;
        while (member) {
            ret += GetSize(member->type);
            member = member->next;
        }
    } break;
    default:
        assert(0);
    }
    return ret;
}

SymTable AddSymTab(char* type_name, Type type, int lineno)
{
    /*
        symtab store the global variables and structure
    */
    if (CheckSymTab(type_name, type, lineno)) {
        unsigned int idx = hash(type_name);
        SymTable new_sym = (SymTable)malloc(sizeof(struct SymTable_));
        new_sym->name = strdup(type_name);
        new_sym->type = type;
        new_sym->op_var = NULL;
        new_sym->isParam = 0;
        new_sym->next = symtable[idx];
        symtable[idx] = new_sym;
        if (symtabstack.depth > 0) {
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
        return new_sym;
    }
    return NULL;
}

FuncTable AddFuncTab(FuncTable func, int isDefined)
{
    if (CheckFuncTab(func, isDefined)) {
        /* there is not conflict */
        FuncTable temp = FuncHead;
        while (temp->next) {
            temp = temp->next;
            if (!strcmp(func->name, temp->name)) {
                /* function declaration */
                assert(func->isDefined == 0);
                temp->para = func->para;
                return temp;
            }
        }
        /* add the new function to the function list */
        temp->next = func;
        return func;
    }
    return func;
}

int AddStructList(Type structure, int lineno)
{
    if (CheckStructName(structure->u.struct_name)) {
        FieldList temp = symtabstack.StructHead;
        while (temp->next) {
            temp = temp->next;
        }
        temp->next = (FieldList)malloc(sizeof(struct FieldList_));
        temp->next->lineno = lineno;
        temp->next->type = structure;
        temp->next->name = strdup(structure->u.struct_name);
        temp->next->next = NULL;
    } else {
        char msg[128];
        sprintf(msg, "Duplicated variable or structure name \"%s\"", structure->u.struct_name);
        SemanticError(16, lineno, msg);
    }
}

int CheckStructName(char* name)
{
    FieldList temp = symtabstack.StructHead;
    while (temp->next) {
        temp = temp->next;
        if (!strcmp(temp->name, name)) {
            return 0;
        }
    }
    unsigned int idx = hash(name);
    SymTable sym_temp = symtable[idx];
    while (sym_temp) {
        if (!strcmp(sym_temp->name, name)) {
            return 0;
        }
        sym_temp = sym_temp->next;
    }
    return 1;
}

Type GetStruct(char* name)
{
    FieldList temp = symtabstack.StructHead;
    while (temp->next) {
        temp = temp->next;
        if (!strcmp(temp->name, name)) {
            return temp->type;
        }
    }
    return NULL;
}

SymTable LookupTab(char* name)
{
    /* if find symbol failed, return NULL */
    Type ret = NULL;
    unsigned int idx = hash(name);
    SymTable temp = symtable[idx];
    while (temp) {
        if (!strcmp(temp->name, name)) {
            ret = temp;
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
    symtabstack.var_stack[--symtabstack.depth] = NULL;
    // temp = symtabstack.var_stack[symtabstack.depth - 1];
    // SymTableNode to_del;
    // while (temp->next) {
    //     to_del = temp;
    //     temp = temp->next;
    //     free(to_del->var);
    //     free(to_del);
    // }
    // free(temp->var);
    // free(temp);
    // if (symtabstack.var_stack[symtabstack.depth - 1] != NULL) {
    //     free(symtabstack.var_stack[symtabstack.depth - 1]);
    //     symtabstack.var_stack[--symtabstack.depth] = NULL;
    // }
}

int CheckSymTab(char* type_name, Type type, int lineno)
{
    int ret = 1;
    unsigned int idx = hash(type_name);
    FieldList st_temp = symtabstack.StructHead;
    while (st_temp->next) {
        st_temp = st_temp->next;
        if (!strcmp(st_temp->name, type_name)) {
            ret = 0;
            break;
        }
    }
    if (ret) {
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
            SymTableNode temp = symtabstack.var_stack[symtabstack.depth - 1];
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
        sprintf(msg, "Duplicated variable name \"%s\"", type_name);
        SemanticError(3, lineno, msg);
    }
    return ret;
}

int CheckFuncTab(FuncTable func, int isDefined)
{
    int ret = 1;
    FuncTable temp = FuncHead;
    assert(temp != NULL);
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
            assert(func->ret_type);
            assert(temp->ret_type);
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
        return 0;
    }
    if (t1 == t2) {
        return 1;
    }
    if (t1->kind != t2->kind) {
        return 0;
    }
    switch (t1->kind) {
    case BASIC:
        return t1->u.basic == t2->u.basic;
    case ARRAY:
        return CheckTypeEql(t1->u.array.elem, t2->u.array.elem);
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
    if (f1 == f2) {
        return 1;
    }
    if (!CheckTypeEql(f1->type, f2->type)) {
        return 0;
    }
    return CheckFieldEql(f1->next, f2->next);
}

void InitSA()
{
    for (int i = 0; i <= 0x3fff; i++) {
        symtable[i] = NULL;
    }
    FuncHead = (FuncTable)malloc(sizeof(struct FuncTable_));
    FuncHead->next = NULL;

    symtabstack.depth = 0;
    symtabstack.StructHead = (FieldList)malloc(sizeof(struct FieldList_));
    symtabstack.StructHead->next = NULL;

    Type temp_type = (Type)malloc(sizeof(struct Type_));
    temp_type->kind = BASIC;
    temp_type->u.basic = 0;

    FuncTable read = (FuncTable)malloc(sizeof(struct FuncTable_));
    read->name = strdup("read");
    read->ret_type = temp_type;
    read->isDefined = 1;
    read->para = NULL;
    read->next = NULL;
    AddFuncTab(read, 1);

    FuncTable write = (FuncTable)malloc(sizeof(struct FuncTable_));
    write->name = strdup("write");
    write->ret_type = temp_type;
    write->isDefined = 1;
    write->para = (FieldList)malloc(sizeof(struct FieldList_));
    write->para->type = temp_type;
    write->para->next = NULL;
    write->next = NULL;
    AddFuncTab(write, 1);
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