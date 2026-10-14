#include <stdio.h>
#include <string.h>
#include <stdint.h>
 
#include "generate.h"
#include "codegen.h"
#include "symbol_table.h"
#include "semantic.h"
#include "codegen_arith.h"
#include "codegen_array.h"
#include "codegen_crypto.h"

#define TEMP_FIRST 4
#define TEMP_COUNT 12

/*
 * Pila de llamadas (libro §7.2): guarda las direcciones de retorno (rl), los
 * argumentos y los temporales vivos de las llamadas anidadas. Las variables
 * siguen con asignación estática (libro §7.1.1); solo esto usa una pila.
 *
 * rx es un temporal libre según isa.md §8; aquí se usa por convención como
 * puntero de pila. La pila crece hacia arriba (emit_stack_push).
 *
 * Vive en la zona de ccom (0x0000–0x0FFF): empieza en la primera dirección
 * libre después de la última área que asignó el semántico
 * (semantic_data_end) y llega hasta 0x0FFF. No puede estar desde 0x1000,
 * que es la zona de los archivos de load_file (la de CRYPTO).
 *
 * Uso por llamada, en palabras: mientras se evalúan los argumentos, una por
 * cada temporal vivo más una por argumento; al saltar, una por cada temporal
 * vivo más una para rl. La profundidad está acotada porque el semántico
 * prohíbe la recursión. No se revisa el desborde en ejecución: el semántico
 * exige al menos 256 bytes libres para la pila.
 */
/*
 * Etiquetas: la tabla de etiquetas se construye en la Fase 3
 * (label_table.c) y la Fase 4 solo la consume. Cada etiqueta de control del
 * .asm sale de label_table_get con el nodo del AST y su rol; el generador
 * no inventa nombres. Si falta una, es un error interno.
 *
 * Quedan fuera de la tabla las etiquetas internas de cada expansión, que no
 * son nodos del AST: __ccom_arith_N_*, __ccom_array_N_* y .L_crypto_*.
 */
static const LabelTable *label_table;

// Etiquetas globales (nodo NULL en la tabla), leídas al empezar.
static const char *arith_error_label;   // destino de / % ^ con error
static const char *program_end_label;   // fin_main (contrato §3)

static CGArithContext arithmetic_context;
static CGCryptoContext crypto_context;
static ASTNode *program_globals;

static int alloc_reg(void);
static void free_reg(int reg);
static void report_error(ASTNode *node, const char *message);

static int temp_used[TEMP_COUNT];
static int generation_errors;

static const char *current_function_name;
static Symbol *current_function_symbol;

static void report_error(ASTNode *node, const char *message) {
    if (node != NULL && node->line > 0) {
        fprintf(stderr,
                "ccom:%d:%d: error de generacion: %s\n",
                node->line,
                node->column,
                message);
    } else {
        fprintf(stderr,
                "ccom: error de generacion: %s\n",
                message);
    }

    generation_errors++;
}
 
static ASTNode *find_child(ASTNode *node, ASTKind kind) {
    for (ASTNode *c = node ? node->child : NULL; c != NULL; c = c->next)
        if (c->kind == kind)
            return c;
 
    return NULL;
}
 
static const char *function_name(ASTNode *function) {
    ASTNode *id = find_child(function, AST_IDENTIFIER);
 
    return id ? id->text : NULL;
}

static ASTNode *child_at(ASTNode *node, int index) {
    ASTNode *child = node ? node->child : NULL;

    while (child != NULL && index > 0) {
        child = child->next;
        index--;
    }

    return child;
}

static const char *reg_name(int reg) {
    static const char *names[16] = {
        "rz", "rl", "rx", "rg",
        "t0", "t1", "t2", "t3",
        "t4", "t5", "t6", "t7",
        "t8", "t9", "t10", "t11"
    };

    if (reg < 0 || reg >= 16)
        return "rz";

    return names[reg];
}

static void reset_registers(void) {
    memset(temp_used, 0, sizeof(temp_used));
}

static int alloc_reg(void) {
    for (int i = 0; i < TEMP_COUNT; i++) {
        if (!temp_used[i]) {
            temp_used[i] = 1;
            return TEMP_FIRST + i;
        }
    }

    return -1;
}

static void free_reg(int reg) {
    if (reg >= TEMP_FIRST &&
        reg < TEMP_FIRST + TEMP_COUNT) {

        temp_used[reg - TEMP_FIRST] = 0;
    }
}

static int emit_mov(int rd, long immediate, ASTNode *node) {
    /* 2147483648 aparece como hijo del menos unario de INT32_MIN. */
    if (immediate < INT32_MIN || immediate > (long) UINT32_MAX) {
        report_error(node, "constante fuera de 32 bits");
        return -1;
    }
    int64_t value = immediate;
    if (value > INT32_MAX)
        value -= INT64_C(4294967296);
    if (value >= -131072 && value <= 131071) {
        cg_op(SLOT_ALU0, "mov %s, %ld", reg_name(rd), (long) value);
        return 0;
    }
    int scratch = alloc_reg();
    if (scratch < 0) {
        report_error(node, "sin registro para construir constante de 32 bits");
        return -1;
    }
    int64_t high = value / 32768, low = value % 32768;
    if (low < 0) { high--; low += 32768; }
    char first[64], second[64];
    snprintf(first, sizeof(first), "mov %s, %ld", reg_name(rd), (long) high);
    snprintf(second, sizeof(second), "mov %s, 15", reg_name(scratch));
    cg_bundle(first, second, NULL, NULL);
    cg_gap();
    cg_op(SLOT_ALU0, "sll %s, %s, %s", reg_name(rd), reg_name(rd), reg_name(scratch));
    cg_op(SLOT_ALU0, "saddi %s, %s, %ld", reg_name(rd), reg_name(rd), (long) low);
    free_reg(scratch);
    return 0;
}

static Symbol *get_symbol(ASTNode *node) {
    if (node == NULL || node->symbol == NULL)
        return NULL;

    return (Symbol *)node->symbol;
}

static Symbol *get_return_symbol(Symbol *function_symbol) {
    if (function_symbol == NULL ||
        function_symbol->decl_node == NULL) {
        return NULL;
    }

    ASTNode *function_node =
        (ASTNode *)function_symbol->decl_node;

    ASTNode *return_type =
        find_child(function_node, AST_RETURN_TYPE);

    if (return_type == NULL)
        return NULL;

    ASTNode *return_id =
        find_child(return_type, AST_IDENTIFIER);

    if (return_id == NULL)
        return NULL;

    return get_symbol(return_id);
}

static int emit_load_symbol(int rd, Symbol *symbol, ASTNode *node) {
    if (symbol == NULL) {
        report_error(node,
                     "identificador sin simbolo semantico");
        return -1;
    }

    if (symbol->address < 0) {
        report_error(node,
                     "simbolo sin direccion de memoria");
        return -1;
    }

    if (symbol->address > 131071) {
        report_error(node,
                     "direccion fuera del rango de lw");
        return -1;
    }

    cg_op(SLOT_LSU,
          "lw %s, %d(rz)",
          reg_name(rd),
          symbol->address);

    return 0;
}

