/*
 * tac.h — representación intermedia de tres direcciones (C2.1a).
 *
 * Alto nivel: cuádruplas de Calser; ISA: instrucciones reales y TAC_PSEUDO.
 * TAC_PSEUDO usa las mismas consultas; debe expandirse antes de emitir ISA.
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
 * block->label es un ID interno estable, independiente del orden de la lista.
 * Listados y diagnósticos numeran B0, B1... por posición en cada función.
 * Sin pertenencia válida a una lista, el diagnóstico conserva el ID interno.
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
 * Impresión estable; tac_build.h construye alto nivel desde el AST validado.
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
typedef struct TacLoopMark TacLoopMark;

typedef enum {
    TAC_STAGE_HIGH,          /* Operaciones Calser; TEMP se imprime tN. */
    TAC_STAGE_ISA_VIRTUAL,   /* ISA/PSEUDO; TEMP vN y PHYS rz/rl/rx. */
    TAC_STAGE_ISA_PHYSICAL   /* ISA/PSEUDO físicos; validar expansión antes de emitir. */
} TacStage;
typedef enum {
    TAC_NONE,               /* Posición ausente; la validación comprueba su uso. */
    TAC_TEMP,               /* Identidad virtual local a la función; para asignación. */
    TAC_VARIABLE,           /* Symbol de variable, parámetro o resultado; alto nivel. */
    TAC_INTEGER,
    TAC_LABEL,              /* Referencia a un bloque, no dirección ni texto. */
    TAC_FUNCTION,           /* Symbol de función llamado por CALL/jas. */
    TAC_PHYS                /* Registro arquitectónico 0..15, distinto de TEMP. */
} TacOperandKind;
typedef struct {
    TacOperandKind kind;              /* Selecciona el miembro de value; consultas y validación. */
    union {
        size_t id;                    /* ID de TEMP o número PHYS; asignación y renombramiento. */
        int64_t integer;              /* Constante sin truncar; C2.3 materializa el valor ISA. */
        const Symbol *symbol;         /* VARIABLE/FUNCTION prestado; globales y llamadas. */
        const TacBlock *block;        /* LABEL prestado; validación y construcción del CFG. */
    } value;                          /* Contenido según kind; todas las consultas. */
} TacOperand;
typedef enum {
    TAC_COPY, TAC_ADDRESS,
    TAC_ADD, TAC_SUB, TAC_AND, TAC_OR, TAC_XOR,
    TAC_EQ, TAC_NE, TAC_LT, TAC_LE, TAC_GT, TAC_GE, TAC_NEG, TAC_NOT,
    TAC_MUL, TAC_DIV, TAC_MOD, TAC_POW, TAC_LOAD, TAC_STORE,
    TAC_AUTH, TAC_LOGOUT, TAC_KV_WRITE, TAC_KV_LOCK, TAC_SANC,
    TAC_CIFRAR, TAC_DESCIFRAR, TAC_PARAM, TAC_CALL, TAC_RETURN, TAC_GOTO,
    TAC_IF_EQ, TAC_IF_NE, TAC_IF_LT, TAC_IF_LE, TAC_IF_GT, TAC_IF_GE,
    TAC_ISA, TAC_PSEUDO, TAC_OP_COUNT
} TacOpcode;
/* Operaciones de destino pendientes de expansión; no son mnemónicos hardware. */
typedef enum {
    TAC_PSEUDO_NONE,      /* Metadatos ausentes en una instrucción que no es PSEUDO. */
    TAC_PSEUDO_MUL, TAC_PSEUDO_DIV, TAC_PSEUDO_MOD, TAC_PSEUDO_POW,
    TAC_PSEUDO_AUTH, TAC_PSEUDO_LOGOUT, TAC_PSEUDO_KV_WRITE,
    TAC_PSEUDO_KV_LOCK, TAC_PSEUDO_SANC,
    TAC_PSEUDO_CIFRAR, TAC_PSEUDO_DESCIFRAR,
    TAC_PSEUDO_EXIT,       /* Termina main sin retornar mediante rl. */
    TAC_PSEUDO_COUNT
} TacPseudoKind;
enum { TAC_DEST = 1u, TAC_SRC1 = 2u, TAC_SRC2 = 4u };
enum { TAC_SELECT_K = 1u, TAC_SELECT_W = 2u, TAC_SELECT_ROUND = 4u };
/* Cada selector ausente vale -1. */
typedef struct {
    int k;                            /* Selector de llave 0..3; validación e impresión CRYPTO. */
    int w;                            /* Selector de palabra 0..3; validación de kv_write. */
    int round;                        /* Selector de ronda 0..3; validación de feis. */
} TacSelectors;
typedef enum {
    TAC_FLOW_NEXT,          /* Continúa; CALL/jas tampoco terminan el bloque. */
    TAC_FLOW_GOTO,          /* Termina con un sucesor explícito. */
    TAC_FLOW_IF,            /* Termina con destino tomado y caída al bloque siguiente. */
    TAC_FLOW_RETURN         /* Termina sin sucesores locales. */
} TacFlow;
typedef enum {
    TAC_FMT_COPY, TAC_FMT_ADDRESS, TAC_FMT_BINARY, TAC_FMT_UNARY,
    TAC_FMT_LOAD, TAC_FMT_STORE, TAC_FMT_CRYPTO, TAC_FMT_PARAM, TAC_FMT_CALL,
    TAC_FMT_RETURN, TAC_FMT_GOTO, TAC_FMT_IF, TAC_FMT_ISA, TAC_FMT_PSEUDO
} TacFormat;
/*
 * reads/writes/address_bases usan TAC_DEST, TAC_SRC1 y TAC_SRC2.
 * allowed[i] usa bits 1u << TacOperandKind, en orden dest/src1/src2.
 * selectors usa TAC_SELECT_*.
 * Las máscaras físicas usan el bit N para el registro arquitectónico N.
 */
