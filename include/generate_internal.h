/*
 * generate_internal.h — Fase 4: lo que comparten los módulos del generador de código.
 *
 * Define GenCtx, el estado de una generación, y las funciones que un módulo
 * de generate_*.c le pide a otro. Todo el estado viaja en GenCtx: ningún
 * módulo tiene variables globales. generate.h sigue siendo la única
 * interfaz pública (generate_code).
 *
 * Usa: ast.h, symbol_table.h, label_table.h, codegen_arith.h, codegen_crypto.h.
 * Lo usa: generate.c, generate_support.c, generate_expr.c, generate_stmt.c y
 * generate_init.c.
 * Referencias: libro §6.4, §7.2; isa.md §6, §8, §9.
 */

#ifndef GENERATE_INTERNAL_H
#define GENERATE_INTERNAL_H

#include <stddef.h>

#include "ast.h"
#include "codegen_arith.h"
#include "codegen_crypto.h"
#include "label_table.h"
#include "symbol_table.h"

#define TEMP_FIRST 4
#define TEMP_COUNT 12

/*
 * GenCtx — estado de una generación de código.
 *
 * Reúne lo que antes eran variables static del módulo, para que cada función
 * reciba el estado por parámetro y dos generaciones no compartan nada. Se
 * crea en generate_code y vive mientras dura la generación.
 */
typedef struct GenCtx {
    const char *source_name;              // nombre prestado para diagnósticos; NULL usa ccom
    const LabelTable *label_table;        // tabla de la Fase 3 (solo se consulta)

    // Etiquetas globales (nodo NULL en la tabla), leídas al empezar.
    const char *arith_error_label;        // destino de / % ^ con error
    const char *program_end_label;        // fin_main (contrato §3)

    CGArithContext arithmetic;            // plantillas de aritmética y arreglos
    CGCryptoContext crypto;               // plantillas CRYPTO; crypto.user = este GenCtx
    ASTNode *program_globals;             // AST_GLOBALS, para main

    int temp_used[TEMP_COUNT];            // temporales t0–t11 reservados
    int generation_errors;                // errores de generación reportados

    const char *current_function_name;    // función que se está generando
    Symbol *current_function_symbol;
    int inside_main;                      // 1 mientras se genera main
} GenCtx;

/* ==========================================================================
 * DIAGNÓSTICOS, REGISTROS, MEMORIA Y PILA (generate_support.c)
 * ======================================================================= */


/*
 * gen_report_error — reporta un error de generación.
 *
 * Entrada:
 *   node  nodo para posición o NULL.
 *   message  mensaje no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe en stderr e incrementa generation_errors.
 * Restricciones:
 *   source_name, si existe, debe vivir durante la generación; NULL usa ccom.
 */
void gen_report_error(GenCtx *gen, ASTNode *node, const char *message);

/*
 * gen_child_at — consulta un hijo por posición.
 *
 * Entrada:
 *   node  nodo o NULL.
 *   index  posición desde 0.
 * Salida:
 *   hijo prestado o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   index no negativo; los hijos opcionales NULL no ocupan posición.
 */
ASTNode *gen_child_at(ASTNode *node, int index);

/*
 * gen_reg_name — obtiene el alias de un registro ISA.
 *
 * Entrada:
 *   reg  número ISA.
 * Salida:
 *   cadena literal; rz si reg está fuera de 0–15.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
const char *gen_reg_name(int reg);

/*
 * gen_reset_registers — libera las reservas de temporales del generador.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: pone temp_used a cero.
 * Restricciones:
 *   usar cuando no se necesitan las reservas anteriores.
 */
void gen_reset_registers(GenCtx *gen);

/*
 * gen_alloc_reg — reserva el primer temporal libre.
 *
 * Entrada: ninguna
 * Salida:
 *   registro ISA 4–15; -1 si no hay disponibles.
 *   Efectos: marca el temporal en temp_used.
 * Restricciones:
 *   solo dispone de t0–t11.
 */
int gen_alloc_reg(GenCtx *gen);

/*
 * gen_free_reg — libera la reserva de un temporal.
 *
 * Entrada:
 *   reg  registro ISA a liberar.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: desmarca temp_used si reg está entre 4 y 15.
 * Restricciones:
 *   ninguna.
 */
void gen_free_reg(GenCtx *gen, int reg);

/*
 * gen_mov — construye una constante de hasta 32 bits.
 *
 * Entrada:
 *   rd  registro destino ISA reservado.
 *   immediate  constante entre INT32_MIN y UINT32_MAX.
 *   node  nodo para diagnósticos o NULL.
 * Salida:
 *   0 si emite; -1 si reporta error.
 *   Efectos: emite instrucciones y reserva/libera un temporal auxiliar si hace falta.
 * Restricciones:
 *   fuera del inmediato de 18 bits necesita un temporal libre; rd debe
 *   seguir reservado.
 */
