/*
 * probar.c — selección manual: fronteras, operandos, control y protocolo de llamada.
 * Usa: tac_lower.h y anotaciones AST mínimas; no requiere parser ni emisor.
 * Referencias: Aho §7.2, §8.1–8.2 y §8.6.
 */
#include "tac_lower.h"
#include "ast.h"
#include <assert.h>
#include <string.h>

typedef struct {
    Symbol symbol;
    Scope scope;
    TacProgram *high;
    TacFunction *function;
    TacBlock *block;
    TacOperand a, b, d;
} Fixture;

/*
 * init — Prepara un main manual con tres virtuales; la fixture presta sus símbolos.
 *
 * Entrada:
 *   fixture escribible.
 * Salida:
 *   ninguna; deja un HIGH con función y bloque propios.
 * Efectos:
 *   reserva tres virtuales.
 * Restricciones:
 *   la fixture conserva vivos sus símbolos hasta liberar HIGH.
 */
static void init(Fixture *f) {
    f->symbol = (Symbol){.kind = SYMBOL_FUNCTION, .name = "main", .type = TYPE_VOID};
    f->scope = (Scope){.kind = SCOPE_GLOBAL};
    f->high = tac_program_new(&f->scope);
    f->function = tac_function_new(f->high, &f->symbol);
    f->block = tac_block_new(f->function);
    f->a = tac_temp_new(f->function);
    f->b = tac_temp_new(f->function);
    f->d = tac_temp_new(f->function);
}

/*
 * append — Agrega una fuente con posición que las instrucciones seleccionadas deben conservar.
 *
 * Entrada:
 *   fixture, opcode y operandos HIGH.
 * Salida:
 *   instrucción propia recién agregada.
 * Efectos:
 *   fija la posición 15:6 para verificar su propagación.
 * Restricciones:
 *   la instrucción debe pertenecer al bloque de la fixture.
 */
static TacInstruction *append(Fixture *f, TacOpcode op, TacOperand d, TacOperand a,
                               TacOperand b) {
    TacInstruction *instruction = tac_append(f->block, op, d, a, b);
    instruction->source_line = 15;
    instruction->source_column = 6;
    return instruction;
}

/*
 * lower — Completa RETURN y exige validación de ambos programas, sin convertir ISA en emisión.
 *
 * Entrada:
 *   fixture y valor de retorno HIGH.
 * Salida:
 *   programa ISA_VIRTUAL propio.
 * Efectos:
 *   completa RETURN, reconstruye, baja y valida.
 * Restricciones:
 *   la salida se libera antes de HIGH; Aho §8.6.
 */
static TacProgram *lower(Fixture *f, TacOperand returned) {
    TacInstruction *end = tac_append(f->block, TAC_RETURN, tac_none(), returned, tac_none());
    end->source_line = 23;
    end->source_column = 4;
    char error[256];
    assert(tac_rebuild_edges(f->function, error, sizeof error));
    TacProgram *isa = tac_lower(f->high, error, sizeof error);
    if (!isa) {
        fprintf(stderr, "selección manual fallida: %s\n", error);
        assert(0);
    }
    assert(!error[0] && isa->stage == TAC_STAGE_ISA_VIRTUAL);
    assert(tac_validate(isa, error, sizeof error));
    assert(tac_validate(f->high, error, sizeof error));
    return isa;
}

/*
 * at — Consulta una instrucción del bloque inicial y comprueba que existe.
 *
 * Entrada:
 *   programa válido e índice de instrucción.
 * Salida:
 *   instrucción prestada del primer bloque.
 * Efectos:
 *   aborta si el índice no existe.
 * Restricciones:
 *   el llamador conserva vivo el programa.
 */
static const TacInstruction *at(const TacProgram *program, size_t index) {
    const TacInstruction *instruction = program->first->first->first;
    while (instruction && index) {
        instruction = instruction->next;
        index--;
    }
    assert(instruction);
    return instruction;
}

