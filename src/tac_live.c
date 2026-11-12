/*
 * tac_live.c — punto fijo de vivacidad de valores (C2.3 C1).
 * Usa tac_locations para efectos; no asigna registros ni genera derrame.
 * Referencias: Aho §9.2.5; preservación conservadora, §10.2.3.
 */
#include "tac_live_internal.h"
#include "tac_analysis_internal.h"
#include <stdlib.h>
#include <string.h>

/* Entrada: universo e identidad. Salida: índice o SIZE_MAX. Efectos: ninguno.
 * Restricciones: identidad local a una función, no igualdad de direcciones.
 */
static size_t location_index(const TacLiveAnalysis *a, TacOperand location) {
    for (size_t i = 0; i < a->count; i++) {
        if (tac_location_equal(a->locations[i], location))
            return i;
    }
    return SIZE_MAX;
}

/* Entrada: análisis en preparación y ubicación. Salida: ninguna.
 * Efectos: amplía el universo sin duplicados. Restricciones: antes de crear conjuntos.
 */
static void collect_location(void *user, TacOperand location, int definition) {
    TacLiveAnalysis *a = user;
    (void)definition;
    if (location_index(a, location) != SIZE_MAX)
        return;
    if (a->count == a->capacity) {
        size_t capacity = a->capacity ? a->capacity * 2 : 16;
        if (capacity < a->capacity)
            capacity = SIZE_MAX;
        TacOperand *locations = tac_analysis_array(capacity, sizeof *locations);
        if (a->count)
            memcpy(locations, a->locations, a->count * sizeof *locations);
        free(a->locations);
        a->locations = locations;
        a->capacity = capacity;
    }
    a->locations[a->count++] = location;
}

/* Entrada: universo terminado. Salida: conjunto propio vacío. Efectos: reserva.
 * Restricciones: todos los conjuntos del resultado comparten ancho.
 */
static TacLiveSet *new_set(const TacLiveAnalysis *a) {
    TacLiveSet *set = tac_allocate(sizeof *set);
    set->width = a->count / 64 + (a->count % 64 != 0);
    set->words = tac_analysis_array(set->width, sizeof *set->words);
    return set;
}

/* Entrada: conjunto e índice existente. Salida: bit. Efectos: ninguno.
 * Restricciones: el índice pertenece al universo que creó el conjunto.
 */
static int bit(const TacLiveSet *set, size_t index) {
    return (set->words[index / 64] >> (index % 64)) & 1u;
}

/* Entrada: conjunto existente e índice válido. Salida: ninguna. Efectos: fija bit.
 * Restricciones: la mutación es privada a la construcción del resultado.
 */
static void put(const TacLiveSet *set, size_t index) {
    set->words[index / 64] |= UINT64_C(1) << (index % 64);
}

/* Entrada: conjuntos compatibles. Salida: 1 si cambió. Efectos: copia src a dst.
 * Restricciones: no compara padding ni punteros; Aho §9.2.5.
 */
static int copy_set(const TacLiveSet *dst, const TacLiveSet *src) {
    int changed = 0;
    for (size_t i = 0; i < dst->width; i++) {
        changed |= dst->words[i] != src->words[i];
        dst->words[i] = src->words[i];
    }
    return changed;
}

/* Entrada: conjuntos compatibles. Salida: ninguna. Efectos: dst |= src.
 * Restricciones: solo usa bits de ubicaciones existentes.
 */
static void union_set(const TacLiveSet *dst, const TacLiveSet *src) {
    for (size_t i = 0; i < dst->width; i++)
        dst->words[i] |= src->words[i];
}

/* Entrada: cuatro conjuntos compatibles. Salida: ninguna.
 * Efectos: dst = use | (after - def). Restricciones: admite dst == after.
 */
static void transfer(const TacLiveSet *dst, const TacLiveSet *use,
                      const TacLiveSet *after, const TacLiveSet *def) {
    for (size_t i = 0; i < dst->width; i++)
        dst->words[i] = use->words[i] | (after->words[i] & ~def->words[i]);
}

/* Entrada: función validada. Salida: ninguna. Efectos: indexa listas y consulta efectos.
 * Restricciones: no usa etiquetas como índices; Aho §9.2.5.
 */
static void prepare(TacLiveAnalysis *a) {
    for (const TacBlock *b = a->function->first; b; b = b->next) {
        a->block_count++;
        for (const TacInstruction *i = b->first; i; i = i->next) {
            a->instruction_count++;
            tac_locations_visit(a->function, i, collect_location, a);
        }
    }
    a->blocks = tac_analysis_array(a->block_count, sizeof *a->blocks);
    a->instructions = tac_analysis_array(a->instruction_count, sizeof *a->instructions);
    a->offsets = tac_analysis_array(a->block_count + 1, sizeof *a->offsets);
}

