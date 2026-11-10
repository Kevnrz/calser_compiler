/*
 * probar_semantica_isa.c — contratos de la semántica común en isa_table.
 *
 * Usa: isa_table.h; biblioteca C. Lo usa make check-semantica-isa/check-bundles.
 * Verifica todas las formas y los efectos sin interpretar texto ni bundles.
 * Referencias: isa.md §6, §9; Aho §10.2.3 (uso del destino preservado).
 */
#include <assert.h>
#include <stdio.h>
#include "isa_table.h"

/*
 * check — compara un descriptor con un contrato físico conocido.
 *
 * Entrada:
 *   name  mnemónico; d/a/b  registros normalizados o -1 si ausentes.
 *   reads/writes/preserves/effects  máscaras esperadas según isa.md §6.
 * Salida:
 *   ninguna; aborta si el descriptor difiere del contrato.
 * Efectos:
 *   ninguna modificación de tabla; ejecuta aserciones.
 * Restricciones:
 *   los valores esperados no se calculan a partir de la tabla probada.
 */
static void check(const char *name, int d, int a, int b,
                  unsigned reads, unsigned writes, unsigned preserves,
                  unsigned effects) {
    const IsaInstruction *instruction = isa_find(name);
    IsaPhysicalEffects physical;
    assert(instruction);
    assert(isa_physical_effects(instruction, d, a, b, 0, &physical));
    assert(physical.reads == reads);
    assert(physical.writes == writes);
    assert(physical.preserves == preserves);
    assert(physical.effects == effects);
    assert(physical.result_distance == (writes ? 2u : 0u));
}

/*
 * all_instructions — cubre los 34 mnemónicos y las 13 formas actuales.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; aborta ante una dependencia o efecto incorrecto.
 * Efectos:
 *   ejecuta aserciones.
 * Restricciones:
 *   máscaras físicas de r4/r5/r6 = 16/32/64; no incluye convenios de ccom.
 */
static void all_instructions(void) {
    const char *alu[] = {
        "add", "sub", "and", "or", "xor", "sll", "srl", "sra",
        "slt", "sle", "seq", "sne"
    };
    const char *branches[] = {"beq", "bne", "blt", "bge"};
    unsigned control = ISA_EFFECT_CONTROL;
    unsigned crypto = ISA_EFFECT_CRYPTO_STATE | ISA_EFFECT_BARRIER;
    for (size_t k = 0; k < sizeof alu / sizeof alu[0]; k++) {
        check(alu[k], 4, 5, 6, 96, 16, 0, ISA_EFFECT_NONE);
        check(alu[k], 4, 4, 4, 16, 16, 0, ISA_EFFECT_NONE);
        check(alu[k], 0, 0, 15, 32769, 0, 0, ISA_EFFECT_NONE);
    }
    check("ifmov", 4, 5, 6, 96, 16, 16, ISA_EFFECT_CONDITIONAL_WRITE);
    check("ifmov", 4, 4, 4, 16, 16, 16, ISA_EFFECT_CONDITIONAL_WRITE);
    check("ifmov", 0, 5, 6, 96, 0, 0, ISA_EFFECT_CONDITIONAL_WRITE);
    check("saddi", 4, 5, -1, 32, 16, 0, ISA_EFFECT_NONE);
    check("saddi", 4, 4, -1, 16, 16, 0, ISA_EFFECT_NONE);
    check("xori", 0, 0, -1, 1, 0, 0, ISA_EFFECT_NONE);
    check("mov", 4, -1, -1, 0, 16, 0, ISA_EFFECT_NONE);
    check("mov", 0, -1, -1, 0, 0, 0, ISA_EFFECT_NONE);
    for (size_t k = 0; k < sizeof branches / sizeof branches[0]; k++) {
        check(branches[k], -1, 4, 5, 48, 0, 0, control);
        check(branches[k], -1, 4, 4, 16, 0, 0, control);
    }
    check("jef", -1, -1, -1, 0, 0, 0, control);
    check("jas", 1, -1, -1, 0, 2, 0, control | ISA_EFFECT_CALL);
    check("jas", 0, -1, -1, 0, 0, 0, control | ISA_EFFECT_CALL);
    check("jefR", 0, 1, -1, 2, 0, 0, control);
    check("jefR", 4, 5, -1, 32, 16, 0, control | ISA_EFFECT_CALL);
    check("jefR", 1, 1, -1, 2, 2, 0, control | ISA_EFFECT_CALL);
    check("jefR", 0, 0, -1, 1, 0, 0, control);
    check("lw", 4, 5, -1, 32, 16, 0, ISA_EFFECT_MEMORY_READ);
    check("lb", 0, 0, -1, 1, 0, 0, ISA_EFFECT_MEMORY_READ);
    check("sw", -1, 5, 4, 48, 0, 0, ISA_EFFECT_MEMORY_WRITE);
    check("sb", -1, 4, 4, 16, 0, 0, ISA_EFFECT_MEMORY_WRITE);
    check("feis", 4, 5, 6, 96, 16, 16,
          crypto | ISA_EFFECT_CONDITIONAL_WRITE);
    check("feis", 4, 4, 4, 16, 16, 16,
          crypto | ISA_EFFECT_CONDITIONAL_WRITE);
    check("feis", 0, 5, 6, 96, 0, 0,
          crypto | ISA_EFFECT_CONDITIONAL_WRITE);
    check("auth", -1, 4, -1, 16, 0, 0,
          crypto | ISA_EFFECT_PIPELINE_STALL);
    check("sanc", -1, 4, -1, 16, 0, 0, crypto);
    check("kv_write", -1, 4, -1, 16, 0, 0, crypto);
    check("kv_lock", -1, -1, -1, 0, 0, 0, crypto);
    check("logout", -1, -1, -1, 0, 0, 0, crypto);
    check("nopi", -1, -1, -1, 0, 0, 0, ISA_EFFECT_NONE);
    assert(isa_find("ifmov")->semantics.preserves == ISA_REG_DEST);
    assert(isa_find("feis")->semantics.preserves == ISA_REG_DEST);
    assert(isa_find("jefR")->semantics.call_if_link_nonzero);
}

