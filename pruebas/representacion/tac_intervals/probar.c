/* Intervalos desde C1: flujo, huecos y posiciones; Aho §9.2.5. */
#include "../tac_analysis/fixture.h"
#include "tac_intervals.h"

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba fronteras y muertes.
 * Restricciones: PSEUDO mantiene fuentes hasta escritura, ISA ordinaria no.
 */
static void positions(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(2), tac_none());
    fixture_isa(f.entry, "mov", f.b, tac_none(), tac_integer(3), tac_none());
    TacInstruction *mul = tac_append_pseudo(f.entry, TAC_PSEUDO_MUL, f.d, f.a, f.b,
                                           (TacSelectors){-1, -1, -1}, 0);
    TacOperand dead = tac_temp_new(f.function);
    TacInstruction *add = fixture_isa(f.entry, "add", dead, f.d, tac_physical(0), tac_none());
    fixture_exit(f.entry);
    fixture_validate(&f);
    char error[256];
    TacLiveAnalysis *live = tac_live_analyze(f.function, NULL, 0, error, sizeof error);
    TacIntervalAnalysis *a = tac_intervals_analyze(f.function, live, error, sizeof error);
    assert(a && tac_intervals_count(a) == 4);
    size_t read, write;
    assert(tac_intervals_positions(a, mul, &read, &write));
    assert(read == 4 && write == 5);
    assert(tac_intervals_find(a, f.a.value.id)->end == write);
    assert(tac_intervals_find(a, f.b.value.id)->end == write);
    assert(tac_intervals_find(a, f.d.value.id)->start == write);
    assert(tac_intervals_positions(a, add, &read, &write));
    assert(tac_intervals_find(a, f.d.value.id)->end == read);
    const TacInterval *unused = tac_intervals_find(a, dead.value.id);
    assert(unused->start == write && unused->end == write);
    assert(!tac_intervals_find(a, 99));
    assert(!tac_intervals_at(a, 99));
    tac_intervals_free(a);
    tac_live_free(live);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba retroceso, ramas y bloque vacío.
 * Restricciones: conserva inalcanzables y varias definiciones sin partir intervalos.
 */
static void flow(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    TacBlock *empty = tac_block_new(f.function);
    TacBlock *test = tac_block_new(f.function);
    TacBlock *body = tac_block_new(f.function);
    TacBlock *end = tac_block_new(f.function);
    TacBlock *unreachable = tac_block_new(f.function);
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(0), tac_none());
    fixture_isa(test, "beq", tac_none(), f.a, tac_physical(0), tac_label(end));
    TacInstruction *increment = fixture_isa(body, "saddi", f.a, f.a, tac_integer(1), tac_none());
    fixture_isa(body, "jef", tac_none(), tac_none(), tac_none(), tac_label(test));
    fixture_isa(end, "add", f.d, f.a, tac_physical(0), tac_none());
    fixture_exit(end);
    TacInstruction *dead = fixture_isa(unreachable, "mov", f.b, tac_none(), tac_integer(7), tac_none());
    fixture_exit(unreachable);
    fixture_validate(&f);
    assert(!empty->first);
    char error[256];
    TacLiveAnalysis *live = tac_live_analyze(f.function, NULL, 0, error, sizeof error);
    TacIntervalAnalysis *a = tac_intervals_analyze(f.function, live, error, sizeof error);
    assert(a);
    size_t read, write;
    assert(tac_intervals_positions(a, increment, &read, &write));
    assert(read == 6); /* MOV, bloque vacío, BEQ, incremento. */
    const TacInterval *range = tac_intervals_find(a, f.a.value.id);
    assert(range->start == 1 && range->end > write);
    assert(range->loop_depth == 1); /* TEMP que atraviesa el lazo, §9.13. */
    assert(tac_intervals_positions(a, dead, &read, &write));
    assert(tac_intervals_find(a, f.b.value.id)->start == write);
    assert(tac_intervals_find(a, f.b.value.id)->end == write);
    assert(tac_intervals_find(a, f.b.value.id)->loop_depth == 0); /* Fuera del lazo. */
    tac_intervals_free(a);
    tac_live_free(live);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: cero. Efectos: prueba envolvente sobre un hueco muerto.
 * Restricciones: no presupone SSA ni divide intervalos.
 */
int main(void) {
    positions();
    flow();
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(1), tac_none());
    fixture_isa(f.entry, "add", f.b, f.a, tac_physical(0), tac_none());
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(2), tac_none());
    fixture_exit(f.entry);
    fixture_validate(&f);
    char error[256];
    TacLiveAnalysis *live = tac_live_analyze(f.function, NULL, 0, error, sizeof error);
    TacIntervalAnalysis *a = tac_intervals_analyze(f.function, live, error, sizeof error);
    assert(a);
    const TacInterval *range = tac_intervals_find(a, f.a.value.id);
    assert(range->start == 1 && range->end == 5);
    assert(range->origin == f.entry->first);
    tac_intervals_free(a);
    tac_live_free(live);
    tac_program_free(f.program);
    puts("PASS intervalos: posiciones, lazo, ramas, vacío, inalcanzable, huecos y definiciones");
    return 0;
}
