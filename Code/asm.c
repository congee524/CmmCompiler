#include "symtab.h"

AsmCodes ACHead;
int* BasicBlock;
int BlockSize, BlockCnt;
RegDesp Reg[32];
AsmOpe RegOpe[32];
VarDesp vardesptable[0x3fff];

void AsmFromLabel(InterCodes IC) {
  AsmOpe lab = NewLabAsmOpe(OpeName(IC->data->u.label.x));
  AddACCode(MakeACNode(A_LABEL, lab));
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
  AsmOpe op_src = GetRegAsmOpe(AllocateImmd(IC));
  AsmOpe op_fp = GetRegAsmOpe(_fp);
  AsmOpe op_off = NewImmdAsmOpe(op_des->offset);
  AddACCode(MakeACNode(A_ADDI, op_src, op_fp, op_off));
  AddACCode(MakeACNode(A_Move, op_des, op_src));
}

void AsmFromDerefR(InterCodes IC) {
  /* x = *y */
  InterCode data = IC->data;
  Operand x = data->u.deref_r.x, y = data->u.deref_r.y;
  AsmOpe op_des = GetReg(x, IC, false);
  AsmOpe op_addr = NewAddrAsmOpe(GetReg(y, IC, true), 0);
  AddACCode(MakeACNode(A_LW, op_des, op_addr));
}

void AsmFromDerefL(InterCodes IC) {
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

void AsmFromFunc(InterCodes IC) {
  /* func_name: */
  AsmOpe op_func = NewLabAsmOpe(OpeName(IC->data->u.func.f));
  AddACCode(MakeACNode(A_LABEL, op_func));

  /* update fp */
  AsmOpe op_sp = GetRegAsmOpe(_sp), op_fp = GetRegAsmOpe(_fp);
  AddACCode(MakeACNode(A_MOVE, op_fp, op_sp));

  /* push param and local var of the function onto stack */
  int offset = 0;
  PushAllParamOnStack(IC, &offset);
  PushAllLocalVarOnStack(IC, &offset);
}

void AsmFromArg(InterCodes IC) {
  /* void */
  /* Handle Args In I_Call */
}

void AsmFromParam(InterCodes IC) {
  /* void */
  /* Handle Params In I_FUNC */
}

void AsmFromDec(InterCodes IC) {
  /* void */
  /* Handle Dec In PushAllLocalVarOnStack */
}

void AsmFromJmp(InterCodes IC) {
  /* Goto x */
  AsmOpe op_lab = NewLabAsmOpe(OpeName(IC->data->u.jmp.x));
  AddACCode(A_J, op_lab);
}

void AsmFromJmpIf(InterCodes IC) {
  /* IF x [relop] y GOTO z */
  InterCode data = IC->data;
  AsmOpe op_x = GetRegAsmOpe(GetReg(data->u.jmp_if.x, IC, true));
  AsmOpe op_y = GetRegAsmOpe(GetReg(data->u.jmp_if.y, IC, true));
  AsmOpe op_lab = NewLabAsmOpe(OpeName(IC->data->u.jmp_if.z));
  switch (data->u.jmp_if.relop) {
    case EQ: {
      AddACCode(MakeACNode(A_BEQ, op_x, op_y, op_lab));
    } break; /* == */
    case NEQ: {
      AddACCode(MakeACNode(A_BNE, op_x, op_y, op_lab));
    } break; /* != */
    case GT: {
      AddACCode(MakeACNode(A_BGT, op_x, op_y, op_lab));
    } break; /* > */
    case LT: {
      AddACCode(MakeACNode(A_BLT, op_x, op_y, op_lab));
    } break; /* < */
    case GEQ: {
      AddACCode(MakeACNode(A_BGE, op_x, op_y, op_lab));
    } break; /* >= */
    case LEQ: {
      AddACCode(MakeACNode(A_BLE, op_x, op_y, op_lab));
    } break; /* <= */
  }
}

void AsmFromReturn(InterCodes IC) {
  /* move the return value to $v0 */
  InterCode data = IC->data;
  AsmOpe op_ret = GetRegAsmOpe(GetReg(data->u.ret.x, IC, true));
  AsmOpe op_v0 = GetRegAsmOpe(_v0);
  AddACCode(MakeACNode(A_MOVE, op_v0, op_ret));
  SpillAll();
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AddACCode(MakeACNode(A_JR, op_ra));
}

void AsmFromRead(InterCodes IC) {
  /* push return address onto stack */
  ShiftStackPointer(-4);
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_SW, op_ra, op_addr));

  /* jal read */
  AddACCode(MakeACNode(A_JAL, NewLabAsmOpe("read")));

  /* recover return address from stack */
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_LW, op_ra, op_addr));
  ShiftStackPointer(4);

  /* store the return value */
  Operand read_x = IC->data->u.read.x;
  AsmOpe op_v0 = GetRegAsmOpe(_v0);
  AsmOpe op_read_x = GetRegAsmOpe(GetReg(read_x, IC, false));
  AddACCode(MakeACNode(A_Move, op_read_x, op_v0));
}

