/*
 * ast.h — Fase 2: interfaz de construcción y representación del árbol de sintaxis abstracta.
 *
 * Expone ASTKind, ASTNode, constructores, posiciones y exportación JSON.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: tipos de la biblioteca C y declaraciones propias.
 * Lo usa: ast.c; parser.y, semantic.c, label_table.c, generate.c y main.c.
 * Referencias: libro §2.5, §6.1.
 */

#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef enum {
    AST_PROGRAM,
    AST_GLOBALS,
    AST_FUNCTIONS,
    AST_FUNCTION,

    AST_PARAMS,
    AST_PARAMETER,

    AST_RETURN_TYPE,
    AST_TYPE,
    AST_MODIFIER,
    AST_DIMENSIONS,

    AST_BLOCK,
    AST_DECLARATION,
    AST_INITIALIZER_LIST,

    AST_ASSIGNMENT,
    AST_CALL_STATEMENT,

    AST_CONDITIONAL,
    AST_ALTERNATIVES,
    AST_ALTERNATIVE,
    AST_RESIDUAL,

    AST_LOOP_L,
    AST_LOOP_S,

    AST_EMIT,
    AST_REWIND,
    AST_CRYPTO,

    AST_ARGUMENTS,

    AST_IDENTIFIER,
    AST_NUMBER,
    AST_BOOLEAN,

    AST_CALL,
    AST_INDEX,

    AST_OR,
    AST_AND,

    AST_EQUAL,
    AST_NOT_EQUAL,

    AST_LESS,
    AST_GREATER,
    AST_LESS_EQUAL,
    AST_GREATER_EQUAL,

    AST_ADD,
    AST_SUB,

    AST_MUL,
    AST_DIV,
    AST_MOD,

    AST_NEG,
    AST_NOT,

    AST_POWER

} ASTKind;

typedef struct ASTNode {
    ASTKind kind;

    char *text;
    long number;

    int line;     // línea del código fuente; 0 = sin línea
    int column;   // columna del código fuente; 0 = sin columna

    void *symbol;   // símbolo al que se refiere este nombre (no dueño); NULL si no aplica

    struct ASTNode *child;
    struct ASTNode *next;

} ASTNode;

/* ==========================================================================
 * NODOS, LISTAS Y POSICIONES
 * ======================================================================= */

/*
 * ast_new — crea un nodo sin hijos ni anotaciones.
 *
 * Entrada:
 *   kind  clase de nodo.
 * Salida:
 *   nodo nuevo.
 *   Efectos: reserva memoria o termina el proceso.
 * Restricciones:
 *   liberar con ast_free.
 */
ASTNode *ast_new(ASTKind kind);
/*
 * ast_add_child — agrega un hijo al final de una lista.
 *
 * Entrada:
 *   parent  padre o NULL.
 *   child  hijo o lista de hermanos, o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: enlaza child al padre; ignora NULL.
 * Restricciones:
 *   nodos sin ciclos ni propiedad compartida.
 */
void ast_add_child(ASTNode *parent, ASTNode *child);

/*
 * ast_set_location — asigna posición a nodos que no la tienen.
 *
 * Entrada:
 *   node  nodo o NULL.
 *   line  línea fuente.
 *   column  columna fuente.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: asigna posición y recorre hijos sin línea.
 * Restricciones:
 *   no desciende por hijos que ya tienen línea; no recorre hermanos del
 *   nodo raíz.
 */
void ast_set_location(ASTNode *node, int line, int column);
/*
 * ast_list — crea un contenedor de nodos enlazados.
 *
 * Entrada:
 *   kind  clase del contenedor.
 *   items  primer elemento o NULL.
 * Salida:
 *   nodo nuevo que posee la lista.
 *   Efectos: reserva nodo.
 * Restricciones:
 *   items sin propiedad compartida.
 */
ASTNode *ast_list(ASTKind kind, ASTNode *items);

/* ==========================================================================
 * PROGRAMA Y FUNCIONES
 * ======================================================================= */

