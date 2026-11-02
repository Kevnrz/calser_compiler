/*
 * generate_expr.c — Fase 4: expresiones, llamadas a funciones y operaciones de cifrado.
 *
 * Evalúa expresiones en temporales, expande la aritmética por software y los
 * accesos a arreglos, genera las llamadas con pila y delega CRYPTO en
 * codegen_crypto.
 *
 * Usa: generate_internal.h, codegen.h, codegen_arith.h, codegen_array.h, codegen_crypto.h.
 * Lo usa: generate.c (gen_expr_setup), generate_stmt.c y generate_init.c.
 * Referencias: libro §6.4, §6.6, §7.2; isa.md §6, §9.
 */

#include <stdio.h>
#include <string.h>

#include "generate_internal.h"
#include "codegen.h"
#include "codegen_array.h"


/* ==========================================================================
 * EXPRESIONES Y EXPANSIONES
 * ======================================================================= */


/*
 * emit_arith_callback — adapta operaciones aritméticas y de arreglos al emisor.
 *
 * Entrada:
 *   user  dato del callback, no utilizado.
 *   op  operación o NULL.
 * Salida:
 *   1 si emite; 0 si el formato o slot no es aceptado.
 *   Efectos: agrega etiquetas u operaciones a codegen.
 * Restricciones:
 *   slots -1, 0, 2 y 3; la etiqueta debe terminar en dos puntos y caber en
 *   128 bytes.
 */
static int emit_arith_callback(void *user, const CGArithOp *op) {
    (void)user;

    if (op == NULL)
        return 0;

    // Las etiquetas se representan con slot == -1.
    if (op->slot == -1) {
        size_t len = strlen(op->text);

        if (len == 0 || op->text[len - 1] != ':')
            return 0;

        char label_name[128];

        if (len >= sizeof(label_name))
            return 0;

        memcpy(label_name, op->text, len - 1);
        label_name[len - 1] = '\0';

        cg_label(label_name);
        return 1;
    }

    // Slot 0 del emisor aritmético = ALU0.
    if (op->slot == 0) {
        cg_op(SLOT_ALU0, "%s", op->text);
        return 1;
    }

    // Slot 2 del emisor aritmético = unidad de saltos BRU.
    if (op->slot == 2) {
        cg_op(SLOT_BRU, "%s", op->text);
        return 1;
    }

    // Slot 3 del emisor de arreglos = LSU.
    if (op->slot == 3) {
        cg_op(SLOT_LSU, "%s", op->text);
        return 1;
    }

    return 0;
}

/*
 * emit_software_arithmetic — expande multiplicación, división, módulo o potencia.
 *
 * Entrada:
 *   node  nodo para diagnósticos.
 *   kind  operación aritmética.
 *   lhs  temporal reservado del operando izquierdo.
 *   rhs  temporal reservado del operando derecho.
 * Salida:
 *   temporal reservado del resultado; -1 si falla.
 *   Efectos: emite rutina, reserva auxiliares y libera lhs/rhs; reporta errores.
 * Restricciones:
 *   necesita un destino y ocho temporales libres; /, % y ^ usan
 *   arith_error_label.
 */
static int emit_software_arithmetic(GenCtx *gen, ASTNode *node, CGArithKind kind, int lhs, int rhs) {
    int dst = gen_alloc_reg(gen);

    if (dst < 0) {
        gen_report_error(gen, 
            node,
            "sin registros para el resultado aritmetico"
        );
        gen_free_reg(gen, lhs);
        gen_free_reg(gen, rhs);
        return -1;
    }

    int work[8];
    int allocated = 0;

    for (int i = 0; i < 8; i++) {
        work[i] = gen_alloc_reg(gen);

        if (work[i] < 0) {
            gen_report_error(gen, 
                node,
                "sin suficientes registros para la rutina aritmetica"
            );

            for (int j = 0; j < allocated; j++)
                gen_free_reg(gen, work[j]);

            gen_free_reg(gen, dst);
            gen_free_reg(gen, lhs);
            gen_free_reg(gen, rhs);

            return -1;
        }

        allocated++;
    }

    const char *error_label = NULL;

    if (kind == CG_ARITH_DIV ||
        kind == CG_ARITH_MOD ||
        kind == CG_ARITH_POWER) {
        error_label = gen->arith_error_label;
    }

    int success = cg_arith_emit(
        &gen->arithmetic,
        kind,
        lhs,
        rhs,
        dst,
        work,
        error_label
    );

    for (int i = 0; i < 8; i++)
        gen_free_reg(gen, work[i]);

    gen_free_reg(gen, lhs);
    gen_free_reg(gen, rhs);

    if (!success) {
        gen_free_reg(gen, dst);

        gen_report_error(gen, 
            node,
            "fallo al generar la rutina aritmetica"
        );

        return -1;
    }

    // dst conserva su reserva porque el llamador necesita
    // su valor como resultado de la expresión.
    return dst;
}

