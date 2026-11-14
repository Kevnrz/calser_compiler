/*
 * main.c — Fases 1–5: coordinación del compilador y sus salidas.
 *
 * Interpreta banderas y encadena análisis, tablas, generación y ensamblado.
 * Permite ensamblar directamente con -x y exportar el AST como JSON.
 *
 * Usa: lexer/parser, semantic, diagnostics, label_table, generate, codegen y assembler.
 * Lo usa: línea de comandos ccom y scripts de pruebas/herramientas.
 * Referencias: enunciado §2.3–§2.5, §5.1.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <time.h>

#include "semantic.h"
#include "diagnostics.h"
#include "diagnostics_json.h"
#include "suggestions.h"
#include "repairs.h"
#include "clones.h"
#include "syntax_service.h"
#include "parser_service.h"
#include <errno.h>
#include "label_table.h"
#include "codegen.h"
#include "generate.h"
#include "tac_build.h"
#include "tac_lower.h"
#include "tac_emit.h"
#include "tac_passes.h"
#include "semantic_scope.h"
#include "assembler.h"

extern FILE *yyin;


// Cantidad de errores léxicos (lexer.l).
extern int lexical_errors;

/* Opciones de línea de comandos (enunciado CE 1108 §5.1):
 *
 *   ccom [opciones] <archivo> [-o <salida>]
 *
 *   -h, --help  muestra ayuda y termina sin compilar
 *   --time  muestra la duracion total en stderr
 *   --check  ejecuta solo análisis léxico/sintáctico/semántico, sin salidas
 *   -t  imprime el AST
 *   -m  imprime la tabla de símbolos con direcciones y la de etiquetas
 *   -v  imprime en stderr el avance de cada fase
 *   -o  nombre del binario de salida
 *   -s  genera además el .asm
 *   -p  muestra los bundles en ensamblador y en binario
 *   -x  la entrada es un .asm y se genera el binario, sin compilar Calser
 *
 * Sin --check, las compilaciones correctas escriben .bin, .hex y .mem con
 * el nombre base de -o.
 */
typedef struct {
    const char *input;         /* archivo fuente (.cls) o, con -x, .asm      */
    char       *output;        /* -o; por defecto: fuente sin extensión .bin */
    int show_time;             /* --time */
    int repair_json, syntax_json, clones_json;
    unsigned clone_minimum;
    int suggest_json;
    size_t suggest_cursor;
    int check_text;           /* distingue --check explícito en modo léxico */
    int tokens_json;          /* --tokens-json: solo lexer Flex y presentación */
    int check_json;           /* --check-json: mismo front-end, protocolo v1 */
    int check_only;            /* --check: fases léxica, sintáctica y semántica */
    int verbose;               /* -v */
    int emit_asm;              /* -s */
    int emit_tac;              /* -i / --tac */
    const char *ast_json;       /* --ast-json archivo */
    int no_schedule;           /* --no-schedule: comparación conservadora */
    int print_ast;             /* -t */
    int print_memory;          /* -m */
    int assemble_only;         /* -x */
    int print_bundles;         /* -p */
    int level;                 /* Nivel efectivo: cero u O1. */
    int explicit_level;        /* Bit por nivel explícito; detecta contradicciones. */
    int opt_seen;              /* --opt explícito, incluso vacío. */
    unsigned passes;           /* Unión de listas sin duplicados. */
    unsigned unroll_factor;    /* Cero: heurística del §11. */
    const char *metrics_path;  /* --metrics, solo con nivel efectivo O1. */
} Options;

#define EXIT_USAGE 2

enum {
    OPT_AST_JSON = 1000, OPT_NO_SCHEDULE, OPT_TIME,
    OPT_PASSES, OPT_UNROLL, OPT_METRICS,
    OPT_CHECK, OPT_CHECK_JSON, OPT_TOKENS_JSON, OPT_SUGGEST_JSON,
    OPT_REPAIR_JSON, OPT_SYNTAX_JSON, OPT_CLONES_JSON
};

#define USAGE "uso: ccom [-h|--help] [-v] [-t] [-m] [-s] [-i|--tac] [-p] [-x] [--check|--check-json|--tokens-json|--suggest-json <cursor>|--repair-json|--syntax-json|--clones-json <min>] [--ast-json <json>] [--no-schedule] [--time] [-O0|-O1|-O2] [--opt=lista] [--unroll=N] [--metrics archivo.json] <archivo> [-o <salida>]"

/*
 * print_help — describe las opciones sin requerir ni procesar una entrada.
 *
 * Entrada: ninguna. Salida: ninguna; escribe la ayuda en stdout.
 * Restricciones: mantener las opciones alineadas con getopt_long en main.
 */
static void print_help(void) {
    puts(USAGE);
    puts("\nCompila un fuente Calser (.cls); con -x ensambla un archivo .asm.\n"
         "Las opciones pueden ir antes o despues del archivo.\n"
         "\nOpciones:\n"
         "  -h, --help           Muestra esta ayuda y termina sin compilar.\n"
         "  -t                   Muestra el arbol sintactico (AST).\n"
         "  -m                   Muestra las tablas de simbolos y etiquetas.\n"
         "  -v                   Muestra el avance de las fases en stderr.\n"
         "  -o <salida>          Nombre del .bin y base de las demas salidas.\n"
         "  -s                   Escribe ademas el ensamblador .asm.\n"
         "  -i, --tac            Escribe ademas la representacion de alto nivel .tac.\n"
         "  -p                   Muestra los bundles en ensamblador y binario.\n"
         "  -x                   Ensambla la entrada .asm; ignora -s, -i, -t y -m.\n"
         "  --ast-json <json>    Exporta el AST; sin -s, -i, -m ni -p termina\n"
         "                       despues del analisis sintactico, salvo con -O1.\n"
         "  --no-schedule        Desactiva la organizacion de bundles.\n"
         "  -O0                  Generador actual (predeterminado).\n"
         "  -O1                  Camino ISA con asignacion de registros.\n"
         "  -O2                  Reservado hasta C3.5; por ahora es error de uso.\n"
         "  --opt=lista          Implica -O1; unroll,rename,dce,reorder,spill.\n"
         "  --unroll=N           Factor 2..16; exige unroll activo.\n"
         "  --metrics <json>     Metricas de una compilacion -O1 exitosa.\n"
         "  --time               Muestra la duracion total en ms en stderr.\n"
         "  --check              Solo analisis lexico, sintactico y semantico;\n"
         "                       no genera archivos. Compatible con -v y --time.\n"
         "  --check-json         Mismo analisis que --check; respuesta JSON v1 en\n"
         "                       stdout, sin archivos. Compatible con -v y --time.\n"
         "  --tokens-json        Tokens, comentarios y errores del lexer Flex en\n"
         "                       JSON v1, sin parser ni salidas. Admite -v y --time.\n"
         "  --suggest-json <n>   Sugerencias Bison/LAC en el cursor UTF-8 (bytes).\n"
         "                       Sin semantica ni artefactos. Admite -v y --time.\n"
         "  --repair-json        Reparaciones de cierres validadas por el parser C.\n"
         "  --syntax-json        Solo sintaxis completa en JSON, para verificacion.\n"
         "                       Ambas sin semantica ni artefactos; -v/--time.\n"
         "  --clones-json <n>    Clones Tipo I/II sobre un paquete CCL1 de instantaneas.\n"
         "                       Minimo 5..100000 tokens; solo -v y --time.\n"
         "\nConsultas: solo -v y --time; sin niveles, optimizacion, metricas ni -i.\n\nSalidas: .bin, .hex y .mem; con -s tambien .asm; con -i tambien .tac.\n"
         "Sin -o se usa el nombre de la entrada con extension .bin.\n"
         "El exito y las rutas generadas se informan en stderr; stdout\n"
         "se conserva para AST, tablas y bundles.\n"
         "\nEjemplos:\n"
         "  ccom programa.cls\n"
         "  ccom -i programa.cls -o salida.bin\n"
         "  ccom --time -s programa.cls -o salida.bin\n"
         "  ccom -x programa.asm\n"
         "  ccom --ast-json arbol.json programa.cls\n"
         "  ccom --check programa.cls\n"
         "\nCodigos de salida:\n"
         "  0  Trabajo completado o ayuda mostrada.\n"
         "  1  Error de compilacion, ensamblado o archivos.\n"
         "  2  Uso incorrecto de la linea de comandos.");
}

