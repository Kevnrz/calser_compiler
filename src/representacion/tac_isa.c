/*
 * tac_isa.c — nivel ISA de representación intermedia de tres direcciones.
 * Usa: tac_internal.h; la semántica de máquina proviene de isa_table (etapa A).
 * Agrega restricciones y resúmenes de la ABI de Calser.
 * Lo usan consultas, validación e impresión. Referencias: Aho §10.2.3; isa.md §6.
 */
#include "tac_internal.h"

/* Convierte forma textual en posiciones normalizadas; no duplica semántica. */
static void operand_shape(IsaForm form, TacOpInfo *info) {
    unsigned reg = TAC_KIND(TAC_TEMP) | TAC_KIND(TAC_PHYS);
    unsigned none = TAC_KIND(TAC_NONE);
    info->allowed[0] = none;
    info->allowed[1] = none;
    info->allowed[2] = none;
    unsigned positions = info->reads | info->writes;
    for (unsigned k = 0; k < 3; k++) {
        if (positions & (1u << k)) {
            info->allowed[k] = reg;
        }
    }
    if (form == ISA_FORM_MOV || form == ISA_FORM_IMM) {
        info->allowed[2] = TAC_KIND(TAC_INTEGER);
    }
}

/* Propiedades de máquina de etapa A más las restricciones/ABI de Calser. */
int tac_isa_info(const TacInstruction *instruction, TacOpInfo *out) {
    const IsaInstruction *isa = instruction->isa.instruction;
    if (!isa || !isa->name || isa_find(isa->name) != isa) {
        return 0;
    }
    const IsaSemantics *semantics = &isa->semantics;
    out->text = isa->name;
    out->reads = semantics->reads | semantics->preserves;
    out->writes = semantics->writes;
    out->format = TAC_FMT_ISA;
    operand_shape(isa->form, out);
    if (instruction->dest.kind == TAC_PHYS &&
        instruction->dest.value.id == TAC_P1_RZ) {
        out->reads &= ~semantics->preserves;
        out->writes = 0;
    }
    out->memory_read = (semantics->effects & ISA_EFFECT_MEMORY_READ) != 0;
    out->memory_write = (semantics->effects & ISA_EFFECT_MEMORY_WRITE) != 0;
    out->crypto_state = (semantics->effects & ISA_EFFECT_CRYPTO_STATE) != 0;
    out->barrier = (semantics->effects & ISA_EFFECT_BARRIER) != 0;
    out->nonremovable = out->memory_write || out->crypto_state ||
                       (semantics->effects & ISA_EFFECT_CONTROL) != 0;
    if (isa->form == ISA_FORM_BRANCH) {
        out->flow = TAC_FLOW_IF;
    } else if (isa->form == ISA_FORM_JEF) {
        out->flow = TAC_FLOW_GOTO;
    } else if (isa->form == ISA_FORM_JEFR) {
        out->flow = TAC_FLOW_RETURN;
        out->uses_globals = 1;
        out->barrier = 1;
    } else if (isa->form == ISA_FORM_JAS) {
        out->barrier = 1;
        out->uses_globals = 1;
        out->defines_globals = 1;
        out->memory_read = 1;
        out->memory_write = 1;
        out->call_clobbers = TAC_P1_CALL_CLOBBERS;
        out->implicit_phys_reads = (1u << TAC_P1_RX);
    }
    if (isa->form == ISA_FORM_FEIS) {
        out->selectors = TAC_SELECT_K | TAC_SELECT_ROUND;
    } else if (isa->form == ISA_FORM_KV_WRITE) {
        out->selectors = TAC_SELECT_K | TAC_SELECT_W;
    }
    return 1;
}

/*
 * Restringe llamadas a jas rl, FUNCTION y retornos a jefR rz, rl; valida
 * objetivos y rangos ISA.
 */
int tac_isa_validate(const TacInstruction *instruction, char *error, size_t cap) {
    const IsaInstruction *isa = instruction->isa.instruction;
    TacOperand target = instruction->isa.target;
    if (isa->form == ISA_FORM_JAS) {
        if (instruction->dest.kind != TAC_PHYS ||
            instruction->dest.value.id != TAC_P1_RL ||
            target.kind != TAC_FUNCTION) {
            return tac_error(error, cap, "jas requiere rl y objetivo FUNCTION");
        }
    } else if (isa->form == ISA_FORM_JEFR) {
        if (instruction->dest.kind != TAC_PHYS ||
            instruction->dest.value.id != TAC_P1_RZ ||
            instruction->src1.kind != TAC_PHYS ||
            instruction->src1.value.id != TAC_P1_RL) {
            return tac_error(error, cap, "jefR solo admite rz, rl como retorno");
        }
    }
    if (isa->form == ISA_FORM_BRANCH || isa->form == ISA_FORM_JEF) {
        if (target.kind != TAC_LABEL) {
            return tac_error(error, cap, "salto ISA requiere objetivo LABEL");
        }
    } else if (isa->form != ISA_FORM_JAS && target.kind != TAC_NONE) {
        return tac_error(error, cap, "objetivo ISA sobrante");
    }
    int memory = isa->form == ISA_FORM_LOAD || isa->form == ISA_FORM_STORE;
    int64_t offset = instruction->isa.displacement;
    if ((memory && !isa_immediate_fits(offset)) || (!memory && offset)) {
        return tac_error(error, cap, "desplazamiento ISA inválido");
    }
    if ((isa->form == ISA_FORM_MOV || isa->form == ISA_FORM_IMM) &&
        !isa_immediate_fits(instruction->src2.value.integer)) {
        return tac_error(error, cap, "inmediato ISA fuera de 18 bits con signo");
    }
    return 1;
}

/*
 * tac_append_isa — construye a mano una operación de máquina estructurada.
 *
 * Entrada:
 *   block: bloque; instruction: fila ISA prestada; operandos normalizados;
 *   displacement/target/selectors: datos de la forma ISA.
 * Salida:
 *   nodo TAC_ISA nuevo; NULL si block es NULL.
 * Efectos:
 *   agrega al final; no expande operaciones ni asigna registros.
 * Restricciones:
 *   validar etapa, rangos, selectores y ABI después (Aho §10.2.3).
 */
TacInstruction *tac_append_isa(TacBlock *block, const IsaInstruction *instruction,
                              TacOperand dest, TacOperand src1, TacOperand src2,
                              int64_t displacement, TacOperand target,
                              TacSelectors selectors) {
    return tac_insert_isa_after(block, block ? block->last : NULL, instruction,
        dest, src1, src2, displacement, target, selectors);
}