/*
 * emit_short_circuit — evalúa & o | con corto circuito.
 *
 * Entrada:
 *   node  nodo AST_AND o AST_OR.
 *   left  expresión izquierda.
 *   right  expresión derecha.
 * Salida:
 *   temporal reservado del resultado; -1 si falla.
 *   Efectos: emite salto y etiqueta; libera el temporal derecho.
 * Restricciones:
 *   operandos bool normalizados a 0 o 1; etiqueta LABEL_SC_END ya
 *   registrada.
 * Referencia: libro §6.6.2; isa.md §9.3.
 */
static int emit_short_circuit(GenCtx *gen, ASTNode *node, ASTNode *left, ASTNode *right) {
    int result = gen_expression(gen, left);

    if (result < 0)
        return -1;

    char end_label[64];

    if (!gen_node_label(gen, end_label, sizeof(end_label), node, LABEL_SC_END,
                    "corto circuito")) {
        gen_free_reg(gen, result);
        return -1;
    }

    cg_op(SLOT_BRU,
          "%s %s, rz, %s",
          node->kind == AST_AND ? "beq" : "bne",
          gen_reg_name(result),
          end_label);

    int other = gen_expression(gen, right);

    if (other < 0) {
        gen_free_reg(gen, result);
        return -1;
    }

    cg_op(SLOT_ALU0,
          "saddi %s, %s, 0",
          gen_reg_name(result),
          gen_reg_name(other));

    gen_free_reg(gen, other);

    cg_label(end_label);

    return result;
}

/*
 * emit_binary_expression — evalúa un operador binario de la ALU o lógico.
 *
 * Entrada:
 *   node  nodo binario con dos operandos.
 * Salida:
 *   temporal reservado del resultado; -1 si falla.
 *   Efectos: emite instrucciones, administra temporales y reporta errores.
 * Restricciones:
 *   tipos ya verificados; *, /, % y ^ se despachan en gen_expression.
 */
static int emit_binary_expression(GenCtx *gen, ASTNode *node) {
    ASTNode *left = gen_child_at(node, 0);
    ASTNode *right = gen_child_at(node, 1);

    if (left == NULL || right == NULL) {
        gen_report_error(gen, node,
                     "expresion binaria incompleta");
        return -1;
    }

    if (node->kind == AST_AND || node->kind == AST_OR)
        return emit_short_circuit(gen, node, left, right);

    int rd_left = gen_expression(gen, left);

    if (rd_left < 0)
        return -1;

    int rd_right = gen_expression(gen, right);

    if (rd_right < 0) {
        gen_free_reg(gen, rd_left);
        return -1;
    }

    const char *op = NULL;

    switch (node->kind) {
        case AST_ADD:
            op = "add";
            break;
        case AST_SUB:
            op = "sub";
            break;
        case AST_EQUAL:
            op = "seq";
            break;
        case AST_NOT_EQUAL:
            op = "sne";
            break;
        case AST_LESS:
            op = "slt";
            break;
        case AST_LESS_EQUAL:
            op = "sle";
            break;
        case AST_GREATER:
            // a > b equivale a b < a.
            op = "slt";
            {
                int tmp = rd_left;
                rd_left = rd_right;
                rd_right = tmp;
            }
            break;

        case AST_GREATER_EQUAL:
            // a >= b equivale a negar a < b.
            cg_op(SLOT_ALU0,
                  "slt %s, %s, %s",
                  gen_reg_name(rd_left),
                  gen_reg_name(rd_left),
                  gen_reg_name(rd_right));

            cg_op(SLOT_ALU0,
                  "seq %s, %s, rz",
                  gen_reg_name(rd_left),
                  gen_reg_name(rd_left));

            gen_free_reg(gen, rd_right);
            return rd_left;

        default:
            gen_report_error(gen, node,
                         "operador binario no implementado");
            gen_free_reg(gen, rd_left);
            gen_free_reg(gen, rd_right);
            return -1;
    }
    // Reutiliza el registro izquierdo como resultado.
    cg_op(SLOT_ALU0,
          "%s %s, %s, %s",
          op,
          gen_reg_name(rd_left),
          gen_reg_name(rd_left),
          gen_reg_name(rd_right));

    gen_free_reg(gen, rd_right);

    return rd_left;
}

