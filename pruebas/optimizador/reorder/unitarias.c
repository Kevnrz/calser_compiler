/* C3.3b: tabla §11.5; calendario por lista, Aho §10.3–10.4.
 * Incluye la implementación para observar distancias/ciclos sin publicar API;
 * se enlaza sin tac_pass_reorder.o. Los dos casos indicados usan ISA manual.
 */
#include "../../../src/optimizador/tac_pass_reorder.c"
#include "../../representacion/tac_analysis/fixture.h"
#include "tac_lower.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
extern ASTNode *root;

/* Entrada: programa válido. Salida: posiciones cambiadas, programa validado.
 * Efectos: ejecuta el gestor real. Restricciones: solo reorder, §11.5.
 */
static size_t run(TacProgram *program) {
    TacMetrics metrics = {0};
    TacPassContext context = {.program = program, .metrics = &metrics,
        .enabled = 1u << 3, .source_name = "reorder"};
    char error[256];
    assert(tac_passes_run(&context, TAC_PASS_VIRTUAL, error, sizeof error));
    size_t count = metrics.counters.reordered;
    tac_metrics_free(&metrics);
    return count;
}

/* Entrada: bloque y nodo prestado. Salida: posición actual.
 * Efectos: ninguno. Restricciones: exige que no haya cambiado de bloque.
 */