static int emit_store_symbol(Symbol *symbol, int rs, ASTNode *node) {
    if (symbol == NULL) {
        report_error(node,
                     "destino sin simbolo semantico");
        return -1;
    }

    if (symbol->address < 0 ||
        symbol->address > 131071) {

        report_error(node,
                     "direccion de almacenamiento invalida");
        return -1;
    }

    cg_op(SLOT_LSU,
          "sw %s, %d(rz)",
          reg_name(rs),
          symbol->address);

    return 0;
}

static void emit_stack_push(int reg) {
    cg_op(SLOT_LSU,
          "sw %s, 0(rx)",
          reg_name(reg));

    cg_op(SLOT_ALU0,
          "saddi rx, rx, 4");
}

static void emit_stack_pop(int reg) {
    cg_op(SLOT_ALU0,
          "saddi rx, rx, -4");

    cg_op(SLOT_LSU,
          "lw %s, 0(rx)",
          reg_name(reg));
}

static int emit_expression(ASTNode *node);
static int emit_function_call(ASTNode *node, int result_needed);

static int emit_arith_callback(void *user, const CGArithOp *op) {
    (void)user;

    if (op == NULL)
        return 0;

    // Labels are represented by slot == -1.
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

    // Arithmetic emitter slot 0 = ALU0.
    if (op->slot == 0) {
        cg_op(SLOT_ALU0, "%s", op->text);
        return 1;
    }

    // Arithmetic emitter slot 2 = branch/jump unit.
    if (op->slot == 2) {
        cg_op(SLOT_BRU, "%s", op->text);
        return 1;
    }

    // Array emitter slot 3 = lsu
    if (op->slot == 3) {
        cg_op(SLOT_LSU, "%s", op->text);
        return 1;
    }

    return 0;
}

static int emit_software_arithmetic(ASTNode *node, CGArithKind kind, int lhs, int rhs) {
    int dst = alloc_reg();

    if (dst < 0) {
        report_error(
            node,
            "sin registros para el resultado aritmetico"
        );
        free_reg(lhs);
        free_reg(rhs);
        return -1;
    }

    int work[8];
    int allocated = 0;

    for (int i = 0; i < 8; i++) {
        work[i] = alloc_reg();

        if (work[i] < 0) {
            report_error(
                node,
                "sin suficientes registros para la rutina aritmetica"
            );

            for (int j = 0; j < allocated; j++)
                free_reg(work[j]);

            free_reg(dst);
            free_reg(lhs);
            free_reg(rhs);

            return -1;
        }

        allocated++;
    }

    const char *error_label = NULL;

    if (kind == CG_ARITH_DIV ||
        kind == CG_ARITH_MOD ||
        kind == CG_ARITH_POWER) {
        error_label = arith_error_label;
    }

    int success = cg_arith_emit(
        &arithmetic_context,
        kind,
        lhs,
        rhs,
        dst,
        work,
        error_label
    );

    for (int i = 0; i < 8; i++)
        free_reg(work[i]);

    free_reg(lhs);
    free_reg(rhs);

    if (!success) {
        free_reg(dst);

        report_error(
            node,
            "fallo al generar la rutina aritmetica"
        );

        return -1;
    }

    // dst remains allocated because the caller needs
    // its value as the expression result.
    return dst;
}

static int node_label(char *buffer, size_t size, ASTNode *node,
                      LabelRole role, const char *what);

/*
 * & y | en corto circuito (libro §6.6.2, Short-Circuit Code; G-11 de
 * ccom_pendientes.md): el lado derecho solo se evalúa si hace falta.
 *
 *   a & b:  r = a ; beq r, rz, fin ; r = b ; fin:
 *   a | b:  r = a ; bne r, rz, fin ; r = b ; fin:
 *
 * El resultado queda en r como 0 o 1, porque los bool ya valen 0 o 1. En
 * los dos caminos (el que salta y el que no) queda ocupado solo r: el
 * registro de b se libera antes de la etiqueta. cg_op deja el bundle de
 * separación después de escribir r y después del salto (isa.md §9.3).
 */
static int emit_short_circuit(ASTNode *node, ASTNode *left, ASTNode *right) {
    int result = emit_expression(left);

    if (result < 0)
        return -1;

    char end_label[64];

    if (!node_label(end_label, sizeof(end_label), node, LABEL_SC_END,
                    "corto circuito")) {
        free_reg(result);
        return -1;
    }

    cg_op(SLOT_BRU,
          "%s %s, rz, %s",
          node->kind == AST_AND ? "beq" : "bne",
          reg_name(result),
          end_label);

    int other = emit_expression(right);

    if (other < 0) {
        free_reg(result);
        return -1;
    }

    cg_op(SLOT_ALU0,
          "saddi %s, %s, 0",
          reg_name(result),
          reg_name(other));

    free_reg(other);

    cg_label(end_label);

    return result;
}

static int emit_binary_expression(ASTNode *node) {
    ASTNode *left = child_at(node, 0);
    ASTNode *right = child_at(node, 1);

    if (left == NULL || right == NULL) {
        report_error(node,
                     "expresion binaria incompleta");
        return -1;
    }

    if (node->kind == AST_AND || node->kind == AST_OR)
        return emit_short_circuit(node, left, right);

    int rd_left = emit_expression(left);

    if (rd_left < 0)
        return -1;

    int rd_right = emit_expression(right);

    if (rd_right < 0) {
        free_reg(rd_left);
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
            // a > b is equivalent to b < a.
            op = "slt";
            {
                int tmp = rd_left;
                rd_left = rd_right;
                rd_right = tmp;
            }
            break;

        case AST_GREATER_EQUAL:
            // a >= b is equivalent to NOT(a < b).
            cg_op(SLOT_ALU0,
                  "slt %s, %s, %s",
                  reg_name(rd_left),
                  reg_name(rd_left),
                  reg_name(rd_right));

            cg_op(SLOT_ALU0,
                  "seq %s, %s, rz",
                  reg_name(rd_left),
                  reg_name(rd_left));

            free_reg(rd_right);
            return rd_left;

        default:
            report_error(node,
                         "operador binario no implementado");
            free_reg(rd_left);
            free_reg(rd_right);
            return -1;
    }
    // left register for the result.
    cg_op(SLOT_ALU0,
          "%s %s, %s, %s",
          op,
          reg_name(rd_left),
          reg_name(rd_left),
          reg_name(rd_right));

    free_reg(rd_right);

    return rd_left;
}

