// SPDX-License-Identifier: MIT
// Metal source compiled at runtime (no offline metal tool is assumed).
// Derived from game_lab/metal_scene_prototype (verified there), extended with
// per-instance scale and CPU-clipped quads so origins far outside the target
// never reach the rasterizer. Fixed and programmable fragments are separate
// libraries so an unsupported [[color(0)]] read cannot disable the fixed path.
static const char* kSceneVertexAndFixedShader=R"METAL(
#include <metal_stdlib>
using namespace metal;
// origin: unclipped integer top-left; size: texture texels; clip: [min,max) in
// target pixels, already intersected with the target on the CPU.
struct Instance { int2 origin; uint2 size; uint scale; uint pad; int2 clipMin; int2 clipMax; };
struct VOut {
    float4 position [[position]];
    int2 origin [[flat]];
    uint2 size [[flat]];
    uint scale [[flat]];
};
vertex VOut scene_vertex(uint vid [[vertex_id]], uint iid [[instance_id]],
                         const device Instance* items [[buffer(0)]], constant uint2& target [[buffer(1)]]) {
    Instance it = items[iid];
    float2 corner = float2(float(vid & 1u), float(vid >> 1u));
    float2 p = mix(float2(it.clipMin), float2(it.clipMax), corner);
    VOut o;
    o.position = float4(p.x / float(target.x) * 2.0 - 1.0, 1.0 - p.y / float(target.y) * 2.0, 0.0, 1.0);
    o.origin = it.origin; o.size = it.size; o.scale = it.scale;
    return o;
}
fragment float4 scene_fixed(VOut in [[stage_in]], texture2d<float, access::read> sprite [[texture(0)]]) {
    int2 local = int2(floor(in.position.xy)) - in.origin;
    if (local.x < 0 || local.y < 0) discard_fragment();
    uint2 t = uint2(local) / in.scale;
    if (t.x >= in.size.x || t.y >= in.size.y) discard_fragment();
    float4 c = sprite.read(t);
    if (c.a == 0.0) discard_fragment();
    return c; // blend one, oneMinusSourceAlpha on rgb and alpha
}
)METAL";

static const char* kSceneProgrammableShader=R"METAL(
#include <metal_stdlib>
using namespace metal;
struct VOut {
    float4 position [[position]];
    int2 origin [[flat]];
    uint2 size [[flat]];
    uint scale [[flat]];
};
fragment float4 scene_programmable(VOut in [[stage_in]], float4 dst [[color(0)]],
                                   texture2d<float, access::read> sprite [[texture(0)]]) {
    int2 local = int2(floor(in.position.xy)) - in.origin;
    if (local.x < 0 || local.y < 0) discard_fragment();
    uint2 t = uint2(local) / in.scale;
    if (t.x >= in.size.x || t.y >= in.size.y) discard_fragment();
    uint4 s = uint4(rint(sprite.read(t) * 255.0));
    if (s.a == 0u) discard_fragment();
    uint4 d = uint4(rint(dst * 255.0));
    uint4 o = s + (d * (255u - s.a) + 127u) / 255u;
    return float4(min(o, uint4(255u))) / 255.0; // validated premultiplied input never clamps
}
)METAL";