/*
 * report_elapsed — muestra tiempo de pared medido con reloj monotónico.
 *
 * Entrada: inicio válido, opciones y código de salida del trabajo realizado.
 * Salida: ninguna; escribe en stderr, sin alterar el código de salida.
 * Restricciones: llamar solo si --time obtuvo la lectura inicial del reloj.
 */
static void report_elapsed(const Options *options, const struct timespec *start,
                           int code) {
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        fprintf(stderr, "ccom: aviso: no se pudo medir la duracion\n");
        return;
    }

    double milliseconds = (double) (end.tv_sec - start->tv_sec) * 1000.0
                        + (double) (end.tv_nsec - start->tv_nsec) / 1000000.0;
    const char *operation = options->assemble_only ? "ensamblado" : "compilacion";
    if (options->check_only)
        operation = "analisis";
    if (!options->assemble_only && options->ast_json && !options->emit_asm &&
        !options->print_memory && !options->print_bundles && !options->emit_tac && !options->level)
        operation = "exportacion AST";

    fprintf(stderr, "ccom: tiempo de %s (%s): %.3f ms\n", operation,
            code == 0 ? "completado" : "fallido", milliseconds);
}

/* ==========================================================================
 * NOMBRES Y ARCHIVOS DE SALIDA
 * ======================================================================= */


/*
 * default_output — construye el nombre binario por defecto.
 *
 * Entrada:
 *   input  ruta de entrada no NULL.
 * Salida:
 *   cadena nueva con .bin; NULL sin memoria.
 *   Efectos: reserva cadena; el llamador la libera.
 * Restricciones:
 *   solo reemplaza extensión del nombre, no de directorios.
 */
static char *default_output(const char *input) {
    const char *slash = strrchr(input, '/');
    const char *base = slash ? slash + 1 : input;
    const char *dot = strrchr(base, '.');

    size_t length = (dot != NULL && dot > base)
                        ? (size_t) (dot - input)
                        : strlen(input);

    char *output = malloc(length + sizeof(".bin"));

    if (output == NULL)
        return NULL;

    memcpy(output, input, length);
    strcpy(output + length, ".bin");

    return output;
}

/*
 * with_extension — construye una salida con otra extensión.
 *
 * Entrada:
 *   output  ruta base no NULL.
 *   extension  extensión no NULL, incluido el punto.
 * Salida:
 *   cadena nueva o NULL sin memoria.
 *   Efectos: reserva cadena; el llamador la libera.
 * Restricciones:
 *   ninguna.
 */
static char *with_extension(const char *output, const char *extension) {
    const char *slash = strrchr(output, '/');
    const char *base = slash ? slash + 1 : output;
    const char *dot = strrchr(base, '.');

    size_t length = (dot != NULL && dot > base)
                        ? (size_t) (dot - output)
                        : strlen(output);

    char *path = malloc(length + strlen(extension) + 1);

    if (path == NULL)
        return NULL;

    memcpy(path, output, length);
    strcpy(path + length, extension);

    return path;
}

/*
 * report_phase — informa el resultado de una fase con -v.
 *
 * Entrada:
 *   options  opciones no NULL.
 *   phase  nombre de fase.
 *   ok  valor no cero si tuvo éxito.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe en stderr cuando verbose está activo.
 * Restricciones:
 *   ninguna.
 */
static void report_phase(const Options *options, const char *phase, int ok) {
    if (options->verbose)
        fprintf(stderr, "ccom: %s ... %s\n", phase, ok ? "ok" : "con errores");
}

/*
 * report_oom — informa que falló una reserva de memoria.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe el mensaje de memoria insuficiente en stderr.
 * Restricciones:
 *   el llamador decide el código de salida; un solo texto para todos los
 *   puntos donde main.c reserva memoria.
 */
static void report_oom(void) {
    fprintf(stderr, "ccom: error: memoria insuficiente\n");
}

/*
 * open_output — abre un archivo con el nombre base de -o.
 *
 * Entrada:
 *   options  opciones con output.
 *   extension  extensión con punto.
 *   mode  modo para fopen.
 *   path  puntero escribible para ruta.
 * Salida:
 *   flujo abierto o NULL si falla.
 *   Efectos: reserva *path, abre/trunca archivo y reporta errores.
 * Restricciones:
 *   liberar *path aun si fopen falla; usar close_output.
 */
static FILE *open_output(const Options *options, const char *extension,
                         const char *mode, char **path) {
    *path = with_extension(options->output, extension);

    if (*path == NULL) {
        report_oom();
        return NULL;
    }

    FILE *out = fopen(*path, mode);

    if (out == NULL)
        perror(*path);

    return out;
}

/*
 * close_output — cierra una salida y libera su ruta.
 *
 * Entrada:
 *   out  flujo abierto o NULL.
 *   path  ruta dinámica o NULL.
 * Salida:
 *   0 si cierra sin error; 1 si falla o no hay flujo.
 *   Efectos: cierra out, reporta fallo y libera path.
 * Restricciones:
 *   ninguna.
 */
static int close_output(FILE *out, char *path) {
    int failed = 1;

    if (out != NULL) {
        failed = ferror(out);

        if (fclose(out) != 0)
            failed = 1;

        if (failed)
            fprintf(stderr, "ccom: error al escribir %s\n", path);
    }

    free(path);

    return failed;
}

