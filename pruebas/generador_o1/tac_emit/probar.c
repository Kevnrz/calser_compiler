/* Contratos D no cubiertos por el corpus: copias, preservación, ABI y límites.
 * Usa fixtures C1 y consultas del IR existente; Aho §7.2, §8.6 y §8.8.
 */
#include "../../representacion/tac_analysis/fixture.h"
#include "tac_emit.h"
#include "codegen.h"
#include "data_area.h"

/* Entrada: fixture virtual terminada, etiquetas y buffer. Salida: plan propio.
 * Efectos: valida, asigna y emite. Restricciones: conserva IR y símbolos prestados.
 */
static TacAllocationResult *emit(Fixture *f, const LabelTable *labels, char error[512]) {
    fixture_validate(f);
    TacAllocationResult *a = tac_allocate_registers(f->program, "ruta/prueba.cls", error, 512);
    assert(a);
    TacEmitInput input = {.program = f->program, .allocation = a,
        .labels = labels, .source_name = "ruta/prueba.cls"};
    data_area_reset();
    int address;
    data_area_alloc(128, &address);
    int ok = tac_emit_program(&input, error, 512);
    if (!ok)
        fprintf(stderr, "%s\n", error);
    assert(ok && !error[0]);
    return a;
}

/* Entrada: ninguna. Salida: ninguna. Efectos: comprueba cinco permutaciones y
 * casillas de preservación, sin implementar otro simulador de plantillas.
 * Restricciones: solo se reconstruyen las copias ADD antes del primer bundle fijo.
 */
static void permutations(void) {
    const int sources[][3] = {{11, 12, 13}, {7, 6, 9}, {7, 9, 6}, {7, 7, 7}, {0, 7, 9}};
    for (size_t test = 0; test < sizeof sources / sizeof sources[0]; test++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
        f.symbol.frame = (FrameLayout){.area_base = 16, .area_size = 96, .temp_base = 64};
        TacOperand values[10];
        for (size_t k = 0; k < 10; k++) {
            values[k] = tac_temp_new(f.function);
            fixture_isa(f.entry, "mov", values[k], tac_none(), tac_integer(100 + k), tac_none());
        }
        TacOperand arguments[3];
        for (size_t k = 0; k < 3; k++)
            arguments[k] = sources[test][k] ? values[sources[test][k] - 4] : tac_physical(0);
        tac_append_pseudo(f.entry, TAC_PSEUDO_CIFRAR, arguments[0], arguments[1], arguments[2],
                          (TacSelectors){0, -1, -1}, 0);
        fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[0], tac_none());
        fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), values[7], tac_none());
        fixture_exit(f.entry);
        LabelEntry error_label = {.role = LABEL_ARITH_ERROR, .name = ".L_arith_error"};
        LabelEntry end = {.role = LABEL_PROGRAM_END, .name = "fin_main", .next = &error_label};
        LabelEntry main = {.role = LABEL_FUNCTION, .name = ".L_func_main", .next = &end};
        LabelTable labels = {.entries = &main};
        char error[512];
        TacAllocationResult *a = emit(&f, &labels, error);
        unsigned regs[16] = {0};
        for (unsigned k = 0; k < 10; k++)
            regs[k + 4] = 100 + k;
        int fixed_seen = 0;
        unsigned saved = 0;
        unsigned restored = 0;
        for (int k = 0; k < cg_ir_count(); k++) {
            const CGIRRecord *record = cg_ir_record(k);
            if (record->kind == CG_IR_FIXED_BUNDLE && !fixed_seen) {
                const int dest[] = {6, 7, 9};
                for (size_t n = 0; n < 3; n++)
                    assert(regs[dest[n]] == (sources[test][n] ? (unsigned)(96 + sources[test][n]) : 0));
                assert(k && cg_ir_record(k - 1)->kind == CG_IR_GAP);
                fixed_seen = 1;
            }
            if (record->kind != CG_IR_OPERATION)
                continue;
            const char *text = record->data.instruction.text;
            char d[8], s[8];
            if (!fixed_seen && sscanf(text, "add %7[^,], %7[^,], rz", d, s) == 2)
                regs[isa_parse_register(d)] = regs[isa_parse_register(s)];
            saved += !strcmp(text, "sw t0, 64(rz)") || !strcmp(text, "sw t7, 92(rz)");
            restored += !strcmp(text, "lw t0, 64(rz)") || !strcmp(text, "lw t7, 92(rz)");
        }
        assert(fixed_seen && saved == 2 && restored == 2);
        assert(cg_label_address(".L_func_main") >= 0 && cg_label_address("fin_main") >= 0);
        tac_allocation_free(a);
        tac_program_free(f.program);
        cg_free();
    }
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba llamadas encadenadas y etiquetas.
 * Restricciones: rl se conserva incluso con cero temporales preservados.
 */
