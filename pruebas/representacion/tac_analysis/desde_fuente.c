/*
 * desde_fuente.c — analiza corpus HIGH/ISA virtual sin generar código (C1).
 * Usa frontend, construcción y selección; captura estructuras antes/después.
 * Referencias: Aho §9.2.5 y §9.6. No asigna registros ni reserva derrame.
 */
#include "tac_build.h"
#include "tac_lower.h"
#include "tac_live.h"
#include "tac_loops.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern FILE *yyin;
extern ASTNode *root;

/* Entrada: objeto vivo y tamaño. Salida: ninguna. Efectos: escribe captura.
 * Restricciones: comparación solo dentro del mismo proceso, no serialización.
 */
static void capture(FILE *out, const void *object, size_t size) {
    if (size)
        assert(fwrite(object, 1, size, out) == size);
}

/* Entrada: programa validado y build prestado. Salida: captura propia rebobinada.
 * Efectos: serializa objetos vivos e imagen para exigir análisis de solo lectura.
 * Restricciones: incluye marcas, metadatos, aristas, ámbitos y símbolos.
 */
static FILE *snapshot(const TacProgram *program, const TacBuildResult *build) {
    FILE *out = tmpfile();
    assert(out);
    capture(out, build, sizeof *build);
    capture(out, build->initial_data, build->initial_data_size);
    for (const Scope *s = scopes_first(); s; s = s->next) {
        capture(out, s, sizeof *s);
        for (const Symbol *v = s->symbols; v; v = v->next)
            capture(out, v, sizeof *v);
    }
    for (const Symbol *s = build->auxiliary_symbols; s; s = s->next)
        capture(out, s, sizeof *s);
    capture(out, program, sizeof *program);
    for (const TacFunction *f = program->first; f; f = f->next) {
        capture(out, f, sizeof *f);
        for (const TacLoopMark *m = f->loop_marks; m; m = m->next)
            capture(out, m, sizeof *m);
        for (const TacBlock *b = f->first; b; b = b->next) {
            capture(out, b, sizeof *b);
            capture(out, b->successors, b->successor_count * sizeof *b->successors);
            capture(out, b->predecessors, b->predecessor_count * sizeof *b->predecessors);
            for (const TacInstruction *i = b->first; i; i = i->next)
                capture(out, i, sizeof *i);
        }
    }
    assert(!ferror(out));
    rewind(out);
    return out;
}

/* Entrada: capturas propias. Salida: ninguna. Efectos: compara y cierra.
 * Restricciones: igualdad byte a byte, sin normalizar nada.
 */
static void unchanged(FILE *first, FILE *second) {
    int a, b;
    do {
        a = fgetc(first);
        b = fgetc(second);
        assert(a == b);
    } while (a != EOF);
    assert(!ferror(first) && !ferror(second));
    assert(!fclose(first));
    assert(!fclose(second));
}

/* Entrada: lista de ámbitos. Salida: raíces propias y su cantidad.
 * Efectos: reúne globales escalares y variables escalares de main, incluso Vi/S.
 * Restricciones: arreglos son memoria, no registros ni raíces escalares.
 */
static TacOperand *observable_roots(size_t *count) {
    size_t capacity = 0;
    for (const Scope *s = scopes_first(); s; s = s->next) {
        for (const Symbol *v = s->symbols; v; v = v->next)
            capacity++;
    }
    TacOperand *roots = calloc(capacity + 1, sizeof *roots);
    assert(roots);
    *count = 0;
    for (const Scope *s = scopes_first(); s; s = s->next) {
        if (s->kind != SCOPE_GLOBAL && (!s->owner || strcmp(s->owner->name, "main")))
            continue;
        for (const Symbol *v = s->symbols; v; v = v->next) {
            if (v->kind == SYMBOL_VARIABLE && !v->dimensions)
                roots[(*count)++] = tac_variable(v);
        }
    }
    return roots;
}

/* Entrada: función válida, modo, raíces. Salida: ninguna.
 * Efectos: ejecuta análisis y publica resumen de lazos para los casos dirigidos.
 * Restricciones: libera análisis antes de liberar/editar la representación.
 */
static void analyze_function(const TacFunction *f, int loops, TacOperand *roots, size_t count) {
    char error[256];
    if (loops) {
        TacLoopAnalysis *a = tac_loops_analyze(f, error, sizeof error);
        if (!a) {
            fprintf(stderr, "%s: %s\n", f->symbol->name, error);
            assert(0);
        }
        assert(tac_loop_marks_validate(a, error, sizeof error));
        size_t marks = 0;
        size_t without = 0;
        size_t maximum = 0;
        for (const TacBlock *b = f->first; b; b = b->next) {
            size_t depth = tac_loops_depth(a, b);
            if (depth > maximum)
                maximum = depth;
        }
        for (const TacLoopMark *m = f->loop_marks; m; m = m->next) {
            marks++;
            without += tac_loop_mark_state(a, m) == TAC_LOOP_MARK_WITHOUT_BACKEDGE;
        }
        printf("%s %s loops=%zu marks=%zu without=%zu depth=%zu\n",
               f->program->stage == TAC_STAGE_HIGH ? "HIGH" : "ISA", f->symbol->name,
               tac_loops_count(a), marks, without, maximum);
        tac_loops_free(a);
    } else {
        TacLiveAnalysis *a = tac_live_analyze(f, roots, count, error, sizeof error);
        assert(a && !error[0]);
        for (const TacBlock *b = f->first; b; b = b->next) {
            assert(tac_live_block(a, b));
            for (const TacInstruction *i = b->first; i; i = i->next)
                assert(tac_live_instruction(a, i));
        }
        tac_live_free(a);
    }
    assert(tac_validate(f->program, error, sizeof error));
}

/* Entrada: modo live/loops y fuente. Salida: 0 válido; 3 rechazo del frontend.
 * Efectos: analiza HIGH/ISA y exige TAC, ámbitos e imagen intactos.
 * Restricciones: libera ISA antes de HIGH y del semántico; sin generador P1.
 */
int main(int argc, char **argv) {
    if (argc != 3 || (strcmp(argv[1], "live") && strcmp(argv[1], "loops")))
        return 2;
    yyin = fopen(argv[2], "r");
    assert(yyin);
    diag_init(argv[2]);
    int accepted = parser_accept(yyin, &root);
    assert(!fclose(yyin));
    int code = 3;
    if (accepted && semantic_analyze(root)) {
        char error[256];
        TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
        assert(build);
        TacProgram *isa = tac_lower(build->program, error, sizeof error);
        assert(isa);
        size_t count;
        TacOperand *roots = observable_roots(&count);
        const TacProgram *programs[] = {build->program, isa};
        for (unsigned stage = 0; stage < 2; stage++) {
            FILE *before = snapshot(programs[stage], build);
            for (const TacFunction *f = programs[stage]->first; f; f = f->next) {
                assert(!stage || !f->loop_marks);
                analyze_function(f, !strcmp(argv[1], "loops"), roots, count);
            }
            unchanged(before, snapshot(programs[stage], build));
        }
        free(roots);
        tac_program_free(isa);
        tac_build_free(build);
        code = 0;
    }
    semantic_free();
    ast_free(root);
    diag_free();
    return code;
}
