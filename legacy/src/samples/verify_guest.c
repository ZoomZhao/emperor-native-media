#include <stdio.h>
#include <stdlib.h>
#include "native_cpu.h"
typedef struct { uint32_t value,ticks,checksum,node,callback; } GuestState;
typedef struct { GuestState state; uint32_t node_value; } GuestSnapshot;
_Static_assert(sizeof(GuestState)==20,"Guest pointers must stay 32-bit");
_Static_assert(sizeof(GuestSnapshot)==24,"Guest snapshot layout changed");
extern uint32_t guest_node,guest_operations[2];
extern GuestState guest_state;
extern GuestSnapshot guest_snapshot;
extern void guest_callback(CPU *),guest_add(CPU *),guest_xor(CPU *);
extern void c_guest_run(CPU *),c_guest_step(CPU *),c_guest_save(CPU *),c_guest_scrub(CPU *),c_guest_restore(CPU *);
extern void (*ptr_initialize_pointers)(uint64_t);
static const uint32_t inputs[]={0,3,0xffffffff,0x80000002,2,0x7fffffff,0x12345678};
typedef struct { uint32_t value,ticks,checksum,node; } Oracle;
static Oracle initial(void) { return (Oracle){0x13579bdf,0,0x2468ace0,0xfffffff0}; }
static void advance(Oracle *o,uint32_t input) {
    uint32_t v=((input>>1)&1)?(o->value^input):(o->value+input);
    o->node+=v;o->checksum=o->checksum*1664525+(v^o->node);
    v+=o->checksum;o->value=v^(o->node>>3);o->ticks++;
}
static int state_matches(CPU *c,const Oracle *o) {
    return guest_state.value==o->value && guest_state.ticks==o->ticks && guest_state.checksum==o->checksum && guest_node==o->node
      && guest_state.node==pointer32(c,&guest_node) && guest_state.callback==pointer32(c,guest_callback);
}
static int snapshot_matches(CPU *c,const Oracle *o) {
    return guest_snapshot.state.value==o->value && guest_snapshot.state.ticks==o->ticks && guest_snapshot.state.checksum==o->checksum && guest_snapshot.node_value==o->node
      && guest_snapshot.state.node==pointer32(c,&guest_node) && guest_snapshot.state.callback==pointer32(c,guest_callback);
}
static void step(CPU *c,uint32_t input) {
    c->esp-=4;*(uint32_t*)(uintptr_t)(c->pointer_offset+c->esp)=input;
    c->esp-=4;*(uint32_t*)(uintptr_t)(c->pointer_offset+c->esp)=pointer32(c,&guest_state);
    c_guest_step(c);c->esp+=8;
}
int main(int argc,char **argv) {
    CPU c;
    if(!initialize_cpu(&c,&guest_node) || !ptr_initialize_pointers) return 2;
    ptr_initialize_pointers(c.pointer_offset);
    if(!state_matches(&c,&(Oracle){0x13579bdf,0,0x2468ace0,0xfffffff0})
       || guest_operations[0]!=pointer32(&c,guest_add) || guest_operations[1]!=pointer32(&c,guest_xor)) return 3;
    Oracle o=initial(),saved=o;
    uint32_t initial_esp=c.esp;
    const char *mode=argc>1?argv[1]:"run";
    int ok=1;
    if(!strcmp(mode,"run")) {
        for(int i=0;i<7;i++) { advance(&o,inputs[i]);if(i==3)saved=o; }
        c_guest_run(&c);
        ok=state_matches(&c,&o) && snapshot_matches(&c,&saved) && c.eax==(o.value^o.checksum^o.node^o.ticks);
    } else if(!strcmp(mode,"save") && argc==3) {
        for(int i=0;i<4;i++) { advance(&o,inputs[i]);step(&c,inputs[i]);if(!state_matches(&c,&o))ok=0; }
        c_guest_save(&c);if(!snapshot_matches(&c,&o))ok=0;
        /* Test-driver adapter canonicalizes known pointers to source-image VAs.
           This is explicit runtime glue, not automatic SR save compatibility. */
        GuestSnapshot canonical=guest_snapshot;
        canonical.state.node=0x402000;canonical.state.callback=0x401000;
        FILE *f=fopen(argv[2],"wb");if(!f)return 4;
        if(fwrite(&canonical,1,sizeof(canonical),f)!=sizeof(canonical))ok=0;
        if(fclose(f))ok=0;
    } else if(!strcmp(mode,"resume") && argc==3) {
        GuestSnapshot canonical;
        FILE *f=fopen(argv[2],"rb");if(!f)return 4;
        size_t n=fread(&canonical,1,sizeof(canonical),f);int extra=fgetc(f);fclose(f);
        if(n!=sizeof(canonical)||extra!=EOF||canonical.state.node!=0x402000||canonical.state.callback!=0x401000)return 5;
        for(int i=0;i<4;i++)advance(&o,inputs[i]);
        if(canonical.state.value!=o.value||canonical.state.ticks!=o.ticks||canonical.state.checksum!=o.checksum||canonical.node_value!=o.node)return 6;
        c_guest_scrub(&c);
        if(guest_state.value||guest_state.ticks||guest_state.checksum||guest_state.node||guest_state.callback||guest_node)return 7;
        guest_snapshot=canonical;
        guest_snapshot.state.node=pointer32(&c,&guest_node);
        guest_snapshot.state.callback=pointer32(&c,guest_callback);
        c_guest_restore(&c);if(!state_matches(&c,&o))ok=0;
        for(int i=4;i<7;i++) { advance(&o,inputs[i]);step(&c,inputs[i]);if(!state_matches(&c,&o))ok=0; }
    } else return 8;
    if(c.esp!=initial_esp)ok=0;
    uint32_t digest=guest_state.value^guest_state.checksum^guest_node^guest_state.ticks;
    printf("{\"kind\":\"translated_i386_guest_arm64\",\"mode\":\"%s\",\"passed\":%s,\"digest\":%u,\"value\":%u,\"ticks\":%u,\"checksum\":%u,\"node\":%u,\"guest_state_bytes\":20,\"pointer_bits\":32,\"pointer_offset\":%llu,\"node_offset\":%u,\"callback_offset\":%u,\"persistent_pointer_adapter\":%s}\n",
       mode,ok?"true":"false",digest,guest_state.value,guest_state.ticks,guest_state.checksum,guest_node,(unsigned long long)c.pointer_offset,guest_state.node,guest_state.callback,strcmp(mode,"run")?"true":"false");
    return ok?0:1;
}
