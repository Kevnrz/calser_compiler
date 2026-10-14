/*
 * codegen_crypto.h — Fase 4: interfaz de expansión de operaciones de cifrado.
 *
 * Expone evaluador de argumentos, contexto y expansión de cifrado.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: ast.h, symbol_table.h.
 * Lo usa: codegen_crypto.c; generate.c y pruebas/crypto.
 * Referencias: isa.md §5.5, §6.4, §7, §9.3; docs/crypto_para_ccom.md.
 */

#ifndef CODEGEN_CRYPTO_H
#define CODEGEN_CRYPTO_H

#include "ast.h"
#include "symbol_table.h"

/* Adaptador del evaluador de Kevin. Registros expresados como números ISA.
 * Evalúa expr en dst (4..15), preservando los registros de preserve_mask.
 * Devuelve 0 si tuvo éxito. Puede usar los demás temporales como scratch.
 * Debe calendarizar sus instrucciones y NO sobrescribir las casillas de
 * temp_base donde cg_crypto acaba de guardar live_mask (también en llamadas).
 * Si no puede cumplirlo, devuelve error; el llamador descarta el código parcial.
 */
typedef int (*CGCryptoEval)(void *user, ASTNode *expr, int dst,
                            unsigned preserve_mask);

typedef struct {
    CGCryptoEval eval;
    void *user;
    const Symbol *function;
    unsigned live_mask;  /* Bits ISA 4..15; valores a preservar antes de evaluar. */
    unsigned next_label; /* Inicializar una vez por programa; compartir entre funciones. */
} CGCryptoContext;

/*
 * cg_crypto — expande una operación CRYPTO del AST.
 *
 * Entrada:
 *   ctx  contexto de evaluación y vivos.
 *   node  nodo AST_CRYPTO validado.
 * Salida:
 *   0 si emite; 1 por contrato o evaluación inválidos.
 *   Efectos: emite plantilla, guarda/restaura vivos e incrementa next_label al cifrar.
 * Restricciones:
 *   selectores NUMBER 0–3; área válida si hay vivos; descartar emisión
 *   parcial; direcciones/bloques ya verificados por el semántico.
 */
int cg_crypto(CGCryptoContext *ctx, ASTNode *node);

#endif
