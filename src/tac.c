/*
 * tac.c — armado y consulta de representación intermedia de tres direcciones.
 * Usa: tac.h y biblioteca C. Lo usan pruebas y futuras pasadas.
 * Referencias: Aho §6.2.2, §8.4; enunciado §4.1.
 */
#include "tac.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define K(k) (1u << (k))
#define N K(TAC_NONE)
#define V (K(TAC_TEMP) | K(TAC_VARIABLE))
#define R (V | K(TAC_INTEGER))
#define L K(TAC_LABEL)
#define S K(TAC_VARIABLE)
#define B(op, text) [op] = {text,TAC_DEST,TAC_SRC1|TAC_SRC2,0,0,0,0,TAC_FLOW_NEXT,TAC_FMT_BINARY,{V,R,R}}
#define U(op, text) [op] = {text,TAC_DEST,TAC_SRC1,0,0,0,0,TAC_FLOW_NEXT,TAC_FMT_UNARY,{V,R,N}}
#define J(op, text) [op] = {text,0,TAC_SRC1|TAC_SRC2,0,0,1,1,TAC_FLOW_IF,TAC_FMT_IF,{L,R,R}}
/* Única tabla de propiedades, inmutable y sin estado de generación.
 * STORE/CRYPTO leen dest como dirección, sin definir el valor de la base.
 * CALL conserva efectos aun cuando su resultado esté muerto (Aho §6.2.1).
 */
static const TacOpInfo operations[TAC_OP_COUNT] = {
    [TAC_COPY] = {"=",TAC_DEST,TAC_SRC1,0,0,0,0,TAC_FLOW_NEXT,TAC_FMT_COPY,{V,R,N}},
    B(TAC_ADD,"+"), B(TAC_SUB,"-"), B(TAC_AND,"and"), B(TAC_OR,"or"),
    B(TAC_XOR,"xor"), B(TAC_EQ,"=="), B(TAC_NE,"!="), B(TAC_LT,"<"),
    B(TAC_LE,"<="), B(TAC_GT,">"), B(TAC_GE,">="),
    U(TAC_NEG,"neg"), U(TAC_NOT,"not"),
    B(TAC_MUL,"*"), B(TAC_DIV,"/"), B(TAC_MOD,"%"), B(TAC_POW,"^"),
    [TAC_LOAD] = {"load",TAC_DEST,TAC_SRC1|TAC_SRC2,1,0,0,0,TAC_FLOW_NEXT,TAC_FMT_LOAD,{V,V,R}},
    [TAC_STORE] = {"store",0,TAC_DEST|TAC_SRC1|TAC_SRC2,0,1,0,1,TAC_FLOW_NEXT,TAC_FMT_STORE,{V,R,R}},
    [TAC_CRYPTO] = {"crypto",0,TAC_DEST|TAC_SRC1|TAC_SRC2,1,1,1,1,TAC_FLOW_NEXT,TAC_FMT_CRYPTO,{V,V,R}},
    [TAC_PARAM] = {"param",0,TAC_SRC1,0,0,0,1,TAC_FLOW_NEXT,TAC_FMT_PARAM,{N,R,N}},
    [TAC_CALL] = {"call",TAC_DEST,0,1,1,1,1,TAC_FLOW_NEXT,TAC_FMT_CALL,{V|N,S,K(TAC_INTEGER)}},
    [TAC_RETURN] = {"return",0,TAC_SRC1,0,0,1,1,TAC_FLOW_RETURN,TAC_FMT_RETURN,{N,R|N,N}},
    [TAC_GOTO] = {"goto",0,0,0,0,1,1,TAC_FLOW_GOTO,TAC_FMT_GOTO,{L,N,N}},
    J(TAC_IF_EQ,"=="), J(TAC_IF_NE,"!="), J(TAC_IF_LT,"<"),
    J(TAC_IF_LE,"<="), J(TAC_IF_GT,">"), J(TAC_IF_GE,">=")
};

