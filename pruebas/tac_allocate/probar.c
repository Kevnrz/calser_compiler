/* Asignación y plan sin emisión; Aho §8.8, §10.2.3; Poletto/Sarkar fuera de Aho. */
#include "../tac_analysis/fixture.h"
#include "tac_allocate.h"
#include "tac_lower.h"
#include "data_area.h"
#include <stdlib.h>

/* Entrada: fixture terminada. Salida: resultado propio. Efectos: asigna y valida.
 * Restricciones: no TEMP en ningún operando al terminar.
 */
static TacAllocationResult *assign(Fixture *f) {
    fixture_validate(f);
    char error[256];
    TacAllocationResult *a = tac_allocate_registers(f->program, "caso.cls", error, sizeof error);
    if (!a)
        fprintf(stderr, "%s\n", error);
    assert(a && !error[0] && f->program->stage == TAC_STAGE_ISA_PHYSICAL);
    assert(tac_validate(f->program, error, sizeof error));
    for (TacBlock *b = f->function->first; b; b = b->next) {
        for (TacInstruction *i = b->first; i; i = i->next) {
            assert(i->dest.kind != TAC_TEMP && i->src1.kind != TAC_TEMP);
            assert(i->src2.kind != TAC_TEMP && i->isa.target.kind != TAC_TEMP);
        }
    }
    return a;
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba doce/trece vivos y diagnóstico.
 * Restricciones: el fallo debe ocurrir dentro del recorrido con el origen exacto.
 */
static void capacity(void) {
    for (unsigned count = 12; count <= 13; count++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        TacOperand values[13];
        for (unsigned k = 0; k < count; k++) {
            values[k] = tac_temp_new(f.function);
            TacInstruction *i = fixture_isa(f.entry, "mov", values[k], tac_none(),
                                            tac_integer(k), tac_none());
            i->source_line = 42;
            i->source_column = 9;
        }
        for (unsigned k = 0; k < count; k++)
            fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[k], tac_none());
        fixture_exit(f.entry);
        fixture_validate(&f);
        char error[256];
        TacAllocationResult *a = tac_allocate_registers(f.program, "ruta/caso.cls", error, sizeof error);
        if (count == 13) {
            assert(!a);
            assert(!strcmp(error, "ruta/caso.cls:42:9: error de generacion: sin registros temporales disponibles"));
            for (TacInstruction *i = f.entry->first; i; i = i->next)
                i->source_line = i->source_column = 0;
            a = tac_allocate_registers(f.program, "ruta/caso.cls", error, sizeof error);
            assert(!a && !strcmp(error, "ruta/caso.cls:0:0: error de generacion: sin registros temporales disponibles"));
            a = tac_allocate_registers(f.program, NULL, error, sizeof error);
            assert(!a && !strcmp(error, "asignación requiere source_name no NULL"));
            assert(f.program->stage == TAC_STAGE_ISA_VIRTUAL);
            assert(f.entry->first->dest.kind == TAC_TEMP);
        } else {
            assert(a);
            const TacAllocationMetrics *m = tac_allocation_metrics(a, f.function);
            assert(m && m->register_count == 12 && m->assigned_registers == TAC_P1_CALL_CLOBBERS);
        }
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba LRU y reutilización ordinaria.
 * Restricciones: ejecutar dos veces produce los mismos registros.
 */
static void lru(void) {
    for (unsigned run = 0; run < 2; run++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        TacInstruction *first = fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(1), tac_none());
        TacInstruction *reuse = fixture_isa(f.entry, "add", f.b, f.a, tac_physical(0), tac_none());
        fixture_exit(f.entry);
        TacAllocationResult *a = assign(&f);
        assert(first->dest.value.id == 4);
        /* Nunca usados llevan más tiempo libres que el registro recién liberado. */
        assert(reuse->dest.value.id == 5);
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    TacOperand values[12];
    for (size_t k = 0; k < 12; k++) {
        values[k] = tac_temp_new(f.function);
        fixture_isa(f.entry, "mov", values[k], tac_none(), tac_integer(1), tac_none());
    }
    TacInstruction *add = fixture_isa(f.entry, "add", f.d, values[0], values[1], tac_none());
    for (size_t k = 2; k < 12; k++)
        fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[k], tac_none());
    fixture_exit(f.entry);
    TacAllocationResult *a = assign(&f);
    assert(add->dest.value.id == add->src1.value.id);
    tac_allocation_free(a);
    tac_program_free(f.program);
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    for (size_t k = 0; k < 12; k++) {
        values[k] = tac_temp_new(f.function);
        fixture_isa(f.entry, "mov", values[k], tac_none(), tac_integer(1), tac_none());
    }
    fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[1], tac_none());
    fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[0], tac_none());
    TacInstruction *oldest = fixture_isa(f.entry, "mov", f.d, tac_none(), tac_integer(1), tac_none());
    for (size_t k = 2; k < 12; k++)
        fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[k], tac_none());
    fixture_exit(f.entry);
    a = assign(&f);
    assert(oldest->dest.value.id == 5); /* r5 se liberó antes que r4. */
    tac_allocation_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba auxiliares y preservación real.
 * Restricciones: ocho auxiliares distintos de D/A/B; fuente muerta no se reutiliza.
 */
