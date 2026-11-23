/* Importación mínima, diseño §12. Un solo semántico/generador recibe la unión.
 * Para admitir importaciones anidadas, load_one podrá recorrer los P de cada
 * archivo y consultar una lista compartida de archivos incluidos/ciclos. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modules.h"
#include "parser_service.h"
#include "lexer_service.h"
#include "diagnostics.h"
#include "diagnostics_json.h"

/* §12: índices estables compartidos con ASTNode.file. */
typedef struct {
    char *path, *name;
    int line, column;
    DiagPosition range;
} ModuleFile;
static ModuleFile *files;
static int file_count;
static int parsing_module;
int modules_in_module(void) { return parsing_module; }

static char *copy(const char *text) {
    char *result = strdup(text);
    if (!result) { fprintf(stderr, "ccom: memoria insuficiente\n"); exit(EXIT_FAILURE); }
    return result;
}

static int register_file(const char *path, const ASTNode *import) {
    ModuleFile *next = realloc(files, (size_t)(file_count + 1) * sizeof(*files));
    if (!next) { fprintf(stderr, "ccom: memoria insuficiente\n"); exit(EXIT_FAILURE); }
    files = next;
    files[file_count] = (ModuleFile) {copy(path), import ? copy(import->text) : NULL,
                                     import ? import->line : 0, import ? import->column : 0,
                                     import ? diag_range(import->line, import->column,
                                                         import->import_end_line, import->import_end_column)
                                            : (DiagPosition) {0}};
    return file_count++;
}

const char *modules_file_name(int file) {
    return file >= 0 && file < file_count ? files[file].path : NULL;
}
const char *modules_import_name(int file) {
    return file > 0 && file < file_count ? files[file].name : NULL;
}
int modules_import_position(int file, int *line, int *column) {
    if (file <= 0 || file >= file_count) return 0;
    if (line) *line = files[file].line;
    if (column) *column = files[file].column;
    return 1;
}

DiagPosition modules_import_range(int file) {
    return file > 0 && file < file_count ? files[file].range : (DiagPosition) {0};
}

/* §12: seleccionar el archivo nunca reinicia el contador del análisis. */
static void select_file(int file) { diag_select_file(file); }

void modules_free(void) {
    diag_free();
    for (int i = 0; i < file_count; i++) {
        free(files[i].path);
        free(files[i].name);
    }
    free(files);
    files = NULL;
    file_count = parsing_module = 0;
    ast_set_current_file(0);
}

/* Inserta después de los módulos anteriores y antes de las declaraciones
 * propias. tail apunta al enlace donde se insertará el próximo módulo. */
static void merge_list(ASTNode ***tail, ASTNode *source) {
    ASTNode *items = source->child;
    if (!items) return;
    ASTNode *last = items;
    while (last->next) last = last->next;
    last->next = **tail;
    **tail = items;
    *tail = &last->next;
    source->child = NULL;
}

/* Analiza UN archivo y, si se da destino, transfiere sus declaraciones a él.
 * Reinicia el léxico por archivo. Los módulos no pueden contener ningún P. */
static ASTNode *load_one(int file, ASTNode *destination,
                         ASTNode ***globals_tail, ASTNode ***functions_tail) {
    select_file(file);
    FILE *input = fopen(modules_file_name(file), "r");
    if (!input) { perror("fopen"); return NULL; }
    lexer_begin(input);
    ast_set_current_file(file);
    ASTNode *tree = NULL;
    parsing_module = destination != NULL;
    int accepted = parser_accept(input, &tree);
    parsing_module = 0;
    ast_set_current_file(0);
    fclose(input);
    if (accepted && destination) {
        ASTNode *imports = tree->child->next->next;
        if (imports) {
            diag_error("error sintactico", imports->line, imports->column, 1,
                       "un modulo no puede importar otros modulos");
            accepted = 0;
        }
    }
    if (!accepted) { ast_free(tree); return NULL; }
    if (destination) {
        merge_list(globals_tail, tree->child);
        merge_list(functions_tail, tree->child->next);
        ast_free(tree);
        return destination;
    }
    return tree;
}

ASTNode *modules_load(const char *main_path, const char *module_dir) {
    modules_free();
    register_file(main_path, NULL);
    diag_init(modules_file_name(0));
    ASTNode *main = load_one(0, NULL, NULL, NULL);
    if (!main) return NULL;
    const char *slash = strrchr(main_path, '/');
    char *directory = module_dir ? copy(module_dir) : copy(main_path);
    if (!module_dir) {
        if (!slash) { free(directory); directory = copy("."); }
        else directory[slash - main_path + (slash == main_path)] = '\0';
    }
    ASTNode **globals_tail = &main->child->child;
    ASTNode **functions_tail = &main->child->next->child;
    ASTNode *imports = main->child->next->next;
    for (ASTNode *imp = imports; imp; imp = imp->next) {
        int repeated = 0;
        for (ASTNode *prev = imports; prev != imp; prev = prev->next)
            if (!strcmp(prev->text, imp->text)) { repeated = 1; break; }
        if (repeated) continue;
        size_t length = strlen(directory) + strlen(imp->text) + 6;
        char *path = malloc(length);
        if (!path) { fprintf(stderr, "ccom: memoria insuficiente\n"); exit(EXIT_FAILURE); }
        snprintf(path, length, "%s/%s.cls", directory, imp->text);
        FILE *probe = fopen(path, "r");
        if (!probe) {
            select_file(0);
            /* §12: ampliar solo el rango JSON; conservar el señalador textual. */
            if (diag_json_active())
                diag_error_at("error", diag_range(imp->line, imp->column,
                                                 imp->import_end_line, imp->import_end_column),
                              "modulo no encontrado: '%s' (se busco %s)", imp->text, path);
            else
                diag_error("error", imp->line, imp->column, 1,
                           "modulo no encontrado: '%s' (se busco %s)", imp->text, path);
            free(path); free(directory); ast_free(main);
            return NULL;
        }
        fclose(probe);
        /* §12: calcular el rango contra las líneas/offsets del principal. */
        select_file(0);
        int file = register_file(path, imp);
        int loaded = load_one(file, main, &globals_tail, &functions_tail) != NULL;
        free(path);
        if (!loaded) { free(directory); ast_free(main); return NULL; }
    }
    free(directory);
    /* §12: cada error semántico seleccionará la procedencia de su nodo. */
    if (imports) select_file(0);
    return main;
}