static size_t position(const TacBlock *block, const TacInstruction *instruction) {
    size_t p = 0;
    for (const TacInstruction *i = block->first; i; i = i->next, p++)
        if (i == instruction)
            return p;
    assert(0);
    return 0;
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba los dos ISA manuales.
 * Restricciones: el terminal está en otro bloque; no agrega un quinto nodo.
 */
static void chains(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    TacOperand fourth = tac_temp_new(f.function);
    TacInstruction *one = fixture_isa(f.entry, "lw", f.a, tac_physical(0), tac_none(), tac_none());
    TacInstruction *two = fixture_isa(f.entry, "add", f.b, f.a, f.a, tac_none());
    TacInstruction *three = fixture_isa(f.entry, "lw", f.d, tac_physical(0), tac_none(), tac_none());
    three->isa.displacement = 4;
    TacInstruction *four = fixture_isa(f.entry, "add", fourth, f.d, f.d, tac_none());
    fixture_exit(tac_block_new(f.function));
    fixture_validate(&f);
    assert(run(f.program) == 2);
    assert(f.entry->first == one && one->next == three && three->next == two && two->next == four);
    tac_program_free(f.program);
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    one = fixture_isa(f.entry, "lw", f.a, tac_physical(0), tac_none(), tac_none());
    two = fixture_isa(f.entry, "add", f.b, f.a, f.a, tac_none());
    three = fixture_isa(f.entry, "add", f.d, f.b, f.b, tac_none());
    fixture_exit(tac_block_new(f.function));
    fixture_validate(&f);
    assert(!run(f.program));
    assert(one->next == two && two->next == three);
    tac_program_free(f.program);
    puts("PASS dos cadenas: 1,3,2,4 (2); una cadena: 1,2,3 (0)");
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba Operación compleja, §11.5.
 * Restricciones: ISA manual, cargas independientes a ambos lados; §4.6.10.
 * Cada lado conserva la calendarización por lista, sin cruzar la PSEUDO.
 */
static void complex_barrier(void) {
    const TacPseudoKind kinds[] = {TAC_PSEUDO_MUL, TAC_PSEUDO_DIV,
                                  TAC_PSEUDO_MOD, TAC_PSEUDO_POW};
    for (size_t kind = 0; kind < sizeof kinds / sizeof kinds[0]; kind++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        TacOperand values[9];
        for (size_t k = 0; k < 9; k++)
            values[k] = tac_temp_new(f.function);
        TacInstruction *nodes[9];
        for (size_t side = 0; side < 2; side++) {
            size_t at = side ? 5 : 0, v = side ? 5 : 0;
            nodes[at] = fixture_isa(f.entry, "lw", values[v], tac_physical(0), tac_none(), tac_none());
            nodes[at + 1] = fixture_isa(f.entry, "add", values[v + 1], values[v], values[v], tac_none());
            nodes[at + 2] = fixture_isa(f.entry, "lw", values[v + 2], tac_physical(0), tac_none(), tac_none());
            nodes[at + 2]->isa.displacement = 4;
            nodes[at + 3] = fixture_isa(f.entry, "add", values[v + 3], values[v + 2], values[v + 2], tac_none());
            if (!side) {
                TacSelectors selectors = {-1, -1, -1};
                nodes[4] = tac_append_pseudo(f.entry, kinds[kind], values[4], values[1], values[3], selectors, 0);
            }
        }
        fixture_exit(tac_block_new(f.function));
        fixture_validate(&f);
        ReorderNode before = {0}, barrier = {0}, after = {0};
        char error[256];
        assert(fill_node(&before, f.function, nodes[0], error, sizeof error));
        assert(fill_node(&barrier, f.function, nodes[4], error, sizeof error));
        assert(fill_node(&after, f.function, nodes[5], error, sizeof error));
        assert(barrier.alone);
        assert(dependency(&before, &barrier) == 1);
        assert(dependency(&barrier, &after) == 1);
        assert(run(f.program) == 4);
        for (size_t k = 0; k < 4; k++) {
            assert(position(f.entry, nodes[k]) < position(f.entry, nodes[4]));
            assert(position(f.entry, nodes[k + 5]) > position(f.entry, nodes[4]));
        }
        const size_t order[] = {0, 2, 1, 3, 4, 5, 7, 6, 8};
        for (size_t k = 0; k < 9; k++)
            assert(position(f.entry, nodes[order[k]]) == k);
        tac_program_free(f.program);
    }
    puts("PASS operación compleja: MUL/DIV/MOD/POW barrera; dos cadenas por lado (4)");
}

/* Entrada: fuente y clase de restricción de la tabla. Salida: ninguna.
 * Efectos: construye ISA real y comprueba posiciones/ventanas concretas.
 * Restricciones: libera todos los análisis antes de editar, §7.1.
 */
static void source(const char *text, int kind) {
    FILE *input = tmpfile();
    assert(input);
    fputs(text, input);
    fputs(" F main [] : void ; f [ 8 , 2 ] ; E ; .", input);
    rewind(input);
    diag_init("reorder");
    assert(parser_accept(input, &root));
    assert(!fclose(input));
    assert(semantic_analyze(root));
    char error[256];
    TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
    assert(build);
    TacProgram *isa = tac_lower(build->program, error, sizeof error);
    assert(isa);
    TacFunction *function = isa->first;
    if (!strcmp(function->symbol->name, "tocar"))
        function = function->next;
    assert(!strcmp(function->symbol->name, "f"));
    TacBlock *block = function->first->next;
    TacInstruction *store = NULL, *load = NULL, *barrier = NULL, *trap = NULL;
    TacInstruction *branch = NULL, *comparison = NULL;
    TacInstruction *original[256];
    size_t n = 0;
    for (TacInstruction *i = block->first; i; i = i->next) {
        assert(n < 256);
        original[n++] = i;
        TacOpInfo info;
        tac_instruction_info(i, &info);
        if (i->op == TAC_ISA) {
            IsaForm form = i->isa.instruction->form;
            if (form == ISA_FORM_STORE && !store)
                store = i;
            if (form == ISA_FORM_LOAD && store && !load)
                load = i;
            if (form == ISA_FORM_JAS)
                barrier = i;
            if (form == ISA_FORM_BRANCH)
                branch = i;
        }
        if (info.may_trap)
            trap = i;
    }
    if (kind == 3) {
        assert(barrier);
    } else if (kind == 4) {
        assert(trap && store);
        ReorderNode a = {0}, b = {0};
        assert(fill_node(&a, function, trap, error, sizeof error));
        assert(fill_node(&b, function, store, error, sizeof error));
        assert(dependency(&a, &b) == 2);  /* RAW y fallo/store simultáneos. */
    } else if (kind == 5) {
        assert(branch && block->last == branch);
        for (size_t k = 0; k + 1 < n; k++)
            if (original[k]->dest.kind == TAC_TEMP &&
                tac_location_equal(original[k]->dest, branch->src1))
                comparison = original[k];
        assert(comparison);
        ReorderNode nodes[256] = {0};
        unsigned char distances[256 * 256] = {0};
        TacInstruction *order[256];
        for (size_t k = 0; k < n; k++)
            assert(fill_node(&nodes[k], function, original[k], error, sizeof error));
        schedule(nodes, n, distances, order);
        size_t a = position(block, comparison), b = position(block, branch);
        assert(distances[a * n + b] == 2);
        assert(nodes[b].cycle >= nodes[a].cycle + 2);
    } else {
        assert(store && load);
        if (kind < 2) {
            assert(store->src1.kind == TAC_PHYS && load->src1.kind == TAC_PHYS);
            assert((store->isa.displacement == load->isa.displacement) == (kind == 0));
        } else {
            assert(store->src1.kind == TAC_TEMP);
        }
    }
    size_t moved = run(isa);
    if (kind == 0 || kind == 2)
        assert(position(block, store) < position(block, load));
    if (kind == 1) {
        ReorderNode a = {0}, b = {0};
        assert(fill_node(&a, function, store, error, sizeof error));
        assert(fill_node(&b, function, load, error, sizeof error));
        assert(!dependency(&a, &b));
    }
    if (kind == 3)
        for (size_t k = 0; k < n; k++) {
            size_t call_before = 0;
            while (original[call_before] != barrier)
                call_before++;
            if (k < call_before)
                assert(position(block, original[k]) < position(block, barrier));
            if (k > call_before)
                assert(position(block, original[k]) > position(block, barrier));
        }
    if (kind == 4)
        assert(position(block, trap) < position(block, store));
    if (kind == 5)
        assert(block->last == branch);
    for (size_t k = 0; k < n; k++)
        (void)position(block, original[k]);
    printf("PASS fuente tabla %d: %zu posiciones cambiadas en el programa\n", kind, moved);
    tac_program_free(isa);
    tac_build_free(build);
    semantic_free();
    ast_free(root);
    root = NULL;
    diag_free();
}

int main(void) {
    chains();
    complex_barrier();
    source("Vm g : int = 0 ; F f [ int a , int b ] : int r ; g = a ; r = g + b ; E ; .", 0);
    source("Vm g : int = 0 ; Vm h : int = 0 ; F f [ int a , int b ] : int r ; g = a ; r = h + b ; E ; .", 1);
    source("Vm g : int = 0 ; Vm datos : int [ 2 ] ; F f [ int a , int b ] : int r ; datos [ b ] = a ; r = g ; E ; .", 2);
    source("F tocar [ int k ] : void ; E ; . F f [ int a , int b ] : void ; Vm x : int = a + b ; tocar [ a ] ; x = x + b ; E ; .", 3);
    source("Vm g : int = 0 ; F f [ int a , int b ] : void ; g = a / b ; E ; .", 4);
    source("Vm g : int = 0 ; F f [ int a , int b ] : void ; C [ a > b ] ; g = a ; A ; g = b ; . E ; .", 5);
    /* Límite de bloques, §11.5: omitir <3 y >256, sin control de presión. */
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    for (size_t k = 0; k < 257; k++)
        fixture_isa(f.entry, "mov", tac_temp_new(f.function), tac_none(), tac_integer(1), tac_none());
    fixture_exit(tac_block_new(f.function));
    fixture_validate(&f);
    assert(!run(f.program));
    tac_program_free(f.program);
    puts("PASS reorder: tabla §11.5, RAW final=2, bloques conservados y omisión >256");
    return 0;
}
