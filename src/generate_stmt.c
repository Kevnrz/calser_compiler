/*
 * generate_stmt.c — Fase 4: sentencias, asignaciones y control de flujo.
 *
 * Genera bloques, asignaciones (escalares, de elemento y de arreglo completo),
 * C/A, los ciclos L y S, el retorno E y el reinicio R.
 *
 * Usa: generate_internal.h, codegen.h, codegen_array.h.
 * Lo usa: generate.c.
 * Referencias: libro §6.6, §7.2; isa.md §6.2.
 */

#include <stdio.h>
#include <string.h>

#include "generate_internal.h"
#include "codegen.h"
#include "codegen_array.h"


/* ==========================================================================
 * BLOQUES, ASIGNACIONES Y CONTROL DE FLUJO
 * ======================================================================= */


/* gen_block — véase el contrato en generate_internal.h. */
void gen_block(GenCtx *gen, ASTNode *block) {
    if (block == NULL)
        return;

    for (ASTNode *stmt = block->child;
         stmt != NULL;
         stmt = stmt->next) {

        gen_statement(gen, stmt);
    }
}

/*
 * emit_array_copy — copia un arreglo completo en otro.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_ASSIGNMENT, para los diagnósticos.
 *   whole  símbolo del arreglo destino.
 *   value  nodo del origen (un identificador de arreglo).
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reserva cuatro temporales de trabajo, copia todas las palabras
 *   con cg_array_copy y los libera; reporta errores.
 * Restricciones:
 *   el semántico ya exigió que el origen sea un arreglo de la misma forma;
 *   cualquiera de los dos puede ser un parámetro arreglo (cg_array_copy usa
 *   la dirección que guarda).
 */
static void emit_array_copy(GenCtx *gen, ASTNode *node, Symbol *whole,
                            ASTNode *value) {
    Symbol *source =
        value->kind == AST_IDENTIFIER ? gen_get_symbol(value) : NULL;

    int work[4];
    int allocated = 0;

    while (allocated < 4 && (work[allocated] = gen_alloc_reg(gen)) >= 0)
        allocated++;

    if (allocated < 4)
        gen_report_error(gen, node, "sin registros para copiar el arreglo");
    else if (source == NULL ||
             !cg_array_copy(&gen->arithmetic, whole, source, work))
        gen_report_error(gen, node, "no se pudo copiar el arreglo");

    for (int i = 0; i < allocated; i++)
        gen_free_reg(gen, work[i]);
}

/*
 * emit_element_store — guarda un valor en un elemento de arreglo.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   target  nodo AST_INDEX del destino.
 *   rd  temporal reservado con el valor a guardar.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: evalúa los índices, calcula la dirección del elemento y
 *   guarda el valor; reporta errores.
 * Restricciones:
 *   consume rd: lo libera en todos los caminos, incluidos los de error.
 *   Los hijos de AST_INDEX son el identificador del arreglo y
 *   AST_ARGUMENTS, con los índices en child/next.
 */
static void emit_element_store(GenCtx *gen, ASTNode *target, int rd) {
    ASTNode *array_id = gen_child_at(target, 0);

    if (array_id == NULL ||
        array_id->kind != AST_IDENTIFIER) {
        gen_report_error(gen, target, "identificador de arreglo invalido");
        gen_free_reg(gen, rd);
        return;
    }

    Symbol *array = gen_get_symbol(array_id);

    if (array == NULL) {
        gen_report_error(gen, array_id, "arreglo sin simbolo semantico");
        gen_free_reg(gen, rd);
        return;
    }

    if (array->dimensions < 1 ||
        array->dimensions > MAX_DIMS) {
        gen_report_error(gen, target, "numero de dimensiones invalido");
        gen_free_reg(gen, rd);
        return;
    }

    // Extrae los índices del contenedor AST_ARGUMENTS.
    int indices[MAX_DIMS];
    int count = 0;

    if (gen_array_indices(gen, target, array, array_id->next->child,
                          indices, &count) != 0) {
        gen_free_reg(gen, rd);
        return;
    }

    // Reserva registros para la dirección del elemento.
    int address = gen_alloc_reg(gen);
    int work[2] = {-1, -1};

    if (address >= 0)
        work[0] = gen_alloc_reg(gen);

    if (work[0] >= 0)
        work[1] = gen_alloc_reg(gen);

    if (address < 0 || work[0] < 0 || work[1] < 0) {
        gen_report_error(gen, target, "sin registros para acceder al arreglo");

        if (address >= 0) gen_free_reg(gen, address);
        if (work[0] >= 0) gen_free_reg(gen, work[0]);
        if (work[1] >= 0) gen_free_reg(gen, work[1]);

        for (int i = 0; i < count; i++)
            gen_free_reg(gen, indices[i]);

        gen_free_reg(gen, rd);
        return;
    }

    // Calcula la dirección del elemento y guarda el valor.
    int ok = cg_array_address(
        &gen->arithmetic,
        array,
        indices,
        count,
        address,
        work
    );

    if (ok) {
        ok = cg_array_store(
            &gen->arithmetic,
            array,
            indices,
            count,
            address,
            rd,
            work
        );
    }

    // Libera registros temporales.
    for (int i = 0; i < count; i++)
        gen_free_reg(gen, indices[i]);

    gen_free_reg(gen, address);
    gen_free_reg(gen, work[0]);
    gen_free_reg(gen, work[1]);
    gen_free_reg(gen, rd);

    if (!ok)
        gen_report_error(gen, target, "no se pudo generar la asignacion al arreglo");
}