static int emit_expression(ASTNode *node) {
    if (node == NULL) {
        report_error(NULL, "expresion nula");
        return -1;
    }

    int rd;

    switch (node->kind) {

        case AST_NUMBER:
        case AST_BOOLEAN:

            rd = alloc_reg();

            if (rd < 0) {
                report_error(node,
                             "sin registros temporales disponibles");
                return -1;
            }

            if (emit_mov(rd, node->number, node) != 0) {
                free_reg(rd);
                return -1;
            }

            return rd;

        case AST_IDENTIFIER:

            rd = alloc_reg();

            if (rd < 0) {
                report_error(node,
                             "sin registros temporales disponibles");
                return -1;
            }

            if (emit_load_symbol(rd,
                                 get_symbol(node),
                                 node) != 0) {

                free_reg(rd);
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

            return emit_binary_expression(node);

        case AST_NEG:
        case AST_NOT: {

            ASTNode *operand = child_at(node, 0);

            if (operand == NULL) {
                report_error(node,
                             "operador unario sin operando");
                return -1;
            }

            rd = emit_expression(operand);

            if (rd < 0)
                return -1;

            if (node->kind == AST_NEG) {
                cg_op(SLOT_ALU0,
                      "sub %s, rz, %s",
                      reg_name(rd),
                      reg_name(rd));
            } else {
                cg_op(SLOT_ALU0,
                      "seq %s, %s, rz",
                      reg_name(rd),
                      reg_name(rd));
            }

            return rd;
        }

        case AST_MUL:
        case AST_DIV:
        case AST_MOD:
        case AST_POWER: {
            ASTNode *left = child_at(node, 0);
            ASTNode *right = child_at(node, 1);

            if (left == NULL || right == NULL) {
                report_error(
                    node,
                    "expresion aritmetica incompleta"
                );
                return -1;
            }

            int lhs = emit_expression(left);

            if (lhs < 0)
                return -1;

            int rhs = emit_expression(right);

            if (rhs < 0) {
                free_reg(lhs);
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
                    free_reg(lhs);
                    free_reg(rhs);
                    return -1;
            }

            return emit_software_arithmetic(
                node,
                kind,
                lhs,
                rhs
            );
        }

        // nombre [ expr ] se escribe igual para una llamada y para la lectura
        // de un arreglo 1D: el parser arma AST_CALL y lo distingue el símbolo
        // que anotó el semántico. Un arreglo sigue por el camino de AST_INDEX
        // (mismos hijos: IDENTIFIER y ARGUMENTS con los índices).
        case AST_CALL: {
            Symbol *named = get_symbol(child_at(node, 0));

            if (named == NULL || named->dimensions == 0)
                return emit_function_call(node, 1);
        }
        /* fallthrough */

        case AST_INDEX: {
            ASTNode *array_id = child_at(node, 0);
            ASTNode *arguments = child_at(node, 1);

            if (array_id == NULL || array_id->kind != AST_IDENTIFIER) {
                report_error(node, "identificador de arreglo invalido");
                return -1;
            }

            Symbol *array = get_symbol(array_id);

            if (array == NULL) {
                report_error(array_id, "arreglo sin simbolo semantico");
                return -1;
            }

            if (array->dimensions < 1 || array->dimensions > MAX_DIMS) {
                report_error(node, "numero de dimensiones invalido");
                return -1;
            }

            // Evaluate the indices.
            // For a 1D array: indices[0] = index.
            // For a 2D array: indices[0] = row, indices[1] = column.
            int indices[MAX_DIMS] = {-1, -1};
            int count = 0;

            for (ASTNode *idx = arguments->child;
                idx != NULL;
                idx = idx->next) {

                if (count >= array->dimensions) {
                    report_error(node, "demasiados indices para el arreglo");

                    for (int i = 0; i < count; i++)
                        free_reg(indices[i]);

                    return -1;
                }

                indices[count] = emit_expression(idx);

                if (indices[count] < 0) {
                    for (int i = 0; i < count; i++)
                        free_reg(indices[i]);

                    return -1;
                }

                count++;
            }

            if (count != array->dimensions) {
                report_error(node, "cantidad incorrecta de indices");

                for (int i = 0; i < count; i++)
                    free_reg(indices[i]);

                return -1;
            }

            // Allocate the destination address and two work registers.
            int address = alloc_reg();
            int work[2] = {-1, -1};
            int value = -1;

            if (address >= 0)
                work[0] = alloc_reg();

            if (work[0] >= 0)
                work[1] = alloc_reg();

            if (work[1] >= 0)
                value = alloc_reg();

            if (address < 0 || work[0] < 0 ||
                work[1] < 0 || value < 0) {

                report_error(node, "sin registros para acceder al arreglo");

                if (address >= 0) free_reg(address);
                if (work[0] >= 0) free_reg(work[0]);
                if (work[1] >= 0) free_reg(work[1]);
                if (value >= 0) free_reg(value);

                for (int i = 0; i < count; i++)
                    free_reg(indices[i]);

                return -1;
            }

            int ok = cg_array_address(
                &arithmetic_context,
                array,
                indices,
                count,
                address,
                work
            );

            if (ok) {
                ok = cg_array_load(
                    &arithmetic_context,
                    array,
                    indices,
                    count,
                    address,
                    value,
                    work
                );
            }

            // Release the index and address-calculation registers.
            // The loaded value remains allocated for the caller.
            for (int i = 0; i < count; i++)
                free_reg(indices[i]);

            free_reg(address);
            free_reg(work[0]);
            free_reg(work[1]);

            if (!ok) {
                free_reg(value);
                report_error(node, "no se pudo generar el acceso al arreglo");
                return -1;
            }

            return value;
        }

        default:
            report_error(node,
                         "nodo no valido como expresion");
            return -1;
    }
}

static int emit_function_call(ASTNode *node, int result_needed) {
    if (node == NULL) {
        report_error(NULL, "llamada a funcion invalida");
        return -1;
    }

    // AST_CALL:
    //   child 0: function identifier
    //   child 1: AST_ARGUMENTS (optional)
    ASTNode *function_id = child_at(node, 0);
    ASTNode *args_node = child_at(node, 1);

    if (function_id == NULL || function_id->text == NULL) {
        report_error(node,
                     "llamada sin identificador de funcion");
        return -1;
    }

    Symbol *callee = get_symbol(function_id);

    if (callee == NULL || callee->kind != SYMBOL_FUNCTION) {
        report_error(function_id,
                     "funcion sin simbolo semantico valido");
        return -1;
    }

    Symbol *return_symbol = NULL;

    if (result_needed) {
        // Locate the function's return variable.
        if (callee->type == TYPE_VOID) {
            report_error(node,
                            "una funcion void no puede usarse como expresion");
            return -1;
        }

        return_symbol = get_return_symbol(callee);

        if (return_symbol == NULL) {
            report_error(node,
                        "no se encontro la variable de retorno");
            return -1;
        }

        if (return_symbol->address < 0 ||
            return_symbol->address > 131071) {
            report_error(node,
                        "direccion de retorno invalida");
            return -1;
        }
    }

    // Unwrap the argument list.
    ASTNode *arg = NULL;

    if (args_node != NULL) {
        if (args_node->kind == AST_ARGUMENTS) {
            arg = args_node->child;
        } else {
            arg = args_node;
        }
    }

    // Count arguments.
    int arg_count = 0;

    for (ASTNode *a = arg; a != NULL; a = a->next) {
        arg_count++;
    }

    // Validate the argument count and parameter list.
    if (arg_count != callee->parameter_count) {
        report_error(node,
                     "cantidad incorrecta de argumentos");
        return -1;
    }

    if (arg_count > 0 && callee->param_list == NULL) {
        report_error(node,
                     "funcion sin lista de parametros");
        return -1;
    }

    for (int i = 0; i < arg_count; i++) {
        Symbol *param = callee->param_list[i];

        if (param == NULL ||
            param->address < 0 ||
            param->address > 131071) {
            report_error(node,
                         "direccion de parametro invalida");
            return -1;
        }
    }

    // Save all currently live temporaries.
    // The callee can overwrite t0-t11, so values already being
    // used by the caller must be preserved across the call.
    int saved_regs[TEMP_COUNT];
    int saved_count = 0;

    for (int i = 0; i < TEMP_COUNT; i++) {
        if (temp_used[i]) {
            int reg = TEMP_FIRST + i;

            saved_regs[saved_count++] = reg;
            emit_stack_push(reg);
        }
    }

    // Evaluate arguments from left to right.
    // Push each result to memory immediately. This preserves it
    // across calls needed to evaluate subsequent arguments.
    //
    // Un argumento arreglo no se evalúa ni pasa por la pila: se pasa su
    // dirección (libro §1.6.6, paso por referencia) más abajo, al escribir
    // las casillas de los parámetros.
    int position = 0;

    for (ASTNode *a = arg; a != NULL; a = a->next, position++) {

        if (callee->param_list[position]->dimensions > 0)
            continue;

        int reg = emit_expression(a);

        if (reg < 0) {
            // Generation has failed. The final assembly must not
            // be used, but release the compiler's register state.
            for (int i = 0; i < saved_count; i++) {
                free_reg(saved_regs[i]);
            }

            return -1;
        }

        emit_stack_push(reg);
        free_reg(reg);
    }

    // Transfer arguments to the callee's statically allocated
    // parameter locations.
    // Pop in reverse order so the stack remains balanced.
    for (int i = arg_count - 1; i >= 0; i--) {
        int reg = alloc_reg();

        if (reg < 0) {
            report_error(node,
                         "sin registros para transferir argumentos");

            for (int j = 0; j < saved_count; j++) {
                free_reg(saved_regs[j]);
            }

            return -1;
        }

        Symbol *param = callee->param_list[i];

        int store_result;

        if (param->dimensions > 0) {
            // Parámetro arreglo: su casilla recibe la dirección del arreglo
            // (o la que guarda el parámetro arreglo que se reenvía). Se
            // escribe aquí, con los escalares, para que una llamada anidada
            // en otro argumento no la pise. La dirección no cambia entre la
            // evaluación y este punto, así que no hace falta apilarla.
            ASTNode *array_arg = child_at(args_node, i);
            Symbol *source =
                array_arg != NULL && array_arg->kind == AST_IDENTIFIER
                    ? get_symbol(array_arg)
                    : NULL;

            store_result =
                source != NULL &&
                cg_array_pass(&arithmetic_context, source, param, reg)
                    ? 0
                    : -1;

            if (store_result != 0)
                report_error(array_arg != NULL ? array_arg : node,
                             "no se pudo pasar el arreglo como argumento");
        }
        else {
            emit_stack_pop(reg);

            store_result = emit_store_symbol(param, reg, node);
        }

        free_reg(reg);

        if (store_result != 0) {
            for (int j = 0; j < saved_count; j++) {
                free_reg(saved_regs[j]);
            }

            return -1;
        }
    }

    // La entrada de la función llamada sale de la tabla: el símbolo guarda
    // su nodo FUNCTION en decl_node.
    char callee_label[128];

    if (!node_label(callee_label, sizeof(callee_label),
                    (ASTNode *) callee->decl_node, LABEL_FUNCTION,
                    "entrada de la funcion")) {
        for (int j = 0; j < saved_count; j++) {
            free_reg(saved_regs[j]);
        }

        return -1;
    }

    // The argument values have now been removed from the stack.
    // Only the caller's saved temporaries remain.
    // Save the old return link before jas overwrites rl.
    emit_stack_push(1);  // rl

    // Call the function.
    // jas sets rl to the return address.
    cg_op(SLOT_BRU, "jas rl, %s", callee_label);

    // The callee's E returns through the new rl.
    // Restore the caller's previous rl after returning.
    emit_stack_pop(1);

    // Restore live caller temporaries in reverse order.
    for (int i = saved_count - 1; i >= 0; i--) {
        emit_stack_pop(saved_regs[i]);
    }

    // If this call is used as a statement, discard the return value.
    if (!result_needed) {
        return 0;
    }

    // Allocate a register for the result.
    int result_reg = alloc_reg();

    if (result_reg < 0) {
        report_error(node,
                    "sin registros para el resultado de la llamada");
        return -1;
    }

    // Load the return value immediately after the call.
    // Another call could overwrite the function's static return variable.
    if (emit_load_symbol(result_reg, return_symbol, node) != 0) {
        free_reg(result_reg);
        return -1;
    }

    return result_reg;
}

static int emit_crypto_argument(void *user, ASTNode *expr, int dst,
                                unsigned preserve_mask) {
    CGCryptoContext *ctx = user;
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
            report_error(expr, "sin casillas libres para preservar argumentos crypto");
            return 1;
        }
        occupied |= 1u << slot;
        saved[count] = reg;
        slots[count++] = ctx->function->temp_base + 4 * (slot - 4);
    }
    cg_gap();
    for (int i = 0; i < count; i++)
        cg_op(SLOT_LSU, "sw %s, %d(rz)", reg_name(saved[i]), slots[i]);
    reset_registers();
    int result = emit_expression(expr);
    if (result < 0)
        return 1;
    if (result != dst)
        cg_op(SLOT_ALU0, "saddi %s, %s, 0", reg_name(dst), reg_name(result));
    for (int i = 0; i < count; i++)
        cg_op(SLOT_LSU, "lw %s, %d(rz)", reg_name(saved[i]), slots[i]);
    reset_registers();
    temp_used[dst - TEMP_FIRST] = 1;
    for (int reg = 4; reg < 16; reg++)
        if (preserve_mask & (1u << reg))
            temp_used[reg - TEMP_FIRST] = 1;
    return 0;
}

