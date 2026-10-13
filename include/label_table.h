#ifndef LABEL_TABLE_H
#define LABEL_TABLE_H

#include "ast.h"

typedef enum {
    LABEL_FUNCTION,

    /* Conditional labels */
    LABEL_NEXT,
    LABEL_END,

    /* Loop labels */
    LABEL_START,
    LABEL_UPDATE

} LabelRole;

typedef struct LabelEntry {
    ASTNode *node;
    LabelRole role;
    char *name;

    struct LabelEntry *next;
} LabelEntry;

typedef struct {
    LabelEntry *entries;
    unsigned int next_id;
} LabelTable;

void label_table_init(LabelTable *table);

int label_table_build(LabelTable *table, ASTNode *root);

const char *label_table_get(
    const LabelTable *table,
    const ASTNode *node,
    LabelRole role
);

void label_table_print(const LabelTable *table);

void label_table_destroy(LabelTable *table);

#endif