int gen_mov(GenCtx *gen, int rd, long immediate, ASTNode *node);

/*
 * gen_get_symbol — consulta la anotación semántica de un nodo.
 *
 * Entrada:
 *   node  nodo o NULL.
 * Salida:
 *   símbolo prestado o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   semantic_free no se ha llamado.
 */
Symbol *gen_get_symbol(ASTNode *node);

/*
 * gen_get_return_symbol — busca la variable de retorno de una función.
 *
 * Entrada:
 *   function_symbol  símbolo de función o NULL.
 * Salida:
 *   símbolo prestado de retorno o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   decl_node sigue vivo; void no tiene variable de retorno.
 */
Symbol *gen_get_return_symbol(Symbol *function_symbol);

/*
 * gen_load_symbol — carga una casilla estática en un registro.
 *
 * Entrada:
 *   rd  registro destino ISA.
 *   symbol  símbolo a cargar.
 *   node  nodo para diagnósticos o NULL.
 * Salida:
 *   0 si emite; -1 si reporta error.
 *   Efectos: emite lw en LSU; reporta direcciones inválidas.
 * Restricciones:
 *   dirección asignada entre 0 y 131071; el semántico garantiza
 *   alineación.
 */
int gen_load_symbol(GenCtx *gen, int rd, Symbol *symbol, ASTNode *node);

/*
 * gen_store_symbol — guarda un registro en una casilla estática.
 *
 * Entrada:
 *   symbol  símbolo destino.
 *   rs  registro fuente ISA.
 *   node  nodo para diagnósticos o NULL.
 * Salida:
 *   0 si emite; -1 si reporta error.
 *   Efectos: emite sw en LSU; reporta direcciones inválidas.
 * Restricciones:
 *   dirección asignada entre 0 y 131071; mutabilidad ya verificada por el
 *   semántico.
 */
int gen_store_symbol(GenCtx *gen, Symbol *symbol, int rs, ASTNode *node);

/*
 * gen_stack_push — apila una palabra en rx.
 *
 * Entrada:
 *   reg  registro ISA cuyo valor se conserva.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite sw y avance de rx en cuatro bytes.
 * Restricciones:
 *   rx apunta a la primera casilla libre; no verifica desborde en
 *   ejecución.
 */
void gen_stack_push(int reg);

/*
 * gen_stack_pop — desapila una palabra desde rx.
 *
 * Entrada:
 *   reg  registro ISA que recibe la palabra.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite retroceso de rx y lw.
 * Restricciones:
 *   debe existir una palabra apilada; no verifica subdesborde.
 */
void gen_stack_pop(int reg);

/*
 * gen_node_label — copia una etiqueta de control de la tabla.
 *
 * Entrada:
 *   buffer  buffer escribible.
 *   size  capacidad del buffer en bytes.
 *   node  nodo del AST o NULL para etiquetas globales.
 *   role  rol de la etiqueta.
 *   what  descripción no NULL para el diagnóstico.
 * Salida:
 *   1 si copia; 0 si falta o no cabe.
 *   Efectos: escribe buffer o reporta error.
 * Restricciones:
 *   tabla de Fase 3 vigente; no crea etiquetas nuevas.
 */
int gen_node_label(GenCtx *gen, char *buffer, size_t size, ASTNode *node,
                      LabelRole role, const char *what);

/* ==========================================================================
 * EXPRESIONES Y LLAMADAS (generate_expr.c)
 * ======================================================================= */


/*
 * gen_expression — evalúa una expresión del AST anotado.
 *
 * Entrada:
 *   node  nodo de expresión validado semánticamente.
 * Salida:
 *   temporal ISA 4–15 reservado con el valor; -1 si falla.
 *   Efectos: emite instrucciones, administra temporales y reporta errores.
 * Restricciones:
 *   el llamador libera el resultado; índices dinámicos deben estar dentro
 *   del arreglo; AST_ARGUMENTS presente en accesos.
 */
int gen_expression(GenCtx *gen, ASTNode *node);

/*
 * gen_function_call — genera una llamada y preserva los temporales vivos.
 *
 * Entrada:
 *   node  nodo AST_CALL de una función.
 *   result_needed  1 si se necesita el resultado, 0 para sentencia.
 * Salida:
 *   registro reservado de retorno si se pidió; 0 si se descarta; -1 si falla.
 *   Efectos: emite argumentos, pila de rx, jas y restauración de rl y temporales.
 * Restricciones:
 *   análisis semántico exitoso y símbolos vivos; lee el retorno estático
 *   antes de otra llamada; falla puede dejar emisión parcial.
 * Referencia: libro §7.2; isa.md §6.2.
 */
