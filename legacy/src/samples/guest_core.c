/* Original research sample, no game code. Freestanding i386 input. */
typedef unsigned int u32;
typedef struct State State;
typedef u32 (*Callback)(State *, u32);
struct State { u32 value, ticks, checksum; u32 *node; Callback callback; };
struct Snapshot { State state; u32 node_value; };

#define NOINLINE __attribute__((noinline))
u32 guest_node = 0xfffffff0u;
NOINLINE u32 guest_callback(State *s, u32 v) {
    s->checksum = s->checksum * 1664525u + (v ^ *s->node);
    return v + s->checksum;
}
State guest_state = {0x13579bdfu, 0u, 0x2468ace0u, &guest_node, guest_callback};
struct Snapshot guest_snapshot;

NOINLINE u32 guest_add(u32 a, u32 b) { return a + b; }
NOINLINE u32 guest_xor(u32 a, u32 b) { return a ^ b; }
u32 (*volatile guest_operations[2])(u32,u32) = {guest_add, guest_xor};

NOINLINE u32 guest_step(State *s, u32 input) {
    u32 v = guest_operations[(input >> 1) & 1](s->value, input);
    *s->node += v;
    v = s->callback(s, v);
    s->value = v ^ (*s->node >> 3);
    s->ticks += 1;
    return v + s->ticks;
}
NOINLINE void guest_save(void) {
    guest_snapshot.state.value = guest_state.value;
    guest_snapshot.state.ticks = guest_state.ticks;
    guest_snapshot.state.checksum = guest_state.checksum;
    guest_snapshot.state.node = guest_state.node;
    guest_snapshot.state.callback = guest_state.callback;
    guest_snapshot.node_value = *guest_state.node;
}
NOINLINE void guest_scrub(void) {
    guest_state.value = guest_state.ticks = guest_state.checksum = 0;
    guest_state.node = (u32 *)0;
    guest_state.callback = (Callback)0;
    guest_node = 0;
}
NOINLINE void guest_restore(void) {
    guest_state.value = guest_snapshot.state.value;
    guest_state.ticks = guest_snapshot.state.ticks;
    guest_state.checksum = guest_snapshot.state.checksum;
    guest_state.node = guest_snapshot.state.node;
    guest_state.callback = guest_snapshot.state.callback;
    *guest_state.node = guest_snapshot.node_value;
}
NOINLINE u32 guest_run(void) {
    guest_step(&guest_state, 0u);
    guest_step(&guest_state, 3u);
    guest_step(&guest_state, 0xffffffffu);
    guest_step(&guest_state, 0x80000002u);
    guest_save();
    guest_scrub();
    guest_restore();
    guest_step(&guest_state, 2u);
    guest_step(&guest_state, 0x7fffffffu);
    guest_step(&guest_state, 0x12345678u);
    return guest_state.value ^ guest_state.checksum ^ guest_node ^ guest_state.ticks;
}
