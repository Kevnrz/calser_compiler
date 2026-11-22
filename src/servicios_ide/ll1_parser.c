/* Máquina predictiva con pila explícita para la gramática auditada de Calser.
 * PRIMERO y SIGUIENTE se resuelven por punto fijo. Cada celda de la tabla
 * debe seleccionar una sola producción. Flex entrega tokens, posiciones y
 * errores léxicos mediante una visita que no construye árboles sintácticos.
 */
#include <string.h>
#include "ll1_parser.h"
#include "lexer_service.h"
#include "diagnostics.h"
#include "ast.h"
#include "parser.tab.h"
#include "ll1_grammar.inc"

_Static_assert(LL1_TERMINALS <= 64, "Los conjuntos deben caber en uint64_t");

typedef struct { uint64_t bits; int nullable; } First;
static uint64_t first[LL1_NONTERMINALS], follow[LL1_NONTERMINALS];
static unsigned char nullable[LL1_NONTERMINALS];
static short table[LL1_NONTERMINALS][LL1_TERMINALS];
static LL1GrammarInfo grammar_info;
static int grammar_ready;

/* Devuelve PRIMERO de una secuencia, sin confundir epsilon con EOF. */
static First first_sequence(const unsigned short *symbols, size_t count) {
    First result = {0, 1};
    for (size_t i = 0; i < count; i++) {
        unsigned s = symbols[i];
        if (s < LL1_TERMINALS) {
            result.bits |= UINT64_C(1) << s;
            result.nullable = 0;
            break;
        }
        unsigned n = s - LL1_TERMINALS;
        result.bits |= first[n];
        if (!nullable[n]) { result.nullable = 0; break; }
    }
    return result;
}

static void prepare_grammar(void) {
    if (grammar_ready) return;
    grammar_info.terminals = LL1_TERMINALS - 1;
    grammar_info.nonterminals = LL1_NONTERMINALS;
    grammar_info.productions = LL1_PRODUCTIONS;
    int changed;
    do {
        changed = 0;
        for (size_t i = 0; i < LL1_PRODUCTIONS; i++) {
            const Production *p = &productions[i];
            unsigned n = p->head - LL1_TERMINALS;
            First value = first_sequence(p->rhs, p->count);
            if ((value.bits & ~first[n]) || (value.nullable && !nullable[n])) changed = 1;
            first[n] |= value.bits;
            nullable[n] |= value.nullable;
        }
    } while (changed);
    follow[LL1_START - LL1_TERMINALS] = UINT64_C(1);
    do {
        changed = 0;
        for (size_t i = 0; i < LL1_PRODUCTIONS; i++) {
            const Production *p = &productions[i];
            for (size_t j = 0; j < p->count; j++) {
                if (p->rhs[j] < LL1_TERMINALS) continue;
                unsigned n = p->rhs[j] - LL1_TERMINALS;
                First suffix = first_sequence(p->rhs + j + 1, p->count - j - 1);
                uint64_t bits = suffix.bits;
                if (suffix.nullable) bits |= follow[p->head - LL1_TERMINALS];
                if (bits & ~follow[n]) changed = 1;
                follow[n] |= bits;
            }
        }
    } while (changed);
    for (size_t n = 0; n < LL1_NONTERMINALS; n++) {
        grammar_info.nullable += nullable[n] != 0;
        for (size_t t = 0; t < LL1_TERMINALS; t++) table[n][t] = -1;
    }
    for (size_t i = 0; i < LL1_PRODUCTIONS; i++) {
        const Production *p = &productions[i];
        unsigned n = p->head - LL1_TERMINALS;
        First value = first_sequence(p->rhs, p->count);
        uint64_t bits = value.bits | (value.nullable ? follow[n] : 0);
        for (size_t t = 0; t < LL1_TERMINALS; t++) {
            if (!(bits & (UINT64_C(1) << t))) continue;
            if (table[n][t] >= 0) grammar_info.conflicts++;
            else { table[n][t] = (short)i; grammar_info.cells++; }
        }
    }
    grammar_ready = 1;
}

