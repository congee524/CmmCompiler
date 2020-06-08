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

/*============= semantic =============*/

void SemanticError(int error_num, int lineno, char* errtext) {
  fprintf(stdout, "Error type %d at Line %d: %s.\n", error_num, lineno,
          errtext);
}

char* TraceVarDec(Node* var_dec, int* dim, int* size, int* m_size) {
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

Type ConstArray(Type fund, int dim, int* size, int level) {
  if (level == dim) {
    return fund;
  }
  Type ret = (Type)malloc(sizeof(struct Type_));
  ret->kind = ARRAY;
  ret->u.array.elem = ConstArray(fund, dim, size, level + 1);
  ret->u.array.size = size[dim - level - 1];
  return ret;
}

Type LookupTab(char* name) {
  /* if find symbol failed, return NULL */
  unsigned int idx = hash(name);
  SymTable temp = symtable[idx];
  while (temp) {
    if (!strcmp(temp->name, name)) {
      return temp->type;
    }
    temp = temp->next;
  }
  FieldList struct_temp = symtabstack.StructHead, member = NULL;
  while (struct_temp->next) {
    struct_temp = struct_temp->next;
    member = struct_temp->type->u.structure;
    while (member) {
      if (!strcmp(member->name, name)) {
        return member->type;
      }
      member = member->next;
    }
  }
  return NULL;
}

void CreateLocalVar() {
  symtabstack.var_stack[symtabstack.depth++] =
      (SymTableNode)malloc(sizeof(struct SymTableNode_));
  symtabstack.var_stack[symtabstack.depth - 1]->var = NULL;
  symtabstack.var_stack[symtabstack.depth - 1]->next = NULL;
}

void DeleteLocalVar() {
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
}

SymTable AddSymTab(char* type_name, Type type, int lineno) {
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

FuncTable AddFuncTab(FuncTable func, int isDefined) {
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

int AddStructList(Type structure, int lineno) {
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
    sprintf(msg, "Duplicated variable or structure name \"%s\"",
            structure->u.struct_name);
    SemanticError(16, lineno, msg);
  }
}

int GetSize(Type type) {
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

int GetStructMemOff(Type type, char* name) {
  assert(type->kind == STRUCTURE);
  FieldList member = type->u.structure;
  int offsite = 0;
  while (member && strcmp(member->name, name)) {
    offsite += GetSize(member->type);
    member = member->next;
  }
  return offsite;
}

Type GetStruct(char* name) {
  FieldList temp = symtabstack.StructHead;
  while (temp->next) {
    temp = temp->next;
    if (!strcmp(temp->name, name)) {
      return temp->type;
    }
  }
  return NULL;
}

Type CheckFuncCall(char* func_name, FieldList para, int lineno) {
  Type ret = NULL;
  FuncTable temp = FuncHead;
  while (temp->next) {
    temp = temp->next;
    if (!strcmp(func_name, temp->name)) {
      ret = temp->ret_type;
      if (!CheckFieldEql(para, temp->para)) {
        char msg[128];
        sprintf(msg, "Mismatched parameters on calling function \"%s\"",
                func_name);
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

Type CheckStructField(FieldList structure, char* name) {
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

int CheckStructName(char* name) {
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

int CheckLogicOPE(Node* exp) {
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

int CheckArithOPE(Node* obj1, Node* obj2) {
  assert(strcmp(obj1->symbol, "Exp") == 0 && strcmp(obj2->symbol, "Exp") == 0);
  if (obj1->eval->type->kind == BASIC &&
      CheckTypeEql(obj1->eval->type, obj2->eval->type)) {
    return 1;
  }
  SemanticError(7, obj1->line, "Wrong operand object type");
  return 0;
}

int CheckSymTab(char* type_name, Type type, int lineno) {
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

int CheckFuncTab(FuncTable func, int isDefined) {
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
      if (!CheckTypeEql(func->ret_type, temp->ret_type) ||
          !CheckFieldEql(func->para, temp->para)) {
        char msg[128];
        sprintf(msg,
                "Conflicting function definitions or declarations at function "
                "\"%s\"",
                func->name);
        SemanticError(19, func->lineno, msg);
        ret = 0;
      }
      break;
    }
  }
  return ret;
}

int CheckTypeEql(Type t1, Type t2) {
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

int CheckFieldEql(FieldList f1, FieldList f2) {
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

void CheckFuncDefined() {
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
      Operand ope = LookupOpe(exp->child[0]->ident);
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
      ret->data->u.single.x = va_arg(ap, Operand);
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

InterCodes GotoCode(Operand label) { return MakeICNode(I_JMP, label); }

InterCodes LabelCode(Operand label) { return MakeICNode(I_LABEL, label); }

RELOP_TYPE GetRelop(Node* relop) {
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

Operand LookupOpe(char* name) {
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

char* RelopName(RELOP_TYPE relop) {
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

InterCodes RemoveNull(InterCodes root) {
  InterCode data = NULL;
  while (root && root->data == NULL) {
    root = root->next;
  }
  InterCodes temp = root;
  while (temp) {
    data = temp->data;
    if (data == NULL) {
      if (temp->prev) {
        temp->prev->next = temp->next;
      }
      if (temp->next) {
        temp->next->prev = temp->prev;
      }
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

void DeleteIRNode(InterCodes to_del) {
  if (to_del->prev) {
    to_del->prev->next = to_del->next;
  }
  if (to_del->next) {
    to_del->next->prev = to_del->prev;
  }
}

InterCodes SimplifyAssign(InterCodes to_simp) {
  // TODO()
  IC_TYPE kind = to_simp->data->kind;
  Operand x = to_simp->next->data->u.unary.x, y = to_simp->data->u.unary.y;
  InterCodes new_code = MakeICNode(kind, x, y);
  to_simp->data->u.unary.x->ref_cnt -= 2;

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
      case I_ASSIGN:
      case I_ADDR:
      case I_DEREF_R:
      case I_ADD:
      case I_SUB:
      case I_MUL:
      case I_DIV: {
        if (temp->next && temp->next->data->kind == I_ASSIGN &&
            data->u.unary.x->ref_cnt == 2 &&
            data->u.unary.x == temp->next->data->u.unary.y) {
          /* change t1 = v1, v2 = t1 to v2 = v1*/
          // temp = SimplifyAssign(temp);
          // data = temp->data;
          data->u.unary.x = temp->next->data->u.unary.x;
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
