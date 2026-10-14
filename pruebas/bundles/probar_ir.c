#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "codegen.h"

#define B(r) (1u << (r))
static CGInstruction describe(const char *text, Slot slot, unsigned reads,
                              unsigned writes, unsigned allowed) {
    CGInstruction ins;
    assert(cg_describe_instruction(text, slot, 27, &ins) == 0);
    assert(ins.known && ins.source_line == 27 && ins.original_slot == slot);
    assert(ins.reads == reads && ins.writes == writes && ins.allowed_slots == allowed);
    assert(ins.result_distance == (writes ? 2u : 0u));
    return ins;
}
int main(void) {
    const char *alu[] = {"add", "sub", "and", "or", "xor", "sll", "srl",
                        "sra", "slt", "sle", "seq", "sne", "ifmov"};
    char text[256];
    for (unsigned i = 0; i < sizeof alu / sizeof alu[0]; i++) {
        snprintf(text, sizeof text, "%s t0, r5, t2", alu[i]);
        for (int slot = 0; slot < 2; slot++) {
            CGInstruction ins = describe(text, (Slot) slot, B(5)|B(6), B(4), 3);
            assert(ins.unit == CG_UNIT_ALU);
            assert(ins.effects == (strcmp(alu[i], "ifmov") == 0 ? CG_EFFECT_CONDITIONAL_WRITE : 0u));
        }
    }
    describe("mov rl, -131072", SLOT_ALU0, 0, B(1), 3);
    describe("  saddi rx, rx, 4  ", SLOT_ALU1, B(2), B(2), 3);
    describe("xori rg, t11, 0x1F", SLOT_ALU0, B(15), B(3), 3);
    describe("add rz, rz, t0", SLOT_ALU0, B(0)|B(4), 0, 3);
    for (int slot = 0; slot < 4; slot++)
        describe("nopi", (Slot) slot, 0, 0, 15);
    const char *memory[] = {"lw", "lb", "sw", "sb"};
    for (int i = 0; i < 4; i++) {
        snprintf(text, sizeof text, "%s t0, -4 ( rx )", memory[i]);
        CGInstruction ins = describe(text, SLOT_LSU, B(2)|(i < 2 ? 0 : B(4)), i < 2 ? B(4) : 0, 8);
        assert(ins.effects == (i < 2 ? CG_EFFECT_MEMORY_READ : CG_EFFECT_MEMORY_WRITE));
    }
    const char *branches[] = {"beq", "bne", "blt", "bge"};
    for (int i = 0; i < 4; i++) {
        snprintf(text, sizeof text, "%s r4, r5, .L_fin", branches[i]);
        assert(describe(text, SLOT_BRU, B(4)|B(5), 0, 4).effects == CG_EFFECT_CONTROL);
    }
    assert(describe("jef -2", SLOT_BRU, 0, 0, 4).effects == (CG_EFFECT_CONTROL|CG_EFFECT_PC_RELATIVE));
    assert(describe("jas rl, .L_func_f", SLOT_BRU, 0, B(1), 4).effects == (CG_EFFECT_CONTROL|CG_EFFECT_CALL));
    assert(describe("jefR rz, rl", SLOT_BRU, B(1), 0, 4).effects == CG_EFFECT_CONTROL);
    assert(describe("jefR r4, r5", SLOT_BRU, B(5), B(4), 4).effects & CG_EFFECT_CALL);
    CGInstruction ins = describe("feis t0, t0, t1, 2, 3", SLOT_ALU1, B(4)|B(5), B(4), 2);
    assert(ins.effects == (CG_EFFECT_CRYPTO_STATE|CG_EFFECT_BARRIER|CG_EFFECT_CONDITIONAL_WRITE));
    ins = describe("auth t6", SLOT_ALU1, B(10), 0, 2);
    assert(ins.effects == (CG_EFFECT_CRYPTO_STATE|CG_EFFECT_BARRIER|CG_EFFECT_PIPELINE_STALL));
    describe("kv_write r10, 0, 3", SLOT_ALU1, B(10), 0, 2);
    describe("sanc r4", SLOT_ALU1, B(4), 0, 2);
    describe("kv_lock", SLOT_ALU1, 0, 0, 2);
    describe("logout", SLOT_ALU1, 0, 0, 2);
    const char *bad[] = {"desconocida r4", "add r4, r5", "add r16, r5, r6",
        "mov t12, 1", "mov r4, 131072", "sll r4, r4, 1", "add r4, r5, r6,",
        "auth", "feis r4, r4, r5, 0, 4", "lw r4, 0(r5) basura", "jef ?"};
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        assert(cg_describe_instruction(bad[i], SLOT_ALU0, 2, &ins) == 1);
        assert(!ins.known && ins.effects == CG_EFFECT_BARRIER && !ins.allowed_slots && !ins.reads && !ins.writes);
    }
    assert(cg_describe_instruction("lw r4, 0(r5)", SLOT_ALU0, 0, &ins) == 1);
    assert(cg_describe_instruction("auth r4", SLOT_ALU0, 0, &ins) == 1);
    assert(cg_describe_instruction("feis r4, r4, r5, 0, 4", SLOT_ALU1, 0, &ins) == 1);
    assert(cg_describe_instruction("kv_write r4, -1, 0", SLOT_ALU1, 0, &ins) == 1);
    assert(cg_describe_instruction("lw r4, 0(r5) basura", SLOT_LSU, 0, &ins) == 1);
    assert(cg_describe_instruction("jef ?", SLOT_BRU, 0, &ins) == 1);
    assert(cg_describe_instruction(NULL, SLOT_ALU0, 0, &ins) == 1);
    assert(cg_describe_instruction("nopi", (Slot) 8, 0, &ins) == 1);
    assert(cg_describe_instruction("nopi", SLOT_ALU0, 0, NULL) == 1);

    cg_init();
    cg_set_line(12);
    char label[] = "inicio";
    cg_label(label);
    strcpy(label, "cambio");
    cg_op(SLOT_ALU0, "mov t0, %d", 3);
    cg_gap();
    char manual[] = "add t1, t1, t2";
    cg_bundle(manual, "feis t3, t3, t4, 0, 1", NULL, "sw t5, 0(t6)");
    strcpy(manual, "modificado");
    assert(cg_ir_count() == 5 && cg_bundle_count() == 4 && cg_line_count() == 5);
    const CGIRRecord *r = cg_ir_record(0);
    assert(r->kind == CG_IR_LABEL && !strcmp(r->data.label, "inicio") && r->source_line == 12 && r->bundle_address == 0);
    r = cg_ir_record(1);
    assert(r->kind == CG_IR_OPERATION && r->data.instruction.known && r->bundle_address == 0);
    assert(!strcmp(r->data.instruction.text, "mov t0, 3"));
    assert(cg_ir_record(2)->data.gap_reason == CG_GAP_AUTOMATIC && cg_ir_record(2)->bundle_address == 1);
    assert(cg_ir_record(3)->data.gap_reason == CG_GAP_EXPLICIT && cg_ir_record(3)->bundle_address == 2);
    r = cg_ir_record(4);
    assert(r->kind == CG_IR_FIXED_BUNDLE && r->bundle_address == 3);
    assert(!strcmp(r->data.slots[0].text, "add t1, t1, t2"));
    assert(r->data.slots[1].unit == CG_UNIT_CRYPTO && r->data.slots[2].unit == CG_UNIT_NONE);
    assert(r->data.slots[3].effects == CG_EFFECT_MEMORY_WRITE);
    assert(!strcmp(cg_line(1), "{ mov t0, 3 | nopi | nopi | nopi }"));
    assert(!strcmp(cg_line(4), "{ add t1, t1, t2 | feis t3, t3, t4, 0, 1 | nopi | sw t5, 0(t6) }"));
    assert(cg_ir_record(-1) == NULL && cg_ir_record(5) == NULL);
    cg_set_line(0);
    for (int i = 0; i < 1000; i++) cg_op(SLOT_ALU0, "mov r4, %d", i);
    assert(cg_ir_count() == 2005);
    assert(cg_ir_record(2003)->source_line == 0);
    assert(cg_label_address("inicio") == 0);
    cg_free();
    cg_free();
    assert(cg_ir_count() == 0 && cg_ir_record(0) == NULL && cg_line_count() == 0);
    cg_init();
    cg_op(SLOT_BRU, "jef fin");
    assert(cg_ir_record(0)->bundle_address == 0 && cg_ir_record(0)->source_line == 0);
    cg_free();
    puts("PASS IR: ISA, dependencias, efectos, slots, fronteras, copia de textos y liberacion");
    return 0;
}
