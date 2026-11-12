/*
 * tac_locations.c — adaptador común de ubicaciones y clasificación (C1).
 * Usa tac_live_internal.h. Las pasadas no reconstruyen efectos por opcode.
 * Referencias: Aho §9.2.5 y §10.2.3; ABI de Calser en tac.h.
 */
#include "tac_live_internal.h"

/* Entrada: operandos por valor. Salida: igualdad de ubicación. Efectos: ninguno.
 * Restricciones: solo compara identidades de VARIABLE/TEMP/PHYS, nunca direcciones.
 */
int tac_location_equal(TacOperand left, TacOperand right) {
    if (left.kind != right.kind)
        return 0;
    if (left.kind == TAC_VARIABLE)
        return left.value.symbol == right.value.symbol;
    return (left.kind == TAC_TEMP || left.kind == TAC_PHYS) && left.value.id == right.value.id;
}

/* Entrada: ubicación, etapa, callback. Salida: ninguna. Efectos: visita si corresponde.
 * Restricciones: rz es constante; HIGH no tiene ubicaciones físicas asignables.
 */
static void visit_value(TacStage stage, TacOperand value, int definition,
                         TacLocationVisit visit, void *user) {
    if (value.kind == TAC_PHYS && (stage == TAC_STAGE_HIGH || value.value.id == TAC_P1_RZ))
        return;
    if (value.kind == TAC_VARIABLE && stage != TAC_STAGE_HIGH)
        return;
    visit(user, value, definition);
}

/* Entrada: función/instrucción válidas, callback. Salida: ninguna.
 * Efectos: enumera valores explícitos, globales HIGH e implícitos físicos ISA.
 * Restricciones: clobbers no son definiciones de valores; Aho §9.2.5.
 */
void tac_locations_visit(const TacFunction *function, const TacInstruction *instruction,
                         TacLocationVisit visit, void *user) {
    TacOpInfo info;
    tac_instruction_info(instruction, &info);
    TacStage stage = function->program->stage;
    for (int definition = 0; definition < 2; definition++) {
        TacOperand values[3];
        size_t count = definition ? tac_definitions(instruction, values) : tac_uses(instruction, values);
        for (size_t i = 0; i < count; i++)
            visit_value(stage, values[i], definition, visit, user);
        if (stage == TAC_STAGE_HIGH && (definition ? info.defines_globals : info.uses_globals)) {
            for (const Symbol *s = function->program->global_scope->symbols; s; s = s->next) {
                if (s->kind == SYMBOL_VARIABLE && !s->dimensions)
                    visit(user, tac_variable(s), definition);
            }
        }
        uint16_t mask = definition ? info.implicit_phys_writes : info.implicit_phys_reads;
        if (stage != TAC_STAGE_HIGH) {
            for (unsigned r = 1; r < 16; r++) {
                if (mask & (1u << r))
                    visit(user, tac_physical(r), definition);
            }
        }
    }
}

/* Entrada: instrucción válida y vista escribible. Salida: máscaras físicas separadas.
 * Efectos: escribe solo restricciones físicas. Restricciones: no mata virtuales.
 */
void tac_locations_physical(const TacInstruction *instruction, TacLiveInstruction *out) {
    TacOpInfo info;
    tac_instruction_info(instruction, &info);
    out->physical_reads = info.implicit_phys_reads;
    out->physical_writes = info.implicit_phys_writes & ~1u;
    out->call_clobbers = info.call_clobbers;
    out->scratch_count = info.scratch_count;
    TacOperand values[] = {instruction->dest, instruction->src1, instruction->src2};
    unsigned reads = info.reads;
    unsigned writes = info.writes;
    if (instruction->op == TAC_ISA) {
        /* preserves es uso de valor, no lectura física adicional (contrato ISA A). */
        reads = instruction->isa.instruction->semantics.reads;
        writes = instruction->isa.instruction->semantics.writes;
    }
    for (unsigned i = 0; i < 3; i++) {
        if (values[i].kind != TAC_PHYS)
            continue;
        uint16_t bit = (uint16_t)(1u << values[i].value.id);
        if (reads & (1u << i))
            out->physical_reads |= bit;
        if (writes & (1u << i))
            out->physical_writes |= bit & ~1u;
    }
}

/* Entrada: función, símbolo y lista de ámbitos prestados. Salida: candidato Vm escalar.
 * Efectos: ninguno. Restricciones: no altera Symbol ni usa el semántico global.
 */
int tac_variable_is_rename_candidate(const TacFunction *function, const Symbol *symbol,
                                      const Scope *first_scope) {
    if (!function || !symbol || symbol->kind != SYMBOL_VARIABLE ||
        symbol->modifier != MODIFIER_VM || symbol->dimensions || symbol->limit_address != -1 ||
        tac_is_global(function->program, tac_variable(symbol)))
        return 0;
    for (const Scope *scope = first_scope; scope; scope = scope->next) {
        if (scope->owner != function->symbol || scope->kind == SCOPE_GLOBAL)
            continue;
        for (const Symbol *s = scope->symbols; s; s = s->next) {
            if (s == symbol)
                return 1;
        }
    }
    return 0;
}
