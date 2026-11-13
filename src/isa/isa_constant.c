/*
 * isa_constant.c — descomposición compartida de constantes de 32 bits.
 * Usa: isa_constant.h; no depende de AST, TAC, GenCtx ni del emisor.
 * Lo usan gen_mov y la futura bajada a ISA virtual.
 * Referencias: Aho §8.1–8.2; isa.md §4.2–4.3.
 */
#include "isa_constant.h"
#include "isa_table.h"

/*
 * isa_constant_plan — elige MOV o MOV/MOV/SLL/SADDI con roles simbólicos.
 *
 * Entrada:
 *   value: entero; out: salida escribible, no NULL.
 * Salida:
 *   1 con plan completo; 0 fuera de rango, sin modificar *out.
 * Efectos:
 *   escribe *out solo al aceptar; ninguna emisión ni reserva.
 * Restricciones:
 *   conserva rango, normalización y descomposición de gen_mov; Aho §8.1–8.2.
 */
int isa_constant_plan(int64_t value, IsaConstantPlan *out) {
    if (value < INT32_MIN || value > UINT32_MAX) {
        return 0;
    }
    /* 2147483648 también llega como magnitud del operando de -2147483648. */
    if (value > INT32_MAX) {
        value -= INT64_C(4294967296);
    }
    IsaConstantPlan plan = {
        .count = 1,
        .steps = {{.op = ISA_CONST_MOV, .target = ISA_CONST_DEST,
                   .immediate = (int32_t)value}},
    };
    if (!isa_immediate_fits(value)) {
        int64_t high = value / 32768;
        int64_t low = value % 32768;
        if (low < 0) {
            high--;
            low += 32768;
        }
        plan.count = 4;
        plan.scratch_count = 1;
        plan.steps[0].immediate = (int32_t)high;
        plan.steps[1] = (IsaConstantStep){
            .op = ISA_CONST_MOV, .target = ISA_CONST_SCRATCH, .immediate = 15,
        };
        plan.steps[2] = (IsaConstantStep){
            .op = ISA_CONST_SLL, .target = ISA_CONST_DEST, .immediate = 0,
        };
        plan.steps[3] = (IsaConstantStep){
            .op = ISA_CONST_SADDI, .target = ISA_CONST_DEST, .immediate = (int32_t)low,
        };
    }
    *out = plan;
    return 1;
}
