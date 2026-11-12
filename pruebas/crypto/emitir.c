/* Evaluador auxiliar de prueba, no implementación de las expresiones de Kevin. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "codegen_crypto.h"

static int calls;
static int fail_eval;
static int eval_destinations[3];
static unsigned eval_preserves[3];
static ASTNode *eval_expressions[3];

static int evaluate(void *user, ASTNode *expr, int dst, unsigned preserve) {
    (void) user;
    calls++;
    if (calls <= 3) {
        eval_destinations[calls - 1] = dst;
        eval_preserves[calls - 1] = preserve;
        eval_expressions[calls - 1] = expr;
    }
    if ((fail_eval && calls == fail_eval) || dst < 4 || dst > 15 || (preserve & (1u << dst)))
        return 1;
    if (expr->kind == AST_IDENTIFIER) {
        Symbol *symbol = expr->symbol;
        if (symbol == NULL)
            return 1;
        cg_op(SLOT_LSU, "lw r%d, %d(rz)", dst, symbol->address);
        return 0;
    }
    if (expr->kind == AST_ADD && expr->child != NULL &&
        expr->child->next != NULL && expr->child->next->kind == AST_NUMBER) {
        if (evaluate(user, expr->child, dst, preserve))
            return 1;
        cg_op(SLOT_ALU0, "saddi r%d, r%d, %ld", dst, dst, expr->child->next->number);
        return 0;
    }
    long value;
    if (expr->kind == AST_NUMBER)
        value = expr->number;
    else if (expr->kind == AST_NEG && expr->child && expr->child->kind == AST_NUMBER)
        value = -expr->child->number;
    else return 1;
    if (value < INT32_MIN || value > INT32_MAX)
        return 1;
    if (value >= -131072 && value <= 131071) {
        cg_op(SLOT_ALU0, "mov r%d, %ld", dst, value);
        return 0;
    }
    int scratch;
    for (scratch = 4; scratch < 16; scratch++)
        if (scratch != dst && !(preserve & (1u << scratch)))
            break;
    if (scratch == 16)
        return 1;
    int64_t high = value / 32768;
    int64_t low = value % 32768;
    if (low < 0) { high--; low += 32768; }
    char a[64], b[64];
    snprintf(a, sizeof(a), "mov r%d, %ld", dst, (long) high);
    snprintf(b, sizeof(b), "mov r%d, 15", scratch);
    cg_bundle(a, b, NULL, NULL);
    cg_gap();
    cg_op(SLOT_ALU0, "sll r%d, r%d, r%d", dst, dst, scratch);
    cg_op(SLOT_ALU0, "saddi r%d, r%d, %ld", dst, dst, (long) low);
    return 0;
}

static int operation(CGCryptoContext *ctx, const char *name, ASTNode *args, int count) {
    for (int i = 0; i < count; i++)
        args[i].next = i + 1 < count ? &args[i + 1] : NULL;
    ASTNode list = {.kind = AST_ARGUMENTS, .child = count ? args : NULL};
    ASTNode node = {.kind = AST_CRYPTO, .text = (char *) name, .child = &list};
    return cg_crypto(ctx, &node);
}

static Symbol function = {.kind = SYMBOL_FUNCTION,
                          .frame = {.area_base = 32, .area_size = 80, .temp_base = 64}};

/*
 * fixed_slot — comprueba una instrucción dentro de un bundle indivisible.
 * Entrada:
 *   índice IR, slot y texto esperado.
 * Salida:
 *   ninguna; exige identidad de clase y texto.
 * Efectos:
 *   aborta si la plantilla separó o cambió el bundle.
 * Restricciones:
 *   consultar antes de la siguiente emisión, que puede mover el IR.
 */
static void fixed_slot(int index, unsigned slot, const char *text) {
    const CGIRRecord *record = cg_ir_record(index);
    assert(record && record->kind == CG_IR_FIXED_BUNDLE);
    assert(strcmp(record->data.slots[slot].text, text) == 0);
}

/*
 * loop_records — verifica la plantilla completa, incluidas rondas y bundle final.
 * Entrada:
 *   inicio en IR, llave, dirección de cifrado e identificador de etiquetas.
 * Salida:
 *   ninguna; exige registros, slots, separaciones y etiquetas exactos.
 * Efectos:
 *   solo consulta IR.
 * Restricciones:
 *   no genera la plantilla ni compara únicamente el ensamblador final.
 */
