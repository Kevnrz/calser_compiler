#include "codegen_array.h"
#include <stdarg.h>
#include <stdio.h>

#define DO(x) do { if (!(x)) return 0; } while (0)
static int reg(int r) { return r >= 4 && r <= 15; }
static const char *arith_reg_name(int reg) {
    static const char *names[16] = {
        "rz", "rl", "rx", "rg",
        "t0", "t1", "t2", "t3",
        "t4", "t5", "t6", "t7",
        "t8", "t9", "t10", "t11"
    };

    if (reg < 0 || reg >= 16)
        return "rz";

    return names[reg];
}
static int context(const CGArithContext *c) { return c && c->emit; }
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
static int alu(CGArithContext *c,const char *mn,int d,int a,int b) {
    return out(c,0,d,a,b,"%s r%d, r%d, r%d",mn,arith_reg_name(d),arith_reg_name(a),arith_reg_name(b));
}
static int mov(CGArithContext *c,int d,int value) {
    return out(c,0,d,-1,-1,"mov r%d, %d",d,arith_reg_name(value));
}
static int load(CGArithContext *c,int d,int base,int offset) {
    return out(c,3,d,base,-1,"lw r%d, %d(r%d)",arith_reg_name(d),offset,arith_reg_name(base));
}
static int store(CGArithContext *c,int value,int base,int offset) {
    /* LSU store metadata: rs1=address base, rs2=value; no destination. */
    return out(c,3,-1,base,value,"sw r%d, %d(r%d)",arith_reg_name(value),offset,arith_reg_name(base));
}
/* Valid arrays in the compiler-owned 4KB region. A parameter stores only a
 * pointer, but still carries full shape. Avoid overflowing host C arithmetic. */
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
static int same(const Symbol *a,const Symbol *b) {
    if(!words(a) || !words(b) || a->type!=b->type || a->dimensions!=b->dimensions)
        return 0;
    for(int i=0;i<a->dimensions;++i) if(a->dim_sizes[i]!=b->dim_sizes[i]) return 0;
    return 1;
}
int cg_array_base(CGArithContext *c,const Symbol *a,int dst) {
    if(!context(c) || !words(a) || !reg(dst)) return 0;
    /* Variable address is the data base; parameter address is a pointer cell. */
    if(a->kind==SYMBOL_PARAMETER) return load(c,dst,0,a->address);
    return mov(c,dst,a->address);
}
static int valid_address(CGArithContext *c,const Symbol *a,const int *idx,
                         int count,int dst,const int w[2]) {
    if(!context(c) || !words(a) || !idx || !w || count!=a->dimensions ||
        !reg(dst) || !reg(w[0]) || !reg(w[1]) || dst==w[0] || dst==w[1] ||
        w[0]==w[1]) return 0;
    for(int i=0;i<count;++i)
        if(!reg(idx[i]) || idx[i]==dst || idx[i]==w[0] || idx[i]==w[1]) return 0;
    return 1;
}
int cg_array_address(CGArithContext *c,const Symbol *a,const int *idx,
                     int count,int dst,const int w[2]) {
    if(!valid_address(c,a,idx,count,dst,w)) return 0;
    DO(cg_array_base(c,a,w[1]));
    if(count==1) {
        DO(alu(c,"add",dst,idx[0],0));
    } else {
        /* Horner expansion of the compile-time columns' binary digits.
         * dst = row*columns using only additions and register shifts. */
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
static int valid_value(CGArithContext *c,const Symbol *a,const int *idx,
                       int count,int address,int value,const int w[2],int reading) {
    if(!valid_address(c,a,idx,count,address,w) || !reg(value) ||
        value==address || value==w[0] || value==w[1]) return 0;
    if(reading) for(int i=0;i<count;++i) if(value==idx[i]) return 0;
    return 1;
}
int cg_array_load(CGArithContext *c,const Symbol *a,const int *idx,
                  int count,int address,int value,const int w[2]) {
    if(!valid_value(c,a,idx,count,address,value,w,1)) return 0;
    DO(cg_array_address(c,a,idx,count,address,w));
    return load(c,value,address,0);
}
int cg_array_store(CGArithContext *c,const Symbol *a,const int *idx,
                   int count,int address,int value,const int w[2]) {
    if(!valid_value(c,a,idx,count,address,value,w,0) || a->modifier==MODIFIER_VI)
        return 0;
    DO(cg_array_address(c,a,idx,count,address,w));
    return store(c,value,address,0);
}
int cg_array_pass(CGArithContext *c,const Symbol *a,const Symbol *p,int temp) {
    if(!context(c) || !same(a,p) || p->kind!=SYMBOL_PARAMETER || !reg(temp) ||
        (a->modifier==MODIFIER_VI && p->modifier!=MODIFIER_VI)) return 0;
    DO(cg_array_base(c,a,temp));
    return store(c,temp,0,p->address);
}
static int increment(CGArithContext *c,int r,int amount) {
    return out(c,0,r,r,-1,"saddi r%d, r%d, %d",r,r,amount);
}
static int loop_label(CGArithContext *c,char name[64]) {
    snprintf(name,64,"__ccom_array_%u_loop",c->next_label++);
    return out(c,-1,-1,-1,-1,"%s:",name);
}
static int repeat(CGArithContext *c,int count,const char *name) {
    return out(c,2,-1,count,0,"bne r%d, rz, %s",count,name);
}
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
int cg_array_zero(CGArithContext *c,const Symbol *a,const int w[2]) {
    if(!context(c) || !words(a) || !w || !reg(w[0]) || !reg(w[1]) || w[0]==w[1])
        return 0;
    DO(cg_array_base(c,a,w[0])); DO(mov(c,w[1],words(a)));
    char name[64]; DO(loop_label(c,name));
    DO(store(c,0,w[0],0)); DO(increment(c,w[0],4));
    DO(increment(c,w[1],-1)); DO(repeat(c,w[1],name));
    return 1;
}
