/*
 * interpretar.h — intérprete de prueba del TAC alto nivel en memoria.
 * Usa: tac_build.h; no se enlaza con ccom ni lee el listado impreso.
 * Referencias: Aho §6.4, §6.4.3, §6.6 y §7.2 (activaciones).
 */
#ifndef TAC_TEST_INTERPRETAR_H
#define TAC_TEST_INTERPRETAR_H
#include "tac_build.h"

/* Estados terminales distintos de un RETURN normal. */
typedef enum {
    TAC_RUN_OK,
    TAC_RUN_DIVISION_BY_ZERO,
    TAC_RUN_NEGATIVE_EXPONENT,
    TAC_RUN_UNSUPPORTED_CRYPTO,
    TAC_RUN_STEP_LIMIT,
    TAC_RUN_INVALID,
    TAC_RUN_NO_MEMORY
} TacRunStatus;

typedef struct {
    TacRunStatus status;          /* Estado final; no confundir error aritmético con retorno. */
    unsigned char *memory;        /* Memoria propia, compartida por todas las funciones. */
    size_t memory_size;           /* Bytes disponibles; accesos comprobados en este espacio. */
    uint64_t steps;               /* Instrucciones ejecutadas, sumando todas las invocaciones. */
    char diagnostic[192];         /* Causa y línea TAC cuando corresponde. */
} TacRunResult;

/*
 * tac_interpret — ejecuta un TacBuildResult válido desde main con memoria nueva.
 * Entrada: build prestado; max_steps compartido por todas las llamadas.
 * Salida: resultado propio, incluso ante fallo; consultar status y diagnostic.
 * Efectos: carga imagen inicial; ejecuta TAC sin modificar programa ni símbolos.
 * Restricciones: solo HIGH; rechaza programas con CRYPTO antes de ejecutarlos.
 *   Memoria estática de 64 KiB, valores de 32 bits; temporales por invocación.
 *   No modela ISA, ciclos ni pipeline; liberar el resultado tras consultarlo.
 */
TacRunResult tac_interpret(const TacBuildResult *build, uint64_t max_steps);

/*
 * tac_run_status_name — nombre estable del estado para pruebas automatizadas.
 * Entrada: estado. Salida: cadena estática; "invalid" si no reconoce el valor.
 * Efectos: ninguno. Restricciones: no liberar la cadena.
 */
const char *tac_run_status_name(TacRunStatus status);

/*
 * tac_run_read_word — consulta memoria little endian sin acceder a símbolos.
 * Entrada: resultado vivo, dirección en bytes y out no NULL.
 * Salida: 1 y palabra leída, o 0 si no hay cuatro bytes disponibles.
 * Efectos: escribe out solo al tener éxito.
 * Restricciones: los resultados de error pueden no tener memoria asignada.
 */
int tac_run_read_word(const TacRunResult *result, uint32_t address, uint32_t *out);

/*
 * tac_run_free — libera la memoria del resultado y limpia su referencia.
 * Entrada: resultado vivo o NULL. Salida: ninguna.
 * Efectos: libera únicamente memoria propia, nunca el TacBuildResult.
 * Restricciones: consultar/copiar los valores antes de liberar.
 */
void tac_run_free(TacRunResult *result);
#endif
