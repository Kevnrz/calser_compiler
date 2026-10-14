/*
 * codegen.c — Fase 4: emisión del IR, bundles e imagen de datos.
 *
 * Registra operaciones con separaciones conservadoras y bundles fijos.
 * Organiza regiones del IR y reconstruye líneas y direcciones finales.
 *
 * Usa: codegen_instruction.c, codegen_schedule.c y codegen_verify.c.
 * Lo usa: generate.c, codegen_crypto.c y main.c.
 * Referencias: isa.md §3, §9, §11.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "codegen_schedule.h"

/* Lista de líneas (etiquetas y bundles), en orden de emisión. */
static char **lines = NULL;
static int line_count = 0;
static int line_capacity = 0;
static int bundles = 0;

/* Para el .asm anotado, por cada línea: la línea del fuente que la generó
   (0 = ninguna) y su dirección (el bundle; en una etiqueta, el que la sigue). */
static int *source_lines = NULL;
static int *addresses = NULL;
static int current_source_line = 0;
static int (*slot_source_lines)[4] = NULL;
static int finalized = 0;

/* Flujo de instrucciones anterior al empaquetado. Se conserva para comparar
 * la emisión original; cg_schedule reconstruye las líneas finales desde él. */
static CGIRRecord *ir = NULL;
static int ir_count = 0;
static int ir_capacity = 0;

/* Imagen de datos y bytes usados (mayor dirección escrita + 4). */
static uint32_t data[CG_DATA_BYTES / 4];
static int data_used = 0;

/* ==========================================================================
 * MEMORIA Y REGISTROS DEL IR
 * ======================================================================= */


/*
 * out_of_memory — termina la emisión por falta de memoria.
 *
 * Entrada: ninguna
 * Salida:
 *   no retorna.
 *   Efectos: escribe stderr y termina el proceso.
 * Restricciones:
 *   ninguna.
 */
static void out_of_memory(void) {
    fprintf(stderr, "ccom: error interno: memoria insuficiente (codegen)\n");
    exit(EXIT_FAILURE);
}

/*
 * copy_text — duplica el texto de una instrucción.
 *
 * Entrada:
 *   text  cadena no NULL.
 * Salida:
 *   cadena nueva que debe liberarse.
 *   Efectos: reserva memoria o termina el proceso.
 * Restricciones:
 *   ninguna.
 */
static char *copy_text(const char *text) {
    char *copy = malloc(strlen(text) + 1);
    if (!copy) out_of_memory();
    strcpy(copy, text);
    return copy;
}

/*
 * describe_copy — copia y describe una instrucción para el IR.
 *
 * Entrada:
 *   text  texto no NULL.
 *   slot  slot ISA 0–3.
 * Salida:
 *   descripción con texto propio.
 *   Efectos: reserva texto y conserva la línea actual.
 * Restricciones:
 *   texto desconocido queda como barrera.
 */
static CGInstruction describe_copy(const char *text, Slot slot) {
    CGInstruction instruction;
    cg_describe_instruction(copy_text(text), slot, current_source_line, &instruction);
    return instruction;
}

/*
 * push_ir — agrega un registro al flujo del IR.
 *
 * Entrada:
 *   record  registro con sus textos ya reservados.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: amplía IR y registra línea/dirección actuales.
 * Restricciones:
 *   no emitir después de cg_schedule; termina el proceso si finalized.
 */
static void push_ir(CGIRRecord record) {
    if (finalized) {
        fprintf(stderr, "ccom: error interno: emision despues de organizar bundles\n");
        exit(EXIT_FAILURE);
    }
    if (ir_count == ir_capacity) {
        int capacity = ir_capacity ? ir_capacity * 2 : 64;
        CGIRRecord *grown = realloc(ir, (size_t) capacity * sizeof(*ir));
        if (!grown) out_of_memory();
        ir = grown;
        ir_capacity = capacity;
    }
    record.source_line = current_source_line;
    record.bundle_address = bundles;
    ir[ir_count++] = record;
}

/*
 * push_line — agrega una línea y toma su propiedad.
 *
 * Entrada:
 *   text  cadena dinámica no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: amplía líneas y metadatos; toma posesión de text.
 * Restricciones:
 *   ninguna.
 */
