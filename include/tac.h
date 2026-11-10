/*
 * tac.h — representación intermedia de tres direcciones (C2.1a).
 *
 * Alto nivel: cuádruplas de Calser; ISA: operaciones descritas por isa_table.
 * Etapas: HIGH, ISA_VIRTUAL e ISA_PHYSICAL; no se mezclan al validar.
 * Los temporales virtuales no tienen el límite de doce registros físicos.
 * Impresión: TEMP tN en HIGH; TEMP vN en ISA; PHYS rz/rl/rx/rg/t0-t11.
 * El listado ISA es diagnóstico, no ensamblador completo para cg_op. C2.3
 * debe asignar etiquetas únicas (B2 local frente a .L_* del generador) y
 * emitir instrucción por instrucción. La equivalencia es de sintaxis, no
 * byte a byte: codegen_arith.c y codegen_array.c también emiten r%d.
 * El productor pasa el ámbito global del semántico, obligatorio y prestado.
 * Un ámbito global vacío en una prueba manual declara que no hay globales.
 * Propiedad: programa -> funciones -> bloques -> instrucciones y aristas.
 * Los Symbol, el ámbito global y las filas ISA son prestados: deben sobrevivir.
 * Las etiquetas referencian bloques de la misma función, no cadenas ni PC.
 * Las consultas de usos/definiciones devuelven solo valores explícitos.
 * Los análisis deben sumar globales y registros implícitos de TacOpInfo.
 * CALL/jas destruyen t0-t11 físicos (0xfff0), nunca todos los virtuales.
 * ifmov/feis usan y definen el destino, gracias a preserves de isa_table.
 * CALL/jas son barreras interiores: desviación deliberada de Aho §8.4.1.
 * Para armar: crear programa, funciones con Symbol, bloques y temporales;
 * agregar instrucciones, asignar selectores y reconstruir aristas; validar.
 * Recorrer funciones desde program->first usando function->next; bloques
 * desde function->first usando block->next; instrucciones desde block->first.
 * Se pueden editar operandos y usar las funciones de inserción/eliminación.
 * Antes de quitar la instrucción visitada, guardar next. No liberar su bloque.
 * Después de editar control u orden, reconstruir aristas y volver a validar.
 * Impresión estable; la forma todavía no se construye desde el AST.
 * Referencias: Aho §6.2, §8.4.1, §10.2.3; enunciado §4.1.
 */
#ifndef TAC_H
#define TAC_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "symbol_table.h"
#include "isa_table.h"

typedef struct TacProgram TacProgram;
typedef struct TacFunction TacFunction;
typedef struct TacBlock TacBlock;
typedef struct TacInstruction TacInstruction;

typedef enum {
    TAC_STAGE_HIGH, TAC_STAGE_ISA_VIRTUAL, TAC_STAGE_ISA_PHYSICAL
} TacStage;
typedef enum {
    TAC_NONE, TAC_TEMP, TAC_VARIABLE, TAC_INTEGER, TAC_LABEL,
    TAC_FUNCTION, TAC_PHYS
} TacOperandKind;
typedef struct {
    TacOperandKind kind;
    union {
        size_t id;
        int64_t integer;
        const Symbol *symbol;
        const TacBlock *block;
    } value;
} TacOperand;
typedef enum {
    TAC_COPY, TAC_ADDRESS,
    TAC_ADD, TAC_SUB, TAC_AND, TAC_OR, TAC_XOR,
    TAC_EQ, TAC_NE, TAC_LT, TAC_LE, TAC_GT, TAC_GE, TAC_NEG, TAC_NOT,
    TAC_MUL, TAC_DIV, TAC_MOD, TAC_POW, TAC_LOAD, TAC_STORE,
    TAC_AUTH, TAC_LOGOUT, TAC_KV_WRITE, TAC_KV_LOCK, TAC_SANC,
    TAC_CIFRAR, TAC_DESCIFRAR, TAC_PARAM, TAC_CALL, TAC_RETURN, TAC_GOTO,
    TAC_IF_EQ, TAC_IF_NE, TAC_IF_LT, TAC_IF_LE, TAC_IF_GT, TAC_IF_GE,
    TAC_ISA, TAC_OP_COUNT
} TacOpcode;
enum { TAC_DEST = 1u, TAC_SRC1 = 2u, TAC_SRC2 = 4u };
enum { TAC_SELECT_K = 1u, TAC_SELECT_W = 2u, TAC_SELECT_ROUND = 4u };
typedef struct { int k, w, round; } TacSelectors;
typedef enum {
    TAC_FLOW_NEXT, TAC_FLOW_GOTO, TAC_FLOW_IF, TAC_FLOW_RETURN
} TacFlow;
typedef enum {
    TAC_FMT_COPY, TAC_FMT_ADDRESS, TAC_FMT_BINARY, TAC_FMT_UNARY,
    TAC_FMT_LOAD, TAC_FMT_STORE, TAC_FMT_CRYPTO, TAC_FMT_PARAM, TAC_FMT_CALL,
    TAC_FMT_RETURN, TAC_FMT_GOTO, TAC_FMT_IF, TAC_FMT_ISA
} TacFormat;
typedef struct {
    const char *text;
    unsigned reads, writes, address_bases, allowed[3], selectors;
    int memory_read, memory_write, barrier, nonremovable, crypto_state;
    int may_trap, may_create_blocks, uses_globals, defines_globals;
    uint16_t implicit_phys_reads, implicit_phys_writes, call_clobbers;
    TacFlow flow;
    TacFormat format;
} TacOpInfo;
typedef struct {
    const IsaInstruction *instruction;
    int64_t displacement;
    TacOperand target;
} TacIsaData;
struct TacInstruction {
    TacOpcode op;
    TacOperand dest, src1, src2;
    TacSelectors selectors;              /* -1 cuando no aplica */
    TacIsaData isa;                       /* solo para TAC_ISA */
    int source_line;
    TacInstruction *next;
};
struct TacBlock {
    size_t label;
    TacFunction *function;
    TacInstruction *first, *last;
    TacBlock **successors, **predecessors;
    size_t successor_count, predecessor_count;
    TacBlock *next;
};
struct TacFunction {
    const Symbol *symbol;
    TacProgram *program;
    size_t next_temp, next_label;
    TacBlock *first, *last, *entry;
    TacFunction *next;
};
struct TacProgram {
    TacStage stage;
    const Scope *global_scope;
    TacFunction *first, *last;
};
enum {
    TAC_P1_RZ = 0, TAC_P1_RL = 1, TAC_P1_RX = 2, TAC_P1_RG = 3,
    TAC_P1_TEMP_FIRST = 4, TAC_P1_TEMP_LAST = 15
};
#define TAC_P1_CALL_CLOBBERS UINT16_C(0xfff0)

