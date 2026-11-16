/*
 * semantic_crypto.c — Fase 3: verificación de las operaciones de cifrado.
 *
 * Valida las siete operaciones CRYPTO (palabras reservadas con regla
 * gramatical propia): cantidad y tipo de argumentos, selectores k y w
 * literales de 0 a 3, y las direcciones constantes de origen y destino, que
 * deben estar alineadas y dentro de la zona de datos de CRYPTO. Las
 * direcciones no pasan por la tabla de símbolos.
 *
 * Usa: semantic_internal.h.
 * Lo usa: semantic.c, al analizar una sentencia de cifrado.
 * Referencias: docs/crypto_para_ccom.md §6; isa.md §6.4.
 */

#include <stdio.h>
#include <string.h>

#include "semantic_internal.h"

// Zona de datos de la memoria y tamaño de un bloque de cifrado, en bytes.
#define DATA_ZONE_FIRST    4096
#define DATA_ZONE_LAST     65535
#define CRYPTO_BLOCK_BYTES 8

/* ==========================================================================
 * OPERACIONES DE CIFRADO
 * ======================================================================= */


/*
 * check_crypto_argument — exige tipo int en un argumento de cifrado.
 *
 * Entrada:
 *   argument  expresión argumento.
 *   position  posición desde 1.
 *   operation  nombre de operación no NULL.
 * Salida:
 *   tipo calculado, incluso si no es int.
 *   Efectos: verifica expresión y reporta incompatibilidad.
 * Restricciones:
 *   ninguna.
 */
static ExprType check_crypto_argument(ASTNode *argument, int position,
                                      const char *operation) {
    ExprType type = semantic_check_expression(argument);

    if (type.kind != ET_ERROR &&
        type.kind != ET_INT) {

        char received_text[TYPE_TEXT_SIZE];

        semantic_error(argument,
                       "argumento %d de '%s': se esperaba int, se recibio %s",
                       position,
                       operation,
                       semantic_type_text(type, received_text));
    }

    return type;
}

/*
 * check_crypto_selector — comprueba un selector literal de cifrado.
 *
 * Entrada:
 *   number  nodo NUMBER o NULL.
 *   name  nombre k o w.
 *   operation  nombre de operación.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta valores fuera de 0–3.
 * Restricciones:
 *   la gramática garantiza la forma NUMBER; no evalúa expresiones.
 */
static void check_crypto_selector(ASTNode *number, const char *name,
                                  const char *operation) {
    if (number == NULL || number->kind != AST_NUMBER)
        return;

    if (number->number < 0 || number->number > 3)
        semantic_error(number,
                       "el argumento '%s' de '%s' debe estar entre 0 y 3",
                       name,
                       operation);
}

/*
 * check_crypto_address — verifica zona y alineación de una dirección constante.
 *
 * Entrada:
 *   argument  expresión para diagnóstico.
 *   type  tipo ya calculado.
 *   position  posición desde 1.
 *   operation  nombre de operación.
 * Salida:
 *   1 si la constante int está en zona y alineada; 0 en otro caso.
 *   Efectos: reporta una vez zona o alineación incorrecta.
 * Restricciones:
 *   solo verifica constantes; zona 4096–65535, múltiplos de 4.
 */
static int check_crypto_address(ASTNode *argument, ExprType type,
                                int position, const char *operation) {
    if (type.kind != ET_INT || !type.is_const)
        return 0;

    if (type.value < DATA_ZONE_FIRST || type.value > DATA_ZONE_LAST) {

        semantic_error(argument,
                       "argumento %d de '%s': la direccion %ld esta fuera "
                       "de la zona de datos [%d..%d]",
                       position,
                       operation,
                       type.value,
                       DATA_ZONE_FIRST,
                       DATA_ZONE_LAST);

        return 0;
    }

    if (type.value % 4 != 0) {

        semantic_error(argument,
                       "argumento %d de '%s': la direccion %ld no es "
                       "multiplo de 4",
                       position,
                       operation,
                       type.value);

        return 0;
    }

    return 1;
}

/*
 * check_crypto_extent — comprueba que los bloques caben desde una dirección.
 *
 * Entrada:
 *   blocks  expresión de bloques.
 *   blocks_type  tipo constante de bloques.
 *   address  tipo constante de dirección validada.
 *   operation  nombre de operación.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta si dirección + 8*bloques excede 65536.
 * Restricciones:
 *   bloques constante positivo; dirección ya verificada.
 */
static void check_crypto_extent(ASTNode *blocks, ExprType blocks_type,
                                ExprType address, const char *operation) {
    long long end =
        (long long) address.value +
        (long long) CRYPTO_BLOCK_BYTES * blocks_type.value;

    if (end > DATA_ZONE_LAST + 1)
        semantic_error(blocks,
                       "argumento 3 de '%s': %ld bloques desde la direccion "
                       "%ld se salen de la memoria",
                       operation,
                       blocks_type.value,
                       address.value);
}

/* semantic_analyze_crypto — véase el contrato en semantic_internal.h. */
void semantic_analyze_crypto(ASTNode *node) {
    const char *operation = node->text;

    ASTNode *arguments = ast_find_child(node, AST_ARGUMENTS);
    ASTNode *argument = arguments ? arguments->child : NULL;

    // cerrar_sesion, sellar: sin argumentos.
    if (argument == NULL)
        return;

    // cargar_llave [ k , w , valor ]
    if (strcmp(operation, "cargar_llave") == 0) {

        ASTNode *w = argument->next;

        check_crypto_selector(argument, "k", operation);
        check_crypto_selector(w, "w", operation);

        if (w != NULL && w->next != NULL)
            check_crypto_argument(w->next, 3, operation);

        return;
    }

    // autenticar, cambiar_ancla: una sola expr.
    if (argument->next == NULL) {
        check_crypto_argument(argument, 1, operation);
        return;
    }

    // cifrar, descifrar [ origen , destino , bloques , k ]
    ASTNode *origin = argument;
    ASTNode *destination = origin->next;
    ASTNode *blocks = destination->next;

    if (blocks == NULL)
        return;

    ExprType origin_type = check_crypto_argument(origin, 1, operation);
    int origin_valid =
        check_crypto_address(origin, origin_type, 1, operation);

    ExprType destination_type =
        check_crypto_argument(destination, 2, operation);
    int destination_valid =
        check_crypto_address(destination, destination_type, 2, operation);

    ExprType blocks_type = check_crypto_argument(blocks, 3, operation);

    if (blocks_type.kind == ET_INT &&
        blocks_type.is_const &&
        blocks_type.value > 0) {

        if (origin_valid)
            check_crypto_extent(blocks, blocks_type, origin_type, operation);

        if (destination_valid)
            check_crypto_extent(blocks, blocks_type, destination_type,
                                operation);
    }

    check_crypto_selector(blocks->next, "k", operation);
}
