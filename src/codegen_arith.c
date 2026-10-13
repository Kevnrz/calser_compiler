#include "codegen_arith.h"
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

static int out(CGArithContext *c, int slot, int d, int a, int b,
               const char *fmt, ...) {
    CGArithOp op;
    va_list ap;
    op.slot = slot; op.rd = d; op.rs1 = a; op.rs2 = b;
    va_start(ap, fmt);
    int n = vsnprintf(op.text, sizeof op.text, fmt, ap);
    va_end(ap);
    return n >= 0 && n < (int)sizeof op.text && c->emit(c->user, &op);
}
#define DO(x) do { if (!(x)) return 0; } while (0)
static int alu(CGArithContext *c, const char *mn, int d, int a, int b) {
    return out(c, 0, d, a, b, "%s r%d, r%d, r%d", mn, d, a, b);
}
static int mov(CGArithContext *c, int d, int imm) {
    return out(c, 0, d, -1, -1, "mov r%d, %d", d, imm);
}
static int copy(CGArithContext *c, int d, int a) {
    return alu(c, "add", d, a, 0);
}
static int branch(CGArithContext *c, const char *mn, int a, int b,
                  const char *label) {
    return out(c, 2, -1, a, b, "%s r%d, r%d, %s", mn, a, b, label);
}
static int jump(CGArithContext *c, const char *label) {
    return out(c, 2, -1, -1, -1, "jef %s", label);
}
static int label(CGArithContext *c, const char *name) {
    return out(c, -1, -1, -1, -1, "%s:", name);
}
static void name(CGArithContext *c, char *buf, const char *suffix) {
    snprintf(buf, 64, "__ccom_arith_%u_%s", c->next_label++, suffix);
}
/* Logical right shift makes this loop finite even for negative multipliers.
 * a/b/bit are scratch; one contains 1. dst may alias lhs or rhs here only:
 * both operands are copied before dst is cleared. */