/*
 * write_word — escribe una palabra little endian.
 *
 * Entrada:
 *   out  flujo no NULL.
 *   word  palabra de 32 bits.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe cuatro bytes.
 * Restricciones:
 *   errores del flujo se comprueban al cerrar.
 */
static void write_word(FILE *out, uint32_t word) {
    unsigned char bytes[4] = {
        (unsigned char) (word & 0xFF),
        (unsigned char) ((word >> 8) & 0xFF),
        (unsigned char) ((word >> 16) & 0xFF),
        (unsigned char) ((word >> 24) & 0xFF)
    };

    fwrite(bytes, 1, sizeof(bytes), out);
}

/* ==========================================================================
 * ENSAMBLADO Y SALIDAS
 * ======================================================================= */


/*
 * write_outputs — escribe las imágenes binaria, hexadecimal y de datos.
 *
 * Entrada:
 *   options  opciones con ruta output.
 *   program  programa ensamblado.
 *   data  palabras de imagen o NULL si vacía.
 *   data_size  cantidad de bytes de datos.
 * Salida:
 *   0 si escribe todo; 1 si alguna salida falla.
 *   Efectos: crea/trunca .bin, .hex y .mem; reporta errores.
 * Restricciones:
 *   data_size múltiplo de cuatro; data contiene data_size/4 palabras.
 */
static int write_outputs(const Options *options, const AsmProgram *program,
                         const uint32_t *data, int data_size) {
    int failed = 0;
    char *path;
    FILE *out;

    /* .bin: encabezado de 4 palabras (cantidad de bundles, punto de entrada
     * 0, tamaño de datos en bytes, dirección de datos 0); cada bundle en 16
     * bytes, slot 0 primero; y los datos. Todo little endian. */
    out = open_output(options, ".bin", "wb", &path);

    if (out != NULL) {
        write_word(out, (uint32_t) program->count);
        write_word(out, 0);
        write_word(out, (uint32_t) data_size);
        write_word(out, 0);

        for (int i = 0; i < program->count; i++) {

            for (int slot = 0; slot < 4; slot++)
                write_word(out, program->bundles[i][slot]);
        }

        for (int word = 0; word < data_size / 4; word++)
            write_word(out, data[word]);
    }

    failed |= close_output(out, path);

    // .hex: un bundle por línea, slot3_slot2_slot1_slot0 (isa.md §11.1).
    out = open_output(options, ".hex", "w", &path);

    if (out != NULL) {

        for (int i = 0; i < program->count; i++)
            fprintf(out, "%08X_%08X_%08X_%08X\n",
                    (unsigned) program->bundles[i][3],
                    (unsigned) program->bundles[i][2],
                    (unsigned) program->bundles[i][1],
                    (unsigned) program->bundles[i][0]);
    }

    failed |= close_output(out, path);

    // .mem: @0 y una palabra de datos por línea.
    out = open_output(options, ".mem", "w", &path);

    if (out != NULL) {
        fprintf(out, "@0\n");

        for (int word = 0; word < data_size / 4; word++)
            fprintf(out, "%08X\n", (unsigned) data[word]);
    }

    failed |= close_output(out, path);

    return failed;
}

/*
 * assemble_and_write — ensambla líneas y escribe las salidas seleccionadas.
 *
 * Entrada:
 *   options  opciones.
 *   lines  arreglo de líneas.
 *   nlines  cantidad de líneas.
 *   name  nombre de entrada para diagnóstico.
 *   data  imagen de datos o NULL si vacía.
 *   data_size  tamaño de datos en bytes.
 * Salida:
 *   0 si ensambla y escribe; 1 si falla.
 *   Efectos: ensambla, muestra bundles con -p, escribe archivos y libera AsmProgram.
 * Restricciones:
 *   datos cumplen write_outputs; líneas cumplen asm_assemble.
 */
