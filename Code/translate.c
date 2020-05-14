#include "symtab.h"

InterCodes CodeHead;

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
    Node* var_dec = dec->child[0];
    while (var_dec->n_child == 4) {
        var_dec = var_dec->child[0];
    }
    Node* id = var_dec->child[0];
    SymTable variable = LookupTab(id->ident);
    Operand ope = variable->op_var;
    Type ope_type = variable->type;
    if (ope_type->kind != BASIC) {
        InterCodes dec_code = MakeInterCodesNode();
        dec_code->kind = I_DEC;
        dec_code->u.dec.size = GetSize(ope_type);
        dec_code->u.dec.x = ope;
        JointCodes(ret, dec_code);
    }
    if (dec->n_child == 3) {
        JointCodes(ret, TranslateExp(dec->child[2], ope));
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
        InterCodes code2 = MakeInterCodesNode();
        code2->data->kind = I_RETURN;
        code2->data->u.ret.x = temp;
        return JointCodes(code1, code2);
    }
    case 5: {
        /* IF LP Exp RP Stmt */
        /* WHILE LP Exp RP Stmt */
        Operand label1 = NewLabel(), label2 = NewLabel();
        if (!strcmp(exp->child[0]->symbol, "IF")) {
            InterCodes code1 = TranslateCond(exp->child[2], label1, label2);
            InterCodes code2 = TranslateStmt(exp->child[4]);
            InterCodes label1_code = LabelCode(label1);
            InterCodes label2_code = LabelCode(label2);
            JointCodes(code2, label2_code);
            JointCodes(label1_code, code2);
            return JointCodes(code1, label1_code);
        } else if (!strcmp(exp->child[0]->symbol, "WHILE")) {
            Operand label3 = NewLabel();
            InterCodes code1 = TranslateCond(exp->child[2], label2, label3);
            InterCodes code2 = TranslateStmt(exp->child[4]);
            InterCodes label1_code = LabelCode(label1);
            InterCodes label2_code = LabelCode(label2);
            InterCodes label3_code = LabelCode(label3);
            InterCodes goto_code = GotoCode(label1);
            JointCodes(goto_code, label3_code);
            JointCodes(code2, goto_code);
            JointCodes(label2_code, code2);
            JointCodes(code1, label2_code);
            return JointCodes(label1_code, code1);
        } else {
            assert(0);
        }
    }
    case 7: {
        /* IF LP Exp RP Stmt ELSE Stmt */
        Operand label1 = NewLabel(), label2 = NewLabel(), label3 = NewLabel();
        InterCodes code1 = TranslateCond(exp->child[2], label1, label2);
        InterCodes code2 = TranslateStmt(exp->child[4]);
        InterCodes code3 = TranslateStmt(exp->child[6]);
        InterCodes label1_code = LabelCode(label1);
        InterCodes label2_code = LabelCode(label2);
        InterCodes label3_code = LabelCode(label3);
        InterCodes goto_code = GotoCode(label3);
        JointCodes(code3, label3_code);
        JointCodes(label2_code, code3);
        JointCodes(goto_code, label2_code);
        JointCodes(code2, goto_code);
        JointCodes(label1_code, code2);
        return JointCodes(code1, label1_code);
    }
    default:
        assert(0);
    }
}