int ll1_grammar_info(LL1GrammarInfo *info) {
    prepare_grammar();
    if (info) *info = grammar_info;
    return grammar_info.conflicts == 0;
}

const char *ll1_terminal_name(size_t index) {
    return index < LL1_TERMINALS ? names[index] : NULL;
}

int ll1_expects(const LL1Result *result, const char *terminal) {
    if (!result || !terminal) return 0;
    for (size_t i = 0; i < LL1_TERMINALS; i++)
        if (!strcmp(terminal, names[i])) return (result->expected & (UINT64_C(1) << i)) != 0;
    return 0;
}

const char *ll1_status_name(LL1Status status) {
    switch (status) {
        case LL1_ACCEPTED: return "accepted";
        case LL1_SYNTAX_ERROR: return "syntax_error";
        case LL1_LEXICAL_ERROR: return "lexical_error";
        case LL1_LIMIT: return "limit";
        case LL1_CANCELLED: return "cancelled";
        case LL1_INTERNAL_ERROR: return "internal_error";
        case LL1_PREFIX_VALID: return "prefix_valid";
        case LL1_IN_COMMENT: return "comment";
        case LL1_INVALID_CONTEXT: return "invalid_context";
    }
    return "internal_error";
}

LL1Limits ll1_default_limits(void) {
    return (LL1Limits){LL1_SOURCE_MAX, LL1_TOKEN_MAX, LL1_STEP_MAX, LL1_STACK_MAX};
}

typedef struct {
    unsigned short stack[LL1_STACK_MAX];
    size_t count, length;
    const char *text;
    LL1Limits limits;
    LL1Result result;
    LL1Cancel cancel;
    void *cancel_data;
    int stopped, prefix;
    size_t cursor, point, comment_start;
    LL1Comment comment;
} Machine;

/* Flex tiene un único observador por proceso. Se instala solo durante la visita
 * y nunca se conserva la dirección de una máquina después de la consulta. */
static Machine *context_machine;

/* La fila de un no terminal usa SIGUIENTE global. Para describir el error se
 * calcula en cambio PRIMERO de toda la continuación concreta de la pila.
 */
static uint64_t expected(const Machine *m) {
    uint64_t bits = 0;
    for (size_t i = m->count; i > 0; i--) {
        unsigned s = m->stack[i - 1];
        if (s < LL1_TERMINALS) return bits | (UINT64_C(1) << s);
        bits |= first[s - LL1_TERMINALS];
        if (!nullable[s - LL1_TERMINALS]) break;
    }
    return bits;
}

static int stop(Machine *m, LL1Status status, LL1Limit limit) {
    m->result.status = status;
    m->result.limit = limit;
    m->result.expected = expected(m);
    m->stopped = 1;
    return 0;
}

static int interrupted(Machine *m) {
    if (m->cancel && m->cancel(m->cancel_data)) {
        stop(m, LL1_CANCELLED, LL1_NO_LIMIT);
        return 1;
    }
    return 0;
}

/* Flex ya determinó el error y su posición. Solo se convierte línea/columna
 * a un rango del texto original. Esto incluye el inicio de un comentario
 * sin cerrar, aunque Flex lo haya descubierto al final del documento.
 */
static void locate(Machine *m, size_t offset) {
    m->result.offset = m->result.end = offset;
    m->result.line = m->result.column = 1;
    for (size_t i = 0; i < offset; i++) {
        if (m->text[i] == '\n') { m->result.line++; m->result.column = 1; }
        else m->result.column++;
    }
}

static void lexical_error(int line, int column, size_t width, void *data) {
    Machine *m = data;
    if (m->stopped) return;
    if (m->prefix && lexer_in_block_comment()) return;
    size_t offset = 0;
    int current_line = 1;
    while (offset < m->length && current_line < line)
        if (m->text[offset++] == '\n') current_line++;
    if (column > 0 && (size_t)(column - 1) <= m->length - offset)
        offset += (size_t)(column - 1);
    if (m->prefix && m->point < m->cursor && offset >= m->point) return;
    m->result.lexical_errors++;
    m->result.offset = offset;
    m->result.end = width <= m->length - offset ? offset + width : m->length;
    m->result.line = line;
    m->result.column = column;
    m->result.unexpected = -1;
    stop(m, LL1_LEXICAL_ERROR, LL1_NO_LIMIT);
}