typedef struct {
    TacLiveAnalysis *analysis;
    TacLiveInstruction *instruction;
} FillContext;

/* Entrada: ubicación enumerada y contexto. Salida: ninguna. Efectos: agrega USE/DEF.
 * Restricciones: el universo ya incluye todas las ubicaciones de la instrucción.
 */
static void fill_location(void *user, TacOperand location, int definition) {
    FillContext *fill = user;
    size_t index = location_index(fill->analysis, location);
    put(definition ? fill->instruction->definitions : fill->instruction->uses, index);
}

/* Entrada: análisis con universo final. Salida: ninguna.
 * Efectos: crea conjuntos por bloque/instrucción y resume USE/DEF.
 * Restricciones: DEF físico explícito no incluye call_clobbers virtuales.
 */
static void summarize(TacLiveAnalysis *a) {
    size_t n = 0;
    size_t k = 0;
    for (const TacBlock *b = a->function->first; b; b = b->next, k++) {
        TacLiveBlock *block = &a->blocks[k];
        *block = (TacLiveBlock){b, new_set(a), new_set(a), new_set(a), new_set(a)};
        a->offsets[k] = n;
        for (const TacInstruction *i = b->first; i; i = i->next, n++) {
            TacLiveInstruction *view = &a->instructions[n];
            view->instruction = i;
            view->uses = new_set(a);
            view->definitions = new_set(a);
            view->before = new_set(a);
            view->after = new_set(a);
            view->preserve = new_set(a);
            FillContext fill = {a, view};
            tac_locations_visit(a->function, i, fill_location, &fill);
            tac_locations_physical(i, view);
            for (size_t w = 0; w < view->uses->width; w++) {
                block->use->words[w] |= view->uses->words[w] & ~block->def->words[w];
                block->def->words[w] |= view->definitions->words[w];
            }
        }
    }
    a->offsets[k] = n;
}

/* Entrada: análisis y bloque de salida. Salida: si es salida abstracta de main HIGH.
 * Efectos: ninguno. Restricciones: raíces de memoria no se convierten en registros ISA.
 */
static int program_exit(const TacLiveAnalysis *a, const TacBlock *block) {
    if (a->function->program->stage != TAC_STAGE_HIGH ||
        strcmp(a->function->symbol->name, "main") || !block->last)
        return 0;
    TacOpInfo info;
    tac_instruction_info(block->last, &info);
    return info.flow == TAC_FLOW_RETURN;
}

/* Entrada: resumen e índices preparados, raíces. Salida: ninguna.
 * Efectos: resuelve LIVE_IN/OUT hasta punto fijo por unión monótona.
 * Restricciones: recorre también bloques inalcanzables; no alteran entradas alcanzables.
 */
static void solve(TacLiveAnalysis *a, const TacOperand *roots, size_t count) {
    TacLiveSet *out = new_set(a);
    TacLiveSet *in = new_set(a);
    int changed;
    do {
        changed = 0;
        for (size_t k = a->block_count; k-- > 0;) {
            TacLiveBlock *view = &a->blocks[k];
            memset(out->words, 0, out->width * sizeof *out->words);
            for (size_t s = 0; s < view->block->successor_count; s++)
                union_set(out, tac_live_block(a, view->block->successors[s])->live_in);
            if (program_exit(a, view->block)) {
                for (size_t r = 0; r < count; r++)
                    put(out, location_index(a, roots[r]));
            }
            transfer(in, view->use, out, view->def);
            changed |= copy_set(view->live_out, out);
            changed |= copy_set(view->live_in, in);
        }
    } while (changed);
    free(out->words);
    free(out);
    free(in->words);
    free(in);
}

/* Entrada: punto fijo completo. Salida: ninguna.
 * Efectos: vivos por instrucción y candidatos de preservación separados.
 * Restricciones: auxiliares virtuales desconocidos generan candidatos conservadores.
 */
static void instruction_liveness(TacLiveAnalysis *a) {
    for (size_t b = 0; b < a->block_count; b++) {
        const TacLiveSet *after = a->blocks[b].live_out;
        for (size_t n = a->offsets[b + 1]; n-- > a->offsets[b];) {
            TacLiveInstruction *view = &a->instructions[n];
            copy_set(view->after, after);
            transfer(view->before, view->uses, view->after, view->definitions);
            if (view->call_clobbers || view->scratch_count) {
                for (size_t k = 0; k < a->count; k++) {
                    TacOperand location = a->locations[k];
                    int affected = location.kind == TAC_TEMP;
                    if (location.kind == TAC_PHYS) {
                        uint16_t mask = view->call_clobbers;
                        if (view->scratch_count)
                            mask |= TAC_P1_CALL_CLOBBERS;
                        affected = (mask & (1u << location.value.id)) != 0;
                    }
                    if (affected && bit(view->before, k) && bit(view->after, k) &&
                        !bit(view->definitions, k))
                        put(view->preserve, k);
                }
            }
            after = view->before;
        }
    }
}