/* gen_array_indices — véase el contrato en generate_internal.h. */
int gen_array_indices(GenCtx *gen, ASTNode *node, Symbol *array,
                      ASTNode *first, int indices[MAX_DIMS], int *count) {
    int n = 0;

    for (int i = 0; i < MAX_DIMS; i++)
        indices[i] = -1;

    for (ASTNode *idx = first; idx != NULL; idx = idx->next) {

        if (n >= array->dimensions) {
            gen_report_error(gen, node, "demasiados indices para el arreglo");

            for (int i = 0; i < n; i++)
                gen_free_reg(gen, indices[i]);

            return -1;
        }

        indices[n] = gen_expression(gen, idx);

        if (indices[n] < 0) {
            for (int i = 0; i < n; i++)
                gen_free_reg(gen, indices[i]);

            return -1;
        }

        n++;
    }

    if (n != array->dimensions) {
        gen_report_error(gen, node, "cantidad incorrecta de indices");

        for (int i = 0; i < n; i++)
            gen_free_reg(gen, indices[i]);

        return -1;
    }

    *count = n;

    return 0;
}

/*
 * emit_unary — evalúa un operador unario: menos aritmético o negación lógica.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_NEG o AST_NOT con un operando.
 * Salida:
 *   temporal reservado del resultado; -1 si falla.
 *   Efectos: emite sub o seq sobre el mismo registro del operando.
 * Restricciones:
 *   tipos ya verificados por el semántico.
 */
static int emit_unary(GenCtx *gen, ASTNode *node) {
    ASTNode *operand = gen_child_at(node, 0);

    if (operand == NULL) {
        gen_report_error(gen, node,
                         "operador unario sin operando");
        return -1;
    }

    int rd = gen_expression(gen, operand);

    if (rd < 0)
        return -1;

    if (node->kind == AST_NEG) {
        cg_op(SLOT_ALU0,
              "sub %s, rz, %s",
              gen_reg_name(rd),
              gen_reg_name(rd));
    } else {
        cg_op(SLOT_ALU0,
              "seq %s, %s, rz",
              gen_reg_name(rd),
              gen_reg_name(rd));
    }

    return rd;
}

/*
 * emit_arithmetic — evalúa *, /, % o ^ con la rutina de software.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_MUL, AST_DIV, AST_MOD o AST_POWER.
 * Salida:
 *   temporal reservado del resultado; -1 si falla.
 *   Efectos: evalúa ambos operandos y expande la rutina con
 *   emit_software_arithmetic, que libera los operandos.
 * Restricciones:
 *   el hardware no tiene multiplicación ni división: la expansión necesita
 *   un destino y ocho temporales libres.
 */
static int emit_arithmetic(GenCtx *gen, ASTNode *node) {
    ASTNode *left = gen_child_at(node, 0);
    ASTNode *right = gen_child_at(node, 1);

    if (left == NULL || right == NULL) {
        gen_report_error(
            gen,
            node,
            "expresion aritmetica incompleta"
        );
        return -1;
    }

    int lhs = gen_expression(gen, left);

    if (lhs < 0)
        return -1;

    int rhs = gen_expression(gen, right);

    if (rhs < 0) {
        gen_free_reg(gen, lhs);
        return -1;
    }

    CGArithKind kind;

    switch (node->kind) {
        case AST_MUL:
            kind = CG_ARITH_MUL;
            break;

        case AST_DIV:
            kind = CG_ARITH_DIV;
            break;

        case AST_MOD:
            kind = CG_ARITH_MOD;
            break;

        case AST_POWER:
            kind = CG_ARITH_POWER;
            break;

        default:
            gen_free_reg(gen, lhs);
            gen_free_reg(gen, rhs);
            return -1;
    }

    return emit_software_arithmetic(
        gen,
        node,
        kind,
        lhs,
        rhs
    );
}

