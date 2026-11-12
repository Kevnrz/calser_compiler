/*
 * tac_live.h — vivacidad reutilizable de valores y restricciones físicas (C2.3 C1).
 * Usa tac.h. Lo usan DCE, la futura asignación y las pruebas.
 * Referencias: Aho §9.2.5; restricciones físicas, §10.2.3.
 */
#ifndef TAC_LIVE_H
#define TAC_LIVE_H
#include "tac.h"

typedef struct TacLiveAnalysis TacLiveAnalysis;
typedef struct TacLiveSet TacLiveSet;

typedef struct {
    const TacBlock *block;          /* Bloque prestado de la función analizada. */
    const TacLiveSet *use;          /* Lecturas anteriores a una definición local. */
    const TacLiveSet *def;          /* Definiciones de valores del bloque. */
    const TacLiveSet *live_in;      /* Valores requeridos a la entrada. */
    const TacLiveSet *live_out;     /* Valores requeridos a la salida. */
} TacLiveBlock;

typedef struct {
    const TacInstruction *instruction; /* Instrucción prestada, sin modificar. */
    const TacLiveSet *uses;         /* Valores explícitos e implícitos leídos. */
    const TacLiveSet *definitions;  /* Definiciones de valores, no clobbers virtuales. */
    const TacLiveSet *before;       /* Valores vivos antes de la instrucción. */
    const TacLiveSet *after;        /* Valores vivos después de la instrucción. */
    uint16_t physical_reads;        /* Máscara física, incluido rz si se lee. */
    uint16_t physical_writes;       /* Escrituras explícitas e implícitas, sin rz. */
    uint16_t call_clobbers;         /* Destruidos, separados de las definiciones. */
    unsigned scratch_count;        /* Auxiliares aún no elegidos, para C2. */
    const TacLiveSet *preserve;     /* Vivos atravesando una expansión; conservador en virtual. */
} TacLiveInstruction;

/* Función y programa validados. Raíces VARIABLE escalares, prestadas; se aplican
 * solo en HIGH al salir de main. ISA las ignora: son memoria, no registros.
 * Resultado propio, NULL con diagnóstico ante contrato inválido; sin memoria
 * termina como los constructores TAC. error puede ser NULL solo con capacity=0.
 * Las vistas/conjuntos pertenecen al resultado; recalcular después de editar.
 * Conservar función, instrucciones y símbolos hasta tac_live_free.
 */
TacLiveAnalysis *tac_live_analyze(const TacFunction *function,
    const TacOperand *exit_roots, size_t root_count, char *error, size_t capacity);
const TacLiveBlock *tac_live_block(const TacLiveAnalysis *analysis, const TacBlock *block);
const TacLiveInstruction *tac_live_instruction(const TacLiveAnalysis *analysis,
                                             const TacInstruction *instruction);
size_t tac_live_location_count(const TacLiveAnalysis *analysis);
TacOperand tac_live_location(const TacLiveAnalysis *analysis, size_t index);
int tac_live_contains(const TacLiveAnalysis *analysis, const TacLiveSet *set, TacOperand location);
void tac_live_free(TacLiveAnalysis *analysis);

/* first_scope: lista prestada (normalmente scopes_first()). Compara identidad,
 * no nombres ni direcciones. Excluye globales, parámetros, retorno y auxiliares S.
 */
int tac_variable_is_rename_candidate(const TacFunction *function, const Symbol *symbol,
                                      const Scope *first_scope);
#endif
