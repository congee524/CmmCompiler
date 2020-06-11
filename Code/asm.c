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

void PushLocalVarOnStack(InterCodes IC) { TODO() }

void AsmFromFunc(InterCodes IC) {
  InterCode data = IC->data;
  TODO()
  PushLocalVarOnStack(IC);
}

void AsmFromCall(InterCodes IC) { SpillAll(); }

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
        TODO()
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