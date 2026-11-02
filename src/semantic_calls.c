/*
 * semantic_calls.c — Fase 3: grafo de llamadas y detección de recursión.
 *
 * Registra las funciones y las llamadas entre ellas durante el análisis de
 * cuerpos y, al final, recorre el grafo en profundidad con tres colores
 * para encontrar ciclos. Una función que aparece en la pila del recorrido y
 * es llamada de nuevo cierra un ciclo y se reporta como error semántico.
 *
 * Usa: semantic_calls.h, semantic_internal.h.
 * Lo usa: semantic.c y semantic_expr.c.
 * Referencias: libro §7.1.1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantic_calls.h"
#include "semantic_internal.h"

// S-05: se prohíben ciclos de llamadas y se mantiene asignación estática.
// El generador sí usa pila para rl, argumentos escalares y temporales vivos.
// Cada llamada de la pasada 2 guarda (llamadora, llamada, nodo CALL)
// en orden de aparición. R no es una llamada.
typedef struct {
    Symbol *caller;
    Symbol *callee;
    ASTNode *call;
} CallEdge;

static CallEdge *call_edges = NULL;
static int call_count = 0;
static int call_capacity = 0;

// Funciones analizadas en la pasada 2, en el orden del archivo.
static Symbol **function_list = NULL;
static int function_count = 0;
static int function_capacity = 0;

// Colores del recorrido en profundidad.
enum { WHITE, GRAY, BLACK };

/* ==========================================================================
 * GRAFO DE FUNCIONES Y LLAMADAS
 * ======================================================================= */


/* calls_add_edge — véase el contrato en semantic_calls.h. */
void calls_add_edge(Symbol *caller, Symbol *callee, ASTNode *call) {
    if (call_count == call_capacity) {

        int capacity = call_capacity == 0 ? 16 : call_capacity * 2;

        CallEdge *edges =
            realloc(call_edges, (size_t) capacity * sizeof(CallEdge));

        if (edges == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(EXIT_FAILURE);
        }

        call_edges = edges;
        call_capacity = capacity;
    }

    call_edges[call_count].caller = caller;
    call_edges[call_count].callee = callee;
    call_edges[call_count].call = call;
    call_count++;
}

/* calls_add_function — véase el contrato en semantic_calls.h. */
void calls_add_function(Symbol *function) {
    if (function_count == function_capacity) {

        int capacity = function_capacity == 0 ? 16 : function_capacity * 2;

        Symbol **list =
            realloc(function_list, (size_t) capacity * sizeof(Symbol *));

        if (list == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(EXIT_FAILURE);
        }

        function_list = list;
        function_capacity = capacity;
    }

    function_list[function_count] = function;
    function_count++;
}

/* calls_reset — véase el contrato en semantic_calls.h. */
void calls_reset(void) {
    free(call_edges);
    free(function_list);

    call_edges = NULL;
    call_count = 0;
    call_capacity = 0;

    function_list = NULL;
    function_count = 0;
    function_capacity = 0;
}

/*
 * function_index — busca una función en la lista del grafo.
 *
 * Entrada:
 *   function  símbolo de función.
 * Salida:
 *   índice desde 0; -1 si no aparece.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int function_index(Symbol *function) {
    for (int i = 0; i < function_count; i++) {

        if (function_list[i] == function)
            return i;
    }

    return -1;
}

/*
 * report_cycle — reporta el ciclo que cierra una arista.
 *
 * Entrada:
 *   stack  pila de índices del recorrido.
 *   from  primera posición del ciclo.
 *   depth  posición de la función actual.
 *   call  nodo CALL que cierra el ciclo.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reserva mensaje, reporta y lo libera.
 * Restricciones:
 *   0 <= from <= depth; índices pertenecen a function_list.
 */
static void report_cycle(int *stack, int from, int depth, ASTNode *call) {
    size_t size = 1;

    for (int i = from; i <= depth; i++)
        size += strlen(function_list[stack[i]]->name) + 4;   // " -> "

    size += strlen(function_list[stack[from]]->name);

    char *text = malloc(size);

    if (text == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    text[0] = '\0';

    for (int i = from; i <= depth; i++) {
        strcat(text, function_list[stack[i]]->name);
        strcat(text, " -> ");
    }

    strcat(text, function_list[stack[from]]->name);

    semantic_error(call, "ciclo de llamadas: %s", text);

    free(text);
}

/*
 * visit_function — recorre llamadas con colores blanco, gris y negro.
 *
 * Entrada:
 *   index  índice de función.
 *   colors  arreglo de colores.
 *   stack  arreglo de índices de la pila.
 *   depth  profundidad actual.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza colores/stack y reporta ciclos.
 * Restricciones:
 *   índice válido; arreglos con function_count entradas; ignora aristas
 *   repetidas.
 */
static void visit_function(int index, int *colors, int *stack, int depth) {
    colors[index] = GRAY;
    stack[depth] = index;

    for (int e = 0; e < call_count; e++) {

        if (call_edges[e].caller != function_list[index])
            continue;

        // Solo la primera llamada de cada par (quien llama, a quien llama),
        // para que cada ciclo se reporte una vez.
        int repeated = 0;

        for (int p = 0; p < e && !repeated; p++)
            repeated = call_edges[p].caller == call_edges[e].caller &&
                       call_edges[p].callee == call_edges[e].callee;

        if (repeated)
            continue;

        int callee = function_index(call_edges[e].callee);

        if (callee < 0)
            continue;

        if (colors[callee] == GRAY) {

            // La función llamada está en la pila: hay un ciclo.
            int from = 0;

            while (stack[from] != callee)
                from++;

            report_cycle(stack, from, depth, call_edges[e].call);
        }
        else if (colors[callee] == WHITE) {
            visit_function(callee, colors, stack, depth + 1);
        }
    }

    // Una función negra no se vuelve a recorrer.
    colors[index] = BLACK;
}

/* calls_check_cycles — véase el contrato en semantic_calls.h. */
void calls_check_cycles(void) {
    if (function_count == 0)
        return;

    int *colors = calloc((size_t) function_count, sizeof(int));   // WHITE
    int *stack = calloc((size_t) function_count, sizeof(int));

    if (colors == NULL || stack == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < function_count; i++) {

        if (colors[i] == WHITE)
            visit_function(i, colors, stack, 0);
    }

    free(colors);
    free(stack);
}
