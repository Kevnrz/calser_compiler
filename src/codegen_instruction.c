#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"

/* Dependencias de isa.md §6. No resuelve destinos ni ensambla: una forma que
 * no se pueda describir queda como barrera, nunca como operación independiente.
 * Mantener esta tabla junto con las formas del ensamblador al ampliar la ISA. */
typedef enum {
    R3, IMM, MOV, BRANCH, JEFR, JEF, JAS, LOAD, STORE,
    FEIS, RS1, NONE, KV_WRITE
} Form;
typedef struct { const char *name; CGUnit unit; Form form; int operands; } Description;
static const Description descriptions[] = {
    {"add", CG_UNIT_ALU, R3, 3}, {"sub", CG_UNIT_ALU, R3, 3},
    {"and", CG_UNIT_ALU, R3, 3}, {"or", CG_UNIT_ALU, R3, 3},
    {"xor", CG_UNIT_ALU, R3, 3}, {"sll", CG_UNIT_ALU, R3, 3},
    {"srl", CG_UNIT_ALU, R3, 3}, {"sra", CG_UNIT_ALU, R3, 3},
    {"slt", CG_UNIT_ALU, R3, 3}, {"sle", CG_UNIT_ALU, R3, 3},
    {"seq", CG_UNIT_ALU, R3, 3}, {"sne", CG_UNIT_ALU, R3, 3},
    {"ifmov", CG_UNIT_ALU, R3, 3},
    {"saddi", CG_UNIT_ALU, IMM, 3}, {"xori", CG_UNIT_ALU, IMM, 3},
    {"mov", CG_UNIT_ALU, MOV, 2},
    {"beq", CG_UNIT_BRU, BRANCH, 3}, {"bne", CG_UNIT_BRU, BRANCH, 3},
    {"blt", CG_UNIT_BRU, BRANCH, 3}, {"bge", CG_UNIT_BRU, BRANCH, 3},
    {"jefR", CG_UNIT_BRU, JEFR, 2}, {"jef", CG_UNIT_BRU, JEF, 1},
    {"jas", CG_UNIT_BRU, JAS, 2},
    {"lw", CG_UNIT_LSU, LOAD, 2}, {"lb", CG_UNIT_LSU, LOAD, 2},
    {"sw", CG_UNIT_LSU, STORE, 2}, {"sb", CG_UNIT_LSU, STORE, 2},
    {"feis", CG_UNIT_CRYPTO, FEIS, 5},
    {"auth", CG_UNIT_CRYPTO, RS1, 1}, {"sanc", CG_UNIT_CRYPTO, RS1, 1},
    {"kv_write", CG_UNIT_CRYPTO, KV_WRITE, 3},
    {"kv_lock", CG_UNIT_CRYPTO, NONE, 0}, {"logout", CG_UNIT_CRYPTO, NONE, 0},
    {"nopi", CG_UNIT_NONE, NONE, 0}
};

static char *trim(char *p) {
    while (isspace((unsigned char) *p)) p++;
    char *end = p + strlen(p);
    while (end > p && isspace((unsigned char) end[-1])) end--;
    *end = '\0';
    return p;
}

static int integer(const char *text, long *out) {
    const char *p = text;
    if (*p == '-' || *p == '+') p++;
    int hex = p[0] == '0' && (p[1] == 'x' || p[1] == 'X');
    if (hex ? !isxdigit((unsigned char) p[2]) : !isdigit((unsigned char) *p))
        return 0;
    char *end;
    errno = 0;
    *out = strtol(text, &end, hex ? 16 : 10);
    return errno == 0 && *end == '\0';
}

static int reg(const char *text) {
    static const char *const aliases[] = {"rz", "rl", "rx", "rg"};
    for (int i = 0; i < 4; i++) if (strcmp(text, aliases[i]) == 0) return i;
    if ((text[0] != 'r' && text[0] != 't') || !isdigit((unsigned char) text[1]))
        return -1;
    char *end;
    errno = 0;
    long n = strtol(text + 1, &end, 10);
    int limit = text[0] == 't' ? 11 : 15;
    return errno == 0 && *end == '\0' && n >= 0 && n <= limit
        ? (int) n + (text[0] == 't' ? 4 : 0) : -1;
}
static uint16_t bit(int r) { return (uint16_t) (1u << r); }
static uint16_t written(int r) { return r == 0 ? 0 : bit(r); }
static int immediate(const char *p) {
    long n;
    return integer(p, &n) && n >= -131072 && n <= 131071;
}
static int selector(const char *p) {
    long n;
    return integer(p, &n) && n >= 0 && n <= 3;
}
static int target(const char *p, unsigned *effects) {
    long n;
    if (integer(p, &n)) { *effects |= CG_EFFECT_PC_RELATIVE; return 1; }
    if (!(isalpha((unsigned char) *p) || *p == '_' || *p == '.')) return 0;
    for (p++; *p; p++)
        if (!(isalnum((unsigned char) *p) || *p == '_' || *p == '.')) return 0;
    return 1;
}
static int address(char *p) {
    char *open = strchr(p, '('), *close = strchr(p, ')');
    if (!open || !close || close < open || *trim(close + 1) != '\0') return -1;
    *open = '\0'; *close = '\0';
    if (!immediate(trim(p))) return -1;
    return reg(trim(open + 1));
}