/*
 * emit_array_read — lee un elemento de un arreglo.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_INDEX, o AST_CALL cuyo nombre es un arreglo.
 * Salida:
 *   temporal reservado con el valor leído; -1 si falla.
 *   Efectos: evalúa los índices, calcula la dirección del elemento y carga
 *   su valor; libera índices y auxiliares y deja reservado solo el valor.
 * Restricciones:
 *   los hijos son el identificador del arreglo y AST_ARGUMENTS con los
 *   índices; el semántico ya verificó la cantidad y los tipos.
 */
static int emit_array_read(GenCtx *gen, ASTNode *node) {
    ASTNode *array_id = gen_child_at(node, 0);
    ASTNode *arguments = gen_child_at(node, 1);

    if (array_id == NULL || array_id->kind != AST_IDENTIFIER) {
        gen_report_error(gen, node, "identificador de arreglo invalido");
        return -1;
    }

    Symbol *array = gen_get_symbol(array_id);

    if (array == NULL) {
        gen_report_error(gen, array_id, "arreglo sin simbolo semantico");
        return -1;
    }

    if (array->dimensions < 1 || array->dimensions > MAX_DIMS) {
        gen_report_error(gen, node, "numero de dimensiones invalido");
        return -1;
    }

    // Evalúa los índices.
    // Arreglo 1D: indices[0] = índice.
    // Arreglo 2D: indices[0] = fila, indices[1] = columna.
    int indices[MAX_DIMS];
    int count = 0;

    if (gen_array_indices(gen, node, array, arguments->child,
                          indices, &count) != 0)
        return -1;

    // Reserva dirección destino y dos registros auxiliares.
    int address = gen_alloc_reg(gen);
    int work[2] = {-1, -1};
    int value = -1;

    if (address >= 0)
        work[0] = gen_alloc_reg(gen);

    if (work[0] >= 0)
        work[1] = gen_alloc_reg(gen);

    if (work[1] >= 0)
        value = gen_alloc_reg(gen);

    if (address < 0 || work[0] < 0 ||
        work[1] < 0 || value < 0) {

        gen_report_error(gen, node, "sin registros para acceder al arreglo");

        if (address >= 0) gen_free_reg(gen, address);
        if (work[0] >= 0) gen_free_reg(gen, work[0]);
        if (work[1] >= 0) gen_free_reg(gen, work[1]);
        if (value >= 0) gen_free_reg(gen, value);

        for (int i = 0; i < count; i++)
            gen_free_reg(gen, indices[i]);

        return -1;
    }

    int ok = cg_array_address(
        &gen->arithmetic,
        array,
        indices,
        count,
        address,
        work
    );

    if (ok) {
        ok = cg_array_load(
            &gen->arithmetic,
            array,
            indices,
            count,
            address,
            value,
            work
        );
    }

    // Libera registros de índices y del cálculo de dirección.
    // El valor cargado conserva su reserva para el llamador.
    for (int i = 0; i < count; i++)
        gen_free_reg(gen, indices[i]);

    gen_free_reg(gen, address);
    gen_free_reg(gen, work[0]);
    gen_free_reg(gen, work[1]);

    if (!ok) {
        gen_free_reg(gen, value);
        gen_report_error(gen, node, "no se pudo generar el acceso al arreglo");
        return -1;
    }

    return value;
}

