/* Fixtures compartidas de C1. No son parte de ccom; Aho §8.4.1 y §9.2.5. */
#ifndef TAC_ANALYSIS_FIXTURE_H
#define TAC_ANALYSIS_FIXTURE_H
#include "tac_live.h"
#include "tac_loops.h"
#include <assert.h>
#include <string.h>

typedef struct {
    Scope scope;
    Symbol symbol;
    Symbol global;
    Symbol local;
    TacProgram *program;
    TacFunction *function;
    TacBlock *entry;
    TacOperand a, b, d;
} Fixture;

/* Entrada: fixture. Salida: ninguna. Efectos: construye main y tres TEMP.
 * Restricciones: los símbolos de la fixture sobreviven al programa.
 */
static inline void fixture_init(Fixture *f, TacStage stage) {
    memset(f, 0, sizeof *f);
    f->global = (Symbol){.name = "g", .kind = SYMBOL_VARIABLE, .type = TYPE_INT,
        .modifier = MODIFIER_VM, .limit_address = -1, .width = 4, .address = 0};
    f->local = (Symbol){.name = "x", .kind = SYMBOL_VARIABLE, .type = TYPE_INT,
        .modifier = MODIFIER_VM, .limit_address = -1, .width = 4, .address = 16};
    f->symbol = (Symbol){.name = "main", .kind = SYMBOL_FUNCTION, .type = TYPE_VOID};
    f->scope = (Scope){.kind = SCOPE_GLOBAL, .symbols = &f->global};
    f->program = tac_program_new(&f->scope);
    f->program->stage = stage;
    f->function = tac_function_new(f->program, &f->symbol);
    f->entry = tac_block_new(f->function);
    f->a = tac_temp_new(f->function);
    f->b = tac_temp_new(f->function);
    f->d = tac_temp_new(f->function);
}

/* Entrada: fixture válida. Salida: ninguna. Efectos: reconstruye y valida CFG.
 * Restricciones: todas las funciones del programa ya están completas.
 */
static inline void fixture_validate(Fixture *f) {
    char error[256];
    if (!tac_rebuild_edges(f->function, error, sizeof error) ||
        !tac_validate(f->program, error, sizeof error)) {
        fprintf(stderr, "fixture inválida: %s\n", error);
        assert(0);
    }
}

/* Entrada: bloque HIGH. Salida: nodo terminal. Efectos: agrega RETURN.
 * Restricciones: no agrega un segundo terminador.
 */
static inline TacInstruction *fixture_return(TacBlock *block) {
    return tac_append(block, TAC_RETURN, tac_none(), tac_none(), tac_none());
}

/* Entrada: bloque ISA y operandos. Salida: instrucción real propia.
 * Efectos: agrega ISA sin metadatos opcionales. Restricciones: la forma es válida.
 */
static inline TacInstruction *fixture_isa(TacBlock *block, const char *name,
    TacOperand dest, TacOperand a, TacOperand b, TacOperand target) {
    return tac_append_isa(block, isa_find(name), dest, a, b, 0, target,
                          (TacSelectors){-1, -1, -1});
}

/* Entrada: bloque ISA de main. Salida: ninguna. Efectos: agrega salida abstracta.
 * Restricciones: no se expande ni se entrega al ensamblador.
 */
static inline void fixture_exit(TacBlock *block) {
    tac_append_pseudo(block, TAC_PSEUDO_EXIT, tac_none(), tac_none(), tac_none(),
                      (TacSelectors){-1, -1, -1}, 0);
}
#endif
