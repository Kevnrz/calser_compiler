/* Auxiliares y preservación de C2; restricciones de P1, Aho §8.8 y §10.2.3.
 * Solo planes: D emitirá guardados en temp_base, preparación y restauración.
 */
#include "tac_allocate_internal.h"

/* Entrada: mapa y operandos aritméticos. Salida: máscara de fuentes/destino.
 * Efectos: ninguno. Restricciones: operandos TEMP válidos, todavía sin reescribir.
 */
static uint16_t operands_mask(const TacFunctionAllocation *f, const TacInstruction *i) {
    TacOperand values[] = {i->dest, i->src1, i->src2};
    uint16_t mask = 0;
    for (size_t k = 0; k < 3; k++)
        mask |= (uint16_t)(1u << f->registers[values[k].value.id]);
    return mask;
}

/* Entrada: mapa y conjunto de valores C1. Salida: máscara física de TEMP en el conjunto.
 * Efectos: ninguno. Restricciones: clobbers no se interpretan como definiciones.
 */
static uint16_t live_mask(const TacFunctionAllocation *f, const TacLiveAnalysis *live,
                          const TacLiveSet *set) {
    uint16_t mask = 0;
    for (size_t k = 0; k < tac_live_location_count(live); k++) {
        TacOperand value = tac_live_location(live, k);
        if (value.kind == TAC_TEMP && tac_live_contains(live, set, value))
            mask |= (uint16_t)(1u << f->registers[value.value.id]);
    }
    return mask;
}

/* Entrada: fuentes/destino prohibidos y vivos afectados. Salida: ocho auxiliares.
 * Efectos: ninguno. Restricciones: r4-r15; primero sin valor a preservar, empate por ID.
 */
static uint16_t scratch_mask(uint16_t forbidden, uint16_t live) {
    uint16_t chosen = 0;
    unsigned count = 0;
    for (unsigned occupied = 0; occupied < 2; occupied++) {
        for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST && count < 8; r++) {
            uint16_t bit = (uint16_t)(1u << r);
            if (!(forbidden & bit) && !!(live & bit) == occupied) {
                chosen |= bit;
                count++;
            }
        }
    }
    return chosen;
}

/* Entrada: función asignada y liveness C1, aún virtual. Salida: ninguna.
 * Efectos: agrega sitios y vectores propios; no modifica instrucciones ni emite.
 * Restricciones: preserva únicamente candidatos C1 en registros destruidos.
 */
void tac_allocate_sites(TacAllocationResult *a, TacFunctionAllocation *f,
                        const TacLiveAnalysis *live) {
    for (TacBlock *b = f->function->first; b; b = b->next) {
        for (TacInstruction *i = b->first; i; i = i->next) {
            const TacLiveInstruction *view = tac_live_instruction(live, i);
            if (!view->call_clobbers && !view->scratch_count)
                continue;
            TacEmissionSite *site = &a->sites[a->site_count++];
            site->instruction = i;
            uint16_t candidates = live_mask(f, live, view->preserve);
            if (view->scratch_count)
                site->scratch_mask = scratch_mask(operands_mask(f, i), candidates);
            uint16_t affected = candidates & (view->scratch_count ?
                site->scratch_mask : view->call_clobbers);
            TacRegisterBinding *saved = tac_analysis_array(12, sizeof *saved);
            site->preserved = saved;
            for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST; r++) {
                if (!(affected & (1u << r)))
                    continue;
                for (size_t k = 0; k < tac_live_location_count(live); k++) {
                    TacOperand value = tac_live_location(live, k);
                    if (value.kind == TAC_TEMP && f->registers[value.value.id] == r &&
                        tac_live_contains(live, view->preserve, value)) {
                        saved[site->preserved_count++] = (TacRegisterBinding){value.value.id, r};
                        break;
                    }
                }
            }
        }
    }
}