static void emit_crypto_statement(ASTNode *node) {
    int used[TEMP_COUNT];
    memcpy(used, temp_used, sizeof(used));
    crypto_context.live_mask = 0;
    for (int i = 0; i < TEMP_COUNT; i++)
        if (used[i]) crypto_context.live_mask |= 1u << (i + TEMP_FIRST);
    crypto_context.function = current_function_symbol;
    if (cg_crypto(&crypto_context, node) != 0)
        report_error(node, "fallo al generar operacion crypto");
    memcpy(temp_used, used, sizeof(used));
}

static ASTNode *declaration_initializer(ASTNode *node) {
    ASTNode *type = find_child(node, AST_TYPE);
    ASTNode *init = type ? type->next : NULL;
    return init && init->kind == AST_DIMENSIONS ? init->next : init;
}

/* Constantes globales en la imagen; las expresiones dinámicas se emiten en main. */
static int constant_value(ASTNode *node, uint32_t *out, int depth) {
    if (!node || depth > 64) return 0;
    uint32_t a, b;
    if (node->kind == AST_NUMBER || node->kind == AST_BOOLEAN) {
        *out = (uint32_t) node->number; return 1;
    }
    if (node->kind == AST_IDENTIFIER) {
        Symbol *sym = get_symbol(node);
        return sym && sym->modifier == MODIFIER_VI && sym->decl_node &&
            constant_value(declaration_initializer(sym->decl_node), out, depth + 1);
    }
    if (!constant_value(node->child, &a, depth + 1)) return 0;
    if (node->kind == AST_NEG) { *out = 0u - a; return 1; }
    if (node->kind == AST_NOT) { *out = !a; return 1; }
    if (!constant_value(node->child->next, &b, depth + 1)) return 0;
    int64_t x = a <= INT32_MAX ? (int64_t) a : (int64_t) a - INT64_C(4294967296);
    int64_t y = b <= INT32_MAX ? (int64_t) b : (int64_t) b - INT64_C(4294967296);
    switch (node->kind) {
        case AST_ADD: *out = a + b; break;
        case AST_SUB: *out = a - b; break;
        case AST_MUL: *out = a * b; break;
        case AST_DIV: if (!b) return 0; *out = (uint32_t) (x / y); break;
        case AST_MOD: if (!b) return 0; *out = (uint32_t) (x % y); break;
        case AST_POWER: {
            if (y < 0) return 0;
            uint32_t result = 1;
            while (b) { if (b & 1) result *= a; a *= a; b >>= 1; }
            *out = result; break;
        }
        case AST_AND: *out = a != 0 && b != 0; break;
        case AST_OR: *out = a != 0 || b != 0; break;
        case AST_EQUAL: *out = a == b; break;
        case AST_NOT_EQUAL: *out = a != b; break;
        case AST_LESS: *out = x < y; break;
        case AST_GREATER: *out = x > y; break;
        case AST_LESS_EQUAL: *out = x <= y; break;
        case AST_GREATER_EQUAL: *out = x >= y; break;
        default: return 0;
    }
    return 1;
}