/* gen_expression — véase el contrato en generate_internal.h. */
int gen_expression(GenCtx *gen, ASTNode *node) {
    if (node == NULL) {
        gen_report_error(gen, NULL, "expresion nula");
        return -1;
    }

    int rd;

    switch (node->kind) {

        case AST_NUMBER:
        case AST_BOOLEAN:

            rd = gen_alloc_reg(gen);

            if (rd < 0) {
                gen_report_error(gen, node,
                                 "sin registros temporales disponibles");
                return -1;
            }

            if (gen_mov(gen, rd, node->number, node) != 0) {
                gen_free_reg(gen, rd);
                return -1;
            }

            return rd;

        case AST_IDENTIFIER:

            rd = gen_alloc_reg(gen);

            if (rd < 0) {
                gen_report_error(gen, node,
                                 "sin registros temporales disponibles");
                return -1;
            }

            if (gen_load_symbol(gen, rd,
                                gen_get_symbol(node),
                                node) != 0) {

                gen_free_reg(gen, rd);
                return -1;
            }

            return rd;

        case AST_ADD:
        case AST_SUB:
        case AST_AND:
        case AST_OR:
        case AST_EQUAL:
        case AST_NOT_EQUAL:
        case AST_LESS:
        case AST_GREATER:
        case AST_LESS_EQUAL:
        case AST_GREATER_EQUAL:

            return emit_binary_expression(gen, node);

        case AST_NEG:
        case AST_NOT:

            return emit_unary(gen, node);

        case AST_MUL:
        case AST_DIV:
        case AST_MOD:
        case AST_POWER:

            return emit_arithmetic(gen, node);

        // nombre [ expr ] se escribe igual para una llamada y para la lectura
        // de un arreglo 1D: el parser arma AST_CALL y lo distingue el símbolo
        // que anotó el semántico. Un arreglo sigue por el camino de AST_INDEX
        // (mismos hijos: IDENTIFIER y ARGUMENTS con los índices).
        case AST_CALL: {
            Symbol *named = gen_get_symbol(gen_child_at(node, 0));

            if (named == NULL || named->dimensions == 0)
                return gen_function_call(gen, node, 1);

            return emit_array_read(gen, node);
        }

        case AST_INDEX:

            return emit_array_read(gen, node);

        default:
            gen_report_error(gen, node,
                             "nodo no valido como expresion");
            return -1;
    }
}

/* gen_call_arguments — véase el contrato en generate_internal.h. */
ASTNode *gen_call_arguments(ASTNode *args_node, int *count) {
    ASTNode *arg = NULL;

    if (args_node != NULL) {
        if (args_node->kind == AST_ARGUMENTS) {
            arg = args_node->child;
        } else {
            arg = args_node;
        }
    }

    // Cuenta argumentos.
    *count = 0;

    for (ASTNode *a = arg; a != NULL; a = a->next) {
        (*count)++;
    }

    return arg;
}

/*
 * CallInfo — lo que se sabe de una llamada una vez validada.
 */
typedef struct {
    ASTNode *args_node;      // AST_ARGUMENTS o NULL
    ASTNode *first_arg;      // primer argumento
    int arg_count;
    Symbol *callee;          // función llamada
    Symbol *return_symbol;   // su variable de retorno; NULL si no se pide resultado
} CallInfo;

/*
 * SavedRegs — temporales vivos del llamador que se apilan alrededor de la llamada.
 */
typedef struct {
    int regs[TEMP_COUNT];
    int count;
} SavedRegs;

/*
 * check_call — valida una llamada antes de emitir nada.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_CALL no NULL.
 *   result_needed  1 si se usa el resultado.
 *   call  estructura a llenar.
 * Salida:
 *   0 si la llamada es válida; -1 si reporta un error.
 *   Efectos: llena *call; reporta el primer problema encontrado.
 * Restricciones:
 *   el orden de las comprobaciones fija cuál error se reporta primero:
 *   identificador, símbolo de función, retorno, cantidad de argumentos,
 *   lista de parámetros y direcciones de los parámetros.
 */