static int multiply(CGArithContext *c, int lhs, int rhs, int dst,
                    int a, int b, int bit, int one) {
    char loop[64], skip[64], end[64];
    name(c, loop, "mul"); name(c, skip, "skip"); name(c, end, "end");
    DO(copy(c, a, lhs)); DO(copy(c, b, rhs)); DO(mov(c, dst, 0));
    DO(label(c, loop)); DO(branch(c, "beq", b, 0, end));
    DO(alu(c, "and", bit, b, one));
    DO(branch(c, "beq", bit, 0, skip));
    DO(alu(c, "add", dst, dst, a)); DO(label(c, skip));
    DO(alu(c, "sll", a, a, one)); DO(alu(c, "srl", b, b, one));
    DO(jump(c, loop)); DO(label(c, end));
    return 1;
}
static int divide(CGArithContext *c, int lhs, int rhs, int dst,
                  const int w[8], int modulo, const char *error) {
    int a=w[0], b=w[1], rem=w[2], count=w[3], one=w[4],
        bit=w[5], tmp=w[6], mask=w[7];
    char apos[64], bpos[64], loop[64], skip[64], end[64];
    name(c, apos, "apos"); name(c, bpos, "bpos");
    name(c, loop, "div"); name(c, skip, "skip"); name(c, end, "end");
    DO(branch(c, "beq", rhs, 0, error));
    DO(copy(c, a, lhs)); DO(copy(c, b, rhs));
    DO(branch(c, "bge", a, 0, apos)); DO(alu(c, "sub", a, 0, a));
    DO(label(c, apos)); DO(branch(c, "bge", b, 0, bpos));
    DO(alu(c, "sub", b, 0, b)); DO(label(c, bpos));
    DO(mov(c, one, 1)); DO(mov(c, count, 31));
    DO(alu(c, "sll", mask, one, count));
    DO(mov(c, count, 32)); DO(mov(c, rem, 0)); DO(mov(c, dst, 0));
    DO(label(c, loop));
    DO(alu(c, "slt", bit, a, 0)); /* Extract unsigned dividend's top bit. */
    DO(alu(c, "sll", a, a, one)); DO(alu(c, "sll", rem, rem, one));
    DO(alu(c, "or", rem, rem, bit)); DO(alu(c, "sll", dst, dst, one));
    /* XOR sign bit maps unsigned ordering onto signed BRU ordering. */
    DO(alu(c, "xor", tmp, rem, mask)); DO(alu(c, "xor", bit, b, mask));
    DO(branch(c, "blt", tmp, bit, skip));
    DO(alu(c, "sub", rem, rem, b));
    DO(out(c, 0, dst, dst, -1, "saddi r%d, r%d, 1", dst, dst));
    DO(label(c, skip));
    DO(out(c, 0, count, count, -1, "saddi r%d, r%d, -1", count, count));
    DO(branch(c, "bne", count, 0, loop));
    if (modulo) {
        DO(copy(c, dst, rem)); DO(branch(c, "bge", lhs, 0, end));
    } else {
        DO(alu(c, "xor", tmp, lhs, rhs));
        DO(branch(c, "bge", tmp, 0, end));
    }
    DO(alu(c, "sub", dst, 0, dst)); DO(label(c, end));
    return 1;
}
static int power(CGArithContext *c, int lhs, int rhs, int dst,
                 const int w[8], const char *error) {
    int base=w[0], exp=w[1], one=w[2], bit=w[3];
    char loop[64], skip[64], end[64];
    name(c, loop, "pow"); name(c, skip, "skip"); name(c, end, "end");
    DO(branch(c, "blt", rhs, 0, error));
    DO(copy(c, base, lhs)); DO(copy(c, exp, rhs));
    DO(mov(c, one, 1)); DO(mov(c, dst, 1));
    DO(label(c, loop)); DO(branch(c, "beq", exp, 0, end));
    DO(alu(c, "and", bit, exp, one));
    DO(branch(c, "beq", bit, 0, skip));
    DO(multiply(c, dst, base, dst, w[4], w[5], w[6], one));
    DO(label(c, skip)); DO(alu(c, "srl", exp, exp, one));
    DO(branch(c, "beq", exp, 0, end));
    DO(multiply(c, base, base, base, w[4], w[5], w[6], one));
    DO(jump(c, loop)); DO(label(c, end));
    return 1;
}
/* [A-Za-z_.][A-Za-z0-9_.]*: la misma clase de etiquetas que acepta el
 * ensamblador (isa.md §11.1), para admitir las de la tabla (.L_arith_error). */
static int valid_label(const char *s) {
    if (!s || !(isalpha((unsigned char)*s) || *s == '_' || *s == '.')) return 0;
    for (++s; *s; ++s)
        if (!(isalnum((unsigned char)*s) || *s == '_' || *s == '.')) return 0;
    return 1;
}
int cg_arith_emit(CGArithContext *c, CGArithKind kind,
                  int lhs, int rhs, int dst, const int w[8],
                  const char *error) {
    if (!c || !c->emit || !w || kind < CG_ARITH_MUL || kind > CG_ARITH_POWER ||
        lhs < 4 || lhs > 15 || rhs < 4 || rhs > 15 ||
        dst < 4 || dst > 15 || dst == lhs || dst == rhs ||
        (kind != CG_ARITH_MUL && !valid_label(error))) return 0;
    unsigned used=(1u<<lhs)|(1u<<rhs)|(1u<<dst);
    for (int i=0; i<8; ++i) {
        if (w[i]<4 || w[i]>15 || (used & (1u<<w[i]))) return 0;
        used |= 1u<<w[i];
    }
    if (kind == CG_ARITH_MUL) {
        DO(mov(c, w[3], 1));
        return multiply(c, lhs, rhs, dst, w[0], w[1], w[2], w[3]);
    }
    if (kind == CG_ARITH_POWER) return power(c, lhs, rhs, dst, w, error);
    return divide(c, lhs, rhs, dst, w, kind == CG_ARITH_MOD, error);
}