static void push_line(char *text) {
    if (line_count == line_capacity) {
        int capacity = line_capacity == 0 ? 256 : line_capacity * 2;
        char **grown = realloc(lines, (size_t) capacity * sizeof(char *));
        int *grown_sources =
            realloc(source_lines, (size_t) capacity * sizeof(int));
        int *grown_addresses =
            realloc(addresses, (size_t) capacity * sizeof(int));

        int (*grown_slots)[4] = realloc(slot_source_lines, (size_t) capacity * sizeof(*slot_source_lines));

        if (grown == NULL || grown_sources == NULL || grown_addresses == NULL || grown_slots == NULL)
            out_of_memory();

        lines = grown;
        source_lines = grown_sources;
        addresses = grown_addresses;
        slot_source_lines = grown_slots;
        line_capacity = capacity;
    }

    source_lines[line_count] = current_source_line;
    addresses[line_count] = bundles;
    for (int slot = 0; slot < 4; slot++) slot_source_lines[line_count][slot] = -1;
    lines[line_count++] = text;
}

/*
 * clear_output — libera las líneas de salida actuales.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera textos/metadatos y reinicia cantidad de bundles.
 * Restricciones:
 *   conserva el IR y la imagen de datos.
 */
static void clear_output(void) {
    for (int i = 0; i < line_count; i++) free(lines[i]);
    free(lines); free(source_lines); free(addresses); free(slot_source_lines);
    lines = NULL; source_lines = NULL; addresses = NULL; slot_source_lines = NULL;
    line_count = line_capacity = bundles = 0;
}

/* ==========================================================================
 * CICLO DE VIDA DEL EMISOR
 * ======================================================================= */


/* cg_init — véase el contrato en codegen.h. */
void cg_init(void) {
    cg_free();
    memset(data, 0, sizeof(data));
    data_used = 0;
}

/* cg_free — véase el contrato en codegen.h. */
void cg_free(void) {
    for (int i = 0; i < ir_count; i++) {
        if (ir[i].kind == CG_IR_LABEL) free((void *) ir[i].data.label);
        else if (ir[i].kind == CG_IR_OPERATION)
            free((void *) ir[i].data.instruction.text);
        else if (ir[i].kind == CG_IR_FIXED_BUNDLE)
            for (int slot = 0; slot < 4; slot++)
                free((void *) ir[i].data.slots[slot].text);
    }
    free(ir);
    ir = NULL;
    ir_count = ir_capacity = 0;
    clear_output();
    current_source_line = 0;
    finalized = 0;
}

/* ==========================================================================
 * ETIQUETAS Y EMISIÓN CONSERVADORA
 * ======================================================================= */


/* cg_label — véase el contrato en codegen.h. */
void cg_label(const char *name) {
    CGIRRecord record = {.kind = CG_IR_LABEL};
    record.data.label = copy_text(name);
    push_ir(record);
    size_t size = strlen(name) + 2;
    char *text = malloc(size);

    if (text == NULL)
        out_of_memory();

    snprintf(text, size, "%s:", name);
    push_line(text);
}

/*
 * emit_bundle — agrega un bundle textual de cuatro slots.
 *
 * Entrada:
 *   s0  texto de slot 0 o NULL.
 *   s1  texto de slot 1 o NULL.
 *   s2  texto de slot 2 o NULL.
 *   s3  texto de slot 3 o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: copia textos en una línea, guarda posiciones e incrementa bundles.
 * Restricciones:
 *   NULL se reemplaza por nopi; no agrega registro al IR.
 */
static void emit_bundle(const char *s0, const char *s1, const char *s2, const char *s3) {
    const char *slot[4] = {
        s0 ? s0 : "nopi",
        s1 ? s1 : "nopi",
        s2 ? s2 : "nopi",
        s3 ? s3 : "nopi"
    };

    size_t size = 8;   // "{ " " }" y los tres " | "

    for (int i = 0; i < 4; i++)
        size += strlen(slot[i]) + 3;

    char *text = malloc(size);

    if (text == NULL)
        out_of_memory();

    snprintf(text, size, "{ %s | %s | %s | %s }",
             slot[0], slot[1], slot[2], slot[3]);

    push_line(text);
    for (int i = 0; i < 4; i++)
        if (strcmp(slot[i], "nopi") != 0) slot_source_lines[line_count - 1][i] = current_source_line;
    bundles++;
}

/* cg_bundle — véase el contrato en codegen.h. */
void cg_bundle(const char *s0, const char *s1, const char *s2, const char *s3) {
    const char *text[4] = {s0, s1, s2, s3};
    CGIRRecord record = {.kind = CG_IR_FIXED_BUNDLE};
    for (int slot = 0; slot < 4; slot++)
        record.data.slots[slot] = describe_copy(text[slot] ? text[slot] : "nopi", (Slot) slot);
    push_ir(record);
    emit_bundle(s0, s1, s2, s3);
}