/*
 * emit_assignment — genera asignación escalar, de elemento o de arreglo completo.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_ASSIGNMENT validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite escrituras/copia, administra temporales y reporta errores.
 * Restricciones:
 *   destino mutable; arreglos completos de igual forma; índices dinámicos
 *   válidos.
 */
static void emit_assignment(GenCtx *gen, ASTNode *node) {
    ASTNode *target = gen_child_at(node, 0);
    ASTNode *value = gen_child_at(node, 1);

    if (target == NULL || value == NULL) {
        gen_report_error(gen, node,
                         "asignacion incompleta");
        return;
    }

    // Asignación de un arreglo completo (destino = nombre de arreglo sin
    // índices): se copian todas sus palabras.
    Symbol *whole =
        target->kind == AST_IDENTIFIER ? gen_get_symbol(target) : NULL;

    if (whole != NULL && whole->dimensions > 0) {
        emit_array_copy(gen, node, whole, value);
        return;
    }

    // Evalúa primero el lado derecho.
    // Conserva su valor en un registro hasta completar la asignación.
    int rd = gen_expression(gen, value);

    if (rd < 0)
        return;

    // Asignaciones escalares.
    if (target->kind == AST_IDENTIFIER) {
        Symbol *symbol = gen_get_symbol(target);

        if (symbol == NULL) {
            gen_report_error(gen, target, "variable sin simbolo semantico");
            gen_free_reg(gen, rd);
            return;
        }

        if (gen_store_symbol(gen, symbol, rd, target) != 0) {
            gen_free_reg(gen, rd);
            return;
        }

        gen_free_reg(gen, rd);
        return;
    }

    // Asignación de elemento de arreglo.
    if (target->kind == AST_INDEX) {
        emit_element_store(gen, target, rd);
        return;
    }

    gen_report_error(gen, target, "destino de asignacion no valido");
    gen_free_reg(gen, rd);
}

/*
 * ConditionalParts — las partes de un C, encontradas por su clase.
 */
typedef struct {
    ASTNode *condition;
    ASTNode *body;
    ASTNode *alternatives;   // AST_ALTERNATIVES o NULL
    ASTNode *residual;       // AST_RESIDUAL o NULL
} ConditionalParts;

/*
 * find_conditional_parts — separa condición, cuerpo, alternativas y residual.
 *
 * Entrada:
 *   node  nodo AST_CONDITIONAL.
 *   parts  estructura a llenar.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: llena *parts; las partes ausentes quedan en NULL.
 * Restricciones:
 *   busca sin suponer posiciones fijas: los constructores omiten hijos NULL,
 *   así que los opcionales se reconocen por su clase.
 */
static void find_conditional_parts(ASTNode *node, ConditionalParts *parts) {
    parts->condition = NULL;
    parts->body = NULL;
    parts->alternatives = NULL;
    parts->residual = NULL;

    for (ASTNode *c = node->child;
         c != NULL;
         c = c->next) {

        if (parts->condition == NULL &&
            c->kind != AST_BLOCK &&
            c->kind != AST_ALTERNATIVES &&
            c->kind != AST_RESIDUAL) {

            parts->condition = c;
        }
        else if (c->kind == AST_BLOCK && parts->body == NULL) {
            parts->body = c;
        }
        else if (c->kind == AST_ALTERNATIVES) {
            parts->alternatives = c;
        }
        else if (c->kind == AST_RESIDUAL) {
            parts->residual = c;
        }
    }
}

