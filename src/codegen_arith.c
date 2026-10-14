/*
 * codegen_arith.c — Fase 4: expansión de aritmética entera por software.
 *
 * Emite multiplicación, división, módulo y potencia mediante ALU y BRU.
 * Entrega operaciones al callback para que el llamador organice los bundles.
 *
 * Usa: codegen_arith.h; biblioteca C.
 * Lo usa: generate.c y pruebas/aritmetica.
 * Referencias: isa.md §6.1, §6.2, §8.
 */

#include "codegen_arith.h"
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

/* ==========================================================================
 * OPERACIONES DEL CALLBACK
 * ======================================================================= */


/*
 * out — formatea y entrega una operación al callback.
 *
 * Entrada:
 *   c  contexto con emit no NULL.
 *   slot  slot 0, 2, 3 o -1 para etiqueta.
 *   d  registro destino o -1.
 *   a  primer registro fuente o -1.
 *   b  segundo registro fuente o -1.
 *   fmt  formato printf.
 *   ...  argumentos del formato.
 * Salida:
 *   1 si formato y callback aceptan; 0 si falla alguno.
 *   Efectos: invoca c->emit con una operación local.
 * Restricciones:
 *   texto menor de 128 bytes; callback no conserva &op.
 */
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
/*
 * alu — entrega una operación ALU de tres registros.
 *
 * Entrada:
 *   c  contexto con callback.
 *   mn  mnemónico no NULL.
 *   d  registro destino.
 *   a  primera fuente.
 *   b  segunda fuente.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   registros ISA; operación en slot 0.
 */
static int alu(CGArithContext *c, const char *mn, int d, int a, int b) {
    return out(c, 0, d, a, b, "%s r%d, r%d, r%d", mn, d, a, b);
}
/*
 * mov — entrega una carga de inmediato.
 *
 * Entrada:
 *   c  contexto con callback.
 *   d  registro destino.
 *   imm  inmediato.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   inmediato debe caber en 18 bits con signo.
 */
static int mov(CGArithContext *c, int d, int imm) {
    return out(c, 0, d, -1, -1, "mov r%d, %d", d, imm);
}
/*
 * copy — entrega copia mediante add con rz.
 *
 * Entrada:
 *   c  contexto con callback.
 *   d  registro destino.
 *   a  registro fuente.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   ninguna.
 */
static int copy(CGArithContext *c, int d, int a) {
    return alu(c, "add", d, a, 0);
}
/*
 * branch — entrega un salto condicional.
 *
 * Entrada:
 *   c  contexto con callback.
 *   mn  mnemónico BRU.
 *   a  primera fuente.
 *   b  segunda fuente.
 *   label  etiqueta destino no NULL.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   slot 2; destino lo define el llamador o la expansión.
 */
static int branch(CGArithContext *c, const char *mn, int a, int b,
                  const char *label) {
    return out(c, 2, -1, a, b, "%s r%d, r%d, %s", mn, a, b, label);
}
/*
 * jump — entrega un salto incondicional.
 *
 * Entrada:
 *   c  contexto con callback.
 *   label  etiqueta no NULL.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   ninguna.
 */
static int jump(CGArithContext *c, const char *label) {
    return out(c, 2, -1, -1, -1, "jef %s", label);
}
/*
 * label — entrega una definición de etiqueta.
 *
 * Entrada:
 *   c  contexto con callback.
 *   name  nombre no NULL sin dos puntos.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback con slot=-1.
 * Restricciones:
 *   ninguna.
 */
static int label(CGArithContext *c, const char *name) {
    return out(c, -1, -1, -1, -1, "%s:", name);
}
/*
 * name — construye un nombre interno de expansión.
 *
 * Entrada:
 *   c  contexto no NULL.
 *   buf  buffer de al menos 64 bytes.
 *   suffix  sufijo no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe buf e incrementa next_label.
 * Restricciones:
 *   contador compartido por todo el programa.
 */
static void name(CGArithContext *c, char *buf, const char *suffix) {
    snprintf(buf, 64, "__ccom_arith_%u_%s", c->next_label++, suffix);
}
/* ==========================================================================
 * EXPANSIONES ARITMÉTICAS
 * ======================================================================= */


/*
 * multiply — expande producto con sumas y desplazamientos.
 *
 * Entrada:
 *   c  contexto con callback.
 *   lhs  operando izquierdo.
 *   rhs  operando derecho.
 *   dst  destino.
 *   a  auxiliar para multiplicando.
 *   b  auxiliar para multiplicador.
 *   bit  auxiliar para bit.
 *   one  registro que contiene 1.
 * Salida:
 *   1 si emite; 0 si falla callback.
 *   Efectos: emite operaciones y etiquetas; avanza next_label.
 * Restricciones:
 *   auxiliares distintos de operandos/destino; dst puede coincidir con
 *   lhs/rhs; resultado envuelve a 32 bits.
 */
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
/*
 * divide — expande cociente o resto con división binaria.
 *
 * Entrada:
 *   c  contexto con callback.
 *   lhs  dividendo.
 *   rhs  divisor.
 *   dst  destino.
 *   w  ocho registros auxiliares distintos.
 *   modulo  1 para resto, 0 para cociente.
 *   error  etiqueta terminal para divisor cero.
 * Salida:
 *   1 si emite; 0 si falla callback.
 *   Efectos: emite operaciones/etiquetas; avanza next_label.
 * Restricciones:
 *   operandos preservados; destino/auxiliares distintos entre sí y de
 *   operandos; no diagnostica desborde dinámico.
 */
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
    DO(alu(c, "slt", bit, a, 0)); /* Extrae el bit alto del dividendo sin signo. */
    DO(alu(c, "sll", a, a, one)); DO(alu(c, "sll", rem, rem, one));
    DO(alu(c, "or", rem, rem, bit)); DO(alu(c, "sll", dst, dst, one));
    /* XOR con el bit de signo transforma el orden sin signo en orden con signo para BRU. */
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
/*
 * power — expande potencia por cuadrados sucesivos.
 *
 * Entrada:
 *   c  contexto con callback.
 *   lhs  base.
 *   rhs  exponente.
 *   dst  destino.
 *   w  ocho registros auxiliares distintos.
 *   error  etiqueta terminal para exponente negativo.
 * Salida:
 *   1 si emite; 0 si falla callback.
 *   Efectos: emite operaciones/etiquetas; avanza next_label.
 * Restricciones:
 *   operandos preservados; destino/auxiliares distintos entre sí y de
 *   operandos; no diagnostica desborde dinámico.
 */
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
/* ==========================================================================
 * VALIDACIÓN DEL CONTRATO
 * ======================================================================= */


/*
 * valid_label — comprueba un nombre de etiqueta.
 *
 * Entrada:
 *   s  nombre o NULL.
 * Salida:
 *   1 si cumple [A-Za-z_.][A-Za-z0-9_.]*; 0 si no cumple.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int valid_label(const char *s) {
    if (!s || !(isalpha((unsigned char)*s) || *s == '_' || *s == '.')) return 0;
    for (++s; *s; ++s)
        if (!(isalnum((unsigned char)*s) || *s == '_' || *s == '.')) return 0;
    return 1;
}
/* cg_arith_emit — véase el contrato en codegen_arith.h. */
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
