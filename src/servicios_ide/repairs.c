/* Motor conservador de reparaciones: primer error, inserciones de cierres LAC
 * y reanálisis completo de cada copia. No inventa operandos, borra código,
 * ejecuta semántica ni escribe fuentes. Memoria y estado se reinician por copia.
 * main.c llama repairs_run; syntax_service aporta expectativas y reanálisis.
 * Los índices de propuestas/precondiciones son UTF-8 del original; reached es
 * de la copia con un byte añadido. Solo son estáticos los catálogos inmutables;
 * cada buffer candidato se libera antes del siguiente. syntax_service usa un
 * contexto global, así que todo el recorrido es serial dentro del proceso.
 * automatic_safe informa una condición C; la UI todavía exige regla y permiso. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repairs.h"
#include "syntax_service.h"
#include "diagnostics_json.h"

typedef struct { int kind, accepted, automatic; size_t position, reached; } Repair;
static const char delimiters[] = ";.])";
static const char *codes[] = {"INSERCION_PUNTO_COMA", "INSERCION_PUNTO_FINAL", "INSERCION_CORCHETE_CIERRE", "INSERCION_PARENTESIS_CIERRE"};
static const char *descriptions[] = {"Insertar punto y coma", "Insertar punto final", "Cerrar corchete", "Cerrar parentesis"};
static int compare(const void *a, const void *b) {
    const Repair *x = a, *y = b;
    if (x->accepted != y->accepted) return y->accepted - x->accepted;
    if (x->reached != y->reached) return x->reached > y->reached ? -1 : 1;
    return x->kind - y->kind;
}
static int emit(FILE *out, const char *source, const char *text, SyntaxAnalysis base,
                Repair *repairs, int count, const char *status, const char *explanation) {
    int code = !strcmp(status, "limite") || !strcmp(status, "error");
    fputs("{\"schema_version\":1,\"operation\":\"repair\",\"offset_unit\":\"utf8\",\"source\":", out);
    diag_json_write_string(out, source);
    fprintf(out, ",\"exit_code\":%d,\"status\":", code); diag_json_write_string(out, status);
    fputs(",\"explanation\":", out); diag_json_write_string(out, explanation);
    fputs(",\"repairs\":[", out);
    for (int i = 0; i < count; i++) {
        Repair r = repairs[i];
        if (i) fputc(',', out);
        fputs("{\"code\":", out); diag_json_write_string(out, codes[r.kind]);
        fputs(",\"description\":", out); diag_json_write_string(out, descriptions[r.kind]);
        fprintf(out, ",\"automatic_safe\":%s,\"edits\":[{\"start\":%zu,\"end\":%zu,\"text\":\"%c\"}],",
                r.automatic ? "true" : "false", r.position, r.position, delimiters[r.kind]);
        fprintf(out, "\"preconditions\":[{\"start\":%zu,\"end\":%zu,\"expected\":", base.previous_start, base.previous_end);
        diag_json_write_bytes(out, text + base.previous_start, base.previous_end - base.previous_start);
        fputs("}],\"reasons\":[", out);
        diag_json_write_string(out, r.accepted ? "La copia completa es aceptada por el parser C." : "La copia supera el token del primer error; quedan errores posteriores.");
        fputc(',', out);
        diag_json_write_string(out, r.automatic ? "Unico punto y coma con aceptacion sintactica completa, sin errores lexicos." : "Requiere revision manual; no garantiza intencion ni semantica.");
        fprintf(out, "],\"validation\":{\"accepted\":%s,\"resolved\":true,\"reached\":%zu,\"target\":%zu,\"description\":",
                r.accepted ? "true" : "false", r.reached, base.offset);
        diag_json_write_string(out, r.accepted ? "Bison/Flex: aceptacion sintactica completa de la copia; semantica no comprobada." : "Bison/Flex: primer error superado; aceptacion sintactica parcial, solo manual.");
        fputs("}}", out);
    }
    fputs("]}\n", out);
    return fflush(out) == 0 && !ferror(out) ? code : 1;
}
int repairs_run(FILE *out, const char *source) {
    size_t length; int limited;
    char *text = syntax_read_source(source, &length, &limited);
    SyntaxAnalysis base = {0}; Repair options[4]; int count = 0;
    if (!text) return emit(out, source, NULL, base, options, 0, limited ? "limite" : "error", "Fuente no disponible o superior a 200000 bytes.");
    base = syntax_analyze(text, length);
    const char *status = NULL, *explanation = NULL;
    if (base.limited) { status = "limite"; explanation = "La consulta supera 50000 tokens."; }
    else if (base.failed) { status = "error"; explanation = "El parser no pudo completar la consulta."; }
    else if (base.lexical) { status = "bloqueado"; explanation = "No se proponen reparaciones sobre errores lexicos."; }
    else if (base.accepted) { status = "sin_opciones"; explanation = "El programa ya es sintacticamente valido; no se repara semantica."; }
    else {
        for (int i = 0; i < 4; i++) {
            if (!(base.closers & (1u << i)) || !base.previous_end) continue;
            size_t position = base.previous_end;
            char *candidate = malloc(length + 2);
            if (!candidate) { status = "error"; explanation = "Sin memoria para validar una copia."; break; }
            memcpy(candidate, text, position);
            candidate[position] = delimiters[i];
            memcpy(candidate + position + 1, text + position, length - position);
            candidate[length + 1] = 0;
            SyntaxAnalysis check = syntax_analyze(candidate, length + 1);
            free(candidate);
            if (check.limited || check.failed) {
                status = check.limited ? "limite" : "error";
                explanation = "No se completaron todas las validaciones; no se ofrecen propuestas parciales.";
                break;
            }
            int resolved = check.accepted || (base.eof ? check.offset >= length + 1 : check.offset > base.offset + 1);
            if (!check.lexical && resolved)
                options[count++] = (Repair) {i, check.accepted, 0, position, check.offset};
        }
        if (!status) {
            qsort(options, count, sizeof(options[0]), compare);
            /* La copia completa y un único ; justifican esta candidatura,
             * incluso antes de una asignación cuyo inicio sea identificador.
             * La UI todavía requiere permiso y valida la línea confirmada. */
            if (count == 1 && options[0].kind == 0 && options[0].accepted && !base.eof)
                options[0].automatic = 1;
            status = count ? "ok" : "sin_opciones";
            explanation = count ? "" : "No hay una insercion de cierre que supere el primer error sin inventar codigo.";
        } else count = 0;
    }
    int code = emit(out, source, text, base, options, count, status, explanation);
    free(text);
    return code;
}
