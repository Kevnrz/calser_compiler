#include <stdio.h>
#include <string.h>
 
#include "generate.h"
#include "codegen.h"
 
/* Fase 4, esqueleto (hito 1 de Kevin).
 * Orden de emisión (contrato §3): main primero, para que arranque en el
 * bundle 0; luego las demás funciones; al final las rutinas. */
 
static ASTNode *find_child(ASTNode *node, ASTKind kind) {
    for (ASTNode *c = node ? node->child : NULL; c != NULL; c = c->next)
        if (c->kind == kind)
            return c;
 
    return NULL;
}
 
static const char *function_name(ASTNode *function) {
    ASTNode *id = find_child(function, AST_IDENTIFIER);
 
    return id ? id->text : NULL;
}
 
// `E` de main termina el programa (contrato §3):
//   fin_main: { nopi | nopi | jef fin_main | nopi }
static void emit_end_of_main(void) {
    cg_label("fin_main");
    cg_bundle(NULL, NULL, "jef fin_main", NULL);
}
 
// main no tiene prólogo (no guarda rl). TODO: cuerpo (expresiones, control).
static void emit_main(ASTNode *function) {
    (void) function;
 
    cg_label(".L_func_main");
    cg_label(".L_body_main");
 
    // TODO(Kevin, paso 2): generar las sentencias del cuerpo. Cada `E` de
    // main debe saltar a fin_main; por ahora el fin va al final.
    emit_end_of_main();
}
 
int generate_code(ASTNode *root) {
    if (root == NULL || root->kind != AST_PROGRAM)
        return 1;
 
    cg_init();
 
    ASTNode *functions = find_child(root, AST_FUNCTIONS);
    ASTNode *main_function = NULL;
 
    for (ASTNode *f = functions ? functions->child : NULL; f != NULL; f = f->next) {
        const char *name = function_name(f);
 
        if (name != NULL && strcmp(name, "main") == 0)
            main_function = f;
    }
 
    if (main_function == NULL) {
        fprintf(stderr, "ccom: error interno: no hay 'main'\n");
        return 1;
    }
 
    emit_main(main_function);
 
    // TODO(Kevin, paso 3): demás funciones (prólogo, .L_body_, E, R, llamadas).
    // TODO(Gabriel): rutinas de *, /, %, ^ al final.
 
    return 0;
}