/*
 * named — Verifica el mnemónico real por identidad de fila, nunca por el listado impreso.
 *
 * Entrada:
 *   instrucción y mnemónico real esperado.
 * Salida:
 *   ninguna.
 * Efectos:
 *   aborta si no coincide la fila de isa_table.
 * Restricciones:
 *   no compara cadenas del listado de salida.
 */
static void named(const TacInstruction *instruction, const char *name) {
    assert(instruction->op == TAC_ISA);
    assert(instruction->isa.instruction == isa_find(name));
}

/*
 * release — Libera primero la salida, según el contrato de símbolos compartidos.
 *
 * Entrada:
 *   fixture y programa seleccionado propios.
 * Salida:
 *   ninguna.
 * Efectos:
 *   libera ISA y luego HIGH.
 * Restricciones:
 *   los símbolos de la fixture sobreviven a ambos programas.
 */
static void release(Fixture *f, TacProgram *isa) {
    tac_program_free(isa);
    tac_program_free(f->high);
}

/*
 * arithmetic — Comprueba selección ALU completa, incluidas comparaciones con fuentes invertidas.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige mnemónicos y fuentes exactos.
 * Efectos:
 *   construye y libera casos ALU y unarios.
 * Restricciones:
 *   no ejecuta optimizaciones; Aho §8.2.
 */
static void arithmetic(void) {
    static const struct {
        TacOpcode op;
        const char *name;
        int swap;
    } cases[] = {
        {TAC_ADD, "add", 0}, {TAC_SUB, "sub", 0},
        {TAC_AND, "and", 0}, {TAC_OR, "or", 0}, {TAC_XOR, "xor", 0},
        {TAC_EQ, "seq", 0}, {TAC_NE, "sne", 0},
        {TAC_LT, "slt", 0}, {TAC_LE, "sle", 0},
        {TAC_GT, "slt", 1}, {TAC_GE, "sle", 1},
    };
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        Fixture f;
        init(&f);
        append(&f, cases[k].op, f.d, f.a, f.b);
        TacProgram *isa = lower(&f, tac_none());
        const TacInstruction *instruction = at(isa, 0);
        named(instruction, cases[k].name);
        assert(instruction->dest.value.id == f.d.value.id);
        assert(instruction->src1.value.id == (cases[k].swap ? f.b : f.a).value.id);
        assert(instruction->src2.value.id == (cases[k].swap ? f.a : f.b).value.id);
        assert(at(isa, 1)->pseudo.kind == TAC_PSEUDO_EXIT);
        release(&f, isa);
    }
    for (int negate = 0; negate < 2; negate++) {
        Fixture f;
        init(&f);
        append(&f, negate ? TAC_NEG : TAC_NOT, f.d, f.a, tac_none());
        TacProgram *isa = lower(&f, tac_none());
        const TacInstruction *instruction = at(isa, 0);
        named(instruction, negate ? "sub" : "seq");
        TacOperand zero = negate ? instruction->src1 : instruction->src2;
        TacOperand value = negate ? instruction->src2 : instruction->src1;
        assert(zero.kind == TAC_PHYS && zero.value.id == TAC_P1_RZ);
        assert(value.kind == TAC_TEMP && value.value.id == f.a.value.id);
        release(&f, isa);
    }
}

/*
 * constants — Las fronteras eligen inmediato o materialización; toda expansión conserva L:C.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige secuencias y posiciones de origen.
 * Efectos:
 *   construye y libera casos de COPY y ADD.
 * Restricciones:
 *   SLL lee el auxiliar virtual del plan compartido; Aho §8.2.
 */
