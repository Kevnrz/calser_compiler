/*
 * probar_pseudo.c — contrato de destino para C3.3/C3.4, sin expandir ni emitir.
 * Usa: tac.h. Lo usa check-tac junto a los casos HIGH e ISA; no requiere Python.
 * Referencias: Aho §8.2, §8.6, §8.8 y §9.2.5.
 */
#include "tac.h"
#include <assert.h>
#include <string.h>

typedef struct {
    TacPseudoKind kind;
    unsigned reads;
    unsigned writes;
    unsigned selectors;
    int crypto;
    int memory;
    int trap;
} Case;

static const Case cases[] = {
    {TAC_PSEUDO_MUL, TAC_SRC1 | TAC_SRC2, TAC_DEST, 0, 0, 0, 0},
    {TAC_PSEUDO_DIV, TAC_SRC1 | TAC_SRC2, TAC_DEST, 0, 0, 0, 1},
    {TAC_PSEUDO_MOD, TAC_SRC1 | TAC_SRC2, TAC_DEST, 0, 0, 0, 1},
    {TAC_PSEUDO_POW, TAC_SRC1 | TAC_SRC2, TAC_DEST, 0, 0, 0, 1},
    {TAC_PSEUDO_AUTH, TAC_SRC1, 0, 0, 1, 0, 0},
    {TAC_PSEUDO_LOGOUT, 0, 0, 0, 1, 0, 0},
    {TAC_PSEUDO_KV_WRITE, TAC_SRC1, 0, TAC_SELECT_K | TAC_SELECT_W, 1, 0, 0},
    {TAC_PSEUDO_KV_LOCK, 0, 0, 0, 1, 0, 0},
    {TAC_PSEUDO_SANC, TAC_SRC1, 0, 0, 1, 0, 0},
    {TAC_PSEUDO_CIFRAR, TAC_DEST | TAC_SRC1 | TAC_SRC2, 0, TAC_SELECT_K, 1, 1, 0},
    {TAC_PSEUDO_DESCIFRAR, TAC_DEST | TAC_SRC1 | TAC_SRC2, 0, TAC_SELECT_K, 1, 1, 0},
    {TAC_PSEUDO_EXIT, 0, 0, 0, 0, 0, 0},
};

typedef struct {
    Symbol symbol;
    Scope scope;
    TacProgram *program;
    TacFunction *function;
    TacBlock *block;
    TacOperand values[3];
} Fixture;

/*
 * fixture_init — prepara tres valores y una función main.
 * Entrada:
 *   fixture escribible y etapa ISA.
 * Salida:
 *   fixture con programa propio y símbolos prestados.
 * Efectos:
 *   reserva programa, función, bloque y virtuales si corresponden.
 * Restricciones:
 *   fixture debe vivir hasta tac_program_free; Aho §10.2.3.
 */
static void fixture_init(Fixture *fixture, TacStage stage) {
    fixture->symbol = (Symbol){.name = "main", .kind = SYMBOL_FUNCTION};
    fixture->scope = (Scope){.kind = SCOPE_GLOBAL};
    fixture->program = tac_program_new(&fixture->scope);
    fixture->program->stage = stage;
    fixture->function = tac_function_new(fixture->program, &fixture->symbol);
    fixture->block = tac_block_new(fixture->function);
    for (size_t k = 0; k < 3; k++) {
        fixture->values[k] = stage == TAC_STAGE_ISA_PHYSICAL ?
                             tac_physical((unsigned)k + 4) : tac_temp_new(fixture->function);
    }
}

/*
 * append_case — construye un caso sin consultar la tabla productiva.
 * Entrada:
 *   fixture vivo y caso de referencia.
 * Salida:
 *   instrucción nueva con argumentos y selectores del caso.
 * Efectos:
 *   agrega al bloque; el caso EXIT termina su camino.
 * Restricciones:
 *   máscara física aritmética 0xff00; valores físicos r4–r6.
 */
static TacInstruction *append_case(Fixture *fixture, const Case *test) {
    TacOperand operands[3];
    for (unsigned k = 0; k < 3; k++) {
        operands[k] = (test->reads | test->writes) & (1u << k) ?
                      fixture->values[k] : tac_none();
    }
    TacSelectors selectors = {-1, -1, -1};
    if (test->selectors & TAC_SELECT_K) {
        selectors.k = 2;
    }
    if (test->selectors & TAC_SELECT_W) {
        selectors.w = 3;
    }
    uint16_t clobbers = test->writes && fixture->program->stage == TAC_STAGE_ISA_PHYSICAL ?
                        UINT16_C(0xff00) : 0;
    return tac_append_pseudo(fixture->block, test->kind, operands[0], operands[1],
                             operands[2], selectors, clobbers);
}

