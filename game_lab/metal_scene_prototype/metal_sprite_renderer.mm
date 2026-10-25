// SPDX-License-Identifier: MIT
#include "metal_sprite_renderer.hpp"
#import <Metal/Metal.h>
#include <chrono>
#include <map>

namespace msp {
namespace {
// Shared vertex stage and the fixed-function fragment. Positions are integer
// pixel edges computed on the CPU; fragment centres sit at i+0.5, so a quad
// [left,left+w*scale) covers exactly those pixel columns. Texels are fetched
// with integer division, never through a sampler or interpolated UVs.
const char* kBaseShader=R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Item { int2 origin; uint2 size; uint scale; uint pad; uint2 target; };
struct VOut { float4 position [[position]]; };
vertex VOut sprite_quad(uint vid [[vertex_id]], constant Item& item [[buffer(0)]]) {
    float2 corner = float2(float(vid & 1u), float(vid >> 1u));
    float2 p = float2(item.origin) + corner * float2(item.size * item.scale);
    VOut o;
    o.position = float4(p.x / float(item.target.x) * 2.0 - 1.0, 1.0 - p.y / float(item.target.y) * 2.0, 0.0, 1.0);
    return o;
}
static inline bool fetch(float4 pos, constant Item& item, texture2d<float, access::read> sprite, thread float4& c) {
    int2 local = int2(floor(pos.xy)) - item.origin;
    if (local.x < 0 || local.y < 0) return false;
    uint2 t = uint2(local) / item.scale;
    if (t.x >= item.size.x || t.y >= item.size.y) return false;
    c = sprite.read(t);
    return true;
}
fragment float4 fixed_fn(VOut in [[stage_in]], constant Item& item [[buffer(0)]],
                         texture2d<float, access::read> sprite [[texture(0)]]) {
    float4 c;
    if (!fetch(in.position, item, sprite, c) || c.a == 0.0) discard_fragment();
    return c; // blend: one, oneMinusSourceAlpha (rgb and alpha)
}
)METAL";
// Programmable blending: read the destination in the fragment shader and apply
// the CPU integer formula verbatim. Compiled separately so an unsupported
// [[color(0)]] read cannot take the fixed-function path down with it.
const char* kProgrammableShader=R"METAL(
#include <metal_stdlib>
using namespace metal;
struct Item { int2 origin; uint2 size; uint scale; uint pad; uint2 target; };
struct VOut { float4 position [[position]]; };
fragment float4 programmable(VOut in [[stage_in]], float4 dst [[color(0)]], constant Item& item [[buffer(0)]],
                             texture2d<float, access::read> sprite [[texture(0)]]) {
    int2 local = int2(floor(in.position.xy)) - item.origin;
    if (local.x < 0 || local.y < 0) discard_fragment();
    uint2 t = uint2(local) / item.scale;
    if (t.x >= item.size.x || t.y >= item.size.y) discard_fragment();
    uint4 s = uint4(rint(sprite.read(t) * 255.0));
    if (s.a == 0u) discard_fragment();
    uint4 d = uint4(rint(dst * 255.0));
    uint4 o = s + (d * (255u - s.a) + 127u) / 255u;
    return float4(min(o, uint4(255u))) / 255.0; // valid premultiplied input never reaches the clamp
}
)METAL";
struct ItemParams { int32_t ox,oy; uint32_t w,h,scale,pad,tw,th; };
static_assert(sizeof(ItemParams)==32,"Metal Item layout");
double nowMs(){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();}
std::string describe(NSError* e){return e?std::string(e.localizedDescription.UTF8String):std::string("unknown error");}
MTLPixelFormat pixelFormat(TargetFormat f){return f==TargetFormat::BGRA8Unorm?MTLPixelFormatBGRA8Unorm:MTLPixelFormatBGRA8Unorm_sRGB;}
}
struct MetalSpriteRenderer::Impl {
    id<MTLDevice> device;id<MTLCommandQueue> queue;id<MTLLibrary> base,programmable;
    DeviceReport report;
    std::map<std::pair<int,int>,id<MTLRenderPipelineState>> pipelines;
    std::map<std::pair<int,int>,std::string> failures;
};
MetalSpriteRenderer::MetalSpriteRenderer():impl_(new Impl){
    auto& r=impl_->report;
    impl_->device=MTLCreateSystemDefaultDevice();
    if(!impl_->device){r.reason="MTLCreateSystemDefaultDevice returned nil";return;}
    id<MTLDevice> d=impl_->device;r.device=true;r.name=d.name.UTF8String;r.registryId=d.registryID;
    r.unifiedMemory=d.hasUnifiedMemory;r.lowPower=d.lowPower;r.removable=d.removable;r.recommendedWorkingSet=d.recommendedMaxWorkingSetSize;
    const std::pair<MTLGPUFamily,const char*> families[]={{MTLGPUFamilyApple1,"Apple1"},{MTLGPUFamilyApple2,"Apple2"},{MTLGPUFamilyApple3,"Apple3"},
        {MTLGPUFamilyApple4,"Apple4"},{MTLGPUFamilyApple5,"Apple5"},{MTLGPUFamilyApple6,"Apple6"},{MTLGPUFamilyApple7,"Apple7"},
        {MTLGPUFamilyApple8,"Apple8"},{MTLGPUFamilyApple9,"Apple9"},{MTLGPUFamilyMac2,"Mac2"},
        {MTLGPUFamilyCommon1,"Common1"},{MTLGPUFamilyCommon2,"Common2"},{MTLGPUFamilyCommon3,"Common3"},{MTLGPUFamilyMetal3,"Metal3"}};
    for(const auto& f:families)if([d supportsFamily:f.first])r.families.push_back(f.second);
    impl_->queue=[d newCommandQueue];
    if(!impl_->queue){r.reason="newCommandQueue failed";return;}
    NSError* error=nil;
    impl_->base=[d newLibraryWithSource:@(kBaseShader) options:nil error:&error];
    r.baseCompile=impl_->base?"ok":describe(error);
    error=nil;impl_->programmable=[d newLibraryWithSource:@(kProgrammableShader) options:nil error:&error];
    r.programmableCompile=impl_->programmable?"ok":describe(error);
    if(!impl_->base)r.reason="base shader compile failed: "+r.baseCompile;
}
MetalSpriteRenderer::~MetalSpriteRenderer()=default;
bool MetalSpriteRenderer::available() const{return impl_->device&&impl_->queue&&impl_->base;}
DeviceReport MetalSpriteRenderer::report() const{return impl_->report;}
bool MetalSpriteRenderer::pathAvailable(BlendPath path,TargetFormat format,std::string& why){
    const auto key=std::make_pair(int(path),int(format));
    if(impl_->pipelines.count(key))return true;
    if(impl_->failures.count(key)){why=impl_->failures[key];return false;}
    if(!available()){why=impl_->report.reason;return false;}
    id<MTLLibrary> fragmentLibrary=path==BlendPath::FixedFunction?impl_->base:impl_->programmable;
    if(!fragmentLibrary){why=impl_->failures[key]="programmable shader compile failed: "+impl_->report.programmableCompile;return false;}
    MTLRenderPipelineDescriptor* desc=[MTLRenderPipelineDescriptor new];
    desc.vertexFunction=[impl_->base newFunctionWithName:@"sprite_quad"];
    desc.fragmentFunction=[fragmentLibrary newFunctionWithName:path==BlendPath::FixedFunction?@"fixed_fn":@"programmable"];
    if(!desc.vertexFunction||!desc.fragmentFunction){why=impl_->failures[key]="shader function missing";return false;}
    auto* color=desc.colorAttachments[0];color.pixelFormat=pixelFormat(format);
    if(path==BlendPath::FixedFunction){
        color.blendingEnabled=YES;color.rgbBlendOperation=color.alphaBlendOperation=MTLBlendOperationAdd;
        color.sourceRGBBlendFactor=color.sourceAlphaBlendFactor=MTLBlendFactorOne;
        color.destinationRGBBlendFactor=color.destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
    }else color.blendingEnabled=NO;
    NSError* error=nil;id<MTLRenderPipelineState> state=[impl_->device newRenderPipelineStateWithDescriptor:desc error:&error];
    if(!state){why=impl_->failures[key]="pipeline creation failed: "+describe(error);return false;}
    impl_->pipelines[key]=state;return true;
}
GpuResult MetalSpriteRenderer::render(const Scene& scene,BlendPath path,TargetFormat format,unsigned iterations){
    GpuResult out;
    if(const auto invalid=validateScene(scene);!invalid.empty()){out.error="invalid scene: "+invalid;return out;}
    if(!pathAvailable(path,format,out.error))return out;
    if(!iterations)iterations=1;
    id<MTLDevice> device=impl_->device;id<MTLRenderPipelineState> state=impl_->pipelines[{int(path),int(format)}];
    @autoreleasepool {
        const double uploadStart=nowMs();
        NSMutableArray<id<MTLTexture>>* textures=[NSMutableArray array];
        for(const auto& sprite:scene.sprites){
            MTLTextureDescriptor* desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                width:sprite.width height:sprite.height mipmapped:NO];
            desc.usage=MTLTextureUsageShaderRead;desc.storageMode=MTLStorageModeShared;
            id<MTLTexture> texture=[device newTextureWithDescriptor:desc];
            if(!texture){out.error="sprite texture allocation failed";return out;}
            [texture replaceRegion:MTLRegionMake2D(0,0,sprite.width,sprite.height) mipmapLevel:0
                withBytes:sprite.pixels.data() bytesPerRow:size_t(sprite.width)*4];
            out.uploadBytes+=sprite.pixels.size()*4;[textures addObject:texture];
        }
        MTLTextureDescriptor* targetDesc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat(format)
            width:scene.width height:scene.height mipmapped:NO];
        targetDesc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;targetDesc.storageMode=MTLStorageModeShared;
        id<MTLTexture> target=[device newTextureWithDescriptor:targetDesc];
        if(!target){out.error="shared render target allocation failed";return out;}
        out.uploadMs=nowMs()-uploadStart;
        const auto order=paintOrder(scene);out.draws=order.size();
        const MTLRegion whole=MTLRegionMake2D(0,0,scene.width,scene.height);
        for(unsigned iteration=0;iteration<iterations;++iteration){
            GpuIteration timing;
            double t=nowMs();
            [target replaceRegion:whole mipmapLevel:0 withBytes:scene.initial.data() bytesPerRow:size_t(scene.width)*4];
            timing.targetResetMs=nowMs()-t;
            t=nowMs();
            id<MTLCommandBuffer> command=[impl_->queue commandBuffer];
            MTLRenderPassDescriptor* pass=[MTLRenderPassDescriptor renderPassDescriptor];
            pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;
            pass.colorAttachments[0].storeAction=MTLStoreActionStore;
            id<MTLRenderCommandEncoder> encoder=[command renderCommandEncoderWithDescriptor:pass];
            if(!command||!encoder){out.error="command buffer/encoder creation failed";return out;}
            [encoder setRenderPipelineState:state];
            for(const auto& item:order){
                const auto& sprite=scene.sprites[item.sprite];
                const ItemParams params{item.left,item.top,sprite.width,sprite.height,scene.scale,0,scene.width,scene.height};
                [encoder setVertexBytes:&params length:sizeof params atIndex:0];
                [encoder setFragmentBytes:&params length:sizeof params atIndex:0];
                [encoder setFragmentTexture:textures[item.sprite] atIndex:0];
                [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
            }
            [encoder endEncoding];
            timing.encodeMs=nowMs()-t;
            t=nowMs();[command commit];[command waitUntilCompleted];timing.waitMs=nowMs()-t;
            if(command.status!=MTLCommandBufferStatusCompleted){out.error="command buffer failed: "+describe(command.error);return out;}
            timing.gpuMs=(command.GPUEndTime-command.GPUStartTime)*1000.0;
            t=nowMs();out.pixels.assign(size_t(scene.width)*scene.height,0);
            [target getBytes:out.pixels.data() bytesPerRow:size_t(scene.width)*4 fromRegion:whole mipmapLevel:0];
            timing.readbackMs=nowMs()-t;
            out.iterations.push_back(timing);
        }
    }
    out.ok=true;return out;
}
} // namespace msp
