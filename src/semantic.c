/*
 * semantic.c — Fase 3: análisis de ámbitos, tipos y direcciones estáticas.
 *
 * Registra firmas antes de analizar cuerpos para admitir llamadas adelantadas.
 * Anota los usos con símbolos, verifica tipos, constantes y ciclos de llamadas,
 * y deja espacio mínimo para la pila de generación.
 *
 * Usa: AST, symbol_table.c, diagnostics.c, semantic_scope.c (ámbitos) y
 * semantic_calls.c (grafo de llamadas y ciclos), semantic_crypto.c (operaciones
 * de cifrado) y data_area.c (contador de la memoria de datos).
 * Lo usa: main.c.
 * Referencias: libro §1.6.3, §2.7, §6.3.4, §6.5, §7.1.1.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantic.h"
#include "semantic_internal.h"
#include "data_area.h"
#include "semantic_calls.h"
#include "semantic_scope.h"
#include "symbol_table.h"
#include "ast.h"
#include "diagnostics.h"

/* ==========================================================================
 * TIPOS DECLARADOS Y CONSULTAS DEL AST
 * ======================================================================= */


/*
 * get_type — convierte un nodo de tipo en tipo de símbolo.
 *
 * Entrada:
 *   node  nodo AST_TYPE o NULL.
 * Salida:
 *   TYPE_INT, TYPE_BOOL o TYPE_VOID; usa VOID ante ausencia o texto desconocido.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static SymbolType get_type(ASTNode *node) {
    if (node == NULL)
        return TYPE_VOID;

    if (node->kind != AST_TYPE)
        return TYPE_VOID;

    if (node->text == NULL)
        return TYPE_VOID;

    if (strcmp(node->text, "int") == 0)
        return TYPE_INT;

    if (strcmp(node->text, "bool") == 0)
        return TYPE_BOOL;

    if (strcmp(node->text, "void") == 0)
        return TYPE_VOID;

    return TYPE_VOID;
}


/*
 * get_modifier — convierte un modificador del AST.
 *
 * Entrada:
 *   node  nodo AST_MODIFIER o NULL.
 * Salida:
 *   MODIFIER_VM, MODIFIER_VI o MODIFIER_NONE.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static SymbolModifier get_modifier(ASTNode *node) {
    if (node == NULL)
        return MODIFIER_NONE;

    if (node->kind != AST_MODIFIER)
        return MODIFIER_NONE;

    if (node->text == NULL)
        return MODIFIER_NONE;

    if (strcmp(node->text, "Vm") == 0)
        return MODIFIER_VM;

    if (strcmp(node->text, "Vi") == 0)
        return MODIFIER_VI;

    return MODIFIER_NONE;
}

/* ==========================================================================
 * DIAGNÓSTICOS
 * ======================================================================= */


/*
 * semantic_operator_symbol — obtiene la grafía de un operador.
 *
 * Entrada:
 *   kind  clase del nodo AST.
 * Salida:
 *   cadena literal o NULL si no es operador.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
const char *semantic_operator_symbol(ASTKind kind) {
    switch (kind) {
        case AST_OR:            return "|";
        case AST_AND:           return "&";
        case AST_EQUAL:         return "==";
        case AST_NOT_EQUAL:     return "!=";
        case AST_LESS:          return "<";
        case AST_GREATER:       return ">";
        case AST_LESS_EQUAL:    return "<=";
        case AST_GREATER_EQUAL: return ">=";
        case AST_ADD:           return "+";
        case AST_SUB:           return "-";
        case AST_MUL:           return "*";
        case AST_DIV:           return "/";
        case AST_MOD:           return "%";
        case AST_POWER:         return "^";
        case AST_NEG:           return "-";
        case AST_NOT:           return "!";
        default:                return NULL;
    }
}

/*
 * count_digits — cuenta dígitos decimales sin el signo.
 *
 * Entrada:
 *   number  entero cuya longitud se pide.
 * Salida:
 *   cantidad de dígitos, al menos 1.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int count_digits(long number) {
    int digits = 1;

    while (number >= 10 || number <= -10) {
        number /= 10;
        digits++;
    }

    return digits;
}

/*
 * semantic_error — reporta un error semántico con posición del AST.
 *
 * Entrada:
 *   node  nodo para el señalador o NULL.
 *   fmt  formato printf.
 *   ...  argumentos del formato.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe diagnóstico mediante diag_verror e incrementa su contador.
 * Restricciones:
 *   el análisis continúa después de reportar.
 */
