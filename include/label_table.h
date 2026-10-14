/*
 * label_table.h — Fase 3: interfaz de construcción y resolución de la tabla de etiquetas.
 *
 * Expone roles, entradas, construcción, consulta y resolución de etiquetas.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: ast.h.
 * Lo usa: label_table.c; main.c y generate.c.
 * Referencias: libro §6.7 (backpatching); enunciado §2.3–§2.5.
 */

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

    /* Etiquetas de condicional */
    LABEL_NEXT,
    LABEL_END,

    /* Etiquetas de ciclo */
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

/* ==========================================================================
 * CONSTRUCCIÓN, CONSULTA Y RESOLUCIÓN
 * ======================================================================= */

/*
 * label_table_init — inicializa una tabla vacía de etiquetas.
 *
 * Entrada:
 *   table  tabla o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: pone entries a NULL y next_id a cero.
 * Restricciones:
 *   liberar tabla anterior antes de reinicializar.
 */
void label_table_init(LabelTable *table);

/*
 * label_table_build — construye las etiquetas de control y globales.
 *
 * Entrada:
 *   table  tabla inicializada vacía.
 *   root  raíz AST o NULL.
 * Salida:
 *   0 si completa; -1 si entrada o reserva falla.
 *   Efectos: registra etiquetas y reinicia next_id.
 * Restricciones:
 *   una sola construcción por tabla vacía; fallo puede dejar entradas que
 *   se liberan con destroy.
 */
int label_table_build(LabelTable *table, ASTNode *root);

/*
 * label_table_get — consulta una etiqueta por nodo y rol.
 *
 * Entrada:
 *   table  tabla o NULL.
 *   node  nodo o NULL para etiqueta global.
 *   role  rol buscado.
 * Salida:
 *   nombre interno o NULL si no existe.
 *   Efectos: ninguno.
 * Restricciones:
 *   no liberar el nombre; válido hasta label_table_destroy.
 */
const char *label_table_get(
    const LabelTable *table,
    const ASTNode *node,
    LabelRole role
);

/*
 * label_table_resolve — completa direcciones por consulta de nombre.
 *
 * Entrada:
 *   table  tabla o NULL.
 *   lookup  callback de dirección o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza address de cada entrada.
 * Restricciones:
 *   lookup devuelve índice de bundle o -1; usar calendario final antes de
 *   resolver.
 */
void label_table_resolve(LabelTable *table, int (*lookup)(const char *name));

/*
 * label_table_print — imprime nombres, nodos y direcciones de etiquetas.
 *
 * Entrada:
 *   table  tabla o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe en stdout; muestra - para direcciones negativas.
 * Restricciones:
 *   nodos prestados todavía vivos.
 */
void label_table_print(const LabelTable *table);

/*
 * label_table_destroy — libera todas las entradas de la tabla.
 *
 * Entrada:
 *   table  tabla o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera nombres/entradas y reinicia contador.
 * Restricciones:
 *   no libera nodos del AST prestados.
 */
void label_table_destroy(LabelTable *table);

#endif