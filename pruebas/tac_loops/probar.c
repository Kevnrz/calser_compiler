/* Pruebas manuales de lazos, marcas y edición C1. Aho §9.6 y §10.2.3. */
#include "../tac_analysis/fixture.h"

/* Entrada: fixture. Salida: análisis propio. Efectos: valida y analiza.
 * Restricciones: exige grafo reducible.
 */
static TacLoopAnalysis *analyze(Fixture *f) {
    fixture_validate(f);
    char error[256];
    TacLoopAnalysis *a = tac_loops_analyze(f->function, error, sizeof error);
    if (!a) {
        fprintf(stderr, "análisis falló: %s\n", error);
        assert(0);
    }
    return a;
}

/* Entrada: bloques propios. Salida: ninguna. Efectos: agrega GOTO.
 * Restricciones: no agrega instrucciones tras un terminador existente.
 */
static void jump(TacBlock *from, TacBlock *to) {
    tac_append(from, TAC_GOTO, tac_label(to), tac_none(), tac_none());
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba unión de dos retrocesos.
 * Restricciones: una cabecera cuenta un lazo aunque tenga dos latches.
 */
static void shared_header(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacBlock *h = tac_block_new(f.function);
    TacBlock *split = tac_block_new(f.function);
    TacBlock *left = tac_block_new(f.function);
    TacBlock *right = tac_block_new(f.function);
    TacBlock *end = tac_block_new(f.function);
    jump(f.entry, h);
    tac_append(h, TAC_IF_EQ, tac_label(end), f.a, tac_integer(0));
    tac_append(split, TAC_IF_EQ, tac_label(right), f.b, tac_integer(0));
    jump(left, h);
    jump(right, h);
    fixture_return(end);
    TacLoopMark description = {.kind = TAC_LOOP_L, .header = h, .body = split,
                               .latch = left, .structural_depth = 1};
    TacLoopMark *mark = tac_loop_mark_add(f.function, &description);
    assert(mark);
    TacLoopAnalysis *a = analyze(&f);
    char error[256];
    assert(tac_loop_marks_validate(a, error, sizeof error));
    assert(tac_loops_count(a) == 1 && tac_loops_header(a, 0) == h);
    assert(tac_loops_contains(a, 0, left) && tac_loops_contains(a, 0, right));
    assert(tac_loops_depth(a, right) == 1 && tac_loops_depth(a, end) == 0);
    assert(tac_loops_dominates(a, h, right));
    assert(!tac_loops_dominates(a, left, right));
    assert(tac_loop_mark_state(a, mark) == TAC_LOOP_MARK_WITH_BACKEDGE);
    mark->latch = end;
    assert(!tac_loop_marks_validate(a, error, sizeof error));
    mark->latch = left;
    mark->kind = TAC_LOOP_S;
    mark->control = &f.local;
    mark->limit = &f.global;
    mark->initial = tac_label(h);
    mark->bound = tac_integer(3);
    mark->step = 1;
    assert(!tac_loop_marks_validate(a, error, sizeof error));
    assert(strstr(error, "capturados"));
    TacFunction alien = {.program = f.program, .symbol = &f.symbol};
    assert(!tac_loops_analyze(&alien, error, sizeof error));
    tac_loops_free(a);
    tac_loop_marks_discard(f.function);
    assert(!f.function->loop_marks);
    tac_loop_marks_discard(f.function);
    assert(tac_block_remove(f.function, f.entry, error, sizeof error) == 0);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba anidamiento y lazo sin marca.
 * Restricciones: R/copia se modelan por CFG, no por marcas sintácticas.
 */
static void nested_and_unreachable(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacBlock *outer = tac_block_new(f.function);
    TacBlock *inner = tac_block_new(f.function);
    TacBlock *body = tac_block_new(f.function);
    TacBlock *latch = tac_block_new(f.function);
    TacBlock *end = tac_block_new(f.function);
    TacBlock *dead = tac_block_new(f.function);
    jump(f.entry, outer);
    tac_append(outer, TAC_IF_EQ, tac_label(end), f.a, tac_integer(0));
    tac_append(inner, TAC_IF_EQ, tac_label(latch), f.b, tac_integer(0));
    jump(body, inner);
    jump(latch, outer);
    fixture_return(end);
    jump(dead, dead);
    TacLoopAnalysis *a = analyze(&f);
    char error[256];
    assert(tac_loop_marks_validate(a, error, sizeof error));
    assert(tac_loops_count(a) == 2);
    assert(tac_loops_depth(a, outer) == 1);
    assert(tac_loops_depth(a, inner) == 2 && tac_loops_depth(a, body) == 2);
    assert(tac_loops_depth(a, latch) == 1);
    assert(!tac_loops_reachable(a, dead) && tac_loops_depth(a, dead) == 0);
    assert(!tac_loops_dominates(a, dead, dead));
    tac_loops_free(a);
    assert(tac_block_remove(f.function, dead, error, sizeof error));
    fixture_validate(&f);
    assert(f.function->last == end && end->next == NULL);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba E y marca sin retroceso.
 * Restricciones: también se admite latch sintáctico inalcanzable tras ramas terminales.
 */
static void no_backedge(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacBlock *h = tac_block_new(f.function);
    TacBlock *body = tac_block_new(f.function);
    TacBlock *end = tac_block_new(f.function);
    TacBlock *dead = tac_block_new(f.function);
    jump(f.entry, h);
    tac_append(h, TAC_IF_EQ, tac_label(end), f.a, tac_integer(0));
    fixture_return(body);
    fixture_return(end);
    jump(dead, h);
    TacLoopMark description = {.kind = TAC_LOOP_L, .header = h, .body = body,
                               .latch = NULL, .structural_depth = 1};
    TacLoopMark *mark = tac_loop_mark_add(f.function, &description);
    assert(mark);
    TacLoopAnalysis *a = analyze(&f);
    char error[256];
    assert(!tac_loops_count(a));
    assert(tac_loop_marks_validate(a, error, sizeof error));
    assert(tac_loop_mark_state(a, mark) == TAC_LOOP_MARK_WITHOUT_BACKEDGE);
    mark->latch = dead;
    assert(tac_loop_marks_validate(a, error, sizeof error));
    assert(tac_loop_mark_state(a, mark) == TAC_LOOP_MARK_WITHOUT_BACKEDGE);
    mark->header = dead;
    assert(tac_loop_marks_validate(a, error, sizeof error));
    assert(tac_loop_mark_state(a, mark) == TAC_LOOP_MARK_UNREACHABLE);
    tac_loops_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: construye ciclo de dos entradas.
 * Restricciones: es TAC válido, pero irreducible y rechazado por el análisis.
 */
static void irreducible(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacBlock *a = tac_block_new(f.function);
    TacBlock *b = tac_block_new(f.function);
    tac_append(f.entry, TAC_IF_EQ, tac_label(b), f.a, tac_integer(0));
    jump(a, b);
    jump(b, a);
    fixture_validate(&f);
    char error[256];
    assert(!tac_loops_analyze(f.function, error, sizeof error));
    assert(strstr(error, "irreducible"));
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba reemplazos atómicos.
 * Restricciones: conserva definiciones, objetivos y bases simbólicas.
 */
static void replacement(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacInstruction *sum = tac_append(f.entry, TAC_ADD, f.d, f.a, f.a);
    f.local.dimensions = 1;
    TacInstruction *address = tac_append(f.entry, TAC_ADDRESS, f.d,
                                        tac_variable(&f.local), tac_none());
    fixture_return(f.entry);
    fixture_validate(&f);
    char error[256];
    size_t replaced;
    assert(tac_replace_reads(f.entry, sum, f.a, f.b, &replaced, error, sizeof error));
    assert(replaced == 2 && sum->dest.value.id == f.d.value.id);
    assert(sum->src1.value.id == f.b.value.id && sum->src2.value.id == f.b.value.id);
    TacInstruction saved = *sum;
    assert(!tac_replace_reads(f.entry, sum, f.b, tac_label(f.entry), &replaced, error, sizeof error));
    assert(!memcmp(&saved, sum, sizeof saved));
    assert(tac_replace_reads(f.entry, address, tac_variable(&f.local),
                             f.a, &replaced, error, sizeof error));
    assert(replaced == 0 && address->src1.value.symbol == &f.local);
    tac_program_free(f.program);
    for (int feis = 0; feis < 2; feis++) {
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        TacInstruction *i = fixture_isa(f.entry, feis ? "feis" : "ifmov",
                                        f.d, f.d, f.a, tac_none());
        if (feis)
            i->selectors = (TacSelectors){1, -1, 2};
        fixture_exit(f.entry);
        fixture_validate(&f);
        saved = *i;
        assert(!tac_replace_reads(f.entry, i, f.d, f.b, &replaced, error, sizeof error));
        assert(strstr(error, "ligada") && replaced == 0);
        assert(!memcmp(&saved, i, sizeof saved));
        tac_program_free(f.program);
    }
}

/* Entrada: ninguna. Salida: ninguna. Efectos: verifica rechazos sin alterar CFG.
 * Restricciones: el llamador debe retirar referencias antes de quitar regiones.
 */
static void removal_rejections(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacBlock *middle = tac_block_new(f.function);
    TacBlock *end = tac_block_new(f.function);
    jump(f.entry, middle);
    fixture_return(middle);
    fixture_return(end);
    fixture_validate(&f);
    char error[256];
    TacBlock saved = *middle;
    assert(!tac_block_remove(f.function, middle, error, sizeof error));
    assert(strstr(error, "LABEL") && !memcmp(&saved, middle, sizeof saved));
    f.entry->first->dest = tac_label(end);
    fixture_validate(&f);
    TacLoopMark description = {.kind = TAC_LOOP_L, .header = middle, .body = middle,
                               .structural_depth = 1};
    assert(tac_loop_mark_add(f.function, &description));
    assert(!tac_block_remove(f.function, middle, error, sizeof error));
    assert(strstr(error, "marca"));
    tac_loop_marks_discard(f.function);
    assert(tac_block_remove(f.function, middle, error, sizeof error));
    fixture_validate(&f);
    tac_program_free(f.program);
    fixture_init(&f, TAC_STAGE_HIGH);
    middle = tac_block_new(f.function);
    end = tac_block_new(f.function);
    tac_append(f.entry, TAC_COPY, f.a, tac_integer(0), tac_none());
    fixture_return(middle);
    fixture_return(end);
    fixture_validate(&f);
    saved = *middle;
    assert(!tac_block_remove(f.function, middle, error, sizeof error));
    assert(strstr(error, "caída alcanzable") && !memcmp(&saved, middle, sizeof saved));
    fixture_validate(&f);
    tac_program_free(f.program);
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    end = tac_block_new(f.function);
    fixture_isa(f.entry, "jef", tac_none(), tac_none(), tac_none(), tac_label(end));
    fixture_exit(end);
    fixture_validate(&f);
    assert(!tac_block_remove(f.function, end, error, sizeof error));
    assert(strstr(error, "LABEL"));
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: inserta ISA/PSEUDO al inicio/medio/final.
 * Restricciones: conserva extremos y todos los metadatos opcionales.
 */
static void insertion(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    TacInstruction *first = tac_insert_isa_after(f.entry, NULL, isa_find("mov"),
        f.a, tac_none(), tac_integer(7), 0, tac_none(), (TacSelectors){-1, -1, -1});
    TacInstruction *end = tac_append_pseudo(f.entry, TAC_PSEUDO_EXIT,
        tac_none(), tac_none(), tac_none(), (TacSelectors){-1, -1, -1}, 0);
    TacInstruction *middle = tac_insert_pseudo_after(f.entry, first, TAC_PSEUDO_AUTH,
        tac_none(), f.a, tac_none(), (TacSelectors){-1, -1, -1}, 0);
    assert(first && middle && first->next == middle && middle->next == end);
    TacInstruction *head = tac_insert_isa_after(f.entry, NULL, isa_find("lw"),
        f.b, tac_physical(0), tac_none(), 28, tac_none(), (TacSelectors){-1, -1, -1});
    assert(f.entry->first == head && head->next == first && f.entry->last == end);
    assert(head->isa.displacement == 28 && middle->pseudo.scratch_clobbers == 0);
    assert(!head->source_line && !head->source_column);
    TacInstruction alien = {0};
    assert(!tac_insert_isa_after(f.entry, &alien, isa_find("mov"), f.a,
        tac_none(), tac_integer(0), 0, tac_none(), (TacSelectors){-1, -1, -1}));
    assert(!tac_insert_pseudo_after(f.entry, &alien, TAC_PSEUDO_AUTH,
        tac_none(), f.a, tac_none(), (TacSelectors){-1, -1, -1}, 0));
    fixture_validate(&f);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: cero si pasan contratos. Efectos: ejecuta pruebas manuales.
 * Restricciones: no usa AST ni compilación de fuentes.
 */
int main(void) {
    shared_header();
    nested_and_unreachable();
    no_backedge();
    irreducible();
    replacement();
    removal_rejections();
    insertion();
    assert(!tac_loops_analyze(NULL, NULL, 0));
    puts("PASS lazos/edición: dominadores, unión, profundidad, sin retroceso, irreducible y rechazos atómicos");
    return 0;
}
