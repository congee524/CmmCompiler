#include "symtab.h"

AsmCodes ACHead;
int* BasicBlock;
int BlockSize, BlockCnt;
RegDesp Reg[32];
AsmOpe RegOpe[32];
VarDesp vardesptable[0x3fff];

void Asm() {
  AsmInit();
  TranslateAsm(ICHead);
}

void AsmInit() {
  ACHead = (AsmCodes)malloc(sizeof(struct AsmCodes_));
  ACHead->data = NULL;
  ACHead->prev = ACHead->next = NULL;

  BlockSize = 64, BlockCnt = 0;
  BasicBlock = (int*)malloc(BlockSize * sizeof(int));

  for (int i = 0; i < 32; i++) {
    Reg[i].var = NULL;
    Reg[i].next_ref = -1;
    RegOpe[i] = (AsmOpe)malloc(sizeof(struct AsmOpe_));
    RegOpe[i]->kind = AO_REG;
    RegOpe[i]->u.no = i;
  }

  for (int i = 0; i < 0x3fff; i++) {
    vardesptable[i] = (VarDesp)malloc(sizeof(struct VarDesp_));
    vardesptable[i]->next = NULL;
  }

  // Reg 24, 25 served as immediate register
  for (int i = 24; i < 26; i++) {
    Reg[i].var = NewTemp();
    Reg[i].next_ref = -1;
  }
}

void AsmFromLabel(InterCodes IC) {
  InterCode data = IC->data;
  AsmOpe lab = NewLabAsmOpe(OpeName(data->u.label.x));
  AsmCodes label_code = MakeACNode(A_LABEL, lab);
  AddACCode(label_code);
}

void AsmFromAssign(InterCodes IC) {
  /* x = y[var|immd] */
  InterCode data = IC->data;
  Operand x = data->u.assign.x, y = data->u.assign.y;
  AsmOpe op_des = GetRegAsmOpe(GetReg(x, IC, false));
  if (y->kind == OP_CONST_INT) {
    AsmOpe op_immd = NewImmdAsmOpe(y->u.ival);
    AddACCode(MakeACNode(A_LI, op_des, op_immd));
  } else {
    AsmOpe op_src = GetRegAsmOpe(GetReg(y, IC, true));
    AddACCode(MakeACNode(A_MOVE, op_des, op_src));
  }
}

void AsmFromAddr(InterCodes IC) {
  /* x = &y */
  InterCode data = IC->data;
  Operand x = data->u.addr.x, y = data->u.addr.y;
  AsmOpe op_des = GetRegAsmOpe(GetReg(x, IC, false));
  VarDesp y_desp = LookupVarDesp(y);
  AsmOpe op_src = GetRegAsmOpe(Allocate(IC));
  AsmOpe op_fp = GetRegAsmOpe(_fp);
  AsmOpe op_off = NewImmdAsmOpe(op_des->offset);
  AddACCode(MakeACNode(A_ADDI, op_src, op_fp, op_off));
  AddACCode(MakeACNode(A_Move, op_des, op_src));
}

AsmFromDerefR(InterCodes IC) {
  /* x = *y */
  InterCode data = IC->data;
  Operand x = data->u.deref_r.x, y = data->u.deref_r.y;
  AsmOpe op_des = GetReg(x, IC, false);
  AsmOpe op_addr = NewAddrAsmOpe(GetReg(y, IC, true), 0);
  AddACCode(MakeACNode(A_LW, op_des, op_addr));
}

AsmFromDerefL(InterCodes IC) {
  /* *x = y */
  InterCode data = IC->data;
  Operand x = data->u.deref_l.x, y = data->u.deref_l.y;
  AsmOpe op_src = GetReg(y, IC, true);
  AsmOpe op_addr = NewAddrAsmOpe(GetReg(x, IC, true), 0);
  AddACCode(MakeACNode(A_SW, op_src, op_addr));
}