int gen_function_call(GenCtx *gen, ASTNode *node, int result_needed);

/*
 * gen_crypto_statement — expande una sentencia de cifrado con sus valores vivos.
 *
 * Entrada:
 *   node  nodo AST_CRYPTO validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: configura crypto_context, emite expansión y reporta errores; restaura temp_used.
 * Restricciones:
 *   se descarta toda la generación si la expansión falla.
 */
void gen_crypto_statement(GenCtx *gen, ASTNode *node);

/*
 * gen_array_indices — evalúa los índices de un acceso a arreglo y los valida.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo del acceso (INDEX o CALL), para los diagnósticos.
 *   array  símbolo del arreglo, con dimensions entre 1 y MAX_DIMS.
 *   first  primer índice de la lista (hijos de AST_ARGUMENTS).
 *   indices  arreglo de MAX_DIMS enteros escribibles.
 *   count  puntero no NULL para la cantidad de índices evaluados.
 * Salida:
 *   0 si hay exactamente array->dimensions índices evaluados; -1 si falla.
 *   Efectos: deja un temporal reservado por índice en indices[0..*count-1]
 *   (arreglo 1D: el índice; 2D: fila y columna); ante un error libera los
 *   que había reservado y reporta.
 * Restricciones:
 *   el llamador libera los índices al terminar de usarlos. Lo comparten la
 *   lectura de un elemento y la asignación a un elemento.
 */
int gen_array_indices(GenCtx *gen, ASTNode *node, Symbol *array,
                      ASTNode *first, int indices[MAX_DIMS], int *count);

/*
 * gen_call_arguments — obtiene la lista de argumentos de una llamada o de un R.
 *
 * Entrada:
 *   args_node  hijo de argumentos del nodo, o NULL si no hay.
 *   count  puntero no NULL para la cantidad de argumentos.
 * Salida:
 *   primer argumento (hermanos enlazados por next), o NULL sin argumentos.
 *   Efectos: escribe *count.
 * Restricciones:
 *   AST_ARGUMENTS contiene las expresiones como hijos; si el nodo no es un
 *   AST_ARGUMENTS se toma él mismo como el primer argumento.
 */
ASTNode *gen_call_arguments(ASTNode *args_node, int *count);

/*
 * gen_expr_setup — prepara los contextos de aritmética, arreglos y CRYPTO.
 *
 * Entrada:
 *   gen  contexto de la generación, ya con cg_init hecho.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: instala los callbacks de emisión y de evaluación de argumentos
 *   y deja los contadores de etiquetas internas en su valor inicial.
 * Restricciones:
 *   una vez por programa, antes de generar cualquier expresión; las
 *   etiquetas internas __ccom_arith_N_*, __ccom_array_N_* y .L_crypto_* se
 *   numeran seguidas entre funciones.
 */
void gen_expr_setup(GenCtx *gen);

/* ==========================================================================
 * SENTENCIAS Y CONTROL DE FLUJO (generate_stmt.c)
 * ======================================================================= */


/*
 * gen_block — genera las sentencias de un bloque en orden.
 *
 * Entrada:
 *   block  nodo AST_BLOCK o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite las sentencias y sus diagnósticos.
 * Restricciones:
 *   contexto de función configurado; NULL representa bloque ausente.
 */
void gen_block(GenCtx *gen, ASTNode *block);

/*
 * gen_statement — despacha una sentencia por su clase.
 *
 * Entrada:
 *   node  nodo de sentencia o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza línea de emisión; emite código y reporta clases no soportadas.
 * Restricciones:
 *   contexto de función configurado y análisis semántico exitoso.
 */
void gen_statement(GenCtx *gen, ASTNode *node);

/*
 * gen_end_of_main — define el destino terminal del programa.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite program_end_label y un bundle con salto a sí mismo.
 * Restricciones:
 *   etiqueta global consultada al iniciar; llamar una vez por programa.
 */
void gen_end_of_main(GenCtx *gen);

/* ==========================================================================
 * INICIALIZACIÓN DE VARIABLES (generate_init.c)
 * ======================================================================= */


/*
 * gen_initialize_declaration — inicializa todas las casillas de una declaración.
 *
 * Entrada:
 *   node  nodo AST_DECLARATION.
 *   image  1 para imagen estática, 0 para instrucciones.
 *   dynamic_only  1 para emitir solo valores dinámicos.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe imagen o emite instrucciones; rellena casillas restantes con cero.
 * Restricciones:
 *   símbolo y ancho asignados por el semántico.
 */
void gen_initialize_declaration(GenCtx *gen, ASTNode *node, int image, int dynamic_only);

#endif