static void initialize_cells(ASTNode *init, Symbol *symbol, int *offset,
                             int image, int dynamic_only) {
    if (init && init->kind == AST_INITIALIZER_LIST) {
        for (ASTNode *item = init->child; item; item = item->next)
            initialize_cells(item, symbol, offset, image, dynamic_only);
        return;
    }
    uint32_t value = 0;
    int constant = !init || constant_value(init, &value, 0);
    int address = symbol->address + *offset;
    *offset += 4;
    if (image) {
        if (cg_data_set(address, constant ? value : 0) != 0)
            report_error(init, "inicializador fuera de la imagen de datos");
    } else if (!dynamic_only || !constant) {
        int reg = init ? emit_expression(init) : alloc_reg();
        if (reg < 0) { report_error(init, "fallo al inicializar variable"); return; }
        if (!init && emit_mov(reg, 0, NULL) != 0) { free_reg(reg); return; }
        cg_op(SLOT_LSU, "sw %s, %d(rz)", reg_name(reg), address);
        free_reg(reg);
    }
}

static void initialize_declaration(ASTNode *node, int image, int dynamic_only) {
    Symbol *symbol = get_symbol(find_child(node, AST_IDENTIFIER));
    if (!symbol || symbol->address < 0) { report_error(node, "declaracion sin direccion"); return; }
    ASTNode *init = declaration_initializer(node);
    int offset = 0;
    if (init) initialize_cells(init, symbol, &offset, image, dynamic_only);
    while (offset < symbol->width)
        initialize_cells(NULL, symbol, &offset, image, dynamic_only);
}

// Copia en buffer la etiqueta de la tabla para ese nodo y rol. Si no está,
// reporta un error interno (no se inventa una) y devuelve 0.
static int node_label(char *buffer, size_t size, ASTNode *node,
                      LabelRole role, const char *what) {
    const char *name = label_table_get(label_table, node, role);

    if (name == NULL || strlen(name) >= size) {
        char message[128];

        snprintf(message, sizeof(message),
                 "error interno: falta la etiqueta de %s en la tabla", what);

        report_error(node, message);

        return 0;
    }

    strcpy(buffer, name);

    return 1;
}

static void emit_statement(ASTNode *node);

static void emit_block(ASTNode *block) {
    if (block == NULL)
        return;

    for (ASTNode *stmt = block->child;
         stmt != NULL;
         stmt = stmt->next) {

        emit_statement(stmt);
    }
}

static void emit_assignment(ASTNode *node) {
    ASTNode *target = child_at(node, 0);
    ASTNode *value = child_at(node, 1);

    if (target == NULL || value == NULL) {
        report_error(node,
                     "asignacion incompleta");
        return;
    }

    // Asignación de un arreglo completo (destino = nombre de arreglo sin
    // índices): se copian todas sus palabras. El semántico ya exigió que el
    // origen sea un arreglo de la misma forma; cualquiera de los dos puede
    // ser un parámetro arreglo (cg_array_copy usa la dirección que guarda).
    Symbol *whole =
        target->kind == AST_IDENTIFIER ? get_symbol(target) : NULL;

    if (whole != NULL && whole->dimensions > 0) {
        Symbol *source =
            value->kind == AST_IDENTIFIER ? get_symbol(value) : NULL;

        int work[4];
        int allocated = 0;

        while (allocated < 4 && (work[allocated] = alloc_reg()) >= 0)
            allocated++;

        if (allocated < 4)
            report_error(node, "sin registros para copiar el arreglo");
        else if (source == NULL ||
                 !cg_array_copy(&arithmetic_context, whole, source, work))
            report_error(node, "no se pudo copiar el arreglo");

        for (int i = 0; i < allocated; i++)
            free_reg(work[i]);

        return;
    }

    // Evaluate the right-hand side first.
    // Keep its value in a register until the assignment is complete.
    int rd = emit_expression(value);

    if (rd < 0)
        return;

    // Scalar assignments.
    if (target->kind == AST_IDENTIFIER) {
        Symbol *symbol = get_symbol(target);

        if (symbol == NULL) {
            report_error(target, "variable sin simbolo semantico");
            free_reg(rd);
            return;
        }

        if (emit_store_symbol(symbol, rd, target) != 0) {
            free_reg(rd);
            return;
        }

        free_reg(rd);
        return;
    }

    // Array assignment
    if (target->kind == AST_INDEX) {
        ASTNode *array_id = child_at(target, 0);

        if (array_id == NULL ||
            array_id->kind != AST_IDENTIFIER) {
            report_error(target, "identificador de arreglo invalido");
            free_reg(rd);
            return;
        }

        Symbol *array = get_symbol(array_id);

        if (array == NULL) {
            report_error(array_id, "arreglo sin simbolo semantico");
            free_reg(rd);
            return;
        }

        if (array->dimensions < 1 ||
            array->dimensions > MAX_DIMS) {
            report_error(target, "numero de dimensiones invalido");
            free_reg(rd);
            return;
        }

        // Extract the index expressions.
        // This assumes the AST_INDEX children are:
        //   child 0: array identifier
        //   child 1 onward: index expressions
        int indices[MAX_DIMS] = {-1, -1};
        int count = 0;

        for (ASTNode *idx = array_id->next->child;
             idx != NULL;
             idx = idx->next) {

            if (count >= array->dimensions) {
                report_error(target, "demasiados indices para el arreglo");

                for (int i = 0; i < count; i++)
                    free_reg(indices[i]);

                free_reg(rd);
                return;
            }

            indices[count] = emit_expression(idx);

            if (indices[count] < 0) {
                for (int i = 0; i < count; i++)
                    free_reg(indices[i]);

                free_reg(rd);
                return;
            }

            count++;
        }

        if (count != array->dimensions) {
            report_error(target, "cantidad incorrecta de indices");

            for (int i = 0; i < count; i++)
                free_reg(indices[i]);

            free_reg(rd);
            return;
        }

        // Allocate registers for the element address.
        int address = alloc_reg();
        int work[2] = {-1, -1};

        if (address >= 0)
            work[0] = alloc_reg();

        if (work[0] >= 0)
            work[1] = alloc_reg();

        if (address < 0 || work[0] < 0 || work[1] < 0) {
            report_error(target, "sin registros para acceder al arreglo");

            if (address >= 0) free_reg(address);
            if (work[0] >= 0) free_reg(work[0]);
            if (work[1] >= 0) free_reg(work[1]);

            for (int i = 0; i < count; i++)
                free_reg(indices[i]);

            free_reg(rd);
            return;
        }

        // Calculate the element address and store the value.
        int ok = cg_array_address(
            &arithmetic_context,
            array,
            indices,
            count,
            address,
            work
        );

        if (ok) {
            ok = cg_array_store(
                &arithmetic_context,
                array,
                indices,
                count,
                address,
                rd,
                work
            );
        }

        // Release temporary registers.
        for (int i = 0; i < count; i++)
            free_reg(indices[i]);

        free_reg(address);
        free_reg(work[0]);
        free_reg(work[1]);
        free_reg(rd);

        if (!ok)
            report_error(target, "no se pudo generar la asignacion al arreglo");

        return;
    }

    report_error(target, "destino de asignacion no valido");
    free_reg(rd);
}