/* Devuelve uno para pedir otro token. EOF se procesa como terminal real de
 * la máquina, no como un prefijo aceptado. Solo pila vacía con EOF acepta.
 */
static int consume(int token, size_t start, size_t end, int line, int column, void *data) {
    Machine *m = data;
    if (m->stopped) return 0;
    if (m->prefix && token && end > m->point) {
        if (interrupted(m)) return 0;
        if (start < m->point) {
            locate(m, m->point);
            stop(m, LL1_INVALID_CONTEXT, LL1_NO_LIMIT);
            m->result.expected = 0;
            return 0;
        }
        return 1;
    }
    m->result.offset = start;
    m->result.end = end;
    m->result.line = line;
    m->result.column = column;
    m->result.unexpected = token;
    if (interrupted(m)) return 0;
    if (!token && m->prefix) {
        locate(m, m->point);
        return stop(m, LL1_PREFIX_VALID, LL1_NO_LIMIT);
    }
    if (token && m->result.consumed >= m->limits.tokens)
        return stop(m, LL1_LIMIT, LL1_TOKEN_LIMIT);
    size_t terminal = 0;
    while (terminal < LL1_TERMINALS && flex_tokens[terminal] != token) terminal++;
    if (terminal == LL1_TERMINALS) return stop(m, LL1_INTERNAL_ERROR, LL1_NO_LIMIT);
    while (m->count) {
        if (interrupted(m)) return 0;
        if (m->result.steps >= m->limits.steps) return stop(m, LL1_LIMIT, LL1_STEP_LIMIT);
        m->result.steps++;
        unsigned top = m->stack[m->count - 1];
        if (top < LL1_TERMINALS) {
            if (top != terminal) return stop(m, LL1_SYNTAX_ERROR, LL1_NO_LIMIT);
            m->count--;
            if (token) {
                m->result.consumed++;
                m->result.expected = expected(m);
                return 1;
            }
            return stop(m, m->count ? LL1_INTERNAL_ERROR : LL1_ACCEPTED, LL1_NO_LIMIT);
        }
        int selected = table[top - LL1_TERMINALS][terminal];
        if (selected < 0) return stop(m, LL1_SYNTAX_ERROR, LL1_NO_LIMIT);
        const Production *p = &productions[selected];
        if (m->count - 1 + p->count > m->limits.stack)
            return stop(m, LL1_LIMIT, LL1_STACK_LIMIT);
        m->count--;
        for (size_t i = p->count; i > 0; i--) m->stack[m->count++] = p->rhs[i - 1];
    }
    return stop(m, LL1_INTERNAL_ERROR, LL1_NO_LIMIT);
}

/* Una frontera UTF-8 válida no apunta al interior de un carácter multibyte.
 * La validación léxica de los bytes restantes sigue perteneciendo a Flex.
 */
static int byte_boundary(const char *text, size_t length, size_t offset) {
    return offset == length || (((unsigned char)text[offset] & 0xc0) != 0x80);
}

/* El fragmento explícito solo indica una palabra, número u operador parcial.
 * No se interpreta como programa ni se permiten sentencias completas aquí.
 */
static int valid_fragment(const char *text, size_t point, size_t cursor) {
    if (point == cursor) return 1;
    unsigned char first_char = (unsigned char)text[point];
    int word = (first_char >= 'a' && first_char <= 'z') ||
               (first_char >= 'A' && first_char <= 'Z') || first_char == '_';
    int number = first_char >= '0' && first_char <= '9';
    if (!word && !number) {
        if (cursor - point > 2) return 0;
        for (size_t i = point; i < cursor; i++)
            if (!strchr("=!<>", text[i]) || !text[i]) return 0;
        return 1;
    }
    for (size_t i = point; i < cursor; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c >= '0' && c <= '9') continue;
        if (word && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')) continue;
        return 0;
    }
    return 1;
}

