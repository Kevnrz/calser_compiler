/* Clones Tipo I/II con los reconocimientos reales de Flex.
 *
 * Tipo I conserva clase y lexema. Tipo II permite una biyección de nombres y
 * variación de literales de la misma clase; operadores y tipos son exactos.
 * Se indexan ventanas por una firma de ocurrencias previas de identificadores,
 * se verifican las colisiones y se extiende cada par con la misma biyección.
 * El proceso atiende un paquete, limita trabajo/memoria y libera todo al salir.
 * Nunca devuelve un conjunto parcial al alcanzar un límite o un error léxico.
 * main.c -> clones_run -> lector CCL1 -> observador Flex -> índice/extensión.
 * ctx solo presta el contexto local al callback; símbolos prestan lexemas de
 * los textos propios hasta su serialización. Los offsets son bytes UTF-8 de
 * cada fuente. El padre puede matar el proceso para cancelar; no se guardan
 * fuentes ni se dejan recursos persistentes fuera del paquete de entrada.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clones.h"
#include "diagnostics.h"
#include "diagnostics_json.h"
#include "lexer_service.h"

#define FILES 200
#define TOKENS 100000
#define PAIRS 500
#define ATTEMPTS 50000
#define WORK 20000000
#define TABLE 262144
#define FILE_BYTES 2000000
#define TOTAL_BYTES 20000000
#define PATH_BYTES 16384

enum { ID, NUMBER, BOOLEAN, FIXED };
typedef struct { size_t start, end; unsigned line, value, kind; int previous; } Token;
typedef struct { char *path, *text; size_t length; unsigned first, count; } Source;
typedef struct {
    const char *text; size_t length; uint64_t hash;
    unsigned id, document; int last;
} Symbol;
typedef struct { uint64_t hash; unsigned file, start; int next; } Window;
typedef struct { unsigned a,b,la,lb,ra,rb,type; } Pair;
typedef struct { unsigned a,b,left,right; int diagonal, next; } Cover;
typedef struct {
    Source files[FILES]; unsigned count, file, minimum, n_tokens, n_symbols;
    Token *tokens; Symbol *symbols; Window *windows; Cover *covers;
    int *buckets, *cover_buckets;
    unsigned *direct, *seen_a, *seen_b, generation;
    Pair pairs[PAIRS]; unsigned n_pairs, n_covers, attempts;
    unsigned long work;
    size_t cursor; unsigned line;
    int limited, failed, lexical;
} Context;
/* Flex observer has no userdata. Only one request runs inside this process. */
static Context *ctx;
static uint64_t mix(uint64_t hash, uint64_t value) {
    return (hash ^ value) * UINT64_C(1099511628211);
}
static uint64_t bytes_hash(const char *s, size_t n) {
    uint64_t h = UINT64_C(1469598103934665603);
    for (size_t i=0;i<n;i++) h=mix(h,(unsigned char)s[i]);
    return h;
}
static int work(void) {
    if (++ctx->work > WORK) ctx->limited=1;
    return !ctx->limited;
}
static void observe(const char *kind, size_t start, size_t end) {
    Context *c=ctx;
    if (c->limited || c->failed || !strcmp(kind,"comentario") || !strcmp(kind,"invalido")) return;
    if (c->n_tokens == TOKENS) { c->limited=1; return; }
    Source *source=&c->files[c->file];
    while (c->cursor < start) if (source->text[c->cursor++]=='\n') c->line++;
    size_t length=end-start;
    uint64_t hash=bytes_hash(source->text+start,length);
    unsigned slot=(unsigned)hash & (TABLE-1);
    while (c->symbols[slot].text && (c->symbols[slot].hash!=hash ||
           c->symbols[slot].length!=length || memcmp(c->symbols[slot].text,source->text+start,length))) {
        if (!work()) return;
        slot=(slot+1)&(TABLE-1);
    }
    Symbol *symbol=&c->symbols[slot];
    if (!symbol->text) *symbol=(Symbol){source->text+start,length,hash,++c->n_symbols,0,-1};
    unsigned k=!strcmp(kind,"identificador") ? ID : !strcmp(kind,"numero") ? NUMBER : !strcmp(kind,"booleano") ? BOOLEAN : FIXED;
    int previous=-1;
    if (k==ID) {
        if (symbol->document==c->file+1) previous=symbol->last;
        symbol->document=c->file+1;
        symbol->last=(int)(c->n_tokens-source->first);
    }
    c->tokens[c->n_tokens++]=(Token){start,end,c->line,symbol->id,k,previous};
    source->count++;
}
static void lexical_error(const char *category, DiagPosition pos, const char *fmt, va_list args, void *data) {
    (void)category; (void)pos; (void)fmt; (void)args; (void)data;
    ctx->lexical=1;
}
/* Length-framed little-endian input avoids a second JSON parser dependency.
 * Paths are identifiers, not filenames to open. No NUL or duplicate paths. */
