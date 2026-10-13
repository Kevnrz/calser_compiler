#ifndef CODEGEN_ARITH_H
#define CODEGEN_ARITH_H

/* Independent K-4 emitter. Register numbers are ISA numbers (t0 = 4).
 * A callback receives one operation, not an already scheduled bundle.
 * rd/rs1/rs2 = -1 when absent. Labels have slot = -1.
 * Return nonzero on success; zero stops emission. */
typedef struct {
    char text[128];
    int slot, rd, rs1, rs2;
} CGArithOp;

typedef int (*CGArithEmit)(void *user, const CGArithOp *op);

typedef struct {
    CGArithEmit emit;
    void *user;
    unsigned next_label; /* Initialize once per program, never per expression. */
} CGArithContext;

typedef enum { CG_ARITH_MUL, CG_ARITH_DIV, CG_ARITH_MOD,
               CG_ARITH_POWER } CGArithKind;

/* lhs/rhs preserved; dst and eight work registers must be distinct from them
 * and from each other, all in 4..15. Work registers are clobbered.
 * error_label is required for /, %, ^: divisor zero or negative exponent
 * jumps there. The caller defines a terminal handler.
 * Valid representable results match semantic.c. Dynamic overflow wraps as
 * the 32-bit ALU does; no runtime overflow diagnosis is implemented here.
 * Returns zero on invalid contract or callback failure. Partial output must
 * be discarded on callback failure. No rl/rx/rg, memory, or jas is used. */
int cg_arith_emit(CGArithContext *ctx, CGArithKind kind,
                  int lhs, int rhs, int dst, const int work[8],
                  const char *error_label);
#endif