static void constants(void) {
    const int64_t values[] = {0, -131072, 131071, -131073, 131072, INT32_MIN,
                              INT32_MAX, 200000, INT64_C(2147483648), UINT32_MAX};
    const unsigned lengths[] = {1, 1, 1, 4, 4, 4, 4, 4, 4, 1};
    for (size_t k = 0; k < sizeof values / sizeof values[0]; k++) {
        Fixture f;
        init(&f);
        append(&f, TAC_COPY, f.d, tac_integer(values[k]), tac_none());
        TacProgram *isa = lower(&f, tac_none());
        named(at(isa, 0), "mov");
        assert(at(isa, 0)->dest.value.id == f.d.value.id);
        if (lengths[k] == 4) {
            named(at(isa, 1), "mov");
            named(at(isa, 2), "sll");
            named(at(isa, 3), "saddi");
            assert(at(isa, 1)->dest.value.id == 3);
            assert(at(isa, 2)->src2.value.id == 3);
        }
        for (unsigned i = 0; i < lengths[k]; i++) {
            assert(at(isa, i)->source_line == 15 && at(isa, i)->source_column == 6);
        }
        assert(at(isa, lengths[k])->source_line == 23);
        assert(at(isa, lengths[k])->source_column == 4);
        assert(at(isa, lengths[k])->pseudo.kind == TAC_PSEUDO_EXIT);
        release(&f, isa);

        init(&f);
        append(&f, TAC_ADD, f.d, f.a, tac_integer(values[k]));
        isa = lower(&f, tac_none());
        int immediate = values[k] >= -131072 && values[k] <= 131071;
        named(at(isa, immediate ? 0 : lengths[k]), immediate ? "saddi" : "add");
        release(&f, isa);
    }
}

/*
 * memory_modes — Direcciones simbólicas, calculadas y fuera de 18 bits conservan los operandos.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige bases, datos y desplazamientos correctos.
 * Efectos:
 *   construye y libera casos LSU y ADDRESS.
 * Restricciones:
 *   direcciones y desplazamientos son bytes; Aho §8.6.
 */
static void memory_modes(void) {
    const int64_t offsets[] = {-131072, 131071, -131073, 131072};
    for (size_t k = 0; k < sizeof offsets / sizeof offsets[0]; k++) {
        Fixture f;
        init(&f);
        append(&f, TAC_LOAD, f.d, f.a, tac_integer(offsets[k]));
        TacProgram *isa = lower(&f, tac_none());
        const TacInstruction *load = at(isa, k < 2 ? 0 : 5);
        named(load, "lw");
        assert(load->dest.value.id == f.d.value.id);
        assert(load->isa.displacement == (k < 2 ? offsets[k] : 0));
        assert(load->src1.kind == TAC_TEMP);
        if (k >= 2) {
            named(at(isa, 4), "add");
            assert(at(isa, 4)->src1.value.id == f.a.value.id);
            assert(load->src1.value.id == at(isa, 4)->dest.value.id);
        }
        release(&f, isa);
    }
    Fixture f;
    init(&f);
    Symbol array = {.kind = SYMBOL_VARIABLE, .name = "a", .dimensions = 1, .address = 64};
    append(&f, TAC_ADDRESS, f.d, tac_variable(&array), tac_none());
    append(&f, TAC_LOAD, f.d, tac_variable(&array), tac_integer(4));
    append(&f, TAC_STORE, f.a, f.b, tac_integer(7));
    TacProgram *isa = lower(&f, tac_none());
    named(at(isa, 0), "mov");
    assert(at(isa, 0)->src2.value.integer == 64);
    named(at(isa, 1), "lw");
    assert(at(isa, 1)->src1.kind == TAC_PHYS && at(isa, 1)->src1.value.id == TAC_P1_RZ);
    assert(at(isa, 1)->isa.displacement == 68);
    named(at(isa, 2), "mov");
    named(at(isa, 3), "add");
    named(at(isa, 4), "sw");
    assert(at(isa, 4)->src1.value.id == at(isa, 3)->dest.value.id);
    assert(at(isa, 4)->src2.value.id == at(isa, 2)->dest.value.id);
    assert(at(isa, 4)->isa.displacement == 0);
    release(&f, isa);
    init(&f);
    array.address = 131072;
    append(&f, TAC_LOAD, f.d, tac_variable(&array), tac_integer(0));
    isa = lower(&f, tac_none());
    named(at(isa, 4), "lw");
    assert(at(isa, 4)->src1.kind == TAC_TEMP && at(isa, 4)->isa.displacement == 0);
    release(&f, isa);
}