static void loop_records(int base, int key, int decrypt, unsigned id) {
    const CGIRKind kinds[] = {
        CG_IR_FIXED_BUNDLE, CG_IR_GAP, CG_IR_FIXED_BUNDLE, CG_IR_LABEL,
        CG_IR_FIXED_BUNDLE, CG_IR_FIXED_BUNDLE, CG_IR_FIXED_BUNDLE,
        CG_IR_FIXED_BUNDLE, CG_IR_FIXED_BUNDLE, CG_IR_FIXED_BUNDLE, CG_IR_GAP,
        CG_IR_FIXED_BUNDLE, CG_IR_GAP, CG_IR_FIXED_BUNDLE,
        CG_IR_FIXED_BUNDLE, CG_IR_FIXED_BUNDLE, CG_IR_LABEL, CG_IR_GAP
    };
    for (unsigned i = 0; i < sizeof kinds / sizeof kinds[0]; i++) {
        const CGIRRecord *record = cg_ir_record(base + (int)i);
        assert(record && record->kind == kinds[i]);
        if (record->kind == CG_IR_GAP)
            assert(record->data.gap_reason == CG_GAP_EXPLICIT);
        if (record->kind == CG_IR_FIXED_BUNDLE) {
            unsigned occupied = i == 0 || i == 6 || i == 8 ? 1u :
                                i == 2 ? 4u : i == 4 || i == 5 || i == 14 ? 8u :
                                i == 15 ? 13u : 2u;
            for (unsigned slot = 0; slot < 4; slot++) {
                if (!(occupied & (1u << slot)))
                    fixed_slot(base + (int)i, slot, "nopi");
            }
        }
    }
    char start[96], end[96], text[128];
    snprintf(start, sizeof start, ".L_crypto_%s_%u", decrypt ? "descifra" : "cifra", id);
    snprintf(end, sizeof end, ".L_crypto_fin_%s_%u", decrypt ? "descifra" : "cifra", id);
    assert(strcmp(cg_ir_record(base + 3)->data.label, start) == 0);
    assert(strcmp(cg_ir_record(base + 16)->data.label, end) == 0);
    fixed_slot(base, 0, "mov t4, 0");
    snprintf(text, sizeof text, "bge rz, t5, %s", end);
    fixed_slot(base + 2, 2, text);
    fixed_slot(base + 4, 3, decrypt ? "lw t0, 4(t2)" : "lw t0, 0(t2)");
    fixed_slot(base + 5, 3, decrypt ? "lw t1, 0(t2)" : "lw t1, 4(t2)");
    fixed_slot(base + 6, 0, "saddi t2, t2, 8");
    const int rounds[] = {7, 9, 11, 13};
    for (int i = 0; i < 4; i++) {
        snprintf(text, sizeof text, "feis t%d, t%d, t%d, %d, %d",
                 i % 2, i % 2, 1 - i % 2, key, decrypt ? 3 - i : i);
        fixed_slot(base + rounds[i], 1, text);
    }
    fixed_slot(base + 8, 0, "saddi t4, t4, 1");
    fixed_slot(base + 14, 3, decrypt ? "sw t0, 4(t3)" : "sw t0, 0(t3)");
    fixed_slot(base + 15, 0, "saddi t3, t3, 8");
    snprintf(text, sizeof text, "blt t4, t5, %s", start);
    fixed_slot(base + 15, 2, text);
    fixed_slot(base + 15, 3, decrypt ? "sw t1, 0(t3)" : "sw t1, 4(t3)");
}

/*
 * rejected_template — exige rechazo atómico de argumentos inválidos.
 * Entrada:
 *   descripción y contador, posiblemente NULL.
 * Salida:
 *   ninguna; exige error sin cambiar emisión ni contador.
 * Efectos:
 *   intenta emitir una plantilla inválida.
 * Restricciones:
 *   no requiere AST, callback ni área de guardado de vivos.
 */
static void rejected_template(const CGCryptoTemplate *operation, unsigned *next_label) {
    int records = cg_ir_count();
    int lines = cg_line_count();
    unsigned saved = next_label ? *next_label : 0;
    assert(cg_crypto_emit_template(operation, next_label) == 1);
    assert(cg_ir_count() == records && cg_line_count() == lines);
    assert(!next_label || *next_label == saved);
}

/*
 * template_contracts — ejercita las siete plantillas sin AST ni evaluador.
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige emisión exacta, validación y contador compartido.
 * Efectos:
 *   crea y libera flujos del emisor.
 * Restricciones:
 *   los valores se consideran preparados; aquí se comprueba la forma de emisión.
 */