void AsmFromWrite(InterCodes IC) {
  /* move the param of write to $a0 */
  AsmOpe op_a0 = GetRegAsmOpe(_a0);
  Operand write_x = IC->data->u.write.x;
  AsmOpe op_write_x = GetRegAsmOpe(GetReg(write_x, IC, true));
  AddACCode(MakeACNode(A_MOVE, op_a0, op_write_x));

  /* push return address onto stack */
  ShiftStackPointer(-4);
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_SW, op_ra, op_addr));

  /* jal write */
  AddACCode(MakeACNode(A_JAL, NewLabAsmOpe("write")));

  /* recover return address from stack */
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_LW, op_ra, op_addr));
  ShiftStackPointer(4);
}

void AsmFromCall(InterCodes IC) {
  InterCode data = IC->data;

  /* push all variable onto stack */
  SpillAll();

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

  /* jal func */
  AsmOpe op_f = NewLabAsmOpe(OpeName(data->u.call.f));
  AddACCode(MakeACNode(A_JAL, op_f));

  /* recover sp */
  AsmOpe op_fp = GetRegAsmOpe(_fp), op_sp = GetRegAsmOpe(_sp);
  AddACCode(MakeACNode(A_MOVE, op_sp, op_fp));

  /* shrink stack for discarding args */
  ShiftStackPointer(4 * args_cnt);

  /* recover fp */
  AsmOpe op_addr = NewAddrAsmOpe(op_sp, 0);
  AddACCode(MakeACNode(A_lw, op_fp, op_addr));
  ShiftStackPointer(4);

  /* recover return address from stack */
  AsmOpe op_ra = GetRegAsmOpe(_ra);
  AsmOpe op_addr = NewAddrAsmOpe(GetRegAsmOpe(_sp), 0);
  AddACCode(MakeACNode(A_LW, op_ra, op_addr));
  ShiftStackPointer(4);

  /* store the return value */
  AsmOpe op_v0 = GetRegAsmOpe(_v0);
  AsmOpe op_x = GetRegAsmOpe(GetReg(data->u.call.x, IC, false));
  AddACCode(MakeACNode(A_Move, op_x, op_v0));
}

void AsmHeadGen() {
  fprintf(fout, ".data\n");
  fprintf(fout, "_prompt: .asciiz \"Enter an Integer:\"\n");
  fprintf(fout, "_ret: .asciiz \"\\n\"\n");
  fprintf(fout, ".globl func@main\n");
  fprintf(fout, ".text\n");
  fprintf(fout, "read:\n");
  fprintf(fout, "  li $v0, 4\n");
  fprintf(fout, "  la $a0, _prompt\n");
  fprintf(fout, "  syscall\n");
  fprintf(fout, "  li $v0, 5\n");
  fprintf(fout, "  syscall\n");
  fprintf(fout, "  jr $ra\n\n");
  fprintf(fout, "write:\n");
  fprintf(fout, "  li $v0, 1\n");
  fprintf(fout, "  syscall\n");
  fprintf(fout, "  li $v0, 4\n");
  fprintf(fout, "  la $a0, _ret\n");
  fprintf(fout, "  syscall\n");
  fprintf(fout, "  move $v0, $0\n");
  fprintf(fout, "  jr $ra\n");
}

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
        AsmFromJmp(temp);
      } break; /* GOTO x */
      case I_JMP_IF: {
        AsmFromJmpIf(temp);
      } break; /* IF x [relop] y GOTO z */
      case I_RETURN: {
        AsmFromReture(temp);
      } break; /* RETURN x */
      case I_DEC: {
        AsmFromDec(temp);
      } break; /* DEC x [size] */
      case I_ARG: {
        AsmFromArg(temp);
      } break; /* ARG x */
      case I_CALL: {
        AsmFromCall(temp);
      } break; /* x := CALL f */
      case I_PARAM: {
        AsmFromParam(temp);
      } break; /* PARAM x */
      case I_READ: {
        AsmFromRead();
      } break; /* READ x */
      case I_WRITE: {
        AsmFromWrite();
      } break; /* WRITE x */
      default:
        assert(0);
    }
    temp = temp->next;
  }
}