/*
 * branches — LE/GT invierten fuentes; la arista tomada sigue antes que la caída.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige fuentes y aristas correctas.
 * Efectos:
 *   crea bloques fuera de orden y los baja.
 * Restricciones:
 *   conserva IDs HIGH; no ordena sucesores; Aho §8.4.1.
 */
static void branches(void) {
    const TacOpcode kinds[] = {TAC_IF_EQ, TAC_IF_NE, TAC_IF_LT, TAC_IF_LE, TAC_IF_GT, TAC_IF_GE};
    const char *names[] = {"beq", "bne", "blt", "bge", "blt", "bge"};
    for (size_t k = 0; k < sizeof kinds / sizeof kinds[0]; k++) {
        for (unsigned zero_side = 0; zero_side < 3; zero_side++) {
            Fixture f;
            init(&f);
            TacOperand a = zero_side == 1 ? tac_integer(0) : f.a;
            TacOperand b = zero_side == 2 ? tac_integer(0) : f.b;
            TacBlock *end = f.block;
            TacBlock *entry = tac_block_insert_after(f.function, NULL);
            TacBlock *fall = tac_block_insert_after(f.function, entry);
            tac_append(entry, kinds[k], tac_label(end), a, b);
            tac_append(fall, TAC_GOTO, tac_label(end), tac_none(), tac_none());
            TacProgram *isa = lower(&f, tac_none());
            const TacInstruction *branch = at(isa, 0);
            named(branch, names[k]);
            int swap = kinds[k] == TAC_IF_LE || kinds[k] == TAC_IF_GT;
            TacOperand left = zero_side == 1 ? tac_physical(TAC_P1_RZ) : f.a;
            TacOperand right = zero_side == 2 ? tac_physical(TAC_P1_RZ) : f.b;
            TacOperand first = swap ? right : left;
            TacOperand second = swap ? left : right;
            assert(branch->src1.kind == first.kind && branch->src1.value.id == first.value.id);
            assert(branch->src2.kind == second.kind && branch->src2.value.id == second.value.id);
            assert(isa->first->next_temp == f.function->next_temp);
            TacBlock *selected = isa->first->first;
            assert(branch->isa.target.value.block == isa->first->last);
            assert(selected->successors[0] == isa->first->last);
            assert(selected->successors[1] == selected->next);
            named(selected->next->first, "jef");
            assert(selected->next->first->isa.target.value.block == isa->first->last);
            assert(entry->label == 1 && fall->label == 2 && end->label == 0);
            assert(f.function->next_label == 3);
            release(&f, isa);
        }
    }
}

/*
 * calls_and_returns — Los PARAM fuera de orden se escriben según posición, sin pila ni preservación.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige protocolo de casillas y retorno.
 * Efectos:
 *   crea firmas y AST mínimos vivos durante la prueba.
 * Restricciones:
 *   no conserva registros ni rl; Aho §7.2.
 */