/* allocate — reserva e inicializa; sin memoria termina con diagnóstico. */
static void *allocate(size_t bytes) {
    void *p = calloc(1, bytes);
    if (!p) { fputs("tac: sin memoria\n", stderr); exit(EXIT_FAILURE); }
    return p;
}
/* identifier — entrega un identificador y evita desbordamiento. */
static size_t identifier(size_t *next) {
    if (*next == SIZE_MAX) { fputs("tac: identificadores agotados\n", stderr); exit(EXIT_FAILURE); }
    return (*next)++;
}
/* tac_none — produce la posición ausente sin reservar memoria. */
TacOperand tac_none(void) { TacOperand o = {0}; return o; }
/* tac_variable — conserva referencia prestada, no copia ni posee el símbolo. */
TacOperand tac_variable(const Symbol *s) {
    TacOperand o = {.kind = TAC_VARIABLE}; o.value.symbol = s; return o;
}
/* tac_integer — conserva la constante sin truncarla al ancho del ISA. */
TacOperand tac_integer(int64_t n) {
    TacOperand o = {.kind = TAC_INTEGER}; o.value.integer = n; return o;
}
/* tac_label — toma la etiqueta local de un bloque vivo. */
TacOperand tac_label(const TacBlock *b) {
    TacOperand o = {.kind = TAC_LABEL}; o.value.id = b->label; return o;
}
/* tac_temp_new — reserva identidad virtual, sin escoger registro físico. */
TacOperand tac_temp_new(TacFunction *f) {
    TacOperand o = {.kind = TAC_TEMP}; o.value.id = identifier(&f->next_temp); return o;
}
/* tac_op_info — consulta segura de la tabla inmutable. */
const TacOpInfo *tac_op_info(TacOpcode op) {
    return (unsigned)op < TAC_OP_COUNT ? &operations[op] : NULL;
}
/* same_value — identidad de un pseudorregistro o símbolo, no su nombre. */
static int same_value(TacOperand a, TacOperand b) {
    return a.kind == b.kind && (a.kind == TAC_TEMP ? a.value.id == b.value.id :
                               a.value.symbol == b.value.symbol);
}
/* values — proyecta una máscara a los valores explícitos, sin duplicados. */
static size_t values(const TacInstruction *i, unsigned mask, TacOperand out[3]) {
    TacOperand operands[3] = {i->dest, i->src1, i->src2};
    size_t count = 0;
    for (size_t k = 0; k < 3; ++k) {
        TacOperand o = operands[k];
        if (!(mask & (1u << k)) || (o.kind != TAC_TEMP && o.kind != TAC_VARIABLE)) continue;
        size_t j;
        for (j = 0; j < count; ++j) if (same_value(o, out[j])) break;
        if (j == count) out[count++] = o;
    }
    return count;
}
/* tac_definitions — proyecta los destinos escritos según la tabla. */
size_t tac_definitions(const TacInstruction *i, TacOperand out[3]) {
    const TacOpInfo *p = i ? tac_op_info(i->op) : NULL;
    return p ? values(i, p->writes, out) : 0;
}
/* tac_uses — proyecta las posiciones leídas según la tabla. */
size_t tac_uses(const TacInstruction *i, TacOperand out[3]) {
    const TacOpInfo *p = i ? tac_op_info(i->op) : NULL;
    return p ? values(i, p->reads, out) : 0;
}
/* tac_program_new — crea propietario vacío. */
TacProgram *tac_program_new(void) { return allocate(sizeof(TacProgram)); }
/* tac_function_new — copia nombre y agrega en orden de inserción. */
TacFunction *tac_function_new(TacProgram *p, const char *name) {
    TacFunction *f = allocate(sizeof *f);
    f->name = allocate(strlen(name) + 1);
    strcpy(f->name, name);
    if (p->last) p->last->next = f; else p->first = f;
    p->last = f;
    return f;
}
/* tac_block_new — etiqueta local única; el primer bloque es la entrada. */
TacBlock *tac_block_new(TacFunction *f) {
    TacBlock *b = allocate(sizeof *b);
    b->function = f; b->label = identifier(&f->next_label);
    if (f->last) f->last->next = b; else f->first = f->entry = b;
    f->last = b;
    return b;
}
/* tac_append — copia cuádrupla; permite forma incompleta hasta validar. */
TacInstruction *tac_append(TacBlock *b, TacOpcode op, TacOperand d, TacOperand a, TacOperand c) {
    TacInstruction *i = allocate(sizeof *i);
    i->op = op; i->dest = d; i->src1 = a; i->src2 = c;
    if (b->last) b->last->next = i; else b->first = i;
    b->last = i;
    return i;
}
/* contains — consulta pertenencia de una arista por identidad. */
static int contains(TacBlock *const *list, size_t n, const TacBlock *b) {
    for (size_t k = 0; k < n; ++k) if (list[k] == b) return 1;
    return 0;
}
/* edge_add — amplía vector de aristas o termina sin memoria. */
static void edge_add(TacBlock ***list, size_t *n, TacBlock *b) {
    if (*n >= SIZE_MAX / sizeof **list - 1) {
        fputs("tac: demasiadas aristas\n", stderr); exit(EXIT_FAILURE);
    }
    TacBlock **v = realloc(*list, (*n + 1) * sizeof *v);
    if (!v) { fputs("tac: sin memoria\n", stderr); exit(EXIT_FAILURE); }
    *list = v; v[(*n)++] = b;
}
/* tac_connect — mantiene ambos sentidos, también para aristas a sí mismo. */
int tac_connect(TacBlock *from, TacBlock *to) {
    if (!from || !to || from->function != to->function) return 0;
    if (!contains(from->successors, from->successor_count, to))
        edge_add(&from->successors, &from->successor_count, to);
    if (!contains(to->predecessors, to->predecessor_count, from))
        edge_add(&to->predecessors, &to->predecessor_count, from);
    return 1;
}
/* fail — diagnóstico acotado, útil aun sin buffer. */
static int fail(char *error, size_t cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (cap) vsnprintf(error, cap, fmt, ap);
    va_end(ap);
    return 0;
}
/* member — comprueba pertenencia sin desreferenciar punteros de aristas. */
static int member(const TacFunction *f, const TacBlock *b) {
    for (const TacBlock *p = f->first; p; p = p->next) if (p == b) return 1;
    return 0;
}
/* target — resuelve etiquetas únicamente dentro de la función (Aho §8.4). */
static TacBlock *target(const TacFunction *f, size_t label) {
    for (TacBlock *b = f->first; b; b = b->next) if (b->label == label) return b;
    return NULL;
}
/* tac_validate — comprueba forma y grafo; no exige SSA ni analiza tipos CLS.
 * Las listas deben ser acíclicas y sus nodos memoria viva del constructor.
 */
