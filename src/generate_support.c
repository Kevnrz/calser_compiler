/*
 * generate_support.c — Fase 4: diagnósticos, registros, memoria, pila y etiquetas.
 *
 * Reporta errores de generación, reserva los temporales t0–t11, carga y guarda
 * variables en sus casillas estáticas, construye constantes de 32 bits,
 * maneja la pila de llamadas y copia etiquetas de la tabla de la Fase 3.
 *
 * Usa: generate_internal.h, codegen.h, label_table.h.
 * Lo usa: generate.c, generate_expr.c, generate_stmt.c y generate_init.c.
 * Referencias: libro §7.1.1, §7.2; isa.md §4.3, §8.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "generate_internal.h"
#include "codegen.h"


/*
 * Pila de llamadas (libro §7.2): guarda las direcciones de retorno (rl), los
 * argumentos y los temporales vivos de las llamadas anidadas. Las variables
 * siguen con asignación estática (libro §7.1.1); solo esto usa una pila.
 *
 * rx es un temporal libre según isa.md §8; aquí se usa por convención como
 * puntero de pila. La pila crece hacia arriba (gen_stack_push).
 *
 * Vive en la zona de ccom (0x0000–0x0FFF): empieza en la primera dirección
 * libre después de la última área que asignó el semántico
 * (data_area_end) y llega hasta 0x0FFF. No puede estar desde 0x1000,
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

/* ==========================================================================
 * DIAGNÓSTICOS, REGISTROS, MEMORIA Y PILA
 * ======================================================================= */


/* gen_report_error — véase el contrato en generate_internal.h. */
void gen_report_error(GenCtx *gen, ASTNode *node, const char *message) {
    const char *source_name = gen->source_name ? gen->source_name : "ccom";
    if (node != NULL && node->line > 0) {
        fprintf(stderr,
                "%s:%d:%d: error de generacion: %s\n",
                source_name,
                node->line,
                node->column,
                message);
    } else {
        fprintf(stderr,
                "%s: error de generacion: %s\n",
                source_name,
                message);
    }

    gen->generation_errors++;
}

/* gen_child_at — véase el contrato en generate_internal.h. */
ASTNode *gen_child_at(ASTNode *node, int index) {
    ASTNode *child = node ? node->child : NULL;

    while (child != NULL && index > 0) {
        child = child->next;
        index--;
    }

    return child;
}

