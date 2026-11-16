/* Reordenamiento ISA virtual dentro de cada bloque: calendario por lista.
 * Usa propiedades, ubicaciones y edición compartidas; Aho §10.3–10.4,
 * diseño §11.5 y distancia mayor de la decisión §4.6.4.
 * Entrada: contexto ISA virtual válido. Salida: éxito o detalle, sin imprimir.
 * Efectos: permuta nodos existentes y acumula posiciones cambiadas.
 */
#include "tac_passes.h"
#include "tac_live.h"
#include <stdlib.h>

enum { REORDER_MAX_BLOCK = 256, REORDER_MAX_LOCATIONS = 8 };

typedef struct {
    TacInstruction *instruction;
    TacOpInfo info;
    TacOperand reads[REORDER_MAX_LOCATIONS];
    TacOperand writes[REORDER_MAX_LOCATIONS];
    size_t read_count, write_count;
    int accesses_memory, writes_memory;
    int memory_known;
    int64_t memory_address;
    IsaUnit unit;
    int alone;
    size_t priority, ready_at, cycle, pending;
    int placed;
} ReorderNode;

typedef struct { ReorderNode *node; int overflow; } LocationCollector;

/* Entrada: ubicación visitada y acumulador. Salida: ninguna.
 * Efectos: reúne ubicaciones distintas, incluidas las implícitas; Aho §10.2.3.
 * Restricciones: nunca trunca silenciosamente el límite de §11.5.
 */
static void collect_location(void *user, TacOperand location, int definition) {
    LocationCollector *collector = user;
    if (location.kind != TAC_TEMP && location.kind != TAC_PHYS && location.kind != TAC_VARIABLE)
        return;
    ReorderNode *node = collector->node;
    TacOperand *values = definition ? node->writes : node->reads;
    size_t *count = definition ? &node->write_count : &node->read_count;
    for (size_t k = 0; k < *count; k++)
        if (tac_location_equal(values[k], location))
            return;
    if (*count == REORDER_MAX_LOCATIONS)
        collector->overflow = 1;
    else
        values[(*count)++] = location;
}

/* Entrada: instrucción y función válidas; nodo inicialmente en cero.
 * Salida: éxito o detalle. Efectos: consulta sin alterar el IR.
 * Restricciones: PSEUDO no tiene isa.instruction; direcciones conocidas solo
 * para LOAD/STORE con base rz, diseño §7.5 y §11.5.
 */
static int fill_node(ReorderNode *node, TacFunction *function, TacInstruction *instruction,
                     char *detail, size_t capacity) {
    node->instruction = instruction;
    tac_instruction_info(instruction, &node->info);
    LocationCollector collector = {.node = node};
    tac_locations_visit(function, instruction, collect_location, &collector);
    if (collector.overflow) {
        if (capacity)
            snprintf(detail, capacity, "demasiadas ubicaciones para reordenamiento");
        return 0;
    }
    node->accesses_memory = node->info.memory_read || node->info.memory_write;
    node->writes_memory = node->info.memory_write;
    node->alone = instruction->op == TAC_PSEUDO;
    if (instruction->op == TAC_ISA) {
        node->unit = instruction->isa.instruction->unit;
        IsaForm form = instruction->isa.instruction->form;
        if ((form == ISA_FORM_LOAD || form == ISA_FORM_STORE) &&
            instruction->src1.kind == TAC_PHYS && instruction->src1.value.id == TAC_P1_RZ) {
            node->memory_known = 1;
            node->memory_address = instruction->isa.displacement;
        }
    }
    return 1;
}

/* Entrada: dos conjuntos de ubicaciones. Salida: intersección no vacía.
 * Efectos: ninguno. Restricciones: compara identidades, no nombres/direcciones.
 */
static int intersects(const TacOperand *a, size_t n, const TacOperand *b, size_t m) {
    for (size_t k = 0; k < n; k++)
        for (size_t j = 0; j < m; j++)
            if (tac_location_equal(a[k], b[j]))
                return 1;
    return 0;
}

/* Entrada: nodos en orden original. Salida: mayor distancia aplicable, §4.6.4.
 * Efectos: ninguno. Restricciones: barrera/terminador/fallo no rebajan RAW=2.
 */
static unsigned char dependency(const ReorderNode *a, const ReorderNode *b) {
    unsigned char distance = intersects(a->writes, a->write_count, b->reads, b->read_count) ? 2 : 0;
    int ordered = intersects(a->reads, a->read_count, b->writes, b->write_count) ||
                  intersects(a->writes, a->write_count, b->writes, b->write_count);
    if (a->accesses_memory && b->accesses_memory && (a->writes_memory || b->writes_memory) &&
        !(a->memory_known && b->memory_known && a->memory_address != b->memory_address))
        ordered = 1;
    if (b->info.flow != TAC_FLOW_NEXT || a->info.barrier || b->info.barrier)
        ordered = 1;
    if ((a->info.may_trap && (b->writes_memory || b->info.barrier)) ||
        (b->info.may_trap && (a->writes_memory || a->info.barrier)))
        ordered = 1;
    return distance ? distance : (unsigned char)ordered;
}

