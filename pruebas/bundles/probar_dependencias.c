#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "codegen_schedule.h"

static int rejected;
static CGInstruction describe(const char *text, Slot slot) {
    CGInstruction op;
    assert(cg_describe_instruction(text, slot, 17, &op) == 0);
    return op;
}
static void bad(CGVerifyKind expected, const CGInstruction *const *ops, int count,
                const CGScheduledBundle *calendar, int size, uint16_t previous) {
    CGScheduleIssue issue;
    assert(cg_verify_schedule(ops,count,calendar,size,previous,&issue)==1);
    if(issue.kind!=expected)fprintf(stderr,"esperado %s, obtenido %s\n",
        cg_verify_kind_name(expected),cg_verify_kind_name(issue.kind));
    assert(issue.kind==expected);
    rejected++;
}
static int callbacks;
static void capture(void *user,const CGScheduledBundle *bundle) { (void)user;(void)bundle;callbacks++; }
int main(int argc,char **argv) {
    CGInstruction producer=describe("mov r4, 1",SLOT_ALU0);
    if(argc==2 && !strcmp(argv[1],"--reject")) {
        producer.result_distance=3; /* Metadatos incompatibles con la ISA. */
        const CGInstruction *ops[]={&producer};
        int failed=cg_schedule_block(ops,1,0,capture,NULL);
        assert(failed==1 && callbacks==0);
        return 0;
    }
    const CGInstruction *ops[3];
    CGScheduledBundle calendar[4];
    CGScheduleIssue issue;
    CGInstruction consumer=describe("add r5, r4, r6",SLOT_ALU0);
    CGInstruction overwrite=describe("mov r4, 2",SLOT_ALU0);
    CGInstruction foreign=describe("mov r6, 3",SLOT_ALU0);
    CGInstruction load=describe("lw r5, 0(rz)",SLOT_LSU);
    CGInstruction store=describe("sw r4, 0(rz)",SLOT_LSU);
    CGInstruction conditional=describe("ifmov r4, r6, r7",SLOT_ALU0);
    CGInstruction jump=describe("jef fin",SLOT_BRU);
    CGInstruction rz=describe("mov rz, 7",SLOT_ALU0);
    CGInstruction read_rz=describe("add r5, rz, rz",SLOT_ALU0);

    /* RAW: ni en el mismo bundle ni en el siguiente; B+2 sí es válido. */
    memset(calendar,0,sizeof calendar);ops[0]=&producer;ops[1]=&consumer;
    calendar[0].slots[0]=&producer;calendar[0].slots[1]=&consumer;
    bad(CG_VERIFY_RAW,ops,2,calendar,1,0);
    calendar[0].slots[1]=NULL;calendar[1].slots[0]=&consumer;
    bad(CG_VERIFY_RAW,ops,2,calendar,2,0);
    calendar[1].slots[0]=NULL;calendar[2].slots[0]=&consumer;
    assert(!cg_verify_schedule(ops,2,calendar,3,0,&issue) && issue.kind==CG_VERIFY_OK);

    /* Una carga, una escritura condicional o un bundle previo son productores. */
    memset(calendar,0,sizeof calendar);ops[0]=&load;
    CGInstruction use_load=describe("add r6, r5, rz",SLOT_ALU0);ops[1]=&use_load;
    calendar[0].slots[3]=&load;calendar[1].slots[0]=&use_load;
    bad(CG_VERIFY_RAW,ops,2,calendar,2,0);
    memset(calendar,0,sizeof calendar);ops[0]=&conditional;ops[1]=&consumer;
    calendar[0].slots[0]=&conditional;calendar[1].slots[0]=&consumer;
    bad(CG_VERIFY_RAW,ops,2,calendar,2,0);
    memset(calendar,0,sizeof calendar);ops[0]=&consumer;calendar[0].slots[0]=&consumer;
    bad(CG_VERIFY_RAW,ops,1,calendar,1,1u<<4);
    calendar[0].slots[0]=NULL;calendar[1].slots[0]=&consumer;
    assert(!cg_verify_schedule(ops,1,calendar,2,1u<<4,NULL));

    /* WAR: sobrescribir r4 antes de la lectura original cambia su entrada. */
    memset(calendar,0,sizeof calendar);ops[0]=&consumer;ops[1]=&overwrite;
    calendar[0].slots[0]=&overwrite;calendar[2].slots[0]=&consumer;
    bad(CG_VERIFY_WAR,ops,2,calendar,3,0);
    calendar[0].slots[0]=&consumer;calendar[1].slots[0]=&overwrite;calendar[2].slots[0]=NULL;
    assert(!cg_verify_schedule(ops,2,calendar,2,0,NULL));

    /* WAW: importa el orden, incluso si nadie lee el valor intermedio. */
    memset(calendar,0,sizeof calendar);ops[0]=&producer;ops[1]=&overwrite;
    calendar[0].slots[0]=&overwrite;calendar[1].slots[0]=&producer;
    bad(CG_VERIFY_WAW,ops,2,calendar,2,0);
    calendar[0].slots[0]=&producer;calendar[0].slots[1]=&overwrite;calendar[1].slots[0]=NULL;
    bad(CG_VERIFY_DESTINATION,ops,2,calendar,1,0);

    /* Memoria: sw -> lw conserva orden; no requiere hueco si no hay RAW de registro. */
    memset(calendar,0,sizeof calendar);ops[0]=&store;ops[1]=&load;
    calendar[0].slots[3]=&load;calendar[1].slots[3]=&store;
    bad(CG_VERIFY_MEMORY,ops,2,calendar,2,0);
    calendar[0].slots[3]=&store;calendar[1].slots[3]=&load;
    assert(!cg_verify_schedule(ops,2,calendar,2,0,NULL));

    memset(calendar,0,sizeof calendar);ops[0]=&producer;ops[1]=&jump;
    calendar[0].slots[2]=&jump;calendar[1].slots[0]=&producer;
    bad(CG_VERIFY_CONTROL,ops,2,calendar,2,0);
    calendar[0].slots[0]=&producer;calendar[1].slots[0]=NULL;
    assert(!cg_verify_schedule(ops,2,calendar,1,0,NULL));

    /* Cobertura y recursos: no omitir, duplicar, inventar ni cambiar de unidad. */
    memset(calendar,0,sizeof calendar);ops[0]=&producer;
    bad(CG_VERIFY_MISSING,ops,1,calendar,0,0);
    calendar[0].slots[0]=&producer;calendar[1].slots[0]=&producer;
    bad(CG_VERIFY_DUPLICATE,ops,1,calendar,2,0);
    calendar[1].slots[0]=NULL;calendar[0].slots[0]=&foreign;
    bad(CG_VERIFY_FOREIGN,ops,1,calendar,1,0);
    calendar[0].slots[0]=&load;ops[0]=&load;
    bad(CG_VERIFY_SLOT,ops,1,calendar,1,0);
    bad(CG_VERIFY_INPUT,NULL,1,calendar,1,0);
    bad(CG_VERIFY_INPUT,ops,257,calendar,1,0);
    bad(CG_VERIFY_INPUT,ops,1,calendar,-1,0);
    bad(CG_VERIFY_INPUT,ops,1,calendar,CG_SCHEDULE_MAX_BUNDLES+1,0);
    CGInstruction invalid=producer;invalid.known=0;ops[0]=&invalid;
    bad(CG_VERIFY_INPUT,ops,1,calendar,1,0);

    /* rz no produce datos: escribir rz no bloquea una lectura del cero constante. */
    memset(calendar,0,sizeof calendar);ops[0]=&rz;ops[1]=&read_rz;
    calendar[0].slots[0]=&rz;calendar[0].slots[1]=&read_rz;
    assert(!cg_verify_schedule(ops,2,calendar,1,0,NULL));
    assert(!cg_verify_schedule(NULL,0,NULL,0,0,NULL));
    printf("PASS dependencias: %d calendarios invalidos rechazados; RAW, WAR, WAW, memoria, control, cobertura y slots\n",rejected);
    return 0;
}
