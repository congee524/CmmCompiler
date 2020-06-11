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
  ret->u.ident = lab;
  return ret;
}

AsmOpe GetRegAsmOpe(int reg_no) {
  assert(reg_no >= 0 && reg_no < 32);
  return RegOpe[reg_no];
}

AsmOpe NewAddrAsmOpe(AsmOpe addr, int off) {
  AsmOpe ret = (AsmOpe)malloc(sizeof(struct AsmOpe_));
  ret->kind = AO_ADDR;
  ret->u.addr = addr;
  ret->u.off = off;
  return ret;
}

AsmOpe NewImmdAsmOpe(int ival) {
  AsmOpe ret = (AsmOpe)malloc(sizeof(struct AsmOpe_));
  ret->kind = AO_IMMD;
  ret->u.ival = ival;
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

VarDesp AddVarDespTab(Operand ope) {
  assert(ope->kind == OP_TEMP || ope->kind == OP_VAR);
  VarDesp ret = vardesptable[hash_ope(ope)];
  while (ret->next) {
    ret = ret->next;
    if (IsSameOpe(ope, temp->var)) assert(0);
  }
  ret->next = (VarDesp)malloc(sizeof(struct VarDesp_));
  ret = ret->next;
  ret->in_stack = false, ret->in_reg = false;
  ret->var = ope, ret->offset = 0, ret->next = NULL;
  return ret;
}

void UpdateOpeNextRef(Operand ope, int lineno) {
  if (ope->kind != OP_TEMP || ope->kind != OP_VAR) return;
  for (int i = 8; i < 24; i++) {
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
  for (int i = 8; i < 24; i++) {
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

void ShiftStackPointer(int offset) {
  if (offset == 0) return;
  AsmOpe op_sp = GetRegAsmOpe(_sp);
  AsmOpe op_off = NewImmdAsmOpe(offset);
  AddACCode(MakeACNode(A_ADDI, op_sp, op_sp, op_off));
}

int GetInactiveReg(InterCodes pre) {
  UpdateRegNextRef(pre);
  int ret = 8, temp_ref = Reg[8].next_ref;
  for (int i = 8; i < 24; i++) {
    if (Reg[i].next_ref == -1) return i;
    if (Reg[i].next_ref > temp_ref) {
      ret = i, temp_ref = Reg[i].next_ref;
    }
  }
  return ret;
}

void Spill(int reg_no) {
  Operand ope = Reg[reg_no].var;
  assert(ope != NULL && (ope->kind == OP_TEMP || ope->kind == OP_VAR));
  VarDesp ope_desp = LookupVarDesp(ope);
  assert(ope_desp->in_stack);
  AsmOpe op_src = GetRegAsmOpe(reg_no);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_fp), ope_desp->offset);
  AsmCodes sw_code = MakeACNode(A_SW, op_src, op_addr);
  AddACCode(sw_code);
  Reg[reg_no].var = NULL, Reg[reg_no].next_ref = -1;
  ope_desp->in_reg = false, ope_desp->reg_no = -1;
}

void SpillAll() {
  for (int i = 8; i < 24; i++) {
    if (Reg[i].var) Spill(i);
  }
}

int Allocate(InterCodes pre) {
  for (int i = 8; i < 24; i++) {
    if (Reg[i].var == NULL) return i;
  }
  int ret = GetInactiveReg(pre);
  Spill(ret);
  return ret;
}

int Ensure(Operand ope, InterCodes pre, bool isload) {
  int ret = -1;
  VarDesp ope_desp = LookupVarDesp(ope);
  assert(ope_desp != NULL);
  if (ope_desp->in_reg) {
    ret = ope_desp->reg_no;
  } else {
    ret = Allocate(pre);
    ope_desp->in_reg = true, ope_desp->reg_no = ret;
    if (isload) {
      // emit MIPS32 code [lw result, x]
      assert(ope_desp->in_stack);
      AsmOpe op_des = GetRegAsmOpe(ret);
      AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_fp), ope_desp->offset);
      AsmCodes lw_code = MakeACNode(A_LW, op_des, op_addr);
      AddACCode(lw_code);
    }
  }
  return ret;
}

int AllocateImmd(InterCodes pre) {
  int immd1_next = Reg[_immd1].next_ref;
  int immd2_next = Reg[_immd2].next_ref;
  if (immd1_next == -1 || immd1_next < immd2_next) {
    assert(immd1_next < pre->lineno);
    Reg[_immd1].next_ref = pre->lineno;
    return _immd1;
  } else {
    assert(immd2_next < pre->lineno);
    Reg[_immd2].next_ref = pre->lineno;
    return _immd2;
  }
}

int GetReg(Operand ope, InterCodes pre, bool isload) {
  int ret = -1;
  if (ope->kind == OP_TEMP || ope->kind == OP_VAR) {
    ret = Ensure(ope, pre, isload);
    Reg[ret].var = ope;
  } else {
    assert(ope->kind == OP_CONST_INT && isload);
    ret = AllocateImmd(pre);
    AsmOpe op_des = GetRegAsmOpe(ret);
    AsmOpe op_immd = NewImmdAsmOpe(ope->u.ival);
    AsmCodes li_code = MakeACNode(A_LI, op_des, op_immd);
    AddACCode(li_code);
  }
  return ret;
}

void FreeReg(int reg_no) {
  VarDesp ope_desp = LookupVarDesp(Reg[reg_no].var);
  assert(ope_desp->in_reg);
  ope_desp->in_reg = false;
  ope_desp->reg_no = -1;
  Reg[reg_no].var = NULL;
  Reg[reg_no].next_ref = -1;
}