/*
 * valid — reconstruye y exige forma válida.
 * Entrada:
 *   fixture con instrucciones completas.
 * Salida:
 *   ninguna; aborta ante un rechazo inesperado.
 * Efectos:
 *   reconstruye aristas y conserva el diagnóstico ante fallo.
 * Restricciones:
 *   listas vivas y acíclicas; Aho §8.4.1.
 */
static void valid(Fixture *fixture) {
    char error[256];
    assert(tac_rebuild_edges(fixture->function, error, sizeof error));
    if (!tac_validate(fixture->program, error, sizeof error)) {
        fprintf(stderr, "PSEUDO inválida: %s\n", error);
        assert(0);
    }
    assert(!error[0]);
}

/*
 * invalid — exige rechazo por la misma causa al validar y reconstruir.
 * Entrada:
 *   fixture alterado y fragmento de diagnóstico esperado.
 * Salida:
 *   ninguna; aborta ante aceptación o causa inesperada.
 * Efectos:
 *   ejecuta ambos validadores; no corrige el programa.
 * Restricciones:
 *   la prueba debe restaurar la mutación antes de liberar.
 */
static void invalid(Fixture *fixture, const char *reason) {
    char error[256];
    assert(!tac_validate(fixture->program, error, sizeof error));
    assert(strstr(error, reason));
    assert(!tac_rebuild_edges(fixture->function, error, sizeof error));
    assert(strstr(error, reason));
}

/*
 * properties — verifica usos, definiciones y efectos por las consultas comunes.
 * Entrada:
 *   fixture, caso independiente e instrucción recién construida.
 * Salida:
 *   ninguna; aborta ante propiedades distintas de las esperadas.
 * Efectos:
 *   ninguno sobre el programa.
 * Restricciones:
 *   valores distintos por posición para comprobar el orden D/A/B; Aho §9.2.5.
 */
static void properties(const Fixture *fixture, const Case *test, TacInstruction *instruction) {
    TacOpInfo info;
    assert(tac_instruction_info(instruction, &info));
    assert(info.reads == test->reads && info.writes == test->writes);
    assert(info.selectors == test->selectors);
    assert(info.crypto_state == test->crypto);
    assert(info.memory_read == test->memory && info.memory_write == test->memory);
    assert(info.may_trap == test->trap);
    int exit = test->kind == TAC_PSEUDO_EXIT;
    assert(info.barrier == (test->crypto || exit));
    assert(info.nonremovable == (test->crypto || exit));
    assert(info.flow == (exit ? TAC_FLOW_RETURN : TAC_FLOW_NEXT));
    assert(info.uses_globals == exit && !info.defines_globals);
    assert(!info.implicit_phys_reads && !info.implicit_phys_writes && !info.address_bases);
    assert(info.scratch_count == (test->writes ? 8u : 0u));
    assert(info.call_clobbers == (test->crypto ? TAC_P1_CALL_CLOBBERS :
                                  instruction->pseudo.scratch_clobbers));
    assert(info.may_create_blocks == (test->writes != 0 || test->memory));
    TacOperand uses[3];
    TacOperand definitions[3];
    size_t count = tac_uses(instruction, uses);
    size_t index = 0;
    for (unsigned k = 0; k < 3; k++) {
        if (test->reads & (1u << k)) {
            assert(index < count);
            assert(uses[index].kind == fixture->values[k].kind);
            assert(uses[index++].value.id == fixture->values[k].value.id);
        }
    }
    assert(index == count);
    assert(tac_definitions(instruction, definitions) == (test->writes ? 1u : 0u));
    if (test->writes) {
        assert(definitions[0].kind == fixture->values[0].kind);
        assert(definitions[0].value.id == fixture->values[0].value.id);
    }
    assert(!instruction->source_line && !instruction->source_column);
}

/*
 * all_kinds — comprueba todas las PSEUDO y su listado sin expandirlas.
 * Entrada:
 *   etapa ISA virtual o física.
 * Salida:
 *   ninguna; aborta ante forma, impresión o control de emisión incorrectos.
 * Efectos:
 *   crea y libera un programa; imprime solo en un archivo temporal.
 * Restricciones:
 *   no entrega PSEUDO al ensamblador; Aho §8.6.
 */
