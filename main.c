#include <stdio.h>

#include "semantic.h"

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

    int result = yyparse();

    fclose(yyin);

    if (result == 0) {
        printf("Parsing successful!\n\n");

        ast_print(root, 0);

        semantic_analyze(root);

        semantic_print_symbols();
        
        semantic_free();

        ast_free(root);

        return 0;
    }

    printf("Parsing failed.\n");

    return 1;
}