void semantic_error(ASTNode *node, const char *fmt, ...) {
    int line = 0;
    int column = 0;
    int length = 1;

    if (node != NULL) {

        ASTNode *name =
            node->kind == AST_IDENTIFIER
                ? node
                : ast_find_child(node, AST_IDENTIFIER);

        // Número literal (con o sin signo): el señalador cubre el número.
        ASTNode *number =
            node->kind == AST_NEG ? node->child : node;

        if (number != NULL && number->kind == AST_NUMBER) {
            line = node->line;
            column = node->column;
            length = count_digits(number->number);

            // Con signo: desde el - hasta el último dígito.
            if (number != node)
                length += number->line == node->line
                              ? number->column - node->column
                              : 1;
        }
        // Operador: el nodo está en la posición del operador (parser.y).
        // Va antes de buscar el hijo IDENTIFIER, que sería un operando.
        else if (semantic_operator_symbol(node->kind) != NULL) {
            line = node->line;
            column = node->column;
            length = (int) strlen(semantic_operator_symbol(node->kind));
        }
        else if (node->kind == AST_BOOLEAN) {
            line = node->line;
            column = node->column;
            length = node->number ? 9 : 5;   // verdadero / falso
        }
        else if (name != NULL && name->text != NULL) {
            line = name->line;
            column = name->column;
            length = (int) strlen(name->text);
        }
        else {
            line = node->line;
            column = node->column;
        }
    }

    va_list args;

    va_start(args, fmt);
    diag_verror("error semantico", line, column, length, fmt, args);
    va_end(args);
}

// Memoria

#define TEMP_WORDS 12             // una casilla para cada registro t0–t11

/* ==========================================================================
 * DIRECCIONES Y DIMENSIONES
 * ======================================================================= */


/*
 * reserve — asigna un desplazamiento estático y avanza el área de datos.
 *
 * Entrada:
 *   width  ancho en bytes no negativo.
 *   what  nombre para diagnóstico.
 *   node  nodo que declara el área.
 *   out_address  puntero escribible para dirección.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe *out_address, avanza el área de datos y reporta el primer desborde.
 * Restricciones:
 *   continúa asignando después de 0x0FFF; widths llegan alineados a cuatro
 *   bytes. El contador vive en data_area.c; el mensaje se emite aquí, en el
 *   mismo punto del recorrido de siempre, para conservar su orden entre los
 *   demás errores semánticos.
 * Referencia: libro §6.3.4.
 */
static void reserve(int width, const char *what, ASTNode *node,
                    int *out_address) {
    if (data_area_alloc(width, out_address))
        semantic_error(node,
                       "'%s' no cabe en la memoria de datos (limite 0x%04X)",
                       what,
                       (unsigned int) (DATA_LIMIT - 1));
}

/*
 * type_width — obtiene el ancho de un tipo escalar.
 *
 * Entrada:
 *   type  tipo de símbolo.
 * Salida:
 *   4 para int/bool; 0 para otros tipos.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int type_width(SymbolType type) {
    switch (type) {
        case TYPE_INT:
        case TYPE_BOOL: return 4;
        default:        return 0;
    }
}

/*
 * fill_dimensions — copia los tamaños declarados de un arreglo.
 *
 * Entrada:
 *   symbol  símbolo nuevo no NULL.
 *   dimensions  nodo DIMENSIONS o nodo opcional.
 *   node  nodo declarador para diagnóstico.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: llena dimensions/dim_sizes y reporta tamaños no positivos.
 * Restricciones:
 *   hasta MAX_DIMS=2; no calcula width.
 */
static void fill_dimensions(Symbol *symbol, ASTNode *dimensions,
                            ASTNode *node) {

    // A-2: en esa posición puede llegar otro nodo (no DIMENSIONS).
    if (dimensions == NULL ||
        dimensions->kind != AST_DIMENSIONS)
        return;

    // Los hijos de DIMENSIONS son nodos NUMBER con el tamaño en number.
    ASTNode *dimension = dimensions->child;

    int reported = 0;

    while (dimension != NULL &&
           symbol->dimensions < MAX_DIMS) {

        // S-06: una dimensión de tamaño 0 no reserva nada. Se reporta una vez
        // por declaración y se sigue: el tamaño 0 queda guardado.
        if (dimension->number <= 0 && !reported) {

            semantic_error(node,
                           "el tamano de un arreglo debe ser mayor que cero");

            reported = 1;
        }

        // Guardar el tamaño antes de incrementar dimensions.
        symbol->dim_sizes[symbol->dimensions] = (int) dimension->number;
        symbol->dimensions++;

        dimension = dimension->next;
    }
}

/* ==========================================================================
 * DECLARACIONES Y PARÁMETROS
 * ======================================================================= */


/*
 * find_parameter — busca un parámetro por nombre.
 *
 * Entrada:
 *   function  función con param_list.
 *   name  nombre no NULL.
 * Salida:
 *   parámetro prestado o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static Symbol *find_parameter(Symbol *function, const char *name) {
    for (int i = 0; i < function->parameter_count; i++) {

        if (strcmp(function->param_list[i]->name, name) == 0)
            return function->param_list[i];
    }

    return NULL;
}

/*
 * check_not_parameter — evita que un local o control S oculte un parámetro.
 *
 * Entrada:
 *   name  nombre no NULL.
 *   node  nodo declarador.
 * Salida:
 *   1 si no choca; 0 si reporta colisión.
 *   Efectos: reporta colisión con parámetro de la función en análisis.
 * Restricciones:
 *   no restringe declaraciones globales.
 */