int tac_validate(const TacProgram *p, char *error, size_t cap) {
    if (cap) error[0] = '\0';
    if (!p) return fail(error, cap, "programa NULL");
    const TacFunction *last_f = NULL;
    for (const TacFunction *f = p->first; f; f = f->next) {
        last_f = f;
        if (!f->name || !*f->name || !f->entry || f->entry != f->first)
            return fail(error, cap, "función sin nombre o entrada inicial válida");
        const TacBlock *last_b = NULL;
        for (const TacBlock *b = f->first; b; b = b->next) {
            last_b = b;
            if (b->function != f || target(f, b->label) != b)
                return fail(error, cap, "%s: B%zu etiqueta duplicada o dueño incorrecto", f->name, b->label);
            const TacInstruction *last_i = NULL;
            for (const TacInstruction *i = b->first; i; i = i->next) {
                last_i = i;
                const TacOpInfo *info = tac_op_info(i->op);
                if (!info) return fail(error, cap, "%s B%zu: operación inválida", f->name, b->label);
                TacOperand os[3] = {i->dest,i->src1,i->src2};
                for (size_t k = 0; k < 3; ++k) {
                    TacOperand o = os[k];
                    if ((unsigned)o.kind > TAC_LABEL || !(info->allowed[k] & K(o.kind)) ||
                        (o.kind == TAC_VARIABLE && (!o.value.symbol || !o.value.symbol->name)) ||
                        (o.kind == TAC_TEMP && o.value.id >= f->next_temp) ||
                        (o.kind == TAC_LABEL && !target(f, o.value.id)))
                        return fail(error, cap, "%s B%zu: operando %zu o etiqueta inválida", f->name, b->label, k);
                }
                if (i->op == TAC_CALL && (i->src1.value.symbol->kind != SYMBOL_FUNCTION || i->src2.value.integer < 0))
                    return fail(error, cap, "%s B%zu: call requiere función y aridad no negativa", f->name, b->label);
                if (info->flow != TAC_FLOW_NEXT && i->next)
                    return fail(error, cap, "%s B%zu: salto/return no está al final", f->name, b->label);
            }
            if (last_i != b->last) return fail(error, cap, "%s B%zu: last incorrecto", f->name, b->label);
            TacFlow flow = b->last ? tac_op_info(b->last->op)->flow : TAC_FLOW_NEXT;
            TacBlock *expected[2];
            size_t n = 0;
            if (flow == TAC_FLOW_GOTO || flow == TAC_FLOW_IF)
                expected[n++] = target(f, b->last->dest.value.id);
            if (flow == TAC_FLOW_IF && !b->next)
                return fail(error, cap, "%s B%zu: if sin bloque de caída", f->name, b->label);
            if ((flow == TAC_FLOW_NEXT || flow == TAC_FLOW_IF) && b->next &&
                (n == 0 || expected[0] != b->next)) expected[n++] = b->next;
            if (b->successor_count != n)
                return fail(error, cap, "%s B%zu: sucesores no coinciden con terminador", f->name, b->label);
            for (size_t k = 0; k < n; ++k) {
                if (b->successors[k] != expected[k] ||
                    !contains(expected[k]->predecessors, expected[k]->predecessor_count, b))
                    return fail(error, cap, "%s B%zu: sucesor/recíproca incorrectos", f->name, b->label);
            }
            for (size_t k = 0; k < b->predecessor_count; ++k) {
                TacBlock *pred = b->predecessors[k];
                if (!member(f, pred) || !contains(pred->successors, pred->successor_count, b))
                    return fail(error, cap, "%s B%zu: predecesor inválido", f->name, b->label);
                for (size_t j = 0; j < k; ++j) if (pred == b->predecessors[j])
                    return fail(error, cap, "%s B%zu: predecesor duplicado", f->name, b->label);
            }
        }
        if (last_b != f->last) return fail(error, cap, "%s: last incorrecto", f->name);
    }
    return last_f == p->last ? 1 : fail(error, cap, "last de programa incorrecto");
}
/* tac_program_free — libera propietarios y aristas; conserva símbolos ajenos. */
void tac_program_free(TacProgram *p) {
    if (!p) return;
    TacFunction *f = p->first;
    while (f) {
        TacFunction *fn = f->next;
        TacBlock *b = f->first;
        while (b) {
            TacBlock *bn = b->next;
            TacInstruction *i = b->first;
            while (i) { TacInstruction *in = i->next; free(i); i = in; }
            free(b->successors); free(b->predecessors); free(b); b = bn;
        }
        free(f->name); free(f); f = fn;
    }
    free(p);
}