/* Entrada: nodos y matriz cero, orden de salida con capacidad n.
 * Salida: ninguna. Efectos: calcula DAG, prioridades y ciclos; Aho §10.3–10.4.
 * Restricciones: sin control de presión; empate por posición original.
 * Los cuatro bits de slots representan 2 ALU, 1 salto y 1 memoria (isa.md §3).
 */
static void schedule(ReorderNode *nodes, size_t n, unsigned char *distances,
                     TacInstruction **order) {
    for (size_t a = 0; a < n; a++)
        for (size_t b = a + 1; b < n; b++) {
            distances[a * n + b] = dependency(&nodes[a], &nodes[b]);
            nodes[b].pending += distances[a * n + b] != 0;
        }
    for (size_t a = n; a-- > 0;)
        for (size_t b = a + 1; b < n; b++) {
            size_t d = distances[a * n + b];
            if (d && d + nodes[b].priority > nodes[a].priority)
                nodes[a].priority = d + nodes[b].priority;
        }
    size_t placed = 0, cycle = 0;
    while (placed < n) {
        unsigned free_slots = 15;
        int occupied_alone = 0;
        for (;;) {
            size_t best = n;
            for (size_t k = 0; k < n; k++) {
                ReorderNode *node = &nodes[k];
                if (node->placed || node->pending || node->ready_at > cycle)
                    continue;
                if (node->alone ? free_slots != 15 : !(isa_unit_slots(node->unit) & free_slots))
                    continue;
                if (best == n || node->priority > nodes[best].priority)
                    best = k;
            }
            if (best == n || occupied_alone)
                break;
            ReorderNode *node = &nodes[best];
            node->cycle = cycle;
            node->placed = 1;
            placed++;
            if (node->alone)
                occupied_alone = 1;
            else {
                unsigned available = isa_unit_slots(node->unit) & free_slots;
                free_slots &= ~(available & (~available + 1u));
            }
            for (size_t b = best + 1; b < n; b++) {
                size_t d = distances[best * n + b];
                if (!d)
                    continue;
                nodes[b].pending--;
                if (cycle + d > nodes[b].ready_at)
                    nodes[b].ready_at = cycle + d;
            }
        }
        cycle++;
    }
    /* Orden estable por (ciclo, posición original), independiente del orden de selección. */
    size_t indices[REORDER_MAX_BLOCK];
    for (size_t k = 0; k < n; k++) {
        size_t p = k;
        while (p && nodes[indices[p - 1]].cycle > nodes[k].cycle) {
            indices[p] = indices[p - 1];
            p--;
        }
        indices[p] = k;
    }
    for (size_t k = 0; k < n; k++)
        order[k] = nodes[indices[k]].instruction;
}

/* Entrada: bloque ISA virtual válido. Salida: éxito y posiciones cambiadas.
 * Efectos: aplica la permutación por API, conserva instrucciones y bloques.
 * Restricciones: fuera de 3..256 no se transforma; diseño §11.5.
 */
static int reorder_block(TacFunction *function, TacBlock *block, size_t *changed,
                         char *detail, size_t capacity) {
    size_t n = 0;
    *changed = 0;
    for (TacInstruction *i = block->first; i; i = i->next)
        n++;
    if (n < 3 || n > REORDER_MAX_BLOCK)
        return 1;
    ReorderNode *nodes = calloc(n, sizeof *nodes);
    unsigned char *distances = calloc(n * n, sizeof *distances);
    TacInstruction **order = malloc(n * sizeof *order);
    int ok = nodes && distances && order;
    if (!ok && capacity)
        snprintf(detail, capacity, "sin memoria para reordenamiento");
    size_t k = 0;
    for (TacInstruction *i = block->first; i && ok; i = i->next)
        ok = fill_node(&nodes[k++], function, i, detail, capacity);
    if (ok) {
        schedule(nodes, n, distances, order);
        for (k = 0; k < n; k++)
            *changed += order[k] != nodes[k].instruction;
        if (*changed)
            ok = tac_block_set_order(block, order, n, detail, capacity);
    }
    free(nodes);
    free(distances);
    free(order);
    return ok;
}

int tac_pass_reorder(TacPassContext *context, char *detail, size_t capacity) {
    for (TacFunction *f = context->program->first; f; f = f->next)
        for (TacBlock *b = f->first; b; b = b->next) {
            size_t changed;
            if (!reorder_block(f, b, &changed, detail, capacity))
                return 0;
            context->metrics->counters.reordered += changed;
        }
    return 1;
}
