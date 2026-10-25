// SPDX-License-Identifier: MIT
// Normal path rules (checked by run.py): no getBytes, no waitUntilCompleted,
// no CPU pixel conversion, no AppKit/drawable/presentation. The startup
// self-test reads its own authored output through a blit into a shared buffer
// inside its command buffer and compares it in the completion handler.
#include "metal_scene_renderer.hpp"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

namespace game_lab::metal_scene {
namespace {
#include "metal_scene_shaders.inc"
struct Instance { int32_t ox,oy; uint32_t w,h,scale,pad; int32_t x0,y0,x1,y1; };
static_assert(sizeof(Instance)==40,"Metal Instance layout");
std::atomic<uint64_t> gBackendGeneration{0};
double nowMs(){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();}
std::string describe(NSError* e){return e?std::string(e.localizedDescription.UTF8String):std::string("unknown_error");}
size_t textureBytes(uint32_t w,uint32_t h){return size_t(w)*h*4;}
}

const char* submitStatusName(SubmitStatus s){
    switch(s){case SubmitStatus::Accepted:return "accepted";case SubmitStatus::Busy:return "busy";case SubmitStatus::Unsupported:return "unsupported";
        case SubmitStatus::Closed:return "closed";case SubmitStatus::Failed:return "failed";}
    return "invalid_submit_status";
}
const char* completionStatusName(CompletionStatus s){
    switch(s){case CompletionStatus::Ready:return "ready";case CompletionStatus::Failed:return "failed";case CompletionStatus::Cancelled:return "cancelled";}
    return "invalid_completion_status";
}
const char* blendPathName(BlendPath p){return p==BlendPath::FixedFunction?"fixed_function":p==BlendPath::Programmable?"programmable":"none";}

struct LiveFrames {std::atomic<size_t> frames{0},bytes{0};};
struct Frame::Impl {
    id<MTLTexture> texture;uint32_t width=0,height=0;size_t bytes=0;FrameDiagnostics diagnostics;std::shared_ptr<LiveFrames> live;
};
Frame::Frame(std::unique_ptr<Impl> impl):impl_(std::move(impl)){impl_->live->frames+=1;impl_->live->bytes+=impl_->bytes;}
Frame::~Frame(){impl_->live->frames-=1;impl_->live->bytes-=impl_->bytes;}
uint32_t Frame::width() const{return impl_->width;}
uint32_t Frame::height() const{return impl_->height;}
const char* Frame::pixelFormatName() const{return "BGRA8Unorm";}
size_t Frame::allocatedBytes() const{return impl_->bytes;}
const FrameDiagnostics& Frame::diagnostics() const{return impl_->diagnostics;}
id<MTLTexture> Frame::texture() const{return impl_->texture;}

namespace {
struct Job {
    std::shared_ptr<const SceneDrawList> list;SceneFrameTicket ticket;CompletionHandler handler;
    std::atomic<bool> delivered{false};
    uint32_t fault=0; // TestFault bits
    double acceptMs=0;
    bool selfTest=false;BlendPath selfTestPath=BlendPath::None;std::vector<uint32_t> selfTestExpected;
    std::function<void(bool,std::string)> selfTestDone;
    // Everything the command buffer touches, held until completion.
    NSMutableArray* pins=nil;size_t pinnedTextures=0,pinnedBytes=0;
    id<MTLTexture> output=nil;id<MTLBuffer> readback=nil;
    FrameDiagnostics diagnostics;
};
struct CacheEntry {
    id<MTLTexture> texture;size_t bytes=0,logical=0;uint64_t generation=0; // bytes = actual allocatedSize
    std::weak_ptr<const SG3DecodeResult> source;std::list<uint64_t>::iterator age;
};
}

struct Renderer::Core : std::enable_shared_from_this<Renderer::Core> {
    Options options;
    id<MTLDevice> device=nil;id<MTLCommandQueue> queue=nil;
    id<MTLRenderPipelineState> fixedPipeline=nil,programmablePipeline=nil;
    dispatch_queue_t worker=nil,callbacks=nil;
    MTLStorageMode uploadStorage=MTLStorageModeShared;
    mutable std::mutex mutex;mutable std::condition_variable changed;
    Capabilities caps;bool closed=false;Counters counts;std::set<std::shared_ptr<Job>> jobs;uint32_t nextFault=0;
    std::atomic<uint64_t> resourceGeneration{1};
    std::shared_ptr<LiveFrames> live=std::make_shared<LiveFrames>();
    // Worker-queue only.
    std::unordered_map<uint64_t,CacheEntry> cache;std::list<uint64_t> ages;size_t cacheBytes=0,cacheLogicalBytes=0; // actual / logical
    int selfTestRemaining=0,selfTestLists=0,selfTestFixedPassed=0,selfTestProgrammablePassed=0; // guarded by mutex