static void calls_and_returns(void) {
    Fixture f;
    init(&f);
    Symbol p0 = {.kind = SYMBOL_PARAMETER, .address = 32};
    Symbol p1 = {.kind = SYMBOL_PARAMETER, .address = 36};
    Symbol result = {.kind = SYMBOL_RETURN, .name = "r", .address = 40};
    Symbol *params[] = {&p0, &p1};
    ASTNode id = {.kind = AST_IDENTIFIER, .symbol = &result};
    ASTNode type = {.kind = AST_RETURN_TYPE, .child = &id};
    ASTNode declaration = {.kind = AST_FUNCTION, .child = &type};
    Symbol callee = {.kind = SYMBOL_FUNCTION, .name = "aux", .type = TYPE_INT,
                     .parameter_count = 2, .param_list = params, .decl_node = &declaration};
    append(&f, TAC_PARAM, tac_none(), f.a, tac_integer(1));
    append(&f, TAC_PARAM, tac_none(), f.b, tac_integer(0));
    append(&f, TAC_CALL, f.d, tac_function_operand(&callee), tac_integer(2));
    TacProgram *isa = lower(&f, tac_none());
    named(at(isa, 0), "sw");
    assert(at(isa, 0)->src2.value.id == f.a.value.id && at(isa, 0)->isa.displacement == 36);
    named(at(isa, 1), "sw");
    assert(at(isa, 1)->src2.value.id == f.b.value.id && at(isa, 1)->isa.displacement == 32);
    named(at(isa, 2), "jas");
    assert(at(isa, 2)->dest.value.id == TAC_P1_RL);
    assert(at(isa, 2)->isa.target.value.symbol == &callee);
    named(at(isa, 3), "lw");
    assert(at(isa, 3)->dest.value.id == f.d.value.id && at(isa, 3)->isa.displacement == 40);
    assert(at(isa, 4)->pseudo.kind == TAC_PSEUDO_EXIT);
    release(&f, isa);
    init(&f);
    append(&f, TAC_PARAM, tac_none(), tac_integer(0), tac_integer(1));
    append(&f, TAC_PARAM, tac_none(), tac_integer(0), tac_integer(0));
    append(&f, TAC_CALL, f.d, tac_function_operand(&callee), tac_integer(2));
    isa = lower(&f, tac_none());
    for (unsigned i = 0; i < 2; i++) {
        named(at(isa, i), "sw");
        assert(at(isa, i)->src2.kind == TAC_PHYS);
        assert(at(isa, i)->src2.value.id == TAC_P1_RZ);
        assert(at(isa, i)->isa.displacement == (i == 0 ? 36 : 32));
    }
    assert(isa->first->next_temp == f.function->next_temp);
    release(&f, isa);
    init(&f);
    f.symbol.name = "aux";
    f.symbol.type = TYPE_INT;
    f.symbol.decl_node = &declaration;
    isa = lower(&f, f.a);
    named(at(isa, 0), "sw");
    assert(at(isa, 0)->isa.displacement == 40);
    named(at(isa, 1), "jefR");
    assert(at(isa, 1)->dest.value.id == TAC_P1_RZ);
    assert(at(isa, 1)->src1.value.id == TAC_P1_RL);
    release(&f, isa);
    init(&f);
    f.symbol.name = "aux";
    f.symbol.decl_node = &declaration;
    isa = lower(&f, tac_variable(&result));
    named(at(isa, 0), "jefR");
    assert(!at(isa, 0)->next);
    release(&f, isa);
    init(&f);
    f.symbol.name = "aux";
    f.symbol.type = TYPE_INT;
    f.symbol.decl_node = &declaration;
    isa = lower(&f, tac_integer(0));
    named(at(isa, 0), "sw");
    assert(at(isa, 0)->src2.kind == TAC_PHYS && at(isa, 0)->src2.value.id == TAC_P1_RZ);
    assert(at(isa, 0)->isa.displacement == 40);
    named(at(isa, 1), "jefR");
    assert(isa->first->next_temp == f.function->next_temp);
    release(&f, isa);
}

/*
 * pseudos — Cada operación compleja conserva un único nodo PSEUDO y los selectores literales.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige discriminantes y selectores sin expandir.
 * Efectos:
 *   construye y libera las once operaciones complejas.
 * Restricciones:
 *   la salida de main agrega otra PSEUDO, EXIT; Aho §8.2.
 */
