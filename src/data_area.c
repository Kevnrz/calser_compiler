/*
 * data_area.c — Fases 3–4: área de datos estática de ccom.
 *
 * Contador de la primera dirección libre (el "offset" de la asignación
 * estática, libro §6.3.4) y aviso de desborde de la zona 0x0000–0x0FFF.
 *
 * Usa: data_area.h.
 * Lo usa: semantic.c y generate.c.
 * Referencias: libro §6.3.4, §7.1.1.
 */

#include "data_area.h"

#define DATA_BASE 0x0000

static int next_address = DATA_BASE;   // libro §6.3.4: el "offset"

// El desborde de la memoria de datos se reporta una sola vez por programa.
static int memory_reported = 0;

/* data_area_reset — véase el contrato en data_area.h. */
void data_area_reset(void) {
    next_address = DATA_BASE;
    memory_reported = 0;
}

/* data_area_alloc — véase el contrato en data_area.h. */
int data_area_alloc(int width, int *address) {
    int first_overflow = next_address + width > DATA_LIMIT && !memory_reported;

    if (first_overflow)
        memory_reported = 1;

    *address = next_address;
    next_address += width;

    return first_overflow;
}

/* data_area_end — véase el contrato en data_area.h. */
int data_area_end(void) {
    return next_address;
}

/* data_area_free — véase el contrato en data_area.h. */
int data_area_free(void) {
    return DATA_LIMIT - next_address;
}

/* data_area_stack_short — véase el contrato en data_area.h. */
int data_area_stack_short(void) {
    return !memory_reported && DATA_LIMIT - next_address < STACK_MIN_BYTES;
}