static int check_call(GenCtx *gen, ASTNode *node, int result_needed,
                      CallInfo *call) {
    // AST_CALL:
    //   hijo 0: identificador de función
    //   hijo 1: AST_ARGUMENTS (opcional)
    ASTNode *function_id = gen_child_at(node, 0);
    call->args_node = gen_child_at(node, 1);

    if (function_id == NULL || function_id->text == NULL) {
        gen_report_error(gen, node,
                         "llamada sin identificador de funcion");
        return -1;
    }

    call->callee = gen_get_symbol(function_id);

    if (call->callee == NULL || call->callee->kind != SYMBOL_FUNCTION) {
        gen_report_error(gen, function_id,
                         "funcion sin simbolo semantico valido");
        return -1;
    }

    call->return_symbol = NULL;

    if (result_needed) {
        // Localiza la variable de retorno de la función.
        if (call->callee->type == TYPE_VOID) {
            gen_report_error(gen, node,
                             "una funcion void no puede usarse como expresion");
            return -1;
        }

        call->return_symbol = gen_get_return_symbol(call->callee);

        if (call->return_symbol == NULL) {
            gen_report_error(gen, node,
                             "no se encontro la variable de retorno");
            return -1;
        }

        if (call->return_symbol->address < 0 ||
            call->return_symbol->address > 131071) {
            gen_report_error(gen, node,
                             "direccion de retorno invalida");
            return -1;
        }
    }

    call->first_arg = gen_call_arguments(call->args_node, &call->arg_count);

    // Verifica cantidad de argumentos y lista de parámetros.
    if (call->arg_count != call->callee->parameter_count) {
        gen_report_error(gen, node,
                         "cantidad incorrecta de argumentos");
        return -1;
    }

    if (call->arg_count > 0 && call->callee->param_list == NULL) {
        gen_report_error(gen, node,
                         "funcion sin lista de parametros");
        return -1;
    }

    for (int i = 0; i < call->arg_count; i++) {
        Symbol *param = call->callee->param_list[i];

        if (param == NULL ||
            param->address < 0 ||
            param->address > 131071) {
            gen_report_error(gen, node,
                             "direccion de parametro invalida");
            return -1;
        }
    }

    return 0;
}

/*
 * save_live_temporaries — apila los temporales vivos del llamador.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   saved  estructura a llenar con los registros apilados.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite un push por cada temporal reservado y los anota en saved.
 * Restricciones:
 *   la función llamada puede sobrescribir t0–t11; los valores que usa el
 *   llamador se deben conservar a través de la llamada.
 */
static void save_live_temporaries(GenCtx *gen, SavedRegs *saved) {
    saved->count = 0;

    for (int i = 0; i < TEMP_COUNT; i++) {
        if (gen->temp_used[i]) {
            int reg = TEMP_FIRST + i;

            saved->regs[saved->count++] = reg;
            gen_stack_push(reg);
        }
    }
}

/*
 * release_saved — libera la reserva de los temporales apilados cuando la
 * generación de la llamada falla.
 */
static void release_saved(GenCtx *gen, const SavedRegs *saved) {
    for (int i = 0; i < saved->count; i++) {
        gen_free_reg(gen, saved->regs[i]);
    }
}

/*
 * push_arguments — evalúa los argumentos escalares y los apila.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   call  llamada ya validada.
 *   saved  temporales apilados, para liberarlos si falla.
 * Salida:
 *   0 si apila todos; -1 si algún argumento falla.
 *   Efectos: evalúa de izquierda a derecha y apila cada resultado de
 *   inmediato para conservarlo durante las llamadas de los argumentos
 *   siguientes.
 * Restricciones:
 *   un argumento arreglo no se evalúa ni pasa por la pila: se pasa su
 *   dirección (libro §1.6.6, paso por referencia) al escribir las casillas
 *   de los parámetros.
 */
static int push_arguments(GenCtx *gen, const CallInfo *call,
                          const SavedRegs *saved) {
    int position = 0;

    for (ASTNode *a = call->first_arg; a != NULL; a = a->next, position++) {

        if (call->callee->param_list[position]->dimensions > 0)
            continue;

        int reg = gen_expression(gen, a);

        if (reg < 0) {
            // La generación falló. Se debe descartar el ensamblador final
            // y liberar las reservas de registros del compilador.
            release_saved(gen, saved);

            return -1;
        }

        gen_stack_push(reg);
        gen_free_reg(gen, reg);
    }

    return 0;
}

/*
 * store_arguments — escribe los argumentos en las casillas de los parámetros.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo de la llamada, para los diagnósticos.
 *   call  llamada ya validada.
 *   saved  temporales apilados, para liberarlos si falla.
 * Salida:
 *   0 si escribe todos; -1 si alguno falla.
 *   Efectos: desapila en orden inverso para mantener la pila equilibrada;
 *   un parámetro arreglo recibe la dirección del arreglo.
 * Restricciones:
 *   las casillas son estáticas: son las de la función llamada.
 */