static void calls(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    ASTNode declarations[3] = {{0}};
    f.symbol.decl_node = &declarations[0];
    Symbol first = {.name = "first", .kind = SYMBOL_FUNCTION, .type = TYPE_VOID,
                    .decl_node = &declarations[1]};
    Symbol last = {.name = "last", .kind = SYMBOL_FUNCTION, .type = TYPE_VOID,
                   .decl_node = &declarations[2]};
    TacFunction *functions[] = {f.function, tac_function_new(f.program, &first),
                              tac_function_new(f.program, &last)};
    TacBlock *blocks[] = {f.entry, tac_block_new(functions[1]), tac_block_new(functions[2])};
    for (unsigned k = 0; k < 2; k++)
        fixture_isa(blocks[k], "jas", tac_physical(1), tac_none(), tac_none(),
                    tac_function_operand(functions[k + 1]->symbol));
    fixture_exit(f.entry);
    char error[512];
    for (unsigned k = 1; k < 3; k++) {
        fixture_isa(blocks[k], "jefR", tac_physical(0), tac_physical(1), tac_none(), tac_none());
        assert(tac_rebuild_edges(functions[k], error, sizeof error));
    }
    LabelEntry entries[5] = {
        {.node = &declarations[0], .role = LABEL_FUNCTION, .name = ".L_func_main"},
        {.node = &declarations[1], .role = LABEL_FUNCTION, .name = ".L_func_first"},
        {.node = &declarations[2], .role = LABEL_FUNCTION, .name = ".L_func_last"},
        {.role = LABEL_PROGRAM_END, .name = "fin_main"},
        {.role = LABEL_ARITH_ERROR, .name = ".L_arith_error"}
    };
    for (unsigned k = 0; k < 4; k++)
        entries[k].next = &entries[k + 1];
    LabelTable labels = {.entries = entries};
    TacAllocationResult *a = emit(&f, &labels, error);
    const char *protocol[] = {"sw rl, 0(rx)", "saddi rx, rx, 4", "jas rl, .L_func_first",
                              "saddi rx, rx, -4", "lw rl, 0(rx)"};
    unsigned at = 0;
    unsigned calls = 0;
    for (int k = 0; k < cg_ir_count(); k++) {
        const CGIRRecord *record = cg_ir_record(k);
        if (record->kind != CG_IR_OPERATION)
            continue;
        const char *text = record->data.instruction.text;
        if (!strcmp(text, protocol[at]) || (at == 2 && !strcmp(text, "jas rl, .L_func_last"))) {
            if (++at == 5) {
                calls++;
                at = 0;
            }
        }
    }
    assert(calls == 2 && at == 0);
    int addresses[3];
    for (unsigned k = 0; k < 3; k++) {
        char label[32];
        snprintf(label, sizeof label, ".L_o1_%u", k);
        addresses[k] = cg_label_address(label);
        assert(addresses[k] >= 0);
        for (unsigned n = 0; n < k; n++)
            assert(addresses[k] != addresses[n]);
    }
    TacEmitInput invalid = {.source_name = NULL};
    assert(!tac_emit_program(&invalid, error, sizeof error));
    assert(!strcmp(error, "emisión requiere source_name no NULL"));
    tac_allocation_free(a);
    tac_program_free(f.program);
    cg_free();
}

/* Entrada: ninguna. Salida: cero. Efectos: ejecuta contratos D y frontera ROM.
 * Restricciones: no genera 4097 bundles para probar una comparación de cantidades.
 */
int main(void) {
    char error[512];
    assert(tac_emit_check_rom(4096, "ruta/rom.cls", error, sizeof error));
    assert(!error[0]);
    assert(!tac_emit_check_rom(4097, "ruta/rom.cls", error, sizeof error));
    assert(!strcmp(error, "ruta/rom.cls:0:0: error de generacion: ROM excedida: 4097 bundles, máximo 4096"));
    permutations();
    calls();
    puts("PASS emisión D: permutaciones, preservación, rl, etiquetas, nombre y ROM");
    return 0;
}