static int check_not_parameter(const char *name, ASTNode *node) {
    if (scopes_function() == NULL ||
        find_parameter(scopes_function(), name) == NULL)
        return 1;

    semantic_error(node,
                   "'%s' ya es un parametro de esta funcion",
                   name);

    return 0;
}

/*
 * analyze_declaration — verifica y reserva una declaración de variable.
 *
 * Entrada:
 *   node  nodo de declaración o NULL.
 * Salida:
 *   1 si continúa, incluso con errores semánticos; 0 ante nodo inválido.
 *   Efectos: crea/anota/inserta símbolo, verifica inicializador y reserva dirección.
 * Restricciones:
 *   inicializador se verifica antes de insertar el nombre; semántico usa
 *   el contador de diagnósticos para el éxito final.
 */
static int analyze_declaration(ASTNode *node) {
    if (node == NULL)
        return 1;

    /*
     * DECLARATION
     *
     * child 0 = modifier
     * child 1 = identifier
     * child 2 = type
     * child 3 = dimensions
     * child 4 = initializer
     */

    ASTNode *modifier = node->child;

    ASTNode *identifier =
        modifier ? modifier->next : NULL;

    ASTNode *type =
        identifier ? identifier->next : NULL;

    ASTNode *dimensions =
        type ? type->next : NULL;

    if (identifier == NULL ||
        identifier->kind != AST_IDENTIFIER) {

        fprintf(stderr,
                "Semantic error: invalid declaration\n");

        return 0;
    }

    // A-2: el inicializador es el hijo que sigue a TYPE o a DIMENSIONS y que
    // no es DIMENSIONS.
    ASTNode *initializer = dimensions;

    if (initializer != NULL &&
        initializer->kind == AST_DIMENSIONS)
        initializer = initializer->next;

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);
    symbol->line = node->line;
    symbol->decl_node = node;

    identifier->symbol = symbol;

    fill_dimensions(symbol, dimensions, node);

    // El inicializador se verifica antes de insertar el símbolo:
    // Vm x : int = x ;  usa el x de afuera, o es error.
    // Una lista se verifica contra la forma del arreglo; cualquier otro
    // valor, como una asignación a la variable declarada (sin mutabilidad:
    // es su valor inicial).
    if (initializer != NULL &&
        initializer->kind == AST_INITIALIZER_LIST)
        semantic_check_initializer_list(symbol, initializer, node);
    else if (initializer != NULL)
        semantic_check_assignable(node,
                         semantic_symbol_value_type(symbol),
                         semantic_check_expression(initializer));

    // Un local que oculta un parámetro no se inserta.
    if (!check_not_parameter(symbol->name, node)) {

        scopes_discard(symbol);

        return 1;
    }

    // Libro §6.3.4: ancho = ancho del tipo × producto de las dimensiones
    int d1 = symbol->dimensions > 0 ? symbol->dim_sizes[0] : 1;
    int d2 = symbol->dimensions > 1 ? symbol->dim_sizes[1] : 1;

    symbol->width = type_width(symbol->type) * d1 * d2;   // d = 1 si no aplica

    // Una redeclaración no se inserta.
    if (!scopes_insert(symbol, node))
        return 1;

    identifier->symbol = symbol;
    reserve(symbol->width, symbol->name, node, &symbol->address);

    return 1;
}


/*
 * create_parameter — crea un parámetro sin insertarlo ni asignar dirección.
 *
 * Entrada:
 *   node  nodo AST_PARAMETER.
 * Salida:
 *   símbolo nuevo; NULL si falta identificador.
 *   Efectos: reserva memoria y verifica dimensiones.
 * Restricciones:
 *   width=4 también para arreglos, cuya casilla contiene una dirección.
 */
static Symbol *create_parameter(ASTNode *node) {
    /*
     * PARAMETER
     *
     * MODIFIER (opcional), TYPE, IDENTIFIER, DIMENSIONS (opcional)
     */
    ASTNode *modifier = ast_find_child(node, AST_MODIFIER);
    ASTNode *type = ast_find_child(node, AST_TYPE);
    ASTNode *identifier = ast_find_child(node, AST_IDENTIFIER);
    ASTNode *dimensions = ast_find_child(node, AST_DIMENSIONS);

    if (identifier == NULL) {

        fprintf(stderr,
                "Semantic error: invalid parameter\n");

        return NULL;
    }

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_PARAMETER,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);
    symbol->line = node->line;
    symbol->decl_node = node;

    fill_dimensions(symbol, dimensions, node);

    // escalar: el valor; arreglo: su dirección (paso por referencia)
    symbol->width = 4;

    return symbol;
}


// Sentencias

static int analyze_statement(ASTNode *statement);

/* ==========================================================================
 * BLOQUES Y CONTROL DE FLUJO
 * ======================================================================= */


/*
 * analyze_statements — recorre sentencias en el ámbito actual.
 *
 * Entrada:
 *   block  nodo BLOCK o NULL.
 * Salida:
 *   1 si continúa; 0 ante error interno de una sentencia.
 *   Efectos: anota símbolos, reserva memoria y reporta errores.
 * Restricciones:
 *   no abre ámbito; informa una vez por bloque sobre código después de
 *   E/R.
 */
