#include <stdio.h>
#include <string.h>
 
#include "generate.h"
#include "codegen.h"
#include "symbol_table.h"
#include "codegen_arith.h"

#define TEMP_FIRST 4
#define TEMP_COUNT 12

// The static data image occupies offsets 0 through 0xFFF.
// Reserve runtime memory beginning at 0x1000 for the call stack.
// This address assumes the target's data memory extends beyond
// the static data image and that the static data base is address 0.
#define CALL_STACK_BASE CG_DATA_BYTES

#define ARITH_ERROR_LABEL "L_arith_error"

static CGArithContext arithmetic_context;

static int temp_used[TEMP_COUNT];
static int label_counter;
static int generation_errors;

static const char *current_function_name;
static Symbol *current_function_symbol;

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
        error_label = ARITH_ERROR_LABEL;
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
    // 18-bit signed immediate.
    if (immediate < -131072L || immediate > 131071L) {
        report_error(node,
                     "constante fuera del rango de mov");
        return -1;
    }

    cg_op(SLOT_ALU0,
          "mov %s, %ld",
          reg_name(rd),
          immediate);

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
          "lw %s, %d(rg)",
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
          "sw %s, %d(rg)",
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
static int emit_function_call(ASTNode *node);

static int emit_binary_expression(ASTNode *node) {
    ASTNode *left = child_at(node, 0);
    ASTNode *right = child_at(node, 1);

    if (left == NULL || right == NULL) {
        report_error(node,
                     "expresion binaria incompleta");
        return -1;
    }

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
        case AST_AND:
            op = "and";
            break;
        case AST_OR:
            op = "or";
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

        case AST_CALL:
            report_error(node,
                         "llamadas en expresiones aun no implementadas");
            return -1;

        case AST_INDEX:
            report_error(node,
                         "acceso a arreglos aun no implementado");
            return -1;

        default:
            report_error(node,
                         "nodo no valido como expresion");
            return -1;
    }
}

static int emit_function_call(ASTNode *node) {
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

    // Locate the function's return variable.
    if (callee->type == TYPE_VOID) {
        report_error(node,
                     "una funcion void no puede usarse como expresion");
        return -1;
    }

    Symbol *return_symbol = get_return_symbol(callee);

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
    for (ASTNode *a = arg; a != NULL; a = a->next) {
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

        emit_stack_pop(reg);

        Symbol *param = callee->param_list[i];

        int store_result = emit_store_symbol(param, reg, node);

        free_reg(reg);

        if (store_result != 0) {
            for (int j = 0; j < saved_count; j++) {
                free_reg(saved_regs[j]);
            }

            return -1;
        }
    }

    // The argument values have now been removed from the stack.
    // Only the caller's saved temporaries remain.
    // Save the old return link before jas overwrites rl.
    emit_stack_push(1);  // rl

    // Call the function.
    // jas sets rl to the return address.
    cg_op(SLOT_BRU,
          "jas rl, .L_func_%s",
          callee->name);

    // The callee's E returns through the new rl.
    // Restore the caller's previous rl after returning.
    emit_stack_pop(1);

    // Restore live caller temporaries in reverse order.
    for (int i = saved_count - 1; i >= 0; i--) {
        emit_stack_pop(saved_regs[i]);
    }

    // Allocate a new register for the result.
    // Existing caller temporaries are still marked as occupied,
    // so the result will not overwrite them.
    int result_reg = alloc_reg();

    if (result_reg < 0) {
        report_error(node,
                     "sin registros para el resultado de la llamada");
        return -1;
    }

    // Load the result immediately after the call. A later call
    // could overwrite this function's static return variable.
    if (emit_load_symbol(result_reg, return_symbol, node) != 0) {
        free_reg(result_reg);
        return -1;
    }

    return result_reg;
}

