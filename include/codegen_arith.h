/*
 * codegen_arith.h — Fase 4: interfaz de expansión de aritmética entera por software.
 *
 * Expone operaciones, contexto y expansión de aritmética entera.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: tipos de la biblioteca C y declaraciones propias.
 * Lo usa: codegen_arith.c; generate.c, codegen_array.h y pruebas/aritmetica.
 * Referencias: isa.md §6.1, §6.2, §8.
 */

#ifndef CODEGEN_ARITH_H
#define CODEGEN_ARITH_H

/* Emisor aritmético independiente. Los registros usan números ISA (t0 = 4).
 * El callback recibe cada operación antes de organizar los bundles.
 * rd/rs1/rs2 = -1 si no aplica. Las etiquetas tienen slot = -1.
 * El callback devuelve no cero al aceptar; cero detiene la emisión. */
typedef struct {
    char text[128];
    int slot, rd, rs1, rs2;
} CGArithOp;

typedef int (*CGArithEmit)(void *user, const CGArithOp *op);

typedef struct {
    CGArithEmit emit;
    void *user;
    unsigned next_label; /* Inicializar una vez por programa y compartir entre expresiones. */
} CGArithContext;

typedef enum { CG_ARITH_MUL, CG_ARITH_DIV, CG_ARITH_MOD,
               CG_ARITH_POWER } CGArithKind;

/*
 * cg_arith_emit — expande aritmética entera sin instrucción de multiplicación.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   kind  operación MUL, DIV, MOD o POWER.
 *   lhs  registro izquierdo.
 *   rhs  registro derecho.
 *   dst  registro destino.
 *   work  ocho registros auxiliares.
 *   error_label  etiqueta de error requerida salvo MUL.
 * Salida:
 *   no cero si emite; 0 ante contrato inválido o fallo de callback.
 *   Efectos: emite operaciones y etiquetas; modifica next_label.
 * Restricciones:
 *   registros 4–15; destino/auxiliares distintos entre sí y de operandos;
 *   descartar emisión parcial; desborde dinámico envuelve.
 */
int cg_arith_emit(CGArithContext *ctx, CGArithKind kind,
                  int lhs, int rhs, int dst, const int work[8],
                  const char *error_label);
#endif