static int analyze_statements(ASTNode *block) {
    if (block == NULL || block->kind != AST_BLOCK)
        return 1;

    ASTNode *statement = block->child;
    ASTNode *previous = NULL;

    int unreachable_reported = 0;

    while (statement != NULL) {

        // E y R transfieren el control: lo que sigue en el mismo bloque no
        // se ejecuta. Se reporta una vez por bloque (la primera sentencia) y
        // se sigue analizando.
        if (!unreachable_reported &&
            previous != NULL &&
            (previous->kind == AST_EMIT || previous->kind == AST_REWIND)) {

            semantic_error(statement,
                           "codigo inalcanzable: "
                           "la sentencia anterior transfiere el control");

            unreachable_reported = 1;
        }

        if (!analyze_statement(statement))
            return 0;

        previous = statement;
        statement = statement->next;
    }

    return 1;
}

/*
 * analyze_block — abre un ámbito, analiza el bloque y regresa al padre.
 *
 * Entrada:
 *   block  nodo BLOCK o NULL.
 *   kind  clase del ámbito.
 * Salida:
 *   1 si continúa; 0 ante error interno.
 *   Efectos: crea ámbito, analiza sentencias y restaura el ámbito actual.
 * Restricciones:
 *   ninguna.
 */
static int analyze_block(ASTNode *block, ScopeKind kind) {

    // P-03: la línea del BLOCK es la de la palabra clave que lo abre.
    scopes_enter(kind, block ? block->line : 0);

    int result = analyze_statements(block);

    scopes_exit();

    return result;
}

/*
 * analyze_conditional — analiza C y cada rama A en su ámbito.
 *
 * Entrada:
 *   node  nodo AST_CONDITIONAL.
 * Salida:
 *   1 si continúa; 0 ante error interno.
 *   Efectos: verifica condiciones y abre un ámbito por rama.
 * Restricciones:
 *   cada condición se verifica antes de abrir su ámbito.
 */
static int analyze_conditional(ASTNode *node) {

    // Rama inicial  C [cond] ;
    semantic_check_condition(node);

    if (!analyze_block(ast_find_child(node, AST_BLOCK), SCOPE_C))
        return 0;

    // Ramas alternativas  A [cond] ;
    ASTNode *alternatives = ast_find_child(node, AST_ALTERNATIVES);
    ASTNode *alternative = alternatives ? alternatives->child : NULL;

    while (alternative != NULL) {

        if (alternative->kind == AST_ALTERNATIVE) {

            semantic_check_condition(alternative);

            if (!analyze_block(ast_find_child(alternative, AST_BLOCK),
                               SCOPE_A))
                return 0;
        }

        alternative = alternative->next;
    }

    // Rama residual  A ;
    ASTNode *residual = ast_find_child(node, AST_RESIDUAL);

    if (residual != NULL &&
        !analyze_block(ast_find_child(residual, AST_BLOCK),
                       SCOPE_RESIDUAL))
        return 0;

    return 1;
}

/*
 * analyze_loop_l — analiza la condición y el ámbito de L.
 *
 * Entrada:
 *   node  nodo AST_LOOP_L.
 * Salida:
 *   1 si continúa; 0 ante error interno.
 *   Efectos: verifica condición bool y analiza cuerpo en SCOPE_L.
 * Restricciones:
 *   ninguna.
 */
static int analyze_loop_l(ASTNode *node) {
    semantic_check_condition(node);

    return analyze_block(ast_find_child(node, AST_BLOCK), SCOPE_L);
}

/*
 * check_loop_s_bounds — verifica inicio, fin y paso del ciclo S.
 *
 * Entrada:
 *   node  nodo AST_LOOP_S.
 *   start  expresión de inicio.
 *   end  expresión de fin.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: evalúa tipos, exige int, rechaza paso cero y avisa si el ciclo
 *   nunca se ejecuta.
 * Restricciones:
 *   inicio y fin se verifican antes de abrir el ámbito del S:
 *   S [ i , 0 , i , 1 ]  usa el i de afuera.
 */
static void check_loop_s_bounds(ASTNode *node, ASTNode *start, ASTNode *end) {
    ExprType start_type = semantic_check_expression(start);
    ExprType end_type = semantic_check_expression(end);

    semantic_expect_type(start, start_type, ET_INT,
                "el limite de 'S' debe ser de tipo int");
    semantic_expect_type(end, end_type, ET_INT,
                "el limite de 'S' debe ser de tipo int");

    // El paso es un NUMBER con su signo ya aplicado.
    ASTNode *step = end ? end->next : NULL;

    if (step != NULL && step->kind == AST_NUMBER) {

        if (step->number == 0) {

            semantic_error(step, "el paso de 'S' no puede ser cero");
        }
        else if (start_type.is_const && end_type.is_const) {

            // Con inicio y fin constantes se conoce el sentido. El fin es
            // exclusivo: el ciclo solo se ejecuta si el paso se acerca a él.
            int runs =
                (step->number > 0 && start_type.value < end_type.value) ||
                (step->number < 0 && start_type.value > end_type.value);

            if (!runs)
                semantic_error(node,
                               "el ciclo 'S' nunca se ejecuta: "
                               "inicio %ld, fin %ld, paso %ld",
                               start_type.value,
                               end_type.value,
                               step->number);
        }
    }
}