/* tac_live_analyze — contrato público en tac_live.h.
 * Entrada: función validable y raíces escalares. Salida: resultado propio o error.
 * Efectos: reserva análisis; no modifica TAC. Restricciones: Aho §9.2.5.
 */
TacLiveAnalysis *tac_live_analyze(const TacFunction *function, const TacOperand *exit_roots,
                                 size_t root_count, char *error, size_t capacity) {
    if (capacity)
        error[0] = '\0';
    if (!function || !function->program || !tac_has_function(function->program, function) || (root_count && !exit_roots) ||
        !tac_validate(function->program, error, capacity)) {
        tac_error(error, capacity, "vivacidad requiere función/programa válidos y raíces");
        return NULL;
    }
    for (size_t i = 0; i < root_count; i++) {
        if (exit_roots[i].kind != TAC_VARIABLE || !exit_roots[i].value.symbol ||
            exit_roots[i].value.symbol->kind != SYMBOL_VARIABLE || exit_roots[i].value.symbol->dimensions) {
            tac_error(error, capacity, "raíz observable requiere VARIABLE escalar");
            return NULL;
        }
    }
    TacLiveAnalysis *a = tac_allocate(sizeof *a);
    a->function = function;
    prepare(a);
    if (function->program->stage == TAC_STAGE_HIGH && !strcmp(function->symbol->name, "main")) {
        for (size_t i = 0; i < root_count; i++)
            collect_location(a, exit_roots[i], 0);
    }
    summarize(a);
    solve(a, exit_roots, root_count);
    instruction_liveness(a);
    return a;
}

/* Entrada: análisis y bloque prestado. Salida: vista o NULL. Efectos: ninguno.
 * Restricciones: válida hasta liberar el análisis; no consultar tras editar TAC.
 */
const TacLiveBlock *tac_live_block(const TacLiveAnalysis *a, const TacBlock *block) {
    if (a) {
        for (size_t i = 0; i < a->block_count; i++) {
            if (a->blocks[i].block == block)
                return &a->blocks[i];
        }
    }
    return NULL;
}

/* Entrada: análisis e instrucción. Salida: vista o NULL. Efectos: ninguno.
 * Restricciones: identidad por puntero, no por posición impresa.
 */
const TacLiveInstruction *tac_live_instruction(const TacLiveAnalysis *a, const TacInstruction *i) {
    if (a) {
        for (size_t n = 0; n < a->instruction_count; n++) {
            if (a->instructions[n].instruction == i)
                return &a->instructions[n];
        }
    }
    return NULL;
}

/* Entrada: análisis o NULL. Salida: tamaño del universo. Efectos: ninguno.
 * Restricciones: no es cantidad de registros necesarios simultáneamente.
 */
size_t tac_live_location_count(const TacLiveAnalysis *a) {
    return a ? a->count : 0;
}

/* Entrada: análisis/índice. Salida: ubicación o NONE fuera de rango. Efectos: ninguno.
 * Restricciones: los símbolos son prestados.
 */
TacOperand tac_live_location(const TacLiveAnalysis *a, size_t index) {
    return a && index < a->count ? a->locations[index] : tac_none();
}

/* Entrada: conjunto del análisis y ubicación. Salida: pertenencia. Efectos: ninguno.
 * Restricciones: no admite conjuntos pertenecientes a otro análisis.
 */
int tac_live_contains(const TacLiveAnalysis *a, const TacLiveSet *set, TacOperand location) {
    if (!a || !set)
        return 0;
    size_t index = location_index(a, location);
    return index != SIZE_MAX && bit(set, index);
}

/* Entrada: conjunto propio o NULL. Salida: ninguna. Efectos: libera.
 * Restricciones: solo usada por la liberación del propietario.
 */
static void free_set(const TacLiveSet *set) {
    if (set) {
        free(set->words);
        free((void *)set);
    }
}

/* Entrada: resultado propio o NULL. Salida: ninguna. Efectos: libera todo el análisis.
 * Restricciones: no libera función, instrucciones ni símbolos prestados.
 */
void tac_live_free(TacLiveAnalysis *a) {
    if (!a)
        return;
    for (size_t i = 0; i < a->instruction_count; i++) {
        free_set(a->instructions[i].uses);
        free_set(a->instructions[i].definitions);
        free_set(a->instructions[i].before);
        free_set(a->instructions[i].after);
        free_set(a->instructions[i].preserve);
    }
    for (size_t i = 0; i < a->block_count; i++) {
        free_set(a->blocks[i].use);
        free_set(a->blocks[i].def);
        free_set(a->blocks[i].live_in);
        free_set(a->blocks[i].live_out);
    }
    free(a->offsets);
    free(a->instructions);
    free(a->blocks);
    free(a->locations);
    free(a);
}