static void all_kinds(TacStage stage) {
    Fixture fixture;
    fixture_init(&fixture, stage);
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        TacInstruction *instruction = append_case(&fixture, &cases[k]);
        properties(&fixture, &cases[k], instruction);
    }
    valid(&fixture);
    assert(!fixture.block->successor_count);
    char error[256];
    assert(!tac_validate_for_emission(fixture.program, error, sizeof error));
    assert(strstr(error, stage == TAC_STAGE_ISA_PHYSICAL ? "sin expandir" : "ISA_PHYSICAL"));
    assert(!tac_validate_for_emission(fixture.program, NULL, 0));
    if (stage == TAC_STAGE_ISA_VIRTUAL) {
        FILE *out = tmpfile();
        assert(out);
        tac_print(out, fixture.program);
        assert(!ferror(out));
        rewind(out);
        char text[1024] = {0};
        assert(fread(text, 1, sizeof text - 1, out) > 0);
        assert(!ferror(out) && feof(out));
        assert(!strcmp(text,
            "function main (entry B0)\nB0: successors []\n"
            "  pseudo.mul v0, v1, v2\n"
            "  pseudo.div v0, v1, v2\n"
            "  pseudo.mod v0, v1, v2\n"
            "  pseudo.pow v0, v1, v2\n"
            "  pseudo.auth v1\n"
            "  pseudo.logout\n"
            "  pseudo.kv_write v1, 2, 3\n"
            "  pseudo.kv_lock\n"
            "  pseudo.sanc v1\n"
            "  pseudo.cifrar v0, v1, v2, 2\n"
            "  pseudo.descifrar v0, v1, v2, 2\n"
            "  pseudo.exit\n\n"));
        assert(!fclose(out));
    }
    tac_program_free(fixture.program);
}

/*
 * invalid_forms — muta discriminantes, etapas, argumentos y metadatos.
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; aborta si se acepta alguna forma inválida.
 * Efectos:
 *   crea y libera una fixture virtual; comprueba usos repetidos.
 * Restricciones:
 *   no imprime ni modifica los esperados de HIGH/ISA.
 */
static void invalid_forms(void) {
    Fixture fixture;
    fixture_init(&fixture, TAC_STAGE_ISA_VIRTUAL);
    TacInstruction *instruction = append_case(&fixture, &cases[0]);
    TacInstruction *exit = append_case(&fixture, &cases[11]);
    valid(&fixture);
    fixture.program->stage = TAC_STAGE_HIGH;
    invalid(&fixture, "etapa");
    fixture.program->stage = TAC_STAGE_ISA_VIRTUAL;
    instruction->pseudo.kind = TAC_PSEUDO_NONE;
    invalid(&fixture, "inválida");
    instruction->pseudo.kind = TAC_PSEUDO_COUNT;
    invalid(&fixture, "inválida");
    instruction->pseudo.kind = (TacPseudoKind)-1;
    invalid(&fixture, "inválida");
    instruction->pseudo.kind = TAC_PSEUDO_MUL;
    instruction->src1 = tac_integer(1);
    invalid(&fixture, "operando");
    instruction->src1 = tac_physical(TAC_P1_RZ);
    invalid(&fixture, "temporales");
    instruction->src1 = fixture.values[1];
    instruction->selectors.k = 0;
    invalid(&fixture, "selector");
    instruction->selectors.k = -1;
    instruction->pseudo.scratch_clobbers = UINT16_C(0xff00);
    invalid(&fixture, "virtual");
    instruction->pseudo.scratch_clobbers = 0;
    TacOperand saved_dest = instruction->dest;
    instruction->dest = instruction->src1;
    invalid(&fixture, "destino aritmético PSEUDO coincide con fuente");
    instruction->dest = instruction->src2;
    invalid(&fixture, "destino aritmético PSEUDO coincide con fuente");
    instruction->dest = saved_dest;
    instruction->isa.target = tac_label(fixture.block);
    invalid(&fixture, "metadatos ISA");
    instruction->isa.target = tac_none();
    fixture.symbol.name = "aux";
    invalid(&fixture, "main");
    fixture.symbol.name = "main";
    TacInstruction *extra = append_case(&fixture, &cases[0]);
    invalid(&fixture, "terminador");
    assert(tac_remove(fixture.block, extra));
    instruction->src2 = instruction->src1;
    TacOperand values[3];
    assert(tac_uses(instruction, values) == 1);
    valid(&fixture);
    instruction->src2 = fixture.values[2];
    exit->dest = fixture.values[0];
    invalid(&fixture, "operando");
    exit->dest = tac_none();
    assert(!tac_op_info(TAC_PSEUDO));
    assert(!tac_append_pseudo(NULL, TAC_PSEUDO_EXIT, tac_none(), tac_none(), tac_none(),
                              (TacSelectors){-1, -1, -1}, 0));
    tac_program_free(fixture.program);
}

/*
 * physical_forms — comprueba auxiliares y el control previo a emisión.
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige máscara válida y programa real sin PSEUDO al emitir.
 * Efectos:
 *   crea, modifica y libera una fixture física.
 * Restricciones:
 *   respeta el contrato de registros de cg_arith_emit; Aho §8.8.
 */