/*
 * emit_alternatives — genera las ramas A [condición] de un C, en orden.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   alternatives  nodo AST_ALTERNATIVES.
 *   end_label  etiqueta del fin del C, adonde salta cada rama al terminar.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: por cada alternativa emite su condición, el salto a la
 *   siguiente rama, el cuerpo y el salto al fin; reporta alternativas
 *   incompletas o sin etiqueta y sigue con la siguiente.
 * Restricciones:
 *   etiquetas LABEL_NEXT de cada A registradas.
 */
static void emit_alternatives(GenCtx *gen, ASTNode *alternatives,
                              const char *end_label) {
    for (ASTNode *alt = alternatives->child;
         alt != NULL;
         alt = alt->next) {

        if (alt->kind != AST_ALTERNATIVE)
            continue;

        ASTNode *alt_condition = gen_child_at(alt, 0);
        ASTNode *alt_body = gen_child_at(alt, 1);

        if (alt_condition == NULL || alt_body == NULL) {
            gen_report_error(gen, alt,
                             "alternativa incompleta");
            continue;
        }

        char following[64];

        if (!gen_node_label(gen, following, sizeof(following), alt, LABEL_NEXT,
                            "siguiente rama del A"))
            continue;

        cg_set_line(alt->line);

        int rc = gen_expression(gen, alt_condition);

        if (rc < 0)
            continue;

        cg_op(SLOT_BRU,
              "beq %s, rz, %s",
              gen_reg_name(rc),
              following);

        gen_free_reg(gen, rc);

        gen_block(gen, alt_body);

        cg_set_line(alt->line);

        cg_op(SLOT_BRU,
              "jef %s",
              end_label);

        cg_label(following);
    }
}

/*
 * emit_conditional — genera C, sus alternativas A y la rama residual.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_CONDITIONAL validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite condiciones, cuerpos, saltos y etiquetas; actualiza línea de emisión.
 * Restricciones:
 *   etiquetas LABEL_NEXT y LABEL_END registradas; condiciones bool.
 */
static void emit_conditional(GenCtx *gen, ASTNode *node) {
    ConditionalParts parts;

    find_conditional_parts(node, &parts);

    if (parts.condition == NULL || parts.body == NULL) {
        gen_report_error(gen, node,
                         "estructura condicional incompleta");
        return;
    }

    char end_label[64];
    char next_label[64];

    if (!gen_node_label(gen, end_label, sizeof(end_label), node, LABEL_END,
                        "fin del C") ||
        !gen_node_label(gen, next_label, sizeof(next_label), node, LABEL_NEXT,
                        "siguiente rama del C"))
        return;

    int rc = gen_expression(gen, parts.condition);

    if (rc < 0)
        return;

    cg_op(SLOT_BRU,
          "beq %s, rz, %s",
          gen_reg_name(rc),
          next_label);

    gen_free_reg(gen, rc);

    gen_block(gen, parts.body);

    // El salto al final del C es del C, no de la última sentencia del cuerpo.
    cg_set_line(node->line);

    cg_op(SLOT_BRU,
          "jef %s",
          end_label);

    cg_label(next_label);

    // Verifica cada alternativa A [condición] en orden.
    if (parts.alternatives != NULL)
        emit_alternatives(gen, parts.alternatives, end_label);

    // La rama residual se ejecuta si ninguna condición anterior se cumplió.
    if (parts.residual != NULL) {
        ASTNode *residual_body = gen_child_at(parts.residual, 0);

        if (residual_body != NULL)
            gen_block(gen, residual_body);
    }

    cg_label(end_label);
}

/*
 * emit_loop_l — genera un ciclo por predicado L.
 *
 * Entrada:
 *   node  nodo AST_LOOP_L validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite condición, cuerpo, salto de retorno y etiquetas.
 * Restricciones:
 *   condición bool y etiquetas START/END registradas.
 */
