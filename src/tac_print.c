/*
 * tac_print.c — impresión de representación intermedia de tres direcciones.
 * Usa: tac.h, inttypes.h. Lo usan pruebas y la futura bandera de impresión.
 * Referencias: Aho §6.2.1, §8.4; enunciado §4.1.
 */
#include "tac.h"
#include <inttypes.h>

/* operand — imprime por valor, sin direcciones del proceso ni estado global. */
static void operand(FILE *out, TacOperand o) {
    switch (o.kind) {
    case TAC_NONE: break;
    case TAC_TEMP: fprintf(out, "t%zu", o.value.id); break;
    case TAC_VARIABLE: fputs(o.value.symbol->name, out); break;
    case TAC_INTEGER: fprintf(out, "%" PRId64, o.value.integer); break;
    case TAC_LABEL: fprintf(out, "B%zu", o.value.id); break;
    }
}
/* instruction — usa formato de tabla; requiere una cuádrupla válida. */
static void instruction(FILE *out, const TacInstruction *i) {
    const TacOpInfo *p = tac_op_info(i->op);
    switch (p->format) {
    case TAC_FMT_COPY:
    case TAC_FMT_BINARY:
    case TAC_FMT_UNARY:
        operand(out, i->dest); fputs(" = ", out);
        if (p->format == TAC_FMT_UNARY) fprintf(out, "%s ", p->text);
        operand(out, i->src1);
        if (p->format == TAC_FMT_BINARY) {
            fprintf(out, " %s ", p->text); operand(out, i->src2);
        }
        break;
    case TAC_FMT_LOAD:
        operand(out, i->dest); fputs(" = ", out); operand(out, i->src1);
        fputc('[', out); operand(out, i->src2); fputc(']', out); break;
    case TAC_FMT_STORE:
        operand(out, i->dest); fputc('[', out); operand(out, i->src1);
        fputs("] = ", out); operand(out, i->src2); break;
    case TAC_FMT_CRYPTO:
        fputs("crypto ", out); operand(out, i->dest); fputs(", ", out);
        operand(out, i->src1); fputs(", ", out); operand(out, i->src2); break;
    case TAC_FMT_PARAM:
    case TAC_FMT_RETURN:
        fputs(p->text, out);
        if (i->src1.kind != TAC_NONE) { fputc(' ', out); operand(out, i->src1); }
        break;
    case TAC_FMT_CALL:
        if (i->dest.kind != TAC_NONE) { operand(out, i->dest); fputs(" = ", out); }
        fputs("call ", out); operand(out, i->src1); fputs(", ", out);
        operand(out, i->src2); break;
    case TAC_FMT_GOTO:
        fputs("goto ", out); operand(out, i->dest); break;
    case TAC_FMT_IF:
        fputs("if ", out); operand(out, i->src1); fprintf(out, " %s ", p->text);
        operand(out, i->src2); fputs(" goto ", out); operand(out, i->dest); break;
    }
    fputc('\n', out);
}
/* tac_print — recorre en orden de inserción; etiquetas locales a cada función. */
void tac_print(FILE *out, const TacProgram *p) {
    for (const TacFunction *f = p->first; f; f = f->next) {
        fprintf(out, "function %s (entry B%zu)\n", f->name, f->entry->label);
        for (const TacBlock *b = f->first; b; b = b->next) {
            fprintf(out, "B%zu: successors [", b->label);
            for (size_t k = 0; k < b->successor_count; ++k)
                fprintf(out, "%sB%zu", k ? ", " : "", b->successors[k]->label);
            fputs("]\n", out);
            for (const TacInstruction *i = b->first; i; i = i->next) {
                fputs("  ", out); instruction(out, i);
            }
        }
        fputc('\n', out);
    }
}