/*
 * numeric_targets — mantiene el efecto PC-relative solo en formas con destino.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; aborta si cambia la marca usada para conservar la emisión C0.
 * Efectos:
 *   ejecuta aserciones.
 * Restricciones:
 *   esta API no valida el rango ni resuelve el destino del salto.
 */
static void numeric_targets(void) {
    const char *names[] = {"beq", "bne", "blt", "bge", "jef", "jas"};
    for (size_t k = 0; k < sizeof names / sizeof names[0]; k++) {
        int d = k == 5 ? 1 : -1;
        int a = k < 4 ? 4 : -1;
        int b = k < 4 ? 5 : -1;
        IsaPhysicalEffects symbolic;
        IsaPhysicalEffects numeric;
        const IsaInstruction *instruction = isa_find(names[k]);
        assert(isa_physical_effects(instruction, d, a, b, 0, &symbolic));
        assert(isa_physical_effects(instruction, d, a, b, 1, &numeric));
        assert(numeric.effects == (symbolic.effects | ISA_EFFECT_PC_RELATIVE));
        assert(numeric.reads == symbolic.reads);
        assert(numeric.writes == symbolic.writes);
        assert(numeric.result_distance == symbolic.result_distance);
    }
}

/*
 * invalid_arguments — rechaza registros ausentes, sobrantes o fuera de rango.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; aborta si una entrada inválida produce efectos utilizables.
 * Efectos:
 *   ejecuta aserciones y comprueba inicialización de la salida.
 * Restricciones:
 *   no prueba sintaxis textual: corresponde a cg_describe_instruction.
 */
static void invalid_arguments(void) {
    const IsaInstruction *add = isa_find("add");
    const int invalid[][3] = {
        {-1, 5, 6}, {4, -1, 6}, {4, 5, -1},
        {16, 5, 6}, {4, 16, 6}, {4, 5, 16}, {-2, 5, 6}
    };
    IsaPhysicalEffects physical;
    for (size_t k = 0; k < sizeof invalid / sizeof invalid[0]; k++) {
        physical = (IsaPhysicalEffects){1, 2, 4, 8, 16};
        assert(!isa_physical_effects(add, invalid[k][0], invalid[k][1],
                                     invalid[k][2], 0, &physical));
        assert(!physical.reads && !physical.writes && !physical.preserves);
        assert(!physical.effects && !physical.result_distance);
    }
    assert(!isa_physical_effects(NULL, -1, -1, -1, 0, &physical));
    assert(!isa_physical_effects(add, 4, 5, 6, 0, NULL));
    assert(!isa_physical_effects(add, 4, 5, 6, 1, &physical));
    assert(!isa_physical_effects(isa_find("jefR"), 0, 1, -1, 1, &physical));
    assert(!isa_physical_effects(isa_find("mov"), 4, 5, -1, 0, &physical));
    assert(!isa_physical_effects(isa_find("sw"), 4, 5, 6, 0, &physical));
    assert(!isa_physical_effects(isa_find("nopi"), -1, -2, -1, 0, &physical));
    assert(!isa_find("desconocida"));
}

/*
 * main — ejecuta los contratos de semántica sin depender del compilador.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   0 tras superar todas las aserciones.
 * Efectos:
 *   imprime una línea de éxito.
 * Restricciones:
 *   compilar sin NDEBUG para conservar las aserciones.
 */
int main(void) {
    all_instructions();
    numeric_targets();
    invalid_arguments();
    puts("PASS semantica ISA: 34 instrucciones, 13 formas, preserves y efectos fisicos");
    return 0;
}