static int assemble_and_write(const Options *options,
                              const char *const *lines, int nlines,
                              const char *name,
                              const uint32_t *data, int data_size) {
    AsmProgram program;

    if (asm_assemble(lines, nlines, name, &program) != 0) {
        report_phase(options, "ensamblado", 0);
        return 1;
    }

    /* Límite de la imagen final, común a O0/O1/-x (enunciado §3.2.1).
     * El nombre real evita filtrar el origen interno "<generado>" de O0. */
    char error[512];
    if (!asm_check_rom((size_t)program.count, options->input, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        report_phase(options, "ensamblado", 0);
        asm_free(&program);
        return 1;
    }

    report_phase(options, "ensamblado", 1);

    // -p: índice del bundle, el bundle en ensamblador y como en el .hex.
    if (options->print_bundles) {

        for (int i = 0; i < program.count; i++)
            printf("%04d  %s  %08X_%08X_%08X_%08X\n",
                   i,
                   program.text[i],
                   (unsigned) program.bundles[i][3],
                   (unsigned) program.bundles[i][2],
                   (unsigned) program.bundles[i][1],
                   (unsigned) program.bundles[i][0]);
    }

    int failed = write_outputs(options, &program, data, data_size);

    asm_free(&program);

    return failed ? 1 : 0;
}

/*
 * assemble_file — procesa una entrada ensamblador con -x.
 *
 * Entrada:
 *   options  opciones con input/output.
 * Salida:
 *   0 si ensambla y escribe; 1 si falla.
 *   Efectos: lee .asm, escribe salidas y reporta errores.
 * Restricciones:
 *   sin imagen de datos: .mem contiene solo @0.
 */
static int assemble_file(const Options *options) {
    FILE *in = fopen(options->input, "r");

    if (!in) {
        perror(options->input);
        return 1;
    }

    char **lines = NULL;
    int count = 0;
    int capacity = 0;

    char *buffer = NULL;
    size_t size = 0;

    int failed = 0;

    while (getline(&buffer, &size, in) != -1) {

        if (count == capacity) {
            int grown_capacity = capacity == 0 ? 256 : capacity * 2;
            char **grown =
                realloc(lines, (size_t) grown_capacity * sizeof(char *));

            if (grown == NULL) {
                failed = 1;
                break;
            }

            lines = grown;
            capacity = grown_capacity;
        }

        lines[count] = strdup(buffer);

        if (lines[count] == NULL) {
            failed = 1;
            break;
        }

        count++;
    }

    free(buffer);

    fclose(in);

    int code = 1;

    if (failed)
        report_oom();
    else
        code = assemble_and_write(options, (const char *const *) lines, count,
                                  options->input, NULL, 0);

    for (int i = 0; i < count; i++)
        free(lines[i]);

    free(lines);

    return code;
}

/* ==========================================================================
 * FASES 1–4 Y RESOLUCIÓN DE ETIQUETAS
 * ======================================================================= */


/*
 * Compilation — estado de una compilación en curso.
 *
 * Reúne lo que cada fase crea y lo que hay que liberar al terminar, para que
 * todas las salidas (éxito o error) cierren con compile_release. Cada campo
 * dice hasta dónde llegó la compilación: compile_release libera solo eso.
 */
typedef struct {
    const Options *options;
    ASTNode *root;         /* AST entregado por el parser; NULL si no hubo   */
    LabelTable *labels;    /* tabla de etiquetas; NULL hasta construirla     */
    TacBuildResult *tac;   /* construcción opcional; posee imagen y auxiliares */
    TacProgram *isa;       /* Programa bajado propio; comparte símbolos de tac. */
    TacAllocationResult *allocation; /* Plan C2, liberar antes de ISA. */
    char *initial_tac;      /* Listado HIGH previo a pasadas, solo -O1 con -i. */
    TacMetrics metrics;    /* Acumulación de E, sin estado global. */
    int semantic_started;  /* hay tabla de símbolos que liberar              */
    int codegen_started;   /* hay código generado que liberar                */
    int exit_code;         /* código de salida cuando una fase corta el paso */
} Compilation;

/*
 * compile_release — libera lo que las fases dejaron vivo.
 *
 * Entrada:
 *   compilation  estado de la compilación no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera etiquetas, símbolos, código, AST y líneas del fuente.
 * Restricciones:
 *   el orden importa: semantic_free invalida los enlaces symbol del AST, así
 *   que va antes de ast_free; diag_free va al final y siempre se ejecuta.
 */
static void compile_release(Compilation *compilation) {
    tac_metrics_free(&compilation->metrics);
    tac_allocation_free(compilation->allocation);
    tac_program_free(compilation->isa);
    free(compilation->initial_tac);
    tac_build_free(compilation->tac);

    if (compilation->labels != NULL) {
        label_table_destroy(compilation->labels);

        free(compilation->labels);
    }

    if (compilation->semantic_started)
        semantic_free();

    if (compilation->codegen_started)
        cg_free();

    if (compilation->root != NULL)
        ast_free(compilation->root);

    diag_free();
}

/*
 * print_labels — imprime la tabla de etiquetas cuando se pidió -m.
 *
 * Entrada:
 *   compilation  estado con la tabla de etiquetas ya construida.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe la tabla en stdout si -m está activo.
 * Restricciones:
 *   la tabla se imprime al final, con las direcciones que se conozcan
 *   (enunciado CE1108 §2.3): el nombre y el nodo salen de la Fase 3, pero
 *   la dirección de cada etiqueta solo existe después de generar el código
 *   (§2.4) y ensamblarlo (§2.5). Sin código generado se imprime con "-".
 */
static void print_labels(const Compilation *compilation) {
    if (!compilation->options->print_memory)
        return;
    if (!compilation->options->level) {
        label_table_print(compilation->labels);
        return;
    }
    puts("Etiquetas emitidas (direccion final):");
    for (int k = 0; k < cg_ir_count(); k++) {
        const CGIRRecord *record = cg_ir_record(k);
        if (record->kind == CG_IR_LABEL)
            printf("  %s = %d\n", record->data.label, cg_label_address(record->data.label));
    }
}

/*
 * phase_parse — fases 1–2: análisis léxico y sintáctico.
 *
 * Entrada:
 *   compilation  estado con options; el fuente ya está abierto en yyin.
 * Salida:
 *   1 si hay AST y la compilación sigue; 0 si corta (exit_code queda en 1).
 *   Efectos: inicializa diagnósticos, construye el AST y reporta errores.
 * Restricciones:
 *   con errores léxicos no se sigue: la recuperación (descartar, truncar,
 *   sustituir por 0) cambia el programa y el semántico podría reportar
 *   errores que no existen.
 */
static int phase_parse(Compilation *compilation) {
    const Options *options = compilation->options;

    // §8.2: los errores muestran el archivo y la línea del programa.
    diag_init(options->input);

    // Aceptación completa: docs/ccom_servicios_ide.md#contrato-yyparse.
    int accepted = parser_accept(yyin, &compilation->root);

    fclose(yyin);

    // El árbol inicial, si llegó a construirse, pertenece a main. Un AST
    // recuperado se libera normalmente, pero jamás se exporta ni se analiza.

    if (lexical_errors > 0) {
        fprintf(stderr,
                "Analisis lexico fallido: %d error(es).\n",
                lexical_errors);

        report_phase(options, "fase 1-2: analisis lexico y sintactico", 0);

        compilation->exit_code = 1;

        return 0;
    }

    if (!accepted) {
        if (parser_recovery_limited())
            fprintf(stderr, "Analisis sintactico incompleto: limite de recuperacion alcanzado.\n");
        report_phase(options, "fase 1-2: analisis lexico y sintactico", 0);

        compilation->exit_code = 1;

        return 0;
    }

    report_phase(options, "fase 1-2: analisis lexico y sintactico", 1);

    return 1;
}

/*
 * phase_export_ast — salidas del AST: -t y --ast-json.
 *
 * Entrada:
 *   compilation  estado con el AST del parser.
 * Salida:
 *   1 si la compilación sigue; 0 si corta (exit_code 0 o 1 según el JSON).
 *   Efectos: imprime el AST o escribe el JSON.
 * Restricciones:
 *   --ast-json solo exporta y termina después del análisis sintáctico, salvo
 *   que se combine con -s, -i, -m, -p u -O1; permite inspeccionar construcciones que
 *   el semántico o el generador rechazarían.
 */
static int phase_export_ast(Compilation *compilation) {
    const Options *options = compilation->options;

    if (options->print_ast)
        ast_print(compilation->root, 0);

    if (options->ast_json) {
        struct stat src_stat, dst_stat;
        int same = stat(options->input, &src_stat) == 0 &&
                   stat(options->ast_json, &dst_stat) == 0 &&
                   src_stat.st_dev == dst_stat.st_dev && src_stat.st_ino == dst_stat.st_ino;
        FILE *out = same ? NULL : fopen(options->ast_json, "w");
        int failed = 0;
        if (same) {
            fprintf(stderr, "ccom: el JSON no puede sobrescribir el fuente\n");
            failed = 1;
        } else if (!out) {
            perror(options->ast_json);
            failed = 1;
        } else {
            failed = ast_write_json(out, compilation->root, options->input);
            if (fclose(out) != 0) failed = 1;
            if (failed) fprintf(stderr, "ccom: error al escribir AST JSON\n");
        }
        /* Exportación sola: inspeccionar incluso construcciones sin codegen. */
        if (failed || (!options->emit_asm && !options->print_memory && !options->print_bundles && !options->emit_tac && !options->level)) {
            compilation->exit_code = failed ? 1 : 0;

            return 0;
        }
    }

    return 1;
}

/*
 * phase_semantic — fase 3: análisis semántico y tabla de símbolos.
 *
 * Entrada:
 *   compilation  estado con el AST.
 * Salida:
 *   1 si el análisis pasa; 0 si corta (exit_code queda en 1).
 *   Efectos: anota el AST, reserva direcciones, reporta errores y, con -m,
 *   imprime la tabla de símbolos.
 * Restricciones:
 *   S-03: si el análisis falla no hay tabla y el código de salida es 1; los
 *   símbolos deben vivir hasta terminar la generación.
 */
static int phase_semantic(Compilation *compilation) {
    const Options *options = compilation->options;
    int errors_before = diag_count();

    compilation->semantic_started = 1;

    if (!semantic_analyze(compilation->root)) {
        fprintf(stderr,
                "Analisis semantico fallido: %d error(es).\n",
                diag_count() - errors_before);

        report_phase(options, "fase 3: analisis semantico", 0);

        compilation->exit_code = 1;

        return 0;
    }

    report_phase(options, "fase 3: analisis semantico", 1);

    if (options->print_memory)
        semantic_print_symbols();

    return 1;
}

/*
 * phase_labels — fase 3: tabla de etiquetas.
 *
 * Entrada:
 *   compilation  estado con el AST ya analizado.
 * Salida:
 *   1 si la tabla queda construida; 0 si corta (exit_code queda en 1).
 *   Efectos: reserva y llena compilation->labels.
 * Restricciones:
 *   compilation->labels se asigna apenas existe la tabla inicializada, así
 *   compile_release la destruye también si label_table_build falla.
 */
static int phase_labels(Compilation *compilation) {
    const Options *options = compilation->options;
    LabelTable *labels = malloc(sizeof(LabelTable));

    if (labels == NULL) {
        report_phase(options, "fase 3: tabla de etiquetas", 0);

        compilation->exit_code = 1;

        return 0;
    }

    label_table_init(labels);

    compilation->labels = labels;

    if (label_table_build(labels, compilation->root) != 0) {
        report_phase(options, "fase 3: tabla de etiquetas", 0);

        compilation->exit_code = 1;

        return 0;
    }

    report_phase(options, "fase 3: tabla de etiquetas", 1);

    return 1;
}

/*
 * phase_tac — construye con -i o -O1, después del semántico (enunciado §4.1).
 * Entrada: compilation con AST y símbolos válidos. Salida: 1 éxito, 0 ante error.
 * Efectos: reserva TAC opcional; no toca estado del generador ni direcciones.
 * Restricciones: compile_release libera TAC antes del ámbito prestado.
 */
static int phase_tac(Compilation *compilation) {
    if (!compilation->options->emit_tac && !compilation->options->level)
        return 1;
    char error[256];
    compilation->tac = tac_build(compilation->root, scopes_global(), error, sizeof error);
    if (compilation->options->level)
        report_phase(compilation->options, "O1: construccion HIGH", compilation->tac != NULL);
    if (!compilation->tac) {
        fprintf(stderr, "%s: error de representacion intermedia: %s\n",
                compilation->options->input, error);
        compilation->exit_code = 1;
        return 0;
    }
    if (compilation->options->level) {
        compilation->metrics.high_initial =
            tac_metrics_instruction_count(compilation->tac->program);
        if (compilation->options->emit_tac) {
            size_t size;
            FILE *snapshot = open_memstream(&compilation->initial_tac, &size);
            if (!snapshot) {
                fprintf(stderr, "%s: no se pudo capturar HIGH inicial\n",
                        compilation->options->input);
                compilation->exit_code = 1;
                return 0;
            }
            tac_build_print(snapshot, compilation->tac);
            int failed = ferror(snapshot);
            if (fclose(snapshot) != 0)
                failed = 1;
            if (failed) {
                fprintf(stderr, "%s: error al capturar HIGH inicial\n",
                        compilation->options->input);
                compilation->exit_code = 1;
                return 0;
            }
        }
    }
    return 1;
}

/* Entrada: compilación y pasada. Salida: ninguna. Efectos: progreso común.
 * Restricciones: adaptador de firma; conserva report_phase del camino P1.
 */
static void report_pass(void *user, const char *name, int ok) {
    const Compilation *compilation = user;
    report_phase(compilation->options, name, ok);
}

/* Entrada: compilación semánticamente válida. Salida: 1 o diagnóstico real.
 * Efectos: HIGH -> ISA -> asignación -> emisor D; Aho §8.1 y §10.2.3.
 * Restricciones: E no transforma ni derrama; D organiza y comprueba ROM una vez.
 */
static int phase_o1(Compilation *c) {
    const Options *o = c->options;
    char error[512] = "";
    c->metrics.level = (unsigned)o->level;
    TacPassContext context = {.program = c->tac->program, .build = c->tac,
        .scopes = scopes_first(), .source_name = o->input, .enabled = o->passes,
        .unroll_factor = o->unroll_factor, .metrics = &c->metrics,
        .report = report_pass, .report_user = c};
    if (!tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error))
        goto fail;
    c->isa = tac_lower(c->tac->program, error, sizeof error);
    report_phase(o, "O1: bajada a ISA virtual", c->isa != NULL);
    if (!c->isa)
        goto fail;
    context.program = c->isa;
    if (!tac_passes_run(&context, TAC_PASS_VIRTUAL, error, sizeof error))
        goto fail;
    TacAllocationOptions allocation_options = {
        .spill = !!(o->passes & (1u << tac_pass_find("spill", 5)))};
    c->allocation = tac_allocate_registers(c->isa, o->input, &allocation_options, error, sizeof error);
    report_phase(o, "O1: asignacion de registros", c->allocation != NULL);
    if (!c->allocation)
        goto fail;
    /* Spill es política de asignación, no pasada del gestor (§9.9). */
    int spill = tac_pass_find("spill", 5);
    if (o->passes & (1u << spill)) {
        c->metrics.executed |= 1u << spill;
        for (const TacFunction *f = c->isa->first; f; f = f->next)
            c->metrics.pass_ms[spill] += tac_allocation_metrics(c->allocation, f)->spill_ms;
        report_pass(c, "spill", 1);
    }
    c->metrics.isa_before = tac_metrics_instruction_count(c->isa);
    c->codegen_started = 1;
    TacEmitInput input = {.program = c->isa, .allocation = c->allocation,
        .initial_data = c->tac->initial_data, .initial_data_size = c->tac->initial_data_size,
        .labels = c->labels, .source_name = o->input, .no_schedule = o->no_schedule};
    int ok = tac_emit_program(&input, error, sizeof error);
    report_phase(o, "O1: emision y bundles finales", ok);
    if (!ok)
        goto fail;
    tac_metrics_collect(&c->metrics, c->isa, c->allocation);
    label_table_resolve(c->labels, cg_label_address);
    return 1;
fail:
    if (strstr(error, o->input) == error)
        fprintf(stderr, "%s\n", error);
    else
        fprintf(stderr, "%s: %s\n", o->input, error);
    c->exit_code = 1;
    return 0;
}

