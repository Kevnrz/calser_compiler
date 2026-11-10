/*
 * generate.c — Fase 4: generación de ensamblador desde el AST anotado.
 *
 * Es el punto de entrada y el recorrido de alto nivel: inicializa los globales,
 * genera main primero y después las demás funciones en orden, y cierra con
 * el fin del programa y el manejador de errores aritméticos. El detalle vive
 * en los módulos que comparten GenCtx (generate_internal.h): generate_support
 * (registros, memoria y pila), generate_expr (expresiones y llamadas),
 * generate_stmt (sentencias y control de flujo) y generate_init
 * (inicializadores). cg_schedule organiza las regiones antes del ensamblado
 * salvo la opción --no-schedule.
 *
 * Usa: generate_internal.h, tabla de etiquetas, data_area (base de la pila) y
 * codegen.
 * Lo usa: main.c después del análisis semántico.
 * Referencias: libro §6.4, §6.6, §6.7, §7.2; isa.md §6, §9.
 */

#include <stdio.h>
#include <string.h>

#include "generate.h"
#include "generate_internal.h"
#include "codegen.h"
#include "data_area.h"

/* ==========================================================================
 * FUNCIONES Y PROGRAMA
 * ======================================================================= */


/*
 * function_name — consulta el nombre de una función del AST.
 *
 * Entrada:
 *   function  nodo de función o NULL.
 * Salida:
 *   texto prestado del identificador o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *function_name(ASTNode *function) {
    ASTNode *id = ast_find_child(function, AST_IDENTIFIER);
 
    return id ? id->text : NULL;
}

/*
 * emit_function — genera prólogo, cuerpo y transferencia final.
 *
 * Entrada:
 *   function  nodo AST_FUNCTION validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: configura función actual, emite etiquetas/cuerpo y luego limpia el contexto.
 * Restricciones:
 *   tabla de etiquetas y símbolos vivos; R salta después de inicializar el
 *   retorno.
 */
static void emit_function(GenCtx *gen, ASTNode *function) {
    ASTNode *id_node = ast_find_child(function, AST_IDENTIFIER);
    const char *name = id_node ? id_node->text : NULL;

    if (name == NULL) {
        gen_report_error(gen, function, "funcion sin identificador");
        return;
    }

    gen->current_function_name = name;
    gen->current_function_symbol = gen_get_symbol(id_node);

    char entry_label[128];
    char body_label[128];

    if (!gen_node_label(gen, entry_label, sizeof(entry_label), function,
                    LABEL_FUNCTION, "entrada de la funcion") ||
        !gen_node_label(gen, body_label, sizeof(body_label), function,
                    LABEL_BODY, "cuerpo de la funcion"))
        return;

    cg_set_line(function->line);
    cg_label(entry_label);

    // Prólogo: el retorno empieza en 0 en cada
    // llamada. Va antes de .L_body_: R salta al cuerpo y conserva el valor.
    // main es void y no tiene retorno.
    Symbol *return_symbol = gen_get_return_symbol(gen->current_function_symbol);

    if (return_symbol != NULL)
        cg_op(SLOT_LSU, "sw rz, %d(rz)", return_symbol->address);

    cg_label(body_label);

    ASTNode *body = ast_find_child(function, AST_BLOCK);

    if (body == NULL) {
        gen_report_error(gen, function, "funcion sin cuerpo");
        return;
    }

    gen_reset_registers(gen);

    gen->inside_main = strcmp(name, "main") == 0;
    if (gen->inside_main)
        for (ASTNode *global = gen->program_globals ? gen->program_globals->child : NULL; global; global = global->next) {
            cg_set_line(global->line);
            gen_initialize_declaration(gen, global, 0, 1);
        }

    gen_block(gen, body);

    // Salto final, por si el control llega al final del cuerpo: el semántico
    // exige que la función contenga un E, no que termine en uno. Si la
    // última sentencia ya es un E, ese E emitió el mismo salto y no se repite.
    ASTNode *last = body->child;

    while (last != NULL && last->next != NULL)
        last = last->next;

    if (last == NULL || last->kind != AST_EMIT) {

        // El salto final no corresponde a ninguna sentencia.
        cg_set_line(0);

        if (gen->inside_main) {
            cg_op(SLOT_BRU, "jef %s", gen->program_end_label);
        } else {
            cg_op(SLOT_BRU, "jefR rz, rl");
        }
    }

    gen->inside_main = 0;

    gen->current_function_name = NULL;
    gen->current_function_symbol = NULL;
}

/* generate_code — véase el contrato en generate.h. */
int generate_code(ASTNode *root, const LabelTable *labels, const char *source_name) {
    if (root == NULL || root->kind != AST_PROGRAM)
        return 1;

    // Todo el estado de la generación vive en este contexto (antes, variables
    // static del módulo).
    GenCtx context = {0};
    GenCtx *gen = &context;
    gen->source_name = source_name;

    // La tabla de etiquetas viene de la Fase 3; aquí solo se consulta.
    gen->label_table = labels;
    gen->arith_error_label = label_table_get(labels, NULL, LABEL_ARITH_ERROR);
    gen->program_end_label = label_table_get(labels, NULL, LABEL_PROGRAM_END);

    if (gen->arith_error_label == NULL || gen->program_end_label == NULL) {
        fprintf(stderr,
                "ccom: error interno: faltan las etiquetas globales "
                "en la tabla\n");
        return 1;
    }
 
    cg_init();
    gen_expr_setup(gen);
    gen->program_globals = ast_find_child(root, AST_GLOBALS);

    gen->generation_errors = 0;
    gen->inside_main = 0;

    gen->current_function_name = NULL;
    gen->current_function_symbol = NULL;

    gen_reset_registers(gen);
    for (ASTNode *global = gen->program_globals ? gen->program_globals->child : NULL; global; global = global->next)
        gen_initialize_declaration(gen, global, 1, 0);
    // Puntero de la pila de llamadas: primera dirección libre de la zona
    // de ccom (ver el comentario de la pila al inicio del archivo).
    cg_op(SLOT_ALU0,
          "mov rx, %d",
          data_area_end());
 
    ASTNode *functions = ast_find_child(root, AST_FUNCTIONS);
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
 
    emit_function(gen, main_function);

    for (ASTNode *f = functions ? functions->child : NULL;
         f != NULL;
         f = f->next) {

        const char *name = function_name(f);

        if (name != NULL && strcmp(name, "main") == 0)
            continue;

        emit_function(gen, f);
    }

    cg_set_line(0);
    gen_end_of_main(gen);

    /* Manejador terminal: no hay consumidor ni camino de continuación.
     * Emitir su salto a sí mismo sin la separación automática de cg_op. */
    char terminal_jump[128];
    snprintf(terminal_jump, sizeof terminal_jump, "jef %s", gen->arith_error_label);
    cg_label(gen->arith_error_label);
    cg_bundle(NULL, NULL, terminal_jump, NULL);

    return gen->generation_errors == 0 ? 0 : 1;
}
