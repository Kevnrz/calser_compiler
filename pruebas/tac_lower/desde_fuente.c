/*
 * desde_fuente.c — fuente -> HIGH -> ISA virtual, sin generador ni emisión.
 * Usa: parser, semántico, tac_build y tac_lower. Lo usa check-tac-lower.
 * Referencias: Aho §8.1–8.2 y §8.6; la prueba conserva HIGH byte a byte.
 */
#include "tac_lower.h"
#include "tac_build.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include <assert.h>
#include <string.h>
extern FILE *yyin;
extern ASTNode *root;
extern int yyparse(void);
extern int lexical_errors;

/*
 * write_bytes — agrega memoria de objetos vivos a una captura privada.
 * Entrada:
 *   flujo, objeto y tamaño legibles.
 * Salida:
 *   ninguna.
 * Efectos:
 *   escribe el flujo; aborta ante fallo.
 * Restricciones:
 *   la representación solo se compara en el mismo proceso.
 */
static void write_bytes(FILE *out, const void *value, size_t size) {
    if (size) {
        assert(fwrite(value, 1, size, out) == size);
    }
}

/*
 * snapshot — captura imagen y objetos HIGH, incluidos IDs, posiciones y aristas.
 * Entrada:
 *   resultado HIGH válido.
 * Salida:
 *   flujo temporal propio, rebobinado.
 * Efectos:
 *   escribe una captura sin modificar objetos ni punteros.
 * Restricciones:
 *   no es serialización persistente; Aho §8.4.1.
 */
static FILE *snapshot(const TacBuildResult *build) {
    FILE *out = tmpfile();
    assert(out);
    write_bytes(out, build, sizeof *build);
    write_bytes(out, build->initial_data, build->initial_data_size);
    write_bytes(out, build->program, sizeof *build->program);
    for (const TacFunction *function = build->program->first; function; function = function->next) {
        write_bytes(out, function, sizeof *function);
        write_bytes(out, function->symbol, sizeof *function->symbol);
        for (const TacBlock *block = function->first; block; block = block->next) {
            write_bytes(out, block, sizeof *block);
            write_bytes(out, block->successors, block->successor_count * sizeof *block->successors);
            write_bytes(out, block->predecessors,
                        block->predecessor_count * sizeof *block->predecessors);
            for (const TacInstruction *instruction = block->first; instruction;
                 instruction = instruction->next) {
                write_bytes(out, instruction, sizeof *instruction);
            }
        }
    }
    tac_build_print(out, build);
    assert(!ferror(out));
    rewind(out);
    return out;
}

/*
 * unchanged — compara capturas completas y cierra ambos flujos.
 * Entrada:
 *   flujos propios rebobinados.
 * Salida:
 *   ninguna.
 * Efectos:
 *   lee y cierra; aborta si la bajada alteró HIGH o la imagen.
 * Restricciones:
 *   no omite bytes ni normaliza etiquetas.
 */
static void unchanged(FILE *before, FILE *after) {
    int a;
    int b;
    do {
        a = fgetc(before);
        b = fgetc(after);
        assert(a == b);
    } while (a != EOF);
    assert(!ferror(before) && !ferror(after));
    assert(!fclose(before));
    assert(!fclose(after));
}

/*
 * main — valida la bajada de un fuente sin invocar el generador actual.
 * Entrada:
 *   ruta de .cls.
 * Salida:
 *   0 y listado; 3 rechazo del frontend; 1 fallo HIGH/ISA.
 * Efectos:
 *   lee el fuente, crea programas y comprueba HIGH intacto.
 * Restricciones:
 *   libera ISA antes de TacBuildResult y del semántico; Aho §8.6.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    yyin = fopen(argv[1], "r");
    assert(yyin);
    diag_init(argv[1]);
    int parsed = yyparse();
    parser_finish();
    assert(!fclose(yyin));
    int code = 3;
    if (!parsed && !lexical_errors && !parser_syntax_errors() && semantic_analyze(root)) {
        char error[256];
        TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
        assert(build);
        FILE *before = snapshot(build);
        TacProgram *isa = tac_lower(build->program, error, sizeof error);
        unchanged(before, snapshot(build));
        if (!isa) {
            fprintf(stderr, "%s: %s\n", argv[1], error);
            code = 1;
        } else {
            assert(isa->stage == TAC_STAGE_ISA_VIRTUAL);
            assert(isa->global_scope == build->program->global_scope);
            assert(tac_validate(isa, error, sizeof error));
            assert(!tac_validate_for_emission(isa, error, sizeof error));
            tac_print(stdout, isa);
            code = ferror(stdout) ? 1 : 0;
        }
        tac_program_free(isa);
        tac_build_free(build);
    }
    semantic_free();
    ast_free(root);
    diag_free();
    return code;
}
