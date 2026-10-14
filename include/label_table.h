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

/* Qué fase llena cada campo de una entrada (enunciado CE1108 §2.3 a §2.5):
 *   - node, role y name: la Fase 3, al armar la tabla a partir del AST
 *     (§2.3, la tabla registra las etiquetas);
 *   - address: la dirección de la etiqueta (su índice de bundle). Solo se
 *     conoce después de generar el código (la Fase 4 lleva el contador de
 *     instrucciones, §2.4) y se completa con label_table_resolve, a partir
 *     de la primera pasada del ensamblado (Fase 5, §2.5).
 *
 * Es el esquema de backpatching del libro §6.7: la etiqueta se registra
 * antes de conocer su dirección y la dirección se completa después. */
typedef struct LabelEntry {
    ASTNode *node;  /* NULL para las etiquetas globales (error aritmético y
                       fin del programa). */
    LabelRole role;
    char *name;
    int address;    /* índice de bundle; -1 mientras no se resuelve (y para
                       las etiquetas que no llegan al .asm) */

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

/* Completa el campo address de cada entrada con lookup(nombre): el índice del
 * bundle donde está definida la etiqueta, o -1 si no lo está. */
void label_table_resolve(LabelTable *table, int (*lookup)(const char *name));

void label_table_print(const LabelTable *table);

void label_table_destroy(LabelTable *table);

#endif