InterCodes TranslateExp(Node* exp, Operand place)
{
    switch (exp->n_child) {
    case 1: {
        /* 1 nodes */
        Node* obj = exp->child[0];
        if (!strcmp(obj->symbol, "ID")) {
            Operand variable = LookupOpe(obj->ident);
            InterCodes code = MakeInterCodesNode();
            code->data->kind = I_ASSIGN;
            code->data->u.assign.x = place;
            code->data->u.assign.y = variable;
            return code;
        } else if (!strcmp(obj->symbol, "INT")) {
            InterCodes code = MakeInterCodesNode();
            code->data->kind = I_ASSIGN;
            code->data->u.assign.x = place;
            code->data->u.assign.y = NewConstInt(obj->ival);
            return code;
        } else if (!strcmp(obj->symbol, "FLOAT")) {
            InterCodes code = MakeInterCodesNode();
            code->data->kind = I_ASSIGN;
            code->data->u.assign.x = place;
            code->data->u.assign.y = NewConstFloat(obj->fval) return code;
        } else {
            assert(0);
        }
        break;
    }
    case 2: {
        /* 2 nodes */
        Node* ope = exp->child[0];
        if (!strcmp(ope->symbol, "MINUS")) {
            /* MINUS Exp */
            Operand t1 = NewTemp();
            InterCodes code1 = TranslateExp(exp->child[1], t1);
            InterCodes code2 = MakeInterCodesNode();
            code2->data->kind = I_SUB;
            code2->data->u.sub.x = place;
            code2->data->u.sub.y = NewConstInt(0);
            code2->data->u.sub.z = t1;
            return JointCodes(code1, code2);
        } else if (!strcmp(ope->symbol, "NOT")) {
            /* NOT Exp */
            Operand label1 = NewLabel(), label2 = NewLabel();
            InterCodes code0 = MakeInterCodesNode();
            code0->data->kind = I_ASSIGN;
            code0->data->u.assign.x = place;
            code0->data->u.assign.y = NewConstInt(0);
            InterCodes code1 = TranslateCond(exp, label1, label2);
            InterCodes label1_code = LabelCode(label1);
            InterCodes code2 = MakeInterCodesNode();
            code2->data->kind = I_ASSIGN;
            code2->data->u.assign.x = place;
            code2->data->u.assign.y = NewConstInt(1);
            JointCodes(code2, LabelCode(label2));
            JointCodes(label1_code, code2);
            JointCodes(code1, label1_code);
            return JointCodes(code0, code1);
        } else {
            assert(0);
        }
        break;
    }
    case 3: {
        /* 3 nodes */
        if (!strcmp(exp->child[2]->symbol, "Exp")) {
            TODO()
            Node* ope = exp->child[1];
            Node* exp1 = exp->child[0];
            Node* exp2 = exp->child[2];
            if (!strcmp(ope->symbol, "ASSIGNOP")) {
                /* Exp ASSIGNOP Exp */
                Operand t1 = NewTemp();
                InterCodes code1 = TranslateExp(exp2, t1);
                if (exp1->n_child == 1
                    && !strcmp(exp1->child[0], "ID")) {
                    /* variable */
                    Operand variable = LookupOpe(exp1->child[0]->ident);
                    InterCodes code2 = MakeInterCodesNode();
                    code2->data->kind = ASSIGN;
                    code2->data->u.assign.x = variable;
                    code2->data->u.assign.y = t1;
                    InterCodes code3 = MakeInterCodesNode();
                    code3->data->kind = ASSIGN;
                    code3->data->u.assign.x = place;
                    code3->data->u.assign.y = variable;
                    JointCodes(code2, code3);
                    return JointCodes(code1, code2);
                } else if (exp1->n_child == 4) {
                    /* Exp LB Exp RB: array */
                    Operand addr = NewTemp();
                    InterCodes code2 = GetArrayAddr(exp1, addr);
                    InterCodes code3 = MakeInterCodesNode();
                    code3->data->kind = I_DEREF_L;
                    code3->data->u.defer_l.x = addr;
                    code3->data->u.defer_r.y = t1;
                    InterCodes code4 = MakeInterCodesNode();
                    code4->data->kind = ASSIGN;
                    code4->data->u.assign.x = place;
                    code4->data->u.assign.y = t1;
                    JointCodes(code3, code4);
                    JointCodes(code2, code3);
                    return JointCodes(code1, code2);
                } else {
                    /* Exp DOT ID: structure */
                    TODO()
                }
            } else if (!strcmp(ope->symbol, "PLUS")) {
                TODO()
            } else if (!strcmp(ope->symbol, "MINUS")) {
                TODO()
            } else if (!strcmp(ope->symbol, "STAR")) {
                TODO()
            } else if (!strcmp(ope->symbol, "DIV")) {
                TODO()
            } else if (!strcmp(ope->symbol, "AND")
                || !strcmp(ope->symbol, "OR")
                || !strcmp(ope->symbol, "RELOP")) {
                Operand label1 = NewLabel(), label2 = NewLabel();
                InterCodes code0 = MakeInterCodesNode();
                code0->data->kind = I_ASSIGN;
                code0->data->u.assign.x = place;
                code0->data->u.assign.y = NewConstInt(0);
                InterCodes code1 = TranslateCond(exp, label1, label2);
                InterCodes label1_code = LabelCode(label1);
                InterCodes code2 = MakeInterCodesNode();
                code2->data->kind = I_ASSIGN;
                code2->data->u.assign.x = place;
                code2->data->u.assign.y = NewConstInt(1);
                JointCodes(code2, LabelCode(label2));
                JointCodes(label1_code, code2);
                JointCodes(code1, label1_code);
                return JointCodes(code0, code1);
            } else {
                assert(0);
            }
        } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
            TODO()
        } else {
            /* ID LP RP | LP EXP RP */
            if (!strcmp(exp->child[0]->symbol, "LP")) {
                /* LP EXP RP */
                TODO()
            } else if (!strcmp(exp->child[0]->symbol, "ID")) {
                /* ID LP RP  function */
                TODO()
            } else {
                assert(0);
            }
        }
        break;
    }
    case 4: {
        if (!strcmp(exp->child[0]->symbol, "ID")) {
            /* ID LP Args RP */
            TODO()
        } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
            /* Exp LB Exp RB */
            TODO()
        } else {
            assert(0);
        }
        break;
    }
    default:
        assert(0);
    }
}