    void start();
    void runSelfTest();
    void finishSelfTest(BlendPath,bool,std::string);
    std::string encode(const std::shared_ptr<Job>&,BlendPath);
    void complete(const std::shared_ptr<Job>&,id<MTLCommandBuffer>);
    bool deliver(const std::shared_ptr<Job>&,Completion);
    void retire(const std::shared_ptr<Job>&);
    void runJob(const std::shared_ptr<Job>&);
    void publishCache(){
        std::lock_guard<std::mutex> lock(mutex);counts.retainedBytes=cacheBytes;counts.retainedLogicalBytes=cacheLogicalBytes;
        counts.retainedEntries=cache.size();counts.peakRetainedBytes=std::max(counts.peakRetainedBytes,cacheBytes);
    }
    void clearCache(){
        const size_t n=cache.size();cache.clear();ages.clear();cacheBytes=cacheLogicalBytes=0;
        {std::lock_guard<std::mutex> lock(mutex);counts.invalidatedEntries+=n;}publishCache();
    }
    void evictOne(){
        const uint64_t id=ages.front();auto e=cache.find(id);cacheBytes-=e->second.bytes;cacheLogicalBytes-=e->second.logical;ages.pop_front();cache.erase(e);
        {std::lock_guard<std::mutex> lock(mutex);++counts.evictions;}publishCache();
    }
};

void Renderer::Core::start(){
    caps.backendGeneration=caps.deviceGeneration=++gBackendGeneration;
    caps.cacheBudgetBytes=options.cacheBudgetBytes;caps.cacheEntryLimit=options.cacheEntryLimit;caps.requestPinBudgetBytes=options.requestPinBudgetBytes;caps.resourceGeneration=1;
    worker=dispatch_queue_create("game_lab.metal_scene.worker",DISPATCH_QUEUE_SERIAL);
    callbacks=dispatch_queue_create("game_lab.metal_scene.callbacks",DISPATCH_QUEUE_SERIAL);
    device=MTLCreateSystemDefaultDevice();
    if(!device){caps.selfTest=SelfTestState::Failed;caps.failure="no_metal_device";return;}
    caps.device=device.name.UTF8String;caps.registryId=device.registryID;
    caps.maxTextureDimension=[device supportsFamily:MTLGPUFamilyApple3]||[device supportsFamily:MTLGPUFamilyMac2]?16384:8192;
    uploadStorage=device.hasUnifiedMemory?MTLStorageModeShared:MTLStorageModeManaged;
    queue=[device newCommandQueue];
    if(!queue){caps.selfTest=SelfTestState::Failed;caps.failure="command_queue_unavailable";return;}
    NSError* error=nil;
    id<MTLLibrary> base=[device newLibraryWithSource:@(kSceneVertexAndFixedShader) options:nil error:&error];
    if(!base){caps.selfTest=SelfTestState::Failed;caps.failure="shader_compile_failed:"+describe(error);return;}
    const auto pipeline=[&](id<MTLLibrary> library,NSString* fragment,bool blend,std::string& detail)->id<MTLRenderPipelineState>{
        MTLRenderPipelineDescriptor* d=[MTLRenderPipelineDescriptor new];
        d.vertexFunction=[base newFunctionWithName:@"scene_vertex"];d.fragmentFunction=[library newFunctionWithName:fragment];
        if(!d.vertexFunction||!d.fragmentFunction){detail="shader_function_missing";return nil;}
        auto* c=d.colorAttachments[0];c.pixelFormat=MTLPixelFormatBGRA8Unorm;c.blendingEnabled=blend;
        if(blend){c.rgbBlendOperation=c.alphaBlendOperation=MTLBlendOperationAdd;
            c.sourceRGBBlendFactor=c.sourceAlphaBlendFactor=MTLBlendFactorOne;
            c.destinationRGBBlendFactor=c.destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;}
        NSError* e=nil;id<MTLRenderPipelineState> s=[device newRenderPipelineStateWithDescriptor:d error:&e];
        if(!s)detail="pipeline_failed:"+describe(e);return s;
    };
    fixedPipeline=pipeline(base,@"scene_fixed",true,caps.fixedDetail);
    error=nil;id<MTLLibrary> programmable=[device newLibraryWithSource:@(kSceneProgrammableShader) options:nil error:&error];
    if(programmable)programmablePipeline=pipeline(programmable,@"scene_programmable",false,caps.programmableDetail);
    else caps.programmableDetail="shader_compile_failed:"+describe(error);
    if(!fixedPipeline&&!programmablePipeline){caps.selfTest=SelfTestState::Failed;caps.failure="no_blend_pipeline";return;}
    auto self=shared_from_this();
    dispatch_async(worker,^{self->runSelfTest();});
}

namespace {
SG3DecodedSprite authoredSprite(uint32_t w,uint32_t h,std::vector<uint32_t> pixels){
    auto d=std::make_shared<SG3DecodeResult>();d->status=SG3DecodeStatus::Decoded;
    d->image.width=w;d->image.height=h;d->image.pixelsBGRA=std::move(pixels);return identifySG3Sprite(std::move(d));
}
uint32_t bgra(uint32_t b,uint32_t g,uint32_t r,uint32_t a){return b|(g<<8)|(r<<16)|(a<<24);}
// Every (a, c<=a, d) for B/G/R and every (a,d) for A through both passes.
SceneDrawList exhaustiveAlphaList(){
    std::vector<uint32_t> src,dst;
    for(uint32_t a=0;a<256;++a){
        std::vector<std::pair<uint32_t,uint32_t>> pairs;
        for(uint32_t c=0;c<=a;++c)for(uint32_t d=0;d<256;++d)pairs.push_back({c,d});
        for(size_t k=0;k<pairs.size();k+=3){
            const auto p0=pairs[k],p1=pairs[std::min(k+1,pairs.size()-1)],p2=pairs[std::min(k+2,pairs.size()-1)];
            src.push_back(bgra(p0.first,p1.first,p2.first,a));dst.push_back(bgra(p0.second,p1.second,p2.second,uint32_t((k/3)%256)));
        }
    }
    SceneDrawList list;list.width=2048;list.height=uint32_t((src.size()+2047)/2048);
    src.resize(size_t(list.width)*list.height,0);dst.resize(src.size(),0);
    list.background={list.width,list.height,dst};list.sprites={authoredSprite(list.width,list.height,src)};
    list.items={{0,0,0,list.width,list.height}};return list;
}
// Mixed per-item scale 1..4, order/ties, translucent/transparent texels,
// negative, huge (+-1e6) and partially visible large origins.
SceneDrawList orderScaleClipList(){
    SceneDrawList list;list.width=61;list.height=41;std::vector<uint32_t> bg;
    for(uint32_t y=0;y<41;++y)for(uint32_t x=0;x<61;++x)bg.push_back(bgra(x*4,y*6,(x+y)*2,x%7==0?128:255));
    list.background={61,41,bg};
    std::vector<uint32_t> asym;
    for(uint32_t y=0;y<3;++y)for(uint32_t x=0;x<5;++x){const uint32_t a=(x==4&&y==2)?128:255;asym.push_back(bgra(std::min(a,x*50+5),std::min(a,y*100+7),std::min(a,200u),a));}
    asym[7]=0;
    std::vector<uint32_t> wide(1024*4);for(size_t i=0;i<wide.size();++i)wide[i]=bgra(uint32_t(i%251)/2,uint32_t(i%127),60,i%3?255:200);
    list.sprites={authoredSprite(5,3,asym),authoredSprite(4,4,std::vector<uint32_t>(16,bgra(0,0,128,128))),
                  authoredSprite(4,4,std::vector<uint32_t>(16,bgra(200,10,0,255))),authoredSprite(1024,4,wide)};
    for(uint32_t s=1;s<=4;++s){
        list.items.push_back({0,int32_t(3*s)-int32_t(10),int32_t(2*s)-4,5*s,3*s});
        list.items.push_back({1,int32_t(10*s),int32_t(5*s),4*s,4*s});
        list.items.push_back({2,int32_t(10*s)+2,int32_t(5*s)+2,4*s,4*s}); // tie-ish overlap after half-red
        list.items.push_back({0,61-int32_t(5*s)+int32_t(s),41-int32_t(3*s)+1,5*s,3*s});
    }
    list.items.push_back({3,-(1024*4)+3,30,1024*4,4*4});          // 3 visible columns of a 4096-px-wide item
    list.items.push_back({3,58,-(4*3)+2,1024*3,4*3});             // right edge, 2 visible rows
    list.items.push_back({0,1000000,1000000,20,12});list.items.push_back({0,-1000000,-1000000,20,12});
    list.items.push_back({2,61,0,8,8});list.items.push_back({2,0,41,8,8});list.items.push_back({2,-8,0,8,8});
    return list;
}
}

void Renderer::Core::runSelfTest(){
    std::vector<SceneDrawList> lists={exhaustiveAlphaList(),orderScaleClipList()};
    std::vector<std::vector<uint32_t>> expected;
    for(const auto& l:lists){auto r=rasterizeSceneDrawListCPU(l);if(!r.valid()){finishSelfTest(BlendPath::None,false,"self_test_reference_invalid");return;}expected.push_back(std::move(r.pixelsBGRA));}
    std::vector<BlendPath> paths;if(fixedPipeline)paths.push_back(BlendPath::FixedFunction);if(programmablePipeline)paths.push_back(BlendPath::Programmable);
    {std::lock_guard<std::mutex> lock(mutex);selfTestLists=int(lists.size());selfTestRemaining=int(paths.size()*lists.size());}
    for(auto path:paths)for(size_t n=0;n<lists.size();++n){
        auto job=std::make_shared<Job>();job->selfTest=true;job->selfTestPath=path;
        job->list=std::make_shared<const SceneDrawList>(lists[n]);job->selfTestExpected=expected[n];
        auto self=shared_from_this();const std::string name=n?"order_scale_clip":"exhaustive_alpha";
        job->selfTestDone=[self,path,name](bool ok,std::string detail){
            dispatch_async(self->worker,^{self->finishSelfTest(path,ok,ok?std::string():name+":"+detail);});};
        const auto error=encode(job,path);
        if(!error.empty())finishSelfTest(path,false,name+":"+error);
    }
}
void Renderer::Core::finishSelfTest(BlendPath path,bool ok,std::string detail){
    std::lock_guard<std::mutex> lock(mutex);
    if(path==BlendPath::None){selfTestRemaining=0;caps.failure=detail;}
    else{
        auto& passed=path==BlendPath::FixedFunction?selfTestFixedPassed:selfTestProgrammablePassed;
        auto& text=path==BlendPath::FixedFunction?caps.fixedDetail:caps.programmableDetail;
        if(ok)++passed;else text+=(text.empty()?"":";")+detail;
        if(--selfTestRemaining>0)return;
    }
    // A path is exact only if every authored list passed on it.
    caps.fixedExact=fixedPipeline&&selfTestFixedPassed==selfTestLists;
    caps.programmableExact=programmablePipeline&&selfTestProgrammablePassed==selfTestLists;
    if(caps.fixedExact)caps.fixedDetail="exact";
    if(caps.programmableExact)caps.programmableDetail="exact";
    caps.blend=caps.fixedExact?BlendPath::FixedFunction:caps.programmableExact?BlendPath::Programmable:BlendPath::None;
    if(options.forceBlendForTesting!=BlendPath::None){
        const bool exact=options.forceBlendForTesting==BlendPath::FixedFunction?caps.fixedExact:caps.programmableExact;
        caps.blend=exact?options.forceBlendForTesting:BlendPath::None;if(!exact)caps.failure="forced_blend_not_exact";
    }
    caps.available=caps.blend!=BlendPath::None;
    if(caps.available)caps.failure.clear();else if(caps.failure.empty())caps.failure="no_exact_blend_path";
    caps.selfTest=caps.available?SelfTestState::Passed:SelfTestState::Failed;
    changed.notify_all();
}

std::string Renderer::Core::encode(const std::shared_ptr<Job>& job,BlendPath path){
    const double started=nowMs();
    const SceneDrawList& s=*job->list;auto& diag=job->diagnostics;
    diag.backendGeneration=caps.backendGeneration;diag.deviceGeneration=caps.deviceGeneration;diag.blend=path;
    const uint64_t generation=resourceGeneration.load();diag.resourceGeneration=generation;
    // Same checks and order as validateSceneDrawList; the per-pixel premultiplied
    // scan is skipped only for a cache hit on the very same live immutable object
    // (it was scanned before its upload).
    if(!s.width||!s.height||s.width>2048||s.height>2048||!s.background.valid()||s.background.width!=s.width||s.background.height!=s.height)return "draw_list_invalid:draw_target";
    if(s.items.size()>sceneDrawItemLimit||s.sprites.size()>sceneDrawResourceLimit)return "draw_list_invalid:draw_budget";
    std::vector<id<MTLTexture>> textures(s.sprites.size(),nil);std::vector<size_t> hitBytes(s.sprites.size(),0);
    {size_t bytes=0;std::set<uint64_t> ids;
     for(size_t n=0;n<s.sprites.size();++n){const auto& r=s.sprites[n];
        if(!r.identity||!r.value||!r.value->image.valid()||!ids.insert(r.identity).second)return "draw_list_invalid:draw_resource";
        const auto& im=r.value->image;const size_t need=im.pixelsBGRA.capacity()*4;
        if(im.width>4096||im.height>4096||need>sceneDrawPixelBudget-bytes)return "draw_list_invalid:draw_resource_budget";bytes+=need;
        if(!job->selfTest){
            auto found=cache.find(r.identity);
            if(found!=cache.end()){
                if(found->second.generation==generation&&found->second.source.lock()==r.value){
                    textures[n]=found->second.texture;hitBytes[n]=found->second.bytes;ages.splice(ages.end(),ages,found->second.age);continue;}
                cacheBytes-=found->second.bytes;cacheLogicalBytes-=found->second.logical;ages.erase(found->second.age);cache.erase(found);
                {std::lock_guard<std::mutex> lock(mutex);++counts.staleReplacements;}publishCache();
            }
        }
        for(auto p:im.pixelsBGRA)if(!scenePremultiplied(p))return "draw_list_invalid:sprite_not_premultiplied";
     }}
    for(const auto& i:s.items){
        if(i.sprite>=s.sprites.size())return "draw_list_invalid:draw_resource_index";
        const auto& im=s.sprites[i.sprite].value->image;
        if(!i.width||!i.height||i.width%im.width||i.height%im.height||i.width/im.width!=i.height/im.height||i.width/im.width>4)return "draw_list_invalid:draw_scale";
        if(i.left< -1000000||i.left>1000000||i.top< -1000000||i.top>1000000)return "draw_list_invalid:draw_origin";
    }
    if(job->fault&uint32_t(TestFault::DelayEncodeMs50))std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // Cull with the CPU reference's exact condition, then clip each quad to the
    // target so no far-away coordinate reaches the rasterizer.
    std::vector<Instance> instances;std::vector<uint32_t> sprites;instances.reserve(s.items.size()+1);
    std::vector<bool> drawn(s.sprites.size(),false);
    for(const auto& i:s.items){
        const int64_t l=i.left,t=i.top,r=l+i.width,b=t+i.height;
        if(l>=int64_t(s.width)||t>=int64_t(s.height)||r<=0||b<=0){++diag.culledItems;continue;}
        const auto& im=s.sprites[i.sprite].value->image;
        instances.push_back({i.left,i.top,im.width,im.height,i.width/im.width,0,int32_t(std::max<int64_t>(l,0)),int32_t(std::max<int64_t>(t,0)),
            int32_t(std::min<int64_t>(r,s.width)),int32_t(std::min<int64_t>(b,s.height))});
        sprites.push_back(i.sprite);drawn[i.sprite]=true;
    }
    diag.items=s.items.size();diag.drawnItems=instances.size();diag.resources=s.sprites.size();
    NSMutableArray* pins=[NSMutableArray array];size_t pinnedBytes=0;
    // Request pin budget: every texture/buffer this command buffer will hold,
    // counted by actual allocatedSize (cache hits included). Self-test lists are
    // fixed authored sizes, run once at startup, and are exempt.
    const size_t pinLimit=job->selfTest?SIZE_MAX:options.requestPinBudgetBytes;
    const auto describeTexture=[&](uint32_t w,uint32_t h,bool target){
        MTLTextureDescriptor* d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:w height:h mipmapped:NO];
        d.usage=target?(MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead):MTLTextureUsageShaderRead;
        d.storageMode=target?MTLStorageModePrivate:uploadStorage;return d;
    };
    // Heap prediction is only an early reject: it can be SMALLER than the real
    // allocatedSize, which is always re-checked after allocation. A prediction
    // larger than reality could only cause a false (fail-closed) refusal.
    const auto predictedFits=[&](MTLTextureDescriptor* d){return [device heapTextureSizeAndAlignWithDescriptor:d].size<=pinLimit-pinnedBytes;};
    const auto pin=[&](id resource,size_t bytes){if(bytes>pinLimit-pinnedBytes)return false;[pins addObject:resource];pinnedBytes+=bytes;return true;};
    const auto allocate=[&](MTLTextureDescriptor* d,const void* bytes,uint32_t w,uint32_t h)->id<MTLTexture>{
        id<MTLTexture> t=[device newTextureWithDescriptor:d];
        if(t&&bytes)[t replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:bytes bytesPerRow:size_t(w)*4];
        return t;
    };
    for(size_t n=0;n<s.sprites.size();++n){
        if(!drawn[n])continue;++diag.resourcesDrawn;
        const auto& r=s.sprites[n];const auto& im=r.value->image;const size_t logical=textureBytes(im.width,im.height);
        if(textures[n]){++diag.cacheHits;if(!pin(textures[n],hitBytes[n]))return "request_pin_budget_exceeded";continue;}
        if(im.width>caps.maxTextureDimension||im.height>caps.maxTextureDimension)return "sprite_exceeds_device_texture_limit";
        MTLTextureDescriptor* d=describeTexture(im.width,im.height,false);
        if(!predictedFits(d))return "request_pin_budget_exceeded";
        id<MTLTexture> t=allocate(d,im.pixelsBGRA.data(),im.width,im.height);
        if(!t)return "sprite_texture_allocation_failed";
        const size_t actual=t.allocatedSize;
        ++diag.uploads;diag.uploadBytes+=logical;diag.uploadAllocatedBytes+=actual;
        if(!pin(t,actual))return "request_pin_budget_exceeded"; // released here, never cached
        textures[n]=t;
        if(job->selfTest||actual>options.cacheBudgetBytes||!options.cacheEntryLimit){++diag.uncachedUploads;diag.uncachedBytes+=actual;}
        else{
            while(!cache.empty()&&(cacheBytes>options.cacheBudgetBytes-actual||cache.size()>=options.cacheEntryLimit))evictOne();
            ages.push_back(r.identity);cache[r.identity]=CacheEntry{t,actual,logical,generation,r.value,std::prev(ages.end())};
            cacheBytes+=actual;cacheLogicalBytes+=logical;publishCache();
        }
    }
    if(!job->selfTest){std::lock_guard<std::mutex> lock(mutex);
        counts.uploads+=diag.uploads;counts.hits+=diag.cacheHits;counts.uncachedUploads+=diag.uncachedUploads;counts.uncachedBytes+=diag.uncachedBytes;
        counts.uploadBytes+=diag.uploadBytes;counts.uploadAllocatedBytes+=diag.uploadAllocatedBytes;}
    MTLTextureDescriptor* backgroundDesc=describeTexture(s.width,s.height,false);
    if(!predictedFits(backgroundDesc))return "request_pin_budget_exceeded";
    id<MTLTexture> background=allocate(backgroundDesc,s.background.pixelsBGRA.data(),s.width,s.height);
    if(!background)return "background_texture_allocation_failed";
    diag.backgroundBytes=textureBytes(s.width,s.height);
    if(!job->selfTest){std::lock_guard<std::mutex> lock(mutex);counts.uploadBytes+=diag.backgroundBytes;counts.uploadAllocatedBytes+=background.allocatedSize;}
    if(!pin(background,background.allocatedSize))return "request_pin_budget_exceeded";
    MTLTextureDescriptor* rt=describeTexture(s.width,s.height,true);
    if(!predictedFits(rt))return "request_pin_budget_exceeded";
    id<MTLTexture> layer=allocate(rt,nullptr,0,0);
    if(!layer)return "render_target_allocation_failed";
    if(!pin(layer,layer.allocatedSize)||!predictedFits(rt))return "request_pin_budget_exceeded";
    id<MTLTexture> output=allocate(rt,nullptr,0,0);
    if(!output)return "render_target_allocation_failed";
    if(!pin(output,output.allocatedSize))return "request_pin_budget_exceeded";
    instances.push_back({0,0,s.width,s.height,1,0,0,0,int32_t(s.width),int32_t(s.height)}); // composite instance
    id<MTLBuffer> buffer=[device newBufferWithBytes:instances.data() length:instances.size()*sizeof(Instance) options:MTLResourceStorageModeShared];
    if(!buffer)return "instance_buffer_allocation_failed";
    if(!pin(buffer,buffer.allocatedSize))return "request_pin_budget_exceeded";
    id<MTLRenderPipelineState> state=path==BlendPath::FixedFunction?fixedPipeline:programmablePipeline;
    if(!state)return "blend_pipeline_unavailable";
    id<MTLCommandBuffer> command=[queue commandBuffer];if(!command)return "command_buffer_unavailable";
    // Every encoder factory below is nullable (MTLCommandBuffer.h). A nil encoder
    // returns a named failure before commit: the uncommitted command buffer and
    // all locals are released, nothing is pinned, no Frame is produced.
    const uint32_t target[2]={s.width,s.height};
    // Pass 1: sprites, vector order, over transparent black.
    MTLRenderPassDescriptor* first=[MTLRenderPassDescriptor renderPassDescriptor];
    first.colorAttachments[0].texture=layer;first.colorAttachments[0].loadAction=MTLLoadActionClear;
    first.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);first.colorAttachments[0].storeAction=MTLStoreActionStore;
    id<MTLRenderCommandEncoder> encoder=(job->fault&uint32_t(TestFault::NilSpriteEncoder))?nil:[command renderCommandEncoderWithDescriptor:first];
    if(!encoder)return "render_encoder_unavailable:sprite_layer";
    [encoder setRenderPipelineState:state];[encoder setVertexBuffer:buffer offset:0 atIndex:0];[encoder setVertexBytes:target length:sizeof target atIndex:1];
    for(size_t begin=0;begin<sprites.size();){
        size_t end=begin+1;while(end<sprites.size()&&sprites[end]==sprites[begin])++end; // consecutive runs keep order
        [encoder setFragmentTexture:textures[sprites[begin]] atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:end-begin baseInstance:begin];
        ++diag.drawCalls;begin=end;
    }
    [encoder endEncoding];
    // Pass 2: numerical background (exact copy), then the whole layer over it.
    id<MTLBlitCommandEncoder> blit=(job->fault&uint32_t(TestFault::NilBackgroundBlit))?nil:[command blitCommandEncoder];
    if(!blit)return "blit_encoder_unavailable:background";
    [blit copyFromTexture:background toTexture:output];[blit endEncoding];
    MTLRenderPassDescriptor* second=[MTLRenderPassDescriptor renderPassDescriptor];
    second.colorAttachments[0].texture=output;second.colorAttachments[0].loadAction=MTLLoadActionLoad;second.colorAttachments[0].storeAction=MTLStoreActionStore;
    encoder=(job->fault&uint32_t(TestFault::NilCompositeEncoder))?nil:[command renderCommandEncoderWithDescriptor:second];
    if(!encoder)return "render_encoder_unavailable:composite";
    [encoder setRenderPipelineState:state];[encoder setVertexBuffer:buffer offset:0 atIndex:0];[encoder setVertexBytes:target length:sizeof target atIndex:1];
    [encoder setFragmentTexture:layer atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:1 baseInstance:instances.size()-1];
    [encoder endEncoding];++diag.drawCalls;
    if(job->selfTest){
        job->readback=[device newBufferWithLength:textureBytes(s.width,s.height) options:MTLResourceStorageModeShared];
        if(!job->readback)return "self_test_readback_allocation_failed";
        blit=[command blitCommandEncoder];
        if(!blit){job->readback=nil;return "blit_encoder_unavailable:self_test_readback";}
        [blit copyFromTexture:output sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(s.width,s.height,1)
            toBuffer:job->readback destinationOffset:0 destinationBytesPerRow:size_t(s.width)*4 destinationBytesPerImage:textureBytes(s.width,s.height)];
        [blit endEncoding];
    }
    if(job->fault&uint32_t(TestFault::FailBeforeCommit))return "injected_failure_before_commit";
    job->pins=pins;job->output=output;job->pinnedTextures=pins.count-1;job->pinnedBytes=pinnedBytes;
    diag.pinnedTextures=job->pinnedTextures;diag.pinnedBytes=pinnedBytes;
    if(job->selfTest){std::lock_guard<std::mutex> lock(mutex);caps.selfTestTransientBytes+=pinnedBytes+job->readback.allocatedSize;}
    else{std::lock_guard<std::mutex> lock(mutex);counts.pinnedTextures+=job->pinnedTextures;counts.pinnedBytes+=pinnedBytes;
        counts.peakPinnedBytes=std::max(counts.peakPinnedBytes,counts.pinnedBytes);}
    diag.encodeMs=nowMs()-started;
    auto self=shared_from_this();auto held=job;
    [command addCompletedHandler:^(id<MTLCommandBuffer> finished){self->complete(held,finished);}];
    [command commit];
    return {};
}