/*
 * ast_program — construye un nodo de programa.
 *
 * Entrada:
 *   globals  globales o NULL.
 *   functions  funciones o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_program(ASTNode *globals,
                     ASTNode *functions);

/*
 * ast_function — construye un nodo de función.
 *
 * Entrada:
 *   name  nombre de función.
 *   params  parámetros o NULL.
 *   return_type  retorno.
 *   body  cuerpo.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_function(char *name,
                      ASTNode *params,
                      ASTNode *return_type,
                      ASTNode *body);


/* ==========================================================================
 * TIPOS, DIMENSIONES Y PARÁMETROS
 * ======================================================================= */

/*
 * ast_type — construye un nodo de tipo.
 *
 * Entrada:
 *   name  nombre del tipo o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_type(const char *name);
/*
 * ast_modifier — construye un nodo de modificador.
 *
 * Entrada:
 *   name  nombre del modificador o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_modifier(const char *name);
/*
 * ast_return_type — construye un nodo de retorno con variable.
 *
 * Entrada:
 *   type  tipo.
 *   name  nombre de variable de retorno.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_return_type(ASTNode *type,
                         char *name);
/*
 * ast_return_void — construye un nodo de retorno void.
 *
 * Entrada: ninguna
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_return_void(void);

/*
 * ast_dimensions — construye un nodo de lista vacía de dimensiones.
 *
 * Entrada: ninguna
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_dimensions(void);
/*
 * ast_add_number — agrega una dimensión numérica al nodo.
 *
 * Entrada:
 *   node  nodo o NULL.
 *   number  valor de dimensión.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: crea y enlaza NUMBER si node no es NULL.
 * Restricciones:
 *   validación de tamaño se hace en el semántico.
 */
void ast_add_number(ASTNode *node,
                    long number);


/*
 * ast_parameter — construye un nodo de parámetro.
 *
 * Entrada:
 *   modifier  modificador o NULL.
 *   type  tipo.
 *   name  nombre.
 *   dims  dimensiones o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_parameter(ASTNode *modifier,
                       ASTNode *type,
                       char *name,
                       ASTNode *dims);


/* ==========================================================================
 * SENTENCIAS Y CONTROL DE FLUJO
 * ======================================================================= */

/*
 * ast_block — construye un nodo de bloque vacío.
 *
 * Entrada: ninguna
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_block(void);

/*
 * ast_declaration — construye un nodo de declaración.
 *
 * Entrada:
 *   modifier  modificador.
 *   name  nombre.
 *   type  tipo.
 *   dims  dimensiones o NULL.
 *   initializer  inicializador o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_declaration(ASTNode *modifier,
                         char *name,
                         ASTNode *type,
                         ASTNode *dims,
                         ASTNode *initializer);

/*
 * ast_assignment — construye un nodo de asignación.
 *
 * Entrada:
 *   target  nombre destino.
 *   args  argumentos/índices o NULL.
 *   index  índice adicional o NULL.
 *   value  valor asignado.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_assignment(char *target,
                        ASTNode *args,
                        ASTNode *index,
                        ASTNode *value);

/*
 * ast_call_statement — construye un nodo de sentencia de llamada.
 *
 * Entrada:
 *   call  nombre de función.
 *   args  argumentos o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_call_statement(char *call,
                           ASTNode *args);

/*
 * ast_conditional — construye un nodo de condicional.
 *
 * Entrada:
 *   condition  condición.
 *   body  bloque inicial.
 *   alts  alternativas o NULL.
 *   residual  residual o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_conditional(ASTNode *condition,
                         ASTNode *body,
                         ASTNode *alts,
                         ASTNode *residual);

/*
 * ast_alternative — construye un nodo de rama alternativa.
 *
 * Entrada:
 *   condition  condición.
 *   body  cuerpo.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_alternative(ASTNode *condition,
                         ASTNode *body);

/*
 * ast_residual — construye un nodo de rama residual.
 *
 * Entrada:
 *   body  cuerpo.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_residual(ASTNode *body);

/*
 * ast_loop_l — construye un nodo de ciclo L.
 *
 * Entrada:
 *   condition  condición.
 *   body  cuerpo.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_loop_l(ASTNode *condition,
                    ASTNode *body);

/*
 * ast_loop_s — construye un nodo de ciclo S.
 *
 * Entrada:
 *   variable  nombre de control.
 *   start  inicio.
 *   end  fin.
 *   step  paso.
 *   body  cuerpo.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_loop_s(char *variable,
                    ASTNode *start,
                    ASTNode *end,
                    ASTNode *step,
                    ASTNode *body);

/*
 * ast_emit — construye un nodo de sentencia E.
 *
 * Entrada: ninguna
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_emit(void);

/*
 * ast_rewind — construye un nodo de sentencia R.
 *
 * Entrada:
 *   name  nombre de función.
 *   args  argumentos o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_rewind(char *name,
                    ASTNode *args);

/*
 * ast_crypto — construye un nodo de operación de cifrado.
 *
 * Entrada:
 *   operation  palabra reservada de la operación.
 *   args  argumentos o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_crypto(const char *operation,
                    ASTNode *args);

/* ==========================================================================
 * EXPRESIONES
 * ======================================================================= */

