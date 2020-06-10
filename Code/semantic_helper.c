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
