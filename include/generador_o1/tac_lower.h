/*
 * tac_lower.h — selección HIGH -> ISA virtual, sin asignación ni emisión.
 * Usa: tac.h. Lo usan pruebas y las futuras pasadas del camino O1.
 * Referencias: Aho §8.1–8.2, §8.6; constantes según isa_constant_plan.
 */
#ifndef TAC_LOWER_H
#define TAC_LOWER_H
#include "tac.h"

/*
 * tac_lower — selecciona instrucciones en un programa nuevo.
 *
 * Entrada:
 *   high: programa HIGH válido de TacBuildResult, con símbolos semánticos vivos.
 *   error: buffer escribible o NULL solo si capacity es cero.
 * Salida:
 *   programa ISA_VIRTUAL propio; NULL con diagnóstico ante forma/datos inválidos.
 * Efectos:
 *   reserva funciones, bloques e instrucciones; no modifica HIGH ni los símbolos.
 * Restricciones:
 *   conserva IDs TEMP de HIGH; agrega otros sin límite de doce registros.
 *   comparte símbolos y ámbito; la imagen inicial sigue en TacBuildResult.
 *   liberar con tac_program_free antes de tac_build_free y semantic_free.
 *   sin memoria termina con diagnóstico, igual que los constructores TAC.
 *   no conserva vivos, asigna físicos, expande PSEUDO ni emite; Aho §8.1–8.2.
 */
TacProgram *tac_lower(const TacProgram *high, char *error, size_t capacity);

#endif
