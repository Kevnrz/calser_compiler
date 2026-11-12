/* Auxiliar D: camino completo sin banderas nuevas; Aho §8.1 y §8.6.
 * Imprime tramos observables y bundles; reutilizable por el comparador de E.
 */
#include "tac_emit.h"
#include "tac_build.h"
#include "tac_lower.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "assembler.h"
#include "codegen.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern FILE *yyin;
extern ASTNode *root;
extern int yyparse(void);
extern int lexical_errors;

/* Entrada: salidas solicitadas. Salida: 1 escritas. Efectos: ensambla y escribe.
 * Restricciones: codegen finalizado; prueba real de ensamblado, sin repetir encoder.
 */
static int write_outputs(const char *source, const char *base) {
    int count = cg_line_count();
    const char **lines = calloc((size_t)count, sizeof *lines);
    assert(lines);
    for (int k = 0; k < count; k++)
        lines[k] = cg_line(k);
    AsmProgram program;
    int ok = !asm_assemble(lines, count, source, &program);
    free(lines);
    if (!ok)
        return 0;
    char path[4096];
    snprintf(path, sizeof path, "%s.asm", base);
    FILE *out = fopen(path, "w");
    assert(out);
    cg_write(out);
    assert(!ferror(out) && !fclose(out));
    snprintf(path, sizeof path, "%s.mem", base);
    out = fopen(path, "w");
    assert(out);
    fputs("@0\n", out);
    for (int k = 0; k < cg_data_size() / 4; k++)
        fprintf(out, "%08x\n", (unsigned)cg_data()[k]);
    assert(!ferror(out) && !fclose(out));
    printf("bundles %d\n", program.count);
    asm_free(&program);
    return 1;
}

/* Entrada: fuente y base de salida. Salida: 0 éxito, 3 rechazo frontend, 1 error D.
 * Efectos: frontend -> construcción -> bajada -> asignación -> emisión/organización
 * -> ensamblado. Restricciones: planes antes de IR, IR antes de build y semántico.
 */
int main(int argc, char **argv) {
    if (argc != 3)
        return 2;
    yyin = fopen(argv[1], "r");
    assert(yyin);
    diag_init(argv[1]);
    int parsed = yyparse();
    assert(!fclose(yyin));
    int status = 3;
    LabelTable labels;
    label_table_init(&labels);
    if (!parsed && !lexical_errors && semantic_analyze(root)) {
        char error[512];
        assert(!label_table_build(&labels, root));
        TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
        TacProgram *isa = build ? tac_lower(build->program, error, sizeof error) : NULL;
        TacAllocationResult *allocation = isa ?
            tac_allocate_registers(isa, argv[1], error, sizeof error) : NULL;
        TacEmitInput input = {.program = isa, .allocation = allocation,
            .initial_data = build ? build->initial_data : NULL,
            .initial_data_size = build ? build->initial_data_size : 0,
            .labels = &labels, .source_name = argv[1]};
        status = allocation && tac_emit_program(&input, error, sizeof error) &&
                 write_outputs(argv[1], argv[2]) ? 0 : 1;
        if (status)
            fprintf(stderr, "%s\n", error);
        for (Scope *s = scopes_first(); s; s = s->next) {
            if (s->kind != SCOPE_GLOBAL && (!s->owner || strcmp(s->owner->name, "main")))
                continue;
            for (Symbol *v = s->symbols; v; v = v->next)
                if (v->kind == SYMBOL_VARIABLE)
                    printf("observable %d %d\n", v->address, v->width);
        }
        tac_allocation_free(allocation);
        tac_program_free(isa);
        tac_build_free(build);
    }
    cg_free();
    label_table_destroy(&labels);
    semantic_free();
    ast_free(root);
    diag_free();
    return status;
}
