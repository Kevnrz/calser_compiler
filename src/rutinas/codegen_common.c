/*
 * codegen_common.c — rutinas de máquina compartidas por O0 y O1.
 * Usa: codegen_common.h, codegen.h e isa_table.h; biblioteca C.
 * Lo usan los dos emisores y las plantillas CRYPTO.
 * Referencias: Aho §7.2 y §8.6–8.8; isa.md §6 y §9.
 */
#include "codegen_common.h"
#include "codegen.h"
#include "isa_table.h"
#include <stdio.h>
#include <string.h>

/* cg_stack_push — véase el contrato en codegen_common.h. */
void cg_stack_push(int reg) {
    cg_op(SLOT_LSU,
          "sw %s, 0(rx)",
          isa_register_name(reg));

    cg_op(SLOT_ALU0,
          "saddi rx, rx, 4");
}

/* cg_stack_pop — véase el contrato en codegen_common.h. */
void cg_stack_pop(int reg) {
    cg_op(SLOT_ALU0,
          "saddi rx, rx, -4");

    cg_op(SLOT_LSU,
          "lw %s, 0(rx)",
          isa_register_name(reg));
}

/*
 * cg_emit_arith_op — adapta operaciones aritméticas y de arreglos al emisor.
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
int cg_emit_arith_op(void *user, const CGArithOp *op) {
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

/* cg_preserve — véase el contrato en codegen_common.h. */
int cg_preserve(const FrameLayout *frame, uint16_t mask, int restore) {
    if (!mask)
        return 1;
    if (!frame || (mask & ~REG_CALL_CLOBBERS) || frame->area_base < 0 ||
        frame->area_size < 4 * REG_TEMP_COUNT || frame->temp_base < frame->area_base ||
        frame->temp_base % 4)
        return 0;
    int64_t end = (int64_t)frame->area_base + frame->area_size;
    if ((int64_t)frame->temp_base + 4 * REG_TEMP_COUNT > end || end > CG_DATA_BYTES)
        return 0;
    if (!restore)
        cg_gap(); /* La última expresión puede haber escrito un registro vivo. */
    for (int r = REG_TEMP_FIRST; r <= REG_TEMP_LAST; r++)
        if (mask & (1u << r))
            cg_op(SLOT_LSU, "%s r%d, %d(rz)", restore ? "lw" : "sw", r,
                  frame->temp_base + 4 * (r - REG_TEMP_FIRST));
    return 1;
}

/* cg_program_end — véase el contrato en codegen_common.h. */
void cg_program_end(const char *label) {
    char jump[128];
    snprintf(jump, sizeof jump, "jef %s", label);
    cg_label(label);
    cg_bundle(NULL, NULL, jump, NULL);
}

/* cg_stack_init — véase el contrato en codegen_common.h. */
void cg_stack_init(int stack_base) {
    cg_op(SLOT_ALU0, "mov rx, %d", stack_base);
}