static void template_contracts(void) {
    const CGCryptoTemplate cases[] = {
        {.operation = CG_CRYPTO_AUTH, .registers = {10, -1, -1}, .k = -1, .w = -1},
        {.operation = CG_CRYPTO_LOGOUT, .registers = {-1, -1, -1}, .k = -1, .w = -1},
        {.operation = CG_CRYPTO_KV_WRITE, .registers = {10, -1, -1}, .k = 2, .w = 3},
        {.operation = CG_CRYPTO_KV_LOCK, .registers = {-1, -1, -1}, .k = -1, .w = -1},
        {.operation = CG_CRYPTO_SANC, .registers = {10, -1, -1}, .k = -1, .w = -1},
        {.operation = CG_CRYPTO_ENCRYPT, .registers = {6, 7, 9}, .k = 2, .w = -1},
        {.operation = CG_CRYPTO_DECRYPT, .registers = {6, 7, 9}, .k = 2, .w = -1},
    };
    const char *simple[] = {"auth t6", "logout", "kv_write t6, 2, 3", "kv_lock", "sanc t6"};
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        cg_init();
        unsigned next = 17;
        int saved_calls = calls;
        assert(cg_crypto_emit_template(&cases[i], &next) == 0);
        assert(calls == saved_calls);
        if (i < 5) {
            assert(cg_ir_count() == 1 && next == 17);
            for (unsigned slot = 0; slot < 4; slot++)
                fixed_slot(0, slot, slot == 1 ? simple[i] : "nopi");
        } else {
            assert(cg_ir_count() == 18 && next == 18);
            loop_records(0, 2, i == 6, 17);
        }
        rejected_template(NULL, &next);
        rejected_template(&cases[i], NULL);
        const int invalid_registers[] = {-2, 0, 16};
        for (unsigned slot = 0; slot < 3; slot++) {
            for (unsigned r = 0; r < 3; r++) {
                CGCryptoTemplate bad = cases[i];
                bad.registers[slot] = invalid_registers[r];
                rejected_template(&bad, &next);
            }
        }
        for (unsigned field = 0; field < 2; field++) {
            CGCryptoTemplate bad = cases[i];
            int present = field == 0 ? bad.k != -1 : bad.w != -1;
            if (field == 0)
                bad.k = present ? -1 : 0;
            else
                bad.w = present ? -1 : 0;
            rejected_template(&bad, &next);
            if (field == 0)
                bad.k = 4;
            else
                bad.w = 4;
            rejected_template(&bad, &next);
        }
        CGCryptoTemplate bad = cases[i];
        bad.operation = (CGCryptoOperation)-1;
        rejected_template(&bad, &next);
        bad.operation = (CGCryptoOperation)99;
        rejected_template(&bad, &next);
        next = UINT_MAX;
        if (i >= 5)
            rejected_template(&cases[i], &next);
        else
            assert(cg_crypto_emit_template(&cases[i], &next) == 0 && next == UINT_MAX);
        cg_free();
    }
    cg_init();
    unsigned next = 41;
    assert(cg_crypto_emit_template(&cases[5], &next) == 0);
    assert(cg_crypto_emit_template(&cases[6], &next) == 0);
    assert(next == 43 && cg_ir_count() == 36);
    loop_records(0, 2, 0, 41);
    loop_records(18, 2, 1, 42);
    cg_free();
    for (int key = 0; key < 4; key++) {
        for (int word = 0; word < 4; word++) {
            cg_init();
            CGCryptoTemplate operation = cases[2];
            operation.k = key;
            operation.w = word;
            next = 1;
            assert(cg_crypto_emit_template(&operation, &next) == 0 && next == 1);
            char text[64];
            snprintf(text, sizeof text, "kv_write t6, %d, %d", key, word);
            fixed_slot(0, 1, text);
            cg_free();
        }
    }
}

/*
 * callback_failures — comprueba orden y fallo en el segundo o tercer argumento.
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige fallo sin restauración ni consumo de etiquetas.
 * Efectos:
 *   usa el evaluador de prueba y deja emisión parcial que luego descarta.
 * Restricciones:
 *   guarda r10 como vivo para detectar su restauración indebida.
 */