typedef struct {
    const char *text;                 /* Operador o mnemónico; impresión y diagnósticos. */
    unsigned reads;                   /* Posiciones leídas, incluido preserves ISA; usos y DCE. */
    unsigned writes;                  /* Posiciones escritas; definiciones y renombramiento. */
    /* Bases VARIABLE que son dirección, no valor; usos y spilling. */
    unsigned address_bases;
    unsigned allowed[3];              /* Clases admitidas por posición; validación de operandos. */
    /* Selectores obligatorios; validación de los demás como -1. */
    unsigned selectors;
    /* Puede leer memoria implícita; dependencias y reordenamiento. */
    int memory_read;
    int memory_write;                 /* Puede escribir memoria implícita; DCE y dependencias. */
    int barrier;                      /* Impide cruzar la operación al reordenar. */
    int nonremovable;                 /* Prohíbe eliminar por resultado muerto; DCE. */
    int crypto_state;                 /* Afecta estado CRYPTO oculto; DCE y orden de operaciones. */
    int may_trap;                     /* Puede fallar; DCE necesita prueba para eliminarla. */
    /* La expansión puede emitir flujo interno; selección y emisión de C2.3. */
    int may_create_blocks;
    int uses_globals;                 /* Lee implícitamente todas las globales; vivacidad y DCE. */
    int defines_globals;              /* Puede modificar todas las globales; dependencias y DCE. */
    uint16_t implicit_phys_reads;     /* Registros leídos fuera de D/A/B; vivacidad y asignación. */
    /* Registros escritos fuera de D/A/B; dependencias y asignación. */
    uint16_t implicit_phys_writes;
    /* Registros que puede destruir CALL/ISA/PSEUDO; conservación y spilling. */
    uint16_t call_clobbers;
    /* Auxiliares distintos requeridos por la expansión; asignación (8 en aritmética). */
    unsigned scratch_count;
    /* Continuación o terminador; construcción y validación del CFG. */
    TacFlow flow;
    TacFormat format;                 /* Selecciona la sintaxis del listado; tac_print. */
} TacOpInfo;

typedef struct {
    /* Fila ISA prestada; consultas de semántica y validación. */
    const IsaInstruction *instruction;
    int64_t displacement;             /* Desplazamiento en bytes de LOAD/STORE; C2.3 e impresión. */
    TacOperand target;                /* LABEL de salto o FUNCTION de jas; validación y CFG. */
} TacIsaData;

/* D/A/B y selectors se comparten con TacInstruction; no se duplican aquí. */
typedef struct {
    TacPseudoKind kind;               /* Operación pendiente; propiedades, validación y emisión. */
    /* Ocho auxiliares aritméticos físicos; cero en virtual y en las demás PSEUDO. */
    uint16_t scratch_clobbers;
} TacPseudoData;

struct TacInstruction {
    TacOpcode op;                     /* Alto nivel, TAC_ISA o TAC_PSEUDO; consultas y pasadas. */
    TacOperand dest;                  /* Posición D; reads/writes determinan si define o usa. */
    TacOperand src1;                  /* Posición A; su función depende de la operación. */
    TacOperand src2;                  /* Posición B; su función depende de la operación. */
    /* Literales CRYPTO, -1 si ausentes; validación e impresión. */
    TacSelectors selectors;
    TacIsaData isa;                   /* Datos exclusivos de TAC_ISA; C2.3 y consultas ISA. */
    TacPseudoData pseudo;             /* Datos exclusivos de TAC_PSEUDO; asignación y expansión. */
    int source_line;                  /* Línea de origen, 0 si desconocida; futuros diagnósticos. */
    int source_column;                /* Columna de origen, 0 si desconocida; diagnósticos L:C. */
    TacInstruction *next;             /* Siguiente en orden de lista; recorrido y reordenamiento. */
};