/*
 * declare_loop_control — crea la variable de control del S y su casilla de límite.
 *
 * Entrada:
 *   node  nodo AST_LOOP_S.
 *   identifier  nodo del identificador de control.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: crea el control Vi, lo inserta, reserva su casilla y la del
 *   límite oculto, y anota el identificador.
 * Restricciones:
 *   ámbito del S abierto. El orden de las dos reservas (control y luego
 *   límite) fija las direcciones.
 */
static void declare_loop_control(ASTNode *node, ASTNode *identifier) {
    Symbol *control =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      TYPE_INT);

    // el ciclo la controla; el cuerpo no puede modificarla
    control->modifier = MODIFIER_VI;
    control->width = type_width(control->type);   // M-1
    control->line = node->line;
    control->decl_node = node;

    if (scopes_insert(control, node)) {
        reserve(control->width,
                control->name,
                node,
                &control->address);

        // Casilla oculta para el fin del ciclo, junto a la variable de
        // control. El fin se evalúa una sola vez al entrar
        // (calser_diseno.md §5.8.2) y el generador lo guarda aquí para
        // no retener un registro durante el cuerpo. No tiene nombre: no
        // entra a la tabla de ámbitos.
        reserve(type_width(TYPE_INT),
                control->name,
                node,
                &control->limit_address);

        identifier->symbol = control;
    }
}

/*
 * analyze_loop_s — analiza límites, control y ámbito del ciclo S.
 *
 * Entrada:
 *   node  nodo AST_LOOP_S.
 * Salida:
 *   1 si continúa; 0 si falta identificador válido.
 *   Efectos: crea control Vi, reserva su casilla y límite oculto; analiza cuerpo y reporta errores.
 * Restricciones:
 *   inicio/fin se verifican en el ámbito exterior; el cuerpo comparte el
 *   ámbito del control.
 */
static int analyze_loop_s(ASTNode *node) {
    /*
     * LOOP_S (posición fija: los 5 hijos siempre existen)
     *
     * child 0 = identifier (variable de control)
     * child 1 = inicio
     * child 2 = fin
     * child 3 = paso (NUMBER)
     * child 4 = body
     */
    ASTNode *identifier = node->child;
    ASTNode *start =
        identifier ? identifier->next : NULL;
    ASTNode *end =
        start ? start->next : NULL;

    if (identifier == NULL ||
        identifier->kind != AST_IDENTIFIER) {
        fprintf(stderr,
                "Semantic error: invalid S loop\n");
        return 0;
    }

    // Inicio y fin se verifican antes de abrir el ámbito del S.
    check_loop_s_bounds(node, start, end);

    // Si la variable de control oculta un parámetro no se crea: dentro del
    // cuerpo, el nombre se refiere al parámetro.
    int declare = check_not_parameter(identifier->text, node);

    scopes_enter(SCOPE_S, node->line);

    if (declare)
        declare_loop_control(node, identifier);

    // El cuerpo comparte el ámbito del S con su variable (no abre otro).
    int result = analyze_statements(ast_find_child(node, AST_BLOCK));

    scopes_exit();

    return result;
}

/*
 * analyze_statement — despacha una sentencia para análisis semántico.
 *
 * Entrada:
 *   statement  sentencia o NULL.
 * Salida:
 *   1 si continúa; 0 ante error interno de un analizador.
 *   Efectos: anota símbolos, modifica ámbitos/direcciones y reporta errores.
 * Restricciones:
 *   EMIT y clases sin caso específico no requieren comprobación aquí.
 */
static int analyze_statement(ASTNode *statement) {
    if (statement == NULL)
        return 1;

    switch (statement->kind) {

        case AST_DECLARATION:
            return analyze_declaration(statement);

        case AST_CONDITIONAL:
            return analyze_conditional(statement);

        case AST_LOOP_L:
            return analyze_loop_l(statement);

        case AST_LOOP_S:
            return analyze_loop_s(statement);

        // Destino (primer hijo: IDENTIFIER, CALL o INDEX) y valor (segundo).
        case AST_ASSIGNMENT:
            semantic_check_assignment(statement);
            return 1;

        // El CALL de la sentencia debe ser una llamada.
        case AST_CALL_STATEMENT:
            semantic_check_call(ast_find_child(statement, AST_CALL), USE_STATEMENT);
            return 1;

        // El nombre debe ser una función; se verifica cada argumento.
        case AST_REWIND:
            semantic_check_call(statement, USE_REWIND);
            return 1;

        // Operación de CRYPTO: se verifican sus argumentos.
        case AST_CRYPTO:
            semantic_analyze_crypto(statement);
            return 1;

        // EMIT: nada que verificar.
        default:
            return 1;
    }
}


/* ==========================================================================
 * FIRMAS Y CUERPOS DE FUNCIONES
 * ======================================================================= */