InterCodes TranslateCond(Node* exp, Operand label_true, Operand label_false)
{
    if (exp->n_child == 2 && !strcmp(exp->child[0]->symbol, "NOT")) {
        /* NOT Exp */
        return TranslateCond(exp->child[1], label_false, label_true);
    }
    if (exp->n_child == 3) {
        if (!strcmp(exp->child[1]->symbol, "RELOP")) {
            /* Exp RELOP Exp */
            Operand t1 = NewTemp(), t2 = NewTemp();
            InterCodes code1 = TranslateExp(exp->child[0], t1);
            InterCodes code2 = TranslateExp(exp->child[2], t2);
            InterCodes code3 = MakeInterCodesNode();
            code3->data->kind = I_JMP_IF;
            code3->data->u.jmp_if.x = t1;
            code3->data->u.jmp_if.y = t2;
            code3->data->u.jmp_if.z = label_true;
            code3->data->u.jmp_if.relop = GetRelop(exp->child[1]);
            InterCodes goto_code = GotoCode(label_false);
            JointCodes(code3, goto_code);
            JointCodes(code2, code3);
            return JointCodes(code1, code2);
        } else if (!strcmp(exp->child[1]->symbol, "AND")) {
            /* Exp AND Exp */
            Operand label1 = NewLabel();
            InterCodes code1 = TranslateCond(exp->child[0], label1, label_false);
            InterCodes code2 = TranslateCond(exp->child[2], label_true, label_false);
            InterCodes label1_code = LabelCode(label1);
            JointCodes(label1_code, code2);
            return JointCodes(code1, label1_code);
        } else if (!strcmp(exp->child[1]->symbol, "OR")) {
            Operand label1 = NewLabel();
            InterCodes code1 = TranslateCond(exp->child[0], label_true, label1);
            InterCodes code2 = TranslateCond(exp->child[2], label_true, label_false);
            InterCodes label1_code = LabelCode(label1);
            JointCodes(label1_code, code2);
            return JointCodes(code1, label1_code);
        }
    }
    Operand t1 = NewTemp();
    InterCodes code1 = TranslateExp(exp, t1);
    InterCodes code2 = MakeInterCodesNode();
    code2->data->kind = I_JMP_IF;
    code2->data->u.jmp_if.x = t1;
    code2->data->u.jmp_if.y = malloc(sizeof(struct Operand_));
    code2->data->u.jmp_if.y->kind = OP_CONST_INT;
    code2->data->u.jmp_if.y->u.value = 0;
    code2->data->u.jmp_if.z = label_true;
    code2->data->u.jmp_if.relop = NEQ;
    InterCodes goto_code = GotoCode(label_false);
    JointCodes(code2, goto_code);
    return JointCodes(code1, code2);
}