static void emit_conditional(ASTNode *node) {
    ASTNode *condition = NULL;
    ASTNode *body = NULL;
    ASTNode *alternatives = NULL;
    ASTNode *residual = NULL;

    // Find the condition and body by kind-independent position.
    // The constructors omit NULL children, so optional nodes
    // must be identified by their kinds.
    for (ASTNode *c = node->child;
         c != NULL;
         c = c->next) {

        if (condition == NULL &&
            c->kind != AST_BLOCK &&
            c->kind != AST_ALTERNATIVES &&
            c->kind != AST_RESIDUAL) {

            condition = c;
        }
        else if (c->kind == AST_BLOCK && body == NULL) {
            body = c;
        }
        else if (c->kind == AST_ALTERNATIVES) {
            alternatives = c;
        }
        else if (c->kind == AST_RESIDUAL) {
            residual = c;
        }
    }

    if (condition == NULL || body == NULL) {
        report_error(node,
                     "estructura condicional incompleta");
        return;
    }

    char end_label[64];
    char next_label[64];

    if (!node_label(end_label, sizeof(end_label), node, LABEL_END,
                    "fin del C") ||
        !node_label(next_label, sizeof(next_label), node, LABEL_NEXT,
                    "siguiente rama del C"))
        return;

    int rc = emit_expression(condition);

    if (rc < 0)
        return;

    cg_op(SLOT_BRU,
          "beq %s, rz, %s",
          reg_name(rc),
          next_label);

    free_reg(rc);

    emit_block(body);

    // El salto al final del C es del C, no de la última sentencia del cuerpo.
    cg_set_line(node->line);

    cg_op(SLOT_BRU,
          "jef %s",
          end_label);

    cg_label(next_label);

    // Each A [condition] alternative is checked in order.
    if (alternatives != NULL) {

        for (ASTNode *alt = alternatives->child;
             alt != NULL;
             alt = alt->next) {

            if (alt->kind != AST_ALTERNATIVE)
                continue;

            ASTNode *alt_condition = child_at(alt, 0);
            ASTNode *alt_body = child_at(alt, 1);

            if (alt_condition == NULL || alt_body == NULL) {
                report_error(alt,
                             "alternativa incompleta");
                continue;
            }

            char following[64];

            if (!node_label(following, sizeof(following), alt, LABEL_NEXT,
                            "siguiente rama del A"))
                continue;

            cg_set_line(alt->line);

            rc = emit_expression(alt_condition);

            if (rc < 0)
                continue;

            cg_op(SLOT_BRU,
                  "beq %s, rz, %s",
                  reg_name(rc),
                  following);

            free_reg(rc);

            emit_block(alt_body);

            cg_set_line(alt->line);

            cg_op(SLOT_BRU,
                  "jef %s",
                  end_label);

            cg_label(following);
        }
    }

    // The residual branch executes if no preceding condition matched.
    if (residual != NULL) {
        ASTNode *residual_body = child_at(residual, 0);

        if (residual_body != NULL)
            emit_block(residual_body);
    }

    cg_label(end_label);
}

static void emit_loop_l(ASTNode *node) {
    ASTNode *condition = child_at(node, 0);
    ASTNode *body = child_at(node, 1);

    if (condition == NULL || body == NULL) {
        report_error(node, "ciclo L incompleto");
        return;
    }

    char start_label[64];
    char end_label[64];

    if (!node_label(start_label, sizeof(start_label), node, LABEL_START,
                    "inicio del L") ||
        !node_label(end_label, sizeof(end_label), node, LABEL_END,
                    "fin del L"))
        return;

    cg_label(start_label);

    int rc = emit_expression(condition);

    if (rc < 0)
        return;

    cg_op(SLOT_BRU,
          "beq %s, rz, %s",
          reg_name(rc),
          end_label);

    free_reg(rc);

    emit_block(body);

    cg_set_line(node->line);

    cg_op(SLOT_BRU,
          "jef %s",
          start_label);

    cg_label(end_label);
}

/*
 * Ciclo S [ var , inicio , fin , paso ] (calser_diseno.md §5.8.2):
 *   - inicio y fin se evalúan una sola vez, al entrar;
 *   - fin es exclusivo;
 *   - paso es un literal distinto de cero (lo garantizan la gramática y el
 *     semántico): con paso positivo se sigue mientras var < fin, y con paso
 *     negativo mientras var > fin.
 *
 * Los valores viven en memoria entre sentencias y los registros solo se
 * usan dentro de cada una (libro §8.6): var está en su casilla, fin en la
 * casilla oculta que reservó el semántico (limit_address) y el paso va como
 * inmediato. Así ningún registro queda ocupado durante el cuerpo, y los S
 * anidados o las rutinas aritméticas dentro de un S tienen los 12
 * temporales disponibles.
 */
