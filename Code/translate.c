#include "symtab.h"

InterCodes CodeHead;
static int VarNo, TempNo, LabelNo;

InterCodes Translate(Node* root)
{
    if (root->n_child == 2) {
        /* ExtDefList := ExtDef ExtDefList */
        InterCodes code1 = TranslateExtDef(root->child[0]);
        InterCodes code2 = Translate(root->child[1]);
        return JointCodes(code1, code2);
    } else if (root->n_child == 1) {
        /* Program := ExtDefList */
        InitTranslate();
        return Translate(root->child[0]);
    }
    return NULL;
}

InterCodes TranslateExtDef(Node* ext_def)
{
    if (ext_def->n_child == 3) {
        /* Specifier FunDec CompSt */
        assert(!strcmp(ext_def->child[2]->symbol, "CompSt"));
        InterCodes code1 = TranslateFunDec(ext_def->child[1]);
        InterCodes code2 = TranslateCompSt(ext_def->child[2]);
        return JointCodes(code1, code2);
    }
    return NULL;
}

InterCodes TranslateFunDec(Node* fun_dec)
{
    InterCodes func_node = MakeInterCodesNode();
    InterCode func = func_node->data;
    func->kind = I_FUNC;

    func->u.func.f = (Operand)malloc(sizeof(struct Operand_));
    func->u.func.f->kind = OP_FUNC;
    func->u.func.f->u.id = strdup(fun_dec->child[0]->ident);

    InterCodes param_list = NULL;
    if (fun_dec->n_child == 4) {
        param_list = TranslateVarList(fun_dec->child[2]);
    }
    return JointCodes(func, param_list);
}

InterCodes TranslateVarList(Node* var_list)
{
    InterCodes param = TranslateParamDec(var_list->child[0]);
    InterCodes param_l = NULL;
    if (var_list->n_child == 3) {
        /* ParamDec COMMA VarList */
        param_l = TranslateVarList(var_list->child[2]);
    }
    return JointCodes(param, param_l);
}

InterCodes TranslateParamDec(Node* param_dec)
{
    Node* var_dec = param_dec->child[1];
    while (var_dec->n_child == 4) {
        var_dec = var_dec->child[0];
    }
    Node* id = var_dec->child[0];
    InterCodes param = MakeInterCodesNode();
    param->data->kind = I_PARAM;
    param->data->u.param.x = LookupOpe(id->ident);
    return param;
}

InterCodes TranslateCompSt(Node* comp_st)
{
    InterCodes code1 = TranslateDefList(comp_st->child[1]);
    InterCodes code2 = TranslateStmtList(comp_st->child[2]);
    return JointCodes(code1, code2);
}

InterCodes TranslateDefList(Node* def_list)
{
    if (def_list->n_child == 0) {
        return NULL:
    }
    Node* dec_list = def_list->child[0]->child[1];
    InterCodes def_code = TranslateDecList(dec_list);
    InterCodes def_list_code = TranslateDefList(def_list->child[1]);
    return JointCodes(def_code, def_list_code);
}

InterCodes TranslateDecList(Node* dec_list)
{
    InterCodes dec_code = TranslateDec(dec_list->child[0]);
    InterCodes dec_list_code = NULL;
    if (dec_list->n_child == 3) {
        dec_list_code = TranslateDecList(dec_list->child[2]);
    }
    return JointCodes(dec_code, dec_list_code);
}

InterCodes TranslateDec(Node* dec)
{
    InterCodes ret = NULL;
    if (dec->n_child == 3) {
        Node* var_dec = dec->child[0];
        while (var_dec->n_child == 4) {
            var_dec = var_dec->child[0];
        }
        Node* id = var_dec->child[0];
        Operand ope = LookupOpe(id->ident);
        ret = TranslateExp(dec->child[2], ope);
    }
    return ret;
}

InterCodes TranslateStmtList(Node* stmt_list)
{
    if (stmt_list->n_child == 2) {
        InterCodes code1 = TranslateStmt(stmt_list->child[0]);
        InterCodes code2 = TranslateStmtList(stmt_list->child[1]);
        return JointCodes(code1, code2);
    } else {
        return NULL;
    }
}

InterCodes TranslateStmt(Node* stmt)
{
    switch (stmt->n_child) {
    case 1: {
        /* CompST */
        return TranslateCompST(stmt->child[0]);
    }
    case 2: {
        /* Exp SEMI */
        return TranslateExp(stmt->child[0]);
    }
    case 3: {
        /* RETURN Exp SEMI */
        Operand temp = NewTemp();
        InterCodes code1 = TranslateExp(exp->child[1], temp);
        TODO()
    }
    case 5: {
        /* IF LP Exp RP Stmt */
        /* WHILE LP Exp RP Stmt */
        TODO()
    }
    case 7: {
        /* IF LP Exp RP Stmt ELSE Stmt */
        TODO()
    }
    default:
        assert(0);
    }
}

InterCodes TranslateExp(Node* exp, Operand place) {
    TODO()
}

InterCodes JointCodes(InterCodes code1, InterCodes code2)
{
    if (code1 == NULL) {
        return code2;
    }
    if (code2 == NULL) {
        return code1;
    }
    InterCodes temp = code1;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = code2;
    code2->prev = temp;
    return code1;
}

InterCodes MakeInterCodesNode()
{
    InterCodes ret = (InterCodes)malloc(sizeof(struct InterCodes_));
    ret->prev = NULL, ret->next = NULL;
    ret->data = (InterCode)malloc(sizeof());
    return ret;
}

Operand NewTemp()
{
    Operand temp = (Operand)malloc(sizeof(struct Operand_));
    temp->kind = OP_TEMP;
    temp->u.var_no = ++TempNo;
    return temp;
}

Operand LookupOpe(char* name)
{
    unsigned int idx = hash(name);
    SymTable temp = symtable[idx];
    while (temp) {
        if (!strcmp(temp->name, name)) {
            if (temp->op_var == NULL) {
                temp->op_var = (Operand)malloc(sizeof(struct Operand_));
                temp->op_var->kind = OP_VAR;
                temp->op_var->u.var_no = ++VarNo;
                //TODO() whether store the name of variable
            }
            return temp->op_var;
        }
        temp = temp->next;
    }

    /* the symbol is a function */
    Operand ret = NULL;
    FuncTable temp = FuncHead;
    while (temp->next) {
        temp = temp->next;
        if (!strcmp(temp->name, name)) {
            ret = (Operand)malloc(sizeof(struct Operand_));
            ret->kind = OP_FUNC;
            ret->u.id = strdup(name);
            break;
        }
    }
    assert(ret != NULL);
    return ret;
}

void InitTranslate()
{
    VarNo = 0;
    TempNo = 0;
    LabelNo = 0;
}
