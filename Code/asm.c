#include "symtab.h"

AsmCodes ACHead;
int* BasicBlock;
int BlockSize, BlockCnt;
RegDesp Reg[32];
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
  }

  for (int i = 0; i < 0x3fff; i++) {
    vardesptable[i] = (VarDesp)malloc(sizeof(struct VarDesp_));
    vardesptable[i]->next = NULL;
  }
}

void AsmFromLabel(InterCode data) {
  AsmOpe lab = NewLabAsmOpe(OpeName(data->u.label.x));
  AsmCodes label_code = MakeACNode(A_LABEL, lab);
  AddACCode(label_code);
}

void TranslateAsm(InterCodes IC) {
  InterCodes temp = IC;
  InterCode data = NULL;
  while (temp) {
    data = temp->data;
    switch (data->kind) {
      case I_LABEL: {
        AsmFromLabel(data);
      } break; /* LABEL x : */
      case I_FUNC: {
        TODO()
      } break; /* FUNCTION f : */
      case I_ASSIGN: {
        TODO()
      } break; /* x := y */
      case I_ADD: {
        TODO()
      } break; /* x := y + z */
      case I_SUB: {
        TODO()
      } break; /* x := y - z */
      case I_MUL: {
        TODO()
      } break; /* x := y * z */
      case I_DIV: {
        TODO()
      } break; /* x := y / z */
      case I_ADDR: {
        TODO()
      } break; /* x := &y */
      case I_DEREF_R: {
        TODO()
      } break; /* x := *y */
      case I_DEREF_L: {
        TODO()
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
        TODO()
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