/*
 * emit_gap — registra y emite una separación.
 *
 * Entrada:
 *   reason  motivo automático o explícito.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: agrega IR de separación y bundle vacío.
 * Restricciones:
 *   antes de cg_schedule.
 */
static void emit_gap(CGGapReason reason) {
    CGIRRecord record = {.kind = CG_IR_GAP};
    record.data.gap_reason = reason;
    push_ir(record);
    emit_bundle(NULL, NULL, NULL, NULL);
}

/* cg_gap — véase el contrato en codegen.h. */
void cg_gap(void) {
    emit_gap(CG_GAP_EXPLICIT);
}

/* cg_op — véase el contrato en codegen.h. */
void cg_op(Slot slot, const char *fmt, ...) {
    char op[256];
    va_list args;

    va_start(args, fmt);
    int length = vsnprintf(op, sizeof(op), fmt, args);
    va_end(args);

    if (length < 0 || (size_t) length >= sizeof(op)) {
        fprintf(stderr, "ccom: error interno: operacion demasiado larga\n");
        exit(EXIT_FAILURE);
    }

    if (slot < SLOT_ALU0 || slot > SLOT_LSU) {
        fprintf(stderr, "ccom: error interno: slot invalido\n");
        exit(EXIT_FAILURE);
    }
    CGIRRecord record = {.kind = CG_IR_OPERATION};
    record.data.instruction = describe_copy(op, slot);
    push_ir(record);

    const char *s[4] = { NULL, NULL, NULL, NULL };

    s[slot] = op;

    emit_bundle(s[0], s[1], s[2], s[3]);
    emit_gap(CG_GAP_AUTOMATIC);
}

typedef struct { uint16_t previous_writes; } ScheduleOutput;

/* ==========================================================================
 * REGIONES Y ORGANIZACIÓN
 * ======================================================================= */


/*
 * scheduled_bundle — reconstruye un bundle organizado y sus posiciones.
 *
 * Entrada:
 *   user  estado ScheduleOutput no NULL.
 *   bundle  bundle con punteros prestados a instrucciones.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite línea y actualiza previous_writes y posiciones por slot.
 * Restricciones:
 *   no agrega IR; los textos de entrada siguen vivos.
 */
static void scheduled_bundle(void *user, const CGScheduledBundle *bundle) {
    ScheduleOutput *state = user;
    const char *text[4] = {NULL, NULL, NULL, NULL};
    current_source_line = 0;
    state->previous_writes = 0;
    for (int slot = 0; slot < 4; slot++) if (bundle->slots[slot]) {
        text[slot] = bundle->slots[slot]->text;
        if (!current_source_line) current_source_line = bundle->slots[slot]->source_line;
        state->previous_writes |= bundle->slots[slot]->writes;
    }
    emit_bundle(text[0], text[1], text[2], text[3]);
    for (int slot = 0; slot < 4; slot++)
        slot_source_lines[line_count - 1][slot] = bundle->slots[slot] ? bundle->slots[slot]->source_line : -1;
}

/*
 * flush_region — organiza la región y agrega separación de frontera.
 *
 * Entrada:
 *   operations  arreglo de operaciones.
 *   count  cantidad por referencia.
 *   state  estado de salida no NULL.
 * Salida:
 *   0 si emite o está vacía; 1 si falla.
 *   Efectos: emite región y hueco, pone *count a cero en éxito.
 * Restricciones:
 *   hasta CG_SCHEDULE_MAX_OPERATIONS; operaciones sin barreras.
 */
static int flush_region(const CGInstruction **operations, int *count, ScheduleOutput *state) {
    if (!*count) return 0;
    if (cg_schedule_block(operations, *count, state->previous_writes, scheduled_bundle, state)) return 1;
    *count = 0;
    /* Frontera segura para etiquetas, caminos de salto y plantillas fijas. */
    CGScheduledBundle gap = {{NULL, NULL, NULL, NULL}};
    scheduled_bundle(state, &gap);
    return 0;
}

