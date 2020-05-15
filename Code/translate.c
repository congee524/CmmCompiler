#include "symtab.h"

InterCodes CodeHead;

InterCodes Translate(Node *root) {
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

InterCodes TranslateExtDef(Node *ext_def) {
  if (ext_def->n_child == 3) {
    /* Specifier FunDec CompSt */
    assert(!strcmp(ext_def->child[2]->symbol, "CompSt"));
    InterCodes code1 = TranslateFunDec(ext_def->child[1]);
    InterCodes code2 = TranslateCompSt(ext_def->child[2]);
    return JointCodes(code1, code2);
  }
  return NULL;
}

InterCodes TranslateFunDec(Node *fun_dec) {
  InterCodes func_node = MakeInterCodesNode();
  func_node->data->kind = I_FUNC;
  func_node->data->u.func.f = LookupOpe(fun_dec->child[0]->ident);

  InterCodes param_list = NULL;
  if (fun_dec->n_child == 4) {
    param_list = TranslateVarList(fun_dec->child[2]);
  }
  return JointCodes(func_node, param_list);
}

InterCodes TranslateVarList(Node *var_list) {
  InterCodes param = TranslateParamDec(var_list->child[0]);
  InterCodes param_l = NULL;
  if (var_list->n_child == 3) {
    /* ParamDec COMMA VarList */
    param_l = TranslateVarList(var_list->child[2]);
  }
  return JointCodes(param, param_l);
}

InterCodes TranslateParamDec(Node *param_dec) {
  Node *var_dec = param_dec->child[1];
  while (var_dec->n_child == 4) {
    var_dec = var_dec->child[0];
  }
  Node *id = var_dec->child[0];
  InterCodes param = MakeInterCodesNode();
  param->data->kind = I_PARAM;
  param->data->u.param.x = LookupOpe(id->ident);
  return param;
}

InterCodes TranslateCompSt(Node *comp_st) {
  InterCodes code1 = TranslateDefList(comp_st->child[1]);
  InterCodes code2 = TranslateStmtList(comp_st->child[2]);
  return JointCodes(code1, code2);
}

InterCodes TranslateDefList(Node *def_list) {
  if (def_list->n_child == 0) {
    return NULL;
  }
  Node *dec_list = def_list->child[0]->child[1];
  InterCodes def_code = TranslateDecList(dec_list);
  InterCodes def_list_code = TranslateDefList(def_list->child[1]);
  if (def_code == NULL && def_list_code == NULL) {
    return NULL;
  } else {
    return JointCodes(def_code, def_list_code);
  }
}

InterCodes TranslateDecList(Node *dec_list) {
  InterCodes dec_code = TranslateDec(dec_list->child[0]);
  InterCodes dec_list_code = NULL;
  if (dec_list->n_child == 3) {
    dec_list_code = TranslateDecList(dec_list->child[2]);
  }
  if (dec_code == NULL && dec_list_code == NULL) {
    return NULL;
  } else {
    return JointCodes(dec_code, dec_list_code);
  }
}

InterCodes TranslateDec(Node *dec) {
  InterCodes ret = NULL;
  Node *var_dec = dec->child[0];
  while (var_dec->n_child == 4) {
    var_dec = var_dec->child[0];
  }
  Node *id = var_dec->child[0];
  Operand ope = LookupOpe(id->ident);
  Type ope_type = LookupTab(id->ident);
  if (ope_type->kind != BASIC) {
    InterCodes dec_code = MakeInterCodesNode();
    Operand raw_ope = NewTemp();
    dec_code->data->kind = I_DEC;
    dec_code->data->u.dec.size = GetSize(ope_type);
    dec_code->data->u.dec.x = raw_ope;
    InterCodes addr_code = MakeInterCodesNode();
    addr_code->data->kind = I_ADDR;
    addr_code->data->u.addr.x = ope;
    addr_code->data->u.addr.y = raw_ope;
    JointCodes(dec_code, addr_code);
    ret = JointCodes(ret, dec_code);
  }
  if (dec->n_child == 3) {
    ret = JointCodes(ret, TranslateExp(dec->child[2], ope));
  }
  return ret;
}

InterCodes TranslateStmtList(Node *stmt_list) {
  if (stmt_list->n_child == 2) {
    InterCodes code1 = TranslateStmt(stmt_list->child[0]);
    InterCodes code2 = TranslateStmtList(stmt_list->child[1]);
    return JointCodes(code1, code2);
  } else {
    return NULL;
  }
}

InterCodes TranslateStmt(Node *stmt) {
  switch (stmt->n_child) {
    case 1: {
      /* CompST */
      return TranslateCompSt(stmt->child[0]);
    }
    case 2: {
      /* Exp SEMI */
      Operand t1 = NewTemp();
      return TranslateExp(stmt->child[0], t1);
    }
    case 3: {
      /* RETURN Exp SEMI */
      Operand temp = NewTemp();
      InterCodes code1 = TranslateExp(stmt->child[1], temp);
      InterCodes code2 = MakeInterCodesNode();
      code2->data->kind = I_RETURN;
      code2->data->u.ret.x = temp;
      return JointCodes(code1, code2);
    }
    case 5: {
      /* IF LP Exp RP Stmt */
      /* WHILE LP Exp RP Stmt */
      Operand label1 = NewLabel(), label2 = NewLabel();
      if (!strcmp(stmt->child[0]->symbol, "IF")) {
        InterCodes code1 = TranslateCond(stmt->child[2], label1, label2);
        InterCodes code2 = TranslateStmt(stmt->child[4]);
        InterCodes label1_code = LabelCode(label1);
        InterCodes label2_code = LabelCode(label2);
        JointCodes(code2, label2_code);
        JointCodes(label1_code, code2);
        return JointCodes(code1, label1_code);
      } else if (!strcmp(stmt->child[0]->symbol, "WHILE")) {
        Operand label3 = NewLabel();
        InterCodes code1 = TranslateCond(stmt->child[2], label2, label3);
        InterCodes code2 = TranslateStmt(stmt->child[4]);
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
      InterCodes code1 = TranslateCond(stmt->child[2], label1, label2);
      InterCodes code2 = TranslateStmt(stmt->child[4]);
      InterCodes code3 = TranslateStmt(stmt->child[6]);
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

InterCodes TranslateExp(Node *exp, Operand place) {
  switch (exp->n_child) {
    case 1: {
      /* 1 nodes */
      Node *obj = exp->child[0];
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
        code->data->u.assign.y = NewConstFloat(obj->fval);
        return code;
      } else {
        assert(0);
      }
      break;
    }
    case 2: {
      /* 2 nodes */
      Node *ope = exp->child[0];
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
        Node *ope = exp->child[1];
        Node *exp1 = exp->child[0];
        Node *exp2 = exp->child[2];
        if (!strcmp(ope->symbol, "ASSIGNOP")) {
          /* Exp1 ASSIGNOP Exp */
          Type l_type = GetNearestType(exp1);
          if (l_type->kind != BASIC) {
            /* array assign */
            Type r_type = GetNearestType(exp2);
            int l_size = GetSize(l_type), r_size = GetSize(r_type);
            int size = (l_size <= r_size) ? l_size : r_size;
            size /= 4; /* assign by 4 byte */
            Operand addr1 = NewTemp(), addr2 = NewTemp();
            InterCodes code1 = GetAddr(exp1, addr1);
            InterCodes code2 = GetAddr(exp2, addr2);
            InterCodes code3 = MakeInterCodesNode();
            code3->data->kind = I_ASSIGN;
            code3->data->u.assign.x = place;
            code3->data->u.assign.y = addr2;
            JointCodes(code2, code3);
            InterCodes addr_inc_r, addr_deref_r, addr_inc_l, addr_deref_l;
            Operand step = NewConstInt(4), t1 = NewTemp();
            for (int i = 1; i < size; i++) {
              addr_deref_r = MakeInterCodesNode();
              addr_deref_r->data->kind = I_DEREF_R;
              addr_deref_r->data->u.deref_r.x = t1;
              addr_deref_r->data->u.deref_r.y = addr2;
              addr_deref_l = MakeInterCodesNode();
              addr_deref_l->data->kind = I_DEREF_L;
              addr_deref_l->data->u.deref_l.x = addr1;
              addr_deref_l->data->u.deref_l.y = t1;
              addr_inc_r = MakeInterCodesNode();
              addr_inc_r->data->kind = I_ADD;
              addr_inc_r->data->u.add.x = addr2;
              addr_inc_r->data->u.add.y = addr2;
              addr_inc_r->data->u.add.z = step;
              addr_inc_l = MakeInterCodesNode();
              addr_inc_l->data->kind = I_ADD;
              addr_inc_l->data->u.add.x = addr1;
              addr_inc_l->data->u.add.y = addr1;
              addr_inc_l->data->u.add.z = step;
              JointCodes(addr_inc_r, addr_inc_l);
              JointCodes(addr_deref_l, addr_inc_r);
              JointCodes(addr_deref_r, addr_deref_l);
              JointCodes(code3, addr_deref_r);
              code3 = addr_inc_l;
            }
            addr_deref_r = MakeInterCodesNode();
            addr_deref_r->data->kind = I_DEREF_R;
            addr_deref_r->data->u.deref_r.x = t1;
            addr_deref_r->data->u.deref_r.y = addr2;
            addr_deref_l = MakeInterCodesNode();
            addr_deref_l->data->kind = I_DEREF_L;
            addr_deref_l->data->u.deref_l.x = addr1;
            addr_deref_l->data->u.deref_l.y = t1;
            JointCodes(addr_deref_r, addr_deref_l);
            JointCodes(code3, addr_deref_r);
            return JointCodes(code1, code2);
          } else {
            Operand t1 = NewTemp();
            InterCodes code1 = TranslateExp(exp2, t1);
            Operand addr = NewTemp();
            InterCodes code2 = GetAddr(exp1, addr);
            InterCodes code3 = MakeInterCodesNode();
            code3->data->kind = I_DEREF_L;
            code3->data->u.deref_l.x = addr;
            code3->data->u.deref_l.y = t1;
            InterCodes code4 = MakeInterCodesNode();
            code4->data->kind = I_ASSIGN;
            code4->data->u.assign.x = place;
            code4->data->u.assign.y = t1;
            JointCodes(code3, code4);
            JointCodes(code2, code3);
            return JointCodes(code1, code2);
          }
        } else if (!strcmp(ope->symbol, "AND") || !strcmp(ope->symbol, "OR") ||
                   !strcmp(ope->symbol, "RELOP")) {
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
          Operand t1 = NewTemp(), t2 = NewTemp();
          InterCodes code1 = TranslateExp(exp1, t1);
          InterCodes code2 = TranslateExp(exp2, t2);
          InterCodes code3 = MakeInterCodesNode();
          code3->data->u.binop.x = place;
          code3->data->u.binop.y = t1;
          code3->data->u.binop.z = t2;
          if (!strcmp(ope->symbol, "PLUS")) {
            code3->data->kind = I_ADD;
          } else if (!strcmp(ope->symbol, "MINUS")) {
            code3->data->kind = I_SUB;
          } else if (!strcmp(ope->symbol, "STAR")) {
            code3->data->kind = I_MUL;
          } else if (!strcmp(ope->symbol, "DIV")) {
            code3->data->kind = I_DIV;
          }
          JointCodes(code2, code3);
          return JointCodes(code1, code2);
        }
      } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
        /* Exp1 DOT ID */
        Operand addr = NewTemp();
        InterCodes code1 = GetAddr(exp, addr);
        InterCodes code2 = MakeInterCodesNode();
        Type type = GetNearestType(exp);
        if (type->kind == BASIC) {
          code2->data->kind = I_DEREF_R;
          code2->data->u.deref_r.x = place;
          code2->data->u.deref_r.y = addr;
        } else {
          code2->data->kind = I_ASSIGN;
          code2->data->u.assign.x = place;
          code2->data->u.assign.y = addr;
        }
        return JointCodes(code1, code2);
      } else {
        /* ID LP RP | LP EXP RP */
        if (!strcmp(exp->child[0]->symbol, "LP")) {
          /* LP EXP RP */
          return TranslateExp(exp->child[1], place);
        } else if (!strcmp(exp->child[0]->symbol, "ID")) {
          /* ID LP RP  function */
          char *id = exp->child[0]->ident;
          InterCodes code = MakeInterCodesNode();
          if (!strcmp(id, "read")) {
            code->data->kind = I_READ;
            code->data->u.read.x = place;
          } else {
            Operand func = LookupOpe(id);
            code->data->kind = I_CALL;
            code->data->u.call.f = func;
            code->data->u.call.x = place;
          }
          return code;
        } else {
          assert(0);
        }
      }
      break;
    }
    case 4: {
      if (!strcmp(exp->child[0]->symbol, "ID")) {
        /* ID LP Args RP */
        ArgList arg_list = (ArgList)malloc(sizeof(struct ArgList_));
        arg_list->next = NULL, arg_list->prev = NULL;
        arg_list->arg = NULL;
        InterCodes code1 = TranslateArgs(exp->child[2], arg_list);
        if (!strcmp(exp->child[0]->ident, "write")) {
          InterCodes code2 = MakeInterCodesNode();
          code2->data->kind = I_WRITE;
          code2->data->u.write.x = arg_list->next->arg;
          return JointCodes(code1, code2);
        }
        ArgList temp = arg_list;
        InterCodes arg_code = NULL;
        while (temp->next) {
          temp = temp->next;
          arg_code = MakeInterCodesNode();
          arg_code->data->kind = I_ARG;
          arg_code->data->u.arg.x = temp->arg;
          JointCodes(code1, arg_code);
        }
        Operand func = LookupOpe(exp->child[0]->ident);
        InterCodes code2 = MakeInterCodesNode();
        code2->data->kind = I_CALL;
        code2->data->u.call.f = func;
        code2->data->u.call.x = place;
        return JointCodes(code1, code2);
      } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
        /* Exp LB Exp RB */
        Operand addr = NewTemp();
        InterCodes code1 = GetAddr(exp, addr);
        InterCodes code2 = MakeInterCodesNode();
        Type type = GetNearestType(exp);
        if (type->kind == BASIC) {
          code2->data->kind = I_DEREF_R;
          code2->data->u.deref_r.x = place;
          code2->data->u.deref_r.y = addr;
        } else {
          code2->data->kind = I_ASSIGN;
          code2->data->u.assign.x = place;
          code2->data->u.assign.y = addr;
        }
        return JointCodes(code1, code2);
      } else {
        assert(0);
      }
      break;
    }
    default:
      assert(0);
  }
}

InterCodes TranslateCond(Node *exp, Operand label_true, Operand label_false) {
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
  code2->data->u.jmp_if.y = NewConstInt(0);
  code2->data->u.jmp_if.z = label_true;
  code2->data->u.jmp_if.relop = NEQ;
  InterCodes goto_code = GotoCode(label_false);
  JointCodes(code2, goto_code);
  return JointCodes(code1, code2);
}

InterCodes TranslateArgs(Node *args, ArgList arg_list) {
  assert(arg_list != NULL);
  Operand t1 = NewTemp();
  InterCodes code1 = TranslateExp(args->child[0], t1);
  AddArgs(arg_list, t1);
  if (args->n_child == 3) {
    InterCodes code2 = TranslateArgs(args->child[2], arg_list);
    return JointCodes(code1, code2);
  }
  return code1;
}

void AddArgs(ArgList arg_list, Operand t1) {
  ArgList new_arg = (ArgList)malloc(sizeof(struct ArgList_));
  new_arg->arg = t1;
  new_arg->next = arg_list->next;
  new_arg->prev = arg_list;
  arg_list->next = new_arg;
  if (new_arg->next) {
    new_arg->next->prev = new_arg;
  }
}

Type GetNearestType(Node *exp) {
  switch (exp->n_child) {
    case 1: {
      /* ID */
      return LookupTab(exp->child[0]->ident);
    } break;
    case 3: {
      /* Exp DOT ID */
      return LookupTab(exp->child[2]->ident);
    } break;
    case 4: {
      /* Exp LB Exp RB */
      return GetNearestType(exp->child[0])->u.array.elem;
    }
    default:
      assert(0);
  }
  return NULL;
}

InterCodes GetAddr(Node *exp, Operand addr) {
  switch (exp->n_child) {
    case 1: {
      /* ID */
      Operand ope = LookupOpe(exp->child[0]->ident);
      Type type = LookupTab(exp->child[0]->ident);
      InterCodes code1 = MakeInterCodesNode();
      if (type->kind == BASIC) {
        code1->data->kind = I_ADDR;
        code1->data->u.addr.x = addr;
        code1->data->u.addr.y = ope;
      } else {
        code1->data->kind = I_ASSIGN;
        code1->data->u.addr.x = addr;
        code1->data->u.addr.y = ope;
      }
      return code1;
    } break;
    case 3: {
      /* Exp DOT ID */
      Operand addr1 = NewTemp();
      InterCodes code1 = GetAddr(exp->child[0], addr1);
      Type type = GetNearestType(exp->child[0]);
      assert(type->kind == STRUCTURE);
      Operand off = NewConstInt(GetStructMemOff(type, exp->child[2]->ident));
      InterCodes code2 = MakeInterCodesNode();
      code2->data->kind = I_ADD;
      code2->data->u.add.x = addr;
      code2->data->u.add.y = addr1;
      code2->data->u.add.z = off;
      return JointCodes(code1, code2);
    } break;
    case 4: {
      /* Exp LB Exp RB */
      Operand addr1 = NewTemp();
      InterCodes code1 = GetAddr(exp->child[0], addr1);
      Type type = GetNearestType(exp);
      Operand size = NewConstInt(GetSize(type));
      Operand len = NewTemp();
      InterCodes code2 = TranslateExp(exp->child[2], len);
      InterCodes code3 = MakeInterCodesNode();
      code3->data->kind = I_MUL;
      code3->data->u.mul.x = len;
      code3->data->u.mul.y = len;
      code3->data->u.mul.z = size;
      InterCodes code4 = MakeInterCodesNode();
      code4->data->kind = I_ADD;
      code4->data->u.add.x = addr;
      code4->data->u.add.y = addr1;
      code4->data->u.add.z = len;
      JointCodes(code3, code4);
      JointCodes(code2, code3);
      return JointCodes(code1, code2);
    }
  }
}

InterCodes JointCodes(InterCodes code1, InterCodes code2) {
  if (code1 == NULL) {
    if (code2 == NULL) {
      assert(0);
    }
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

InterCodes MakeInterCodesNode() {
  InterCodes ret = (InterCodes)malloc(sizeof(struct InterCodes_));
  ret->prev = NULL, ret->next = NULL;
  ret->data = (InterCode)malloc(sizeof(struct InterCode_));
  return ret;
}

Operand NewTemp() {
  static int TempNo = 0;
  Operand temp = (Operand)malloc(sizeof(struct Operand_));
  temp->kind = OP_TEMP;
  temp->u.var_no = ++TempNo;
  temp->ref_cnt = 0;
  return temp;
}

Operand NewLabel() {
  static int LabelNo = 0;
  Operand label = (Operand)malloc(sizeof(struct Operand_));
  label->kind = OP_LABEL;
  label->u.var_no = ++LabelNo;
  label->ref_cnt = 0;
  return label;
}

Operand NewConstInt(int ival) {
  Operand temp_int = (Operand)malloc(sizeof(struct Operand_));
  temp_int->kind = OP_CONST_INT;
  temp_int->u.value = ival;
  temp_int->ref_cnt = 0;
  return temp_int;
}

Operand NewConstFloat(float fval) {
  Operand temp_float = (Operand)malloc(sizeof(struct Operand_));
  temp_float->kind = OP_CONST_FLOAT;
  temp_float->u.fval = fval;
  temp_float->ref_cnt = 0;
  return temp_float;
}

InterCodes GotoCode(Operand label) {
  InterCodes goto_code = MakeInterCodesNode();
  goto_code->data->kind = I_JMP;
  goto_code->data->u.jmp.x = label;
  return goto_code;
}

InterCodes LabelCode(Operand label) {
  assert(label != NULL);
  InterCodes label_code = MakeInterCodesNode();
  label_code->data->kind = I_LABEL;
  label_code->data->u.label.x = label;
  return label_code;
}

RELOP_TYPE GetRelop(Node *relop) {
  assert(strcmp(relop->symbol, "RELOP") == 0);
  switch (relop->ival) {
    case 0: /* > */
      return GE;
    case 1: /* < */
      return LE;
    case 2: /* >= */
      return GEQ;
    case 3: /* <= */
      return LEQ;
    case 4: /* == */
      return EQ;
    case 5: /* != */
      return NEQ;
    default:
      assert(0);
  }
}

Operand LookupOpe(char *name) {
  static int VarNo = 0;
  unsigned int idx = hash(name);
  SymTable temp = symtable[idx];
  while (temp) {
    if (!strcmp(temp->name, name)) {
      if (temp->op_var == NULL) {
        temp->op_var = (Operand)malloc(sizeof(struct Operand_));
        temp->op_var->kind = OP_VAR;
        temp->op_var->ref_cnt = 0;
        temp->op_var->u.var_no = ++VarNo;
      }
      return temp->op_var;
    }
    temp = temp->next;
  }

  /* the symbol is a function */
  FuncTable temp_func = FuncHead;
  while (temp_func->next) {
    temp_func = temp_func->next;
    if (!strcmp(temp_func->name, name)) {
      if (temp_func->op_func == NULL) {
        temp_func->op_func = (Operand)malloc(sizeof(struct Operand_));
        temp_func->op_func->kind = OP_FUNC;
        temp_func->op_func->u.id = strdup(name);
        temp_func->op_func->ref_cnt = 0;
      }
      return temp_func->op_func;
    }
  }
  assert(0);
  return NULL;
}

void InitTranslate() { /* void */
}
