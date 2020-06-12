#include "symtab.h"

static unsigned int hash(char* name) {
  unsigned int val = 0, i;
  for (; *name; ++name) {
    val = (val << 2) + *name;
    if (i = val & ~0x3fff) {
      val = (val ^ (i >> 22)) & 0x3fff;
    }
  }
  return val;
}

/*============= translate =============*/

InterCodes SimplifyPlace(InterCodes code, Operand place, int isVar) {
  if (code == NULL) return code;
  InterCodes temp = code;
  while (temp->next) {
    temp = temp->next;
  }
  if (temp->data->kind == I_ASSIGN && temp->data->u.assign.x == place &&
      !(isVar && temp->data->u.assign.y->kind != OP_VAR)) {
    memcpy(place, temp->data->u.assign.y, sizeof(struct Operand_));
    if (temp != code) {
      temp->prev->next = NULL;
    } else {
      code = NULL;
    }
  }
  return code;
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

Type GetNearestType(Node* exp) {
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

InterCodes GetAddr(Node* exp, Operand addr) {
  switch (exp->n_child) {
    case 1: {
      /* ID */
      Operand ope = LookupVarOpe(exp->child[0]->ident);
      Type type = LookupTab(exp->child[0]->ident);
      InterCodes code1 = MakeICNode(I_ASSIGN, addr, ope);
      if (type->kind == BASIC) {
        code1->data->kind = I_ADDR;
      }
      return code1;
    } break;
    case 3: {
      /* Exp DOT ID */
      Operand addr1 = NewTemp();
      InterCodes code1 = GetAddr(exp->child[0], addr1);
      Type type = GetNearestType(exp->child[0]);
      Operand off = NewConstInt(GetStructMemOff(type, exp->child[2]->ident));
      InterCodes code2 = MakeICNode(I_ADD, addr, addr1, off);
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
      InterCodes code3 = MakeICNode(I_MUL, len, len, size);
      InterCodes code4 = MakeICNode(I_ADD, addr, addr1, len);
      JointCodes(code3, code4);
      JointCodes(code2, code3);
      return JointCodes(code1, code2);
    }
  }
}

InterCodes JointCodes(InterCodes code1, InterCodes code2) {
  if (code1 == NULL && code2 == NULL) {
    InterCodes ret = (InterCodes)malloc(sizeof(struct InterCodes_));
    ret->prev = NULL, ret->next = NULL, ret->data = NULL;
    return ret;
  }
  if (code1 == NULL) return code2;
  if (code2 == NULL) return code1;
  InterCodes temp = code1;
  while (temp->next) {
    temp = temp->next;
  }
  temp->next = code2;
  code2->prev = temp;
  return code1;
}

InterCodes MakeICNode(IC_TYPE kind, ...) {
  InterCodes ret = (InterCodes)malloc(sizeof(struct InterCodes_));
  ret->prev = NULL, ret->next = NULL;
  ret->data = (InterCode)malloc(sizeof(struct InterCode_));
  ret->data->kind = kind;
  va_list ap;
  va_start(ap, kind);
  switch (kind) {
    case I_LABEL:
    case I_JMP:
    case I_RETURN:
    case I_ARG:
    case I_PARAM:
    case I_READ:
    case I_WRITE:
    case I_FUNC: {
      ret->data->u.unitary.x = va_arg(ap, Operand);
    } break;
    case I_ASSIGN:
    case I_ADDR:
    case I_DEREF_L:
    case I_DEREF_R:
    case I_CALL: {
      ret->data->u.unary.x = va_arg(ap, Operand);
      ret->data->u.unary.y = va_arg(ap, Operand);
    } break;
    case I_ADD:
    case I_SUB:
    case I_MUL:
    case I_DIV: {
      ret->data->u.binop.x = va_arg(ap, Operand);
      ret->data->u.binop.y = va_arg(ap, Operand);
      ret->data->u.binop.z = va_arg(ap, Operand);
    } break;
    default:
      break;
  }
  va_end(ap);
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
  temp_int->u.ival = ival;
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

InterCodes GotoCode(Operand label) { return MakeICNode(I_JMP, label); }

InterCodes LabelCode(Operand label) { return MakeICNode(I_LABEL, label); }

RELOP_TYPE GetRelop(Node* relop) {
  assert(strcmp(relop->symbol, "RELOP") == 0);
  switch (relop->ival) {
    case 0: /* > */
      return GT;
    case 1: /* < */
      return LT;
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

Operand LookupVarOpe(char* name) {
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
  return NULL;
}

Operand LookupFuncOpe(char* name) {
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
  return NULL;
}

static void DeleteICNode(InterCodes to_del) {
  if (to_del->prev) {
    to_del->prev->next = to_del->next;
  }
  if (to_del->next) {
    to_del->next->prev = to_del->prev;
  }
}

char* OpeName(Operand ope) {
  char s[64];
  switch (ope->kind) {
    case OP_TEMP: {
      sprintf(s, "t%d", ope->u.var_no);
    } break;
    case OP_VAR: {
      sprintf(s, "v%d", ope->u.var_no);
    } break;
    case OP_CONST_INT: {
      sprintf(s, "#%d", ope->u.ival);
    } break;
    case OP_CONST_FLOAT: {
      sprintf(s, "#%f", ope->u.fval);
    } break;
    case OP_LABEL: {
      sprintf(s, "label%d", ope->u.var_no);
    } break;
    case OP_FUNC: {
      sprintf(s, "func_%s", ope->u.id);
    } break;
    default:
      assert(0);
  }
  return strdup(s);
}

char* RelopName(RELOP_TYPE relop) {
  switch (relop) {
    case GEQ:
      return ">=";
    case LEQ:
      return "<=";
    case GT:
      return ">";
    case LT:
      return "<";
    case EQ:
      return "==";
    case NEQ:
      return "!=";
  }
}

InterCodes RemoveNull(InterCodes root) {
  InterCode data = NULL;
  while (root && root->data == NULL) {
    root = root->next;
  }
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
    if (data == NULL) {
      DeleteICNode(temp);
    }
    temp = temp->next;
  }
  return root;
}

void CalRefCnt(InterCodes root) {
  InterCode data = NULL;
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
    if (data) {
      switch (data->kind) {
        case I_LABEL: {
          /* LABEL x : */
          data->u.label.x->ref_cnt++;
        } break;
        case I_FUNC: {
          /* FUNCTION f : */
          data->u.func.f->ref_cnt++;
        } break;
        case I_ASSIGN: {
          /* x := y */
          data->u.unary.x->ref_cnt++;
          data->u.unary.y->ref_cnt++;
        } break;
        case I_ADD:
        case I_SUB:
        case I_MUL:
        case I_DIV: {
          /* x y z */
          data->u.binop.x->ref_cnt++;
          data->u.binop.y->ref_cnt++;
          data->u.binop.z->ref_cnt++;
        } break;
        case I_ADDR: {
          /* x := &y */
          data->u.unary.x->ref_cnt++;
          data->u.unary.y->ref_cnt++;
        } break;
        case I_DEREF_R: {
          /* x := *y */
          data->u.unary.x->ref_cnt++;
          data->u.unary.y->ref_cnt++;
        } break;
        case I_DEREF_L: {
          /* *x := y */
          data->u.unary.x->ref_cnt++;
          data->u.unary.y->ref_cnt++;
        } break;
        case I_JMP: {
          /* GOTO x */
          data->u.jmp.x->ref_cnt++;
        } break;
        case I_JMP_IF: {
          /* IF x [relop] y GOTO z */
          data->u.jmp_if.x->ref_cnt++;
          data->u.jmp_if.y->ref_cnt++;
          data->u.jmp_if.z->ref_cnt++;
        } break;
        case I_RETURN: {
          /* RETURN x */
          data->u.ret.x->ref_cnt++;
        } break;
        case I_DEC: {
          /* DEC x [size] */
          data->u.dec.x->ref_cnt++;
        } break;
        case I_ARG: {
          /* ARG x */
          data->u.arg.x->ref_cnt++;
        } break;
        case I_CALL: {
          /* x := CALL f */
          data->u.call.x->ref_cnt++;
          data->u.call.f->ref_cnt++;
        } break;
        case I_PARAM: {
          /* PARAM x */
          data->u.param.x->ref_cnt++;
        } break;
        case I_READ: {
          /* READ x */
          data->u.read.x->ref_cnt++;
        } break;
        case I_WRITE: {
          /* WRITE x */
          data->u.write.x->ref_cnt++;
        } break;
        default:
          break;
      }
    }
    temp = temp->next;
  }
}

InterCodes SimplifyAssign(InterCodes root) {
  InterCode data = NULL;
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
    switch (data->kind) {
      case I_ASSIGN:
      case I_ADDR:
      case I_DEREF_R: {
        if (temp->next && temp->next->data->kind == I_ASSIGN &&
            data->u.unary.x->ref_cnt == 2 &&
            data->u.unary.x == temp->next->data->u.unary.y) {
          data->u.unary.x->ref_cnt -= 2;
          data->u.unary.x = temp->next->data->u.unary.x;
          if (temp->next->next) {
            temp->next->next->prev = temp;
          }
          temp->next = temp->next->next;
        }
      } break;
      case I_ADD:
      case I_SUB:
      case I_MUL:
      case I_DIV: {
        if (temp->next && temp->next->data->kind == I_ASSIGN &&
            data->u.binop.x->ref_cnt == 2 &&
            data->u.binop.x == temp->next->data->u.unary.y) {
          data->u.binop.x->ref_cnt -= 2;
          data->u.binop.x = temp->next->data->u.unary.x;
          if (temp->next->next) {
            temp->next->next->prev = temp;
          }
          temp->next = temp->next->next;
        }
      } break;
      case I_DEREF_L:
      case I_DEC:
      default:
        break;
    }
    temp = temp->next;
  }
  return root;
}

InterCodes SimplifyUselessVar(InterCodes root) {
  InterCode data = NULL;
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
    switch (data->kind) {
      case I_ASSIGN:
      case I_ADDR:
      case I_DEREF_R: {
        if (data->u.unary.x->ref_cnt == 1) {
          DeleteICNode(temp);
        }
      } break;
      case I_ADD:
      case I_SUB:
      case I_MUL:
      case I_DIV: {
        if (data->u.binop.x->ref_cnt == 1) {
          DeleteICNode(temp);
        }
      } break;
      default:
        break;
    }
    temp = temp->next;
  }
  return root;
}