static void callback_failures(void) {
    for (int fail = 2; fail <= 3; fail++) {
        cg_init();
        calls = 0;
        fail_eval = fail;
        CGCryptoContext ctx = {.eval = evaluate, .function = &function,
                               .live_mask = 1u << 10, .next_label = 8};
        ASTNode args[] = {{.kind = AST_NUMBER, .number = 4096},
                          {.kind = AST_NUMBER, .number = 8192},
                          {.kind = AST_NUMBER, .number = 1},
                          {.kind = AST_NUMBER, .number = 0}};
        assert(operation(&ctx, "cifrar", args, 4) == 1);
        assert(calls == fail && ctx.next_label == 8);
        const int destinations[] = {6, 7, 9};
        const unsigned preserves[] = {0, 1u << 6, (1u << 6) | (1u << 7)};
        for (int i = 0; i < fail; i++) {
            assert(eval_destinations[i] == destinations[i]);
            assert(eval_preserves[i] == preserves[i]);
            assert(eval_expressions[i] == &args[i]);
        }
        int stores = 0;
        int results = 0;
        for (int i = 0; i < cg_ir_count(); i++) {
            const CGIRRecord *record = cg_ir_record(i);
            assert(record->kind != CG_IR_LABEL && record->kind != CG_IR_FIXED_BUNDLE);
            if (record->kind != CG_IR_OPERATION)
                continue;
            const char *text = record->data.instruction.text;
            if (strncmp(text, "sw ", 3) == 0) {
                assert(strcmp(text, "sw r10, 88(rz)") == 0);
                stores++;
            } else {
                assert(strncmp(text, "mov ", 4) == 0);
                results++;
            }
        }
        assert(stores == 1 && results == fail - 1);
        cg_free();
    }
    fail_eval = 0;
}

static int contracts(void) {
    CGCryptoContext ctx = {.eval = evaluate, .function = &function, .next_label = 1};
    ASTNode number = {.kind = AST_NUMBER, .number = 7};
    ASTNode list = {.kind = AST_ARGUMENTS, .child = &number};
    ASTNode node = {.kind = AST_CRYPTO, .text = "autenticar", .child = &list};
    cg_init();
    assert(cg_crypto(NULL, &node) == 1);
    assert(cg_crypto(&ctx, NULL) == 1);
    ctx.live_mask = 1;
    assert(cg_crypto(&ctx, &node) == 1);
    ctx.live_mask = 0xFFF0;
    ctx.function = NULL;
    assert(cg_crypto(&ctx, &node) == 1);
    Symbol bad = function;
    ctx.function = &bad;
    bad.frame.temp_base = 65;
    assert(cg_crypto(&ctx, &node) == 1);
    bad.frame.temp_base = 80;
    assert(cg_crypto(&ctx, &node) == 1);
    bad = function;
    bad.frame.area_base = INT_MAX;
    assert(cg_crypto(&ctx, &node) == 1);
    ctx.function = &function;
    ctx.live_mask = 0;
    ctx.eval = NULL;
    assert(cg_crypto(&ctx, &node) == 1);
    ctx.eval = evaluate;
    node.text = "desconocida";
    assert(cg_crypto(&ctx, &node) == 1);
    node.text = "sellar";
    assert(cg_crypto(&ctx, &node) == 1);
    ASTNode args[4] = {{.kind = AST_NUMBER, .number = 4096},
                       {.kind = AST_NUMBER, .number = 8192},
                       {.kind = AST_NUMBER, .number = 1},
                       {.kind = AST_NUMBER, .number = 4}};
    assert(operation(&ctx, "cifrar", args, 4) == 1);
    args[3].number = 0;
    args[3].kind = AST_IDENTIFIER;
    assert(operation(&ctx, "cifrar", args, 4) == 1);
    args[3].kind = AST_NUMBER;
    ctx.next_label = UINT_MAX;
    assert(operation(&ctx, "cifrar", args, 4) == 1);
    ctx.next_label = 1;
    args[0].number = -1;
    assert(operation(&ctx, "cargar_llave", args, 3) == 1);
    assert(cg_line_count() == 0 && calls == 0);
    fail_eval = 1;
    assert(operation(&ctx, "autenticar", &number, 1) == 1);
    assert(calls == 1 && ctx.next_label == 1);
    cg_free();
    fail_eval = 0;
    template_contracts();
    callback_failures();
    puts("PASS contrato crypto, siete plantillas directas y fallos del evaluador");
    return 0;
}

