/*
 * isa_table.h — Fases 4–5: tabla de instrucciones de la ISA y lectura de operandos.
 *
 * Es el único lugar donde se declaran los mnemónicos, su unidad, su funct y
 * la forma de sus operandos y su semántica de dependencias (isa.md §6). El organizador de bundles la usa
 * para describir dependencias y el ensamblador para codificar; agregar una
 * instrucción es agregar una fila. También ofrece la lectura de registros y
 * enteros que ambos comparten, para que acepten exactamente el mismo texto.
 *
 * También fija rangos, capacidad de ROM y convención de registros de ccom.
 * Usa: biblioteca C.
 * Lo usa: isa_table.c; codegen.h, codegen_instruction.c y assembler.c.
 * Referencias: isa.md §3, §4.2, §6.
 */

#ifndef ISA_TABLE_H
#define ISA_TABLE_H

#include <stdint.h>

/* Límites del procesador (isa.md §4.2 y §6; enunciado §3.1.2 y §3.2.1). */
#define ISA_IMMEDIATE_BITS 18
#define ISA_OFFSET_BITS    18
#define ISA_TARGET_BITS    22
#define ISA_ROM_BUNDLES    4096

/* Convención de ccom; el ISA solo fija rz. rx conserva la pila del Proyecto I;
 * rg es auxiliar del derrame y no permanece vivo (Aho §7.2 y §8.8). */
enum { REG_ZERO = 0, REG_LINK = 1, REG_STACK = 2, REG_SPILL = 3,
       REG_TEMP_FIRST = 4, REG_TEMP_LAST = 15, REG_TEMP_COUNT = 12 };
#define REG_CALL_CLOBBERS UINT16_C(0xfff0)

/* Entrada: entero sin normalizar. Salida: 1 si cabe con signo; sin efectos.
 * Restricciones: inline para consumidores que se enlazan solos, como isa_constant. */
static inline int isa_immediate_fits(int64_t value) {
    return value >= -(INT64_C(1) << (ISA_IMMEDIATE_BITS - 1)) &&
           value <   (INT64_C(1) << (ISA_IMMEDIATE_BITS - 1));
}

/* Entrada: registro ISA. Salida: alias prestado rz/rl/rx/rg/t0..t11.
 * Efectos: ninguno. Restricciones: fuera de 0..15 devuelve rz. */
const char *isa_register_name(int reg);


/* ==========================================================================
 * TABLA DE INSTRUCCIONES
 * ======================================================================= */

/* Unidad funcional de una instrucción. Los slots de cada unidad salen de
 * isa_unit_slots (isa.md §3). NONE es nopi; UNKNOWN marca texto que no se
 * pudo describir y el organizador trata como barrera. */
typedef enum {
    ISA_UNIT_NONE, ISA_UNIT_ALU, ISA_UNIT_BRU, ISA_UNIT_LSU,
    ISA_UNIT_CRYPTO, ISA_UNIT_UNKNOWN
} IsaUnit;

/* Forma de los operandos de una instrucción; fija su cantidad. */
typedef enum {
    ISA_FORM_R3,       /* rd, rs1, rs2                 ALU formato R            */
    ISA_FORM_IMM,      /* rd, rs1, imm                 saddi, xori              */
    ISA_FORM_MOV,      /* rd, imm                      mov (rs1 = 0)            */
    ISA_FORM_BRANCH,   /* rs1, rs2, destino            beq, bne, blt, bge       */
    ISA_FORM_JEFR,     /* rd, rs1                                               */
    ISA_FORM_JEF,      /* destino                      (rd = 0)                 */
    ISA_FORM_JAS,      /* rd, destino                                           */
    ISA_FORM_LOAD,     /* rd, imm(rs1)                 lw, lb                   */
    ISA_FORM_STORE,    /* rs2, imm(rs1)                sw, sb: fuente en 25..22 */
    ISA_FORM_FEIS,     /* rd, rs1, rs2, k, i                                    */
    ISA_FORM_RS1,      /* rs1                          auth, sanc               */
    ISA_FORM_NONE,     /* sin operandos                logout, kv_lock, nopi    */
    ISA_FORM_KV_WRITE  /* rs1, k, w                    w va en el campo i       */
} IsaForm;

/* Posiciones normalizadas: STORE usa SRC1 como base y SRC2 como dato. */
enum {
    ISA_REG_DEST = 1u,
    ISA_REG_SRC1 = 2u,
    ISA_REG_SRC2 = 4u
};

