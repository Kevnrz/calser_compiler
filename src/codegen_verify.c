#include "codegen_schedule.h"

/* Este verificador no consulta las aristas ni los tiempos internos del
 * organizador: reconstruye posiciones desde el calendario que va a emitirse. */
static int fail(CGScheduleIssue *out, CGVerifyKind kind, int operation,
                 int related, int bundle, int slot, uint16_t registers) {
    if (out) *out = (CGScheduleIssue) {kind, operation, related, bundle, slot, registers};
    return 1;
}
static int index_of(const CGInstruction *const *ops, int count, const CGInstruction *op) {
    for (int i = 0; i < count; i++) if (ops[i] == op) return i;
    return -1;
}
static int memory(const CGInstruction *op) {
    return (op->effects & (CG_EFFECT_MEMORY_READ | CG_EFFECT_MEMORY_WRITE)) != 0;
}
const char *cg_verify_kind_name(CGVerifyKind kind) {
    switch (kind) {
        case CG_VERIFY_OK: return "valido";
        case CG_VERIFY_INPUT: return "entrada invalida";
        case CG_VERIFY_MISSING: return "operacion omitida";
        case CG_VERIFY_DUPLICATE: return "operacion duplicada";
        case CG_VERIFY_FOREIGN: return "operacion ajena";
        case CG_VERIFY_SLOT: return "slot incompatible";
        case CG_VERIFY_DESTINATION: return "destino duplicado en un bundle";
        case CG_VERIFY_RAW: return "RAW: resultado leido demasiado pronto";
        case CG_VERIFY_WAR: return "WAR: escritura adelantada sobre una lectura";
        case CG_VERIFY_WAW: return "WAW: orden de escrituras alterado";
        case CG_VERIFY_MEMORY: return "orden de memoria alterado";
        case CG_VERIFY_CONTROL: return "salto adelantado sobre trabajo previo";
    }
    return "error desconocido";
}
int cg_verify_schedule(const CGInstruction *const *ops, int count,
                        const CGScheduledBundle *bundles, int bundle_count,
                        uint16_t previous_writes, CGScheduleIssue *issue) {
    if (issue) *issue = (CGScheduleIssue) {CG_VERIFY_OK, -1, -1, -1, -1, 0};
    if (count < 0 || count > CG_SCHEDULE_MAX_OPERATIONS || bundle_count < 0 ||
        bundle_count > CG_SCHEDULE_MAX_BUNDLES || (count && !ops) || (bundle_count && !bundles))
        return fail(issue, CG_VERIFY_INPUT, -1, -1, -1, -1, 0);
    int positions[CG_SCHEDULE_MAX_OPERATIONS], slots[CG_SCHEDULE_MAX_OPERATIONS];
    for (int i = 0; i < count; i++) {
        positions[i] = slots[i] = -1;
        if (!ops[i] || !ops[i]->known || !ops[i]->allowed_slots || (ops[i]->allowed_slots & ~15u) ||
            (ops[i]->writes & 1u) || ops[i]->result_distance != (ops[i]->writes ? 2u : 0u) ||
            (ops[i]->effects & (CG_EFFECT_BARRIER | CG_EFFECT_PC_RELATIVE)) ||
            ((ops[i]->effects & CG_EFFECT_CONTROL) && i != count - 1))
            return fail(issue, CG_VERIFY_INPUT, i, -1, -1, -1, 0);
        for (int j = 0; j < i; j++) if (ops[j] == ops[i])
            return fail(issue, CG_VERIFY_INPUT, i, j, -1, -1, 0);
    }
    uint16_t pending = previous_writes;
    for (int b = 0; b < bundle_count; b++) {
        uint16_t written = 0;
        for (int s = 0; s < 4; s++) {
            const CGInstruction *op = bundles[b].slots[s];
            if (!op) continue;
            int i = index_of(ops, count, op);
            if (i < 0) return fail(issue, CG_VERIFY_FOREIGN, -1, -1, b, s, 0);
            if (positions[i] >= 0) return fail(issue, CG_VERIFY_DUPLICATE, i, -1, b, s, 0);
            if (!(op->allowed_slots & (1u << s))) return fail(issue, CG_VERIFY_SLOT, i, -1, b, s, 0);
            if (written & op->writes) return fail(issue, CG_VERIFY_DESTINATION, i, -1, b, s, written & op->writes);
            if (pending & op->reads) return fail(issue, CG_VERIFY_RAW, i, -1, b, s, pending & op->reads);
            positions[i] = b; slots[i] = s;
            written |= op->writes;
        }
        pending = written;
    }
    for (int i = 0; i < count; i++) {
        if (positions[i] < 0) return fail(issue, CG_VERIFY_MISSING, i, -1, -1, -1, 0);
    }
    for (int i = 0; i < count; i++) for (int j = i + 1; j < count; j++) {
        uint16_t registers = ops[i]->writes & ops[j]->reads;
        if (registers && positions[j] < positions[i] + 2)
            return fail(issue, CG_VERIFY_RAW, j, i, positions[j], slots[j], registers);
        registers = ops[i]->reads & ops[j]->writes;
        if (registers && positions[j] < positions[i] + 1)
            return fail(issue, CG_VERIFY_WAR, j, i, positions[j], slots[j], registers);
        registers = ops[i]->writes & ops[j]->writes;
        if (registers && positions[j] < positions[i] + 1)
            return fail(issue, CG_VERIFY_WAW, j, i, positions[j], slots[j], registers);
        if (memory(ops[i]) && memory(ops[j]) && positions[j] < positions[i] + 1)
            return fail(issue, CG_VERIFY_MEMORY, j, i, positions[j], slots[j], 0);
        if ((ops[j]->effects & CG_EFFECT_CONTROL) && positions[j] < positions[i])
            return fail(issue, CG_VERIFY_CONTROL, j, i, positions[j], slots[j], 0);
    }
    return 0;
}
