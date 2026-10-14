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
    int print_ast;             /* -t */
    int print_memory;          /* -m */
    int assemble_only;         /* -x */
    int print_bundles;         /* -p */
} Options;

#define EXIT_USAGE 2

#define USAGE "uso: ccom [-v] [-t] [-m] [-s] [-p] [-x] [--ast-json <json>] <archivo> [-o <salida>]"

// Nombre por defecto del binario: el fuente sin extensión + ".bin". La
// extensión es lo que sigue al último '.' del nombre del archivo (no de los
// directorios). Devuelve memoria dinámica.
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

// -v: una línea por fase en stderr.
static void report_phase(const Options *options, const char *phase, int ok) {
    if (options->verbose)
        fprintf(stderr, "ccom: %s ... %s\n", phase, ok ? "ok" : "con errores");
}

// Abre <nombre base de -o><extension> para escribir; reporta si no puede.
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

// Cierra un archivo de salida y libera su nombre. Devuelve 1 si falló.
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

// Palabra de 32 bits little endian.
static void write_word(FILE *out, uint32_t word) {
    unsigned char bytes[4] = {
        (unsigned char) (word & 0xFF),
        (unsigned char) ((word >> 8) & 0xFF),
        (unsigned char) ((word >> 16) & 0xFF),
        (unsigned char) ((word >> 24) & 0xFF)
    };

    fwrite(bytes, 1, sizeof(bytes), out);
}

/* Archivos de salida (fase4_contrato_comun.md §5). data son las palabras de
 * la imagen de datos desde la dirección 0 y data_size sus bytes.
 * Devuelve 1 si alguno no se pudo escribir. */
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

/* Ensambla las líneas (isa.md §11), imprime los bundles con -p y escribe
 * los archivos de salida. name es el nombre para los errores de ensamblado.
 * Devuelve el código de salida. */
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

/* -x: ensambla options->input (un .asm) sin compilar Calser. No hay imagen
 * de datos: el tamaño es 0 y el .mem queda solo con @0.
 * Devuelve el código de salida. */
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

// Compila options->input. Devuelve el código de salida.
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

    if (options->print_memory)
        label_table_print(labels);

    int gen_failed = generate_code(root, labels);
    report_phase(options, "fase 4: generacion de codigo", !gen_failed);
    if (gen_failed) {
        label_table_destroy(labels);

        free(labels);

        semantic_free();

        ast_free(root);

        diag_free();
        return 1; 
    }

    // Primera línea del .asm: un comentario con el fuente. El ensamblado
    // recibe la misma línea, así los errores de <generado> citan la línea
    // del .asm que escribe -s.
    size_t header_size = strlen(options->input) + sizeof("# ccom: ");
    char *header = malloc(header_size);

    if (header == NULL) {
        fprintf(stderr, "ccom: error: memoria insuficiente\n");
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

    label_table_destroy(labels);

    free(labels);

    semantic_free();

    cg_free();

    ast_free(root);

    diag_free();

    return output_failed ? 1 : 0;
}

int main(int argc, char **argv) {
    Options options = { 0 };
    int option;

    // Sin '+' al inicio: getopt reordena, así que las opciones pueden ir
    // antes o después del archivo (ccom prog.cls -o prog.bin).
    static const struct option long_options[] = {
        {"ast-json", required_argument, NULL, 1000},
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