/* Valores compatibles con los nombres públicos CG_EFFECT_* de C0. */
typedef enum {
    ISA_EFFECT_NONE = 0,
    ISA_EFFECT_MEMORY_READ = 1u << 0,
    ISA_EFFECT_MEMORY_WRITE = 1u << 1,
    ISA_EFFECT_CONTROL = 1u << 2,
    ISA_EFFECT_CALL = 1u << 3,
    ISA_EFFECT_CRYPTO_STATE = 1u << 4,
    ISA_EFFECT_CONDITIONAL_WRITE = 1u << 5,
    ISA_EFFECT_PIPELINE_STALL = 1u << 6,
    ISA_EFFECT_PC_RELATIVE = 1u << 7,
    ISA_EFFECT_BARRIER = 1u << 8
} IsaEffect;

typedef struct {
    unsigned reads;
    unsigned writes;
    /* ifmov/feis usan el valor anterior si no escriben. Para análisis de
     * valores, usos = reads | preserves (Aho §10.2.3). No sumar preserves a
     * lecturas físicas: una WAW de distancia 1 pasaría a RAW de distancia 2
     * y cambiaría los bundles del organizador actual (isa.md §9).
     */
    unsigned preserves;
    unsigned effects;
    unsigned result_distance;
    int call_if_link_nonzero;
} IsaSemantics;

typedef struct {
    const char *name;
    IsaUnit unit;
    unsigned funct;
    IsaForm form;
    IsaSemantics semantics;
} IsaInstruction;

typedef struct {
    uint16_t reads;
    uint16_t writes;
    uint16_t preserves;
    unsigned effects;
    unsigned result_distance;
} IsaPhysicalEffects;

/*
 * isa_physical_effects — instancia la semántica con registros físicos.
 *
 * Entrada:
 *   instruction  descriptor válido o NULL.
 *   dest, src1, src2  registros 0..15; -1 en posiciones ausentes.
 *   pc_relative  no cero si el destino de salto es numérico.
 *   out  salida escribible o NULL.
 * Salida:
 *   1 en éxito; 0 si faltan registros, sobran o están fuera de rango.
 * Efectos:
 *   inicializa out a cero, incluso ante error; no define ni preserva rz.
 * Restricciones:
 *   el llamador valida texto, inmediatos, selectores y slots. pc_relative
 *   solo corresponde a BRANCH, JEF y JAS. No agrega convenciones de ccom.
 *   preserves se mantiene separado de reads (Aho §10.2.3; isa.md §9).
 */
int isa_physical_effects(const IsaInstruction *instruction,
                         int dest, int src1, int src2, int pc_relative,
                         IsaPhysicalEffects *out);

/*
 * isa_find — busca una instrucción por su mnemónico.
 *
 * Entrada:
 *   name  mnemónico no NULL, sin operandos.
 * Salida:
 *   fila de la tabla (no liberar) o NULL si no existe.
 *   Efectos: ninguno.
 * Restricciones:
 *   distingue mayúsculas; incluye nopi (unidad NONE, funct 0).
 */
const IsaInstruction *isa_find(const char *name);

/*
 * isa_operand_count — obtiene la cantidad de operandos de una forma.
 *
 * Entrada:
 *   form  forma de instrucción.
 * Salida:
 *   cantidad esperada, entre 0 y 5.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int isa_operand_count(IsaForm form);

/*
 * isa_unit_slots — obtiene los slots donde puede ir una unidad.
 *
 * Entrada:
 *   unit  unidad funcional.
 * Salida:
 *   máscara de bits 0–3: la ALU admite 0 y 1, CRYPTO el 1, la BRU el 2 y
 *   la LSU el 3; nopi cabe en los cuatro; UNKNOWN en ninguno.
 *   Efectos: ninguno.
 * Restricciones:
 *   isa.md §3.
 */
unsigned isa_unit_slots(IsaUnit unit);

/* ==========================================================================
 * LECTURA DE OPERANDOS
 * ======================================================================= */

/*
 * isa_trim — recorta espacios en una cadena.
 *
 * Entrada:
 *   text  cadena modificable no NULL.
 * Salida:
 *   puntero prestado al texto útil, dentro de text.
 *   Efectos: escribe NUL al final del texto útil.
 * Restricciones:
 *   ninguna.
 */
char *isa_trim(char *text);

/*
 * isa_parse_register — reconoce un registro ISA o su alias.
 *
 * Entrada:
 *   text  operando no NULL.
 * Salida:
 *   número ISA 0–15 o -1 si es inválido.
 *   Efectos: ninguno.
 * Restricciones:
 *   no admite ceros iniciales; rz, rl, rx y rg son los registros 0–3 y
 *   t0–t11 equivalen a r4–r15.
 */
int isa_parse_register(const char *text);

/*
 * isa_parse_integer — lee un entero decimal o hexadecimal.
 *
 * Entrada:
 *   text  operando no NULL.
 *   value  puntero no NULL para el valor.
 * Salida:
 *   1 si convierte todo el operando sin error; 0 si falla.
 *   Efectos: escribe *value al intentar strtol.
 * Restricciones:
 *   usa el rango de long del anfitrión; admite signo y prefijo 0x o 0X.
 */
int isa_parse_integer(const char *text, long *value);

#endif
