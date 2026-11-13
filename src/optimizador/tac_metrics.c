/* Escritor único de métricas E; escapes compartidos con AST, Aho §9.2.5. */
#include "tac_metrics.h"
#include "tac_passes.h"
#include "json_text.h"
#include "codegen.h"
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* Entrada: ninguna. Salida: ms monotónicos. Efectos: consulta reloj.
 * Restricciones: cero si el sistema no proporciona reloj.
 */
double tac_metrics_clock(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0;
    return (double)now.tv_sec * 1000.0 + (double)now.tv_nsec / 1000000.0;
}
/* Entrada: programa vivo. Salida: cantidad de instrucciones. Efectos: ninguno.
 * Restricciones: no confundir PSEUDO con su expansión posterior.
 */
size_t tac_metrics_instruction_count(const TacProgram *program) {
    size_t count = 0;
    for (const TacFunction *f = program->first; f; f = f->next)
        for (const TacBlock *b = f->first; b; b = b->next)
            for (const TacInstruction *i = b->first; i; i = i->next)
                count++;
    return count;
}
/* Entrada: instrucción descrita. Salida: no-NOP. Efectos: ninguno.
 * Restricciones: el IR conserva texto canónico incluso después de organizar.
 */
static int non_nop(const CGInstruction *instruction) {
    return instruction->text && strcmp(instruction->text, "nopi") != 0;
}
/* Entrada: métricas, programa físico y plan. Salida: ninguna.
 * Efectos: agrega máximos y cuenta IR. Restricciones: codegen final todavía vivo.
 */
void tac_metrics_collect(TacMetrics *m, const TacProgram *program,
                         const TacAllocationResult *allocation) {
    for (const TacFunction *f = program->first; f; f = f->next) {
        const TacAllocationMetrics *a = tac_allocation_metrics(allocation, f);
        if (a->demand > m->demand)
            m->demand = a->demand;
        if (a->pressure > m->pressure)
            m->pressure = a->pressure;
        m->registers |= a->assigned_registers;
    }
    m->emitted = 0;
    for (int k = 0; k < cg_ir_count(); k++) {
        const CGIRRecord *record = cg_ir_record(k);
        if (record->kind == CG_IR_OPERATION)
            m->emitted += non_nop(&record->data.instruction);
        else if (record->kind == CG_IR_FIXED_BUNDLE)
            for (unsigned slot = 0; slot < 4; slot++)
                m->emitted += non_nop(&record->data.slots[slot]);
    }
    m->bundles = (size_t)cg_bundle_count();
}
/* Entrada: flujo/métricas y mapa HIGH o de tiempos. Salida: ninguna.
 * Efectos: escribe solo claves de pasadas ejecutadas. Restricciones: orden fijo.
 */
static void pass_map(FILE *out, const TacMetrics *m, int times) {
    int comma = 0;
    fputc('{', out);
    for (size_t k = 0; k < TAC_PASS_COUNT; k++) {
        const TacPass *pass = tac_pass_at(k);
        if (!(m->executed & (1u << k)) || (!times && pass->point != TAC_PASS_HIGH))
            continue;
        if (comma++)
            fputc(',', out);
        json_write_string(out, pass->name);
        if (times)
            fprintf(out, ":%.6f", m->pass_ms[k]);
        else
            fprintf(out, ":%zu", m->high_after[k]);
    }
    fputc('}', out);
}
/* Entrada: rutas reales, métricas y tiempo sin serialización. Salida: 1 o error.
 * Efectos: escribe/cierra JSON. Restricciones: compilación ya exitosa; no pisa fuente.
 */
int tac_metrics_write(const char *path, const char *source, const TacMetrics *m,
                       double total_ms) {
    struct stat input;
    struct stat output;
    if (stat(source, &input) == 0 && stat(path, &output) == 0 &&
        input.st_dev == output.st_dev && input.st_ino == output.st_ino) {
        fprintf(stderr, "%s: las métricas no pueden sobrescribir el fuente\n", source);
        return 0;
    }
    FILE *out = fopen(path, "w");
    if (!out) {
        fprintf(stderr, "%s: no se pudo escribir métricas en %s\n", source, path);
        return 0;
    }
    fputs("{\"programa\":", out);
    json_write_string(out, source);
    fputs(",\"nivel\":\"O1\",\"pasadas\":[", out);
    int comma = 0;
    for (size_t k = 0; k < TAC_PASS_COUNT; k++) {
        if (!(m->executed & (1u << k)))
            continue;
        if (comma++)
            fputc(',', out);
        json_write_string(out, tac_pass_at(k)->name);
    }
    fprintf(out, "],\"alto_nivel\":{\"inicial\":%zu,\"despues\":", m->high_initial);
    pass_map(out, m, 0);
    fprintf(out, "},\"isa_antes_expandir\":%zu,\"emitidas\":%zu,\"bundles\":%zu,"
        "\"nops\":%zu,\"bytes\":%zu,\"demanda\":%zu,\"presion\":%zu,"
        "\"mascara_registros\":\"0x%04x\",\"tiempo_ms\":{\"total\":%.6f,\"pasadas\":",
        m->isa_before, m->emitted, m->bundles, 4 * m->bundles - m->emitted,
        16 * m->bundles, m->demand, m->pressure, (unsigned)m->registers, total_ms);
    pass_map(out, m, 1);
    fputs("},\"contadores\":{\"desenrollados\":[],\"renombradas\":0,\"eliminadas\":0,"
        "\"bloques_eliminados\":0,\"reordenadas\":0,\"derramados\":0,"
        "\"cargas_derrame\":0,\"guardados_derrame\":0},\"ciclos_sim\":null}\n", out);
    int failed = ferror(out);
    if (fclose(out) != 0)
        failed = 1;
    if (failed) {
        remove(path);
        fprintf(stderr, "%s: error al escribir métricas en %s\n", source, path);
    }
    return !failed;
}
