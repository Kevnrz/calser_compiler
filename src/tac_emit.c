/* D: emite ISA física hacia codegen y cg_schedule; Aho §8.6 y §10.2.3.
 * No duplica el parser ni la semántica ISA; no reserva derrame ni altera P1.
 */
#include "tac_emit_internal.h"
#include "data_area.h"
#include <inttypes.h>
#include <string.h>

/* Entrada: origen opcional y mensaje. Salida: 0 con diagnóstico.
 * Efectos: escribe error. Restricciones: source_name ya validado, sin respaldo.
 */
int te_error(TacEmitter *e, const TacInstruction *i, const char *message) {
    return tac_error(e->error, e->capacity, "%s:%d:%d: error de generacion: %s",
        e->input->source_name, i ? i->source_line : 0, i ? i->source_column : 0, message);
}

/* Entrada: función prestada. Salida: etiqueta P1 o NULL.
 * Efectos: ninguno. Restricciones: tabla y decl_node semánticos vivos.
 */
const char *te_function_label(const TacEmitter *e, const Symbol *function) {
    return label_table_get(e->input->labels, function->decl_node, LABEL_FUNCTION);
}

/* Entrada: cantidad final y nombre. Salida: 1 válida, 0 con diagnóstico.
 * Efectos: solo escribe error. Restricciones: no cuenta IR ni emisión conservadora.
 */
int tac_emit_check_rom(size_t count, const char *source, char *error, size_t capacity) {
    if (!source)
        return tac_error(error, capacity, "emisión requiere source_name no NULL");
    if (count > 4096)
        return tac_error(error, capacity,
            "%s:0:0: error de generacion: ROM excedida: %zu bundles, máximo 4096", source, count);
    if (capacity)
        error[0] = '\0';
    return 1;
}

/* Entrada: programa y plan. Salida: 1 válidos, 0 con diagnóstico.
 * Efectos: prepara etiquetas globales. Restricciones: no recalcula C2 ni sus máscaras.
 */
static int prepare(TacEmitter *e) {
    const TacEmitInput *in = e->input;
    char detail[256];
    if (!in->program || in->program->stage != TAC_STAGE_ISA_PHYSICAL || !in->allocation)
        return te_error(e, NULL, "emisión requiere ISA física y plan de asignación");
    if (!tac_validate(in->program, detail, sizeof detail))
        return te_error(e, NULL, detail);
    if (!in->labels || in->initial_data_size > CG_DATA_BYTES || in->initial_data_size % 4 ||
        (in->initial_data_size && !in->initial_data))
        return te_error(e, NULL, "etiquetas o imagen inicial inválidas");
    e->end_label = label_table_get(in->labels, NULL, LABEL_PROGRAM_END);
    e->error_label = label_table_get(in->labels, NULL, LABEL_ARITH_ERROR);
    if (!e->end_label || !e->error_label)
        return te_error(e, NULL, "faltan etiquetas de fin o error aritmético");
    int main_found = 0;
    for (const TacFunction *f = in->program->first; f; f = f->next) {
        main_found += !strcmp(f->symbol->name, "main");
        if (!tac_allocation_metrics(in->allocation, f) || !te_function_label(e, f->symbol))
            return te_error(e, NULL, "función sin plan o etiqueta de entrada");
        for (const TacBlock *b = f->first; b; b = b->next)
            e->block_count++;
    }
    if (main_found != 1)
        return te_error(e, NULL, "emisión requiere una función main");
    e->blocks = tac_analysis_array(e->block_count, sizeof *e->blocks);
    size_t k = 0;
    for (const TacFunction *f = in->program->first; f; f = f->next) {
        for (const TacBlock *b = f->first; b; b = b->next) {
            e->blocks[k].block = b;
            snprintf(e->blocks[k].name, sizeof e->blocks[k].name, ".L_o1_%zu", k);
            k++;
        }
    }
    return 1;
}

/* Entrada: bloque del programa. Salida: nombre único prestado o NULL.
 * Efectos: ninguno. Restricciones: identidad por puntero, no por label ni Bn.
 */
static const char *block_label(const TacEmitter *e, const TacBlock *block) {
    for (size_t k = 0; k < e->block_count; k++)
        if (e->blocks[k].block == block)
            return e->blocks[k].name;
    return NULL;
}

/* Entrada: operando físico/inmediato/ausente válido y buffer de 32 bytes.
 * Salida: texto prestado. Efectos: formatea enteros. Restricciones: no materializa.
 */
static const char *operand_text(TacOperand operand, char buffer[32]) {
    if (operand.kind == TAC_PHYS)
        return gen_reg_name((int)operand.value.id);
    if (operand.kind == TAC_INTEGER) {
        snprintf(buffer, 32, "%" PRId64, operand.value.integer);
        return buffer;
    }
    return "";
}

/* Entrada: ISA física ordinaria y mapa. Salida: 1 emitida, 0 con diagnóstico.
 * Efectos: llama cg_op. Restricciones: cg_describe_instruction/isa_table validan
 * texto y dependencias; aquí solo se adapta la forma a la sintaxis (Aho §8.6).
 */