/*
 * phase_codegen — fase 4: generación, organización de bundles y direcciones.
 *
 * Entrada:
 *   compilation  estado con AST, símbolos y tabla de etiquetas.
 * Salida:
 *   1 si el calendario queda definitivo; 0 si corta (exit_code queda en 1).
 *   Efectos: genera el código, lo organiza (salvo --no-schedule) y resuelve
 *   las direcciones de la tabla de etiquetas.
 * Restricciones:
 *   si falla, la tabla de etiquetas de -m se imprime igual, con "-" en las
 *   direcciones, antes de liberar.
 */
static int phase_codegen(Compilation *compilation) {
    const Options *options = compilation->options;

    if (options->level)
        return phase_o1(compilation);
    compilation->codegen_started = 1;

    int gen_failed = generate_code(compilation->root, compilation->labels, options->input);
    report_phase(options, "fase 4: generacion de codigo", !gen_failed);
    if (!gen_failed && !options->no_schedule) {
        gen_failed = cg_schedule();
        report_phase(options, "fase 4: organizacion de bundles", !gen_failed);
    }
    if (gen_failed) {
        print_labels(compilation);

        compilation->exit_code = 1;

        return 0;
    }

    // El calendario ya es definitivo. Resolver con sus índices finales,
    // antes del listado y del ensamblado, nunca con las direcciones del IR
    // conservador. La primera pasada del ensamblador cuenta esos mismos
    // bundles (libro §6.7, backpatching; enunciado §2.5). Una etiqueta que no
    // llega al .asm (un & constante en un inicializador global) queda con -1.
    label_table_resolve(compilation->labels, cg_label_address);

    return 1;
}

