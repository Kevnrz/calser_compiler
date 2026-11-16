/* Renombramiento local HIGH: conserva la última definición en cada bloque.
 * Usa identidades y propiedades compartidas; Aho §8.5, §10.2.3; diseño §11.3.
 * Entrada: contexto HIGH válido. Salida: éxito o detalle, sin imprimir.
 * Efectos: sustituye destinos/lecturas y acumula definiciones renombradas.
 */
#include "tac_passes.h"
#include "tac_live.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const Symbol *variable;
    size_t total;
    size_t seen;
    TacOperand current;
} RenameEntry;

/* Entrada: tabla y símbolo prestados. Salida: entrada por identidad o NULL.
 * Efectos: ninguno. Restricciones: no compara nombres ni direcciones, §11.3.
 */
static RenameEntry *find_entry(RenameEntry *table, size_t count, const Symbol *variable) {
    for (size_t k = 0; k < count; k++)
        if (table[k].variable == variable)
            return &table[k];
    return NULL;
}

/* Entrada: función, bloque y ámbitos. Salida: tabla propia con total >= 2.
 * Efectos: cuenta también las inicializaciones implícitas, diseño §4.5.
 * Restricciones: NULL con failed si falta memoria; cero entradas no es error.
 */
static RenameEntry *collect(const TacPassContext *context, const TacFunction *function,
                           const TacBlock *block, size_t *count, int *failed,
                           char *detail, size_t capacity) {
    size_t maximum = 0;
    for (const TacInstruction *i = block->first; i; i = i->next) {
        TacOperand definitions[3];
        maximum += tac_definitions(i, definitions);
    }
    RenameEntry *table = maximum ? calloc(maximum, sizeof *table) : NULL;
    if (maximum && !table) {
        *failed = 1;
        if (capacity)
            snprintf(detail, capacity, "sin memoria para tabla de renombramiento");
        return NULL;
    }
    *count = 0;
    for (const TacInstruction *i = block->first; i; i = i->next) {
        TacOperand definitions[3];
        size_t n = tac_definitions(i, definitions);
        for (size_t k = 0; k < n; k++) {
            TacOperand d = definitions[k];
            if (d.kind != TAC_VARIABLE ||
                !tac_variable_is_rename_candidate(function, d.value.symbol, context->scopes))
                continue;
            RenameEntry *entry = find_entry(table, *count, d.value.symbol);
            if (!entry) {
                entry = &table[(*count)++];
                entry->variable = d.value.symbol;
                entry->current = tac_none();
            }
            entry->total++;
        }
    }
    size_t kept = 0;
    for (size_t k = 0; k < *count; k++)
        if (table[k].total >= 2)
            table[kept++] = table[k];
    *count = kept;
    return table;
}

/* Entrada: definición, variable y raíces. Salida: si puede diferir su escritura.
 * Efectos: ninguno. Restricciones: en main protege TODO may_trap/uses_globals
 * hasta la siguiente definición, también si lee explícitamente la variable;
 * decisión §4.6.3 y estado observable ante fallos de §11.1.
 */
static int can_rename(const TacInstruction *definition, const Symbol *variable,
                      int is_main, const TacOperand *roots, size_t root_count) {
    if (!is_main)
        return 1;
    size_t k = 0;
    while (k < root_count && !tac_location_equal(roots[k], tac_variable(variable)))
        k++;
    if (k == root_count)
        return 1;
    for (const TacInstruction *i = definition->next; i; i = i->next) {
        TacOperand definitions[3];
        size_t n = tac_definitions(i, definitions);
        for (k = 0; k < n; k++)
            if (tac_location_equal(definitions[k], tac_variable(variable)))
                return 1;
        TacOpInfo info;
        tac_instruction_info(i, &info);
        if (info.may_trap || info.uses_globals)
            return 0;
    }
    return 1;
}

/* Entrada: bloque y raíces prestados. Salida: definiciones renombradas o fallo.
 * Efectos: sustituye lecturas antes de procesar la escritura de cada nodo.
 * Restricciones: última definición intacta; no cruza bloques, Aho §8.5.
 */
static size_t rename_block(TacPassContext *context, TacFunction *function, TacBlock *block,
                          int is_main, const TacOperand *roots, size_t root_count,
                          int *failed, char *detail, size_t capacity) {
    size_t count = 0;
    RenameEntry *table = collect(context, function, block, &count, failed, detail, capacity);
    size_t renamed = 0;
    if (!count) {
        free(table);
        return 0;
    }
    for (TacInstruction *i = block->first; i && !*failed; i = i->next) {
        for (size_t k = 0; k < count; k++) {
            RenameEntry *entry = &table[k];
            if (entry->current.kind != TAC_NONE &&
                !tac_replace_reads(block, i, tac_variable(entry->variable), entry->current,
                                   NULL, detail, capacity)) {
                *failed = 1;
                break;
            }
        }
        if (*failed)
            break;
        TacOperand definitions[3];
        size_t n = tac_definitions(i, definitions);
        for (size_t k = 0; k < n; k++) {
            if (definitions[k].kind != TAC_VARIABLE)
                continue;
            RenameEntry *entry = find_entry(table, count, definitions[k].value.symbol);
            if (!entry)
                continue;
            entry->seen++;
            if (entry->seen < entry->total &&
                can_rename(i, entry->variable, is_main, roots, root_count)) {
                entry->current = tac_temp_new(function);
                i->dest = entry->current;
                renamed++;
            } else {
                entry->current = tac_none();
            }
        }
    }
    free(table);
    return renamed;
}

int tac_pass_rename(TacPassContext *context, char *detail, size_t capacity) {
    size_t root_count = 0;
    TacOperand *roots = tac_observable_roots(context->scopes, &root_count);
    int failed = 0;
    for (TacFunction *f = context->program->first; f && !failed; f = f->next) {
        int is_main = !strcmp(f->symbol->name, "main");
        for (TacBlock *b = f->first; b && !failed; b = b->next)
            context->metrics->counters.renamed +=
                rename_block(context, f, b, is_main, roots, root_count, &failed, detail, capacity);
    }
    free(roots);
    return !failed;
}