static void arithmetic(void) {
    const unsigned extras[] = {0, 8, 10};
    for (size_t case_id = 0; case_id < sizeof extras / sizeof extras[0]; case_id++) {
        unsigned extra = extras[case_id];
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        if (!extra) {
            while (f.function->next_temp < 8)
                tac_temp_new(f.function);
            f.a = (TacOperand){.kind = TAC_TEMP, .value.id = 5};
            f.b = (TacOperand){.kind = TAC_TEMP, .value.id = 6};
            f.d = (TacOperand){.kind = TAC_TEMP, .value.id = 7};
        }
        TacOperand kept[10];
        for (unsigned k = 0; k < extra; k++) {
            kept[k] = tac_temp_new(f.function);
            fixture_isa(f.entry, "mov", kept[k], tac_none(), tac_integer(k), tac_none());
        }
        fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(2), tac_none());
        fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(3), tac_none());
        TacInstruction *mul = tac_append_pseudo(f.entry, TAC_PSEUDO_MUL, f.d, f.a, f.b,
                                               (TacSelectors){-1, -1, -1}, 0);
        for (unsigned k = 0; k < extra; k++)
            fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), kept[k], tac_none());
        fixture_exit(f.entry);
        if (extra == 10) {
            /* Diez vivos cruzan MUL y sus dos fuentes mueren allí: los doce
             * registros siguen ocupados en escritura. Sin extender las fuentes
             * hasta esa posición, el destino recibiría el registro de una fuente.
             */
            mul->source_line = 42;
            mul->source_column = 9;
            fixture_validate(&f);
            char error[256];
            TacAllocationResult *result = tac_allocate_registers(f.program,
                "ruta/mul.cls", error, sizeof error);
            assert(!result);
            assert(!strcmp(error,
                "ruta/mul.cls:42:9: error de generacion: sin registros temporales disponibles"));
            tac_program_free(f.program);
            continue;
        }
        TacAllocationResult *a = assign(&f);
        assert(mul->dest.value.id != mul->src1.value.id);
        assert(mul->dest.value.id != mul->src2.value.id);
        const TacEmissionSite *site = tac_allocation_site(a, mul);
        assert(site && site->scratch_mask == mul->pseudo.scratch_clobbers);
        unsigned bits = 0;
        for (unsigned r = 0; r < 16; r++)
            bits += !!(site->scratch_mask & (1u << r));
        assert(bits == 8 && !(site->scratch_mask & (1u << mul->dest.value.id)));
        assert(!(site->scratch_mask & (1u << mul->src1.value.id)));
        assert(!(site->scratch_mask & (1u << mul->src2.value.id)));
        assert(site->preserved_count == (extra ? 7 : 0));
        for (size_t k = 0; k < site->preserved_count; k++) {
            assert(site->preserved[k].temp_id == kept[k].value.id);
            assert(site->scratch_mask & (1u << site->preserved[k].physical_register));
        }
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba siete CRYPTO y CALL.
 * Restricciones: no preserva fuentes muertas ni interpreta clobbers como defs.
 */
static void barriers(void) {
    for (TacPseudoKind kind = TAC_PSEUDO_AUTH; kind <= TAC_PSEUDO_DESCIFRAR; kind++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(1), tac_none());
        fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(2), tac_none());
        TacInstruction probe = {.op = TAC_PSEUDO, .pseudo = {.kind = kind}};
        TacOpInfo info;
        assert(tac_instruction_info(&probe, &info));
        TacOperand operands[3] = {tac_none(), tac_none(), tac_none()};
        for (unsigned k = 0; k < 3; k++)
            if (info.reads & (1u << k))
                operands[k] = f.b;
        TacSelectors selectors = {-1, -1, -1};
        if (info.selectors & TAC_SELECT_K)
            selectors.k = 0;
        if (info.selectors & TAC_SELECT_W)
            selectors.w = 1;
        TacInstruction *crypto = tac_append_pseudo(f.entry, kind,
            operands[0], operands[1], operands[2], selectors, 0);
        fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), f.a, tac_none());
        fixture_exit(f.entry);
        TacAllocationResult *a = assign(&f);
        const TacEmissionSite *site = tac_allocation_site(a, crypto);
        assert(site && !site->scratch_mask && site->preserved_count == 1);
        assert(site->preserved[0].temp_id == f.a.value.id);
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    Symbol callee = {.name = "aux", .kind = SYMBOL_FUNCTION, .type = TYPE_VOID};
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(1), tac_none());
    fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(2), tac_none());
    TacInstruction *call = fixture_isa(f.entry, "jas", tac_physical(1), tac_none(),
                                      tac_none(), tac_function_operand(&callee));
    fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), f.a, tac_none());
    fixture_exit(f.entry);
    fixture_validate(&f);
    char error[256];
    TacLiveAnalysis *live = tac_live_analyze(f.function, NULL, 0, error, sizeof error);
    const TacLiveInstruction *view = tac_live_instruction(live, call);
    assert(view->call_clobbers == TAC_P1_CALL_CLOBBERS);
    assert(!tac_live_contains(live, view->definitions, f.a));
    assert(tac_live_contains(live, view->preserve, f.a));
    assert(!tac_live_contains(live, view->preserve, f.b));
    tac_live_free(live);
    TacAllocationResult *a = assign(&f);
    assert(tac_allocation_site(a, call)->preserved_count == 1);
    assert(tac_allocation_site(a, call)->preserved[0].temp_id == f.a.value.id);
    tac_allocation_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: comprueba destinos ligados y bajada.
 * Restricciones: no cambia identidad de lectura/definición de ifmov/feis.
 */