static void physical_forms(void) {
    Fixture fixture;
    fixture_init(&fixture, TAC_STAGE_ISA_PHYSICAL);
    TacInstruction *instruction = append_case(&fixture, &cases[0]);
    TacInstruction *exit = append_case(&fixture, &cases[11]);
    valid(&fixture);
    const uint16_t invalid_masks[] = {0, UINT16_C(0x7f00), UINT16_C(0xff80),
                                      UINT16_C(0xfe01), UINT16_C(0xfe10)};
    for (size_t k = 0; k < sizeof invalid_masks / sizeof invalid_masks[0]; k++) {
        instruction->pseudo.scratch_clobbers = invalid_masks[k];
        invalid(&fixture, "ocho auxiliares");
    }
    instruction->pseudo.scratch_clobbers = UINT16_C(0xff00);
    instruction->dest = instruction->src1;
    invalid(&fixture, "coincide");
    instruction->dest = fixture.values[0];
    assert(tac_remove(fixture.block, instruction));
    char error[256];
    assert(!tac_validate_for_emission(fixture.program, error, sizeof error));
    assert(strstr(error, "sin expandir"));
    exit->pseudo.scratch_clobbers = UINT16_C(0xff00);
    invalid(&fixture, "no aritmética");
    exit->pseudo.scratch_clobbers = 0;
    assert(tac_remove(fixture.block, exit));
    TacInstruction *ret = tac_append_isa(fixture.block, isa_find("jefR"),
        tac_physical(TAC_P1_RZ), tac_physical(TAC_P1_RL), tac_none(), 0, tac_none(),
        (TacSelectors){-1, -1, -1});
    valid(&fixture);
    assert(tac_validate_for_emission(fixture.program, error, sizeof error));
    assert(!error[0]);
    ret->pseudo.kind = TAC_PSEUDO_AUTH;
    invalid(&fixture, "metadatos PSEUDO");
    ret->pseudo.kind = TAC_PSEUDO_NONE;
    assert(!tac_validate_for_emission(NULL, error, sizeof error));
    tac_program_free(fixture.program);
}

/*
 * per_kind_errors — prueba etapa HIGH, selectores y operandos de cada PSEUDO.
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; aborta si un discriminante omite alguna restricción.
 * Efectos:
 *   crea y libera una fixture independiente por operación.
 * Restricciones:
 *   cada mutación se restaura antes de continuar.
 */
static void per_kind_errors(void) {
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        Fixture fixture;
        fixture_init(&fixture, TAC_STAGE_ISA_VIRTUAL);
        TacInstruction *instruction = append_case(&fixture, &cases[k]);
        if (cases[k].kind != TAC_PSEUDO_EXIT) {
            append_case(&fixture, &cases[11]);
        }
        valid(&fixture);
        fixture.program->stage = TAC_STAGE_HIGH;
        invalid(&fixture, "etapa");
        fixture.program->stage = TAC_STAGE_ISA_VIRTUAL;
        instruction->selectors.round = 0;
        invalid(&fixture, "selector");
        instruction->selectors.round = -1;
        if (cases[k].selectors & TAC_SELECT_K) {
            instruction->selectors.k = -1;
            invalid(&fixture, "selector");
            instruction->selectors.k = 4;
            invalid(&fixture, "selector");
            instruction->selectors.k = 2;
        }
        if (cases[k].selectors & TAC_SELECT_W) {
            instruction->selectors.w = 4;
            invalid(&fixture, "selector");
            instruction->selectors.w = 3;
        }
        TacOperand *operands[] = {&instruction->dest, &instruction->src1, &instruction->src2};
        for (unsigned i = 0; i < 3; i++) {
            TacOperand saved = *operands[i];
            *operands[i] = tac_integer(0);
            invalid(&fixture, "operando");
            *operands[i] = saved;
        }
        valid(&fixture);
        tac_program_free(fixture.program);
    }
}

/*
 * main — ejecuta los contratos sin alterar las salidas de referencia.
 * Entrada:
 *   ninguna.
 * Salida:
 *   cero si todas las aserciones se cumplen.
 * Efectos:
 *   ejecuta casos válidos e inválidos, con impresión temporal.
 * Restricciones:
 *   aserciones activas; no requiere compilador ni emisor P1.
 */
int main(void) {
    all_kinds(TAC_STAGE_ISA_VIRTUAL);
    all_kinds(TAC_STAGE_ISA_PHYSICAL);
    invalid_forms();
    physical_forms();
    per_kind_errors();
    return 0;
}