/* cg_schedule — véase el contrato en codegen.h. */
int cg_schedule(void) {
    if (finalized) return 0;
    /* Sin resolución de destinos numéricos o efectos desconocidos, conservar
     * todo el programa: acortar otras regiones también cambiaría esos saltos. */
    for (int i = 0; i < ir_count; i++) {
        const CGIRRecord *r = &ir[i];
        int count = r->kind == CG_IR_OPERATION ? 1 : r->kind == CG_IR_FIXED_BUNDLE ? 4 : 0;
        for (int slot = 0; slot < count; slot++) {
            const CGInstruction *op = r->kind == CG_IR_OPERATION ? &r->data.instruction : &r->data.slots[slot];
            if (!op->known || (op->effects & CG_EFFECT_PC_RELATIVE)) {
                finalized = 1;
                return 0;
            }
        }
    }
    int saved_line = current_source_line;
    clear_output();
    ScheduleOutput state = {0};
    const CGInstruction *operations[CG_SCHEDULE_MAX_OPERATIONS];
    int count = 0;
    for (int i = 0; i < ir_count; i++) {
        const CGIRRecord *r = &ir[i];
        if (r->kind == CG_IR_GAP && r->data.gap_reason == CG_GAP_AUTOMATIC) continue;
        if (r->kind == CG_IR_OPERATION && !(r->data.instruction.effects & CG_EFFECT_BARRIER)) {
            operations[count++] = &r->data.instruction;
            if (count == CG_SCHEDULE_MAX_OPERATIONS || (r->data.instruction.effects & CG_EFFECT_CONTROL))
                if (flush_region(operations, &count, &state)) return 1;
            continue;
        }
        if (flush_region(operations, &count, &state)) return 1;
        current_source_line = r->source_line;
        if (r->kind == CG_IR_LABEL) {
            size_t size = strlen(r->data.label) + 2;
            char *label = malloc(size);
            if (!label) out_of_memory();
            snprintf(label, size, "%s:", r->data.label);
            push_line(label);
        } else if (r->kind == CG_IR_FIXED_BUNDLE) {
            emit_bundle(r->data.slots[0].text, r->data.slots[1].text,
                        r->data.slots[2].text, r->data.slots[3].text);
            state.previous_writes = 0;
            for (int slot = 0; slot < 4; slot++) state.previous_writes |= r->data.slots[slot].writes;
        } else if (r->kind == CG_IR_GAP) {
            emit_bundle(NULL, NULL, NULL, NULL);
            state.previous_writes = 0;
        } else {
            /* Operación de estado, aislada y con separación conservadora. */
            if (r->data.instruction.reads & state.previous_writes)
                emit_bundle(NULL, NULL, NULL, NULL);
            const char *text[4] = {NULL, NULL, NULL, NULL};
            text[r->data.instruction.original_slot] = r->data.instruction.text;
            emit_bundle(text[0], text[1], text[2], text[3]);
            emit_bundle(NULL, NULL, NULL, NULL);
            state.previous_writes = 0;
        }
    }
    if (flush_region(operations, &count, &state)) return 1;
    current_source_line = saved_line;
    finalized = 1;
    return 0;
}

/* ==========================================================================
 * LISTADOS Y DIRECCIONES
 * ======================================================================= */


/* cg_write — véase el contrato en codegen.h. */
void cg_write(FILE *out) {
    for (int i = 0; i < line_count; i++) {
        // Las etiquetas van pegadas al margen; los bundles con sangría.
        if (lines[i][0] == '{')
            fprintf(out, "  %s\n", lines[i]);
        else
            fprintf(out, "%s\n", lines[i]);
    }
}

/* cg_set_line — véase el contrato en codegen.h. */
void cg_set_line(int line) {
    current_source_line = line > 0 ? line : 0;
}

/* cg_label_address — véase el contrato en codegen.h. */
int cg_label_address(const char *name) {
    size_t length = strlen(name);

    for (int i = 0; i < line_count; i++) {

        if (lines[i][0] != '{' &&
            strlen(lines[i]) == length + 1 &&
            strncmp(lines[i], name, length) == 0 &&
            lines[i][length] == ':')
            return addresses[i];
    }

    return -1;
}

/*
 * internal_label — reconoce etiquetas internas de expansiones.
 *
 * Entrada:
 *   name  nombre no NULL.
 * Salida:
 *   1 para prefijos aritmético, arreglo o CRYPTO; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int internal_label(const char *name) {
    return strncmp(name, "__ccom_arith_", 13) == 0 ||
           strncmp(name, "__ccom_array_", 13) == 0 ||
           strncmp(name, ".L_crypto_", 10) == 0;
}

/*
 * write_padded — escribe texto y completa una columna con espacios.
 *
 * Entrada:
 *   out  flujo no NULL.
 *   text  texto no NULL.
 *   width  ancho mínimo en caracteres.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe en out.
 * Restricciones:
 *   no trunca texto más ancho que width.
 */
static void write_padded(FILE *out, const char *text, int width) {
    int length = (int) strlen(text);

    fputs(text, out);

    for (int i = length; i < width; i++)
        fputc(' ', out);
}

