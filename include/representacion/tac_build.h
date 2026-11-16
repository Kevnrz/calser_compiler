/*
 * tac_build.h — construcción de TAC de alto nivel desde el AST validado.
 * Usa: tac.h y ast.h. Lo usan main y las pruebas; no genera ISA ni binarios.
 * Referencias: Aho §6.4, §6.4.3, §6.6, §8.4.1; enunciado §4.1.
 */
#ifndef TAC_BUILD_H
#define TAC_BUILD_H
#include "ast.h"
#include "tac.h"

typedef struct {
    TacProgram *program;       /* Programa propio; presta símbolos del semántico. */
    uint32_t *initial_data;     /* Imagen propia, en palabras de 32 bits. */
    size_t initial_data_size;   /* Extensión de la imagen en bytes, como cg_data_size. */
    Symbol *auxiliary_symbols; /* Símbolos propios del límite de S; lista por next. */
} TacBuildResult;

/*
 * tac_build — traduce sin modificar el AST, símbolos ni estado del generador.
 * Entrada: root semánticamente válido; global_scope real y vivo; error/capacity.
 * Salida: resultado propio o NULL con diagnóstico; error admite NULL con cero.
 * Efectos: reserva TAC, imagen y símbolos auxiliares; valida forma y aristas.
 * Restricciones: liberar antes de semantic_free; sin memoria termina con diagnóstico.
 */
TacBuildResult *tac_build(ASTNode *root, const Scope *global_scope,
                         char *error, size_t capacity);

/*
 * tac_build_print — imprime imagen inicial y el listado diagnóstico de tac_print.
 * Entrada: out vivo; result válido. Salida: ninguna.
 * Efectos: escribe out, sin cerrarlo ni modificar result.
 * Restricciones: el llamador comprueba errores de escritura y cierre.
 */
void tac_build_print(FILE *out, const TacBuildResult *result);

/*
 * tac_build_free — destruye el resultado conservando AST y símbolos prestados.
 * Entrada: result propio o NULL. Salida: ninguna.
 * Efectos: libera TAC antes de sus símbolos auxiliares y la imagen inicial.
 * Restricciones: no usar result después de liberar.
 */
void tac_build_free(TacBuildResult *result);
#endif
