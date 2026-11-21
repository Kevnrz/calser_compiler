/* Auxiliares y preservación de C2; restricciones de P1, Aho §8.8 y §10.2.3.
 * Solo planes: D emitirá guardados en temp_base, preparación y restauración.
 */
#include "tac_allocate_internal.h"
#include "codegen_crypto.h"

/* Entrada: mapa y operandos aritméticos. Salida: máscara de fuentes/destino.
 * Efectos: ninguno. Restricciones: operandos TEMP válidos, todavía sin reescribir.
 */
static uint16_t operands_mask(const TacFunctionAllocation *f, const TacInstruction *i) {
    TacOperand values[] = {i->dest, i->src1, i->src2};
    uint16_t mask = 0;
    for (size_t k = 0; k < 3; k++)
        if (f->registers[values[k].value.id])
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
        if (value.kind == TAC_TEMP && f->registers[value.value.id] && tac_live_contains_at(live, set, k))
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

/* Entrada: operando virtual y mapa. Salida: si tiene casilla de derrame. */
static int spilled(const TacFunctionAllocation *f, TacOperand value) {
    return value.kind == TAC_TEMP && f->spill_slot && f->spill_slot[value.value.id] >= 0;
}

/* Registro prestado: primero sin valor vivo; nunca pisa fuentes/destino.
 * Entrada: máscara usada y vivos. Salida: r4-r15 o cero, diseño §11.6.
 */
static unsigned borrow(uint16_t used, uint16_t live) {
    for (int occupied = 0; occupied < 2; occupied++)
        for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST; r++)
            if (!(used & (1u << r)) && !!(live & (1u << r)) == occupied)
                return r;
    return 0;
}

/* Entrada: instrucción virtual y sitio. Salida: éxito o diagnóstico.
 * Efectos: planifica fuentes distintas/destino, no reescribe todavía: los
 * demás análisis requieren todo el programa virtual válido (§7.1, §11.6).
 */
static int plan_spill(TacFunctionAllocation *f, const TacLiveAnalysis *live,
                       const TacLiveInstruction *view, TacInstruction *i,
                       TacEmissionSite *site, uint16_t *borrowed,
                       char *error, size_t capacity) {
    TacOpInfo info;
    tac_instruction_info(i, &info);
    TacOperand values[] = {i->dest, i->src1, i->src2};
    unsigned mapped[3] = {0};
    uint16_t used = 0, sources = 0;
    for (size_t k = 0; k < 3; k++) {
        if (values[k].kind == TAC_PHYS)
            mapped[k] = (unsigned)values[k].value.id;
        else if (values[k].kind == TAC_TEMP && !spilled(f, values[k]))
            mapped[k] = f->registers[values[k].value.id];
        if (mapped[k]) {
            used |= (uint16_t)(1u << mapped[k]);
            if (info.reads & (1u << k))
                sources |= (uint16_t)(1u << mapped[k]);
        }
    }
    uint16_t after = live_mask(f, live, view->after);
    unsigned first_source = 0;
    int crypto = info.crypto_state && i->op == TAC_PSEUDO;
    for (size_t k = 0; k < 3; k++) {
        if (!(info.reads & (1u << k)) || !spilled(f, values[k]))
            continue;
        for (size_t j = 0; j < k; j++)
            if ((info.reads & (1u << j)) && tac_location_equal(values[j], values[k]))
                site->spill_registers[k] = site->spill_registers[j];
        if (site->spill_registers[k])
            continue;
        unsigned reg = 0;
        if (crypto) {
            const unsigned fixed[] = {CG_CRYPTO_REG_SOURCE, CG_CRYPTO_REG_DEST, CG_CRYPTO_REG_COUNT};
            reg = i->pseudo.kind == TAC_PSEUDO_CIFRAR || i->pseudo.kind == TAC_PSEUDO_DESCIFRAR ?
                  fixed[k] : CG_CRYPTO_REG_ARGUMENT;
        } else if (!info.scratch_count && !(used & (1u << TAC_P1_RG))) {
            reg = TAC_P1_RG;
        } else if (!info.scratch_count && k && mapped[0] && (info.writes & TAC_DEST) &&
                   !(sources & (1u << mapped[0])) && !(site->load_count > 1)) {
            reg = mapped[0];
        } else {
            reg = borrow(used, after);
            if (reg)
                *borrowed |= (uint16_t)(1u << reg);
        }
        if (!reg)
            return tac_error(error, capacity, "sin auxiliar para fuente derramada");
        if (!first_source)
            first_source = reg;
        site->spill_registers[k] = reg;
        site->loads[site->load_count++] = (TacSpillMove){reg,
            f->function->symbol->frame.spill_base + 4 * f->spill_slot[values[k].value.id]};
        used |= (uint16_t)(1u << reg);
    }
    for (size_t k = 0; k < 3; k++) {
        if (!(info.writes & (1u << k)) || !spilled(f, values[k]))
            continue;
        unsigned reg = !info.scratch_count ? (first_source ? first_source : TAC_P1_RG) : borrow(used, after);
        if (!reg)
            return tac_error(error, capacity, "sin auxiliar para destino derramado");
        if (info.scratch_count)
            *borrowed |= (uint16_t)(1u << reg);
        site->spill_registers[k] = reg;
        site->stores[site->store_count++] = (TacSpillMove){reg,
            f->function->symbol->frame.spill_base + 4 * f->spill_slot[values[k].value.id]};
    }
    f->metrics.spill_loads += site->load_count;
    f->metrics.spill_stores += site->store_count;
    return 1;
}

