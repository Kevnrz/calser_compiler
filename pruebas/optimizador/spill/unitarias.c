/* C3.4: planes de derrame sobre ISA manual, diseño §7.6/11.6; Aho §8.8.4.
 * El mapa privado fuerza cada estado sin depender de una víctima incidental.
 */
#include "../../representacion/tac_analysis/fixture.h"
#include "../../../src/generador_o1/tac_allocate_internal.h"
#include "tac_lower.h"
#include "tac_build.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "parser_service.h"
#include "diagnostics.h"
#include "data_area.h"
#include <stdlib.h>
extern ASTNode *root;

/* Entrada: fixture y máscara de víctimas a/b/d. Salida: plan propio.
 * Efectos: consulta liveness y crea sitios; conserva la ISA virtual.
 */
static TacAllocationResult *plan(Fixture *f, unsigned victims) {
    fixture_validate(f);
    TacAllocationResult *a = calloc(1, sizeof *a);
    assert(a);
    a->function_count = 1;
    a->functions = calloc(1, sizeof *a->functions);
    a->sites = calloc(64, sizeof *a->sites);
    assert(a->functions && a->sites);
    TacFunctionAllocation *g = a->functions;
    g->function = f->function;
    g->registers = calloc(f->function->next_temp, sizeof *g->registers);
    g->spill_slot = malloc(f->function->next_temp * sizeof *g->spill_slot);
    assert(g->registers && g->spill_slot);
    f->symbol.frame.spill_base = 128;
    unsigned next = 4;
    for (size_t k = 0; k < f->function->next_temp; k++) {
        g->registers[k] = victims & (1u << k) ? 0 : next++;
        g->spill_slot[k] = victims & (1u << k) ? (int)g->spill_count++ : -1;
    }
    char error[256];
    TacLiveAnalysis *live = tac_live_analyze(f->function, NULL, 0, error, sizeof error);
    assert(live);
    assert(tac_allocate_sites(a, g, live, error, sizeof error));
    tac_live_free(live);
    return a;
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba fuentes/destino normales,
 * base y valor de STORE, duplicados y auxiliares complejos, tabla §11.6.
 */
static void normal_and_complex(void) {
    for (unsigned test = 0; test < 6; test++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(2), tac_none());
        fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(3), tac_none());
        TacInstruction *i;
        unsigned victims;
        if (test == 3) {
            i = fixture_isa(f.entry, "sw", tac_none(), f.a, f.b, tac_none());
            victims = 3;
        } else if (test == 4) {
            i = tac_append_pseudo(f.entry, TAC_PSEUDO_MUL, f.d, f.a, f.b,
                                  (TacSelectors){-1, -1, -1}, 0);
            victims = 7;
        } else {
            i = fixture_isa(f.entry, "add", f.d, f.a, test == 5 ? f.a : f.b, tac_none());
            victims = test == 0 || test == 5 ? 1 : test == 1 ? 4 : 3;
        }
        fixture_exit(f.entry);
        TacAllocationResult *a = plan(&f, victims);
        const TacEmissionSite *s = tac_allocation_site(a, i);
        assert(s);
        if (test == 0 || test == 5) {
            assert(s->load_count == 1 && s->loads[0].physical_register == TAC_P1_RG);
            assert(!s->store_count);
            if (test == 5)
                assert(s->spill_registers[1] == s->spill_registers[2]);
        } else if (test == 1) {
            assert(!s->load_count && s->store_count == 1);
            assert(s->stores[0].physical_register == TAC_P1_RG);
            assert(s->spill_registers[0] != a->functions[0].registers[f.a.value.id]);
        } else if (test == 2) {
            assert(s->load_count == 2 && !s->store_count);
            assert(s->loads[0].physical_register == TAC_P1_RG);
            assert(s->loads[1].physical_register == a->functions[0].registers[f.d.value.id]);
        } else if (test == 3) {
            assert(s->load_count == 2 && !s->store_count);
            assert(s->loads[0].physical_register == TAC_P1_RG);
            assert(s->loads[1].physical_register >= TAC_P1_TEMP_FIRST);
        } else {
            assert(s->load_count == 2 && s->store_count == 1);
            unsigned count = 0;
            for (unsigned r = 4; r <= 15; r++)
                count += !!(s->scratch_mask & (1u << r));
            assert(count == 8);
            for (size_t k = 0; k < 3; k++) {
                assert(s->spill_registers[k] >= 4 && s->spill_registers[k] <= 15);
                assert(!(s->scratch_mask & (1u << s->spill_registers[k])));
                for (size_t j = 0; j < k; j++)
                    assert(s->spill_registers[k] != s->spill_registers[j]);
            }
        }
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
}