/*
 * write_listing — escribe el .asm anotado de -s.
 *
 * Entrada:
 *   options  opciones con la ruta base de salida.
 *   header  comentario inicial con el nombre del fuente.
 * Salida:
 *   0 si escribe todo; 1 si falla.
 *   Efectos: crea/trunca el .asm y reporta errores de escritura.
 * Restricciones:
 *   anotado según el enunciado CE1108 §6.2: la dirección y la línea del
 *   fuente de cada bundle, la tabla de etiquetas con sus direcciones y la
 *   de variables con sus direcciones de datos. Todo lo agregado va en
 *   comentarios con # (isa.md §11.1): ccom -x da los mismos bundles.
 */
static int write_listing(const Options *options, const char *header) {
    char *path;
    FILE *out = open_output(options, ".asm", "w", &path);

    if (out != NULL) {
        fprintf(out, "%s\n", header);
        cg_write_listing(out);
        fprintf(out, "\n# Variables (direccion de datos)\n");
        semantic_fprint_symbols(out, "#   ");
    }

    return close_output(out, path);
}

/*
 * assemble_generated — ensambla las líneas del generador y escribe las salidas.
 *
 * Entrada:
 *   options  opciones con la ruta base de salida.
 *   header  primera línea del .asm, ya construida.
 * Salida:
 *   0 si ensambla y escribe; 1 si falla.
 *   Efectos: ensambla, escribe .bin/.hex/.mem y reporta errores.
 * Restricciones:
 *   el ensamblado toma las líneas del generador (contrato §1) después de la
 *   cabecera; es la misma línea que escribe -s, así los errores de
 *   <generado> citan la línea del .asm.
 */
static int assemble_generated(const Options *options, const char *header) {
    int line_count = cg_line_count() + 1;
    const char **lines = malloc((size_t) line_count * sizeof(char *));

    if (lines == NULL) {
        report_oom();

        return 1;
    }

    lines[0] = header;

    for (int i = 1; i < line_count; i++)
        lines[i] = cg_line(i - 1);

    int failed = assemble_and_write(options, lines, line_count,
                                    options->level ? options->input : "<generado>",
                                    cg_data(), cg_data_size()) != 0;

    free(lines);

    return failed;
}

/*
 * write_tac — escribe imagen y listado al completar la generación actual.
 * Entrada: compilation con TAC válido. Salida: 0 éxito; 1 ante error de archivo.
 * Efectos: crea .tac; ante error de escritura/cierre elimina el archivo parcial.
 * Restricciones: no puede sobrescribir el fuente, tampoco por enlace al mismo inode.
 */
static int write_tac(const Compilation *compilation) {
    const Options *options = compilation->options;
    char *path = with_extension(options->output, ".tac");
    if (!path) {
        report_oom();
        return 1;
    }
    struct stat source;
    struct stat target;
    if (stat(options->input, &source) == 0 && stat(path, &target) == 0 &&
        source.st_dev == target.st_dev && source.st_ino == target.st_ino) {
        fprintf(stderr, "ccom: el TAC no puede sobrescribir el fuente\n");
        free(path);
        return 1;
    }
    FILE *out = fopen(path, "w");
    if (!out) {
        perror(path);
        free(path);
        return 1;
    }
    if (compilation->initial_tac)
        fputs(compilation->initial_tac, out);
    else
        tac_build_print(out, compilation->tac);
    int failed = ferror(out);
    if (fclose(out) != 0)
        failed = 1;
    if (failed) {
        fprintf(stderr, "ccom: error al escribir %s\n", path);
        if (remove(path) != 0)
            perror(path);
    }
    free(path);
    return failed;
}

/*
 * phase_emit — escribe el .asm (-s) y los binarios de una compilación exitosa.
 *
 * Entrada:
 *   compilation  estado con el código generado y las etiquetas resueltas.
 * Salida:
 *   0 si completa; 1 si falla alguna salida.
 *   Efectos: escribe .asm, .bin, .hex y .mem y, con -m, la tabla de
 *   etiquetas con sus direcciones.
 * Restricciones:
 *   última fase: su valor de retorno es el código de salida de compile.
 */
static int phase_emit(const Compilation *compilation) {
    const Options *options = compilation->options;

    // Primera línea del .asm: un comentario con el fuente.
    size_t header_size = strlen(options->input) + sizeof("# ccom: ");
    char *header = malloc(header_size);

    if (header == NULL) {
        report_oom();

        print_labels(compilation);

        return 1;
    }

    snprintf(header, header_size, "# ccom: %s", options->input);

    int output_failed = 0;

    if (options->emit_asm)
        output_failed = write_listing(options, header);

    if (assemble_generated(options, header) != 0)
        output_failed = 1;

    if (!output_failed && options->emit_tac)
        output_failed = write_tac(compilation);

    free(header);

    print_labels(compilation);

    return output_failed ? 1 : 0;
}

/*
 * compile — coordina las fases de compilación y las salidas.
 *
 * Entrada:
 *   options  opciones con rutas y banderas.
 * Salida:
 *   0 si completa; 1 si falla alguna fase o salida.
 *   Efectos: lee fuente, crea/libera AST/tablas/IR; escribe archivos y diagnósticos.
 * Restricciones:
 *   las fases corren en orden y la primera que corta fija el código de
 *   salida; los símbolos viven hasta terminar la generación y todo se libera
 *   al final con compile_release.
 */
