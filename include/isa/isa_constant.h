/*
 * isa_constant.h — selección pura para materializar constantes de 32 bits.
 * Usa: stdint.h. Lo usan gen_mov y la futura bajada a ISA virtual.
 * No elige registros ni emite instrucciones; Aho §8.1–8.2; isa.md §4.3.
 */
#ifndef ISA_CONSTANT_H
#define ISA_CONSTANT_H
#include <stdint.h>

typedef enum {
    ISA_CONST_MOV,
    ISA_CONST_SLL,
    ISA_CONST_SADDI
} IsaConstantOpcode;

typedef enum {
    ISA_CONST_DEST,       /* Destino solicitado por el consumidor. */
    ISA_CONST_SCRATCH     /* Único auxiliar de la secuencia larga. */
} IsaConstantRegister;

typedef struct {
    IsaConstantOpcode op;       /* Paso de materialización. */
    IsaConstantRegister target; /* Registro simbólico escrito. */
    int32_t immediate;          /* Inmediato; cero para SLL. */
} IsaConstantStep;

typedef struct {
    unsigned count;             /* Uno o cuatro pasos válidos. */
    unsigned scratch_count;     /* Cero o un auxiliar. */
    IsaConstantStep steps[4];   /* Secuencia en orden. */
} IsaConstantPlan;

/*
 * isa_constant_plan — selecciona la secuencia actual de gen_mov sin emitirla.
 *
 * Entrada:
 *   value: entero; out: salida escribible, no NULL.
 * Salida:
 *   1 con el plan; 0 fuera de INT32_MIN..UINT32_MAX, sin modificar *out.
 * Efectos:
 *   solo escribe *out al aceptar; sin reservas, diagnósticos ni estado global.
 * Restricciones:
 *   valores sobre INT32_MAX se interpretan con signo restando 2^32.
 *   SLL significa DEST = DEST << SCRATCH; SADDI, DEST += immediate.
 *   MOV escribe target. El plan largo conserva SADDI aunque el bajo sea cero.
 *   Registros físicos/virtuales, bundles y separaciones los decide el consumidor.
 *   Referencias: Aho §8.1–8.2; isa.md §4.2–4.3.
 */
int isa_constant_plan(int64_t value, IsaConstantPlan *out);

#endif
