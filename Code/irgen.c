#include "symtab.h"

char* OpeName(Operand ope)
{
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
        sprintf(s, "a%d", ope->u.var_no);
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

char* RelopName(RELOP_TYPE relop)
{
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

void IRGen(InterCodes root)
{
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
            fprintf(fout, "%s := %s\n", OpeName(data->u.unary.x), OpeName(data->u.unary.y));
        } break;
        case I_ADD: {
            /* x := y + z */
            fprintf(fout, "%s := %s + %s\n", OpeName(data->u.binop.x), OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break;
        case I_SUB: {
            /* x := y - z */
            fprintf(fout, "%s := %s - %s\n", OpeName(data->u.binop.x), OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break;
        case I_MUL: {
            /* x := y * z */
            fprintf(fout, "%s := %s * %s\n", OpeName(data->u.binop.x), OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break;
        case I_DIV: {
            /* x := y / z */
            fprintf(fout, "%s := %s / %s\n", OpeName(data->u.binop.x), OpeName(data->u.binop.y), OpeName(data->u.binop.z));
        } break;
        case I_ADDR: {
            /* x := &y */
            fprintf(fout, "%s := &%s\n", OpeName(data->u.unary.x), OpeName(data->u.unary.y));
        } break;
        case I_DEREF_R: {
            /* x := *y */
            fprintf(fout, "%s := *%s\n", OpeName(data->u.unary.x), OpeName(data->u.unary.y));
        } break;
        case I_DEREF_L: {
            /* *x := y */
            fprintf(fout, "*%s := %s\n", OpeName(data->u.unary.x), OpeName(data->u.unary.y));
        } break;
        case I_JMP: {
            /* GOTO x */
            fprintf(fout, "GOTO %s\n", OpeName(data->u.jmp.x));
        } break;
        case I_JMP_IF: {
            /* IF x [relop] y GOTO z */
            fprintf(fout, "IF %s %s %s GOTO %s\n", OpeName(data->u.jmp_if.x), RelopName(data->u.jmp_if.relop), OpeName(data->u.jmp_if.y), OpeName(data->u.jmp_if.z));
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
            fprintf(fout, "%s := CALL %s\n", OpeName(data->u.call.x), OpeName(data->u.call.f));
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