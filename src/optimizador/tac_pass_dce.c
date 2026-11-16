/* Pasada dce: HIGH; estado observable y fallos, §7.2; contrato común §7.1, Aho §9.1.
 * Entrada: contexto y buffer de detalle. Salida: 1 sin transformar por ahora.
 * Efectos: ninguno; la optimización corresponde a C3. No imprime diagnósticos.
 */
#include "tac_passes.h"

int tac_pass_dce(TacPassContext *context, char *detail, size_t capacity) {
    (void)context;
    (void)detail;
    (void)capacity;
    return 1;
}
