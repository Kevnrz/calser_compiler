#include <stdio.h>

#include "semantic.h"
#include "diagnostics.h"

extern int yyparse(void);
extern FILE *yyin;

extern ASTNode *root;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr,
                "Usage: %s <source-file>\n",
                argv[0]);

        return 1;
    }

    yyin = fopen(argv[1], "r");

    if (!yyin) {
        perror("fopen");
        return 1;
    }

    // §8.2: los errores muestran el archivo y la línea del programa.
    diag_init(argv[1]);

    int result = yyparse();

    fclose(yyin);

    if (result == 0) {
        printf("Parsing successful!\n\n");

        ast_print(root, 0);

        // S-03: si el análisis falla no hay tabla y el código de salida es 1.
        int errors_before = diag_count();

        if (!semantic_analyze(root)) {
            fprintf(stderr,
                    "Analisis semantico fallido: %d error(es).\n",
                    diag_count() - errors_before);

            semantic_free();

            ast_free(root);

            diag_free();

            return 1;
        }

        semantic_print_symbols();

        semantic_free();

        ast_free(root);

        diag_free();

        return 0;
    }

    printf("Parsing failed.\n");

    diag_free();

    return 1;
}