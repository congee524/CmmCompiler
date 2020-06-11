#include "symtab.h"

InterCodes ICHead;

InterCodes Translate(Node *root) {
  if (root->n_child == 2) {
    /* ExtDefList := ExtDef ExtDefList */
    InterCodes code1 = TranslateExtDef(root->child[0]);
    InterCodes code2 = Translate(root->child[1]);
    return JointCodes(code1, code2);
  } else if (root->n_child == 1) {
    /* Program := ExtDefList */
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
  InterCodes func_node =
      MakeICNode(I_FUNC, LookupFuncOpe(fun_dec->child[0]->ident));
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
  InterCodes param = MakeICNode(I_PARAM, LookupVarOpe(id->ident));
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
  return JointCodes(def_code, def_list_code);
}

InterCodes TranslateDecList(Node *dec_list) {
  InterCodes dec_code = TranslateDec(dec_list->child[0]);
  InterCodes dec_list_code = NULL;
  if (dec_list->n_child == 3) {
    dec_list_code = TranslateDecList(dec_list->child[2]);
  }
  return JointCodes(dec_code, dec_list_code);
}

InterCodes TranslateDec(Node *dec) {
  InterCodes ret = NULL;
  Node *var_dec = dec->child[0];
  while (var_dec->n_child == 4) {
    var_dec = var_dec->child[0];
  }
  Node *id = var_dec->child[0];
  Operand ope = LookupVarOpe(id->ident);
  Type ope_type = LookupTab(id->ident);
  if (ope_type->kind != BASIC) {
    Operand raw_ope = NewTemp();
    InterCodes dec_code = MakeICNode(I_DEC);
    dec_code->data->u.dec.size = GetSize(ope_type);
    dec_code->data->u.dec.x = raw_ope;
    InterCodes addr_code = MakeICNode(I_ADDR, ope, raw_ope);
    dec_code = JointCodes(dec_code, addr_code);
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
    } break;
    case 2: {
      /* Exp SEMI */
      Operand tt = NewTemp();
      InterCodes code = TranslateExp(stmt->child[0], tt);
      return SimplifyPlace(code, tt, 0);
    } break;
    case 3: {
      /* RETURN Exp SEMI */
      Operand temp = NewTemp();
      InterCodes code1 = TranslateExp(stmt->child[1], temp);
      code1 = SimplifyPlace(code1, temp, 0);
      InterCodes code2 = MakeICNode(I_RETURN, temp);
      return JointCodes(code1, code2);
    } break;
    case 5: {
      /* IF LP Exp RP Stmt */
      /* WHILE LP Exp RP Stmt */
      Operand label1 = NewLabel(), label2 = NewLabel();
      if (!strcmp(stmt->child[0]->symbol, "IF")) {
        InterCodes code1 = TranslateCond(stmt->child[2], label1, label2);
        InterCodes code2 = TranslateStmt(stmt->child[4]);
        InterCodes label1_code = LabelCode(label1);
        InterCodes label2_code = LabelCode(label2);
        code2 = JointCodes(code2, label2_code);
        label1_code = JointCodes(label1_code, code2);
        return JointCodes(code1, label1_code);
      } else if (!strcmp(stmt->child[0]->symbol, "WHILE")) {
        Operand label3 = NewLabel();
        InterCodes code1 = TranslateCond(stmt->child[2], label2, label3);
        InterCodes code2 = TranslateStmt(stmt->child[4]);
        InterCodes label1_code = LabelCode(label1);
        InterCodes label2_code = LabelCode(label2);
        InterCodes label3_code = LabelCode(label3);
        InterCodes goto_code = GotoCode(label1);
        goto_code = JointCodes(goto_code, label3_code);
        code2 = JointCodes(code2, goto_code);
        label2_code = JointCodes(label2_code, code2);
        code1 = JointCodes(code1, label2_code);
        return JointCodes(label1_code, code1);
      } else {
        assert(0);
      }
    } break;
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
      code3 = JointCodes(code3, label3_code);
      label2_code = JointCodes(label2_code, code3);
      goto_code = JointCodes(goto_code, label2_code);
      code2 = JointCodes(code2, goto_code);
      label1_code = JointCodes(label1_code, code2);
      return JointCodes(code1, label1_code);
    } break;
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
        return MakeICNode(I_ASSIGN, place, LookupVarOpe(obj->ident));
      } else if (!strcmp(obj->symbol, "INT")) {
        return MakeICNode(I_ASSIGN, place, NewConstInt(obj->ival));
      } else if (!strcmp(obj->symbol, "FLOAT")) {
        return MakeICNode(I_ASSIGN, place, NewConstFloat(obj->fval));
      } else {
        assert(0);
      }
      return NULL;
    } break;
    case 2: {
      /* 2 nodes */
      Node *ope = exp->child[0];
      if (!strcmp(ope->symbol, "MINUS")) {
        /* MINUS Exp */
        Operand t1 = NewTemp();
        InterCodes code1 = TranslateExp(exp->child[1], t1);
        code1 = SimplifyPlace(code1, t1, 0);
        InterCodes code2 = MakeICNode(I_SUB, place, NewConstInt(0), t1);
        return JointCodes(code1, code2);
      } else if (!strcmp(ope->symbol, "NOT")) {
        /* NOT Exp */
        Operand label1 = NewLabel(), label2 = NewLabel();
        InterCodes code0 = MakeICNode(I_ASSIGN, place, NewConstInt(0));
        InterCodes code1 = TranslateCond(exp, label1, label2);
        InterCodes label1_code = LabelCode(label1);
        InterCodes code2 = MakeICNode(I_ASSIGN, place, NewConstInt(1));
        code2 = JointCodes(code2, LabelCode(label2));
        label1_code = JointCodes(label1_code, code2);
        code1 = JointCodes(code1, label1_code);
        return JointCodes(code0, code1);
      } else {
        assert(0);
      }
    } break;
    case 3: {
      /* 3 nodes */
      if (!strcmp(exp->child[2]->symbol, "Exp")) {
        Node *ope = exp->child[1], *exp1 = exp->child[0], *exp2 = exp->child[2];
        if (!strcmp(ope->symbol, "ASSIGNOP")) {
          /* Exp1 ASSIGNOP Exp */
          Type l_type = GetNearestType(exp1);
          if (l_type->kind != BASIC) {
            /* array assign */
            Type r_type = GetNearestType(exp2);
            int l_size = GetSize(l_type), r_size = GetSize(r_type);
            int size = ((l_size <= r_size) ? l_size : r_size) / 4;
            Operand addr1 = NewTemp(), addr2 = NewTemp();
            InterCodes code1 = GetAddr(exp1, addr1);
            InterCodes code2 = GetAddr(exp2, addr2);
            InterCodes code3 = MakeICNode(I_ASSIGN, place, addr2);
            code2 = JointCodes(code2, code3);
            InterCodes addr_inc_r, addr_deref_r, addr_inc_l, addr_deref_l;
            Operand step = NewConstInt(4), t1 = NewTemp();
            for (int i = 1; i < size; i++) {
              addr_deref_r = MakeICNode(I_DEREF_R, t1, addr2);
              addr_deref_l = MakeICNode(I_DEREF_L, addr1, t1);
              addr_inc_r = MakeICNode(I_ADD, addr2, addr2, step);
              addr_inc_l = MakeICNode(I_ADD, addr1, addr1, step);
              addr_inc_r = JointCodes(addr_inc_r, addr_inc_l);
              addr_deref_l = JointCodes(addr_deref_l, addr_inc_r);
              addr_deref_r = JointCodes(addr_deref_r, addr_deref_l);
              code3 = JointCodes(code3, addr_deref_r);
              code3 = addr_inc_l;
            }
            addr_deref_r = MakeICNode(I_DEREF_R, t1, addr2);
            addr_deref_l = MakeICNode(I_DEREF_L, addr1, t1);
            addr_deref_r = JointCodes(addr_deref_r, addr_deref_l);
            code3 = JointCodes(code3, addr_deref_r);
            return JointCodes(code1, code2);
          } else {
            Operand t1 = NewTemp();
            InterCodes code1 = TranslateExp(exp2, t1);
            code1 = SimplifyPlace(code1, t1, 0);
            if (!strcmp(exp1->child[0]->symbol, "ID")) {
              Operand variable = LookupVarOpe(exp1->child[0]->ident);
              InterCodes code2 = MakeICNode(I_ASSIGN, variable, t1);
              InterCodes code3 = MakeICNode(I_ASSIGN, place, variable);
              code2 = JointCodes(code2, code3);
              return JointCodes(code1, code2);
            } else {
              Operand addr = NewTemp();
              InterCodes code2 = GetAddr(exp1, addr);
              InterCodes code3 = MakeICNode(I_DEREF_L, addr, t1);
              InterCodes code4 = MakeICNode(I_ASSIGN, place, t1);
              code3 = JointCodes(code3, code4);
              code2 = JointCodes(code2, code3);
              return JointCodes(code1, code2);
            }
          }
        } else if (!strcmp(ope->symbol, "AND") || !strcmp(ope->symbol, "OR") ||
                   !strcmp(ope->symbol, "RELOP")) {
          Operand label1 = NewLabel(), label2 = NewLabel();
          InterCodes code0 = MakeICNode(I_ASSIGN, place, NewConstInt(0));
          InterCodes code1 = TranslateCond(exp, label1, label2);
          InterCodes label1_code = LabelCode(label1);
          InterCodes code2 = MakeICNode(I_ASSIGN, place, NewConstInt(1));
          code2 = JointCodes(code2, LabelCode(label2));
          label1_code = JointCodes(label1_code, code2);
          code1 = JointCodes(code1, label1_code);
          return JointCodes(code0, code1);
        } else {
          Operand t1 = NewTemp(), t2 = NewTemp();
          InterCodes code1 = TranslateExp(exp1, t1);
          code1 = SimplifyPlace(code1, t1, 0);
          InterCodes code2 = TranslateExp(exp2, t2);
          code2 = SimplifyPlace(code2, t2, 0);
          InterCodes code3 = MakeICNode(I_ADD, place, t1, t2);
          if (!strcmp(ope->symbol, "PLUS")) {
            code3->data->kind = I_ADD;
          } else if (!strcmp(ope->symbol, "MINUS")) {
            code3->data->kind = I_SUB;
          } else if (!strcmp(ope->symbol, "STAR")) {
            code3->data->kind = I_MUL;
          } else if (!strcmp(ope->symbol, "DIV")) {
            code3->data->kind = I_DIV;
          }
          code2 = JointCodes(code2, code3);
          return JointCodes(code1, code2);
        }
      } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
        /* Exp1 DOT ID */
        Operand addr = NewTemp();
        InterCodes code1 = GetAddr(exp, addr);
        InterCodes code2 = MakeICNode(I_ASSIGN, place, addr);
        Type type = GetNearestType(exp);
        if (type->kind == BASIC) {
          code2->data->kind = I_DEREF_R;
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
          InterCodes code = MakeICNode(I_TEMP);
          if (!strcmp(id, "read")) {
            code->data->kind = I_READ;
            code->data->u.read.x = place;
          } else {
            Operand func = LookupFuncOpe(id);
            code->data->kind = I_CALL;
            code->data->u.call.f = func;
            code->data->u.call.x = place;
          }
          return code;
        } else {
          assert(0);
        }
      }
    } break;
    case 4: {
      if (!strcmp(exp->child[0]->symbol, "ID")) {
        /* ID LP Args RP */
        ArgList arg_list = (ArgList)malloc(sizeof(struct ArgList_));
        arg_list->next = NULL, arg_list->prev = NULL, arg_list->arg = NULL;
        InterCodes code1 = TranslateArgs(exp->child[2], arg_list);
        if (!strcmp(exp->child[0]->ident, "write")) {
          InterCodes code2 = MakeICNode(I_WRITE, arg_list->next->arg);
          InterCodes code3 = MakeICNode(I_ASSIGN, place, NewConstInt(0));
          code2 = JointCodes(code2, code3);
          return JointCodes(code1, code2);
        }
        ArgList temp = arg_list;
        InterCodes arg_code = NULL;
        while (temp->next) {
          temp = temp->next;
          arg_code = MakeICNode(I_ARG, temp->arg);
          code1 = JointCodes(code1, arg_code);
        }
        Operand func = LookupFuncOpe(exp->child[0]->ident);
        InterCodes code2 = MakeICNode(I_CALL, place, func);
        return JointCodes(code1, code2);
      } else if (!strcmp(exp->child[0]->symbol, "Exp")) {
        /* Exp LB Exp RB */
        Operand addr = NewTemp();
        InterCodes code1 = GetAddr(exp, addr);
        InterCodes code2 = MakeICNode(I_ASSIGN, place, addr);
        Type type = GetNearestType(exp);
        if (type->kind == BASIC) {
          code2->data->kind = I_DEREF_R;
        }
        return JointCodes(code1, code2);
      } else {
        assert(0);
      }
    } break;
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
      code1 = SimplifyPlace(code1, t1, 0);
      InterCodes code2 = TranslateExp(exp->child[2], t2);
      code2 = SimplifyPlace(code2, t2, 0);
      InterCodes code3 = MakeICNode(I_JMP_IF);
      code3->data->u.jmp_if.x = t1;
      code3->data->u.jmp_if.y = t2;
      code3->data->u.jmp_if.z = label_true;
      code3->data->u.jmp_if.relop = GetRelop(exp->child[1]);
      InterCodes goto_code = GotoCode(label_false);
      code3 = JointCodes(code3, goto_code);
      code2 = JointCodes(code2, code3);
      return JointCodes(code1, code2);
    } else if (!strcmp(exp->child[1]->symbol, "AND")) {
      /* Exp AND Exp */
      Operand label1 = NewLabel();
      InterCodes code1 = TranslateCond(exp->child[0], label1, label_false);
      InterCodes code2 = TranslateCond(exp->child[2], label_true, label_false);
      InterCodes label1_code = LabelCode(label1);
      label1_code = JointCodes(label1_code, code2);
      return JointCodes(code1, label1_code);
    } else if (!strcmp(exp->child[1]->symbol, "OR")) {
      /* Exp OR Exp */
      Operand label1 = NewLabel();
      InterCodes code1 = TranslateCond(exp->child[0], label_true, label1);
      InterCodes code2 = TranslateCond(exp->child[2], label_true, label_false);
      InterCodes label1_code = LabelCode(label1);
      label1_code = JointCodes(label1_code, code2);
      return JointCodes(code1, label1_code);
    }
  }
  Operand t1 = NewTemp();
  InterCodes code1 = TranslateExp(exp, t1);
  code1 = SimplifyPlace(code1, t1, 0);
  InterCodes code2 = MakeICNode(I_JMP_IF);
  code2->data->u.jmp_if.x = t1;
  code2->data->u.jmp_if.y = NewConstInt(0);
  code2->data->u.jmp_if.z = label_true;
  code2->data->u.jmp_if.relop = NEQ;
  code2 = JointCodes(code2, GotoCode(label_false));
  return JointCodes(code1, code2);
}