static void linked_and_lower(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(1), tac_none());
    fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(2), tac_none());
    fixture_isa(f.entry, "mov", f.d, tac_none(), tac_integer(3), tac_none());
    TacInstruction *ifmov = fixture_isa(f.entry, "ifmov", f.d, f.a, f.b, tac_none());
    TacInstruction *feis = fixture_isa(f.entry, "feis", f.d, f.a, f.b, tac_none());
    feis->selectors.k = feis->selectors.round = 0;
    fixture_exit(f.entry);
    TacAllocationResult *a = assign(&f);
    assert(ifmov->dest.value.id == feis->dest.value.id);
    TacOperand uses[3];
    assert(tac_uses(ifmov, uses) == 3 && uses[0].value.id == ifmov->dest.value.id);
    assert(tac_uses(feis, uses) == 3 && uses[0].value.id == feis->dest.value.id);
    tac_allocation_free(a);
    tac_program_free(f.program);
    for (unsigned both = 0; both < 2; both++) {
        fixture_init(&f, TAC_STAGE_HIGH);
        tac_append(f.entry, TAC_COPY, f.a, tac_integer(3), tac_none());
        tac_append(f.entry, TAC_COPY, f.b, tac_integer(4), tac_none());
        tac_append(f.entry, TAC_MUL, f.a, f.a, both ? f.a : f.b);
        fixture_return(f.entry);
        fixture_validate(&f);
        char error[256];
        TacProgram *isa = tac_lower(f.program, error, sizeof error);
        assert(isa);
        TacInstruction *mul = isa->first->first->first->next->next;
        assert(mul->op == TAC_PSEUDO && mul->dest.value.id != f.a.value.id);
        assert(mul->next->op == TAC_ISA && !strcmp(mul->next->isa.instruction->name, "add"));
        assert(mul->next->dest.value.id == f.a.value.id);
        assert(mul->next->src1.value.id == mul->dest.value.id);
        TacAllocationResult *result = tac_allocate_registers(isa, "alias.cls", error, sizeof error);
        assert(result);
        tac_allocation_free(result);
        tac_program_free(isa);
        tac_program_free(f.program);
    }
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba extensión estática y pila.
 * Restricciones: no restauración; cada compilación nueva llama data_area_reset.
 */
static void spill_area(void) {
    Symbol a = {.kind = SYMBOL_FUNCTION, .frame = {.area_base = 0, .area_size = 80,
                                                .temp_base = 32, .spill_base = -1}};
    Symbol b = {.kind = SYMBOL_FUNCTION, .frame = {.spill_base = -1}};
    data_area_reset();
    int address;
    data_area_alloc(160, &address);
    assert(data_area_reserve_spill(&a, 0) && a.frame.spill_base == -1);
    assert(data_area_reserve_spill(&a, 16) && a.frame.spill_base == 160);
    assert(data_area_reserve_spill(&b, 8) && b.frame.spill_base == 176);
    assert(a.frame.area_size == 80 && a.frame.temp_base == 32);
    assert(!data_area_stack_short());
    assert(!data_area_reserve_spill(&a, 4));
    for (int short_stack = 0; short_stack < 2; short_stack++) {
        data_area_reset();
        a.frame.spill_size = 0;
        a.frame.spill_base = -1;
        data_area_alloc(DATA_LIMIT - STACK_MIN_BYTES - 4, &address);
        assert(data_area_reserve_spill(&a, short_stack ? 8 : 4));
        assert(data_area_stack_short() == short_stack);
    }
    data_area_reset();
    a.frame.spill_size = 0;
    assert(!data_area_reserve_spill(&a, -4));
    assert(!data_area_reserve_spill(&a, 3));
    assert(!data_area_reserve_spill(&a, DATA_LIMIT + 4));
    data_area_alloc(DATA_LIMIT - 4, &address);
    assert(!data_area_reserve_spill(&a, 8));
    assert(data_area_end() == DATA_LIMIT + 4); /* Fallo no revierte. */
    data_area_reset();
    assert(data_area_end() == 0);
}

/* Entrada: ninguna. Salida: cero. Efectos: ejecuta contratos C2 sin CLI/emisión. */
int main(void) {
    capacity();
    lru();
    arithmetic();
    barriers();
    linked_and_lower();
    spill_area();
    puts("PASS asignación: capacidad, LRU, auxiliares, preservación, ligados, bajada y derrame");
    return 0;
}
