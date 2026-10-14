/*
 * codegen_array.c — Fase 4: cálculo de direcciones y operaciones de arreglos.
 *
 * Calcula elementos por filas y trata parámetros como casillas de dirección.
 * Emite accesos, paso por referencia, copia y puesta en cero mediante callback.
 *
 * Usa: codegen_array.h, symbol_table.h y contrato de CGArithContext.
 * Lo usa: generate.c y pruebas/arreglos.
 * Referencias: libro §6.4.3, §7.3; isa.md §6.3.
 */

#include "codegen_array.h"
#include <stdarg.h>
#include <stdio.h>

#define DO(x) do { if (!(x)) return 0; } while (0)
/* ==========================================================================
 * OPERACIONES Y VALIDACIÓN
 * ======================================================================= */


/*
 * reg — verifica un temporal ISA.
 *
 * Entrada:
 *   r  número de registro.
 * Salida:
 *   1 entre 4 y 15; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int reg(int r) { return r >= 4 && r <= 15; }
/*
 * context — comprueba un contexto de emisión.
 *
 * Entrada:
 *   c  contexto o NULL.
 * Salida:
 *   1 si tiene callback; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int context(const CGArithContext *c) { return c && c->emit; }
/*
 * out — formatea y entrega una operación al callback.
 *
 * Entrada:
 *   c  contexto con emit no NULL.
 *   slot  slot 0, 2, 3 o -1 para etiqueta.
 *   d  registro destino o -1.
 *   a  primer registro fuente o -1.
 *   b  segundo registro fuente o -1.
 *   fmt  formato printf.
 *   ...  argumentos del formato.
 * Salida:
 *   1 si formato y callback aceptan; 0 si falla alguno.
 *   Efectos: invoca c->emit con una operación local.
 * Restricciones:
 *   texto menor de 128 bytes; callback no conserva &op.
 */
static int out(CGArithContext *c, int slot, int d, int a, int b,
               const char *fmt, ...) {
    CGArithOp op;
    va_list ap;
    op.slot=slot; op.rd=d; op.rs1=a; op.rs2=b;
    va_start(ap,fmt);
    int n=vsnprintf(op.text,sizeof op.text,fmt,ap);
    va_end(ap);
    return n>=0 && n<(int)sizeof op.text && c->emit(c->user,&op);
}
/*
 * alu — entrega una operación ALU de tres registros.
 *
 * Entrada:
 *   c  contexto con callback.
 *   mn  mnemónico no NULL.
 *   d  registro destino.
 *   a  primera fuente.
 *   b  segunda fuente.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   registros ISA; operación en slot 0.
 */
static int alu(CGArithContext *c,const char *mn,int d,int a,int b) {
    return out(c,0,d,a,b,"%s r%d, r%d, r%d",mn,d,a,b);
}
/*
 * mov — entrega una carga de inmediato.
 *
 * Entrada:
 *   c  contexto con callback.
 *   d  registro destino.
 *   value  inmediato.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   inmediato debe caber en 18 bits con signo.
 */
static int mov(CGArithContext *c,int d,int value) {
    return out(c,0,d,-1,-1,"mov r%d, %d",d,value);
}
/*
 * load — entrega una carga lw al callback.
 *
 * Entrada:
 *   c  contexto con callback.
 *   d  destino.
 *   base  registro base.
 *   offset  desplazamiento en bytes.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback en slot 3.
 * Restricciones:
 *   registros ISA; offset de 18 bits con signo.
 */
static int load(CGArithContext *c,int d,int base,int offset) {
    return out(c,3,d,base,-1,"lw r%d, %d(r%d)",d,offset,base);
}
/*
 * store — entrega un almacenamiento sw al callback.
 *
 * Entrada:
 *   c  contexto con callback.
 *   value  registro del valor.
 *   base  registro base.
 *   offset  desplazamiento en bytes.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback con base como rs1 y valor como rs2.
 * Restricciones:
 *   offset de 18 bits con signo.
 */
static int store(CGArithContext *c,int value,int base,int offset) {
    /* Metadatos de sw: rs1=base de dirección, rs2=valor; no escribe registro. */
    return out(c,3,-1,base,value,"sw r%d, %d(r%d)",value,offset,base);
}
/*
 * words — valida forma y área de un arreglo.
 *
 * Entrada:
 *   a  símbolo de arreglo o NULL.
 * Salida:
 *   cantidad de palabras; 0 si contrato inválido.
 *   Efectos: ninguno.
 * Restricciones:
 *   una o dos dimensiones; área 0–4095; parámetro ocupa casilla de cuatro
 *   bytes.
 */
