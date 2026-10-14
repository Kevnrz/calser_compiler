/*
 * codegen_crypto.c — Fase 4: expansión de operaciones de cifrado.
 *
 * Preserva temporales vivos antes de evaluar argumentos. Emite plantillas
 * de bundles fijos para bóveda, autenticación y cuatro rondas Feistel.
 *
 * Usa: codegen.h, AST anotado y evaluador del llamador.
 * Lo usa: generate.c y pruebas/crypto.
 * Referencias: isa.md §5.5, §6.4, §7, §9.3; docs/crypto_para_ccom.md.
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "codegen.h"
#include "codegen_crypto.h"

#define TEMP_MASK 0xFFF0u

/* ==========================================================================
 * VALIDACIÓN Y VALORES VIVOS
 * ======================================================================= */


/*
 * selector — comprueba un selector literal de cifrado.
 *
 * Entrada:
 *   node  nodo o NULL.
 * Salida:
 *   1 si AST_NUMBER entre 0 y 3; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int selector(const ASTNode *node) {
    return node != NULL && node->kind == AST_NUMBER &&
           node->number >= 0 && node->number <= 3;
}

/*
 * valid_area — comprueba las casillas de temporales vivos.
 *
 * Entrada:
 *   ctx  contexto no NULL.
 * Salida:
 *   1 si área válida o no hay vivos; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   48 bytes alineados dentro del área y de CG_DATA_BYTES cuando live_mask
 *   no es cero.
 */
static int valid_area(const CGCryptoContext *ctx) {
    if (ctx->live_mask == 0)
        return 1;
    const Symbol *f = ctx->function;
    if (f == NULL || f->kind != SYMBOL_FUNCTION || f->area_base < 0 ||
        f->area_size < 48 || f->temp_base < f->area_base || f->temp_base % 4)
        return 0;
    int64_t end = (int64_t) f->area_base + f->area_size;
    return (int64_t) f->temp_base + 48 <= end && end <= CG_DATA_BYTES;
}

/*
 * save_live — guarda temporales vivos antes de evaluar argumentos.
 *
 * Entrada:
 *   ctx  contexto validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite separación y sw en casillas de temp_base.
 * Restricciones:
 *   live_mask solo bits ISA 4–15; área comprobada.
 */
static void save_live(const CGCryptoContext *ctx) {
    if (ctx->live_mask == 0)
        return;
    cg_gap();  /* La última expresión del llamador puede haber escrito un vivo. */
    for (int r = 4; r < 16; r++)
        if (ctx->live_mask & (1u << r))
            cg_op(SLOT_LSU, "sw r%d, %d(rz)", r, ctx->function->temp_base + 4 * (r - 4));
}

/*
 * restore_live — restaura temporales vivos después de la expansión.
 *
 * Entrada:
 *   ctx  contexto validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite lw desde casillas de temp_base.
 * Restricciones:
 *   las casillas guardadas no fueron sobrescritas por el evaluador.
 */
static void restore_live(const CGCryptoContext *ctx) {
    for (int r = 4; r < 16; r++)
        if (ctx->live_mask & (1u << r))
            cg_op(SLOT_LSU, "lw r%d, %d(rz)", r, ctx->function->temp_base + 4 * (r - 4));
}

/*
 * evaluate — evalúa un argumento y separa su resultado.
 *
 * Entrada:
 *   ctx  contexto con eval.
 *   node  expresión.
 *   dst  destino ISA 4–15.
 *   preserve_mask  máscara de registros a preservar.
 * Salida:
 *   0 si evalúa; 1 si callback falla.
 *   Efectos: invoca eval y emite cg_gap al aceptar.
 * Restricciones:
 *   evaluador no pisa casillas de live_mask; preservar argumentos previos.
 */
static int evaluate(CGCryptoContext *ctx, ASTNode *node, int dst,
                    unsigned preserve_mask) {
    if (ctx->eval(ctx->user, node, dst, preserve_mask) != 0)
        return 1;
    cg_gap(); /* Frontera segura incluso si el evaluador terminó con una escritura. */
    return 0;
}

/* ==========================================================================
 * PLANTILLAS DE CIFRADO
 * ======================================================================= */


/*
 * emit_loop — emite la plantilla de cuatro rondas Feistel por bloque.
 *
 * Entrada:
 *   start  etiqueta de inicio.
 *   end  etiqueta de fin.
 *   key  selector de llave 0–3.
 *   decrypt  1 para descifrar, 0 para cifrar.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite bundles fijos, separaciones y etiquetas.
 * Restricciones:
 *   t2/t3 contienen origen/destino, t5 cantidad; usa t0/t1/t4; bloques de
 *   ocho bytes.
 */
