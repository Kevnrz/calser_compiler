/*
 * tac_build.c — propiedad, funciones y ayudas de la construcción desde AST.
 * Usa: tac_build_internal.h. Lo usan main y pruebas, solo con AST validado.
 * Referencias: Aho §6.4, §8.4.1; enunciado §4.1. No modifica el generador.
 */
#include "tac_build_internal.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

/* Reserva inicializada; conserva el manejo fatal de memoria de tac.c. */
void *tb_allocate(size_t bytes) {
    void *memory = calloc(1, bytes ? bytes : 1);
    if (!memory) {
        fputs("tac_build: sin memoria\n", stderr);
        exit(EXIT_FAILURE);
    }
    return memory;
}

/* Conserva el primer diagnóstico y permite liberar un resultado incompleto. */
void tb_fail(TacBuilder *builder, const ASTNode *node, const char *message) {
    if (!builder->failed && builder->capacity) {
        snprintf(builder->error, builder->capacity, "linea %d: %s",
                 node ? node->line : 0, message);
    }
    builder->failed = 1;
}

/* Acceso posicional a hijos de la forma AST ya validada. */
ASTNode *tb_child(ASTNode *node, size_t index) {
    ASTNode *child = node ? node->child : NULL;
    while (child && index) {
        child = child->next;
        index--;
    }
    return child;
}

/* Símbolo prestado; nunca busca nombres ni crea otra identidad semántica. */
const Symbol *tb_symbol(ASTNode *node) {
    return node ? (const Symbol *)node->symbol : NULL;
}

/* Agrega una instrucción al camino abierto y conserva línea/columna de origen. */
TacInstruction *tb_emit(TacBuilder *builder, ASTNode *node, TacOpcode op,
                         TacOperand dest, TacOperand src1, TacOperand src2) {
    if (builder->failed || !builder->block) {
        return NULL;
    }
    TacInstruction *instruction = tac_append(builder->block, op, dest, src1, src2);
    instruction->source_line = node ? node->line : 0;
    instruction->source_column = node ? node->column : 0;
    return instruction;
}

/* Materializa el resultado de una operación en una identidad virtual nueva. */
TacOperand tb_value(TacBuilder *builder, ASTNode *node, TacOpcode op,
                     TacOperand src1, TacOperand src2) {
    TacOperand result = tac_temp_new(builder->function);
    tb_emit(builder, node, op, result, src1, src2);
    return result;
}

/* Inserta después del actual; permite reservar destinos antes de traducir ramas. */
TacBlock *tb_reserve_block(TacBuilder *builder) {
    return tac_block_insert_after(builder->function, builder->block);
}

/* Cierra un camino; NULL significa que E/R ya lo cerró. */
void tb_jump(TacBuilder *builder, ASTNode *node, TacBlock *target) {
    tb_emit(builder, node, TAC_GOTO, tac_label(target), tac_none(), tac_none());
    builder->block = NULL;
}

/* Traduce entrada y cuerpo separados: R omite la inicialización del retorno. */
static void build_function(TacBuilder *builder, ASTNode *node) {
    const Symbol *symbol = tb_symbol(ast_find_child(node, AST_IDENTIFIER));
    builder->function = tac_function_new(builder->result->program, symbol);
    builder->block = tac_block_new(builder->function);
    ASTNode *return_type = ast_find_child(node, AST_RETURN_TYPE);
    builder->return_symbol = tb_symbol(ast_find_child(return_type, AST_IDENTIFIER));
    if (builder->return_symbol) {
        tb_emit(builder, node, TAC_COPY, tac_variable(builder->return_symbol),
                tac_integer(0), tac_none());
    }
    builder->body = tb_reserve_block(builder);
    tb_jump(builder, node, builder->body);
    builder->block = builder->body;
    if (!strcmp(symbol->name, "main")) {
        for (ASTNode *global = tb_child(builder->globals, 0); global; global = global->next) {
            tb_initialize(builder, global, TB_DYNAMIC);
        }
    }
    tb_statements(builder, ast_find_child(node, AST_BLOCK));
    tb_return(builder, NULL);
    if (!builder->failed && !tac_rebuild_edges(builder->function,
                                               builder->error, builder->capacity)) {
        builder->failed = 1;
    }
}

/* tac_build — véase el contrato en tac_build.h. */
TacBuildResult *tac_build(ASTNode *root, const Scope *global_scope,
                         char *error, size_t capacity) {
    if (capacity) {
        error[0] = '\0';
    }
    TacBuilder builder = {.error = error, .capacity = capacity};
    if (!root || root->kind != AST_PROGRAM || !global_scope ||
        global_scope->kind != SCOPE_GLOBAL) {
        tb_fail(&builder, root, "se requiere AST validado y ambito global");
        return NULL;
    }
    builder.result = tb_allocate(sizeof *builder.result);
    builder.result->program = tac_program_new(global_scope);
    builder.globals = ast_find_child(root, AST_GLOBALS);
    for (ASTNode *node = tb_child(builder.globals, 0); node; node = node->next) {
        const Symbol *symbol = tb_symbol(ast_find_child(node, AST_IDENTIFIER));
        size_t end = (size_t)symbol->address + (size_t)symbol->width;
        if (end > builder.result->initial_data_size) {
            builder.result->initial_data_size = end;
        }
    }
    builder.result->initial_data = tb_allocate(builder.result->initial_data_size);
    for (ASTNode *node = tb_child(builder.globals, 0); node; node = node->next) {
        tb_initialize(&builder, node, TB_IMAGE);
    }
    ASTNode *functions = ast_find_child(root, AST_FUNCTIONS);
    for (ASTNode *node = tb_child(functions, 0); node && !builder.failed; node = node->next) {
        build_function(&builder, node);
    }
    if (!builder.failed && !tac_validate(builder.result->program, error, capacity)) {
        builder.failed = 1;
    }
    if (builder.failed) {
        tac_build_free(builder.result);
        return NULL;
    }
    return builder.result;
}

/* tac_build_print — véase el contrato en tac_build.h. */
void tac_build_print(FILE *out, const TacBuildResult *result) {
    fprintf(out, "initial_data bytes=%zu\n", result->initial_data_size);
    for (size_t offset = 0; offset < result->initial_data_size; offset += 4) {
        fprintf(out, "  @%08zx = %08" PRIx32 "\n", offset, result->initial_data[offset / 4]);
    }
    fputc('\n', out);
    tac_print(out, result->program);
}

/* tac_build_free — véase el contrato en tac_build.h. */
void tac_build_free(TacBuildResult *result) {
    if (!result) {
        return;
    }
    tac_program_free(result->program);
    Symbol *symbol = result->auxiliary_symbols;
    while (symbol) {
        Symbol *next = symbol->next;
        free(symbol->name);
        free(symbol);
        symbol = next;
    }
    free(result->initial_data);
    free(result);
}