static int words(const Symbol *a) {
    if (!a || (a->kind!=SYMBOL_VARIABLE && a->kind!=SYMBOL_PARAMETER) ||
        (a->type!=TYPE_INT && a->type!=TYPE_BOOL) ||
        a->dimensions<1 || a->dimensions>2 || a->address<0 ||
        a->address>4092 || a->address%4) return 0;
    int n=1;
    for(int i=0;i<a->dimensions;++i) {
        if(a->dim_sizes[i]<=0 || a->dim_sizes[i]>1024/n) return 0;
        n*=a->dim_sizes[i];
    }
    if(a->kind==SYMBOL_PARAMETER) return a->width==4 ? n : 0;
    return a->width==4*n && a->address<=4096-4*n ? n : 0;
}
/*
 * same — compara tipo y forma de dos arreglos válidos.
 *
 * Entrada:
 *   a  primer símbolo o NULL.
 *   b  segundo símbolo o NULL.
 * Salida:
 *   1 si ambos válidos y de igual forma/tipo; 0 si no.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int same(const Symbol *a,const Symbol *b) {
    if(!words(a) || !words(b) || a->type!=b->type || a->dimensions!=b->dimensions)
        return 0;
    for(int i=0;i<a->dimensions;++i) if(a->dim_sizes[i]!=b->dim_sizes[i]) return 0;
    return 1;
}
/* ==========================================================================
 * BASE Y ACCESO A ELEMENTOS
 * ======================================================================= */


/* cg_array_base — véase el contrato en codegen_array.h. */
int cg_array_base(CGArithContext *c,const Symbol *a,int dst) {
    if(!context(c) || !words(a) || !reg(dst)) return 0;
    /* La dirección de variable es la base; la de parámetro es una casilla de dirección. */
    if(a->kind==SYMBOL_PARAMETER) return load(c,dst,0,a->address);
    return mov(c,dst,a->address);
}
/*
 * valid_address — valida registros para calcular una dirección.
 *
 * Entrada:
 *   c  contexto.
 *   a  arreglo.
 *   idx  registros de índices.
 *   count  cantidad de índices.
 *   dst  destino de dirección.
 *   w  dos auxiliares.
 * Salida:
 *   1 si contrato válido; 0 si no.
 *   Efectos: ninguno.
 * Restricciones:
 *   count coincide con dimensions; destino/auxiliares 4–15 distintos entre
 *   sí y de índices.
 */
static int valid_address(CGArithContext *c,const Symbol *a,const int *idx,
                         int count,int dst,const int w[2]) {
    if(!context(c) || !words(a) || !idx || !w || count!=a->dimensions ||
        !reg(dst) || !reg(w[0]) || !reg(w[1]) || dst==w[0] || dst==w[1] ||
        w[0]==w[1]) return 0;
    for(int i=0;i<count;++i)
        if(!reg(idx[i]) || idx[i]==dst || idx[i]==w[0] || idx[i]==w[1]) return 0;
    return 1;
}
/* cg_array_address — véase el contrato en codegen_array.h. */
int cg_array_address(CGArithContext *c,const Symbol *a,const int *idx,
                     int count,int dst,const int w[2]) {
    if(!valid_address(c,a,idx,count,dst,w)) return 0;
    DO(cg_array_base(c,a,w[1]));
    if(count==1) {
        DO(alu(c,"add",dst,idx[0],0));
    } else {
        /* Expansión de Horner de los bits del número de columnas conocido.
         * dst = fila*columnas mediante sumas y desplazamientos de registros. */
        unsigned cols=(unsigned)a->dim_sizes[1], top=1;
        while(top<=cols/2) top<<=1;
        DO(mov(c,w[0],1)); DO(alu(c,"add",dst,idx[0],0));
        for(unsigned bit=top>>1;bit;bit>>=1) {
            DO(alu(c,"sll",dst,dst,w[0]));
            if(cols & bit) DO(alu(c,"add",dst,dst,idx[0]));
        }
        DO(alu(c,"add",dst,dst,idx[1]));
    }
    DO(mov(c,w[0],2)); DO(alu(c,"sll",dst,dst,w[0]));
    return alu(c,"add",dst,dst,w[1]);
}
/*
 * valid_value — valida registros de un acceso y su valor.
 *
 * Entrada:
 *   c  contexto.
 *   a  arreglo.
 *   idx  registros de índices.
 *   count  cantidad de índices.
 *   address  registro de dirección.
 *   value  registro del valor.
 *   w  dos auxiliares.
 *   reading  1 para lectura, 0 para escritura.
 * Salida:
 *   1 si contrato válido; 0 si no.
 *   Efectos: ninguno.
 * Restricciones:
 *   value distinto de address/auxiliares; en lectura también de índices.
 */