static void emit_loop_s(ASTNode *node) {
    ASTNode *id_node    = child_at(node, 0);
    ASTNode *start_node = child_at(node, 1);
    ASTNode *end_node   = child_at(node, 2);
    ASTNode *step_node  = child_at(node, 3);
    ASTNode *body       = child_at(node, 4);

    if (!id_node || id_node->kind != AST_IDENTIFIER ||
        !start_node || !end_node || !body ||
        !step_node || step_node->kind != AST_NUMBER ||
        step_node->number == 0) {
        report_error(node, "estructura del ciclo S invalida");
        return;
    }

    Symbol *loop_var = get_symbol(id_node);

    if (!loop_var || loop_var->type != TYPE_INT ||
        loop_var->kind == SYMBOL_FUNCTION) {
        report_error(id_node, "variable de control debe ser int");
        return;
    }

    if (loop_var->limit_address < 0) {
        report_error(id_node, "ciclo S sin casilla para su fin");
        return;
    }

    long step = step_node->number;
    int limit = loop_var->limit_address;

    // Al entrar: var = inicio y la casilla oculta = fin. Los dos se evalúan
    // antes de escribir var, porque fin puede usar un nombre igual de afuera.
    int start = emit_expression(start_node);

    if (start < 0)
        return;

    int end = emit_expression(end_node);

    if (end < 0) {
        free_reg(start);
        return;
    }

    emit_store_symbol(loop_var, start, id_node);
    cg_op(SLOT_LSU, "sw %s, %d(rz)", reg_name(end), limit);

    free_reg(start);
    free_reg(end);

    char test_label[64];
    char update_label[64];
    char end_label[64];

    if (!node_label(test_label, sizeof(test_label), node, LABEL_START,
                    "inicio del S") ||
        !node_label(update_label, sizeof(update_label), node, LABEL_UPDATE,
                    "incremento del S") ||
        !node_label(end_label, sizeof(end_label), node, LABEL_END,
                    "fin del S"))
        return;

    cg_label(test_label);

    // Cada vuelta: se cargan var y fin y se compara con signo (isa.md §6.2,
    // bge). Se sale cuando ya no se cumple la condición de seguir:
    //   paso > 0: sigue si var < fin, sale si var >= fin
    //   paso < 0: sigue si var > fin, sale si fin >= var
    int current = alloc_reg();
    int bound = current >= 0 ? alloc_reg() : -1;

    if (current < 0 || bound < 0) {
        report_error(node, "sin registros para generar el ciclo S");

        if (current >= 0)
            free_reg(current);

        return;
    }

    if (emit_load_symbol(current, loop_var, id_node) != 0) {
        free_reg(current);
        free_reg(bound);
        return;
    }

    cg_op(SLOT_LSU, "lw %s, %d(rz)", reg_name(bound), limit);

    if (step > 0)
        cg_op(SLOT_BRU, "bge %s, %s, %s",
              reg_name(current), reg_name(bound), end_label);
    else
        cg_op(SLOT_BRU, "bge %s, %s, %s",
              reg_name(bound), reg_name(current), end_label);

    // Ningún registro queda ocupado durante el cuerpo.
    free_reg(current);
    free_reg(bound);

    emit_block(body);

    // Inicio del incremento (rol UPDATE de la tabla). Es del S.
    cg_set_line(node->line);
    cg_label(update_label);

    // var = var + paso. El paso va como inmediato si cabe en 18 bits con
    // signo (isa.md §4.2); si no, se carga como constante de 32 bits (§4.3).
    current = alloc_reg();

    if (current < 0) {
        report_error(node, "sin registros para generar el ciclo S");
        return;
    }

    if (emit_load_symbol(current, loop_var, id_node) != 0) {
        free_reg(current);
        return;
    }

    if (step >= -131072 && step <= 131071) {
        cg_op(SLOT_ALU0, "saddi %s, %s, %ld",
              reg_name(current), reg_name(current), step);
    }
    else {
        int amount = alloc_reg();

        if (amount < 0 || emit_mov(amount, step, step_node) != 0) {
            if (amount < 0)
                report_error(node, "sin registros para generar el ciclo S");
            else
                free_reg(amount);

            free_reg(current);
            return;
        }

        cg_op(SLOT_ALU0, "add %s, %s, %s",
              reg_name(current), reg_name(current), reg_name(amount));

        free_reg(amount);
    }

    emit_store_symbol(loop_var, current, id_node);

    free_reg(current);

    cg_op(SLOT_BRU, "jef %s", test_label);
    cg_label(end_label);
}

static int inside_main;
 
// `E` de main termina el programa (contrato §3):
//   fin_main: { nopi | nopi | jef fin_main | nopi }
static void emit_end_of_main(void) {
    char jump[96];

    snprintf(jump, sizeof(jump), "jef %s", program_end_label);

    cg_label(program_end_label);
    cg_bundle(NULL, NULL, jump, NULL);
}

static void emit_return(ASTNode *node) {
    (void)node;

    if (inside_main) {
        cg_op(SLOT_BRU, "jef %s", program_end_label);
    } else {
        // jefR rz, rl:
        // jump to the address in rl without replacing rl.
        // Verify the return-link convention against isa.md.
        cg_op(SLOT_BRU, "jefR rz, rl");
    }
}

static void emit_rewind(ASTNode *node) {
    if (node == NULL) {
        report_error(NULL, "sentencia R invalida");
        return;
    }

    // AST_REWIND:
    //   child 0: AST_IDENTIFIER (function name)
    //   child 1: AST_ARGUMENTS (optional)
    ASTNode *target_id = child_at(node, 0);
    ASTNode *args_node = child_at(node, 1);

    if (target_id == NULL || target_id->text == NULL) {
        report_error(node,
                     "sentencia R sin identificador de funcion");
        return;
    }

    // R is restricted to the current function.
    if (current_function_name == NULL ||
        strcmp(target_id->text, current_function_name) != 0) {

        report_error(node,
                     "R debe referirse a la funcion actual");
        return;
    }

    if (current_function_symbol == NULL) {
        report_error(node,
                     "funcion actual sin simbolo semantico");
        return;
    }

    // Obtain the argument list.
    // The AST_ARGUMENTS node contains the expressions as a
    // linked list through child/next.
    // If the function has no arguments, args_node may be NULL.
    ASTNode *arg = NULL;

    if (args_node != NULL) {
        if (args_node->kind == AST_ARGUMENTS) {
            arg = args_node->child;
        } else {
            arg = args_node;
        }
    }

    // Count arguments.
    int arg_count = 0;

    for (ASTNode *a = arg; a != NULL; a = a->next) {
        arg_count++;
    }

    // Validate the number of arguments.
    int param_count = current_function_symbol->parameter_count;

    if (arg_count != param_count) {
        report_error(node,
                     "cantidad incorrecta de argumentos en R");
        return;
    }

    // Every argument must keep its value until all arguments have been evaluated.
    // This implementation supports up to TEMP_COUNT arguments.
    if (arg_count > TEMP_COUNT) {
        report_error(node,
                     "demasiados argumentos para R");
        return;
    }

    // Validate the parameter list before emitting any stores.
    // This prevents discovering a missing parameter halfway
    // through updating the function's parameters.
    if (param_count > 0 &&
        current_function_symbol->param_list == NULL) {

        report_error(node,
                     "funcion sin lista de parametros");
        return;
    }

    for (int i = 0; i < param_count; i++) {
        Symbol *param = current_function_symbol->param_list[i];

        if (param == NULL) {
            report_error(node,
                         "parametro invalido en la tabla de simbolos");
            return;
        }

        if (param->address < 0 || param->address > 131071) {
            report_error(node,
                         "direccion de parametro invalida en R");
            return;
        }
    }

    // Evaluate every argument.
    // Do not update parameters yet.
    int arg_regs[TEMP_COUNT];
    int evaluated = 0;

    for (ASTNode *a = arg; a != NULL; a = a->next) {
        int reg = emit_expression(a);

        if (reg < 0) {
            // Release the registers allocated for earlier arguments.
            for (int i = 0; i < evaluated; i++) {
                free_reg(arg_regs[i]);
            }

            return;
        }

        arg_regs[evaluated++] = reg;
    }

    // Write the new values to the parameter addresses.
    // Arguments and parameters correspond by declaration order.
    for (int i = 0; i < param_count; i++) {
        Symbol *param = current_function_symbol->param_list[i];

        if (emit_store_symbol(param, arg_regs[i], node) != 0) {
            for (int j = 0; j < evaluated; j++) {
                free_reg(arg_regs[j]);
            }

            return;
        }
    }

    // Release the temporary registers.
    for (int i = 0; i < evaluated; i++) {
        free_reg(arg_regs[i]);
    }

    // Restart the current function body.
    // Do not modify rl: R is a restart/tail-call operation,
    // not a new function call.
    char body_label[128];

    if (!node_label(body_label, sizeof(body_label),
                    (ASTNode *) current_function_symbol->decl_node,
                    LABEL_BODY, "cuerpo de la funcion"))
        return;

    cg_op(SLOT_BRU, "jef %s", body_label);
}