/* Entrada: ninguna. Salida: ninguna. Efectos: obliga a prestar un registro
 * ocupado en STORE y comprueba su preservación, no solo la carga de derrame.
 */
static void occupied_borrow(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(128), tac_none());
    fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(7), tac_none());
    TacOperand alive[12] = {f.d};
    for (size_t k = 1; k < 12; k++)
        alive[k] = tac_temp_new(f.function);
    for (size_t k = 0; k < 12; k++)
        fixture_isa(f.entry, "mov", alive[k], tac_none(), tac_integer(1), tac_none());
    TacInstruction *store = fixture_isa(f.entry, "sw", tac_none(), f.a, f.b, tac_none());
    for (size_t k = 0; k < 12; k++)
        fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), alive[k], tac_none());
    fixture_exit(f.entry);
    TacAllocationResult *a = plan(&f, 3);
    const TacEmissionSite *s = tac_allocation_site(a, store);
    assert(s && s->load_count == 2 && s->preserved_count == 1);
    assert(s->loads[1].physical_register == 4);
    assert(s->preserved[0].physical_register == 4 && s->preserved[0].temp_id == f.d.value.id);
    tac_allocation_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: fuerza fuentes CRYPTO distintas
 * y repetidas; cada valor distinto requiere una carga directa al registro fijo.
 */
static void crypto(void) {
    for (int repeated = 0; repeated < 2; repeated++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(4096), tac_none());
        fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(1), tac_none());
        TacInstruction *i = tac_append_pseudo(f.entry, TAC_PSEUDO_CIFRAR, f.a,
            repeated ? f.a : f.b, f.b, (TacSelectors){0,-1,-1}, 0);
        fixture_exit(f.entry);
        TacAllocationResult *a = plan(&f, 3);
        const TacEmissionSite *s = tac_allocation_site(a, i);
        assert(s && s->load_count == 2 && !s->store_count && !s->scratch_mask);
        assert(s->loads[0].physical_register == 6);
        assert(s->loads[1].physical_register == (repeated ? 9 : 7));
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
}

/* Entrada: fuente. Salida: mapa de víctimas medido con la asignación pública.
 * Efectos: frontend y selección reales; no emite ni cambia el programa fuente.
 */