/*
 * register_parameters — crea los parámetros de una firma y los agrega a la función.
 *
 * Entrada:
 *   function  símbolo de la función ya insertado en el global.
 *   params  nodo AST_PARAMS o NULL.
 * Salida:
 *   1 si continúa; 0 ante un parámetro inválido.
 *   Efectos: llena param_list; reporta parámetros repetidos y los descarta.
 * Restricciones:
 *   los parámetros solo quedan en param_list, que no es dueña: pasan a
 *   pertenecer al ámbito de la función cuando la pasada 2 los inserta.
 *   La pasada 2 llega a todas las funciones registradas, salvo que un
 *   error interno (nodo inválido) detenga el análisis: solo en ese camino
 *   quedarían sin liberar.
 */
static int register_parameters(Symbol *function, ASTNode *params) {
    ASTNode *parameter = params ? params->child : NULL;

    while (parameter != NULL) {

        Symbol *symbol = create_parameter(parameter);

        if (symbol == NULL)
            return 0;

        // Todos los parámetros comparten el ámbito de la función. Un
        // parámetro duplicado no se agrega a param_list.
        Symbol *first = find_parameter(function, symbol->name);

        if (first != NULL) {

            semantic_error(parameter,
                           "'%s' ya fue declarado en este ambito (linea %d)",
                           symbol->name,
                           first->line);

            scopes_discard(symbol);
        }
        else {

            // Lista ordenada de parámetros de la función (también lo cuenta).
            symbol_add_param(function, symbol);
        }

        parameter = parameter->next;
    }

    return 1;
}

/*
 * register_signature — registra una firma antes de analizar cuerpos.
 *
 * Entrada:
 *   node  nodo AST_FUNCTION.
 * Salida:
 *   1 si continúa, incluso con redeclaración; 0 ante nodo inválido.
 *   Efectos: crea función y parámetros; anota identificador y reporta colisiones.
 * Restricciones:
 *   primera pasada; parámetros pasan al ámbito dueño en la segunda.
 */
static int register_signature(ASTNode *node) {
    /*
     * FUNCTION
     *
     * IDENTIFIER, PARAMS, RETURN_TYPE, BLOCK
     */
    ASTNode *identifier = ast_find_child(node, AST_IDENTIFIER);
    ASTNode *params = ast_find_child(node, AST_PARAMS);
    ASTNode *return_type = ast_find_child(node, AST_RETURN_TYPE);

    if (identifier == NULL) {
        fprintf(stderr,
                "Semantic error: invalid function\n");
        return 0;
    }

    // Función repetida: no se inserta ni se registran sus parámetros. Como
    // ningún símbolo apunta a este nodo, la pasada 2 no lo analiza.
    Symbol *existing =
        symbol_lookup_current(scopes_global(), identifier->text);

    if (existing != NULL) {

        // Se marca el segundo FUNCTION, señalando la línea del primero.
        semantic_error(node,
                       "la funcion '%s' ya fue declarada (linea %d)",
                       identifier->text,
                       existing->line);

        return 1;
    }

    // RETURN_TYPE sin hijos es void.
    ASTNode *ret_type = ast_find_child(return_type, AST_TYPE);
    ASTNode *ret_id = ast_find_child(return_type, AST_IDENTIFIER);

    // La función pertenece al ámbito global.
    Symbol *function =
        symbol_create(identifier->text,
                      SYMBOL_FUNCTION,
                      ret_type ? get_type(ret_type) : TYPE_VOID);

    function->line = node->line;
    function->decl_node = node;

    symbol_insert(scopes_global(), function);
    identifier->symbol = function;

    if (!register_parameters(function, params))
        return 0;

    // La variable de retorno comparte ese ámbito. En el texto el parámetro
    // aparece primero: el error se marca en la variable de retorno y señala
    // al parámetro. El parámetro se conserva y la pasada 2 no crea la
    // variable de retorno.
    Symbol *clash =
        ret_id ? find_parameter(function, ret_id->text) : NULL;

    if (clash != NULL) {

        semantic_error(return_type,
                       "'%s' ya fue declarado en este ambito (linea %d)",
                       ret_id->text,
                       clash->line);
    }

    return 1;
}

/*
 * register_signatures — registra todas las firmas en orden del archivo.
 *
 * Entrada:
 *   functions  nodo FUNCTIONS o NULL.
 * Salida:
 *   1 si continúa; 0 ante error interno.
 *   Efectos: llena símbolos globales y listas de parámetros.
 * Restricciones:
 *   debe ejecutarse antes de analizar cuerpos.
 */
static int register_signatures(ASTNode *functions) {
    if (functions == NULL || functions->kind != AST_FUNCTIONS)
        return 1;

    ASTNode *function = functions->child;

    while (function != NULL) {

        if (!register_signature(function))
            return 0;

        function = function->next;
    }

    return 1;
}

/*
 * check_main — verifica la firma del punto de entrada.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta si falta main o si recibe parámetros/devuelve valor.
 * Restricciones:
 *   firmas ya registradas en el ámbito global.
 */