static void pseudos(void) {
    const struct {
        TacOpcode high;
        TacPseudoKind low;
    } cases[] = {
        {TAC_MUL, TAC_PSEUDO_MUL}, {TAC_DIV, TAC_PSEUDO_DIV},
        {TAC_MOD, TAC_PSEUDO_MOD}, {TAC_POW, TAC_PSEUDO_POW},
        {TAC_AUTH, TAC_PSEUDO_AUTH}, {TAC_LOGOUT, TAC_PSEUDO_LOGOUT},
        {TAC_KV_WRITE, TAC_PSEUDO_KV_WRITE}, {TAC_KV_LOCK, TAC_PSEUDO_KV_LOCK},
        {TAC_SANC, TAC_PSEUDO_SANC}, {TAC_CIFRAR, TAC_PSEUDO_CIFRAR},
        {TAC_DESCIFRAR, TAC_PSEUDO_DESCIFRAR},
    };
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        Fixture f;
        for (unsigned zeros = 0; zeros < 4; zeros++) {
            init(&f);
            const TacOpInfo *info = tac_op_info(cases[k].high);
            TacOperand d = (info->reads | info->writes) & TAC_DEST ? f.d : tac_none();
            TacOperand a = info->reads & TAC_SRC1 ? f.a : tac_none();
            TacOperand b = info->reads & TAC_SRC2 ? f.b : tac_none();
            if (zeros) {
                if (zeros == 1 && (info->reads & TAC_DEST)) {
                    d = tac_integer(0);
                }
                if (zeros != 3 && (info->reads & TAC_SRC1)) {
                    a = tac_integer(0);
                }
                if (zeros != 2 && (info->reads & TAC_SRC2)) {
                    b = tac_integer(0);
                }
            }
            TacInstruction *source = append(&f, cases[k].high, d, a, b);
            source->selectors.k = info->selectors & TAC_SELECT_K ? 2 : -1;
            source->selectors.w = info->selectors & TAC_SELECT_W ? 3 : -1;
            TacProgram *isa = lower(&f, tac_none());
            TacInstruction target = {.op = TAC_PSEUDO, .pseudo = {.kind = cases[k].low}};
            TacOpInfo selected;
            assert(tac_instruction_info(&target, &selected));
            TacOperand inputs[] = {d, a, b};
            unsigned prefix = 0;
            for (unsigned i = 0; i < 3; i++) {
                if (selected.scratch_count && (selected.reads & (1u << i)) &&
                    inputs[i].kind == TAC_INTEGER) {
                    prefix++;
                }
            }
            const TacInstruction *instruction = at(isa, prefix);
            assert(instruction->op == TAC_PSEUDO && instruction->pseudo.kind == cases[k].low);
            TacOperand operands[] = {instruction->dest, instruction->src1, instruction->src2};
            unsigned materialized = 0;
            for (unsigned i = 0; i < 3; i++) {
                if (!(selected.reads & (1u << i))) {
                    continue;
                }
                if (inputs[i].kind == TAC_TEMP) {
                    assert(operands[i].kind == TAC_TEMP);
                    assert(operands[i].value.id == inputs[i].value.id);
                } else if (selected.scratch_count) {
                    assert(operands[i].kind == TAC_TEMP);
                    named(at(isa, materialized), "mov");
                    assert(at(isa, materialized)->src2.value.integer == 0);
                    assert(at(isa, materialized)->dest.value.id == operands[i].value.id);
                    materialized++;
                } else {
                    assert(operands[i].kind == TAC_PHYS);
                    assert(operands[i].value.id == TAC_P1_RZ);
                }
            }
            assert(instruction->selectors.k == source->selectors.k);
            assert(instruction->selectors.w == source->selectors.w);
            assert(!instruction->pseudo.scratch_clobbers);
            assert(at(isa, prefix + 1)->pseudo.kind == TAC_PSEUDO_EXIT);
            assert(isa->first->next_temp == f.function->next_temp + prefix);
            release(&f, isa);
        }
    }
}