static void emit_loop_l(GenCtx *gen, ASTNode *node) {
    ASTNode *condition = gen_child_at(node, 0);
    ASTNode *body = gen_child_at(node, 1);

    if (condition == NULL || body == NULL) {
        gen_report_error(gen, node, "ciclo L incompleto");
        return;
    }

    char start_label[64];
    char end_label[64];

    if (!gen_node_label(gen, start_label, sizeof(start_label), node, LABEL_START,
                    "inicio del L") ||
        !gen_node_label(gen, end_label, sizeof(end_label), node, LABEL_END,
                    "fin del L"))
        return;

    cg_label(start_label);

    int rc = gen_expression(gen, condition);

    if (rc < 0)
        return;

    cg_op(SLOT_BRU,
          "beq %s, rz, %s",
          gen_reg_name(rc),
          end_label);

    gen_free_reg(gen, rc);

    gen_block(gen, body);

    cg_set_line(node->line);

    cg_op(SLOT_BRU,
          "jef %s",
          start_label);

    cg_label(end_label);
}

/*
 * LoopS — lo que el ciclo S necesita en sus cuatro fases.
 */
typedef struct {
    ASTNode *node;           // AST_LOOP_S
    ASTNode *id_node;        // variable de control
    ASTNode *start_node;
    ASTNode *end_node;
    ASTNode *step_node;      // AST_NUMBER con el signo aplicado
    ASTNode *body;
    Symbol *var;             // símbolo de la variable de control
    long step;
    int limit;               // casilla oculta donde se guarda el fin
    char test_label[64];     // LABEL_START: prueba de cada vuelta
    char update_label[64];   // LABEL_UPDATE: incremento
    char end_label[64];      // LABEL_END
} LoopS;

/*
 * loop_s_prepare — valida la estructura del S y reúne sus partes.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_LOOP_S.
 *   loop  estructura a llenar.
 * Salida:
 *   0 si el ciclo es válido; -1 si reporta un error.
 *   Efectos: llena *loop; reporta estructura inválida, variable de control
 *   que no es int o ciclo sin casilla para su fin.
 * Restricciones:
 *   los cinco hijos tienen posición fija: variable, inicio, fin, paso y cuerpo.
 */
static int loop_s_prepare(GenCtx *gen, ASTNode *node, LoopS *loop) {
    loop->node = node;
    loop->id_node    = gen_child_at(node, 0);
    loop->start_node = gen_child_at(node, 1);
    loop->end_node   = gen_child_at(node, 2);
    loop->step_node  = gen_child_at(node, 3);
    loop->body       = gen_child_at(node, 4);

    if (!loop->id_node || loop->id_node->kind != AST_IDENTIFIER ||
        !loop->start_node || !loop->end_node || !loop->body ||
        !loop->step_node || loop->step_node->kind != AST_NUMBER ||
        loop->step_node->number == 0) {
        gen_report_error(gen, node, "estructura del ciclo S invalida");
        return -1;
    }

    loop->var = gen_get_symbol(loop->id_node);

    if (!loop->var || loop->var->type != TYPE_INT ||
        loop->var->kind == SYMBOL_FUNCTION) {
        gen_report_error(gen, loop->id_node, "variable de control debe ser int");
        return -1;
    }

    if (loop->var->limit_address < 0) {
        gen_report_error(gen, loop->id_node, "ciclo S sin casilla para su fin");
        return -1;
    }

    loop->step = loop->step_node->number;
    loop->limit = loop->var->limit_address;

    return 0;
}

/*
 * loop_s_enter — inicializa el ciclo: variable de control y fin.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   loop  ciclo ya validado.
 * Salida:
 *   0 si emite la entrada y obtiene las etiquetas; -1 si falla.
 *   Efectos: al entrar, var = inicio y la casilla oculta = fin; copia las
 *   tres etiquetas del ciclo desde la tabla.
 * Restricciones:
 *   inicio y fin se evalúan antes de escribir var, porque fin puede usar un
 *   nombre igual de afuera.
 */
static int loop_s_enter(GenCtx *gen, LoopS *loop) {
    int start = gen_expression(gen, loop->start_node);

    if (start < 0)
        return -1;

    int end = gen_expression(gen, loop->end_node);

    if (end < 0) {
        gen_free_reg(gen, start);
        return -1;
    }

    gen_store_symbol(gen, loop->var, start, loop->id_node);
    cg_op(SLOT_LSU, "sw %s, %d(rz)", gen_reg_name(end), loop->limit);

    gen_free_reg(gen, start);
    gen_free_reg(gen, end);

    if (!gen_node_label(gen, loop->test_label, sizeof(loop->test_label),
                        loop->node, LABEL_START, "inicio del S") ||
        !gen_node_label(gen, loop->update_label, sizeof(loop->update_label),
                        loop->node, LABEL_UPDATE, "incremento del S") ||
        !gen_node_label(gen, loop->end_label, sizeof(loop->end_label),
                        loop->node, LABEL_END, "fin del S"))
        return -1;

    return 0;
}