static int emit_isa(TacEmitter *e, const TacInstruction *i) {
    const IsaInstruction *isa = i->isa.instruction;
    char buffers[3][32];
    const char *d = operand_text(i->dest, buffers[0]);
    const char *a = operand_text(i->src1, buffers[1]);
    const char *b = operand_text(i->src2, buffers[2]);
    unsigned allowed = isa_unit_slots(isa->unit);
    Slot slot = SLOT_ALU0;
    while (slot < SLOT_LSU && !(allowed & (1u << slot)))
        slot++;
    const char *target = i->isa.target.kind == TAC_LABEL ?
        block_label(e, i->isa.target.value.block) : NULL;
    switch (isa->form) {
        case ISA_FORM_R3:
        case ISA_FORM_IMM:
            cg_op(slot, "%s %s, %s, %s", isa->name, d, a, b);
            break;
        case ISA_FORM_MOV:
            cg_op(slot, "%s %s, %s", isa->name, d, b);
            break;
        case ISA_FORM_BRANCH:
            cg_op(slot, "%s %s, %s, %s", isa->name, a, b, target);
            break;
        case ISA_FORM_JEF:
            cg_op(slot, "%s %s", isa->name, target);
            break;
        case ISA_FORM_JEFR:
            cg_op(slot, "%s %s, %s", isa->name, d, a);
            break;
        case ISA_FORM_LOAD:
        case ISA_FORM_STORE:
            cg_op(slot, "%s %s, %" PRId64 "(%s)", isa->name,
                isa->form == ISA_FORM_LOAD ? d : b, i->isa.displacement, a);
            break;
        case ISA_FORM_FEIS:
            cg_op(slot, "%s %s, %s, %s, %d, %d", isa->name, d, a, b,
                i->selectors.k, i->selectors.round);
            break;
        case ISA_FORM_RS1:
            cg_op(slot, "%s %s", isa->name, a);
            break;
        case ISA_FORM_KV_WRITE:
            cg_op(slot, "%s %s, %d, %d", isa->name, a, i->selectors.k, i->selectors.w);
            break;
        case ISA_FORM_NONE:
            cg_op(slot, "%s", isa->name);
            break;
        case ISA_FORM_JAS:
            return te_error(e, i, "jas requiere el protocolo de llamada");
    }
    return 1;
}

/* Entrada: función física. Salida: 1 emitida, 0 con diagnóstico.
 * Efectos: etiquetas e instrucciones en orden de lista. Restricciones: no repite
 * el prólogo/retorno inicial de tac_build; R conserva su objetivo (Aho §7.2).
 */
static int emit_function(TacEmitter *e, const TacFunction *f) {
    cg_set_line(0);
    cg_label(te_function_label(e, f->symbol));
    for (const TacBlock *b = f->first; b; b = b->next) {
        cg_set_line(0);
        cg_label(block_label(e, b));
        for (const TacInstruction *i = b->first; i; i = i->next) {
            cg_set_line(i->source_line);
            int special = i->op == TAC_PSEUDO || i->isa.instruction->form == ISA_FORM_JAS;
            if (!(special ? te_emit_special(e, f, i) : emit_isa(e, i)))
                return 0;
        }
    }
    return 1;
}

/* Entrada: etiqueta global del P1. Salida: ninguna. Efectos: terminal fijo.
 * Restricciones: no usar cg_op, que añadiría separación automática; duplicación
 * conocida con gen_end_of_main/generate.c, pendiente de limpieza posterior.
 */
static void emit_terminal(const char *label) {
    char jump[128];
    snprintf(jump, sizeof jump, "jef %s", label);
    cg_label(label);
    cg_bundle(NULL, NULL, jump, NULL);
}

/* Entrada: datos prestados. Salida: 1 éxito, 0 con diagnóstico.
 * Efectos: carga imagen, comprueba pila, emite y organiza. Restricciones: contrato
 * público; no llama tac_validate_for_emission sobre PSEUDO aún sin expandir.
 */
int tac_emit_program(const TacEmitInput *in, char *error, size_t capacity) {
    if (!in || !in->source_name)
        return tac_error(error, capacity, "emisión requiere source_name no NULL");
    TacEmitter e = {.input = in, .error = error, .capacity = capacity,
                    .arithmetic = {.emit = gen_arith_emit_op}};
    if (!prepare(&e))
        return 0;
    cg_init();
    int ok = 1;
    for (size_t address = 0; address < in->initial_data_size; address += 4) {
        if (cg_data_set((int)address, in->initial_data[address / 4]))
            ok = te_error(&e, NULL, "imagen inicial fuera del área de datos");
    }
    if (data_area_stack_short())
        ok = te_error(&e, NULL, "no queda espacio para la pila de llamadas: se necesitan 256 bytes");
    if (ok) {
        cg_set_line(0);
        cg_op(SLOT_ALU0, "mov rx, %d", data_area_end());
        for (unsigned main_pass = 0; main_pass < 2 && ok; main_pass++) {
            for (const TacFunction *f = in->program->first; f && ok; f = f->next) {
                if ((!strcmp(f->symbol->name, "main")) == (main_pass == 0))
                    ok = emit_function(&e, f);
            }
        }
    }
    if (ok) {
        cg_set_line(0);
        emit_terminal(e.end_label);
        emit_terminal(e.error_label);
        if (cg_schedule())
            ok = te_error(&e, NULL, "falló la calendarización");
        else
            ok = tac_emit_check_rom((size_t)cg_bundle_count(), in->source_name, error, capacity);
    }
    free(e.blocks);
    return ok;
}
