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

static int evaluate(void *user, ASTNode *expr, int dst, unsigned preserve) {
    (void) user;
    calls++;
    if (fail_eval || dst < 4 || dst > 15 || (preserve & (1u << dst)))
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

static Symbol function = {.kind = SYMBOL_FUNCTION, .area_base = 32,
                          .area_size = 80, .temp_base = 64};

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
    bad.temp_base = 65;
    assert(cg_crypto(&ctx, &node) == 1);
    bad.temp_base = 80;
    assert(cg_crypto(&ctx, &node) == 1);
    bad = function;
    bad.area_base = INT_MAX;
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
    puts("PASS contrato crypto y fallo del evaluador");
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
