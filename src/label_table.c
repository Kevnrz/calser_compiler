#include <stdlib.h>
#include <string.h>

#include "label_table.h"

static char *copy_string(const char *text) {
    size_t length = strlen(text) + 1;

    char *copy = malloc(length);

    if (copy == NULL)
        return NULL;

    memcpy(copy, text, length);

    return copy;
}

void label_table_init(LabelTable *table) {
    if (table == NULL)
        return;

    table->entries = NULL;
    table->next_id = 0;
}

static int label_table_add(LabelTable *table, ASTNode *node, LabelRole role, const char *prefix) {
    char buffer[128];

    unsigned int id = table->next_id;

    int length = snprintf(
        buffer,
        sizeof(buffer),
        ".L_%s_%u",
        prefix,
        id
    );

    if (length < 0 || (size_t)length >= sizeof(buffer))
        return -1;

    char *name = copy_string(buffer);

    if (name == NULL)
        return -1;

    LabelEntry *entry = malloc(sizeof(LabelEntry));

    if (entry == NULL) {
        free(name);
        return -1;
    }

    entry->node = node;
    entry->role = role;
    entry->name = name;

    entry->next = table->entries;
    table->entries = entry;

    table->next_id++;

    return 0;
}

static int label_node(LabelTable *table, ASTNode *node) {
    switch (node->kind) {
        case AST_FUNCTION:
            return label_table_add(table, node, LABEL_FUNCTION, "func");
        case AST_CONDITIONAL:
            if (label_table_add(table, node, LABEL_NEXT, "if_next") != 0)
                return -1;

            return label_table_add(table, node, LABEL_END, "if_end");
        case AST_ALTERNATIVE:
            return label_table_add(table, node, LABEL_NEXT, "elif_next");
        case AST_LOOP_L:
            if (label_table_add(table, node, LABEL_START, "while_start") != 0)
                return -1;

            return label_table_add(table, node, LABEL_END, "while_end");
        case AST_LOOP_S:
            if (label_table_add(table, node, LABEL_START, "for_start") != 0)
                return -1;

            if (label_table_add(table, node, LABEL_UPDATE, "for_update") != 0)
                return -1;

            return label_table_add(table, node, LABEL_END, "for_end");
        default:
            return 0;
    }
}

static int label_traverse(LabelTable *table, ASTNode *node) {
    while (node != NULL) {

        if (label_node(table, node) != 0)
            return -1;

        if (label_traverse(table, node->child) != 0)
            return -1;

        node = node->next;
    }

    return 0;
}

int label_table_build(LabelTable *table, ASTNode *root) {
    if (table == NULL)
        return -1;

    if (table->entries != NULL)
        return -1;

    table->next_id = 0;

    return label_traverse(table, root);
}

const char *label_table_get(const LabelTable *table, const ASTNode *node, LabelRole role) {
    if (table == NULL || node == NULL)
        return NULL;

    const LabelEntry *entry = table->entries;

    while (entry != NULL) {

        if (entry->node == node && entry->role == role)
            return entry->name;

        entry = entry->next;
    }

    return NULL;
}

void label_table_destroy(LabelTable *table) {
    if (table == NULL)
        return;

    LabelEntry *entry = table->entries;

    while (entry != NULL) {

        LabelEntry *next = entry->next;

        free(entry->name);
        free(entry);

        entry = next;
    }

    table->entries = NULL;
    table->next_id = 0;
}

static const char *label_role_name(LabelRole role) {
    switch (role) {
        case LABEL_FUNCTION: return "FUNCTION";
        case LABEL_START:    return "START";
        case LABEL_NEXT:     return "NEXT";
        case LABEL_END:      return "END";
        case LABEL_UPDATE:   return "UPDATE";
        default:             return "UNKNOWN";
    }
}

void label_table_print(const LabelTable *table) {
    if (table == NULL)
        return;

    printf("\n===== LABEL TABLE =====\n");

    const LabelEntry *entry = table->entries;

    while (entry != NULL) {
        printf(
            "%-10s | %-12s | AST kind: %d | Line: %d\n",
            entry->name,
            label_role_name(entry->role),
            (int)entry->node->kind,
            entry->node->line
        );

        entry = entry->next;
    }

    printf("=======================\n");
}