InterCodes TranslateArgs(Node *args, ArgList arg_list) {
  assert(arg_list != NULL);
  Operand t1 = NewTemp();
  InterCodes code1 = TranslateExp(args->child[0], t1);
  code1 = SimplifyPlace(code1, t1, 1);
  AddArgs(arg_list, t1);
  if (args->n_child == 3) {
    InterCodes code2 = TranslateArgs(args->child[2], arg_list);
    return JointCodes(code1, code2);
  }
  return code1;
}

InterCodes OptimIRCode(InterCodes root) {
  root = RemoveNull(root);
  // CalRefCnt(root);
  // root = SimplifyAssign(root);
  // root = SimplifyUselessVar(root);
  return root;
}

void IRGen(InterCodes root) {
  InterCode data = NULL;
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
    if (data) {
      switch (data->kind) {
        case I_LABEL: {
          fprintf(fout, "LABEL %s :\n", OpeName(data->u.label.x));
        } break; /* LABEL x : */
        case I_FUNC: {
          char *func_name = OpeName(data->u.func.f);
          if (!strcmp(func_name, "func@main")) {
            func_name = "main";
          }
          fprintf(fout, "FUNCTION %s :\n", OpeName(data->u.func.f));
        } break; /* FUNCTION f : */
        case I_ASSIGN: {
          fprintf(fout, "%s := %s\n", OpeName(data->u.unary.x),
                  OpeName(data->u.unary.y));
        } break; /* x := y */
        case I_ADD: {
          fprintf(fout, "%s := %s + %s\n", OpeName(data->u.binop.x),
                  OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break; /* x := y + z */
        case I_SUB: {
          fprintf(fout, "%s := %s - %s\n", OpeName(data->u.binop.x),
                  OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break; /* x := y - z */
        case I_MUL: {
          fprintf(fout, "%s := %s * %s\n", OpeName(data->u.binop.x),
                  OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break; /* x := y * z */
        case I_DIV: {
          fprintf(fout, "%s := %s / %s\n", OpeName(data->u.binop.x),
                  OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break; /* x := y / z */
        case I_ADDR: {
          fprintf(fout, "%s := &%s\n", OpeName(data->u.unary.x),
                  OpeName(data->u.unary.y));
        } break; /* x := &y */
        case I_DEREF_R: {
          fprintf(fout, "%s := *%s\n", OpeName(data->u.unary.x),
                  OpeName(data->u.unary.y));
        } break; /* x := *y */
        case I_DEREF_L: {
          fprintf(fout, "*%s := %s\n", OpeName(data->u.unary.x),
                  OpeName(data->u.unary.y));
        } break; /* *x := y */
        case I_JMP: {
          fprintf(fout, "GOTO %s\n", OpeName(data->u.jmp.x));
        } break; /* GOTO x */
        case I_JMP_IF: {
          fprintf(fout, "IF %s %s %s GOTO %s\n", OpeName(data->u.jmp_if.x),
                  RelopName(data->u.jmp_if.relop), OpeName(data->u.jmp_if.y),
                  OpeName(data->u.jmp_if.z));
        } break; /* IF x [relop] y GOTO z */
        case I_RETURN: {
          fprintf(fout, "RETURN %s\n", OpeName(data->u.ret.x));
        } break; /* RETURN x */
        case I_DEC: {
          fprintf(fout, "DEC %s %d\n", OpeName(data->u.dec.x),
                  data->u.dec.size);
        } break; /* DEC x [size] */
        case I_ARG: {
          fprintf(fout, "ARG %s\n", OpeName(data->u.arg.x));
        } break; /* ARG x */
        case I_CALL: {
          fprintf(fout, "%s := CALL %s\n", OpeName(data->u.call.x),
                  OpeName(data->u.call.f));
        } break; /* x := CALL f */
        case I_PARAM: {
          fprintf(fout, "PARAM %s\n", OpeName(data->u.param.x));
        } break; /* PARAM x */
        case I_READ: {
          fprintf(fout, "READ %s\n", OpeName(data->u.read.x));
        } break; /* READ x */
        case I_WRITE: {
          fprintf(fout, "WRITE %s\n", OpeName(data->u.write.x));
        } break; /* WRITE x */
        default:
          assert(0);
      }
    }
    temp = temp->next;
  }
}
