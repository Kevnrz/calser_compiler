/*
 * tac_print.c — impresión estable de representación intermedia de tres direcciones.
 * Usa: tac.h y biblioteca C. Lo usan pruebas y la futura bandera de impresión.
 * Referencias: Aho §6.2.1, §8.4.1; sintaxis ISA según isa_table y isa.md §11.
 */
#include "tac.h"
#include <inttypes.h>

/* Alias físicos del ISA; solo recibe registros válidos (0..15). */
static void print_physical(FILE *out, size_t id) {
    static const char *const aliases[] = {"rz", "rl", "rx", "rg"};
    if (id < 4) {
        fputs(aliases[id], out);
    } else {
        fprintf(out, "t%zu", id - 4);
    }
}

/* HIGH usa tN; ISA usa vN para TEMP y alias del ISA para PHYS. */
static void print_operand(FILE *out, TacStage stage, TacOperand operand) {
    switch (operand.kind) {
        case TAC_NONE:
            break;
        case TAC_TEMP:
            fprintf(out, "%c%zu", stage == TAC_STAGE_HIGH ? 't' : 'v', operand.value.id);
            break;
        case TAC_PHYS:
            print_physical(out, operand.value.id);
            break;
        case TAC_VARIABLE:
        case TAC_FUNCTION:
            fputs(operand.value.symbol->name, out);
            break;
        case TAC_INTEGER:
            fprintf(out, "%" PRId64, operand.value.integer);
            break;
        case TAC_LABEL:
            fprintf(out, "B%zu", operand.value.block->label);
            break;
    }
}

/* Par de operandos separados con la misma sintaxis en ambos niveles. */
static void print_pair(FILE *out, TacStage stage, TacOperand first, TacOperand second) {
    print_operand(out, stage, first);
    fputs(", ", out);
    print_operand(out, stage, second);
}

/* Conserva el orden de argumentos Calser, distinto de kv_write del ISA. */
static void print_crypto(FILE *out, TacStage stage, const TacInstruction *instruction,
                          const TacOpInfo *info) {
    fprintf(out, "%s [", info->text);
    if (info->selectors & TAC_SELECT_W) {
        fprintf(out, "%d, %d, ", instruction->selectors.k, instruction->selectors.w);
    }
    if (info->reads & TAC_DEST) {
        print_operand(out, stage, instruction->dest);
        fputs(", ", out);
    }
    if (info->reads & TAC_SRC1) {
        print_operand(out, stage, instruction->src1);
    }
    if (info->reads & TAC_SRC2) {
        fputs(", ", out);
        print_operand(out, stage, instruction->src2);
    }
    if ((info->selectors & TAC_SELECT_K) && !(info->selectors & TAC_SELECT_W)) {
        fprintf(out, ", %d", instruction->selectors.k);
    }
    fputc(']', out);
}

/*
 * La forma ISA decide sintaxis. La semántica de máquina viene de isa_table;
 * tac_isa_info agrega restricciones y resúmenes de la ABI de Calser.
 */
static void print_isa(FILE *out, TacStage stage, const TacInstruction *instruction) {
    const IsaInstruction *isa = instruction->isa.instruction;
    fputs(isa->name, out);
    if (isa->form == ISA_FORM_NONE) {
        return;
    }
    fputc(' ', out);
    switch (isa->form) {
        case ISA_FORM_R3:
        case ISA_FORM_IMM:
        case ISA_FORM_FEIS:
            print_pair(out, stage, instruction->dest, instruction->src1);
            fputs(", ", out);
            print_operand(out, stage, instruction->src2);
            if (isa->form == ISA_FORM_FEIS) {
                fprintf(out, ", %d, %d", instruction->selectors.k,
                        instruction->selectors.round);
            }
            break;
        case ISA_FORM_MOV:
            print_pair(out, stage, instruction->dest, instruction->src2);
            break;
        case ISA_FORM_BRANCH:
            print_pair(out, stage, instruction->src1, instruction->src2);
            fputs(", ", out);
            print_operand(out, stage, instruction->isa.target);
            break;
        case ISA_FORM_JAS:
            print_operand(out, stage, instruction->dest);
            fprintf(out, ", .L_func_%s", instruction->isa.target.value.symbol->name);
            break;
        case ISA_FORM_JEFR:
            print_pair(out, stage, instruction->dest, instruction->src1);
            break;
        case ISA_FORM_JEF:
            print_operand(out, stage, instruction->isa.target);
            break;
        case ISA_FORM_LOAD:
        case ISA_FORM_STORE:
            print_operand(out, stage, isa->form == ISA_FORM_LOAD ? instruction->dest :
                          instruction->src2);
            fprintf(out, ", %" PRId64 "(", instruction->isa.displacement);
            print_operand(out, stage, instruction->src1);
            fputc(')', out);
            break;
        case ISA_FORM_RS1:
        case ISA_FORM_KV_WRITE:
            print_operand(out, stage, instruction->src1);
            if (isa->form == ISA_FORM_KV_WRITE) {
                fprintf(out, ", %d, %d", instruction->selectors.k,
                        instruction->selectors.w);
            }
            break;
        case ISA_FORM_NONE:
            break;
    }
}