static void make_label(char *buffer, size_t size, const char *prefix) {
    snprintf(buffer,
             size,
             ".L_%s_%d",
             prefix,
             label_counter++);
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

    // only simple scalar assignments.
    if (target->kind != AST_IDENTIFIER) {
        report_error(target,
                     "solo se permiten destinos escalares por ahora");
        return;
    }

    int rd = emit_expression(value);

    if (rd < 0)
        return;

    emit_store_symbol(get_symbol(target), rd, target);

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

    make_label(end_label, sizeof(end_label), "if_end");

    int rc = emit_expression(condition);

    if (rc < 0)
        return;

    make_label(next_label, sizeof(next_label), "if_next");

    cg_op(SLOT_BRU,
          "beq %s, rz, %s",
          reg_name(rc),
          next_label);

    free_reg(rc);

    emit_block(body);

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

            make_label(following,
                       sizeof(following),
                       "alt_next");

            rc = emit_expression(alt_condition);

            if (rc < 0)
                continue;

            cg_op(SLOT_BRU,
                  "beq %s, rz, %s",
                  reg_name(rc),
                  following);

            free_reg(rc);

            emit_block(alt_body);

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

    make_label(start_label, sizeof(start_label), "loop_start");
    make_label(end_label, sizeof(end_label), "loop_end");

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

    cg_op(SLOT_BRU,
          "jef %s",
          start_label);

    cg_label(end_label);
}

static int inside_main;
 
// `E` de main termina el programa (contrato §3):
//   fin_main: { nopi | nopi | jef fin_main | nopi }
static void emit_end_of_main(void) {
    cg_label("fin_main");
    cg_bundle(NULL, NULL, "jef fin_main", NULL);
}

static void emit_return(ASTNode *node) {
    (void)node;

    if (inside_main) {
        cg_op(SLOT_BRU, "jef fin_main");
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
    cg_op(SLOT_BRU,
          "jef .L_body_%s",
          current_function_name);
}

static void emit_statement(ASTNode *node) {
    if (node == NULL)
        return;

    switch (node->kind) {

        case AST_BLOCK:
            emit_block(node);
            break;

        case AST_ASSIGNMENT:
            emit_assignment(node);
            break;

        case AST_DECLARATION:
            /*
             * Declarations without initializers require no
             * instructions if storage is statically allocated.
             *
             * Initializer handling will be added separately.
             */
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

        case AST_CALL_STATEMENT:
            return emit_function_call(node);
            break;

        case AST_REWIND:
            emit_rewind(node);
            break;

        case AST_LOOP_S:
            report_error(node,
                         "ciclo S aun no implementado");
            break;

        case AST_CRYPTO:
            report_error(node,
                         "operaciones criptograficas aun no implementadas");
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

    snprintf(entry_label,
             sizeof(entry_label),
             ".L_func_%s",
             name);

    snprintf(body_label,
             sizeof(body_label),
             ".L_body_%s",
             name);

    cg_label(entry_label);
    cg_label(body_label);

    ASTNode *body = find_child(function, AST_BLOCK);

    if (body == NULL) {
        report_error(function, "funcion sin cuerpo");
        return;
    }

    reset_registers();

    inside_main = strcmp(name, "main") == 0;

    emit_block(body);

    // If control reaches the end of main, terminate the program.
    // Other functions need a well-defined fall-through policy.
    if (inside_main) {
        cg_op(SLOT_BRU, "jef fin_main");
    } else {
        // This assumes falling off the end of a function
        // should return to the caller.
        cg_op(SLOT_BRU, "jefR rz, rl");
    }

    inside_main = 0;

    current_function_name = NULL;
    current_function_symbol = NULL;
}
 
int generate_code(ASTNode *root) {
    if (root == NULL || root->kind != AST_PROGRAM)
        return 1;
 
    cg_init();

    arithmetic_context.emit = emit_arith_callback;
    arithmetic_context.user = NULL;
    arithmetic_context.next_label = 0;

    label_counter = 0;
    generation_errors = 0;
    inside_main = 0;

    current_function_name = NULL;
    current_function_symbol = NULL;

    reset_registers();
    // Initialize return-link stack pointer
    cg_op(SLOT_ALU0,
          "mov rx, %d",
          CALL_STACK_BASE);
 
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

    emit_end_of_main();

    cg_label(ARITH_ERROR_LABEL);
    cg_op(SLOT_BRU, "jef %s", ARITH_ERROR_LABEL);

    return generation_errors == 0 ? 0 : 1;
}
