/*
 * data_area.h — Fases 3–4: interfaz del área de datos estática de ccom.
 *
 * Lleva la cuenta de la primera dirección libre de la zona de ccom
 * (0x0000–0x0FFF) mientras el semántico reserva variables, parámetros y
 * áreas de función, y avisa cuándo una reserva se pasa del límite. No emite
 * diagnósticos: el texto, la posición y el orden de los errores los decide
 * quien reserva, en el mismo punto del recorrido en que reserva.
 *
 * Usa: biblioteca C.
 * Lo usa: data_area.c; semantic.c y generate.c.
 * Referencias: libro §6.3.4, §7.1.1.
 */

#ifndef DATA_AREA_H
#define DATA_AREA_H

/* M-2: las variables de ccom ocupan 0x0000–0x0FFF; desde 0x1000 están los
 * archivos de load_file (la zona de CRYPTO). */
#define DATA_LIMIT 0x1000          /* primera dirección que NO se puede usar */

/* Espacio mínimo que debe quedar libre, después de la última área y antes de
 * DATA_LIMIT, para la pila de llamadas de la Fase 4 (generate.c). */
#define STACK_MIN_BYTES 256

/*
 * data_area_reset — vacía el área y olvida el desborde anterior.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: la primera dirección libre vuelve a 0 y se rearma el aviso de
 *   desborde.
 * Restricciones:
 *   se llama al empezar cada análisis.
 */
void data_area_reset(void);

/*
 * data_area_alloc — reserva un tramo contiguo y avanza la primera dirección libre.
 *
 * Entrada:
 *   width  ancho en bytes no negativo (llega alineado a cuatro bytes).
 *   address  puntero escribible para la dirección asignada.
 * Salida:
 *   1 solo si esta reserva es la primera que se pasa de DATA_LIMIT en el
 *   programa; 0 en cualquier otro caso. El llamador reporta el error.
 *   Efectos: escribe *address (la dirección libre anterior) y avanza.
 * Restricciones:
 *   sigue asignando después de DATA_LIMIT: el límite se reporta una sola
 *   vez por programa, no se rechaza la reserva.
 */
int data_area_alloc(int width, int *address);

/*
 * data_area_end — consulta la primera dirección libre del área.
 *
 * Entrada: ninguna
 * Salida:
 *   desplazamiento en bytes, múltiplo de 4.
 *   Efectos: ninguno.
 * Restricciones:
 *   tras un análisis exitoso es la base de la pila de llamadas; durante el
 *   análisis marca el inicio de la siguiente área de función.
 */
int data_area_end(void);

/*
 * data_area_free — consulta cuántos bytes quedan antes de DATA_LIMIT.
 *
 * Entrada: ninguna
 * Salida:
 *   DATA_LIMIT menos la primera dirección libre; negativo si ya se pasó.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int data_area_free(void);

/*
 * data_area_stack_short — indica si falta espacio para la pila de llamadas.
 *
 * Entrada: ninguna
 * Salida:
 *   1 si no hubo desborde reportado y quedan menos de STACK_MIN_BYTES; 0 en
 *   otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   si las variables ya se pasaron del límite, ese error ya se reportó y
 *   no se suma otro.
 */
int data_area_stack_short(void);

#endif