static int compile(const Options *options) {
    double started = options->metrics_path ? tac_metrics_clock() : 0;
    yyin = fopen(options->input, "r");

    if (!yyin) {
        perror(options->level ? options->input : "fopen");
        return 1;
    }

    Compilation compilation = { .options = options };
    int code;

    if (options->check_only) {
        /* E4: ejecutar exactamente el front-end existente y liberar su AST/
         * símbolos, sin exportar, generar código ni abrir archivos de salida. */
        code = phase_parse(&compilation) && phase_semantic(&compilation)
                   ? 0 : compilation.exit_code;
    } else if (phase_parse(&compilation) &&
        phase_export_ast(&compilation) &&
        phase_semantic(&compilation) &&
        phase_labels(&compilation) &&
        phase_tac(&compilation) &&
        phase_codegen(&compilation))
        code = phase_emit(&compilation);
    else
        code = compilation.exit_code;

    if (!code && options->metrics_path &&
        !tac_metrics_write(options->metrics_path, options->input, &compilation.metrics,
                           tac_metrics_clock() - started))
        code = 1;
    compile_release(&compilation);

    return code;
}

/* ==========================================================================
 * BANDERAS Y PUNTO DE ENTRADA
 * ======================================================================= */


/*
 * parse_options — interpreta las banderas y el archivo de entrada.
 *
 * Entrada:
 *   argc  cantidad de argumentos.
 *   argv  arreglo de argumentos del proceso.
 *   options  opciones a llenar; inicializadas en cero.
 * Salida:
 *   -1 si hay que seguir; en otro caso, el código con que termina el
 *   proceso: 0 tras la ayuda, 1 sin memoria, 2 por uso incorrecto.
 *   Efectos: getopt_long puede reordenar argv; escribe ayuda o mensajes.
 * Restricciones:
 *   exactamente una entrada salvo ayuda; si devuelve un código, options->output
 *   ya quedó liberado.
 */
static int parse_options(int argc, char **argv, Options *options) {
    int option;

    // Sin '+' al inicio: getopt reordena, así que las opciones pueden ir
    // antes o después del archivo (ccom prog.cls -o prog.bin).
    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"ast-json", required_argument, NULL, OPT_AST_JSON},
        {"no-schedule", no_argument, NULL, OPT_NO_SCHEDULE},
        {"time", no_argument, NULL, OPT_TIME},
        {"tac", no_argument, NULL, 'i'},
        {"opt", required_argument, NULL, OPT_PASSES},
        {"unroll", required_argument, NULL, OPT_UNROLL},
        {"metrics", required_argument, NULL, OPT_METRICS},
        {"check", no_argument, NULL, OPT_CHECK},
        {"check-json", no_argument, NULL, OPT_CHECK_JSON},
        {"tokens-json", no_argument, NULL, OPT_TOKENS_JSON},
        {"suggest-json", required_argument, NULL, OPT_SUGGEST_JSON},
        {"repair-json", no_argument, NULL, OPT_REPAIR_JSON},
        {"syntax-json", no_argument, NULL, OPT_SYNTAX_JSON},
        {"clones-json", required_argument, NULL, OPT_CLONES_JSON},
        {NULL, 0, NULL, 0}
    };
    while ((option = getopt_long(argc, argv, "ho:vstmxpiO:", long_options, NULL)) != -1) {

        switch (option) {

            case 'h':
                print_help();
                free(options->output);
                return 0;

            case 'o':
                free(options->output);
                options->output = strdup(optarg);

                if (options->output == NULL) {
                    report_oom();
                    return 1;
                }

                break;

            case 'O':
                if (strlen(optarg) != 1 || optarg[0] < '0' || optarg[0] > '2')
                    goto usage_error;
                options->explicit_level |= 1 << (optarg[0] - '0');
                break;
            case OPT_PASSES: {
                options->opt_seen = 1;
                const char *part = optarg;
                do {
                    const char *comma = strchr(part, ',');
                    size_t length = comma ? (size_t)(comma - part) : strlen(part);
                    int id = tac_pass_find(part, length);
                    if (id < 0)
                        goto usage_error;
                    options->passes |= 1u << id;
                    part = comma ? comma + 1 : NULL;
                } while (part);
                break;
            }
            case OPT_UNROLL: {
                char *end;
                unsigned long factor = strtoul(optarg, &end, 10);
                if (!*optarg || *end || factor < 2 || factor > 16)
                    goto usage_error;
                options->unroll_factor = (unsigned)factor;
                break;
            }
            case OPT_METRICS: options->metrics_path = optarg; break;
            case OPT_AST_JSON: options->ast_json = optarg; break;
            case OPT_NO_SCHEDULE: options->no_schedule = 1; break;
            case OPT_TIME: options->show_time = 1; break;
            case OPT_CHECK: options->check_only = options->check_text = 1; break;
            case OPT_CHECK_JSON: options->check_only = options->check_json = 1; break;
            case OPT_TOKENS_JSON: options->check_only = options->tokens_json = 1; break;
            case OPT_SUGGEST_JSON: {
                char *end = NULL;
                errno = 0;
                unsigned long long value = strtoull(optarg, &end, 10);
                if (errno || !*optarg || *end || strspn(optarg, "0123456789") != strlen(optarg) || value > SIZE_MAX) {
                    fprintf(stderr, "ccom: cursor UTF-8 invalido\n");
                    free(options->output); return EXIT_USAGE;
                }
                options->check_only = options->suggest_json = 1;
                options->suggest_cursor = (size_t) value;
                break;
            }
            case OPT_REPAIR_JSON: options->check_only = options->repair_json = 1; break;
            case OPT_SYNTAX_JSON: options->check_only = options->syntax_json = 1; break;
            case OPT_CLONES_JSON: {
                char *end = NULL; errno = 0;
                unsigned long value = strtoul(optarg, &end, 10);
                if (errno || !*optarg || *end || strspn(optarg, "0123456789") != strlen(optarg) || value < 5 || value > 100000) {
                    fprintf(stderr, "ccom: minimo de clones invalido (5..100000)\n");
                    free(options->output); return EXIT_USAGE;
                }
                options->check_only = options->clones_json = 1;
                options->clone_minimum = (unsigned) value;
                break;
            }
            case 'v': options->verbose = 1;       break;
            case 's': options->emit_asm = 1;      break;
            case 'i': options->emit_tac = 1;      break;
            case 't': options->print_ast = 1;     break;
            case 'm': options->print_memory = 1;  break;
            case 'x': options->assemble_only = 1; break;
            case 'p': options->print_bundles = 1; break;

            default:
                free(options->output);
                fprintf(stderr, "%s\n", USAGE);
                return EXIT_USAGE;
        }
    }

    if (options->explicit_level == 4 ||
        (options->explicit_level && (options->explicit_level & (options->explicit_level - 1))) ||
        (options->opt_seen && options->explicit_level == 1))
        goto usage_error;
    options->level = options->explicit_level == 2 || options->opt_seen;
    if ((options->unroll_factor && !(options->passes & 1u)) ||
        (options->metrics_path && !options->level) ||
        (options->assemble_only && (options->level || options->unroll_factor || options->metrics_path)))
        goto usage_error;

    // Exactamente un archivo de entrada.
    if (argc - optind != 1) {
        free(options->output);
        fprintf(stderr, "%s\n", USAGE);
        return EXIT_USAGE;
    }

    options->input = argv[optind];

    int services = options->check_json + options->tokens_json + options->suggest_json + options->repair_json + options->syntax_json + options->clones_json;
    if (services + options->check_text > 1) {
        fprintf(stderr, "ccom: las operaciones de consulta son excluyentes\n");
        free(options->output);
        return EXIT_USAGE;
    }
    if (options->check_only) {
        if (options->output || options->emit_asm || options->ast_json ||
            options->no_schedule || options->print_ast || options->print_memory ||
            options->assemble_only || options->print_bundles || options->emit_tac ||
            options->explicit_level || options->opt_seen || options->unroll_factor ||
            options->metrics_path) {
            fprintf(stderr, "ccom: %s solo admite -v y --time; no admite opciones de salida o ensamblado\n",
                    options->clones_json ? "--clones-json" : options->repair_json ? "--repair-json" : options->syntax_json ? "--syntax-json" : options->suggest_json ? "--suggest-json" : options->tokens_json ? "--tokens-json" : "--check");
            free(options->output);
            return EXIT_USAGE;
        }
        return -1;
    }

    if (options->output == NULL) {
        options->output = default_output(options->input);

        if (options->output == NULL) {
            report_oom();
            return 1;
        }
    }

    return -1;