/*
 * zero_arithmetic — Comprueba fuentes rz en la ALU sin plegar operaciones.
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige fuentes, destinos y reservas de virtuales exactos.
 * Efectos:
 *   construye y libera casos ALU con cero.
 * Restricciones:
 *   el cero inmediato de SADDI y los destinos TEMP se conservan; Aho §8.1–8.2.
 */
static void zero_arithmetic(void) {
    const struct {
        TacOpcode op;
        const char *name;
        int swap;
    } cases[] = {
        {TAC_ADD, "add", 0}, {TAC_SUB, "sub", 0},
        {TAC_AND, "and", 0}, {TAC_OR, "or", 0}, {TAC_XOR, "xor", 0},
        {TAC_EQ, "seq", 0}, {TAC_NE, "sne", 0},
        {TAC_LT, "slt", 0}, {TAC_LE, "sle", 0},
        {TAC_GT, "slt", 1}, {TAC_GE, "sle", 1},
    };
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        for (unsigned side = 0; side < 2; side++) {
            Fixture f;
            init(&f);
            TacOperand a = side == 0 ? tac_integer(0) : f.a;
            TacOperand b = side == 1 ? tac_integer(0) : f.a;
            append(&f, cases[k].op, f.d, a, b);
            TacProgram *isa = lower(&f, tac_none());
            const TacInstruction *instruction = at(isa, 0);
            if (cases[k].op == TAC_ADD && side == 1) {
                named(instruction, "saddi");
                assert(instruction->src1.kind == TAC_TEMP);
                assert(instruction->src2.kind == TAC_INTEGER);
                assert(instruction->src2.value.integer == 0);
            } else {
                named(instruction, cases[k].name);
                TacOperand zero = (side == 0) != cases[k].swap ?
                    instruction->src1 : instruction->src2;
                TacOperand value = (side == 0) != cases[k].swap ?
                    instruction->src2 : instruction->src1;
                assert(zero.kind == TAC_PHYS && zero.value.id == TAC_P1_RZ);
                assert(value.kind == TAC_TEMP && value.value.id == f.a.value.id);
            }
            assert(instruction->dest.kind == TAC_TEMP);
            assert(instruction->dest.value.id == f.d.value.id);
            assert(isa->first->next_temp == f.function->next_temp);
            release(&f, isa);
        }
    }
    for (unsigned negate = 0; negate < 2; negate++) {
        Fixture f;
        init(&f);
        append(&f, negate ? TAC_NEG : TAC_NOT, f.d, tac_integer(0), tac_none());
        TacProgram *isa = lower(&f, tac_none());
        named(at(isa, 0), negate ? "sub" : "seq");
        assert(at(isa, 0)->src1.kind == TAC_PHYS);
        assert(at(isa, 0)->src2.kind == TAC_PHYS);
        assert(at(isa, 0)->src1.value.id == TAC_P1_RZ);
        assert(at(isa, 0)->src2.value.id == TAC_P1_RZ);
        assert(at(isa, 0)->dest.value.id == f.d.value.id);
        assert(isa->first->next_temp == f.function->next_temp);
        release(&f, isa);
    }
}

/*
 * zero_memory — Comprueba bases y datos cero sin perder definiciones ni identidad TEMP.
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige instrucciones y reservas exactas.
 * Efectos:
 *   construye y libera un caso de COPY, ADDRESS, LOAD y STORE.
 * Restricciones:
 *   conserva la identidad TEMP aunque su valor sea cero; Aho §8.1–8.2 y §8.6.
 */