int cg_describe_instruction(const char *text, Slot slot, int source_line,
                            CGInstruction *out) {
    if (!out) return 1;
    *out = (CGInstruction) {.text = text, .unit = CG_UNIT_UNKNOWN,
        .original_slot = slot, .source_line = source_line > 0 ? source_line : 0,
        .effects = CG_EFFECT_BARRIER};
    if (!text || slot < SLOT_ALU0 || slot > SLOT_LSU || strlen(text) >= 256)
        return 1;
    char buffer[256];
    strcpy(buffer, text);
    char *name = trim(buffer), *rest = name;
    while (*rest && !isspace((unsigned char) *rest)) rest++;
    if (*rest) *rest++ = '\0';
    rest = trim(rest);
    const Description *desc = NULL;
    for (size_t i = 0; i < sizeof descriptions / sizeof descriptions[0]; i++)
        if (strcmp(name, descriptions[i].name) == 0) { desc = &descriptions[i]; break; }
    if (!desc) return 1;
    CGInstruction ins = *out;
    ins.unit = desc->unit;
    ins.effects = 0;
    switch (ins.unit) {
        case CG_UNIT_ALU: ins.allowed_slots = 3; break;
        case CG_UNIT_BRU: ins.allowed_slots = 4; ins.effects = CG_EFFECT_CONTROL; break;
        case CG_UNIT_LSU: ins.allowed_slots = 8; break;
        case CG_UNIT_CRYPTO:
            ins.allowed_slots = 2;
            ins.effects = CG_EFFECT_CRYPTO_STATE | CG_EFFECT_BARRIER;
            break;
        case CG_UNIT_NONE: ins.allowed_slots = 15; break;
        default: return 1;
    }
    if (!(ins.allowed_slots & (1u << slot))) return 1;
    char *args[5];
    int count = 0;
    if (*rest) for (;;) {
        if (count == 5) return 1;
        char *comma = strchr(rest, ',');
        if (comma) *comma = '\0';
        args[count] = trim(rest);
        if (!*args[count++]) return 1;
        if (!comma) break;
        rest = comma + 1;
    }
    if (count != desc->operands) return 1;
    int d, a, b;
    switch (desc->form) {
        case R3: case FEIS:
            d = reg(args[0]); a = reg(args[1]); b = reg(args[2]);
            if (d < 0 || a < 0 || b < 0) return 1;
            ins.reads = bit(a) | bit(b); ins.writes = written(d);
            if (desc->form == FEIS) {
                if (!selector(args[3]) || !selector(args[4])) return 1;
                ins.effects |= CG_EFFECT_CONDITIONAL_WRITE;
            }
            if (strcmp(name, "ifmov") == 0) ins.effects |= CG_EFFECT_CONDITIONAL_WRITE;
            break;
        case IMM: case MOV:
            d = reg(args[0]);
            if (d < 0 || !immediate(args[count - 1])) return 1;
            if (desc->form == IMM) {
                a = reg(args[1]); if (a < 0) return 1;
                ins.reads = bit(a);
            }
            ins.writes = written(d);
            break;
        case BRANCH:
            a = reg(args[0]); b = reg(args[1]);
            if (a < 0 || b < 0 || !target(args[2], &ins.effects)) return 1;
            ins.reads = bit(a) | bit(b);
            break;
        case JEFR:
            d = reg(args[0]); a = reg(args[1]);
            if (d < 0 || a < 0) return 1;
            ins.writes = written(d); ins.reads = bit(a);
            if (d != 0) ins.effects |= CG_EFFECT_CALL;
            break;
        case JAS:
            d = reg(args[0]);
            if (d < 0 || !target(args[1], &ins.effects)) return 1;
            ins.writes = written(d); ins.effects |= CG_EFFECT_CALL;
            break;
        case JEF:
            if (!target(args[0], &ins.effects)) return 1;
            break;
        case LOAD: case STORE:
            d = reg(args[0]); a = address(args[1]);
            if (d < 0 || a < 0) return 1;
            ins.reads = bit(a);
            if (desc->form == LOAD) {
                ins.writes = written(d); ins.effects |= CG_EFFECT_MEMORY_READ;
            } else {
                ins.reads |= bit(d); ins.effects |= CG_EFFECT_MEMORY_WRITE;
            }
            break;
        case RS1: case KV_WRITE:
            a = reg(args[0]); if (a < 0) return 1;
            ins.reads = bit(a);
            if (desc->form == KV_WRITE && (!selector(args[1]) || !selector(args[2]))) return 1;
            if (strcmp(name, "auth") == 0) ins.effects |= CG_EFFECT_PIPELINE_STALL;
            break;
        case NONE: break;
    }
    ins.result_distance = ins.writes ? 2 : 0;
    ins.known = 1;
    *out = ins;
    return 0;
}