/* Entrada: función asignada y liveness C1, aún virtual. Salida: ninguna.
 * Efectos: agrega sitios y vectores propios; no modifica instrucciones ni emite.
 * Restricciones: preserva únicamente candidatos C1 en registros destruidos.
 */
int tac_allocate_sites(TacAllocationResult *a, TacFunctionAllocation *f,
                        const TacLiveAnalysis *live, char *error, size_t capacity) {
    for (TacBlock *b = f->function->first; b; b = b->next) {
        for (TacInstruction *i = b->first; i; i = i->next) {
            const TacLiveInstruction *view = tac_live_instruction(live, i);
            int has_spill = spilled(f, i->dest) || spilled(f, i->src1) || spilled(f, i->src2);
            if (!view->call_clobbers && !view->scratch_count && !has_spill)
                continue;
            TacEmissionSite *site = &a->sites[a->site_count++];
            site->instruction = i;
            uint16_t candidates = live_mask(f, live, view->preserve), borrowed = 0;
            if (has_spill && !plan_spill(f, live, view, i, site, &borrowed, error, capacity))
                return 0;
            if (view->scratch_count) {
                uint16_t operands = operands_mask(f, i);
                for (size_t k = 0; k < 3; k++)
                    if (site->spill_registers[k])
                        operands |= (uint16_t)(1u << site->spill_registers[k]);
                site->scratch_mask = scratch_mask(operands, candidates);
            }
            uint16_t affected = (candidates & (view->scratch_count ?
                site->scratch_mask : view->call_clobbers)) | (borrowed & live_mask(f, live, view->after));
            TacRegisterBinding *saved = tac_analysis_array(12, sizeof *saved);
            site->preserved = saved;
            for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST; r++) {
                if (!(affected & (1u << r)))
                    continue;
                for (size_t k = 0; k < tac_live_location_count(live); k++) {
                    TacOperand value = tac_live_location(live, k);
                    if (value.kind == TAC_TEMP && f->registers[value.value.id] == r &&
                        (tac_live_contains_at(live, view->preserve, k) ||
                         tac_live_contains_at(live, view->after, k))) {
                        saved[site->preserved_count++] = (TacRegisterBinding){value.value.id, r};
                        break;
                    }
                }
            }
        }
    }
    return 1;
}