/*
 * loop_s_test — emite la prueba de cada vuelta.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   loop  ciclo con sus etiquetas ya obtenidas.
 * Salida:
 *   0 si emite la prueba; -1 si falla.
 *   Efectos: etiqueta de inicio, carga de var y fin y salto de salida.
 * Restricciones:
 *   cada vuelta se cargan var y fin y se compara con signo (isa.md §6.2,
 *   bge). Se sale cuando ya no se cumple la condición de seguir:
 *     paso > 0: sigue si var < fin, sale si var >= fin
 *     paso < 0: sigue si var > fin, sale si fin >= var
 *   Ningún registro queda ocupado durante el cuerpo.
 */
static int loop_s_test(GenCtx *gen, LoopS *loop) {
    cg_label(loop->test_label);

    int current = gen_alloc_reg(gen);
    int bound = current >= 0 ? gen_alloc_reg(gen) : -1;

    if (current < 0 || bound < 0) {
        gen_report_error(gen, loop->node, "sin registros para generar el ciclo S");

        if (current >= 0)
            gen_free_reg(gen, current);

        return -1;
    }

    if (gen_load_symbol(gen, current, loop->var, loop->id_node) != 0) {
        gen_free_reg(gen, current);
        gen_free_reg(gen, bound);
        return -1;
    }

    cg_op(SLOT_LSU, "lw %s, %d(rz)", gen_reg_name(bound), loop->limit);

    if (loop->step > 0)
        cg_op(SLOT_BRU, "bge %s, %s, %s",
              gen_reg_name(current), gen_reg_name(bound), loop->end_label);
    else
        cg_op(SLOT_BRU, "bge %s, %s, %s",
              gen_reg_name(bound), gen_reg_name(current), loop->end_label);

    gen_free_reg(gen, current);
    gen_free_reg(gen, bound);

    return 0;
}

/*
 * loop_s_update — emite el incremento y el salto de regreso a la prueba.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   loop  ciclo con sus etiquetas ya obtenidas.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: var = var + paso, salto a la prueba y etiqueta de fin.
 * Restricciones:
 *   el incremento es del S, no de la última sentencia del cuerpo. El paso va
 *   como inmediato si cabe en 18 bits con signo (isa.md §4.2); si no, se
 *   carga como constante de 32 bits (§4.3).
 */
static void loop_s_update(GenCtx *gen, LoopS *loop) {
    // Inicio del incremento (rol UPDATE de la tabla). Es del S.
    cg_set_line(loop->node->line);
    cg_label(loop->update_label);

    int current = gen_alloc_reg(gen);

    if (current < 0) {
        gen_report_error(gen, loop->node, "sin registros para generar el ciclo S");
        return;
    }

    if (gen_load_symbol(gen, current, loop->var, loop->id_node) != 0) {
        gen_free_reg(gen, current);
        return;
    }

    if (loop->step >= -131072 && loop->step <= 131071) {
        cg_op(SLOT_ALU0, "saddi %s, %s, %ld",
              gen_reg_name(current), gen_reg_name(current), loop->step);
    }
    else {
        int amount = gen_alloc_reg(gen);

        if (amount < 0 || gen_mov(gen, amount, loop->step, loop->step_node) != 0) {
            if (amount < 0)
                gen_report_error(gen, loop->node, "sin registros para generar el ciclo S");
            else
                gen_free_reg(gen, amount);

            gen_free_reg(gen, current);
            return;
        }

        cg_op(SLOT_ALU0, "add %s, %s, %s",
              gen_reg_name(current), gen_reg_name(current), gen_reg_name(amount));

        gen_free_reg(gen, amount);
    }

    gen_store_symbol(gen, loop->var, current, loop->id_node);

    gen_free_reg(gen, current);

    cg_op(SLOT_BRU, "jef %s", loop->test_label);
    cg_label(loop->end_label);
}

/*
 * emit_loop_s — genera el ciclo S: entrada, prueba, cuerpo e incremento.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_LOOP_S validado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite el ciclo; ante un error corta donde está y deja emisión parcial.
 * Restricciones:
 *   el fin se evalúa una sola vez al entrar y se guarda en una casilla oculta
 *   junto a la variable de control (calser_diseno.md §5.8.2).
 */
