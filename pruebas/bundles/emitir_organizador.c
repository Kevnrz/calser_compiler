#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "codegen_schedule.h"

static void save(const char *dir, const char *name, const char *suffix) {
    char path[1024];
    int length = snprintf(path, sizeof path, "%s/%s.%s.asm", dir, name, suffix);
    assert(length > 0 && length < (int) sizeof path);
    FILE *out = fopen(path, "w"); assert(out);
    cg_write_listing(out); assert(fclose(out) == 0);
}
static void finish(const char *dir, const char *name) {
    cg_set_line(0);cg_label("fin");cg_bundle(NULL,NULL,"jef fin",NULL);
    save(dir,name,"scalar");
    int ir_before=cg_ir_count();
    assert(cg_schedule()==0 && cg_ir_count()==ir_before);
    save(dir,name,"scheduled");
    int before=cg_bundle_count();assert(cg_schedule()==0 && cg_bundle_count()==before);
    cg_free();
}
static uint32_t random_state;
static unsigned next(void) { random_state=random_state*1664525u+1013904223u;return random_state; }
static CGScheduledBundle captured[10];static int captured_count;
static void capture(void *user, const CGScheduledBundle *bundle) { (void)user;assert(captured_count<10);captured[captured_count++]=*bundle; }
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    CGInstruction read;
    assert(!cg_describe_instruction("add r5, r4, r4",SLOT_ALU0,1,&read));
    const CGInstruction *ops[]={&read};
    assert(!cg_schedule_block(ops,1,1u<<4,capture,NULL));
    assert(captured_count==2 && !captured[0].slots[0] && captured[1].slots[0]==&read);
    assert(cg_schedule_block(ops,257,0,capture,NULL)==1);
    assert(cg_schedule_block(ops,1,0,NULL,NULL)==1);

    cg_init();cg_set_line(1);
    cg_op(SLOT_ALU0,"mov r4, 7");cg_op(SLOT_ALU0,"mov r5, 8");
    cg_op(SLOT_LSU,"sw rz, 0(rz)");cg_op(SLOT_BRU,"jef destino");
    cg_label("destino");finish(argv[1],"four_slots");

    cg_init();cg_set_line(1);cg_op(SLOT_ALU0,"mov r4, 3");
    cg_op(SLOT_ALU0,"add r5, r4, r4");cg_op(SLOT_ALU0,"mov r6, 7");finish(argv[1],"raw_filler");

    cg_init();cg_set_line(1);cg_op(SLOT_ALU0,"mov r4, 1");
    cg_op(SLOT_ALU0,"add r5, r4, r4");cg_op(SLOT_ALU0,"mov r4, 7");
    cg_op(SLOT_LSU,"sw r5, 0(rz)");finish(argv[1],"war");

    cg_init();cg_set_line(1);cg_op(SLOT_ALU0,"mov r4, 1");
    cg_op(SLOT_ALU0,"mov r4, 2");cg_op(SLOT_ALU0,"mov r5, 3");finish(argv[1],"waw");

    cg_init();cg_set_line(1);cg_op(SLOT_ALU0,"mov r4, 7");cg_op(SLOT_LSU,"sw r4, 0(rz)");
    cg_op(SLOT_LSU,"lw r5, 0(rz)");cg_op(SLOT_ALU0,"mov r6, 11");
    cg_op(SLOT_LSU,"sb r6, 1(rz)");cg_op(SLOT_LSU,"lb r7, 1(rz)");finish(argv[1],"memory_alias");

    cg_init();cg_set_line(1);cg_op(SLOT_ALU0,"mov r4, 11");cg_op(SLOT_ALU0,"mov r5, 0");
    cg_op(SLOT_ALU0,"mov r6, 22");cg_op(SLOT_ALU0,"ifmov r4, r6, r5");
    cg_op(SLOT_LSU,"sw r4, 0(rz)");cg_op(SLOT_ALU0,"mov r5, 1");
    cg_op(SLOT_ALU0,"ifmov r4, r6, r5");cg_op(SLOT_LSU,"sw r4, 4(rz)");finish(argv[1],"conditional_write");

    for(int taken=0;taken<2;taken++){
        cg_init();cg_set_line(10);cg_op(SLOT_ALU0,"mov r4, %d",taken);
        cg_set_line(12);cg_op(SLOT_LSU,"sw r4, 0(rz)");
        cg_set_line(13);cg_op(SLOT_BRU,"bne r4, rz, destino");
        cg_set_line(14);cg_op(SLOT_ALU0,"mov r5, 99");cg_op(SLOT_LSU,"sw r5, 4(rz)");
        cg_label("destino");finish(argv[1],taken?"branch_taken":"branch_fallthrough");
    }
    cg_init();cg_op(SLOT_ALU0,"mov r4, 3");cg_label("medio");
    cg_op(SLOT_ALU0,"add r5, r4, r4");finish(argv[1],"label_entry");

    cg_init();cg_op(SLOT_ALU0,"mov r4, 3");cg_gap();
    cg_op(SLOT_ALU0,"add r5, r4, r4");finish(argv[1],"explicit_gap");

    cg_init();cg_set_line(10);cg_bundle("mov r4, 3","mov r5, 4",NULL,NULL);cg_gap();
    cg_op(SLOT_ALU0,"add r6, r4, r5");
    cg_bundle(NULL,NULL,NULL,"sw r6, 0(rz)");finish(argv[1],"fixed_bundle");

    cg_init();cg_bundle("mov r4, 3",NULL,NULL,NULL);cg_label("entrada");
    cg_op(SLOT_ALU0,"add r5, r4, r4");finish(argv[1],"fixed_entry");

    cg_init();cg_bundle("mov r4, 999",NULL,NULL,NULL);cg_op(SLOT_ALU1,"auth r4");
    finish(argv[1],"fixed_state_entry");

    cg_init();cg_op(SLOT_ALU1,"auth r4");cg_op(SLOT_ALU0,"mov r5, 7");
    cg_op(SLOT_ALU1,"logout");finish(argv[1],"state_barrier");

    cg_init();cg_op(SLOT_ALU0,"mov r4, 5");cg_op(SLOT_BRU,"jef 2");
    cg_op(SLOT_ALU0,"mov r5, 6");finish(argv[1],"numeric_fallback");
    cg_init();cg_op(SLOT_ALU0,"desconocida r4");finish(argv[1],"unknown_fallback");

    cg_init();for(int i=0;i<600;i++)cg_op(SLOT_ALU0,"mov r%d, %d",4+i%12,i);
    finish(argv[1],"long_region");
    cg_init();
    for(int slot=0;slot<4;slot++){
        char long_text[240];
        memset(long_text,' ',sizeof long_text);
        const char *mn=slot<2?"mov":slot==2?"lw":"jef";
        memcpy(long_text,mn,strlen(mn));
        if(slot<2)snprintf(long_text+190,sizeof long_text-190,"r%d, %d",4+slot,slot);
        else snprintf(long_text+190,sizeof long_text-190,"%s",slot==2?"r6, 0(rz)":"salto_largo");
        cg_op(slot<2?SLOT_ALU0:slot==2?SLOT_LSU:SLOT_BRU,"%s",long_text);
    }
    cg_label("salto_largo");finish(argv[1],"long_listing");
    const char *alu[]={"add","sub","xor","and","or","sll","srl","sra","ifmov","slt"};
    for(int seed=0;seed<128;seed++){
        cg_init();random_state=(uint32_t)seed+1;
        for(int r=4;r<12;r++)cg_op(SLOT_ALU0,"mov r%d, %u",r,next()%16);
        for(int i=0;i<40;i++){
            unsigned operation=next()%14,d=4+next()%8,a=4+next()%8,b=4+next()%8;
            cg_set_line(i+1);
            if(operation<10)cg_op(SLOT_ALU0,"%s r%u, r%u, r%u",alu[operation],d,a,b);
            else if(operation<12)cg_op(SLOT_LSU,"sw r%u, %u(rz)",a,(next()%8)*4);
            else cg_op(SLOT_LSU,"lw r%u, %u(rz)",d,(next()%8)*4);
        }
        char name[32];snprintf(name,sizeof name,"random_%03d",seed);finish(argv[1],name);
    }
    puts("PASS emisor del organizador: escenarios de dependencias, fronteras y 128 programas aleatorios");
    return 0;
}