static void check_main(void) {
    Symbol *main_function = symbol_lookup_current(scopes_global(), "main");

    if (main_function == NULL ||
        main_function->kind != SYMBOL_FUNCTION) {

        semantic_error(NULL,
                       "el archivo principal debe declarar 'main'");

        return;
    }

    if (main_function->parameter_count > 0 ||
        main_function->type != TYPE_VOID) {

        semantic_error(main_function->decl_node,
                       "'main' debe declararse como F main [] : void");
    }
}

/*
 * begin_frame — abre el área estática de una función.
 *
 * Entrada:
 *   function  símbolo de la función.
 *   node  nodo AST_FUNCTION, para diagnóstico.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: fija area_base y, salvo en main, reserva la casilla +0 de rl.
 * Restricciones:
 *   se llama antes de reservar variable de retorno y parámetros.
 *
 * Área estática de la función (libro §7.1.1):
 *
 * +0  casilla reservada (todas excepto main); rl se salva en la pila
 * +4  variable de retorno, si no es void
 * ... parámetros, en orden
 * ... locales de todos sus bloques, en el orden del recorrido
 * ... 12 palabras para salvar temporales vivos (también en main)
 */
static void begin_frame(Symbol *function, ASTNode *node) {
    function->area_base = data_area_end();

    // M-5: main no reserva la casilla +0 para rl
    if (strcmp(function->name, "main") != 0) {

        int rl_address;   // no hay símbolo para rl

        reserve(4, function->name, node, &rl_address);
    }
}

/*
 * declare_return_variable — crea la variable de retorno en el ámbito de la función.
 *
 * Entrada:
 *   function  símbolo de la función.
 *   return_type  nodo AST_RETURN_TYPE o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: crea, inserta y reserva la variable; anota el identificador.
 * Restricciones:
 *   ámbito de la función abierto; va antes que los parámetros (queda en +4).
 *   Si se llama como un parámetro (ya reportado en la pasada 1) no se
 *   crea: el nombre se refiere al parámetro.
 */
static void declare_return_variable(Symbol *function, ASTNode *return_type) {
    ASTNode *ret_type = ast_find_child(return_type, AST_TYPE);
    ASTNode *ret_id = ast_find_child(return_type, AST_IDENTIFIER);

    if (ret_id != NULL &&
        find_parameter(function, ret_id->text) == NULL) {

        Symbol *ret = symbol_create(ret_id->text,
                                    SYMBOL_RETURN,
                                    get_type(ret_type));

        ret->modifier = MODIFIER_VM;   // el cuerpo la asigna
        ret->width = type_width(ret->type);   // M-1
        ret->line = return_type->line;
        ret->decl_node = return_type;

        if (scopes_insert(ret, return_type)) {
            ret_id->symbol = ret;
            reserve(ret->width, ret->name, return_type, &ret->address);
        }
    }
}

/*
 * declare_parameters — pasa los parámetros al ámbito de la función y los reserva.
 *
 * Entrada:
 *   function  símbolo de la función con su param_list.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: inserta cada parámetro en el ámbito actual y reserva su dirección.
 * Restricciones:
 *   los parámetros ya existen en param_list: desde aquí pertenecen al
 *   ámbito de la función. La inserción no puede fallar (los duplicados se
 *   descartaron en la pasada 1).
 */
static void declare_parameters(Symbol *function) {
    for (int i = 0; i < function->parameter_count; i++) {

        Symbol *parameter = function->param_list[i];

        if (!symbol_insert(scopes_current(), parameter))
            continue;

        reserve(parameter->width,
                parameter->name,
                parameter->decl_node,
                &parameter->address);
    }
}

/*
 * check_has_emit — exige al menos una sentencia E en el cuerpo.
 *
 * Entrada:
 *   function  símbolo de la función.
 *   node  nodo AST_FUNCTION, para diagnóstico.
 *   body  nodo AST_BLOCK del cuerpo.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta un error si no hay E.
 * Restricciones:
 *   cuenta los bloques anidados (también main). Con R y sin E la función
 *   nunca terminaría.
 */
static void check_has_emit(Symbol *function, ASTNode *node, ASTNode *body) {
    if (!ast_contains_kind(body, AST_EMIT)) {

        if (ast_contains_kind(body, AST_REWIND))
            semantic_error(node,
                           "la funcion '%s' usa 'R' pero no contiene ningun 'E'; "
                           "la recursion nunca terminaria",
                           function->name);
        else
            semantic_error(node,
                           "la funcion '%s' no contiene ninguna sentencia 'E'",
                           function->name);
    }
}

/*
 * end_frame — cierra el área estática de una función.
 *
 * Entrada:
 *   function  símbolo de la función.
 *   node  nodo AST_FUNCTION, para diagnóstico.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reserva las casillas de t0–t11 y fija area_size.
 * Restricciones:
 *   se llama después de reservar todas las locales del cuerpo.
 */