static int valid_value(CGArithContext *c,const Symbol *a,const int *idx,
                       int count,int address,int value,const int w[2],int reading) {
    if(!valid_address(c,a,idx,count,address,w) || !reg(value) ||
        value==address || value==w[0] || value==w[1]) return 0;
    if(reading) for(int i=0;i<count;++i) if(value==idx[i]) return 0;
    return 1;
}
/* cg_array_load — véase el contrato en codegen_array.h. */
int cg_array_load(CGArithContext *c,const Symbol *a,const int *idx,
                  int count,int address,int value,const int w[2]) {
    if(!valid_value(c,a,idx,count,address,value,w,1)) return 0;
    DO(cg_array_address(c,a,idx,count,address,w));
    return load(c,value,address,0);
}
/* cg_array_store — véase el contrato en codegen_array.h. */
int cg_array_store(CGArithContext *c,const Symbol *a,const int *idx,
                   int count,int address,int value,const int w[2]) {
    if(!valid_value(c,a,idx,count,address,value,w,0) || a->modifier==MODIFIER_VI)
        return 0;
    DO(cg_array_address(c,a,idx,count,address,w));
    return store(c,value,address,0);
}
/* cg_array_pass — véase el contrato en codegen_array.h. */
int cg_array_pass(CGArithContext *c,const Symbol *a,const Symbol *p,int temp) {
    if(!context(c) || !same(a,p) || p->kind!=SYMBOL_PARAMETER || !reg(temp) ||
        (a->modifier==MODIFIER_VI && p->modifier!=MODIFIER_VI)) return 0;
    DO(cg_array_base(c,a,temp));
    return store(c,temp,0,p->address);
}
/* ==========================================================================
 * CICLOS DE COPIA E INICIALIZACIÓN
 * ======================================================================= */


/*
 * increment — entrega avance de un registro mediante saddi.
 *
 * Entrada:
 *   c  contexto con callback.
 *   r  registro ISA.
 *   amount  incremento con signo.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback.
 * Restricciones:
 *   amount de 18 bits con signo.
 */
static int increment(CGArithContext *c,int r,int amount) {
    return out(c,0,r,r,-1,"saddi r%d, r%d, %d",r,r,amount);
}
/*
 * loop_label — define una etiqueta interna de ciclo de arreglo.
 *
 * Entrada:
 *   c  contexto con callback.
 *   name  buffer de al menos 64 bytes.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: escribe name, incrementa next_label e invoca callback.
 * Restricciones:
 *   contador compartido por programa.
 */
static int loop_label(CGArithContext *c,char name[64]) {
    snprintf(name,64,"__ccom_array_%u_loop",c->next_label++);
    return out(c,-1,-1,-1,-1,"%s:",name);
}
/*
 * repeat — entrega repetición mientras el contador no sea cero.
 *
 * Entrada:
 *   c  contexto con callback.
 *   count  registro contador.
 *   name  etiqueta de ciclo no NULL.
 * Salida:
 *   no cero si acepta; 0 si falla.
 *   Efectos: invoca callback en BRU.
 * Restricciones:
 *   ninguna.
 */
static int repeat(CGArithContext *c,int count,const char *name) {
    return out(c,2,-1,count,0,"bne r%d, r0, %s",count,name);
}
/* cg_array_copy — véase el contrato en codegen_array.h. */
int cg_array_copy(CGArithContext *c,const Symbol *dst,const Symbol *src,
                  const int w[4]) {
    if(!context(c) || !same(dst,src) || dst->modifier==MODIFIER_VI || !w)
        return 0;
    for(int i=0;i<4;++i) {
        if(!reg(w[i])) return 0;
        for(int j=0;j<i;++j) if(w[i]==w[j]) return 0;
    }
    DO(cg_array_base(c,src,w[0])); DO(cg_array_base(c,dst,w[1]));
    DO(mov(c,w[3],words(src)));
    char name[64]; DO(loop_label(c,name));
    DO(load(c,w[2],w[0],0)); DO(store(c,w[2],w[1],0));
    DO(increment(c,w[0],4)); DO(increment(c,w[1],4));
    DO(increment(c,w[3],-1)); DO(repeat(c,w[3],name));
    return 1;
}
/* cg_array_zero — véase el contrato en codegen_array.h. */
int cg_array_zero(CGArithContext *c,const Symbol *a,const int w[2]) {
    if(!context(c) || !words(a) || !w || !reg(w[0]) || !reg(w[1]) || w[0]==w[1])
        return 0;
    DO(cg_array_base(c,a,w[0])); DO(mov(c,w[1],words(a)));
    char name[64]; DO(loop_label(c,name));
    DO(store(c,0,w[0],0)); DO(increment(c,w[0],4));
    DO(increment(c,w[1],-1)); DO(repeat(c,w[1],name));
    return 1;
}