static int read_u32(FILE *in, uint32_t *value) {
    unsigned char b[4];
    if (fread(b,1,4,in)!=4) return 0;
    *value=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;
    return 1;
}
static int read_package(FILE *in) {
    char magic[4]; uint32_t n;
    if (fread(magic,1,4,in)!=4 || memcmp(magic,"CCL1",4) || !read_u32(in,&n)) return 0;
    if (n>FILES) { ctx->limited=1; return 0; }
    ctx->count=n;
    size_t total=0;
    for (unsigned i=0;i<n;i++) {
        uint32_t path_length,length;
        if (!read_u32(in,&path_length) || !read_u32(in,&length)) return 0;
        if (length>FILE_BYTES || total+length>TOTAL_BYTES || path_length>PATH_BYTES) { ctx->limited=1; return 0; }
        if (!path_length) return 0;
        Source *s=&ctx->files[i];
        s->path=malloc((size_t)path_length+1); s->text=malloc((size_t)length+1);
        if (!s->path || !s->text) return 0;
        if (fread(s->path,1,path_length,in)!=path_length || fread(s->text,1,length,in)!=length) return 0;
        if (memchr(s->path,0,path_length)) return 0;
        s->path[path_length]=0; s->text[length]=0; s->length=length;
        for (unsigned j=0;j<i;j++) if (!strcmp(s->path,ctx->files[j].path)) return 0;
        total+=length;
    }
    return fgetc(in)==EOF && !ferror(in);
}
static int compatible(Token a, Token b, int parameterized) {
    if (!work() || a.kind!=b.kind) return 0;
    if (!parameterized || a.kind==FIXED) return a.value==b.value;
    if (a.kind!=ID) return 1;
    unsigned g=ctx->generation;
    if (ctx->seen_a[a.value]==g) return ctx->direct[a.value]==b.value;
    if (ctx->seen_b[b.value]==g) return 0;
    ctx->seen_a[a.value]=ctx->seen_b[b.value]=g;
    ctx->direct[a.value]=b.value;
    return 1;
}
static uint64_t signature(Token *tokens, unsigned start, int parameterized) {
    uint64_t hash=UINT64_C(1469598103934665603);
    for (unsigned j=0;j<ctx->minimum && work();j++) {
        Token t=tokens[start+j];
        unsigned value=t.value;
        if (parameterized) {
            if (t.kind==ID) value=t.previous >= (int)start ? start+j-(unsigned)t.previous : 0;
            else if (t.kind==NUMBER || t.kind==BOOLEAN) value=0;
        }
        hash=mix(mix(hash,t.kind),value);
    }
    return hash;
}
static unsigned cover_slot(unsigned a,unsigned b,int diagonal) {
    return (unsigned)mix(mix(a+1,b+1),(uint32_t)diagonal)&(TABLE-1);
}
static int covered(unsigned a,unsigned b,unsigned start,unsigned other) {
    int diagonal=(int)start-(int)other;
    unsigned slot=cover_slot(a,b,diagonal);
    for (int i=ctx->cover_buckets[slot];i>=0;i=ctx->covers[i].next) {
        if (!work()) return 1;
        Cover c=ctx->covers[i];
        if (c.a==a && c.b==b && c.diagonal==diagonal && c.left<=other && other+ctx->minimum<=c.right) return 1;
    }
    return 0;
}
static void verify(unsigned a,unsigned b,unsigned other,unsigned start,int parameterized) {
    Context *c=ctx;
    Token *ta=c->tokens+c->files[a].first, *tb=c->tokens+c->files[b].first;
    c->generation++;
    for (unsigned k=0;k<c->minimum;k++) if (!compatible(ta[other+k],tb[start+k],parameterized)) return;
    unsigned la=other,lb=start,ra=other+c->minimum,rb=start+c->minimum;
    while (ra<c->files[a].count && rb<c->files[b].count && compatible(ta[ra],tb[rb],parameterized)) { ra++; rb++; }
    while (la && lb && compatible(ta[la-1],tb[lb-1],parameterized)) { la--; lb--; }
    if (c->limited) return;
    /* Covers include exact matches during the parameterized pass, so repeated
     * seeds cannot exhaust the verification budget on the same diagonal. */
    if (c->n_covers==ATTEMPTS) { c->limited=1; return; }
    int diagonal=(int)start-(int)other;
    unsigned slot=cover_slot(a,b,diagonal), ci=c->n_covers++;
    c->covers[ci]=(Cover){a,b,la,ra,diagonal,c->cover_buckets[slot]}; c->cover_buckets[slot]=(int)ci;
    int exact=1;
    for (unsigned i=0;i<ra-la;i++) {
        if (!work()) return;
        if (ta[la+i].value!=tb[lb+i].value) { exact=0; break; }
    }
    if (parameterized && exact) return;
    if (c->n_pairs==PAIRS) { c->limited=1; return; }
    c->pairs[c->n_pairs++]=(Pair){a,b,la,lb,ra,rb,parameterized ? 2u : 1u};
}
static void compare(int parameterized) {
    Context *c=ctx;
    memset(c->buckets,0xff,TABLE*sizeof(int));
    memset(c->cover_buckets,0xff,TABLE*sizeof(int));
    c->n_covers=0;
    unsigned windows=0;
    for (unsigned f=0;f<c->count && !c->limited;f++) {
        Source *s=&c->files[f];
        if (s->count<c->minimum) continue;
        unsigned pending=windows;
        for (unsigned start=0;start<=s->count-c->minimum && !c->limited;start++) {
            uint64_t hash=signature(c->tokens+s->first,start,parameterized);
            unsigned slot=(unsigned)hash&(TABLE-1);
            for (int wi=c->buckets[slot];wi>=0 && !c->limited;wi=c->windows[wi].next) {
                if (!work()) break;
                Window w=c->windows[wi];
                if (w.hash!=hash) continue;
                if (++c->attempts>ATTEMPTS) { c->limited=1; break; }
                if (!covered(w.file,f,start,w.start)) verify(w.file,f,w.start,start,parameterized);
            }
            c->windows[windows++]=(Window){hash,f,start,-1};
        }
        /* Publish only after this document: no within-document comparisons. */
        for (unsigned i=pending;i<windows;i++) {
            unsigned slot=(unsigned)c->windows[i].hash&(TABLE-1);
            c->windows[i].next=c->buckets[slot]; c->buckets[slot]=(int)i;
        }
    }
}
static int pair_order(const void *left,const void *right) {
    const Pair *a=left,*b=right;
    unsigned na=a->ra-a->la,nb=b->ra-b->la;
    if (na!=nb) return na>nb ? -1 : 1;
    const unsigned va[]={a->type,a->a,a->b,a->la,a->lb}, vb[]={b->type,b->a,b->b,b->la,b->lb};
    for (int i=0;i<5;i++) if (va[i]!=vb[i]) return va[i]<vb[i] ? -1 : 1;
    return 0;
}
static void fragment(FILE *out,unsigned file,unsigned left,unsigned right) {
    Token *t=ctx->tokens+ctx->files[file].first;
    fprintf(out,"{\"file\":%u,\"start\":%zu,\"end\":%zu,\"line_start\":%u,\"line_end\":%u}",file,t[left].start,t[right-1].end,t[left].line,t[right-1].line);
}
int clones_run(FILE *out,const char *snapshot,unsigned minimum) {
    Context c={0}; ctx=&c; c.minimum=minimum;
    FILE *input=fopen(snapshot,"rb");
    if (!input || !read_package(input)) c.failed=1;
    if (input) fclose(input);
    c.tokens=calloc(TOKENS,sizeof(Token)); c.symbols=calloc(TABLE,sizeof(Symbol));
    c.windows=calloc(TOKENS,sizeof(Window)); c.covers=calloc(ATTEMPTS,sizeof(Cover));
    c.buckets=malloc(TABLE*sizeof(int)); c.cover_buckets=malloc(TABLE*sizeof(int));
    c.direct=calloc(TOKENS+1,sizeof(unsigned));
    c.seen_a=calloc(TOKENS+1,sizeof(unsigned)); c.seen_b=calloc(TOKENS+1,sizeof(unsigned));
    if (!c.tokens || !c.symbols || !c.windows || !c.covers || !c.buckets || !c.cover_buckets || !c.direct || !c.seen_a || !c.seen_b) c.failed=1;
    if (!c.failed && !c.limited) {
        diag_set_observer(lexical_error,NULL,0); diag_init(NULL); lexer_set_observer(observe);
        for (c.file=0;c.file<c.count && !c.limited;c.file++) {
            c.cursor=0; c.line=1; c.files[c.file].first=c.n_tokens;
            lexer_scan_text(c.files[c.file].text,c.files[c.file].length);
        }
        lexer_set_observer(NULL); diag_set_observer(NULL,NULL,1); diag_free();
        if (!c.lexical && !c.limited) { compare(0); if (!c.limited) compare(1); }
    }
    const char *status=c.limited ? "limit" : c.failed ? "error" : c.lexical ? "lexical_error" : "ok";
    const char *message=c.limited ? "Limite de clones alcanzado (archivos, bytes, tokens, verificaciones, pares o trabajo). Aumenta el minimo o reduce la carpeta." :
        c.failed ? "Paquete de instantaneas invalido, no disponible o memoria insuficiente." :
        c.lexical ? "Hay errores lexicos en una instantanea. Corrigelos antes de comparar clones." : "";
    int code=strcmp(status,"ok")!=0;
    fputs("{\"schema_version\":1,\"operation\":\"clones\",\"offset_unit\":\"utf8\",\"source\":",out); json_write_string(out,snapshot);
    fprintf(out,",\"exit_code\":%d,\"status\":\"%s\",\"minimum\":%u,\"explanation\":",code,status,minimum); json_write_string(out,message);
    fputs(",\"files\":[",out);
    /* Even malformed input returns valid JSON, without incomplete file names. */
    if (!c.failed) for (unsigned i=0;i<c.count;i++) {
        if (i) fputc(',',out);
        fprintf(out,"{\"id\":%u,\"path\":",i); json_write_string(out,c.files[i].path);
        fprintf(out,",\"bytes\":%zu}",c.files[i].length);
    }
    fputs("],\"pairs\":[",out);
    if (!code) {
        qsort(c.pairs,c.n_pairs,sizeof(Pair),pair_order);
        unsigned count=0;
        for (unsigned i=0;i<c.n_pairs;i++) {
            Pair p=c.pairs[i]; int contained=0;
            for (unsigned j=0;j<i;j++) {
                Pair q=c.pairs[j];
                if (q.type==p.type && q.a==p.a && q.b==p.b && q.la<=p.la && p.ra<=q.ra && q.lb<=p.lb && p.rb<=q.rb) { contained=1; break; }
            }
            if (contained) continue;
            if (count++) fputc(',',out);
            fprintf(out,"{\"type\":\"%s\",\"tokens\":%u,\"a\":",p.type==1 ? "I" : "II",p.ra-p.la);
            fragment(out,p.a,p.la,p.ra); fputs(",\"b\":",out); fragment(out,p.b,p.lb,p.rb); fputc('}',out);
        }
    }
    fputs("]}\n",out);
    for (unsigned i=0;i<c.count;i++) { free(c.files[i].path); free(c.files[i].text); }
    free(c.tokens); free(c.symbols); free(c.windows); free(c.covers); free(c.buckets); free(c.cover_buckets);
    free(c.direct); free(c.seen_a); free(c.seen_b); ctx=NULL;
    return fflush(out)==0 && !ferror(out) ? code : 1;
}