void AsmGen(AsmCodes root) {
  AsmHeadGen();
  AsmCodes temp = root->next;
  AsmCode data = NULL;
  char *op1_name, *op2_name, *op3_name;
  while (temp) {
    data = temp->data;
    switch (data->kind) {
      case A_LABEL: {
        op1_name = AsmOpeName(data->u.unary.op1);
        if (!strncmp(op1_name, "func@", 5)) {
          fprintf(fout, "\n%s:\n", op1_name);
        } else {
          fprintf(fout, "%s:\n", op1_name);
        }
      } break; /* lab: */
      case A_J: {
        op1_name = AsmOpeName(data->u.unary.op1);
        fprintf(fout, "  j %s\n", op1_name);
      } break; /* j lab */
      case A_JAL: {
        op1_name = AsmOpeName(data->u.unary.op1);
        fprintf(fout, "  jal %s\n", op1_name);
      } break; /* jal lab */
      case A_JR: {
        op1_name = AsmOpeName(data->u.unary.op1);
        fprintf(fout, "  jr %s\n", op1_name);
      } break; /* jr src1 */
      case A_MFLO: {
        op1_name = AsmOpeName(data->u.unary.op1);
        fprintf(fout, "  mflo %s\n", op1_name);
      } break; /* mflo des */
      case A_DIV: {
        op1_name = AsmOpeName(data->u.binary.op1);
        op2_name = AsmOpeName(data->u.binary.op2);
        fprintf(fout, "  div %s, %s\n", op1_name, op2_name);
      } break; /* div src1, reg2 */
      case A_LI: {
        op1_name = AsmOpeName(data->u.binary.op1);
        op2_name = AsmOpeName(data->u.binary.op2);
        fprintf(fout, "  li %s, %s\n", op1_name, op2_name);
      } break; /* li des, immd */
      case A_MOVE: {
        op1_name = AsmOpeName(data->u.binary.op1);
        op2_name = AsmOpeName(data->u.binary.op2);
        fprintf(fout, "  move %s, %s\n", op1_name, op2_name);
      } break; /* move des, src1 */
      case A_LW: {
        op1_name = AsmOpeName(data->u.binary.op1);
        op2_name = AsmOpeName(data->u.binary.op2);
        fprintf(fout, "  lw %s, %s\n", op1_name, op2_name);
      } break; /* lw des, addr */
      case A_SW: {
        op1_name = AsmOpeName(data->u.binary.op1);
        op2_name = AsmOpeName(data->u.binary.op2);
        fprintf(fout, "  sw %s, %s\n", op1_name, op2_name);
      } break; /* sw src1, addr */
      case A_ADDI: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  addi %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* addi des, src1, immd */
      case A_ADD: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  add %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* add des, src1, src2 */
      case A_SUB: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  sub %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* sub des, src1, src2 */
      case A_MUL: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  mul %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* mul des, src1, src2 */
      case A_BEQ: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  beq %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* beq src1, src2, lab */
      case A_BNE: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  bne %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* bne src1, src2, lab */
      case A_BGT: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  bgt %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* bgt src1, src2, lab */
      case A_BLT: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  blt %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* blt src1, src2, lab */
      case A_BGE: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  bge %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* bge src1, src2, lab */
      case A_BLE: {
        op1_name = AsmOpeName(data->u.ternary.op1);
        op2_name = AsmOpeName(data->u.ternary.op2);
        op3_name = AsmOpeName(data->u.ternary.op3);
        fprintf(fout, "  ble %s, %s, %s\n", op1_name, op2_name, op3_name);
      } break; /* ble src1, src2, lab */
      default:
        assert(0);
    }
    temp = temp->next;
  }
}