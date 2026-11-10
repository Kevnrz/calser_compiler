/*
 * ejecutar.c — auxiliar de pruebas: fuente -> AST validado -> TAC en memoria.
 * Usa: interpretar.h, parser y semántico. Lo usa probar_valores.py, no ccom.
 * Referencias: Aho §6.4, §6.6 y §7.2. No lee el .tac ni genera ensamblador.
 */
#include "interpretar.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
extern FILE *yyin;
extern ASTNode *root;
extern int yyparse(void);
extern int lexical_errors;

/*
 * json_string — escapa cadenas sin depender del texto del listado TAC.
 * Entrada: cadena no NULL. Salida: ninguna.
 * Efectos: escribe una cadena JSON en stdout.
 * Restricciones: el llamador comprueba errores del flujo al terminar.
 */
static void json_string(const char *text) {
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
    json_string(tac_run_status_name(result->status));
    printf(",\"steps\":%" PRIu64 ",\"diagnostic\":", result->steps);
    json_string(result->diagnostic);
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
                json_string(symbol->name);
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
    if (argc < 2 || argc > 3) {
        return 2;
    }
    uint64_t limit = UINT64_C(1000000);
    if (argc == 3) {
        char *end;
        errno = 0;
        unsigned long long parsed = strtoull(argv[2], &end, 10);
        if (argv[2][0] == '-' || end == argv[2] || *end || errno || parsed > UINT64_MAX) {
            return 2;
        }
        limit = (uint64_t)parsed;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror(argv[1]);
        return 1;
    }
    diag_init(argv[1]);
    int parsed = yyparse();
    fclose(yyin);
    if (parsed || lexical_errors) {
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