void AsmFromBinaryOpe(InterCodes IC) {
  /* x = y op z */
  InterCode data = IC->data;
  Operand x = data->u.binop.x, y = data->u.binop.y, z = data->u.binop.z;
  int ry = GetReg(y, IC, true), rz = GetReg(z, IC, true);
  int rx = GetReg(x, IC, false);
  AsmOpe x = GetRegAsmOpe(rx), y = GetRegAsmOpe(ry), z = GetRegAsmOpe(rz);
  switch (data->kind) {
    case I_ADD: {
      AddACCode(MakeACNode(A_ADD, x, y, z))
    } break;
    case I_SUB: {
      AddACCode(MakeACNode(A_SUB, x, y, z))
    } break;
    case I_MUL: {
      AddACCode(MakeACNode(A_MUL, x, y, z));
    } break;
    case I_DIV: {
      AddACCode(MakeACNode(A_DIV, y, z));
      AddACCode(MakeACNode(A_MFLO, x));
    } break;
    default:
      assert(0);
  }
  UpdateRegNextRef(IC);
  if (Reg[ry].next_ref == -1) FreeReg(ry);
  if (Reg[rz].next_ref == -1) FreeReg(rz);
}

void PushVarOnStack(Operand ope, int* offset) {
  if (ope->kind != OP_TEMP || ope->kind != OP_VAR) return;
  VarDesp var_desp = LookupVarDesp(ope);
  if (var_desp && var_desp->in_stack) return;
  var_desp = AddVarDespTab(ope);
  ShiftStackPointer(4);
  *offset = *offset + 4;
  var_desp->in_stack = true, var_desp->offset = -(*offset);
}

void PushAllLocalVarOnStack(InterCodes IC, int* offset) {
  InterCodes var_detector = IC;
  InterCode data = NULL;
  while (var_detector->next) {
    var_detector = var_detector->next;
    data = var_detector->data;
    if (data->kind == I_FUNC) break;
    swtich(data->kind) {
      case I_LABEL:
      case I_FUNC:
      case I_JMP: {
      } break;
      case I_DEC: {
        PushVarOnStack(data->u.dec.x, offset);
      } break; /* DEC x [size] */
      case I_CALL: {
        PushVarOnStack(data->u.call.x, offset);
      } break; /* x := CALL f */
      case I_RETURN:
      case I_ARG:
      case I_PARAM:
      case I_READ:
      case I_WRITE: {
        PushVarOnStack(data->u.unitary.x, offset);
      } break; /* ret|arg|param|read|write x */
      case I_ASSIGN:
      case I_ADDR:
      case I_DEREF_R:
      case I_DEREF_L: {
        PushVarOnStack(data->u.unary.x, offset);
        PushVarOnStack(data->u.unary.y);
      } break; /* []x = []y */
      case I_JMP_IF: {
        PushVarOnStack(data->u.jmp_if.x, offset);
        PushVarOnStack(data->u.jmp_if.y, offset);
      } break; /* IF x [relop] y GOTO z */
      case I_ADD:
      case I_SUB:
      case I_MUL:
      case I_DIV: {
        PushVarOnStack(data->u.binop.x, offset);
        PushVarOnStack(data->u.binop.y, offset);
        PushVarOnStack(data->u.binop.z, offset);
      } break; /* x = y op z */
      default:
        assert(0);
    }
  }
}

void AsmFromFunc(InterCodes IC) {
  InterCode data = IC->data;

  /* func_name: */
  AsmOpe op_func = NewLabAsmOpe(OpeName(data->u.func.f));
  AddACCode(MakeACNode(A_LABEL, op_func));

  /* Update sp fp */
  AsmOpe op_sp = GetRegAsmOpe(_sp), op_fp = GetRegAsmOpe(_fp);
  AddACCode(MakeACNode(A_MOVE, op_fp, op_sp));

  /* push param and local var in the function onto stack */
  int offset = 0;
  TODO()  // push param onto stack and reg
  PushAllLocalVarOnStack(IC, &offset);
}

void AsmFromArg(InterCodes IC) {
  /* void */
  /* Handle Args In Func Call */
}

InterCodes GetFuncArgs(InterCodes IC, int* args_cnt) {
  InterCodes args_ic = IC->prev;
  while (args_ic && args_ic->data->kind == I_ARG) {
    *args_cnt = *args_cnt + 1;
    args_ic = args_ic->prev;
  }
  return args_ic;
}