/* Imprime una cuádrupla válida usando el formato de su tabla. */
static void print_instruction(FILE *out, TacStage stage, const TacInstruction *instruction) {
    TacOpInfo info;
    tac_instruction_info(instruction, &info);
    switch (info.format) {
        case TAC_FMT_COPY:
        case TAC_FMT_ADDRESS:
        case TAC_FMT_UNARY:
        case TAC_FMT_BINARY:
            print_operand(out, stage, instruction->dest);
            fputs(" = ", out);
            if (info.format == TAC_FMT_ADDRESS || info.format == TAC_FMT_UNARY) {
                fprintf(out, "%s ", info.text);
            }
            print_operand(out, stage, instruction->src1);
            if (info.format == TAC_FMT_BINARY) {
                fprintf(out, " %s ", info.text);
                print_operand(out, stage, instruction->src2);
            }
            break;
        case TAC_FMT_LOAD:
            print_operand(out, stage, instruction->dest);
            fputs(" = ", out);
            print_operand(out, stage, instruction->src1);
            fputc('[', out);
            print_operand(out, stage, instruction->src2);
            fputc(']', out);
            break;
        case TAC_FMT_STORE:
            print_operand(out, stage, instruction->dest);
            fputc('[', out);
            print_operand(out, stage, instruction->src1);
            fputs("] = ", out);
            print_operand(out, stage, instruction->src2);
            break;
        case TAC_FMT_CRYPTO:
            print_crypto(out, stage, instruction, &info);
            break;
        case TAC_FMT_PARAM:
            fputs("param ", out);
            print_pair(out, stage, instruction->src1, instruction->src2);
            break;
        case TAC_FMT_CALL:
            if (instruction->dest.kind != TAC_NONE) {
                print_operand(out, stage, instruction->dest);
                fputs(" = ", out);
            }
            fputs("call ", out);
            print_pair(out, stage, instruction->src1, instruction->src2);
            break;
        case TAC_FMT_RETURN:
            fputs("return", out);
            if (instruction->src1.kind != TAC_NONE) {
                fputc(' ', out);
                print_operand(out, stage, instruction->src1);
            }
            break;
        case TAC_FMT_GOTO:
            fputs("goto ", out);
            print_operand(out, stage, instruction->dest);
            break;
        case TAC_FMT_IF:
            fputs("if ", out);
            print_operand(out, stage, instruction->src1);
            fprintf(out, " %s ", info.text);
            print_operand(out, stage, instruction->src2);
            fputs(" goto ", out);
            print_operand(out, stage, instruction->dest);
            break;
        case TAC_FMT_ISA:
            print_isa(out, stage, instruction);
            break;
    }
    fputc('\n', out);
}

/*
 * tac_print — recorre funciones, bloques e instrucciones en orden de lista.
 *
 * Entrada:
 *   out: FILE vivo; program: programa válido.
 * Salida:
 *   ninguna.
 * Efectos:
 *   imprime en orden estable; no posee el flujo.
 * Restricciones:
 *   comprobar ferror; etiquetas locales, sin direcciones del proceso (Aho §6.2.1).
 */
void tac_print(FILE *out, const TacProgram *program) {
    for (const TacFunction *function = program->first; function; function = function->next) {
        fprintf(out, "function %s (entry B%zu)\n", function->symbol->name,
                function->entry->label);
        for (const TacBlock *block = function->first; block; block = block->next) {
            fprintf(out, "B%zu: successors [", block->label);
            for (size_t k = 0; k < block->successor_count; k++) {
                fprintf(out, "%sB%zu", k ? ", " : "", block->successors[k]->label);
            }
            fputs("]\n", out);
            for (const TacInstruction *instruction = block->first; instruction;
                 instruction = instruction->next) {
                fputs("  ", out);
                print_instruction(out, program->stage, instruction);
            }
        }
        fputc('\n', out);
    }
}