/* cg_write_listing — véase el contrato en codegen.h. */
void cg_write_listing(FILE *out) {
    static const char *const EMPTY = "{ nopi | nopi | nopi | nopi }";

    // Columna de los comentarios: el texto más largo, con un mínimo.
    int width = 44;

    for (int i = 0; i < line_count; i++) {
        int length = (int) strlen(lines[i]) + (lines[i][0] == '{' ? 2 : 0);

        if (length + 2 > width)
            width = length + 2;
    }

    // La línea del fuente solo se muestra cuando cambia. Los bundles vacíos
    // de separación no la llevan.
    int shown_line = 0;

    for (int i = 0; i < line_count; i++) {

        if (lines[i][0] != '{') {
            write_padded(out, lines[i], width);
            fprintf(out, "# bundle %d\n", addresses[i]);
            continue;
        }

        fputs("  ", out);
        write_padded(out, lines[i], width - 2);
        fprintf(out, "# %04d", addresses[i]);

        if (strcmp(lines[i], EMPTY) != 0) {

            if (source_lines[i] > 0 && source_lines[i] != shown_line) {
                fprintf(out, "  linea %d", source_lines[i]);
                shown_line = source_lines[i];
            }
            // Un bundle que no es de ninguna sentencia (el salto final de
            // una función, fin_main) lo dice, para que no se le atribuya la
            // línea anterior.
            else if (source_lines[i] == 0 && shown_line != 0) {
                fprintf(out, "  linea -");
                shown_line = 0;
            }
        }

        int mixed = 0;
        for (int slot = 0; slot < 4; slot++)
            if (slot_source_lines[i][slot] >= 0 && slot_source_lines[i][slot] != source_lines[i]) mixed = 1;
        if (mixed) {
            fputs("  slots ", out);
            for (int slot = 0; slot < 4; slot++)
                fprintf(out, "%s%d=%d", slot ? "," : "", slot, slot_source_lines[i][slot] < 0 ? 0 : slot_source_lines[i][slot]);
        }
        fputc('\n', out);
    }

    // Tabla de etiquetas en orden de dirección; en el empate, el de emisión.
    fprintf(out, "\n# Tabla de etiquetas (direccion = indice de bundle)\n");

    int previous = -1;

    for (;;) {
        int next = -1;

        for (int i = 0; i < line_count; i++) {

            if (lines[i][0] != '{' && addresses[i] > previous &&
                (next < 0 || addresses[i] < next))
                next = addresses[i];
        }

        if (next < 0)
            break;

        for (int i = 0; i < line_count; i++) {

            if (lines[i][0] == '{' || addresses[i] != next)
                continue;

            // La etiqueta sin los dos puntos.
            size_t length = strlen(lines[i]);
            fprintf(out, "#   %-28.*s %5d%s\n",
                    (int) (length > 0 ? length - 1 : 0), lines[i], next,
                    internal_label(lines[i]) ? "  (interna)" : "");
        }

        previous = next;
    }
}

/* ==========================================================================
 * CONSULTAS DEL IR Y SALIDA
 * ======================================================================= */


/* cg_ir_count — véase el contrato en codegen.h. */
int cg_ir_count(void) {
    return ir_count;
}

/* cg_ir_record — véase el contrato en codegen.h. */
const CGIRRecord *cg_ir_record(int index) {
    return index >= 0 && index < ir_count ? &ir[index] : NULL;
}

/* cg_line_count — véase el contrato en codegen.h. */
int cg_line_count(void) {
    return line_count;
}

/* cg_line — véase el contrato en codegen.h. */
const char *cg_line(int i) {
    return (i >= 0 && i < line_count) ? lines[i] : NULL;
}

/* cg_bundle_count — véase el contrato en codegen.h. */
int cg_bundle_count(void) {
    return bundles;
}

/* ==========================================================================
 * IMAGEN DE DATOS
 * ======================================================================= */


/* cg_data_set — véase el contrato en codegen.h. */
int cg_data_set(int addr, uint32_t value) {
    if (addr < 0 || addr + 4 > CG_DATA_BYTES || addr % 4 != 0)
        return -1;

    data[addr / 4] = value;

    if (addr + 4 > data_used)
        data_used = addr + 4;

    return 0;
}

/* cg_data_size — véase el contrato en codegen.h. */
int cg_data_size(void) {
    return data_used;
}

/* cg_data — véase el contrato en codegen.h. */
const uint32_t *cg_data(void) {
    return data;
}