/* Contratos completos por función en las implementaciones.
 * Constructores: argumentos vivos/no NULL; global_scope debe ser SCOPE_GLOBAL.
 * tac_program_new con NULL/no GLOBAL termina con diagnóstico, nunca devuelve
 * NULL; no probar ese caso en el mismo proceso.
 * Sin memoria o IDs size_t: diagnóstico y termina; nunca limita a doce TEMP.
 * Las instrucciones nuevas tienen selectores {-1,-1,-1} y línea cero.
 */
TacOperand tac_none(void);
TacOperand tac_variable(const Symbol *symbol);
TacOperand tac_integer(int64_t value);
TacOperand tac_label(const TacBlock *block);
TacOperand tac_function_operand(const Symbol *symbol);
TacOperand tac_physical(unsigned register_number);
TacOperand tac_temp_new(TacFunction *function);
int tac_is_global(const TacProgram *program, TacOperand operand);
const TacOpInfo *tac_op_info(TacOpcode op);
int tac_instruction_info(const TacInstruction *instruction, TacOpInfo *out);
/* Capacidad 3; sin duplicados; 0 si instrucción/op inválida. Solo explícitos.
 * Orden dest/src1/src2. PHYS rz nunca es definición. No omitir los resúmenes
 * de globales, implicit_phys_* y call_clobbers al implementar un análisis.
 */
size_t tac_definitions(const TacInstruction *instruction, TacOperand out[3]);
size_t tac_uses(const TacInstruction *instruction, TacOperand out[3]);
TacProgram *tac_program_new(const Scope *global_scope);
TacFunction *tac_function_new(TacProgram *program, const Symbol *symbol);
TacBlock *tac_block_new(TacFunction *function);
/* previous NULL = inicio y actualiza entry; previo ajeno = NULL, sin cambios. */
TacBlock *tac_block_insert_after(TacFunction *function, TacBlock *previous);
TacInstruction *tac_append(TacBlock *block, TacOpcode op,
                          TacOperand dest, TacOperand src1, TacOperand src2);
/* previous NULL = inicio; previo ajeno = NULL, sin cambios. */
TacInstruction *tac_insert_after(TacBlock *block, TacInstruction *previous,
                               TacOpcode op, TacOperand dest,
                               TacOperand src1, TacOperand src2);
/* Quita/libera: 1 si pertenecía al bloque, 0 si ajena/NULL. */
int tac_remove(TacBlock *block, TacInstruction *instruction);
TacInstruction *tac_append_isa(TacBlock *block, const IsaInstruction *instruction,
                              TacOperand dest, TacOperand src1, TacOperand src2,
                              int64_t displacement, TacOperand target,
                              TacSelectors selectors);
/* Ambos sentidos, sin duplicados. connect conserva el orden de llamadas.
 * 0 si NULL o cruza funciones; disconnect también devuelve 0 si no existía.
 */
int tac_connect(TacBlock *from, TacBlock *to);
int tac_disconnect(TacBlock *from, TacBlock *to);
/* 1 éxito; 0 con diagnóstico. error puede ser NULL solo con capacity=0.
 * rebuild valida toda la forma antes de reemplazar aristas; no exige que las
 * aristas antiguas sean correctas. IF: [destino, caída], sin duplicados.
 * Las listas next deben ser acíclicas y contener memoria viva del constructor.
 */
int tac_rebuild_edges(TacFunction *function, char *error, size_t capacity);
int tac_validate(const TacProgram *program, char *error, size_t capacity);
/* Requiere programa válido; el llamador comprueba ferror del FILE prestado. */
void tac_print(FILE *out, const TacProgram *program);
void tac_program_free(TacProgram *program);
#endif