static void emit_statement(ASTNode *node) {
    if (node == NULL)
        return;

    // .asm anotado: los bundles de esta sentencia llevan su línea.
    cg_set_line(node->line);

    switch (node->kind) {

        case AST_BLOCK:
            emit_block(node);
            break;

        case AST_ASSIGNMENT:
            emit_assignment(node);
            break;

        case AST_DECLARATION:
            initialize_declaration(node, 0, 0);
            break;

        case AST_CONDITIONAL:
            emit_conditional(node);
            break;

        case AST_LOOP_L:
            emit_loop_l(node);
            break;

        case AST_EMIT:
            emit_return(node);
            break;

        case AST_CALL_STATEMENT: {
            ASTNode *call = node->child;

            if (call == NULL || call->kind != AST_CALL) {
                report_error(node, "llamada como sentencia invalida");
                
            } else {
                emit_function_call(call, 0);
            }
            break;
        }

        case AST_REWIND:
            emit_rewind(node);
            break;

        case AST_LOOP_S:
            emit_loop_s(node);
            break;

        case AST_CRYPTO:
            emit_crypto_statement(node);
            break;

        default:
            report_error(node,
                         "tipo de sentencia no soportado");
            break;
    }
}

static void emit_function(ASTNode *function) {
    ASTNode *id_node = find_child(function, AST_IDENTIFIER);
    const char *name = id_node ? id_node->text : NULL;

    if (name == NULL) {
        report_error(function, "funcion sin identificador");
        return;
    }

    current_function_name = name;
    current_function_symbol = get_symbol(id_node);

    char entry_label[128];
    char body_label[128];

    if (!node_label(entry_label, sizeof(entry_label), function,
                    LABEL_FUNCTION, "entrada de la funcion") ||
        !node_label(body_label, sizeof(body_label), function,
                    LABEL_BODY, "cuerpo de la funcion"))
        return;

    cg_set_line(function->line);
    cg_label(entry_label);

    // Prólogo (fase4_contrato_comun.md §3): el retorno empieza en 0 en cada
    // llamada. Va antes de .L_body_: R salta al cuerpo y conserva el valor.
    // main es void y no tiene retorno.
    Symbol *return_symbol = get_return_symbol(current_function_symbol);

    if (return_symbol != NULL)
        cg_op(SLOT_LSU, "sw rz, %d(rz)", return_symbol->address);

    cg_label(body_label);

    ASTNode *body = find_child(function, AST_BLOCK);

    if (body == NULL) {
        report_error(function, "funcion sin cuerpo");
        return;
    }

    reset_registers();

    inside_main = strcmp(name, "main") == 0;
    if (inside_main)
        for (ASTNode *g = program_globals ? program_globals->child : NULL; g; g = g->next) {
            cg_set_line(g->line);
            initialize_declaration(g, 0, 1);
        }

    emit_block(body);

    // Salto final, por si el control llega al final del cuerpo: el semántico
    // exige que la función contenga un E, no que termine en uno. Si la
    // última sentencia ya es un E, ese E emitió el mismo salto y no se repite.
    ASTNode *last = body->child;

    while (last != NULL && last->next != NULL)
        last = last->next;

    if (last == NULL || last->kind != AST_EMIT) {

        // El salto final no corresponde a ninguna sentencia.
        cg_set_line(0);

        if (inside_main) {
            cg_op(SLOT_BRU, "jef %s", program_end_label);
        } else {
            cg_op(SLOT_BRU, "jefR rz, rl");
        }
    }

    inside_main = 0;

    current_function_name = NULL;
    current_function_symbol = NULL;
}
 
int generate_code(ASTNode *root, const LabelTable *labels) {
    if (root == NULL || root->kind != AST_PROGRAM)
        return 1;

    // La tabla de etiquetas viene de la Fase 3; aquí solo se consulta.
    label_table = labels;
    arith_error_label = label_table_get(labels, NULL, LABEL_ARITH_ERROR);
    program_end_label = label_table_get(labels, NULL, LABEL_PROGRAM_END);

    if (arith_error_label == NULL || program_end_label == NULL) {
        fprintf(stderr,
                "ccom: error interno: faltan las etiquetas globales "
                "en la tabla\n");
        return 1;
    }
 
    cg_init();
    crypto_context = (CGCryptoContext) {.eval = emit_crypto_argument, .next_label = 1};
    crypto_context.user = &crypto_context;
    program_globals = find_child(root, AST_GLOBALS);

    arithmetic_context.emit = emit_arith_callback;
    arithmetic_context.user = NULL;
    arithmetic_context.next_label = 0;

    generation_errors = 0;
    inside_main = 0;

    current_function_name = NULL;
    current_function_symbol = NULL;

    reset_registers();
    for (ASTNode *g = program_globals ? program_globals->child : NULL; g; g = g->next)
        initialize_declaration(g, 1, 0);
    // Puntero de la pila de llamadas: primera dirección libre de la zona
    // de ccom (ver el comentario de la pila al inicio del archivo).
    cg_op(SLOT_ALU0,
          "mov rx, %d",
          semantic_data_end());
 
    ASTNode *functions = find_child(root, AST_FUNCTIONS);
    ASTNode *main_function = NULL;
 
    for (ASTNode *f = functions ? functions->child : NULL; f != NULL; f = f->next) {
        const char *name = function_name(f);
 
        if (name != NULL && strcmp(name, "main") == 0)
            main_function = f;
    }
 
    if (main_function == NULL) {
        fprintf(stderr, "ccom: error interno: no hay 'main'\n");
        return 1;
    }
 
    emit_function(main_function);

    for (ASTNode *f = functions ? functions->child : NULL;
         f != NULL;
         f = f->next) {

        const char *name = function_name(f);

        if (name != NULL && strcmp(name, "main") == 0)
            continue;

        emit_function(f);
    }

    cg_set_line(0);
    emit_end_of_main();

    cg_label(arith_error_label);
    cg_op(SLOT_BRU, "jef %s", arith_error_label);

    return generation_errors == 0 ? 0 : 1;
}