static int loop(CGCryptoContext *ctx, const char *name, int origin, int destination,
                int blocks, int key, int dynamic) {
    ASTNode args[4] = {{.kind = AST_NUMBER, .number = origin},
                       {.kind = AST_NUMBER, .number = destination},
                       {.kind = AST_NUMBER, .number = blocks},
                       {.kind = AST_NUMBER, .number = key}};
    Symbol symbols[3] = {{.address = 16}, {.address = 20}, {.address = 24}};
    ASTNode children[2] = {{.kind = AST_IDENTIFIER, .symbol = &symbols[0]},
                          {.kind = AST_NUMBER, .number = 0}};
    if (dynamic) {
        children[0].next = &children[1];
        args[0].kind = AST_ADD;
        args[0].child = &children[0];
        args[1].kind = args[2].kind = AST_IDENTIFIER;
        args[1].symbol = &symbols[1];
        args[2].symbol = &symbols[2];
    }
    return operation(ctx, name, args, 4);
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--contract") == 0)
        return contracts();
    if (argc < 5 || argc > 6)
        return 2;
    int blocks = atoi(argv[2]);
    int key = atoi(argv[3]);
    unsigned mask = (unsigned) strtoul(argv[4], NULL, 0);
    int dynamic = argc == 6 && strcmp(argv[5], "dynamic") == 0;
    int single = strcmp(argv[1], "single") == 0;
    int inplace = strcmp(argv[1], "inplace") == 0;
    if (!single && !inplace && strcmp(argv[1], "demo") != 0)
        return 2;
    cg_init();
    CGCryptoContext ctx = {.eval = evaluate, .function = &function,
                           .live_mask = mask, .next_label = 1};
    for (int r = 1; r < 16; r++)
        cg_op(SLOT_ALU0, "mov r%d, %d", r, 1000 + r);
    for (int i = 0; i < 3; i++) {
        int values[] = {4096, inplace ? 4096 : 8192, blocks};
        cg_op(SLOT_ALU0, "mov t0, %d", values[i]);
        cg_op(SLOT_LSU, "sw t0, %d(rz)", 16 + 4 * i);
    }
    cg_op(SLOT_ALU0, "mov t0, 1004");
    ASTNode token = {.kind = AST_NUMBER, .number = 193210613};
    ASTNode arg = {.kind = AST_NEG, .child = &token};
    if (operation(&ctx, "autenticar", &arg, 1)) return 1;
    long values[] = {0x0123ABCD, -1985229329L, 0x13579BDF, 0x2468ACE0};
    for (int word = 0; word < 4; word++) {
        ASTNode literal = {.kind = AST_NUMBER, .number = -values[word]};
        ASTNode args[3] = {{.kind = AST_NUMBER, .number = key},
                           {.kind = AST_NUMBER, .number = word},
                           {.kind = values[word] < 0 ? AST_NEG : AST_NUMBER,
                            .number = values[word], .child = values[word] < 0 ? &literal : NULL}};
        if (operation(&ctx, "cargar_llave", args, 3)) return 1;
    }
    if (operation(&ctx, "sellar", NULL, 0)) return 1;
    arg = (ASTNode) {.kind = AST_NUMBER, .number = 5};
    if (operation(&ctx, "cambiar_ancla", &arg, 1)) return 1;
    if (loop(&ctx, "cifrar", 4096, inplace ? 4096 : 8192, blocks, key, dynamic)) return 1;
    if (!single) {
        if (dynamic) {
            cg_op(SLOT_ALU0, "mov t0, %d", inplace ? 4096 : 8192);
            cg_op(SLOT_LSU, "sw t0, 16(rz)");
            cg_op(SLOT_ALU0, "mov t0, %d", inplace ? 4096 : 12288);
            cg_op(SLOT_LSU, "sw t0, 20(rz)");
            cg_op(SLOT_ALU0, "mov t0, 1004");
        }
        if (loop(&ctx, "descifrar", inplace ? 4096 : 8192,
                 inplace ? 4096 : 12288, blocks, key, dynamic)) return 1;
        if (operation(&ctx, "cerrar_sesion", NULL, 0)) return 1;
        if (loop(&ctx, "cifrar", 4096, 16384, 1, key, 0)) return 1;
        arg.number = 999;
        if (operation(&ctx, "autenticar", &arg, 1)) return 1;
    }
    cg_label("fin_main");
    cg_bundle(NULL, NULL, "jef fin_main", NULL);
    cg_write(stdout);
    cg_free();
    return ferror(stdout) ? 1 : 0;
}