static void emit_loop_s(GenCtx *gen, ASTNode *node) {
    LoopS loop;

    if (loop_s_prepare(gen, node, &loop) != 0)
        return;

    if (loop_s_enter(gen, &loop) != 0)
        return;

    if (loop_s_test(gen, &loop) != 0)
        return;

    gen_block(gen, loop.body);

    loop_s_update(gen, &loop);
}

/* gen_end_of_main — véase el contrato en generate_internal.h. */
void gen_end_of_main(GenCtx *gen) {
    char jump[96];

    snprintf(jump, sizeof(jump), "jef %s", gen->program_end_label);

    cg_label(gen->program_end_label);
    cg_bundle(NULL, NULL, jump, NULL);
}

/*
 * emit_return — genera la transferencia de control de E.
 *
 * Entrada:
 *   node  nodo E, no utilizado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite jef a fin_main o jefR rz, rl.
 * Restricciones:
 *   inside_main refleja la función actual; rl contiene el enlace de
 *   retorno fuera de main.
 */
static void emit_return(GenCtx *gen, ASTNode *node) {
    (void)node;

    if (gen->inside_main) {
        cg_op(SLOT_BRU, "jef %s", gen->program_end_label);
    } else {
        // jefR rz, rl salta a la dirección de rl. El nuevo enlace se escribe
        // en rz y se descarta (isa.md §6.2 y §8).
        cg_op(SLOT_BRU, "jefR rz, rl");
    }
}

/*
 * RewindInfo — lo que se sabe de un R una vez validado.
 */
typedef struct {
    ASTNode *first_arg;      // primer argumento
    int arg_count;
    int param_count;         // parámetros de la función actual
} RewindInfo;

/*
 * check_rewind — valida un R antes de emitir nada.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_REWIND no NULL.
 *   info  estructura a llenar.
 * Salida:
 *   0 si el R es válido; -1 si reporta un error.
 *   Efectos: llena *info; reporta el primer problema encontrado.
 * Restricciones:
 *   AST_REWIND tiene el identificador de la función y, opcional, sus
 *   argumentos. R se restringe a la función actual. Se verifica la lista
 *   de parámetros antes de emitir escrituras, para no encontrar un
 *   parámetro ausente después de empezar a actualizarlos.
 */
static int check_rewind(GenCtx *gen, ASTNode *node, RewindInfo *info) {
    ASTNode *target_id = gen_child_at(node, 0);
    ASTNode *args_node = gen_child_at(node, 1);

    if (target_id == NULL || target_id->text == NULL) {
        gen_report_error(gen, node,
                         "sentencia R sin identificador de funcion");
        return -1;
    }

    // R se restringe a la función actual.
    if (gen->current_function_name == NULL ||
        strcmp(target_id->text, gen->current_function_name) != 0) {

        gen_report_error(gen, node,
                         "R debe referirse a la funcion actual");
        return -1;
    }

    if (gen->current_function_symbol == NULL) {
        gen_report_error(gen, node,
                         "funcion actual sin simbolo semantico");
        return -1;
    }

    // Obtiene la lista de argumentos y verifica su cantidad.
    info->first_arg = gen_call_arguments(args_node, &info->arg_count);
    info->param_count = gen->current_function_symbol->parameter_count;

    if (info->arg_count != info->param_count) {
        gen_report_error(gen, node,
                         "cantidad incorrecta de argumentos en R");
        return -1;
    }

    // Cada argumento conserva su valor hasta evaluar todos los argumentos.
    // Esta implementación admite hasta TEMP_COUNT argumentos.
    if (info->arg_count > TEMP_COUNT) {
        gen_report_error(gen, node,
                         "demasiados argumentos para R");
        return -1;
    }

    if (info->param_count > 0 &&
        gen->current_function_symbol->param_list == NULL) {

        gen_report_error(gen, node,
                         "funcion sin lista de parametros");
        return -1;
    }

    for (int i = 0; i < info->param_count; i++) {
        Symbol *param = gen->current_function_symbol->param_list[i];

        if (param == NULL) {
            gen_report_error(gen, node,
                             "parametro invalido en la tabla de simbolos");
            return -1;
        }

        if (param->address < 0 || param->address > 131071) {
            gen_report_error(gen, node,
                             "direccion de parametro invalida en R");
            return -1;
        }
    }

    return 0;
}

