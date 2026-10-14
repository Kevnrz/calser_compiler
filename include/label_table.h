#ifndef LABEL_TABLE_H
#define LABEL_TABLE_H

#include "ast.h"

/* Tabla de etiquetas: una etiqueta por cada punto de control del programa
 * (funciones, C/A, L, S, corto circuito de & y |) más las globales.
 *
 * Se construye en la Fase 3, a partir del AST, y se imprime con -m. La
 * Fase 4 (generate.c) la consume: cada etiqueta de control del .asm sale de
 * label_table_get, así que la tabla de -m y el .asm coinciden.
 *
 * No están aquí las etiquetas internas de cada expansión, que no son nodos
 * del AST: __ccom_arith_N_* (rutinas aritméticas), __ccom_array_N_*
 * (copia y puesta en cero de arreglos) y .L_crypto_* (plantillas de
 * cifrado). */

typedef enum {
    LABEL_FUNCTION,

    /* Conditional labels */
    LABEL_NEXT,
    LABEL_END,

    /* Loop labels */
    LABEL_START,
    LABEL_UPDATE,

    LABEL_BODY,
    LABEL_SC_END,
    LABEL_ARITH_ERROR,

    /* fin_main: donde termina el programa (fase4_contrato_comun.md §3). */
    LABEL_PROGRAM_END

} LabelRole;

typedef struct LabelEntry {
    ASTNode *node;  /* NULL para las etiquetas globales (error aritmético y
                       fin del programa). */
    LabelRole role;
    char *name;

    struct LabelEntry *next;
} LabelEntry;

typedef struct {
    LabelEntry *entries;
    unsigned int next_id;
} LabelTable;

void label_table_init(LabelTable *table);

int label_table_build(LabelTable *table, ASTNode *root);

/* Para LABEL_ARITH_ERROR y LABEL_PROGRAM_END, consultar con node = NULL. */
const char *label_table_get(
    const LabelTable *table,
    const ASTNode *node,
    LabelRole role
);

void label_table_print(const LabelTable *table);

void label_table_destroy(LabelTable *table);

#endif