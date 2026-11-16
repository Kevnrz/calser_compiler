#include "codegen_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int emit(void *user,const CGArithOp *op) {
    FILE *f=user;
    if(op->slot==-1) return fprintf(f,"%s\n",op->text)>=0;
    const char *s[4]={"nopi","nopi","nopi","nopi"};
    if(op->slot<0 || op->slot>3) return 0;
    s[op->slot]=op->text;
    if(fprintf(f,"{ %s | %s | %s | %s }\n",s[0],s[1],s[2],s[3])<0) return 0;
    return fprintf(f,"{ nopi | nopi | nopi | nopi }\n")>=0;
}
static Symbol array(int dims,int rows,int cols,int param,int address) {
    Symbol a={0};
    a.kind=param?SYMBOL_PARAMETER:SYMBOL_VARIABLE; a.type=TYPE_INT;
    a.modifier=MODIFIER_VM; a.dimensions=dims; a.dim_sizes[0]=rows;
    a.dim_sizes[1]=dims==2?cols:0; a.address=address;
    a.width=param?4:rows*(dims==2?cols:1)*4;
    return a;
}
static int reject(void *user,const CGArithOp *op) {
    (void)op; ++*(int *)user; return 0;
}
static int contracts(void) {
    int calls=0,w[2]={9,10},idx[2]={4,5};
    CGArithContext c={reject,&calls,0};
    Symbol a=array(2,3,4,0,256),p=array(2,3,4,1,128);
    if(cg_array_address(&c,&a,idx,1,7,w)||calls) return 1;
    if(cg_array_address(&c,&a,idx,2,4,w)||calls) return 1;
    w[1]=9;
    if(cg_array_address(&c,&a,idx,2,7,w)||calls) return 1;
    w[1]=10; a.modifier=MODIFIER_VI;
    if(cg_array_store(&c,&a,idx,2,7,6,w)||calls) return 1;
    if(cg_array_pass(&c,&a,&p,11)||calls) return 1;
    a.modifier=MODIFIER_VM; p.dim_sizes[1]=5;
    if(cg_array_pass(&c,&a,&p,11)||calls) return 1;
    a.address=257;
    if(cg_array_address(&c,&a,idx,2,7,w)||calls) return 1;
    a.address=256; a.dim_sizes[0]=2147483647;
    if(cg_array_address(&c,&a,idx,2,7,w)||calls) return 1;
    a.dim_sizes[0]=3;
    if(cg_array_address(&c,&a,idx,2,7,w)||calls!=1) return 1;
    puts("PASS contratos y fallo del emisor"); return 0;
}
int main(int argc,char **argv) {
    if(argc==2 && !strcmp(argv[1],"--contract")) return contracts();
    if(argc!=6) return 2;
    int dims=atoi(argv[2]),rows=atoi(argv[3]),cols=atoi(argv[4]),param=atoi(argv[5]);
    if(dims<1 || dims>2 || rows<1 || rows>1024 || cols<1 || cols>1024 ||
       rows>1024/(dims==2?cols:1) || param<0 || param>1) return 2;
    Symbol a=array(dims,rows,cols,param,param?64:256);
    Symbol b=array(dims,rows,cols,1,128);
    int idx[2]={4,5},w[2]={9,10},cp[4]={9,10,11,12},ok=0;
    CGArithContext c={emit,stdout,0};
    if(!strcmp(argv[1],"address")) ok=cg_array_address(&c,&a,idx,dims,7,w);
    else if(!strcmp(argv[1],"load")) ok=cg_array_load(&c,&a,idx,dims,7,8,w);
    else if(!strcmp(argv[1],"store")) {
        ok=cg_array_store(&c,&a,idx,dims,7,6,w);
        if(ok) ok=cg_array_load(&c,&a,idx,dims,7,8,w);
    } else if(!strcmp(argv[1],"pass")) ok=cg_array_pass(&c,&a,&b,11);
    else if(!strcmp(argv[1],"forward")) {
        ok=cg_array_pass(&c,&a,&b,11);
        if(ok) ok=cg_array_load(&c,&b,idx,dims,7,8,w);
    } else if(!strcmp(argv[1],"copy")) {
        ok=cg_array_copy(&c,&b,&a,cp);
        if(ok) ok=cg_array_copy(&c,&b,&a,cp); /* Unique loop labels. */
    } else if(!strcmp(argv[1],"zero")) {
        ok=cg_array_zero(&c,&a,w);
        if(ok) ok=cg_array_zero(&c,&a,w);
    } else if(!strcmp(argv[1],"sameindex")) {
        idx[1]=idx[0]; ok=cg_array_address(&c,&a,idx,dims,7,w);
    } else return 2;
    return ok && !ferror(stdout)?0:1;
}
