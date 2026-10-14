/*
 * main.c — Fases 1–5: coordinación del compilador y sus salidas.
 *
 * Interpreta banderas y encadena análisis, tablas, generación y ensamblado.
 * Permite ensamblar directamente con -x y exportar el AST como JSON.
 *
 * Usa: lexer/parser, semantic, diagnostics, label_table, generate, codegen y assembler.
 * Lo usa: línea de comandos ccom y scripts de pruebas/herramientas.
 * Referencias: enunciado §2.3–§2.5, §5.1; fase4_contrato_comun.md §5.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "semantic.h"
#include "diagnostics.h"
#include "label_table.h"
#include "codegen.h"
#include "generate.h"
#include "assembler.h"

extern int yyparse(void);
extern FILE *yyin;

extern ASTNode *root;

// Cantidad de errores léxicos (lexer.l).
extern int lexical_errors;

/* Opciones de línea de comandos (enunciado CE 1108 §5.1):
 *
 *   ccom [opciones] <archivo> [-o <salida>]
 *
 *   -t  imprime el AST
 *   -m  imprime la tabla de símbolos con direcciones y la de etiquetas
 *   -v  imprime en stderr el avance de cada fase
 *   -o  nombre del binario de salida
 *   -s  genera además el .asm
 *   -p  muestra los bundles en ensamblador y en binario
 *   -x  la entrada es un .asm y se genera el binario, sin compilar Calser
 *
 * Sin errores siempre se escriben .bin, .hex y .mem con el nombre base de
 * -o (fase4_contrato_comun.md §5).
 */
typedef struct {
    const char *input;         /* archivo fuente (.cls) o, con -x, .asm      */
    char       *output;        /* -o; por defecto: fuente sin extensión .bin */
    int verbose;               /* -v */
    int emit_asm;              /* -s */
    const char *ast_json;       /* --ast-json archivo */
    int no_schedule;           /* --no-schedule: comparación conservadora */
    int print_ast;             /* -t */
    int print_memory;          /* -m */
    int assemble_only;         /* -x */
    int print_bundles;         /* -p */
} Options;

#define EXIT_USAGE 2

#define USAGE "uso: ccom [-v] [-t] [-m] [-s] [-p] [-x] [--ast-json <json>] [--no-schedule] <archivo> [-o <salida>]"

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
        fprintf(stderr, "ccom: error: memoria insuficiente\n");
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
        fprintf(stderr, "ccom: error: memoria insuficiente\n");
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
 * compile — coordina las fases de compilación y las salidas.
 *
 * Entrada:
 *   options  opciones con rutas y banderas.
 * Salida:
 *   0 si completa; 1 si falla alguna fase o salida.
 *   Efectos: lee fuente, crea/libera AST/tablas/IR; escribe archivos y diagnósticos.
 * Restricciones:
 *   --ast-json solo puede terminar antes del semántico; símbolos deben
 *   vivir hasta terminar generación.
 */