static void emit_loop(const char *start, const char *end, int key, int decrypt) {
    char branch[128], round[128];
    int lo = decrypt ? 4 : 0;
    int hi = decrypt ? 0 : 4;

    cg_bundle("mov t4, 0", NULL, NULL, NULL);
    cg_gap();
    snprintf(branch, sizeof(branch), "bge rz, t5, %s", end);
    cg_bundle(NULL, NULL, branch, NULL);
    cg_label(start);
    char load[64];
    snprintf(load, sizeof(load), "lw t0, %d(t2)", lo);
    cg_bundle(NULL, NULL, NULL, load);
    snprintf(load, sizeof(load), "lw t1, %d(t2)", hi);
    cg_bundle(NULL, NULL, NULL, load);
    cg_bundle("saddi t2, t2, 8", NULL, NULL, NULL);

    for (int i = 0; i < 4; i++) {
        int dst = i % 2;
        int subkey = decrypt ? 3 - i : i;
        snprintf(round, sizeof(round), "feis t%d, t%d, t%d, %d, %d",
                 dst, dst, 1 - dst, key, subkey);
        cg_bundle(NULL, round, NULL, NULL);
        if (i == 0)
            cg_bundle("saddi t4, t4, 1", NULL, NULL, NULL);
        else if (i < 3)
            cg_gap();
    }

    char store[64];
    snprintf(store, sizeof(store), "sw t0, %d(t3)", lo);
    cg_bundle(NULL, NULL, NULL, store);
    snprintf(store, sizeof(store), "sw t1, %d(t3)", hi);
    snprintf(branch, sizeof(branch), "blt t4, t5, %s", start);
    /* Los tres slots leen el destino/contador ANTERIOR a este bundle. */
    cg_bundle("saddi t3, t3, 8", NULL, branch, store);
    cg_label(end);
    cg_gap(); /* Separar t3 de la siguiente sentencia cuando el salto no se toma. */
}

/* cg_crypto — véase el contrato en codegen_crypto.h. */
int cg_crypto(CGCryptoContext *ctx, ASTNode *node) {
    if (ctx == NULL || node == NULL || node->kind != AST_CRYPTO || node->text == NULL ||
        (ctx->live_mask & ~TEMP_MASK) != 0 || !valid_area(ctx))
        return 1;

    ASTNode *args[4] = {NULL, NULL, NULL, NULL};
    int count = 0;
    if (node->child != NULL) {
        if (node->child->kind != AST_ARGUMENTS || node->child->next != NULL)
            return 1;
        for (ASTNode *a = node->child->child; a != NULL; a = a->next) {
            if (count == 4)
                return 1;
            args[count++] = a;
        }
    }

    const char *op = node->text;
    int encrypt = strcmp(op, "cifrar") == 0;
    int decrypt = strcmp(op, "descifrar") == 0;
    int load_key = strcmp(op, "cargar_llave") == 0;
    const char *simple = NULL;
    int expected;
    if (encrypt || decrypt) expected = 4;
    else if (load_key) expected = 3;
    else if (strcmp(op, "autenticar") == 0) { simple = "auth"; expected = 1; }
    else if (strcmp(op, "cambiar_ancla") == 0) { simple = "sanc"; expected = 1; }
    else if (strcmp(op, "cerrar_sesion") == 0) { simple = "logout"; expected = 0; }
    else if (strcmp(op, "sellar") == 0) { simple = "kv_lock"; expected = 0; }
    else return 1;

    if (count != expected || (expected != 0 && ctx->eval == NULL) ||
        (load_key && (!selector(args[0]) || !selector(args[1]))) ||
        ((encrypt || decrypt) && (!selector(args[3]) || ctx->next_label == UINT_MAX)))
        return 1;

    save_live(ctx);
    if (encrypt || decrypt) {
        if (evaluate(ctx, args[0], 6, 0) ||
            evaluate(ctx, args[1], 7, 1u << 6) ||
            evaluate(ctx, args[2], 9, (1u << 6) | (1u << 7)))
            return 1;
        char start[96], end[96];
        unsigned id = ctx->next_label++;
        snprintf(start, sizeof(start), ".L_crypto_%s_%u", decrypt ? "descifra" : "cifra", id);
        snprintf(end, sizeof(end), ".L_crypto_fin_%s_%u", decrypt ? "descifra" : "cifra", id);
        emit_loop(start, end, (int) args[3]->number, decrypt);
    }
    else if (expected == 0) {
        cg_bundle(NULL, simple, NULL, NULL);
    }
    else {
        if (evaluate(ctx, load_key ? args[2] : args[0], 10, 0))
            return 1;
        char instruction[96];
        if (load_key)
            snprintf(instruction, sizeof(instruction), "kv_write t6, %ld, %ld",
                     args[0]->number, args[1]->number);
        else
            snprintf(instruction, sizeof(instruction), "%s t6", simple);
        cg_bundle(NULL, instruction, NULL, NULL);
    }
    restore_live(ctx);
    return 0;
}
