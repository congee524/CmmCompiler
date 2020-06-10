#include "symtab.h"

static unsigned int hash_ope(Operand ope) {
  assert(ope->kind == OP_TEMP || ope->kind == OP_VAR);
  int id = ope->u.var_no << 1;
  if (ope->kind == OP_VAR) id |= 1;
  return id % 0x3fff;
}

AsmCodes MakeACNode(AC_TYPE kind, ...) {
  AsmCodes ret = (AsmCodes)malloc(sizeof(struct AsmCodes_));
  ret->prev = ret->next = NULL;
  ret->data = (AsmCode)malloc(sizeof(struct AsmCode_));
  ret->data->kind = kind;
  va_list ap;
  va_start(ap, kind);
  switch (kind) {
    case A_LABEL:
    case A_J:
    case A_JAL:
    case A_JR:
    case A_MFLO: {
      ret->data->u.unary.op1 = va_arg(ap, AsmOpe);
    } break;
    case A_DIV:
    case A_LI:
    case A_MOVE:
    case A_LW:
    case A_SW: {
      ret->data->u.binary.op1 = va_arg(ap, AsmOpe);
      ret->data->u.binary.op2 = va_arg(ap, AsmOpe);
    } break;
    case A_ADDI:
    case A_ADD:
    case A_SUB:
    case A_MUL:
    case A_BEQ:
    case A_BNE:
    case A_BGT:
    case A_BLT:
    case A_BGE:
    case A_BLE: {
      ret->data->u.ternary.op1 = va_arg(ap, AsmOpe);
      ret->data->u.ternary.op2 = va_arg(ap, AsmOpe);
      ret->data->u.ternary.op3 = va_arg(ap, AsmOpe);
    } break;
    default:
      assert(0);
  }
  return ret;
}

AsmOpe NewLabAsmOpe(char *lab) {
  AsmOpe ret = (AsmOpe)malloc(sizeof(struct AsmOpe_));
  ret->kind = AO_LABEL;
  ret->ident = lab;
  return ret;
}

void AddACCode(AsmCodes code) {
  static AsmCodes last = NULL;
  if (last == NULL) {
    last = ACHead;
    assert(last->next == NULL);
  }
  if (code) {
    last->next = code;
    code->prev = last;
    while (last->next) last = last->next;
  }
}

void ExpandBlock() {
  BlockSize += 64;
  BasicBlock = (int *)realloc(BasicBlock, (BlockSize) * sizeof(int));
}

void DividBlock(InterCodes IC) {
  InterCodes temp = IC;
  InterCode data = NULL;
  int lineno = 0;
  BasicBlock[BlockCnt++] = lineno;
  while (temp) {
    data = temp->data;
    temp->lineno = lineno++;
    switch (data->kind) {
      case I_LABEL:
      case I_CALL: {
        if (BlockCnt >= BlockSize) ExpandBlock();
        BasicBlock[BlockCnt++] = temp->lineno;
      } break;
      case I_JMP:
      case I_JMP_IF: {
        if (BlockCnt >= BlockSize) ExpandBlock();
        BasicBlock[BlockCnt++] = temp->lineno + 1;
      } break;
      default:
        break;
    }
    temp = temp->next;
  }
  if (BlockCnt >= BlockSize) ExpandBlock();
  BasicBlock[BlockCnt++] = lineno;
}

bool IsSameOpe(Operand x, Operand y) {
  if (x->kind != y->kind) return false;
  if (x->kind == OP_TEMP || x->kind == OP_VAR || x->kind == OP_LABEL) {
    return x->u.var_no == y->u.var_no;
  } else if (x->kind == OP_FUNC) {
    return !strcmp(x->u.id, y->u.id);
  } else if (x->kind == OP_CONST_INT) {
    return x->u.ival == y->u.ival;
  } else if (x->kind == OP_CONST_FLOAT) {
    return x->u.fval == y->u.fval;
  }
}

VarDesp LookupVarDesp(Operand ope) {
  assert(ope->kind == OP_TEMP || ope->kind == OP_VAR);
  VarDesp ret = vardesptable[hash_ope(ope)];
  while (ret->next) {
    ret = ret->next;
    if (IsSameOpe(ope, temp->var)) return ret;
  }
  return NULL;
}

void UpdateOpeNextRef(Operand ope, int lineno) {
  if (ope->kind != OP_TEMP || ope->kind != OP_VAR) return;
  for (int i = 8; i < 26; i++) {
    if (Reg[i].next_ref == -1 && IsSameOpe(ope, Reg[i].var)) {
      Reg[i].next_ref = lineno;
      return;
    }
  }
}

void UpdateRegNextRef(InterCodes pre) {
  int block_bound = -1;
  for (int i = 0; i < BlockCnt; i++) {
    if (BasicBlock[i] > pre->lineno) {
      block_bound = BasicBlock[i];
      break;
    }
  }
  for (int i = 8; i < 26; i++) {
    Reg[i].next_ref = -1;
  }
  InterCodes temp = pre;
  InterCode data = NULL;
  int line = -1;
  while (temp && temp->lineno < block_bound) {
    data = temp->data;
    line = temp->lineno;
    switch (data->kind) {
      case I_LABEL:
      case I_JMP:
      case I_RETURN:
      case I_ARG:
      case I_PARAM:
      case I_READ:
      case I_WRITE:
      case I_FUNC: {
        UpdateOpeNextRef(data->u.unitary.x, line);
      } break;
      case I_ASSIGN:
      case I_ADDR:
      case I_DEREF_L:
      case I_DEREF_R:
      case I_CALL: {
        UpdateOpeNextRef(data->u.unary.x, line);
        UpdateOpeNextRef(data->u.unary.y, line);
      } break;
      case I_ADD:
      case I_SUB:
      case I_MUL:
      case I_DIV: {
        UpdateOpeNextRef(data->u.binop.x, line);
        UpdateOpeNextRef(data->u.binop.y, line);
        UpdateOpeNextRef(data->u.binop.z, line);
      } break;
      case I_DEC: {
        UpdateOpeNextRef(data->u.dec.x, line);
      } break;
      case I_JMP_IF: {
        UpdateOpeNextRef(data->u.jmp_if.x, line);
        UpdateOpeNextRef(data->u.jmp_if.y, line);
        UpdateOpeNextRef(data->u.jmp_if.z, line);
      } break;
      default:
        assert(0);
    }
    temp = temp->next;
  }
}

int GetInactiveReg(InterCodes pre) {
  UpdateRegNextRef(pre);
  int ret = 8, temp_ref = Reg[8].next_ref;
  for (int i = 8; i < 26; i++) {
    if (Reg[i].next_ref == -1) {
      ret = i;
      break;
    }
    if (Reg[i].next_ref > temp_ref) {
      ret = i, temp_ref = Reg[i].next_ref;
    }
  }
  Spill(ret);
  return ret;
}

void Spill(int reg_no) {
  Operand ope = Reg[reg_no].var;
  assert(ope != NULL && (ope->kind == OP_TEMP || ope->kind == OP_VAR));
  TODO()
}

int Allocate(Operand ope, InterCodes pre) {
  for (int i = 8; i < 26; i++) {
    if (Reg[i].var == NULL) return i;
  }
  int ret = GetInactiveReg(pre);
}

int Ensure(Operand ope, InterCodes pre) {
  int ret = -1;
  VarDesp ope_desp = LookupVarDesp(ope);
  assert(ope_desp != NULL);
  if (ope_desp->in_reg) {
    ret = ope_desp->reg_no;
  } else {
    ret = Allocate(ope);
    // emit MIPS32 code [lw result, x]
    TODO()
  }
  return ret;
}