/* gen_reg_name — véase el contrato en generate_internal.h. */
const char *gen_reg_name(int reg) {
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

/* gen_reset_registers — véase el contrato en generate_internal.h. */
void gen_reset_registers(GenCtx *gen) {
    memset(gen->temp_used, 0, sizeof(gen->temp_used));
}

/* gen_alloc_reg — véase el contrato en generate_internal.h. */
int gen_alloc_reg(GenCtx *gen) {
    for (int i = 0; i < TEMP_COUNT; i++) {
        if (!gen->temp_used[i]) {
            gen->temp_used[i] = 1;
            return TEMP_FIRST + i;
        }
    }

    return -1;
}

/* gen_free_reg — véase el contrato en generate_internal.h. */
void gen_free_reg(GenCtx *gen, int reg) {
    if (reg >= TEMP_FIRST &&
        reg < TEMP_FIRST + TEMP_COUNT) {

        gen->temp_used[reg - TEMP_FIRST] = 0;
    }
}

/* gen_mov — véase el contrato en generate_internal.h. */
int gen_mov(GenCtx *gen, int rd, long immediate, ASTNode *node) {
    /* 2147483648 aparece como hijo del menos unario de INT32_MIN. */
    if (immediate < INT32_MIN || immediate > (long) UINT32_MAX) {
        gen_report_error(gen, node, "constante fuera de 32 bits");
        return -1;
    }
    int64_t value = immediate;
    if (value > INT32_MAX)
        value -= INT64_C(4294967296);
    if (value >= -131072 && value <= 131071) {
        cg_op(SLOT_ALU0, "mov %s, %ld", gen_reg_name(rd), (long) value);
        return 0;
    }
    int scratch = gen_alloc_reg(gen);
    if (scratch < 0) {
        gen_report_error(gen, node, "sin registro para construir constante de 32 bits");
        return -1;
    }
    int64_t high = value / 32768, low = value % 32768;
    if (low < 0) { high--; low += 32768; }
    char first[64], second[64];
    snprintf(first, sizeof(first), "mov %s, %ld", gen_reg_name(rd), (long) high);
    snprintf(second, sizeof(second), "mov %s, 15", gen_reg_name(scratch));
    cg_bundle(first, second, NULL, NULL);
    cg_gap();
    cg_op(SLOT_ALU0, "sll %s, %s, %s", gen_reg_name(rd), gen_reg_name(rd), gen_reg_name(scratch));
    cg_op(SLOT_ALU0, "saddi %s, %s, %ld", gen_reg_name(rd), gen_reg_name(rd), (long) low);
    gen_free_reg(gen, scratch);
    return 0;
}

/* gen_get_symbol — véase el contrato en generate_internal.h. */
Symbol *gen_get_symbol(ASTNode *node) {
    if (node == NULL || node->symbol == NULL)
        return NULL;

    return (Symbol *)node->symbol;
}

/* gen_get_return_symbol — véase el contrato en generate_internal.h. */
Symbol *gen_get_return_symbol(Symbol *function_symbol) {
    if (function_symbol == NULL ||
        function_symbol->decl_node == NULL) {
        return NULL;
    }

    ASTNode *function_node =
        (ASTNode *)function_symbol->decl_node;

    ASTNode *return_type =
        ast_find_child(function_node, AST_RETURN_TYPE);

    if (return_type == NULL)
        return NULL;

    ASTNode *return_id =
        ast_find_child(return_type, AST_IDENTIFIER);

    if (return_id == NULL)
        return NULL;

    return gen_get_symbol(return_id);
}

/* gen_load_symbol — véase el contrato en generate_internal.h. */
int gen_load_symbol(GenCtx *gen, int rd, Symbol *symbol, ASTNode *node) {
    if (symbol == NULL) {
        gen_report_error(gen, node,
                     "identificador sin simbolo semantico");
        return -1;
    }

    if (symbol->address < 0) {
        gen_report_error(gen, node,
                     "simbolo sin direccion de memoria");
        return -1;
    }

    if (symbol->address > 131071) {
        gen_report_error(gen, node,
                     "direccion fuera del rango de lw");
        return -1;
    }

    cg_op(SLOT_LSU,
          "lw %s, %d(rz)",
          gen_reg_name(rd),
          symbol->address);

    return 0;
}

/* gen_store_symbol — véase el contrato en generate_internal.h. */
int gen_store_symbol(GenCtx *gen, Symbol *symbol, int rs, ASTNode *node) {
    if (symbol == NULL) {
        gen_report_error(gen, node,
                     "destino sin simbolo semantico");
        return -1;
    }

    if (symbol->address < 0 ||
        symbol->address > 131071) {

        gen_report_error(gen, node,
                     "direccion de almacenamiento invalida");
        return -1;
    }

    cg_op(SLOT_LSU,
          "sw %s, %d(rz)",
          gen_reg_name(rs),
          symbol->address);

    return 0;
}

/* gen_stack_push — véase el contrato en generate_internal.h. */
void gen_stack_push(int reg) {
    cg_op(SLOT_LSU,
          "sw %s, 0(rx)",
          gen_reg_name(reg));

    cg_op(SLOT_ALU0,
          "saddi rx, rx, 4");
}

/* gen_stack_pop — véase el contrato en generate_internal.h. */
void gen_stack_pop(int reg) {
    cg_op(SLOT_ALU0,
          "saddi rx, rx, -4");

    cg_op(SLOT_LSU,
          "lw %s, 0(rx)",
          gen_reg_name(reg));
}

/* gen_node_label — véase el contrato en generate_internal.h. */
int gen_node_label(GenCtx *gen, char *buffer, size_t size, ASTNode *node,
                      LabelRole role, const char *what) {
    const char *name = label_table_get(gen->label_table, node, role);

    if (name == NULL || strlen(name) >= size) {
        char message[128];

        snprintf(message, sizeof(message),
                 "error interno: falta la etiqueta de %s en la tabla", what);

        gen_report_error(gen, node, message);

        return 0;
    }

    strcpy(buffer, name);

    return 1;
}