static int compile(const Options *options) {
    yyin = fopen(options->input, "r");

    if (!yyin) {
        perror("fopen");
        return 1;
    }

    // §8.2: los errores muestran el archivo y la línea del programa.
    diag_init(options->input);

    int result = yyparse();

    fclose(yyin);

    // Con errores léxicos no hay AST, tabla ni análisis semántico: la
    // recuperación (descartar, truncar, sustituir por 0) cambia el programa
    // y el semántico podría reportar errores que no existen.
    if (lexical_errors > 0) {

        if (result == 0)
            ast_free(root);

        fprintf(stderr,
                "Analisis lexico fallido: %d error(es).\n",
                lexical_errors);

        report_phase(options, "fase 1-2: analisis lexico y sintactico", 0);

        diag_free();

        return 1;
    }

    if (result != 0) {
        report_phase(options, "fase 1-2: analisis lexico y sintactico", 0);

        diag_free();

        return 1;
    }

    report_phase(options, "fase 1-2: analisis lexico y sintactico", 1);

    if (options->print_ast)
        ast_print(root, 0);

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
            failed = ast_write_json(out, root, options->input);
            if (fclose(out) != 0) failed = 1;
            if (failed) fprintf(stderr, "ccom: error al escribir AST JSON\n");
        }
        /* Exportación sola: inspeccionar incluso construcciones sin codegen. */
        if (failed || (!options->emit_asm && !options->print_memory && !options->print_bundles)) {
            ast_free(root);
            diag_free();
            return failed ? 1 : 0;
        }
    }

    // S-03: si el análisis falla no hay tabla y el código de salida es 1.
    int errors_before = diag_count();

    if (!semantic_analyze(root)) {
        fprintf(stderr,
                "Analisis semantico fallido: %d error(es).\n",
                diag_count() - errors_before);

        report_phase(options, "fase 3: analisis semantico", 0);

        semantic_free();

        ast_free(root);

        diag_free();

        return 1;
    }

    report_phase(options, "fase 3: analisis semantico", 1);

    if (options->print_memory)
        semantic_print_symbols();

    LabelTable *labels = malloc(sizeof(LabelTable));

    if (labels == NULL) {
        report_phase(options, "fase 3: tabla de etiquetas", 0);

        semantic_free();

        ast_free(root);

        diag_free();

        return 1;
    }

    label_table_init(labels);

    if (label_table_build(labels, root) != 0) {
        report_phase(options, "fase 3: tabla de etiquetas", 0);

        label_table_destroy(labels);

        free(labels);

        semantic_free();

        ast_free(root);

        diag_free();

        return 1;
    }

    report_phase(options, "fase 3: tabla de etiquetas", 1);

    // La tabla de etiquetas de -m se imprime al final, con las direcciones
    // (enunciado CE1108 §2.3): el nombre y el nodo salen de la Fase 3, pero
    // la dirección de cada etiqueta solo se conoce después de generar el
    // código (§2.4) y ensamblarlo (§2.5).
    int gen_failed = generate_code(root, labels);
    report_phase(options, "fase 4: generacion de codigo", !gen_failed);
    if (!gen_failed && !options->no_schedule) {
        gen_failed = cg_schedule();
        report_phase(options, "fase 4: organizacion de bundles", !gen_failed);
    }
    if (gen_failed) {
        // Sin código generado no hay direcciones: se imprime con "-".
        if (options->print_memory)
            label_table_print(labels);

        label_table_destroy(labels);

        free(labels);

        semantic_free();
        cg_free();

        ast_free(root);

        diag_free();
        return 1; 
    }

    // El calendario ya es definitivo. Resolver con sus índices finales,
    // antes del listado y del ensamblado, nunca con las direcciones del IR
    // conservador. La primera pasada del ensamblador cuenta esos mismos
    // bundles (libro §6.7, backpatching; enunciado §2.5). Una etiqueta que no
    // llega al .asm (un & constante en un inicializador global) queda con -1.
    label_table_resolve(labels, cg_label_address);

    // Primera línea del .asm: un comentario con el fuente. El ensamblado
    // recibe la misma línea, así los errores de <generado> citan la línea
    // del .asm que escribe -s.
    size_t header_size = strlen(options->input) + sizeof("# ccom: ");
    char *header = malloc(header_size);

    if (header == NULL) {
        fprintf(stderr, "ccom: error: memoria insuficiente\n");

        if (options->print_memory)
            label_table_print(labels);

        label_table_destroy(labels);
        free(labels);
        semantic_free();
        cg_free();
        ast_free(root);
        diag_free();
        return 1;
    }

    snprintf(header, header_size, "# ccom: %s", options->input);

    // -s: además el .asm (isa.md §11), anotado (enunciado CE1108 §6.2): la
    // dirección y la línea del fuente de cada bundle, la tabla de etiquetas
    // con sus direcciones y la de variables con sus direcciones de datos.
    // Todo lo agregado va en comentarios con # (isa.md §11.1): ccom -x da
    // los mismos bundles.
    int output_failed = 0;

    if (options->emit_asm) {
        char *path;
        FILE *out = open_output(options, ".asm", "w", &path);

        if (out != NULL) {
            fprintf(out, "%s\n", header);
            cg_write_listing(out);
            fprintf(out, "\n# Variables (direccion de datos)\n");
            semantic_fprint_symbols(out, "#   ");
        }

        output_failed = close_output(out, path);
    }

    // El ensamblado toma las líneas del generador (contrato §1), después de
    // la cabecera.
    int line_count = cg_line_count() + 1;
    const char **lines = malloc((size_t) line_count * sizeof(char *));

    if (lines == NULL) {
        fprintf(stderr, "ccom: error: memoria insuficiente\n");
        output_failed = 1;
    }
    else {

        lines[0] = header;

        for (int i = 1; i < line_count; i++)
            lines[i] = cg_line(i - 1);

        if (assemble_and_write(options, lines, line_count, "<generado>",
                               cg_data(), cg_data_size()) != 0)
            output_failed = 1;

        free(lines);
    }

    free(header);

    // Tabla de etiquetas con direcciones, después del ensamblado.
    if (options->print_memory)
        label_table_print(labels);

    label_table_destroy(labels);

    free(labels);

    semantic_free();

    cg_free();

    ast_free(root);

    diag_free();

    return output_failed ? 1 : 0;
}

/* ==========================================================================
 * BANDERAS Y PUNTO DE ENTRADA
 * ======================================================================= */


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
 *   exactamente una entrada; -s se ignora con -x.
 */
int main(int argc, char **argv) {
    Options options = { 0 };
    int option;

    // Sin '+' al inicio: getopt reordena, así que las opciones pueden ir
    // antes o después del archivo (ccom prog.cls -o prog.bin).
    static const struct option long_options[] = {
        {"ast-json", required_argument, NULL, 1000},
        {"no-schedule", no_argument, NULL, 1001},
        {NULL, 0, NULL, 0}
    };
    while ((option = getopt_long(argc, argv, "o:vstmxp", long_options, NULL)) != -1) {

        switch (option) {

            case 'o':
                free(options.output);
                options.output = strdup(optarg);

                if (options.output == NULL) {
                    fprintf(stderr, "ccom: error: memoria insuficiente\n");
                    return 1;
                }

                break;

            case 1000: options.ast_json = optarg; break;
            case 1001: options.no_schedule = 1; break;
            case 'v': options.verbose = 1;       break;
            case 's': options.emit_asm = 1;      break;
            case 't': options.print_ast = 1;     break;
            case 'm': options.print_memory = 1;  break;
            case 'x': options.assemble_only = 1; break;
            case 'p': options.print_bundles = 1; break;

            default:
                free(options.output);
                fprintf(stderr, "%s\n", USAGE);
                return EXIT_USAGE;
        }
    }

    // Exactamente un archivo de entrada.
    if (argc - optind != 1) {
        free(options.output);
        fprintf(stderr, "%s\n", USAGE);
        return EXIT_USAGE;
    }

    options.input = argv[optind];

    if (options.output == NULL) {
        options.output = default_output(options.input);

        if (options.output == NULL) {
            fprintf(stderr, "ccom: error: memoria insuficiente\n");
            return 1;
        }
    }

    int code;

    if (options.assemble_only) {

        // La entrada ya es el .asm: -s lo sobrescribiría.
        if (options.emit_asm)
            fprintf(stderr, "ccom: aviso: -s se ignora con -x\n");

        code = assemble_file(&options);
    }
    else {
        code = compile(&options);
    }

    free(options.output);

    return code;
}
