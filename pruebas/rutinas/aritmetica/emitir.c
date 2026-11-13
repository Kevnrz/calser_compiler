#include "codegen_arith.h"
#include <stdio.h>
#include <string.h>
static int emit(void *user, const CGArithOp *op) {
    FILE *f=user;
    if (op->slot==-1) return fprintf(f,"%s\n",op->text)>=0;
    const char *s[4]={"nopi","nopi","nopi","nopi"};
    if (op->slot<0 || op->slot>3) return 0;
    s[op->slot]=op->text;
    if (fprintf(f,"{ %s | %s | %s | %s }\n",s[0],s[1],s[2],s[3])<0) return 0;
    /* Conservative scheduler: one whole bundle of separation. */
    return fprintf(f,"{ nopi | nopi | nopi | nopi }\n")>=0;
}
static int reject(void *user,const CGArithOp *op) {
    (void)op; ++*(int *)user; return 0;
}
static int contracts(void) {
    int w[8]={7,8,9,10,11,12,13,14},calls=0;
    CGArithContext c={reject,&calls,0};
    if(cg_arith_emit(&c,CG_ARITH_DIV,4,5,6,w,NULL)||calls) return 1;
    if(cg_arith_emit(&c,CG_ARITH_DIV,4,5,6,w,"bad label")||calls) return 1;
    if(cg_arith_emit(&c,CG_ARITH_MUL,4,5,4,w,NULL)||calls) return 1;
    w[7]=7;
    if(cg_arith_emit(&c,CG_ARITH_MUL,4,5,6,w,NULL)||calls) return 1;
    w[7]=16;
    if(cg_arith_emit(&c,CG_ARITH_MUL,4,5,6,w,NULL)||calls) return 1;
    w[7]=14;
    if(cg_arith_emit(&c,CG_ARITH_MUL,4,5,6,w,NULL)||calls!=1) return 1;
    puts("PASS contrato y fallo del emisor"); return 0;
}
int main(int argc,char **argv) {
    if(argc==2 && !strcmp(argv[1],"--contract")) return contracts();
    if(argc!=2) return 2;
    const char *names[]={"mul","div","mod","pow"}; int kind=-1;
    for(int i=0;i<4;++i) if(!strcmp(argv[1],names[i])) kind=i;
    if(kind<0) return 2;
    int w[8]={7,8,9,10,11,12,13,14}; CGArithContext c={emit,stdout,0};
    /* Two expansions test uniqueness of labels across expressions. */
    for(int i=0;i<2;++i)
        if(!cg_arith_emit(&c,(CGArithKind)kind,4,5,6,w,"error_aritmetico")) return 1;
    return ferror(stdout)?1:0;
}
