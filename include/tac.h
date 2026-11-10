/*
 * tac.h — representación intermedia de tres direcciones, independiente del AST.
 * Usa: symbol_table.h; lo usan optimizaciones y pruebas/tac.
 * Referencias: Aho §6.2, §8.4, §10.2.3; enunciado §4.1.
 */
#ifndef TAC_H
#define TAC_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "symbol_table.h"

typedef enum { TAC_NONE, TAC_TEMP, TAC_VARIABLE, TAC_INTEGER, TAC_LABEL } TacOperandKind;
typedef struct {
    TacOperandKind kind;
    union { size_t id; const Symbol *symbol; int64_t integer; } value;
} TacOperand;
typedef enum {
    TAC_COPY, TAC_ADD, TAC_SUB, TAC_AND, TAC_OR, TAC_XOR,
    TAC_EQ, TAC_NE, TAC_LT, TAC_LE, TAC_GT, TAC_GE, TAC_NEG, TAC_NOT,
    TAC_MUL, TAC_DIV, TAC_MOD, TAC_POW, TAC_LOAD, TAC_STORE, TAC_CRYPTO,
    TAC_PARAM, TAC_CALL, TAC_RETURN, TAC_GOTO,
    TAC_IF_EQ, TAC_IF_NE, TAC_IF_LT, TAC_IF_LE, TAC_IF_GT, TAC_IF_GE,
    TAC_OP_COUNT
} TacOpcode;
enum { TAC_DEST = 1, TAC_SRC1 = 2, TAC_SRC2 = 4 };
typedef enum { TAC_FLOW_NEXT, TAC_FLOW_RETURN, TAC_FLOW_GOTO, TAC_FLOW_IF } TacFlow;
typedef enum { TAC_FMT_COPY, TAC_FMT_BINARY, TAC_FMT_UNARY, TAC_FMT_LOAD,
    TAC_FMT_STORE, TAC_FMT_CRYPTO, TAC_FMT_PARAM, TAC_FMT_CALL,
    TAC_FMT_RETURN, TAC_FMT_GOTO, TAC_FMT_IF } TacFormat;
/* Tabla inmutable: máscaras de posiciones, memoria abstracta y control.
 * allowed[k] contiene los tipos permitidos en destino, fuente 1 y fuente 2.
 * NONE permite un operando opcional. CALL define destino solo si está presente.
 * Los efectos de memoria son implícitos: defs/uses devuelve solo valores,
 * nunca inventa un temporal para representar memoria (Aho §6.2.2).
 */
typedef struct {
    const char *text;
    unsigned writes, reads;
    int memory_read, memory_write, barrier, side_effect;
    TacFlow flow;
    TacFormat format;
    unsigned allowed[3];
} TacOpInfo;
typedef struct TacInstruction {
    TacOpcode op;
    TacOperand dest, src1, src2;
    struct TacInstruction *next;
} TacInstruction;
typedef struct TacBlock {
    size_t label;
    struct TacFunction *function;
    TacInstruction *first, *last;
    struct TacBlock **successors, **predecessors;
    size_t successor_count, predecessor_count;
    struct TacBlock *next;
} TacBlock;
typedef struct TacFunction {
    char *name;
    size_t next_temp, next_label;
    TacBlock *first, *last, *entry;
    struct TacFunction *next;
} TacFunction;
typedef struct { TacFunction *first, *last; } TacProgram;

/* Constructores de operandos por valor; Symbol prestado, nunca se libera. */
TacOperand tac_none(void);
TacOperand tac_variable(const Symbol *symbol);
TacOperand tac_integer(int64_t value);
TacOperand tac_label(const TacBlock *block);
/* Reserva temporal virtual local a la función; sin límite de registros ISA.
 * Como los demás constructores, termina con diagnóstico si falta memoria
 * o se agota el espacio de identificadores size_t. */
TacOperand tac_temp_new(TacFunction *function);
/* Propiedades por tabla; NULL si opcode inválido. */
const TacOpInfo *tac_op_info(TacOpcode op);
/* Devuelve operandos TEMP/VARIABLE en orden dest, src1, src2, sin duplicar.
 * out tiene capacidad 3; retorno 0..3. Instrucción/op inválido devuelve 0. */
size_t tac_definitions(const TacInstruction *instruction, TacOperand out[3]);
size_t tac_uses(const TacInstruction *instruction, TacOperand out[3]);
/* Propiedad: programa -> funciones -> bloques -> instrucciones y aristas.
 * Función copia nombre; primer bloque es entrada. No se posee Symbol.
 * Argumentos no NULL y pertenecientes al programa, salvo donde se indica.
 */
TacProgram *tac_program_new(void);
TacFunction *tac_function_new(TacProgram *program, const char *name);
TacBlock *tac_block_new(TacFunction *function);
TacInstruction *tac_append(TacBlock *block, TacOpcode op,
                          TacOperand dest, TacOperand src1, TacOperand src2);
/* Conecta ambos sentidos, sin duplicados; 0 si cruza funciones o recibe NULL.
 * En IF: sucesores [destino, siguiente bloque]; si coinciden, solo uno.
 * Sin salto: sucesor único = siguiente bloque; RETURN: ninguno.
 */
int tac_connect(TacBlock *from, TacBlock *to);
/* Valida operandos, etiquetas locales, terminadores, aristas y entrada.
 * 1 válido; 0 y diagnóstico en error (si capacity > 0). No modifica nada. */
int tac_validate(const TacProgram *program, char *error, size_t capacity);
/* Recorrido público: program->first/next, function->first/next y
 * block->first/next, en orden de inserción. Se pueden editar operandos y op
 * durante el recorrido y conectar aristas; revalidar al terminar. Para borrar
 * o reenlazar nodos guardar next antes; mantener first/last/entry y aristas.
 * No liberar la lista que se recorre. Agregar al final permite visitar lo
 * agregado; ninguna función de consulta modifica las listas.
 */
void tac_program_free(TacProgram *program);
/* Impresión determinista; requiere programa válido, FILE vivo.
 * No posee el flujo; el llamador consulta ferror. */
void tac_print(FILE *out, const TacProgram *program);
#endif
