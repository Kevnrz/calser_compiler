#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "semantic.h"
#include "diagnostics.h"
#include "label_table.h"
#include "codegen.h"
#include "generate.h"

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
 */
typedef struct {
    const char *input;         /* archivo fuente (.cls) o, con -x, .asm      */
    char       *output;        /* -o; por defecto: fuente sin extensión .bin */
    int verbose;               /* -v */
    int emit_asm;              /* -s */
    int print_ast;             /* -t */
    int print_memory;          /* -m */
    int assemble_only;         /* -x */
    int print_bundles;         /* -p */
} Options;

#define EXIT_USAGE 2

#define USAGE "uso: ccom [-v] [-t] [-m] [-s] [-p] [-x] <archivo> [-o <salida>]"

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

static char *asm_path_for(const char *output) {
    const char *slash = strrchr(output, '/');
    const char *base = slash ? slash + 1 : output;
    const char *dot = strrchr(base, '.');

    size_t length = (dot != NULL && dot > base)
                        ? (size_t) (dot - output)
                        : strlen(output);

    char *path = malloc(length + sizeof(".asm"));

    if (path == NULL)
        return NULL;

    memcpy(path, output, length);
    strcpy(path + length, ".asm");

    return path;
}

// -v: una línea por fase en stderr.
static void report_phase(const Options *options, const char *phase, int ok) {
    if (options->verbose)
        fprintf(stderr, "ccom: %s ... %s\n", phase, ok ? "ok" : "con errores");
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

    int gen_failed = generate_code(root);
    report_phase(options, "fase 4: generacion de codigo", !gen_failed);
    if (gen_failed) {
        label_table_destroy(labels);

        free(labels);

        semantic_free();

        ast_free(root);

        diag_free();
        return 1; 
    }

    if (options->emit_asm) {
        char *asm_path = asm_path_for(options->output);
        FILE *f = fopen(asm_path, "w");
        if (f) { cg_write(f); fclose(f); } else perror("fopen");
    }

    if (options->print_bundles)
        fprintf(stderr,
                "ccom: aviso: -p todavia no esta disponible "
                "(requiere la generacion de codigo)\n");

    label_table_destroy(labels);

    free(labels);

    semantic_free();

    ast_free(root);

    diag_free();

    return 0;
}

int main(int argc, char **argv) {
    Options options = { 0 };
    int option;

    // Sin '+' al inicio: getopt reordena, así que las opciones pueden ir
    // antes o después del archivo (ccom prog.cls -o prog.bin).
    while ((option = getopt(argc, argv, "o:vstmxp")) != -1) {

        switch (option) {

            case 'o':
                free(options.output);
                options.output = strdup(optarg);

                if (options.output == NULL) {
                    fprintf(stderr, "ccom: error: memoria insuficiente\n");
                    return 1;
                }

                break;

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

    if (options.assemble_only) {
        free(options.output);
        fprintf(stderr,
                "ccom: error: -x todavia no esta disponible "
                "(requiere el ensamblado)\n");
        return 1;
    }

    if (options.output == NULL) {
        options.output = default_output(options.input);

        if (options.output == NULL) {
            fprintf(stderr, "ccom: error: memoria insuficiente\n");
            return 1;
        }
    }

    int code = compile(&options);

    free(options.output);

    return code;
}
