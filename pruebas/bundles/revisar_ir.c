#include <stdio.h>
#include "codegen.h"
#include "diagnostics.h"
#include "generate.h"
#include "semantic.h"
#include "label_table.h"
extern FILE *yyin;
extern ASTNode *root;
extern int yyparse(void);
extern int lexical_errors;
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    yyin = fopen(argv[1], "r");
    if (!yyin) return 1;
    diag_init(argv[1]);
    int result = yyparse();
    fclose(yyin);
    if (result || lexical_errors) return 1;
    if (!semantic_analyze(root)) return 1;
    LabelTable labels;
    label_table_init(&labels);
    if (label_table_build(&labels, root) || generate_code(root, &labels)) return 1;
    int operations = 0, fixed = 0, failure = 0;
    for (int i = 0; i < cg_ir_count(); i++) {
        const CGIRRecord *record = cg_ir_record(i);
        if (record->kind == CG_IR_OPERATION) {
            operations++;
            if (!record->data.instruction.known) {
                fprintf(stderr, "sin descripcion: %s\n", record->data.instruction.text);
                failure = 1;
            }
        } else if (record->kind == CG_IR_FIXED_BUNDLE) {
            fixed++;
            for (int s = 0; s < 4; s++) if (!record->data.slots[s].known) {
                fprintf(stderr, "sin descripcion en plantilla: %s\n", record->data.slots[s].text);
                failure = 1;
            }
        }
    }
    printf("%s: %d operaciones, %d bundles fijos, %d registros IR\n", argv[1], operations, fixed, cg_ir_count());
    cg_free();
    label_table_destroy(&labels);
    semantic_free();
    ast_free(root);
    diag_free();
    return failure;
}
