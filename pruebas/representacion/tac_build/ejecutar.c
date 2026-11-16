/*
 * ejecutar.c — auxiliar de pruebas: fuente -> AST validado -> TAC en memoria.
 * Usa: interpretar.h, parser y semántico. Lo usa probar_valores.py, no ccom.
 * Referencias: Aho §6.4, §6.6 y §7.2. No lee el .tac ni genera ensamblador.
 */
#include "interpretar.h"
#include "tac_passes.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
extern FILE *yyin;
extern ASTNode *root;

/*
 * result_json_string — escapa cadenas sin depender del texto del listado TAC.
 * Entrada: cadena no NULL. Salida: ninguna.
 * Efectos: escribe una cadena JSON en stdout.
 * Restricciones: el llamador comprueba errores del flujo al terminar.
 */
static void result_json_string(const char *text) {
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (*p == '"' || *p == '\\') {
            putchar('\\');
            putchar(*p);
        } else if (*p < 32) {
            printf("\\u%04x", *p);
        } else {
            putchar(*p);
        }
    }
    putchar('"');
}

/*
 * unique_name — aplica la misma exclusión de homónimos que la suite del simulador.
 * Entrada: nombre de símbolo. Salida: 1 si hay una única casilla de datos con él.
 * Efectos: ninguno. Restricciones: tabla semántica vigente; funciones excluidas.
 */
static int unique_name(const char *name) {
    size_t count = 0;
    for (const Scope *scope = scopes_first(); scope; scope = scope->next) {
        for (const Symbol *symbol = scope->symbols; symbol; symbol = symbol->next) {
            if (symbol->kind != SYMBOL_FUNCTION && !strcmp(symbol->name, name)) {
                count++;
            }
        }
    }
    return count == 1;
}

/*
 * print_result — exporta estados y valores de memoria, no instrucciones impresas.
 * Entrada: resultado vivo; símbolos del semántico vivos. Salida: ninguna.
 * Efectos: escribe JSON; cada dato se interpreta con signo de 32 bits.
 * Restricciones: solo publica valores cuando la ejecución terminó normalmente.
 */
static void print_result(const TacRunResult *result) {
    fputs("{\"status\":", stdout);
    result_json_string(tac_run_status_name(result->status));
    printf(",\"steps\":%" PRIu64 ",\"diagnostic\":", result->steps);
    result_json_string(result->diagnostic);
    fputs(",\"values\":{", stdout);
    int comma = 0;
    if (result->status == TAC_RUN_OK) {
        for (const Scope *scope = scopes_first(); scope; scope = scope->next) {
            for (const Symbol *symbol = scope->symbols; symbol; symbol = symbol->next) {
                if (symbol->kind == SYMBOL_FUNCTION || !unique_name(symbol->name)) {
                    continue;
                }
                if (comma) {
                    putchar(',');
                }
                comma = 1;
                result_json_string(symbol->name);
                fputs(":[", stdout);
                for (int offset = 0; offset < symbol->width; offset += 4) {
                    uint32_t word = 0;
                    if (!tac_run_read_word(result, (uint32_t)(symbol->address + offset), &word)) {
                        fputs("lectura de resultado fuera de memoria\n", stderr);
                        exit(EXIT_FAILURE);
                    }
                    int64_t value = word <= INT32_MAX ? (int64_t)word :
                                    (int64_t)word - INT64_C(4294967296);
                    printf("%s%" PRId64, offset ? "," : "", value);
                }
                putchar(']');
            }
        }
    }
    puts("}}");
}

/*
 * main — construye e interpreta sin pasar por generate_code ni por tac_print.
 * Entrada: fuente y límite opcional de pasos. Salida: 0 con JSON, 1 por fase, 2 por uso.
 * Efectos: crea y libera AST, semántico, TAC y memoria interpretada.
 * Restricciones: los errores de ejecución se informan en status, no como retorno normal.
 */
int main(int argc, char **argv) {
    const char *source = NULL;
    unsigned passes = 0, factor = 0;
    uint64_t limit = UINT64_C(1000000);
    int have_limit = 0;
    for (int k = 1; k < argc; k++) {
        if (!strncmp(argv[k], "--opt=", 6)) {
            const char *name = argv[k] + 6;
            do {
                const char *end = strchr(name, ',');
                size_t length = end ? (size_t)(end - name) : strlen(name);
                int index = tac_pass_find(name, length);
                if (index < 0 || tac_pass_at((size_t)index)->point != TAC_PASS_HIGH)
                    return 2;
                passes |= 1u << index;
                name = end ? end + 1 : NULL;
            } while (name);
        } else if (!strncmp(argv[k], "--unroll=", 9)) {
            char *end;
            errno = 0;
            unsigned long value = strtoul(argv[k] + 9, &end, 10);
            if (errno || *end || value < 2 || value > 16)
                return 2;
            factor = (unsigned)value;
        } else if (!source) {
            if (argv[k][0] == '-')
                return 2;
            source = argv[k];
        } else if (!have_limit) {
            char *end;
            errno = 0;
            unsigned long long value = strtoull(argv[k], &end, 10);
            if (argv[k][0] == '-' || end == argv[k] || *end || errno || value > UINT64_MAX)
                return 2;
            limit = (uint64_t)value;
            have_limit = 1;
        } else {
            return 2;
        }
    }
    if (!source || (factor && !(passes & 1u)))
        return 2;
    yyin = fopen(source, "r");
    if (!yyin) {
        perror(source);
        return 1;
    }
    diag_init(source);
    int accepted = parser_accept(yyin, &root);
    fclose(yyin);
    if (!accepted) {
        ast_free(root);
        diag_free();
        return 1;
    }
    if (!semantic_analyze(root)) {
        semantic_free();
        ast_free(root);
        diag_free();
        return 1;
    }
    char error[256];
    TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
    if (!build) {
        fprintf(stderr, "%s\n", error);
        semantic_free();
        ast_free(root);
        diag_free();
        return 1;
    }
    /* Mismo gestor que ccom; la salida sigue siendo el resultado interpretado (§9.11). */
    TacMetrics metrics = {0};
    TacPassContext context = {.program = build->program, .build = build,
        .scopes = scopes_first(), .source_name = source, .enabled = passes,
        .unroll_factor = factor, .metrics = &metrics};
    int ok = tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error);
    tac_metrics_free(&metrics);
    if (!ok) {
        fprintf(stderr, "%s\n", error);
        tac_build_free(build);
        semantic_free();
        ast_free(root);
        diag_free();
        return 1;
    }
    TacRunResult result = tac_interpret(build, limit);
    TacRunResult repeated = tac_interpret(build, limit);
    int same = result.status == repeated.status && result.steps == repeated.steps &&
               result.memory_size == repeated.memory_size &&
               !strcmp(result.diagnostic, repeated.diagnostic);
    if (same && result.memory_size) {
        same = result.memory && repeated.memory &&
               !memcmp(result.memory, repeated.memory, result.memory_size);
    }
    if (same) {
        print_result(&result);
    } else {
        fputs("dos ejecuciones del mismo TAC no coinciden\n", stderr);
    }
    tac_run_free(&repeated);
    tac_run_free(&result);
    tac_build_free(build);
    semantic_free();
    ast_free(root);
    diag_free();
    return !same || ferror(stdout) ? 1 : 0;
}
