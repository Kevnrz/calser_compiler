/* Reserva privada de vectores de análisis. El contrato de memoria es el de TAC. */
#ifndef TAC_ANALYSIS_INTERNAL_H
#define TAC_ANALYSIS_INTERNAL_H
#include "tac_internal.h"
#include <stdlib.h>
/* Entrada: cantidad/ancho. Salida: vector propio cero. Efectos: reserva o termina.
 * Restricciones: controla el producto; un vector vacío sigue siendo liberable.
 */
static inline void *tac_analysis_array(size_t count, size_t width) {
    if (width && count > SIZE_MAX / width) {
        fputs("tac: análisis demasiado grande\n", stderr);
        exit(EXIT_FAILURE);
    }
    return tac_allocate(count && width ? count * width : 1);
}
#endif
