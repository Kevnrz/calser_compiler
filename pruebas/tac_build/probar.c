/*
 * probar.c — construcción en memoria desde fuentes validados por el semántico.
 * Usa: parser, semántico, generador actual y tac_build; lo usa check-tac-build.
 * Referencias: Aho §6.4, §6.6, §8.4.1; no interpreta ni ejecuta TAC (C2).
 */
#include "tac_build.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "generate.h"
#include "codegen.h"
#include "data_area.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern FILE *yyin;
extern ASTNode *root;
extern int yyparse(void);
extern int lexical_errors;

/* Guarda AST y símbolos para detectar modificaciones de los datos prestados. */
static FILE *snapshot(const char *source) {
    FILE *out = tmpfile();
    assert(out);
    assert(!ast_write_json(out, root, source));
    semantic_fprint_symbols(out, "");
    assert(!ferror(out));
    rewind(out);
    return out;
}

/* Compara bytes completos, incluyendo el fin de ambos flujos. */
static void same_files(FILE *first, FILE *second) {
    int left;
    int right;
    do {
        left = fgetc(first);
        right = fgetc(second);
        assert(left == right);
    } while (left != EOF);
    assert(!ferror(first) && !ferror(second));
    assert(!fclose(first));
    assert(!fclose(second));
}

/*
 * main — valida construcción y comprueba que no altera AST, ámbitos ni emisor.
 * Entrada: ruta de un .cls válido. Salida: 0 y listado completo, 1/2 ante fallo.
 * Efectos: construye y libera todas las fases; no escribe salidas del compilador.
 * Restricciones: AST y símbolos sobreviven a TacBuildResult; aserciones activas.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    yyin = fopen(argv[1], "r");
    assert(yyin);
    diag_init(argv[1]);
    int parsed = yyparse();
    assert(!fclose(yyin));
    assert(!parsed && !lexical_errors);
    assert(semantic_analyze(root));
    LabelTable labels;
    label_table_init(&labels);
    assert(!label_table_build(&labels, root));
    assert(!generate_code(root, &labels, argv[1]));
    int lines = cg_line_count();
    int records = cg_ir_count();
    int area = data_area_end();
    int size = cg_data_size();
    uint32_t *image = malloc((size_t)size + 1);
    assert(image);
    memcpy(image, cg_data(), (size_t)size);
    FILE *before = snapshot(argv[1]);
    char error[256];
    assert(!tac_build(NULL, scopes_global(), error, sizeof error));
    assert(error[0]);
    assert(!tac_build(root, NULL, NULL, 0));
    TacBuildResult *result = tac_build(root, scopes_global(), error, sizeof error);
    if (!result) {
        fprintf(stderr, "%s\n", error);
    }
    assert(result && !error[0]);
    assert(tac_validate(result->program, error, sizeof error));
    assert(result->program->global_scope == scopes_global());
    assert(result->initial_data_size == (size_t)size);
    assert(!memcmp(result->initial_data, image, (size_t)size));
    assert(cg_line_count() == lines && cg_ir_count() == records);
    assert(data_area_end() == area);
    assert(!memcmp(cg_data(), image, (size_t)size));
    same_files(before, snapshot(argv[1]));
    tac_build_print(stdout, result);
    tac_build_free(result);
    tac_build_free(NULL);
    free(image);
    cg_free();
    label_table_destroy(&labels);
    semantic_free();
    ast_free(root);
    diag_free();
    return ferror(stdout) ? 1 : 0;
}
