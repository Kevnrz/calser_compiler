/*
 * isa_table.h — Fases 4–5: tabla de instrucciones de la ISA y lectura de operandos.
 *
 * Es el único lugar donde se declaran los mnemónicos, su unidad, su funct y
 * la forma de sus operandos (isa.md §6). El organizador de bundles la usa
 * para describir dependencias y el ensamblador para codificar; agregar una
 * instrucción es agregar una fila. También ofrece la lectura de registros y
 * enteros que ambos comparten, para que acepten exactamente el mismo texto.
 *
 * Usa: biblioteca C.
 * Lo usa: isa_table.c; codegen.h, codegen_instruction.c y assembler.c.
 * Referencias: isa.md §3, §4.2, §6.
 */

#ifndef ISA_TABLE_H
#define ISA_TABLE_H

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

typedef struct {
    const char *name;
    IsaUnit unit;
    unsigned funct;
    IsaForm form;
} IsaInstruction;

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