usage_error:
    free(options->output);
    fprintf(stderr, "%s\n", USAGE);
    return EXIT_USAGE;
}

/*
 * report_success — confirma una operación completada, sin alterar stdout.
 *
 * Solo se llama con código 0 después de cerrar las salidas. El mensaje no
 * sustituye los códigos de retorno; --time conserva su posición al final.
 */
static void report_success(const Options *options) {
    if (options->check_only) {
        fprintf(stderr, "ccom: Análisis correcto: sin errores.\n");
        return;
    }

    int ast_only = !options->assemble_only && options->ast_json &&
                   !options->emit_asm && !options->print_memory &&
                   !options->print_bundles && !options->emit_tac && !options->level;

    fprintf(stderr, "ccom: %s\n",
            ast_only ? "AST exportado correctamente." :
            options->assemble_only ? "Ensamblado correcto." : "Compilación correcta.");
    fprintf(stderr, "ccom: Archivos generados:\n");

    if (!ast_only) {
        const char *extensions[] = { ".bin", ".hex", ".mem", ".asm", ".tac" };
        int count = options->assemble_only ? 3 : 5;
        for (int i = 0; i < count; i++) {
            if ((i == 3 && !options->emit_asm) || (i == 4 && !options->emit_tac))
                continue;
            char *path = with_extension(options->output, extensions[i]);
            if (path) {
                fprintf(stderr, "  %s\n", path);
                free(path);
            } else {
                fprintf(stderr, "  (ruta no disponible para %s)\n", extensions[i]);
            }
        }
    }
    if (options->ast_json && !options->assemble_only)
        fprintf(stderr, "  %s\n", options->ast_json);
    if (options->metrics_path && !options->assemble_only)
        fprintf(stderr, "  %s\n", options->metrics_path);
}

/*
 * main — interpreta banderas y ejecuta compilación o ensamblado.
 *
 * Entrada:
 *   argc  cantidad de argumentos.
 *   argv  arreglo de argumentos del proceso.
 * Salida:
 *   0 si completa; 1 si falla; 2 por uso incorrecto.
 *   Efectos: getopt_long puede reordenar argv; lee/escribe archivos y mensajes.
 * Restricciones:
 *   exactamente una entrada salvo ayuda; -s se ignora con -x.
 */
int main(int argc, char **argv) {
    Options options = { 0 };
    int status = parse_options(argc, argv, &options);

    if (status >= 0)
        return status;

    struct timespec start;
    int timed = options.show_time;
    if (timed && clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        fprintf(stderr, "ccom: aviso: no se pudo medir la duracion\n");
        timed = 0;
    }

    int code;
    if (options.check_json) diag_json_begin();
    if (options.tokens_json) diag_json_lexical_begin();

    if (options.clones_json) {
        code = clones_run(stdout, options.input, options.clone_minimum);
        report_phase(&options, "clones lexicos", code == 0);
    }
    else if (options.repair_json || options.syntax_json) {
        code = options.repair_json ? repairs_run(stdout, options.input) : syntax_json_run(stdout, options.input);
        report_phase(&options, options.repair_json ? "reparaciones sintacticas" : "verificacion sintactica", code == 0);
    }
    else if (options.suggest_json) {
        code = suggestions_run(stdout, options.input, options.suggest_cursor);
        report_phase(&options, "sugerencias sintacticas", code == 0);
    }
    else if (options.tokens_json) {
        extern int lexer_scan_only(void);
        yyin = fopen(options.input, "r");
        if (!yyin) { perror(options.input); code = 1; }
        else {
            struct stat source_stat;
            if (fstat(fileno(yyin), &source_stat) != 0 || source_stat.st_size > 8000000) {
                fprintf(stderr, "ccom: entrada lexica no disponible o superior a 8 MB\n");
                code = 1;
            } else {
                diag_init(options.input);
                code = lexer_scan_only();
            }
            fclose(yyin);
        }
        report_phase(&options, "fase 1: analisis lexico", code == 0);
    }
    else if (options.assemble_only) {

        // La entrada ya es el .asm: -s lo sobrescribiría.
        if (options.emit_asm)
            fprintf(stderr, "ccom: aviso: -s se ignora con -x\n");

        if (options.emit_tac)
            fprintf(stderr, "ccom: aviso: -i se ignora con -x\n");

        code = assemble_file(&options);
    }
    else {
        code = compile(&options);
    }

    if (options.check_json || options.tokens_json)
        code = diag_json_finish(stdout, options.input, code);

    if (options.tokens_json) diag_free();
    if (code == 0 && !options.suggest_json && !options.repair_json && !options.syntax_json && !options.clones_json) {
        if (options.tokens_json) fprintf(stderr, "ccom: Analisis lexico correcto.\n");
        else report_success(&options);
    }

    if (timed)
        report_elapsed(&options, &start, code);

    free(options.output);

    return code;
}