static int store_arguments(GenCtx *gen, ASTNode *node, const CallInfo *call,
                           const SavedRegs *saved) {
    for (int i = call->arg_count - 1; i >= 0; i--) {
        int reg = gen_alloc_reg(gen);

        if (reg < 0) {
            gen_report_error(gen, node,
                             "sin registros para transferir argumentos");

            release_saved(gen, saved);

            return -1;
        }

        Symbol *param = call->callee->param_list[i];

        int store_result;

        if (param->dimensions > 0) {
            // Parámetro arreglo: su casilla recibe la dirección del arreglo
            // (o la que guarda el parámetro arreglo que se reenvía). Se
            // escribe aquí, con los escalares, para que una llamada anidada
            // en otro argumento no la pise. La dirección no cambia entre la
            // evaluación y este punto, así que no hace falta apilarla.
            ASTNode *array_arg = gen_child_at(call->args_node, i);
            Symbol *source =
                array_arg != NULL && array_arg->kind == AST_IDENTIFIER
                    ? gen_get_symbol(array_arg)
                    : NULL;

            store_result =
                source != NULL &&
                cg_array_pass(&gen->arithmetic, source, param, reg)
                    ? 0
                    : -1;

            if (store_result != 0)
                gen_report_error(gen, array_arg != NULL ? array_arg : node,
                                 "no se pudo pasar el arreglo como argumento");
        }
        else {
            gen_stack_pop(reg);

            store_result = gen_store_symbol(gen, param, reg, node);
        }

        gen_free_reg(gen, reg);

        if (store_result != 0) {
            release_saved(gen, saved);

            return -1;
        }
    }

    return 0;
}

/*
 * jump_to_callee — salta a la función llamada y restaura lo apilado al volver.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   call  llamada ya validada.
 *   saved  temporales apilados que se restauran al volver.
 * Salida:
 *   0 si emite el salto; -1 si falta la etiqueta de entrada.
 *   Efectos: guarda rl, emite jas, restaura rl y los temporales vivos.
 * Restricciones:
 *   la entrada de la función llamada sale de la tabla de etiquetas: el
 *   símbolo guarda su nodo FUNCTION en decl_node.
 */
static int jump_to_callee(GenCtx *gen, const CallInfo *call,
                          const SavedRegs *saved) {
    char callee_label[128];

    if (!gen_node_label(gen, callee_label, sizeof(callee_label),
                        (ASTNode *) call->callee->decl_node, LABEL_FUNCTION,
                        "entrada de la funcion")) {
        release_saved(gen, saved);

        return -1;
    }

    // Los valores de argumentos ya salieron de la pila.
    // Solo quedan los temporales vivos guardados del llamador.
    // Guarda el enlace de retorno anterior antes de que jas escriba rl.
    gen_stack_push(1);  // rl

    // Invoca la función.
    // jas escribe la dirección de retorno en rl.
    cg_op(SLOT_BRU, "jas rl, %s", callee_label);

    // El E de la función llamada vuelve mediante el nuevo rl.
    // Restaura el rl anterior del llamador al volver.
    gen_stack_pop(1);

    // Restaura los temporales vivos del llamador en orden inverso.
    for (int i = saved->count - 1; i >= 0; i--) {
        gen_stack_pop(saved->regs[i]);
    }

    return 0;
}

/* gen_function_call — véase el contrato en generate_internal.h. */
int gen_function_call(GenCtx *gen, ASTNode *node, int result_needed) {
    if (node == NULL) {
        gen_report_error(gen, NULL, "llamada a funcion invalida");
        return -1;
    }

    CallInfo call;
    SavedRegs saved;

    if (check_call(gen, node, result_needed, &call) != 0)
        return -1;

    save_live_temporaries(gen, &saved);

    // Evalúa los argumentos escalares y los apila; luego los transfiere a
    // las casillas estáticas de parámetros de la función llamada.
    if (push_arguments(gen, &call, &saved) != 0 ||
        store_arguments(gen, node, &call, &saved) != 0 ||
        jump_to_callee(gen, &call, &saved) != 0)
        return -1;

    // Si la llamada es una sentencia, descarta el valor de retorno.
    if (!result_needed) {
        return 0;
    }

    // Reserva un registro para el resultado.
    int result_reg = gen_alloc_reg(gen);

    if (result_reg < 0) {
        gen_report_error(gen, node,
                         "sin registros para el resultado de la llamada");
        return -1;
    }

    // Lee el valor de retorno inmediatamente después de llamar.
    // Otra llamada puede sobrescribir la variable estática de retorno.
    if (gen_load_symbol(gen, result_reg, call.return_symbol, node) != 0) {
        gen_free_reg(gen, result_reg);
        return -1;
    }

    return result_reg;
}