static void inspect(const char *path) {
    FILE *in = fopen(path, "r");
    assert(in);
    diag_init(path);
    assert(parser_accept(in, &root));
    fclose(in);
    assert(semantic_analyze(root));
    char error[256];
    TacBuildResult *high = tac_build(root, scopes_global(), error, sizeof error);
    assert(high);
    TacProgram *isa = tac_lower(high->program, error, sizeof error);
    assert(isa);
    TacAllocationOptions options = {.spill = 1};
    TacAllocationResult *a = tac_allocate_registers(isa, path, &options, error, sizeof error);
    if (!a)
        fprintf(stderr, "%s\n", error);
    assert(a);
    for (size_t k = 0; k < a->function_count; k++) {
        TacFunctionAllocation *f = &a->functions[k];
        for (size_t id = 0; id < f->function->next_temp; id++)
            if (f->spill_slot[id] >= 0)
                printf("%s v%zu casilla=%d\n", f->function->symbol->name, id,
                       f->function->symbol->frame.spill_base + 4 * f->spill_slot[id]);
    }
    tac_allocation_free(a);
    tac_program_free(isa);
    tac_build_free(high);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: clique de trece intervalos:
 * coloreo optimista, reserva de casilla y dos formas de agotamiento, §11.6.
 */
static void color_and_space(void) {
    for (unsigned mode = 0; mode < 3; mode++) {
        data_area_reset();
        int base;
        if (mode)
            data_area_alloc(mode == 1 ? DATA_LIMIT - STACK_MIN_BYTES : DATA_LIMIT, &base);
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        TacOperand values[13];
        for (size_t k = 0; k < 13; k++) {
            values[k] = tac_temp_new(f.function);
            fixture_isa(f.entry, "mov", values[k], tac_none(), tac_integer(1), tac_none());
        }
        for (size_t k = 0; k < 13; k++)
            fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[k], tac_none());
        fixture_exit(f.entry);
        fixture_validate(&f);
        char error[256];
        TacAllocationOptions options = {.spill = 1};
        TacAllocationResult *a = tac_allocate_registers(f.program, "clique.cls", &options,
                                                       error, sizeof error);
        if (!mode) {
            assert(a && f.program->stage == TAC_STAGE_ISA_PHYSICAL);
            const TacAllocationMetrics *m = tac_allocation_metrics(a, f.function);
            assert(m->spilled == 1 && m->spill_loads == 1 && m->spill_stores == 1);
            assert(f.symbol.frame.spill_size == 4 && m->pressure == 12);
        } else {
            assert(!a);
            assert(strstr(error, mode == 1 ? "no queda espacio para la pila" : "sin espacio de datos para derrame"));
        }
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
    data_area_reset();
}

/* Entrada: ninguna. Salida: ninguna. Efectos: distingue distancia de rotación
 * con trece intervalos disjuntos cuyo orden de IDs difiere del programa.
 * Restricciones: prueba separaciones anteriores y posteriores, diseño §11.6.
 */
static void distance_color(void) {
    for (unsigned reverse = 0; reverse < 2; reverse++) {
        data_area_reset();
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        TacOperand values[13];
        TacInstruction *definitions[13];
        for (size_t k = 0; k < 13; k++)
            values[k] = tac_temp_new(f.function);
        size_t order[13];
        if (!reverse) {
            order[0] = 0;
            order[1] = 12;
            for (size_t k = 1; k <= 11; k++)
                order[k + 1] = k;
        } else {
            for (size_t k = 1; k <= 11; k++)
                order[k - 1] = k;
            order[11] = 12;
            order[12] = 0;
        }
        for (size_t k = 0; k < 13; k++) {
            size_t id = order[k];
            definitions[id] = fixture_isa(f.entry, "mov", values[id], tac_none(),
                                           tac_integer(1), tac_none());
        }
        fixture_exit(f.entry);
        fixture_validate(&f);
        char error[256];
        TacAllocationOptions options = {.spill = 1};
        TacAllocationResult *a = tac_allocate_registers(f.program, "distancia.cls", &options,
                                                       error, sizeof error);
        assert(a && !tac_allocation_metrics(a, f.function)->spilled);
        /* v12 se desapila primero: infinito en todos los colores, elige t0.
         * Para v0, el más lejano es v11/t1 o v1/t11, nunca v12/t0 vecino.
         */
        assert(definitions[12]->dest.value.id == 4);
        assert(definitions[0]->dest.value.id == (reverse ? 15 : 5));
        tac_allocation_free(a);
        tac_program_free(f.program);
    }
    data_area_reset();
}

int main(int argc, char **argv) {
    if (argc == 2) {
        inspect(argv[1]);
        return 0;
    }
    normal_and_complex();
    crypto();
    occupied_borrow();
    color_and_space();
    distance_color();
    puts("PASS spill unitarias: fuente, destino, dos fuentes sin destino, duplicados, complejas y CRYPTO");
    return 0;
}