/* Flex clasifica el comentario. La máquina solo recuerda si su intervalo
 * alcanza el cursor y continúa abierto en ese punto de la consulta.
 */
static void observe_context(const char *kind, size_t start, size_t end) {
    if (!context_machine || strcmp(kind, "comentario") ||
        end != context_machine->cursor || !lexer_comment_open()) return;
    context_machine->comment = lexer_in_block_comment() ? LL1_BLOCK_COMMENT : LL1_LINE_COMMENT;
    context_machine->comment_start = start;
}

static LL1Context finish(const Machine *m) {
    LL1Context context = {0};
    context.analysis = m->result;
    context.cursor = m->cursor;
    context.point = m->point;
    if (m->result.status == LL1_PREFIX_VALID && m->comment != LL1_NO_COMMENT) {
        context.analysis.status = LL1_IN_COMMENT;
        context.analysis.expected = 0;
        context.comment = m->comment;
        context.comment_start = m->comment_start;
    }
    context.can_end = context.analysis.status == LL1_PREFIX_VALID &&
                      (context.analysis.expected & UINT64_C(1)) != 0;
    return context;
}

static LL1Context execute(const char *text, size_t length, size_t cursor, size_t point,
                           int prefix, const LL1Limits *limits, LL1Cancel cancel, void *data) {
    Machine m = {0};
    m.result.status = LL1_INTERNAL_ERROR;
    m.result.line = m.result.column = 1;
    m.limits = limits ? *limits : ll1_default_limits();
    m.prefix = prefix;
    m.cursor = cursor;
    m.point = point;
    if ((!text && length) || !m.limits.source || m.limits.source > LL1_SOURCE_MAX ||
        !m.limits.tokens || m.limits.tokens > LL1_TOKEN_MAX ||
        !m.limits.steps || m.limits.steps > LL1_STEP_MAX ||
        m.limits.stack < 2 || m.limits.stack > LL1_STACK_MAX || !ll1_grammar_info(NULL))
        return finish(&m);
    m.stack[0] = 0;
    m.stack[1] = LL1_START;
    m.count = 2;
    m.text = text ? text : "";
    m.length = length;
    m.cancel = cancel;
    m.cancel_data = data;
    m.result.expected = expected(&m);
    if (length > m.limits.source) { stop(&m, LL1_LIMIT, LL1_SOURCE_LIMIT); return finish(&m); }
    if (prefix && (cursor > length || point > cursor ||
        !byte_boundary(m.text, length, cursor) || !byte_boundary(m.text, length, point) ||
        !valid_fragment(m.text, point, cursor))) {
        stop(&m, LL1_INVALID_CONTEXT, LL1_NO_LIMIT);
        m.result.expected = 0;
        return finish(&m);
    }
    if (interrupted(&m)) return finish(&m);
    diag_init(NULL);
    diag_set_observer(NULL, NULL, 0);
    context_machine = prefix ? &m : NULL;
    lexer_set_observer(prefix ? observe_context : NULL);
    int scanned = lexer_visit_text(m.text, prefix ? cursor : length, consume, lexical_error, &m);
    lexer_set_observer(NULL);
    context_machine = NULL;
    if (scanned < 0 || !m.stopped) stop(&m, LL1_INTERNAL_ERROR, LL1_NO_LIMIT);
    diag_set_observer(NULL, NULL, 1);
    diag_free();
    return finish(&m);
}

LL1Result ll1_analyze(const char *text, size_t length, const LL1Limits *limits,
                      LL1Cancel cancel, void *data) {
    return execute(text, length, length, length, 0, limits, cancel, data).analysis;
}

LL1Context ll1_context(const char *text, size_t length, size_t cursor, size_t point,
                       const LL1Limits *limits, LL1Cancel cancel, void *data) {
    return execute(text, length, cursor, point, 1, limits, cancel, data);
}
