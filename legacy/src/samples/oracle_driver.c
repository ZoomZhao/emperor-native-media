/* Runs only our authored C sample, with the installed Apple compiler. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "guest_core.c"

static uint32_t expected_value=0x13579bdfu, expected_ticks=0;
static uint32_t expected_checksum=0x2468ace0u, expected_node=0xfffffff0u;
static void expected_step(uint32_t input) {
    uint32_t v=((input>>1)&1) ? (expected_value^input) : (expected_value+input);
    expected_node+=v;
    expected_checksum=expected_checksum*1664525u+(v^expected_node);
    v+=expected_checksum;
    expected_value=v^(expected_node>>3);
    expected_ticks++;
}
int main(void) {
    uint32_t seq[]={0u,3u,0xffffffffu,0x80000002u,2u,0x7fffffffu,0x12345678u};
    uint32_t saved[4];
    for (int i=0;i<7;i++) {
        expected_step(seq[i]);
        if(i==3) { saved[0]=expected_value; saved[1]=expected_ticks; saved[2]=expected_checksum; saved[3]=expected_node; }
    }
    uint32_t digest=guest_run();
    int ok=guest_state.value==expected_value && guest_state.ticks==expected_ticks && guest_state.checksum==expected_checksum && guest_node==expected_node
        && guest_snapshot.state.value==saved[0] && guest_snapshot.state.ticks==saved[1] && guest_snapshot.state.checksum==saved[2] && guest_snapshot.node_value==saved[3]
        && guest_state.node==&guest_node && guest_state.callback==guest_callback && guest_snapshot.state.node==&guest_node && guest_snapshot.state.callback==guest_callback
        && digest==(expected_value^expected_checksum^expected_node^expected_ticks);
    printf("{\"kind\":\"authored_C_arm64_oracle_only\",\"passed\":%s,\"digest\":%u,\"value\":%u,\"ticks\":%u,\"checksum\":%u,\"node\":%u,\"saved\":[%u,%u,%u,%u],\"native_pointer_size\":%zu,\"state_size\":%zu}\n",
      ok?"true":"false",digest,guest_state.value,guest_state.ticks,guest_state.checksum,guest_node,saved[0],saved[1],saved[2],saved[3],sizeof(void*),sizeof(State));
    return ok?0:1;
}