void AsmFromCall(InterCodes IC) {
  InterCode data = IC->data;

  /* push all variable onto stack */
  SpillAll();

  /* push args onto stack */
  int args_cnt = 0;
  InterCodes args_ic = GetFuncArgs(IC, &args_cnt);
  ShiftStackPointer(-4 * args_cnt);
  for (int i = 0; i < args_cnt; i++) {
    args_ic = args_ic->next;
    Operand _arg = args_ic->data.u.arg.x;
    AsmOpe op_arg = GetRegAsmOpe(GetReg(_arg, IC, true));
    AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 4 * i);
    AddACCode(MakeACNode(A_SW, op_arg, op_addr));
  }

  /* push return address onto stack */
  ShiftStackPointer(-4);
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_SW, op_ra, op_addr));

  /* push frame pointer onto stack */
  ShiftStackPointer(-4);
  AsmOpe op_fp = GetRegAsmOpe(_fp);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_SW, op_fp, op_addr));

  /* jal func */
  AsmOpe op_f = NewLabAsmOpe(OpeName(data->u.call.f));
  AddACCode(MakeACNode(A_JAL, op_f));

  /* recover fp and sp */
  AsmOpe op_fp = GetRegAsmOpe(_fp), op_sp = GetRegAsmOpe(_sp);
  AddACCode(MakeACNode(A_MOVE, op_sp, op_fp));
  AsmOpe op_addr = NewAddrAsmOpe(op_sp, 0);
  AddACCode(MakeACNode(A_lw, op_fp, op_addr));
  ShiftStackPointer(4);

  /* recover return address from stack */
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_LW, op_ra, op_addr));
  ShiftStackPointer(4);

  /* shrink stack for discarding args */
  ShiftStackPointer(4 * args_cnt);

  /* store the return value */
  AsmOpe op_v0 = GetRegAsmOpe(_v0);
  AsmOpe op_x = GetRegAsmOpe(GetReg(data->u.call.x, pre, false));
  AddACCode(MakeACNode(A_Move, op_x, op_v0));
}

void TranslateAsm(InterCodes IC) {
  InterCodes temp = IC;
  InterCode data = NULL;
  while (temp) {
    data = temp->data;
    switch (data->kind) {
      case I_LABEL: {
        AsmFromLabel(temp);
      } break; /* LABEL x : */
      case I_FUNC: {
        AsmFromFunc(temp);
      } break; /* FUNCTION f : */
      case I_ASSIGN: {
        AsmFromAssign(temp);
      } break; /* x := y */
      case I_ADD:
      case I_SUB:
      case I_MUL:
      case I_DIV: {
        AsmFromBinaryOpe(temp);
      } break; /* x := y [+-/*] z */
      case I_ADDR: {
        AsmFromAddr(temp);
      } break; /* x := &y */
      case I_DEREF_R: {
        AsmFromDerefR(temp);
      } break; /* x := *y */
      case I_DEREF_L: {
        AsmFromDerefL(temp);
      } break; /* *x := y */
      case I_JMP: {
        TODO()
      } break; /* GOTO x */
      case I_JMP_IF: {
        TODO()
      } break; /* IF x [relop] y GOTO z */
      case I_RETURN: {
        TODO()
      } break; /* RETURN x */
      case I_DEC: {
        TODO()
      } break; /* DEC x [size] */
      case I_ARG: {
        AsmFromArg(temp);
      } break; /* ARG x */
      case I_CALL: {
        AsmFromCall(temp);
      } break; /* x := CALL f */
      case I_PARAM: {
        TODO()
      } break; /* PARAM x */
      case I_READ: {
        TODO()
      } break; /* READ x */
      case I_WRITE: {
        TODO()
      } break; /* WRITE x */
      default:
        assert(0);
    }
    temp = temp->next;
  }
}

void AsmGen(AsmCodes root) {
  AsmCodes temp = root;
  AsmCode data = NULL;
  while (temp) {
    data = temp->data;
    switch (data->kind) {
      // TODO()
      default:
        assert(0);
    }
    temp = temp->next;
  }
}