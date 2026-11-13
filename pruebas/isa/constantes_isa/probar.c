/*
 * probar.c — contrato puro de materialización de constantes, sin emisor.
 * Usa: isa_constant.h y biblioteca C. Lo usa make check-constantes-isa.
 * Referencias: Aho §8.1–8.2; isa.md §4.2–4.3.
 */
#include "isa_constant.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
 * step — comprueba el opcode, rol e inmediato de una posición del plan.
 *
 * Entrada:
 *   actual: paso válido; op/target/immediate: referencia independiente.
 * Salida:
 *   ninguna; aborta ante un paso inesperado.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   el inmediato de SLL es cero y no representa su cantidad de desplazamiento.
 */
static void step(const IsaConstantStep *actual, IsaConstantOpcode op,
                  IsaConstantRegister target, int32_t immediate) {
    assert(actual->op == op);
    assert(actual->target == target);
    assert(actual->immediate == immediate);
}

/*
 * execute — recompone la palabra aplicando los pasos y roles simbólicos.
 *
 * Entrada:
 *   plan válido con uno o cuatro pasos.
 * Salida:
 *   patrón de 32 bits del destino.
 * Efectos:
 *   ninguno sobre el plan ni estado externo.
 * Restricciones:
 *   usa uint32_t para evitar desborde con signo y desplazar negativos; Aho §8.2.
 */
static uint32_t execute(const IsaConstantPlan *plan) {
    uint32_t values[2] = {0, 0};
    for (unsigned k = 0; k < plan->count; k++) {
        const IsaConstantStep *current = &plan->steps[k];
        assert(current->target == ISA_CONST_DEST || current->target == ISA_CONST_SCRATCH);
        assert(current->immediate >= -131072 && current->immediate <= 131071);
        switch (current->op) {
            case ISA_CONST_MOV:
                values[current->target] = (uint32_t)current->immediate;
                break;
            case ISA_CONST_SLL:
                assert(values[ISA_CONST_SCRATCH] < 32);
                values[ISA_CONST_DEST] <<= values[ISA_CONST_SCRATCH];
                break;
            case ISA_CONST_SADDI:
                values[ISA_CONST_DEST] += (uint32_t)current->immediate;
                break;
            default:
                assert(0);
        }
    }
    return values[ISA_CONST_DEST];
}

/*
 * accepted — comprueba fronteras, normalización, pasos y auxiliares.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   cantidad de casos aprobados; aborta si difieren de la referencia.
 * Efectos:
 *   construye planes en memoria, sin emisión ni asignación de registros.
 * Restricciones:
 *   conserva SADDI con bajo cero y la corrección del resto negativo.
 */
static size_t accepted(void) {
    static const struct {
        int64_t input;
        unsigned count;
        int32_t first;
        int32_t low;
    } cases[] = {
        {0, 1, 0, 0},
        {131071, 1, 131071, 0},
        {-131071, 1, -131071, 0},
        {131072, 4, 4, 0},
        {-131072, 1, -131072, 0},
        {131073, 4, 4, 1},
        {-131073, 4, -5, 32767},
        {INT32_MIN, 4, -65536, 0},
        {INT32_MAX, 4, 65535, 32767},
        {200000, 4, 6, 3392},
        {INT64_C(2147483648), 4, -65536, 0},
        {UINT32_MAX, 1, -1, 0},
    };
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        IsaConstantPlan plan;
        assert(isa_constant_plan(cases[k].input, &plan));
        assert(plan.count == cases[k].count);
        step(&plan.steps[0], ISA_CONST_MOV, ISA_CONST_DEST, cases[k].first);
        if (cases[k].count == 1) {
            assert(plan.scratch_count == 0);
        } else {
            assert(plan.scratch_count == 1);
            step(&plan.steps[1], ISA_CONST_MOV, ISA_CONST_SCRATCH, 15);
            step(&plan.steps[2], ISA_CONST_SLL, ISA_CONST_DEST, 0);
            step(&plan.steps[3], ISA_CONST_SADDI, ISA_CONST_DEST, cases[k].low);
            assert(plan.steps[3].immediate >= 0 && plan.steps[3].immediate < 32768);
            int64_t recomposed = (int64_t)plan.steps[0].immediate * 32768 +
                                 plan.steps[3].immediate;
            assert(recomposed >= INT32_MIN && recomposed <= INT32_MAX);
            assert((uint32_t)recomposed == (uint32_t)cases[k].input);
        }
        assert(execute(&plan) == (uint32_t)cases[k].input);
    }
    return sizeof cases / sizeof cases[0];
}

/*
 * rejected — comprueba rechazo sin modificar siquiera los bytes de salida.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; aborta ante aceptación o modificación inesperada.
 * Efectos:
 *   construye un plan válido y conserva su representación para comparar.
 * Restricciones:
 *   el rango se comprueba antes de truncar o normalizar el valor.
 */
static void rejected(void) {
    const int64_t inputs[] = {-INT64_C(2147483649), INT64_C(4294967296)};
    IsaConstantPlan plan;
    assert(isa_constant_plan(200000, &plan));
    unsigned char before[sizeof plan];
    memcpy(before, &plan, sizeof plan);
    for (size_t k = 0; k < sizeof inputs / sizeof inputs[0]; k++) {
        assert(!isa_constant_plan(inputs[k], &plan));
        assert(!memcmp(before, &plan, sizeof plan));
    }
}

/*
 * main — ejecuta los contratos independientes del generador.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   cero si todas las aserciones se cumplen y stdout no falla.
 * Efectos:
 *   imprime un resumen, sin crear archivos.
 * Restricciones:
 *   aserciones activas; no requiere Python ni el compilador ccom.
 */
int main(void) {
    size_t count = accepted();
    rejected();
    printf("PASS constantes ISA: %zu casos, pasos, auxiliares y rechazos sin modificar salida\n",
           count);
    return ferror(stdout) ? 1 : 0;
}