struct TacBlock {
    size_t label;                     /* ID interno estable; no es el número de presentación. */
    TacFunction *function;            /* Función propietaria; validación de pertenencia. */
    /* Primera instrucción poseída, o NULL; recorrido y edición. */
    TacInstruction *first;
    TacInstruction *last;             /* Última instrucción, o NULL; inserción y terminadores. */
    TacBlock **successors;            /* Vector propio de destinos prestados; CFG y análisis. */
    TacBlock **predecessors;          /* Vector propio de orígenes prestados; CFG y análisis. */
    size_t successor_count;           /* Cantidad de sucesores; recorrido y validación del CFG. */
    /* Cantidad de predecesores; recorrido y validación del CFG. */
    size_t predecessor_count;
    /* Siguiente bloque; determina caída y número de presentación en listados/diagnósticos. */
    TacBlock *next;
};

struct TacFunction {
    const Symbol *symbol;             /* Symbol FUNCTION prestado; identidad y llamadas de C2.3. */
    TacProgram *program;              /* Programa propietario; acceso a etapa y ámbito global. */
    /* Próximo ID virtual sin reutilizar; expansión y renombramiento. */
    size_t next_temp;
    /* Próximo ID interno local sin reutilizar; independiente del número de presentación. */
    size_t next_label;
    TacBlock *first;                  /* Primer bloque poseído; recorrido y orden de caída. */
    /* Último bloque poseído; inserción y validación del cierre. */
    TacBlock *last;
    TacBlock *entry;                  /* Entrada, igual a first; raíz del CFG para los análisis. */
    TacFunction *next;                /* Siguiente función del programa; recorrido e impresión. */
    /* Colección propia solo HIGH; descartar antes de invalidar bloques (unrolling). */
    TacLoopMark *loop_marks;
};

struct TacProgram {
    TacStage stage;                   /* Nivel común de las instrucciones; validación y pasadas. */
    /* SCOPE_GLOBAL obligatorio y prestado; efectos globales de DCE. */
    const Scope *global_scope;
    TacFunction *first;               /* Primera función poseída, o NULL; recorrido del programa. */
    TacFunction *last;                /* Última función poseída, o NULL; inserción y validación. */
};
enum {
    TAC_P1_RZ = 0, TAC_P1_RL = 1, TAC_P1_RX = 2, TAC_P1_RG = 3,
    TAC_P1_TEMP_FIRST = 4, TAC_P1_TEMP_LAST = 15
};
#define TAC_P1_CALL_CLOBBERS UINT16_C(0xfff0)

/* Contratos completos por función en las implementaciones.
 * Las precondiciones y rechazos están en el contrato de cada constructor.
 * tac_program_new exige SCOPE_GLOBAL no NULL y termina con diagnóstico si no
 * se cumple; nunca devuelve NULL. No probar ese caso en el mismo proceso.
 * Ante agotamiento de memoria o identificadores size_t: diagnóstico y termina;
 * nunca limita a doce TEMP.
 * Las instrucciones nuevas tienen selectores {-1,-1,-1} y línea/columna cero.
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
/* Transaccionales: 0 con diagnóstico y sin cambios. replace solo toca lecturas
 * no ligadas; excluye bases simbólicas. block_remove rechaza entry, LABEL ajenos,
 * marcas y caídas alcanzables alteradas. Reconstruir/validar después de editar CFG.
 */
int tac_replace_reads(TacBlock *block, TacInstruction *instruction,
    TacOperand from, TacOperand to, size_t *replaced, char *error, size_t capacity);
int tac_block_remove(TacFunction *function, TacBlock *block, char *error, size_t capacity);
/* previous NULL = principio; previo ajeno = NULL. Inicializa los mismos datos
 * que append; puede dejar forma temporalmente inválida, validar después.
 */
TacInstruction *tac_insert_isa_after(TacBlock *block, TacInstruction *previous,
    const IsaInstruction *instruction, TacOperand dest, TacOperand src1, TacOperand src2,
    int64_t displacement, TacOperand target, TacSelectors selectors);
TacInstruction *tac_insert_pseudo_after(TacBlock *block, TacInstruction *previous,
    TacPseudoKind kind, TacOperand dest, TacOperand src1, TacOperand src2,
    TacSelectors selectors, uint16_t scratch_clobbers);

TacInstruction *tac_append_isa(TacBlock *block, const IsaInstruction *instruction,
                              TacOperand dest, TacOperand src1, TacOperand src2,
                              int64_t displacement, TacOperand target,
                              TacSelectors selectors);
/* Usa D/A/B y selectores compartidos; scratch_clobbers solo en aritmética física.
 * No expande ni asigna auxiliares: reconstruir/validar después de construir.
 */
TacInstruction *tac_append_pseudo(TacBlock *block, TacPseudoKind kind,
                                 TacOperand dest, TacOperand src1, TacOperand src2,
                                 TacSelectors selectors, uint16_t scratch_clobbers);
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
/* Exige forma válida, etapa ISA_PHYSICAL y ninguna PSEUDO pendiente de expansión.
 * Es el control obligatorio del futuro emisor antes de entregar ISA al ensamblador.
 */
int tac_validate_for_emission(const TacProgram *program, char *error, size_t capacity);
/* Requiere programa válido; el llamador comprueba ferror del FILE prestado. */
void tac_print(FILE *out, const TacProgram *program);
void tac_program_free(TacProgram *program);
#endif