void Renderer::Core::complete(const std::shared_ptr<Job>& job,id<MTLCommandBuffer> command){
    bool ok=command.status==MTLCommandBufferStatusCompleted;std::string reason=ok?"":"gpu_command_failed:"+describe(command.error);
    if(ok&&(job->fault&uint32_t(TestFault::FailAtCompletion))){ok=false;reason="injected_failure_at_completion";}
    job->diagnostics.gpuMs=(command.GPUEndTime-command.GPUStartTime)*1000.0;
    if(job->selfTest){
        size_t bad=0;
        if(ok&&!job->readback){ok=false;reason="self_test_readback_missing";}
        if(ok){const auto* got=static_cast<const uint32_t*>(job->readback.contents);
            for(size_t i=0;i<job->selfTestExpected.size();++i)bad+=got[i]!=job->selfTestExpected[i];}
        const auto done=job->selfTestDone;job->pins=nil;job->output=nil;job->readback=nil;
        done(ok&&!bad,ok?std::to_string(bad)+"_of_"+std::to_string(job->selfTestExpected.size())+"_pixels_differ":reason);
        return;
    }
    job->diagnostics.acceptToCompleteMs=nowMs()-job->acceptMs;
    if(ok){
        auto impl=std::make_unique<Frame::Impl>();impl->texture=job->output;impl->width=job->list->width;impl->height=job->list->height;
        impl->bytes=job->output.allocatedSize;impl->diagnostics=job->diagnostics;impl->live=live;
        deliver(job,{CompletionStatus::Ready,job->ticket,std::make_shared<const Frame>(std::move(impl)),{}});
    }else deliver(job,{CompletionStatus::Failed,job->ticket,nullptr,reason});
    retire(job);
}
bool Renderer::Core::deliver(const std::shared_ptr<Job>& job,Completion completion){
    if(job->delivered.exchange(true))return false;
    {std::lock_guard<std::mutex> lock(mutex);
     if(completion.status==CompletionStatus::Ready)++counts.ready;else if(completion.status==CompletionStatus::Failed)++counts.failed;else ++counts.cancelled;}
    CompletionHandler handler=std::move(job->handler);job->handler=nullptr;
    dispatch_async(callbacks,^{if(handler)handler(completion);});
    return true;
}
void Renderer::Core::retire(const std::shared_ptr<Job>& job){
    std::lock_guard<std::mutex> lock(mutex);
    counts.pinnedTextures-=job->pinnedTextures;counts.pinnedBytes-=job->pinnedBytes;job->pinnedTextures=job->pinnedBytes=0;
    job->pins=nil;job->output=nil;job->list.reset();
    --counts.inFlight;jobs.erase(job);changed.notify_all();
}
void Renderer::Core::runJob(const std::shared_ptr<Job>& job){
    if(job->delivered.load()){retire(job);return;} // cancelled before encoding: nothing reaches the GPU
    BlendPath path;{std::lock_guard<std::mutex> lock(mutex);path=caps.blend;}
    const auto error=encode(job,path);
    if(!error.empty()){deliver(job,{CompletionStatus::Failed,job->ticket,nullptr,error});retire(job);}
}