InterCodes GetArrayAddr(Node* exp, Operand addr)
{
    Node* temp = exp;
    int dim = 0;
    InterCodes ret = NULL;
    while (temp->n_child != 1) {
        temp = temp->child[0];
        ++dim;
    }
    int len[dim];
    Operand off[dim];
    for (int i = 0; i < dim; i++) {
        off[i] = NewTemp();
    }
    SymTable fund_array = LookupTab(temp->child[0]->ident);
    Type fund_type = fund_array->type;
    Type temp_type = fund_type;
    temp = exp;
    for (int i = 1; i <= dim; i++) {
        JointCodes(ret, TranslateExp(temp->child[2], off[dim - i]));
        temp_type = temp_type->u.array.elem;
        temp = temp->child[0];
        len[i - 1] = GetSize(temp_type);
    }
    InterCodes code1 = MakeInterCodesNode();
    if (fund_array->isParam == 1) {
        code1->data->kind = I_ASSIGN;
        code1->data->u.assign.x = addr;
        code1->data->u.assign.y = fund_array->op_var;
    } else {
        code1->data->kind = I_ADDR;
        code1->data->u.addr.x = addr;
        code1->data->u.addr.y = fund_array->op_var;
    }
    JointCodes(ret, code1);
    InterCodes temp = NULL, temp_add = NULL;
    for (int i = 0; i < dim; i++) {
        temp = MakeInterCodesNode();
        temp->data->kind = I_MUL;
        temp->data->u.mul.x = off[i];
        temp->data->u.mul.y = off[i];
        temp->data->u.mul.z = NewConstInt(len[i]);
        temp_add = MakeInterCodesNode();
        temp_add->data->kind = I_ADD;
        temp_add->data->u.add.x = addr;
        temp_add->data->u.add.y = addr;
        temp_add->data->u.add.z = off[i];
        JointCodes(temp, temp_add);
        JointCodes(code1, temp);
    }
    return ret;
}

char* TraceArray(Node* exp, int* offsite)
{
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
    ret->data = (InterCode)malloc(sizeof(struct InterCode_));
    return ret;
}

Operand NewTemp()
{
    static int TempNo = 0;
    Operand temp = (Operand)malloc(sizeof(struct Operand_));
    temp->kind = OP_TEMP;
    temp->u.var_no = ++TempNo;
    return temp;
}

Operand NewLabel()
{
    static int LabelNo = 0;
    Operand label = (Operand)malloc(sizeof(struct Operand_));
    label->kind = OP_LABEL;
    label->u.var_no = ++LabelNo;
    return label;
}

Operand NewConstInt(int ival)
{
    Operand temp_int = (Operand)malloc(sizeof(struct Operand_));
    temp_int->kind = OP_CONST_INT;
    temp_int->value = ival;
    return temp_int;
}

Operand NewConstFloat(float fval)
{
    Operand temp_float = (Operand)malloc(sizeof(struct Operand_));
    temp_float->kind = OP_CONST_FLOAT;
    temp_float->fval = fval;
    return temp_float;
}

InterCodes GotoCode(Operand label)
{
    InterCodes goto_code = MakeInterCodesNode();
    goto_code->data->kind = I_JMP;
    goto_code->data->u.jmp.x = label;
    return goto_code;
}

InterCodes LabelCode(Operand label)
{
    InterCodes label_code = MakeInterCodesNode();
    label_code->kind = I_LABEL;
    label_code->u.label.x = label;
    return label_code;
}

RELOP_TYPE GetRelop(Node* relop)
{
    assert(strcmp(relop->symbol, "RELOP") == 0);
    char* ident = relop->ident;
    if (!strcmp(ident, ">=")) {
        return GEQ;
    } else if (!strcmp(ident, "<=")) {
        return LEQ;
    } else if (!strcmp(ident, ">")) {
        return GE;
    } else if (!strcmp(ident, "<")) {
        return LE;
    } else if (!strcmp(ident, "==")) {
        return EQ;
    } else if (!strcmp(ident, "!=")) {
        return NEQ;
    }
    INFO("match failed!");
    assert(0);
}

Operand LookupOpe(char* name)
{
    static int VarNo = 0;
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
    /* void */
}
