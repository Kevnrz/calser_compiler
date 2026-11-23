/* Importación mínima de módulos: docs/diseno_ccom.md §12. */
#ifndef MODULES_H
#define MODULES_H
#include "ast.h"
#include "diagnostics.h"
/* Devuelve un AST propio o NULL ante errores. Liberar con ast_free antes de
 * modules_free. Solo el principal puede importar; module_dir puede ser NULL. */
ASTNode *modules_load(const char *main_path, const char *module_dir);
void modules_free(void);
/* Contexto del parser para diagnosticar un P fuera del prefijo de un módulo. */
int modules_in_module(void);
/* §12: rutas/nombres prestados hasta modules_free; NULL si no está registrado.
 * La posición corresponde al primer P del principal que incluyó ese módulo. */
const char *modules_file_name(int file);
const char *modules_import_name(int file);
int modules_import_position(int file, int *line, int *column);
/* §12: rango completo P..; en bytes del principal; located=0 si no hay P. */
DiagPosition modules_import_range(int file);
#endif
