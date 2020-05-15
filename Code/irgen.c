#include "symtab.h"

char *OpeName(Operand ope) {
  char s[64];
  switch (ope->kind) {
    case OP_TEMP: {
      sprintf(s, "t%d", ope->u.var_no);
    } break;
    case OP_VAR: {
      sprintf(s, "v%d", ope->u.var_no);
    } break;
    case OP_CONST_INT: {
      sprintf(s, "#%d", ope->u.value);
    } break;
    case OP_CONST_FLOAT: {
      sprintf(s, "#%f", ope->u.fval);
    } break;
    case OP_ADDR: {
      sprintf(s, "t%d", ope->u.var_no);
    } break;
    case OP_LABEL: {
      sprintf(s, "label%d", ope->u.var_no);
    } break;
    case OP_FUNC: {
      sprintf(s, "%s", ope->u.id);
    } break;
    default:
      assert(0);
  }
  return strdup(s);
}

char *RelopName(RELOP_TYPE relop) {
  switch (relop) {
    case GEQ:
      return ">=";
    case LEQ:
      return "<=";
    case GE:
      return ">";
    case LE:
      return "<";
    case EQ:
      return "==";
    case NEQ:
      return "!=";
  }
}

void CalRefCnt(InterCodes root) {
  InterCode data = NULL;
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
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
    temp = temp->next;
  }
}

void DeleteIRNode(InterCodes to_del) {
  if (to_del->prev) {
    to_del->prev->next = to_del->next;
  }
  if (to_del->next) {
    to_del->next->prev = to_del->prev;
  }
}

InterCodes SimplifyAssign(InterCodes to_simp) {
  InterCodes new_code = MakeInterCodesNode();
  new_code->data->kind = I_ASSIGN;
  new_code->data->u.assign.x = to_simp->next->data->u.assign.x;
  new_code->data->u.assign.y = to_simp->data->u.assign.y;
  to_simp->data->u.assign.x->ref_cnt -= 2;

  new_code->prev = to_simp->prev;
  new_code->next = to_simp->next->next;
  if (to_simp->next->next) {
    to_simp->next->next->prev = new_code;
  }
  if (to_simp->prev) {
    to_simp->prev->next = new_code;
  }
  return new_code;
}

InterCodes RemoveTempVar(InterCodes root) {
  InterCode data = NULL;
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
    switch (data->kind) {
      case I_ASSIGN: {
        if (temp->next && temp->next->data->kind == I_ASSIGN &&
            data->u.assign.x->ref_cnt == 2 &&
            data->u.assign.x == temp->next->data->u.assign.y) {
          /* change t1 = v1, v2 = t1 to v2 = v1*/
          temp = SimplifyAssign(temp);
          data = temp->data;
        }
      }
      case I_ADDR:
      case I_DEREF_R:
      case I_DEREF_L: {
        /* x y */
        if (data->u.unary.x->ref_cnt == 1) DeleteIRNode(temp);
      } break;
      case I_ADD:
      case I_SUB:
      case I_MUL:
      case I_DIV: {
        /* x y z */
        if (data->u.binop.x->ref_cnt == 1) {
          DeleteIRNode(temp);
        }
      } break;
      case I_DEC: {
        /* DEC x [size] */
        if (data->u.dec.x->ref_cnt == 1) DeleteIRNode(temp);
      } break;
      default:
        break;
    }
    temp = temp->next;
  }
  return root;
}

InterCodes OptimIRCode(InterCodes root) {
  CalRefCnt(root);
  root = RemoveTempVar(root);
  return root;
}

void IRGen(InterCodes root) {
  InterCode data = NULL;
  while (root) {
    data = root->data;
    switch (data->kind) {
      case I_LABEL: {
        /* LABEL x : */
        fprintf(fout, "LABEL %s :\n", OpeName(data->u.label.x));
      } break;
      case I_FUNC: {
        /* FUNCTION f : */
        fprintf(fout, "FUNCTION %s :\n", OpeName(data->u.func.f));
      } break;
      case I_ASSIGN: {
        /* x := y */
        fprintf(fout, "%s := %s\n", OpeName(data->u.unary.x),
                OpeName(data->u.unary.y));
      } break;
      case I_ADD: {
        /* x := y + z */
        fprintf(fout, "%s := %s + %s\n", OpeName(data->u.binop.x),
                OpeName(data->u.binop.y), OpeName(data->u.binop.z));
      } break;
      case I_SUB: {
        /* x := y - z */
        fprintf(fout, "%s := %s - %s\n", OpeName(data->u.binop.x),
                OpeName(data->u.binop.y), OpeName(data->u.binop.z));
      } break;
      case I_MUL: {
        /* x := y * z */
        fprintf(fout, "%s := %s * %s\n", OpeName(data->u.binop.x),
                OpeName(data->u.binop.y), OpeName(data->u.binop.z));
      } break;
      case I_DIV: {
        /* x := y / z */
        fprintf(fout, "%s := %s / %s\n", OpeName(data->u.binop.x),
                OpeName(data->u.binop.y), OpeName(data->u.binop.z));
      } break;
      case I_ADDR: {
        /* x := &y */
        fprintf(fout, "%s := &%s\n", OpeName(data->u.unary.x),
                OpeName(data->u.unary.y));
      } break;
      case I_DEREF_R: {
        /* x := *y */
        fprintf(fout, "%s := *%s\n", OpeName(data->u.unary.x),
                OpeName(data->u.unary.y));
      } break;
      case I_DEREF_L: {
        /* *x := y */
        fprintf(fout, "*%s := %s\n", OpeName(data->u.unary.x),
                OpeName(data->u.unary.y));
      } break;
      case I_JMP: {
        /* GOTO x */
        fprintf(fout, "GOTO %s\n", OpeName(data->u.jmp.x));
      } break;
      case I_JMP_IF: {
        /* IF x [relop] y GOTO z */
        fprintf(fout, "IF %s %s %s GOTO %s\n", OpeName(data->u.jmp_if.x),
                RelopName(data->u.jmp_if.relop), OpeName(data->u.jmp_if.y),
                OpeName(data->u.jmp_if.z));
      } break;
      case I_RETURN: {
        /* RETURN x */
        fprintf(fout, "RETURN %s\n", OpeName(data->u.ret.x));
      } break;
      case I_DEC: {
        /* DEC x [size] */
        fprintf(fout, "DEC %s %d\n", OpeName(data->u.dec.x), data->u.dec.size);
      } break;
      case I_ARG: {
        /* ARG x */
        fprintf(fout, "ARG %s\n", OpeName(data->u.arg.x));
      } break;
      case I_CALL: {
        /* x := CALL f */
        fprintf(fout, "%s := CALL %s\n", OpeName(data->u.call.x),
                OpeName(data->u.call.f));
      } break;
      case I_PARAM: {
        /* PARAM x */
        fprintf(fout, "PARAM %s\n", OpeName(data->u.param.x));
      } break;
      case I_READ: {
        /* READ x */
        fprintf(fout, "READ %s\n", OpeName(data->u.read.x));
      } break;
      case I_WRITE: {
        /* WRITE x */
        fprintf(fout, "WRITE %s\n", OpeName(data->u.write.x));
      } break;
      default:
        assert(0);
    }
    root = root->next;
  }
}