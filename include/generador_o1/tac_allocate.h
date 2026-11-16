/* Asignación ISA virtual -> física sobre el mismo programa, sin emisión.
 * Linear scan: Poletto y Sarkar 1999 (fuera de Aho); LRU: Aho §10.2.3.
 */
#ifndef TAC_ALLOCATE_H
#define TAC_ALLOCATE_H
#include "tac_intervals.h"

typedef struct TacAllocationResult TacAllocationResult;
/* Política futura del recorrido; NULL o spill=0: sin derrame (§7.6). */
typedef struct { int spill; } TacAllocationOptions;
typedef struct { unsigned physical_register; int address; } TacSpillMove;
typedef struct {
    size_t temp_id;                   /* TEMP original cuyo valor debe guardarse. */
    unsigned physical_register;       /* Registro que contiene ese valor. */
} TacRegisterBinding;
typedef struct {
    const TacInstruction *instruction; /* Sitio prestado del programa reescrito. */
    /* Movimientos vacíos hasta C3.4; orden de emisión de §11.6. */
    TacSpillMove loads[3];
    size_t load_count;
    TacSpillMove stores[1];
    size_t store_count;
    uint16_t scratch_mask;             /* Ocho auxiliares aritméticos o cero. */
    const TacRegisterBinding *preserved; /* Vector del resultado, orden físico. */
    size_t preserved_count;            /* Vivos afectados según C1. */
} TacEmissionSite;
typedef struct {
    uint16_t assigned_registers;       /* Solo registros asignados a valores. */
    size_t demand;                    /* Máximo TEMP vivos antes de asignar. */
    size_t pressure;                  /* Máximo de intervalos activos asignados. */
    /* Declaraciones adelantadas de P12 para la recolección de P9 (§9.10). */
    size_t spilled, spill_loads, spill_stores;
    double spill_ms;                  /* Tiempo del derrame integrado, cero hoy. */
    unsigned register_count;           /* Cantidad de bits de la máscara. */
} TacAllocationMetrics;

/* Programa ISA_VIRTUAL validado, source_name obligatorio y prestado.
 * Reescribe TEMP a PHYS, auxiliares y stage; valida al terminar.
 * NULL con diagnóstico: se abandona la compilación, IR puede quedar inválido.
 * No derrama, emite, modifica símbolos ni reserva datos. C3.4 extenderá el punto
 * interno de presión dentro del recorrido; no hay resolutor ni procesamiento posterior.
 * Resultado posee planes, no el programa. Liberarlo antes de programa y símbolos.
 * Sin memoria termina según contrato TAC. error puede ser NULL solo con capacity=0.
 */
TacAllocationResult *tac_allocate_registers(TacProgram *program, const char *source_name,
                                 const TacAllocationOptions *options,
                                 char *error, size_t capacity);
const TacEmissionSite *tac_allocation_site(const TacAllocationResult *result,
                                          const TacInstruction *instruction);
const TacAllocationMetrics *tac_allocation_metrics(const TacAllocationResult *result,
                                                   const TacFunction *function);
void tac_allocation_free(TacAllocationResult *result);
#endif