Renderer::Renderer(Options options):core_(std::make_shared<Core>()){core_->options=options;core_->start();}
Renderer::~Renderer(){shutdown();}
bool Renderer::available() const{std::lock_guard<std::mutex> lock(core_->mutex);return core_->caps.available;}
Capabilities Renderer::capabilities() const{
    std::lock_guard<std::mutex> lock(core_->mutex);auto c=core_->caps;c.resourceGeneration=core_->resourceGeneration.load();return c;
}
SubmitResult Renderer::submit(std::shared_ptr<const SceneDrawList> list,SceneFrameTicket ticket,CompletionHandler handler){
    auto& c=*core_;std::shared_ptr<Job> job;
    {std::lock_guard<std::mutex> lock(c.mutex);
     const auto refuse=[&](SubmitStatus s,const char* reason,uint64_t& counter){++counter;return SubmitResult{s,reason};};
     if(c.closed)return refuse(SubmitStatus::Closed,"backend_closed",c.counts.closed);
     if(c.caps.selfTest==SelfTestState::Pending)return refuse(SubmitStatus::Busy,"self_test_pending",c.counts.busy);
     if(!c.caps.available){++c.counts.unsupported;return {SubmitStatus::Unsupported,c.caps.failure.empty()?"unsupported":c.caps.failure};}
     if(!list)return refuse(SubmitStatus::Failed,"null_draw_list",c.counts.refused);
     if(!handler)return refuse(SubmitStatus::Failed,"null_completion_handler",c.counts.refused);
     if(!list->width||!list->height||list->width>2048||list->height>2048)return refuse(SubmitStatus::Failed,"draw_target",c.counts.refused);
     if(ticket.width!=list->width||ticket.height!=list->height)return refuse(SubmitStatus::Failed,"ticket_target_mismatch",c.counts.refused);
     if(c.counts.inFlight>=2)return refuse(SubmitStatus::Busy,"max_in_flight",c.counts.busy);
     job=std::make_shared<Job>();job->list=std::move(list);job->ticket=ticket;job->handler=std::move(handler);job->acceptMs=nowMs();
     job->fault=c.nextFault;c.nextFault=0;
     ++c.counts.accepted;++c.counts.inFlight;c.jobs.insert(job);}
    auto core=core_;
    dispatch_async(c.worker,^{core->runJob(job);});
    return {SubmitStatus::Accepted,{}};
}
void Renderer::invalidateResources(){
    auto core=core_;core->resourceGeneration.fetch_add(1);
    dispatch_async(core->worker,^{core->clearCache();});
}
void Renderer::shutdown(){
    std::vector<std::shared_ptr<Job>> pending;
    {std::lock_guard<std::mutex> lock(core_->mutex);core_->closed=true;pending.assign(core_->jobs.begin(),core_->jobs.end());}
    for(const auto& job:pending)core_->deliver(job,{CompletionStatus::Cancelled,job->ticket,nullptr,"backend_shutdown"});
    // Drop resident cache textures once queued work has run; in-flight jobs keep their own pins.
    auto core=core_;
    dispatch_async(core->worker,^{core->clearCache();});
}
Counters Renderer::counters() const{
    std::lock_guard<std::mutex> lock(core_->mutex);auto c=core_->counts;
    c.liveFrames=core_->live->frames.load();c.liveFrameBytes=core_->live->bytes.load();return c;
}
bool Renderer::waitForSelfTestForTesting(double seconds) const{
    std::unique_lock<std::mutex> lock(core_->mutex);
    return core_->changed.wait_for(lock,std::chrono::duration<double>(seconds),[&]{return core_->caps.selfTest!=SelfTestState::Pending;});
}
bool Renderer::waitIdleForTesting(double seconds) const{
    dispatch_sync(core_->worker,^{});
    {std::unique_lock<std::mutex> lock(core_->mutex);
     if(!core_->changed.wait_for(lock,std::chrono::duration<double>(seconds),[&]{return core_->counts.inFlight==0;}))return false;}
    dispatch_sync(core_->worker,^{});dispatch_sync(core_->callbacks,^{});return true;
}
void Renderer::injectFaultForTesting(TestFault fault){std::lock_guard<std::mutex> lock(core_->mutex);core_->nextFault|=uint32_t(fault);}
} // namespace game_lab::metal_scene