/*
 * emit_rewind_arguments — evalúa los argumentos de R y actualiza los parámetros.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_REWIND, para los diagnósticos.
 *   info  R ya validado.
 * Salida:
 *   0 si actualiza todos los parámetros; -1 si algo falla.
 *   Efectos: evalúa todos los argumentos sin tocar los parámetros, escribe
 *   los valores nuevos en sus casillas y libera los temporales.
 * Restricciones:
 *   evaluar todo antes de escribir evita que un argumento lea un parámetro
 *   que R ya actualizó. Argumentos y parámetros se corresponden por orden
 *   de declaración.
 */
static int emit_rewind_arguments(GenCtx *gen, ASTNode *node,
                                 const RewindInfo *info) {
    // Evalúa todos los argumentos.
    // Todavía no actualiza parámetros.
    int arg_regs[TEMP_COUNT];
    int evaluated = 0;

    for (ASTNode *a = info->first_arg; a != NULL; a = a->next) {
        int reg = gen_expression(gen, a);

        if (reg < 0) {
            // Libera los registros reservados para argumentos anteriores.
            for (int i = 0; i < evaluated; i++) {
                gen_free_reg(gen, arg_regs[i]);
            }

            return -1;
        }

        arg_regs[evaluated++] = reg;
    }

    // Escribe los valores nuevos en las direcciones de parámetros.
    for (int i = 0; i < info->param_count; i++) {
        Symbol *param = gen->current_function_symbol->param_list[i];

        if (gen_store_symbol(gen, param, arg_regs[i], node) != 0) {
            for (int j = 0; j < evaluated; j++) {
                gen_free_reg(gen, arg_regs[j]);
            }

            return -1;
        }
    }

    // Libera los registros temporales.
    for (int i = 0; i < evaluated; i++) {
        gen_free_reg(gen, arg_regs[i]);
    }

    return 0;
}

/*
 * emit_rewind — genera R: reinicia el cuerpo de la función actual.
 *
 * Entrada:
 *   gen  contexto de la generación.
 *   node  nodo AST_REWIND o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza los parámetros con los argumentos y salta al cuerpo.
 * Restricciones:
 *   conserva rl: R reinicia el cuerpo de la función, sin crear otro enlace
 *   de llamada.
 */
static void emit_rewind(GenCtx *gen, ASTNode *node) {
    if (node == NULL) {
        gen_report_error(gen, NULL, "sentencia R invalida");
        return;
    }

    RewindInfo info;

    if (check_rewind(gen, node, &info) != 0)
        return;

    if (emit_rewind_arguments(gen, node, &info) != 0)
        return;

    // Reinicia el cuerpo de la función actual.
    char body_label[128];

    if (!gen_node_label(gen, body_label, sizeof(body_label),
                        (ASTNode *) gen->current_function_symbol->decl_node,
                        LABEL_BODY, "cuerpo de la funcion"))
        return;

    cg_op(SLOT_BRU, "jef %s", body_label);
}

/* gen_statement — véase el contrato en generate_internal.h. */
void gen_statement(GenCtx *gen, ASTNode *node) {
    if (node == NULL)
        return;

    // .asm anotado: los bundles de esta sentencia llevan su línea.
    cg_set_line(node->line);

    switch (node->kind) {

        case AST_BLOCK:
            gen_block(gen, node);
            break;

        case AST_ASSIGNMENT:
            emit_assignment(gen, node);
            break;

        case AST_DECLARATION:
            gen_initialize_declaration(gen, node, 0, 0);
            break;

        case AST_CONDITIONAL:
            emit_conditional(gen, node);
            break;

        case AST_LOOP_L:
            emit_loop_l(gen, node);
            break;

        case AST_EMIT:
            emit_return(gen, node);
            break;

        case AST_CALL_STATEMENT: {
            ASTNode *call = node->child;

            if (call == NULL || call->kind != AST_CALL) {
                gen_report_error(gen, node, "llamada como sentencia invalida");
                
            } else {
                gen_function_call(gen, call, 0);
            }
            break;
        }

        case AST_REWIND:
            emit_rewind(gen, node);
            break;

        case AST_LOOP_S:
            emit_loop_s(gen, node);
            break;

        case AST_CRYPTO:
            gen_crypto_statement(gen, node);
            break;

        default:
            gen_report_error(gen, node,
                         "tipo de sentencia no soportado");
            break;
    }
}