/*
 * emit_crypto_argument — evalúa un argumento preservando argumentos de cifrado previos.
 *
 * Entrada:
 *   user  contexto CGCryptoContext no NULL.
 *   expr  expresión validada.
 *   dst  destino ISA 4–15.
 *   preserve_mask  bits ISA de argumentos que deben conservarse.
 * Salida:
 *   0 si evalúa; 1 si falla.
 *   Efectos: emite guardado/restauración en temp_base y cambia temp_used.
 * Restricciones:
 *   casillas usadas por live_mask no se reutilizan; requiere área válida
 *   para preserve_mask no vacío.
 */
static int emit_crypto_argument(void *user, ASTNode *expr, int dst,
                                unsigned preserve_mask) {
    GenCtx *gen = user;                  // crypto.user apunta al GenCtx
    CGCryptoContext *ctx = &gen->crypto;
    int saved[12], slots[12], count = 0;
    unsigned occupied = ctx->live_mask;
    for (int reg = 4; reg < 16; reg++) {
        if (!(preserve_mask & (1u << reg)))
            continue;
        int slot;
        for (slot = 4; slot < 16; slot++)
            if (!(occupied & (1u << slot)))
                break;
        if (slot == 16 || ctx->function == NULL || ctx->function->temp_base < 0) {
            gen_report_error(gen, expr, "sin casillas libres para preservar argumentos crypto");
            return 1;
        }
        occupied |= 1u << slot;
        saved[count] = reg;
        slots[count++] = ctx->function->temp_base + 4 * (slot - 4);
    }
    cg_gap();
    for (int i = 0; i < count; i++)
        cg_op(SLOT_LSU, "sw %s, %d(rz)", gen_reg_name(saved[i]), slots[i]);
    gen_reset_registers(gen);
    int result = gen_expression(gen, expr);
    if (result < 0)
        return 1;
    if (result != dst)
        cg_op(SLOT_ALU0, "saddi %s, %s, 0", gen_reg_name(dst), gen_reg_name(result));
    for (int i = 0; i < count; i++)
        cg_op(SLOT_LSU, "lw %s, %d(rz)", gen_reg_name(saved[i]), slots[i]);
    gen_reset_registers(gen);
    gen->temp_used[dst - TEMP_FIRST] = 1;
    for (int reg = 4; reg < 16; reg++)
        if (preserve_mask & (1u << reg))
            gen->temp_used[reg - TEMP_FIRST] = 1;
    return 0;
}

/* gen_crypto_statement — véase el contrato en generate_internal.h. */
void gen_crypto_statement(GenCtx *gen, ASTNode *node) {
    int used[TEMP_COUNT];
    memcpy(used, gen->temp_used, sizeof(used));
    gen->crypto.live_mask = 0;
    for (int i = 0; i < TEMP_COUNT; i++)
        if (used[i]) gen->crypto.live_mask |= 1u << (i + TEMP_FIRST);
    gen->crypto.function = gen->current_function_symbol;
    if (cg_crypto(&gen->crypto, node) != 0)
        gen_report_error(gen, node, "fallo al generar operacion crypto");
    memcpy(gen->temp_used, used, sizeof(used));
}

/* gen_expr_setup — véase el contrato en generate_internal.h. */
void gen_expr_setup(GenCtx *gen) {
    // CRYPTO evalúa sus argumentos con emit_crypto_argument; user lleva el
    // GenCtx para que el callback llegue a los temporales y a los errores.
    gen->crypto = (CGCryptoContext) {.eval = emit_crypto_argument, .next_label = 1};
    gen->crypto.user = gen;

    // La aritmética y los arreglos emiten por emit_arith_callback.
    gen->arithmetic.emit = emit_arith_callback;
    gen->arithmetic.user = NULL;
    gen->arithmetic.next_label = 0;
}
