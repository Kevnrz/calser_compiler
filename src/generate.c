#include <stdio.h>
#include <string.h>
 
#include "generate.h"
#include "codegen.h"
#include "symbol_table.h"

#define TEMP_FIRST 4
#define TEMP_COUNT 12

static int temp_used[TEMP_COUNT];
static int label_counter;
static int generation_errors;
 
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

static int emit_expression(ASTNode *node);

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
        case AST_POWER:
            report_error(node,
                         "operador requiere rutina de software");
            return -1;

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
        /*
         * jefR rz, rl:
         * jump to the address in rl without replacing rl.
         *
         * Verify the return-link convention against isa.md.
         */
        cg_op(SLOT_BRU, "jefR rz, rl");
    }
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
            report_error(node,
                         "llamadas como sentencia aun no implementadas");
            break;

        case AST_REWIND:
            report_error(node,
                         "R aun no implementado");
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
    const char *name = function_name(function);

    if (name == NULL) {
        report_error(function, "funcion sin identificador");
        return;
    }

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

    /*
     * If control reaches the end of main, terminate the program.
     * Other functions need a well-defined fall-through policy.
     */
    if (inside_main) {
        cg_op(SLOT_BRU, "jef fin_main");
    } else {
        /*
         * This assumes falling off the end of a function
         * should return to the caller.
         */
        cg_op(SLOT_BRU, "jefR rz, rl");
    }

    inside_main = 0;
}

 
// main no tiene prólogo (no guarda rl). TODO: cuerpo (expresiones, control).
static void emit_main(ASTNode *function) {
    (void) function;
 
    cg_label(".L_func_main");
    cg_label(".L_body_main");
 
    // TODO(Kevin, paso 2): generar las sentencias del cuerpo. Cada `E` de
    // main debe saltar a fin_main; por ahora el fin va al final.
    emit_end_of_main();
}
 
int generate_code(ASTNode *root) {
    if (root == NULL || root->kind != AST_PROGRAM)
        return 1;
 
    cg_init();

    label_counter = 0;
    generation_errors = 0;
    inside_main = 0;
 
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

    return generation_errors == 0 ? 0 : 1;
}