static void end_frame(Symbol *function, ASTNode *node) {
    // Reserva al final del área casillas de t0–t11 para expansiones CRYPTO;
    // las llamadas normales guardan sus temporales vivos en la pila de rx.
    reserve(TEMP_WORDS * type_width(TYPE_INT), function->name, node,
            &function->temp_base);

    // El área termina donde quedó el offset, incluidos los temporales.
    function->area_size = data_area_end() - function->area_base;
}

/*
 * analyze_function — asigna ámbito, direcciones y cuerpo de una función.
 *
 * Entrada:
 *   node  nodo AST_FUNCTION.
 * Salida:
 *   1 si continúa o ignora duplicado; 0 ante error interno.
 *   Efectos: inserta parámetros/retorno, reserva área y temporales, verifica cuerpo y E.
 * Restricciones:
 *   firma registrada en pasada 1; +0 reservado fuera de main, rl se salva
 *   en pila al generar. El orden de las reservas fija las direcciones.
 */
static int analyze_function(ASTNode *node) {
    /*
     * FUNCTION
     *
     * IDENTIFIER, PARAMS, RETURN_TYPE, BLOCK
     */
    ASTNode *identifier = ast_find_child(node, AST_IDENTIFIER);
    ASTNode *return_type = ast_find_child(node, AST_RETURN_TYPE);
    ASTNode *body = ast_find_child(node, AST_BLOCK);

    if (identifier == NULL) {
        fprintf(stderr,
                "Semantic error: invalid function\n");
        return 0;
    }

    // La pasada 1 ya creó el símbolo de la función.
    Symbol *function =
        symbol_lookup_current(scopes_global(), identifier->text);

    // Función repetida (ya reportada en la pasada 1): el símbolo con ese
    // nombre no apunta a este nodo, así que no se analiza.
    if (function == NULL ||
        function->decl_node != node)
        return 1;

    scopes_set_function(function);

    // Orden del archivo, para buscar ciclos de llamadas.
    calls_add_function(function);

    begin_frame(function, node);

    // Crea el ámbito de función.
    scopes_enter(SCOPE_FUNCTION, function->line);

    declare_return_variable(function, return_type);

    declare_parameters(function);

    // S-01: el cuerpo se recorre completo, en el mismo ámbito de la función.
    if (!analyze_statements(body))
        return 0;

    // Toda función debe tener al menos un E en su cuerpo.
    check_has_emit(function, node, body);

    end_frame(function, node);

    // Regresa al ámbito global.
    scopes_exit();

    scopes_set_function(NULL);

    return 1;
}


/* ==========================================================================
 * PROGRAMA Y RESERVA DE PILA
 * ======================================================================= */


/* semantic_analyze — véase el contrato en semantic.h. */
int semantic_analyze(ASTNode *root) {
    if (root == NULL)
        return 0;

    if (root->kind != AST_PROGRAM) {
        fprintf(stderr,
                "Semantic error: root is not PROGRAM\n");
        return 0;
    }

    // Los errores semánticos se acumulan: el resultado sale del contador.
    int errors_before = diag_count();

    scopes_open_global();

    data_area_reset();
    scopes_set_function(NULL);

    calls_reset();

    /*
     * PROGRAM
     *
     * GLOBALS, FUNCTIONS
     */
    ASTNode *globals = ast_find_child(root, AST_GLOBALS);
    ASTNode *functions = ast_find_child(root, AST_FUNCTIONS);

    // Declaraciones globales.
    ASTNode *decl = globals ? globals->child : NULL;

    while (decl != NULL) {

        if (!analyze_declaration(decl))
            return 0;

        decl = decl->next;
    }

    // Pasada 1: todas las firmas antes que cualquier cuerpo.
    if (!register_signatures(functions))
        return 0;

    check_main();

    // Pasada 2: ámbitos, direcciones y cuerpos.
    ASTNode *function = functions ? functions->child : NULL;

    while (function != NULL) {

        if (!analyze_function(function))
            return 0;

        function = function->next;
    }

    // Recursión indirecta: después de analizar todos los cuerpos.
    calls_check_cycles();

    // La pila de llamadas ocupa lo que queda libre en la zona de ccom. Si
    // las variables ya se pasaron del límite, ese error ya se reportó.
    if (data_area_stack_short()) {

        semantic_error(NULL,
                       "no queda espacio para la pila de llamadas: "
                       "%d bytes libres, se necesitan %d (limite 0x%04X)",
                       data_area_free(),
                       STACK_MIN_BYTES,
                       (unsigned int) (DATA_LIMIT - 1));
    }

    return diag_count() == errors_before;
}

/* ==========================================================================
 * CONSULTAS Y LIBERACIÓN
 * ======================================================================= */



/* semantic_print_symbols — véase el contrato en semantic.h. */
void semantic_print_symbols(void) {
    symbol_table_print(scopes_first());
}

/* semantic_fprint_symbols — véase el contrato en semantic.h. */
void semantic_fprint_symbols(FILE *out, const char *prefix) {
    symbol_table_fprint(out, scopes_first(), prefix, 1);
}

/* semantic_free — véase el contrato en semantic.h. */
void semantic_free(void)
{
    scopes_release();

    calls_reset();
}