/*
 * ast_number — construye un nodo de literal entero.
 *
 * Entrada:
 *   value  valor entero.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_number(long value);
/*
 * ast_boolean — construye un nodo de literal bool.
 *
 * Entrada:
 *   value  valor almacenado, normalmente 0 o 1.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_boolean(int value);
/*
 * ast_arguments — construye un nodo de lista de argumentos.
 *
 * Entrada:
 *   arguments  expresiones enlazadas o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_arguments(ASTNode *arguments);
/*
 * ast_identifier — construye un nodo de identificador.
 *
 * Entrada:
 *   name  nombre o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo y copia texto si corresponde.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_identifier(const char *name);

/*
 * ast_binary — construye un nodo de operador binario.
 *
 * Entrada:
 *   kind  clase del operador.
 *   left  operando izquierdo.
 *   right  operando derecho.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_binary(ASTKind kind,
                    ASTNode *left,
                    ASTNode *right);

/*
 * ast_unary — construye un nodo de operador unario.
 *
 * Entrada:
 *   kind  clase del operador.
 *   child  operando.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_unary(ASTKind kind,
                   ASTNode *child);

/*
 * ast_call — construye un nodo de llamada o acceso de un corchete.
 *
 * Entrada:
 *   name  nombre.
 *   args  argumentos o NULL.
 * Salida:
 *   nodo nuevo; liberar con ast_free.
 *   Efectos: reserva nodo, copia nombres y toma propiedad de los hijos recibidos.
 * Restricciones:
 *   AST sin ciclos ni propiedad compartida; tipos y usos se verifican
 *   después.
 */
ASTNode *ast_call(char *name,
                  ASTNode *args);

/*
 * ast_index — construye un acceso con índices bajo ARGUMENTS.
 *
 * Entrada:
 *   name  nombre del arreglo.
 *   args  argumentos o lista de índices o NULL.
 *   index  índice adicional o ARGUMENTS o NULL.
 * Salida:
 *   nodo AST_INDEX nuevo.
 *   Efectos: copia nombre, toma índices y libera wrapper index si es ARGUMENTS.
 * Restricciones:
 *   nodos sin propiedad compartida; no valida cantidad de dimensiones.
 */
ASTNode *ast_index(char *name,
                   ASTNode *args,
                   ASTNode *index);


/* ==========================================================================
 * IMPRESIÓN Y LIBERACIÓN
 * ======================================================================= */

/*
 * ast_print — imprime el AST con sangría.
 *
 * Entrada:
 *   node  nodo o NULL.
 *   indent  profundidad inicial no negativa.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe árbol y hermanos en stdout.
 * Restricciones:
 *   AST sin ciclos.
 */
void ast_print(ASTNode *node,
               int indent);

/*
 * ast_write_json — exporta el AST sintáctico con esquema y fuente.
 *
 * Entrada:
 *   out  flujo o NULL.
 *   node  raíz o NULL.
 *   source  nombre de fuente o NULL.
 * Salida:
 *   0 si escribe; 1 si entrada inválida o ferror.
 *   Efectos: escribe JSON en out.
 * Restricciones:
 *   nodos vivos sin ciclos; no cierra el flujo.
 */
int ast_write_json(FILE *out, const ASTNode *node, const char *source);

/*
 * ast_free — libera nodo, hijos y hermanos.
 *
 * Entrada:
 *   node  nodo o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera texto y nodos del árbol/lista.
 * Restricciones:
 *   sin ciclos ni nodos compartidos; symbol es prestado y no se libera.
 */
void ast_free(ASTNode *node);

#endif