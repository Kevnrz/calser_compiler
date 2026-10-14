/*
 * codegen_schedule.c — Fase 4: planificación de operaciones en bundles.
 *
 * Construye dependencias y selecciona operaciones listas para cada slot.
 * Verifica el calendario completo antes de entregarlo al callback.
 *
 * Usa: codegen.h y codegen_verify.c.
 * Lo usa: codegen.c y pruebas del organizador.
 * Referencias: isa.md §3, §9; docs/organizacion_bundles.md.
 */

#include <stdio.h>
#include <stdlib.h>
#include "codegen_schedule.h"

/* ==========================================================================
 * DEPENDENCIAS
 * ======================================================================= */


/*
 * memory — consulta si una instrucción tiene efecto en memoria.
 *
 * Entrada:
 *   op  instrucción no NULL.
 * Salida:
 *   1 si lee/escribe memoria; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int memory(const CGInstruction *op) {
    return (op->effects & (CG_EFFECT_MEMORY_READ | CG_EFFECT_MEMORY_WRITE)) != 0;
}

/*
 * dependency — calcula distancia de dependencia entre dos operaciones.
 *
 * Entrada:
 *   a  operación anterior no NULL.
 *   b  operación posterior no NULL.
 * Salida:
 *   0 sin arista; distancia mínima + 1 si hay dependencia.
 *   Efectos: ninguno.
 * Restricciones:
 *   lectura tras escritura usa result_distance; WAR/WAW/memoria requieren
 *   distancia 1.
 */
static unsigned char dependency(const CGInstruction *a, const CGInstruction *b) {
    int distance = -1;
    if (a->writes & b->reads) distance = (int) a->result_distance;
    if ((a->reads & b->writes) || (a->writes & b->writes) || (memory(a) && memory(b)))
        if (distance < 1) distance = 1;
    if ((b->effects & CG_EFFECT_CONTROL) && distance < 0) distance = 0;
    return (unsigned char) (distance + 1);
}

/* ==========================================================================
 * PLANIFICACIÓN Y VERIFICACIÓN
 * ======================================================================= */


/* cg_schedule_block — véase el contrato en codegen_schedule.h. */
int cg_schedule_block(const CGInstruction *const *ops, int count,
                      uint16_t previous_writes, CGScheduleEmit emit, void *user) {
    if (!emit || count < 0 || count > CG_SCHEDULE_MAX_OPERATIONS || (count && !ops))
        return 1;
    for (int i = 0; i < count; i++) {
        if (!ops[i] || !ops[i]->known || !ops[i]->allowed_slots ||
            (ops[i]->effects & (CG_EFFECT_BARRIER | CG_EFFECT_PC_RELATIVE)) ||
            ((ops[i]->effects & CG_EFFECT_CONTROL) && i != count - 1)) return 1;
    }
    if (!count) return 0;
    unsigned char *edges = calloc((size_t) count * count, sizeof(*edges));
    int *placed = malloc((size_t) count * sizeof(*placed));
    int capacity = 2 * count + 1;
    CGScheduledBundle *calendar = calloc((size_t) capacity, sizeof(*calendar));
    if (!edges || !placed || !calendar) {
        fprintf(stderr, "ccom: memoria insuficiente (organizacion de bundles)\n");
        free(edges); free(placed); free(calendar);
        return 1;
    }
    for (int i = 0; i < count; i++) {
        placed[i] = -1;
        for (int j = 0; j < i; j++) edges[j * count + i] = dependency(ops[j], ops[i]);
    }
    int remaining = count, bundle_count = 0;
    for (int cycle = 0; remaining; cycle++) {
        if (cycle == capacity) {
            fprintf(stderr, "ccom: error interno: organizador sin progreso\n");
            free(edges); free(placed); free(calendar);
            return 1;
        }
        CGScheduledBundle bundle = {{NULL, NULL, NULL, NULL}};
        for (int i = 0; i < count; i++) {
            if (placed[i] >= 0 || (cycle == 0 && (ops[i]->reads & previous_writes))) continue;
            int ready = 1;
            for (int j = 0; j < i; j++) {
                unsigned char edge = edges[j * count + i];
                if (edge && (placed[j] < 0 || cycle < placed[j] + edge - 1)) {
                    ready = 0; break;
                }
            }
            if (!ready) continue;
            int slot = (int) ops[i]->original_slot;
            if (slot < 0 || slot > 3 || bundle.slots[slot] || !(ops[i]->allowed_slots & (1u << slot))) {
                for (slot = 0; slot < 4; slot++)
                    if (!bundle.slots[slot] && (ops[i]->allowed_slots & (1u << slot))) break;
            }
            if (slot == 4) continue;
            bundle.slots[slot] = ops[i]; placed[i] = cycle; remaining--;
        }
        calendar[bundle_count++] = bundle;
    }
    CGScheduleIssue issue;
    int failed = cg_verify_schedule(ops, count, calendar, bundle_count, previous_writes, &issue);
    if (failed) {
        int line = issue.operation >= 0 ? ops[issue.operation]->source_line : 0;
        fprintf(stderr, "ccom: error interno de bundles: %s; operacion %d, "
                "bundle %d, slot %d, registros 0x%04X, linea %d\n",
                cg_verify_kind_name(issue.kind), issue.operation, issue.bundle,
                issue.slot, (unsigned) issue.registers, line);
    } else {
        for (int i = 0; i < bundle_count; i++) emit(user, &calendar[i]);
    }
    free(edges); free(placed); free(calendar);
    return failed;
}
