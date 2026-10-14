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

/* Expande un AST_CRYPTO validado semánticamente. 0 = éxito, 1 = error.
 * Guarda los temporales vivos ANTES de evaluar argumentos y los restaura
 * al terminar. Casilla de rN: function->temp_base + 4*(N-4).
 * Origen/destino/bloques se evalúan en r6/r7/r9, preservando los argumentos
 * anteriores. Los selectores k/w son AST_NUMBER, nunca registros.
 * Emite bundles calendarizados con cg_bundle. No altera rl, rx o rg.
 * Un error del evaluador puede dejar emisión parcial: abortar la generación.
 * Con live_mask=0 no se requiere function; con valores vivos requiere un área
 * válida de 48 bytes. El contexto solo puede modificarse entre expansiones.
 */
int cg_crypto(CGCryptoContext *ctx, ASTNode *node);

#endif