static void zero_memory(void) {
    Fixture f;
    init(&f);
    Symbol scalar = {.kind = SYMBOL_VARIABLE, .name = "g", .address = 16};
    Symbol array = {.kind = SYMBOL_VARIABLE, .name = "a", .dimensions = 1, .address = 0};
    append(&f, TAC_COPY, tac_variable(&scalar), tac_integer(0), tac_none());
    append(&f, TAC_STORE, f.a, tac_integer(0), tac_integer(0));
    append(&f, TAC_LOAD, f.d, tac_variable(&array), f.a);
    append(&f, TAC_ADDRESS, f.b, tac_variable(&array), tac_none());
    append(&f, TAC_COPY, f.a, tac_integer(0), tac_none());
    append(&f, TAC_EQ, f.d, f.a, tac_integer(0));
    TacProgram *isa = lower(&f, tac_none());
    for (unsigned i = 0; i < 2; i++) {
        named(at(isa, i), "sw");
        assert(at(isa, i)->src2.kind == TAC_PHYS);
        assert(at(isa, i)->src2.value.id == TAC_P1_RZ);
        assert(at(isa, i)->isa.displacement == (i == 0 ? 16 : 0));
    }
    named(at(isa, 2), "add");
    assert(at(isa, 2)->src1.kind == TAC_PHYS && at(isa, 2)->src1.value.id == TAC_P1_RZ);
    assert(at(isa, 2)->src2.kind == TAC_TEMP && at(isa, 2)->src2.value.id == f.a.value.id);
    named(at(isa, 3), "lw");
    assert(at(isa, 3)->src1.value.id == at(isa, 2)->dest.value.id);
    for (unsigned i = 4; i < 6; i++) {
        named(at(isa, i), "mov");
        assert(at(isa, i)->dest.kind == TAC_TEMP);
        assert(at(isa, i)->dest.value.id == (i == 4 ? f.b : f.a).value.id);
        assert(at(isa, i)->src2.value.integer == 0);
    }
    named(at(isa, 6), "seq");
    assert(at(isa, 6)->src1.kind == TAC_TEMP && at(isa, 6)->src1.value.id == f.a.value.id);
    assert(at(isa, 6)->src2.kind == TAC_PHYS && at(isa, 6)->src2.value.id == TAC_P1_RZ);
    assert(isa->first->next_temp == f.function->next_temp + 1);
    release(&f, isa);
}

/*
 * failures — Los errores semánticos de anotación no dejan una salida parcial ni mutan HIGH.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   ninguna; exige errores con posición y rechazos de etapa.
 * Efectos:
 *   construye y libera HIGH manuales.
 * Restricciones:
 *   no corrige datos del productor.
 */
static void failures(void) {
    char error[256];
    assert(!tac_lower(NULL, error, sizeof error));
    assert(!tac_lower(NULL, NULL, 0));
    Fixture f;
    init(&f);
    append(&f, TAC_COPY, f.a, tac_integer(INT64_C(4294967296)), tac_none());
    tac_append(f.block, TAC_RETURN, tac_none(), tac_none(), tac_none());
    assert(tac_rebuild_edges(f.function, error, sizeof error));
    assert(!tac_lower(f.high, error, sizeof error));
    assert(strstr(error, "15:6") && strstr(error, "constante fuera de 32 bits"));
    assert(tac_validate(f.high, error, sizeof error));
    f.high->stage = TAC_STAGE_ISA_VIRTUAL;
    assert(!tac_lower(f.high, error, sizeof error));
    assert(strstr(error, "HIGH"));
    tac_program_free(f.high);
}

/*
 * main — Ejecuta contratos independientes del corpus y de la salida impresa.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   cero si se cumplen los contratos y stdout no falla.
 * Efectos:
 *   ejecuta pruebas y escribe un resumen.
 * Restricciones:
 *   aserciones activas; no requiere frontend ni emisor.
 */
int main(void) {
    arithmetic();
    constants();
    memory_modes();
    branches();
    calls_and_returns();
    pseudos();
    zero_arithmetic();
    zero_memory();
    failures();
    puts("PASS selección manual: ALU, constantes, direcciones, saltos, PARAM/CALL/RETURN y PSEUDO");
    return ferror(stdout) ? 1 : 0;
}
