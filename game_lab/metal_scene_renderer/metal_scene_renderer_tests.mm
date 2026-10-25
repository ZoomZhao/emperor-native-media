// SPDX-License-Identifier: MIT
// Isolated tests for game_lab::metal_scene::Renderer.
//   (no args)  authored textures/scenes only; never reads original data
//   --real     EMPEROR_GAMEDATA_ROOT scenes captured through the existing
//              stage45 SceneDrawList path; unset -> SKIP (exit 0)
// Output: one JSON object per line. Exit 0 pass/SKIP, 1 failure, 2 no Metal capability.
#include "metal_scene_renderer.hpp"
#import <Metal/Metal.h>
#include "../native_session_source.hpp"
#include "../app/map_surface.hpp"
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <pthread.h>

using namespace game_lab;
using namespace game_lab::metal_scene;
namespace {
size_t checks=0,failures=0;std::atomic<size_t> callbacksOnMain{0},callbacksTotal{0};
struct Failure : std::runtime_error {using std::runtime_error::runtime_error;};
void need(bool ok,const std::string& why){++checks;if(!ok){++failures;throw Failure(why);}}
SG3DecodedSprite own(uint32_t w,uint32_t h,std::vector<uint32_t> pixels){
    auto p=std::make_shared<SG3DecodeResult>();p->status=SG3DecodeStatus::Decoded;p->image.width=w;p->image.height=h;p->image.pixelsBGRA=std::move(pixels);
    return identifySG3Sprite(p);
}
uint32_t bgra(uint32_t b,uint32_t g,uint32_t r,uint32_t a){return b|(g<<8)|(r<<16)|(a<<24);}
SceneFrameTicket ticket(uint64_t serial,uint32_t w,uint32_t h){return {{1,serial,serial*7,1,1},serial,w,h};}

// Collects completions per requestSerial and checks exactly-once delivery.
struct Collector {
    std::mutex m;std::condition_variable cv;std::map<uint64_t,std::vector<Completion>> bySerial;size_t total=0;
    CompletionHandler handler(){return [this](Completion c){++callbacksTotal;if(pthread_main_np())++callbacksOnMain;std::lock_guard<std::mutex> l(m);bySerial[c.ticket.requestSerial].push_back(std::move(c));++total;cv.notify_all();};}
    bool waitTotal(size_t n,double seconds=10){std::unique_lock<std::mutex> l(m);return cv.wait_for(l,std::chrono::duration<double>(seconds),[&]{return total>=n;});}
    Completion one(uint64_t serial){std::lock_guard<std::mutex> l(m);auto& v=bySerial[serial];need(v.size()==1,"exactly one completion for serial "+std::to_string(serial)+" got "+std::to_string(v.size()));return v.front();}
};
struct Diff {size_t pixels=0,channels[4]={0,0,0,0};};
Diff compare(const std::vector<uint32_t>& a,const std::vector<uint32_t>& b){
    need(a.size()==b.size(),"comparison dimensions");Diff d;
    for(size_t i=0;i<a.size();++i)if(a[i]!=b[i]){++d.pixels;for(unsigned c=0;c<4;++c)d.channels[c]+=((a[i]>>(8*c))&255)!=((b[i]>>(8*c))&255);}
    return d;
}
std::string diffJson(const Diff& d,size_t denominator){
    std::ostringstream o;o<<"\"diff_pixels\":"<<d.pixels<<",\"pixels\":"<<denominator<<",\"diff_channels_bgra\":["<<d.channels[0]<<","<<d.channels[1]<<","<<d.channels[2]<<","<<d.channels[3]<<"]";return o.str();
}
// Renders synchronously for tests: submit, wait for its single completion, read back.
std::shared_ptr<const Frame> renderNow(Renderer& r,Collector& c,std::shared_ptr<const SceneDrawList> list,uint64_t serial){
    const auto before=c.total;
    const auto s=r.submit(list,ticket(serial,list->width,list->height),c.handler());
    need(s.status==SubmitStatus::Accepted,std::string("submit accepted: ")+submitStatusName(s.status)+" "+s.reason);
    need(c.waitTotal(before+1),"completion arrives");
    const auto done=c.one(serial);
    need(done.status==CompletionStatus::Ready,std::string("ready: ")+completionStatusName(done.status)+" "+done.reason);
    need(done.ticket==ticket(serial,list->width,list->height),"ticket echoed unchanged");
    need(done.frame&&done.frame->width()==list->width&&done.frame->height()==list->height&&std::string(done.frame->pixelFormatName())=="BGRA8Unorm","frame size/format");
    return done.frame;
}
std::vector<uint32_t> readback(const Frame& f){DecodedImage out;std::string error;need(readFrameForTesting(f,out,error),"readback: "+error);return out.pixelsBGRA;}
std::shared_ptr<const SceneDrawList> share(SceneDrawList l){return std::make_shared<const SceneDrawList>(std::move(l));}

SceneDrawList randomList(uint32_t seed,uint32_t w,uint32_t h,size_t spriteCount,size_t itemCount){
    std::mt19937 rng(seed);auto u=[&](uint32_t n){return uint32_t(rng()%n);};
    SceneDrawList l;l.width=w;l.height=h;l.background={w,h,{}};
    for(size_t i=0;i<size_t(w)*h;++i)l.background.pixelsBGRA.push_back(rng());          // arbitrary numerical bytes
    for(size_t s=0;s<spriteCount;++s){const uint32_t sw=1+u(80),sh=1+u(110);std::vector<uint32_t> px;
        for(size_t p=0;p<size_t(sw)*sh;++p){const uint32_t k=u(4),a=k==0?0:k==1?255:1+u(254);px.push_back(a?bgra(u(a+1),u(a+1),u(a+1),a):0);}
        l.sprites.push_back(own(sw,sh,px));}
    for(size_t i=0;i<itemCount;++i){const uint32_t s=u(uint32_t(spriteCount)),scale=1+u(4);const auto& im=l.sprites[s].value->image;
        int32_t left=int32_t(u(w+400))-200,top=int32_t(u(h+400))-200;
        const uint32_t far=u(40);if(far==0){left=1000000;}else if(far==1){left=-1000000;top=-1000000;}else if(far==2){left=-int32_t(im.width*scale)+1;}else if(far==3){top=int32_t(h)-1;}
        else if(far==4){left=30000;}else if(far==5){top=-25000;}
        l.items.push_back({s,left,top,im.width*scale,im.height*scale});}
    need(validateSceneDrawList(l).empty(),"authored random list valid");return l;
}

void capabilityReport(Renderer& r,bool& usable){
    need(r.waitForSelfTestForTesting(30),"self-test finishes");const auto c=r.capabilities();
    std::cout<<"{\"capabilities\":{\"available\":"<<(c.available?"true":"false")<<",\"device\":\""<<c.device<<"\",\"registry\":"<<c.registryId
        <<",\"blend\":\""<<blendPathName(c.blend)<<"\",\"fixed_exact\":"<<c.fixedExact<<",\"programmable_exact\":"<<c.programmableExact
        <<",\"fixed_detail\":\""<<c.fixedDetail<<"\",\"programmable_detail\":\""<<c.programmableDetail<<"\",\"failure\":\""<<c.failure
        <<"\",\"format\":\""<<c.pixelFormat<<"\",\"backend_generation\":"<<c.backendGeneration<<",\"device_generation\":"<<c.deviceGeneration
        <<",\"resource_generation\":"<<c.resourceGeneration<<",\"cache_budget\":"<<c.cacheBudgetBytes<<",\"cache_entries\":"<<c.cacheEntryLimit
        <<",\"max_in_flight\":"<<c.maxInFlight<<",\"max_texture\":"<<c.maxTextureDimension<<"}}\n";
    usable=c.available;
}

void authored(){
    Renderer renderer;bool usable=false;capabilityReport(renderer,usable);
    if(!usable){std::cout<<"{\"RESULT\":\"MISSING_CAPABILITY\",\"failure\":\""<<renderer.capabilities().failure<<"\"}\n";std::exit(2);}
    Collector c;uint64_t serial=100;

    // 1. Two-pass counterexample: flattening onto the background differs.
    {SceneDrawListBuilder b(1,1);auto faint=own(1,1,{0x01000000});b.append(faint,0,0,1);b.append(faint,0,0,1);b.list.background={1,1,{0xff404040}};
     auto list=share(b.list);const auto cpu=rasterizeSceneDrawListCPU(*list);auto frame=renderNow(renderer,c,list,++serial);const auto gpu=readback(*frame);
     const uint32_t flattened=sceneBlendOver(0x01000000,sceneBlendOver(0x01000000,0xff404040));
     need(cpu.pixelsBGRA[0]==0xff3f3f3f&&gpu==cpu.pixelsBGRA,"two-pass GPU equals CPU 0xff3f3f3f");need(flattened!=gpu[0],"flattened single pass would differ");
     std::cout<<"{\"test\":\"two_pass_counterexample\",\"cpu\":\""<<std::hex<<cpu.pixelsBGRA[0]<<"\",\"gpu\":\""<<gpu[0]<<"\",\"flattened\":\""<<flattened<<std::dec<<"\",\"passed\":true}\n";}

    // 2. Mixed per-item scale 1..4, far/large coordinates, clipping, arbitrary background.
    size_t parityPixels=0,parityDiff=0;
    for(uint32_t seed:{1u,2u,3u,4u}){
        const uint32_t w=seed==4?2048:seed==3?127:816,h=seed==4?1536:seed==3?95:620;
        auto list=share(randomList(seed,w,h,48,seed==4?20000:6000));
        std::set<uint32_t> scales;size_t far=0,partial=0;
        for(const auto& i:list->items){scales.insert(i.width/list->sprites[i.sprite].value->image.width);
            far+=i.left>4096||i.left< -4096||i.top>4096||i.top< -4096;partial+=(i.left<0||i.top<0||int64_t(i.left)+i.width>w||int64_t(i.top)+i.height>h)&&!(i.left>=int(w)||i.top>=int(h)||int64_t(i.left)+i.width<=0||int64_t(i.top)+i.height<=0);}
        need(scales.size()==4,"list mixes all four scales");
        const auto cpu=rasterizeSceneDrawListCPU(*list);need(cpu.valid(),"cpu reference");
        auto frame=renderNow(renderer,c,list,++serial);const auto d=compare(cpu.pixelsBGRA,readback(*frame));
        const auto& g=frame->diagnostics();
        std::cout<<"{\"test\":\"mixed_scale_parity\",\"seed\":"<<seed<<",\"size\":["<<w<<","<<h<<"],\"items\":"<<g.items<<",\"drawn\":"<<g.drawnItems<<",\"culled\":"<<g.culledItems
            <<",\"far_origins\":"<<far<<",\"partly_clipped\":"<<partial<<",\"draw_calls\":"<<g.drawCalls<<","<<diffJson(d,cpu.pixelsBGRA.size())
            <<",\"encode_ms\":"<<g.encodeMs<<",\"gpu_ms\":"<<g.gpuMs<<",\"accept_to_complete_ms\":"<<g.acceptToCompleteMs<<"}\n";
        need(d.pixels==0,"mixed-scale GPU equals CPU");need(far>0&&partial>0&&g.culledItems>0,"far/partial/culled exercised");
        parityPixels+=cpu.pixelsBGRA.size();parityDiff+=d.pixels;
    }
    std::cout<<"{\"test\":\"mixed_scale_parity_total\",\"diff_pixels\":"<<parityDiff<<",\"pixels\":"<<parityPixels<<"}\n";
    {Options forced;forced.forceBlendForTesting=BlendPath::Programmable;Renderer prog(forced);bool ok=false;capabilityReport(prog,ok);need(ok&&prog.capabilities().blend==BlendPath::Programmable,"forced programmable path available");
     Collector pc;size_t px=0,diff=0;
     for(uint32_t seed:{1u,4u}){auto list=share(randomList(seed,seed==4?2048:816,seed==4?1536:620,48,seed==4?20000:6000));const auto cpu=rasterizeSceneDrawListCPU(*list);
        auto f=renderNow(prog,pc,list,seed);need(f->diagnostics().blend==BlendPath::Programmable,"frame used programmable");const auto d=compare(cpu.pixelsBGRA,readback(*f));px+=cpu.pixelsBGRA.size();diff+=d.pixels;}
     need(diff==0,"programmable GPU equals CPU");
     std::cout<<"{\"test\":\"programmable_mixed_scale_parity\",\"diff_pixels\":"<<diff<<",\"pixels\":"<<px<<",\"passed\":true}\n";}

    // 3. Cache: warm hits, LRU budget/entry eviction, oversized uncached, invalidation, stale identity.
    {auto list=share(randomList(9,200,150,12,400));const auto cpu=rasterizeSceneDrawListCPU(*list);
     auto first=renderNow(renderer,c,list,++serial);auto warm=renderNow(renderer,c,list,++serial);
     need(first->diagnostics().uploads==first->diagnostics().resourcesDrawn&&first->diagnostics().cacheHits==0,"cold uploads all drawn resources");
     need(warm->diagnostics().uploads==0&&warm->diagnostics().cacheHits==warm->diagnostics().resourcesDrawn,"warm request hits every drawn resource");
     need(compare(cpu.pixelsBGRA,readback(*warm)).pixels==0,"warm pixels equal");
     const auto genBefore=renderer.capabilities().resourceGeneration;renderer.invalidateResources();
     auto after=renderNow(renderer,c,list,++serial);
     need(renderer.capabilities().resourceGeneration==genBefore+1&&after->diagnostics().cacheHits==0&&after->diagnostics().uploads==after->diagnostics().resourcesDrawn,"invalidation forbids reuse");
     need(compare(cpu.pixelsBGRA,readback(*after)).pixels==0,"post-invalidation pixels equal");
     // Same identity, different immutable object: never reuse the old texture.
     auto forged=*list;forged.sprites[0]=SG3DecodedSprite{list->sprites[0].identity,std::make_shared<const SG3DecodeResult>(*own(list->sprites[0].value->image.width,list->sprites[0].value->image.height,
         std::vector<uint32_t>(list->sprites[0].value->image.pixelsBGRA.size(),bgra(1,2,3,255))).value)};
     auto forgedList=share(forged);const auto stale0=renderer.counters().staleReplacements;auto forgedFrame=renderNow(renderer,c,forgedList,++serial);
     need(compare(rasterizeSceneDrawListCPU(*forgedList).pixelsBGRA,readback(*forgedFrame)).pixels==0,"identity reuse with new object renders new pixels");
     need(renderer.counters().staleReplacements==stale0+1,"stale identity replaced, not reused");
     const auto k=renderer.counters();
     std::cout<<"{\"test\":\"cache_hits_invalidation\",\"cold_uploads\":"<<first->diagnostics().uploads<<",\"warm_hits\":"<<warm->diagnostics().cacheHits
         <<",\"after_invalidate_uploads\":"<<after->diagnostics().uploads<<",\"stale_replacements\":"<<k.staleReplacements<<",\"passed\":true}\n";}
    {Options small;small.cacheBudgetBytes=3*64*64*4;small.cacheEntryLimit=2;Renderer tiny(small);bool ok=false;capabilityReport(tiny,ok);need(ok,"small-budget renderer usable");
     Collector tc;SceneDrawList l;l.width=64;l.height=64;l.background={64,64,std::vector<uint32_t>(4096,0xff000000)};
     for(uint32_t s=0;s<5;++s){l.sprites.push_back(own(64,64,std::vector<uint32_t>(4096,bgra(s*40,0,0,255))));l.items.push_back({s,0,0,64,64});}
     l.sprites.push_back(own(128,64,std::vector<uint32_t>(8192,bgra(0,9,0,255))));l.items.push_back({5,0,0,128,64}); // 32 KiB: cacheable here, oversized for the 16 KiB budget below
     auto list=share(l);auto f=renderNow(tiny,tc,list,1);auto k=tiny.counters();
     need(k.retainedEntries<=2&&k.retainedBytes<=small.cacheBudgetBytes&&k.evictions>=4,"entry limit/budget bound residency");
     need(compare(rasterizeSceneDrawListCPU(*list).pixelsBGRA,readback(*f)).pixels==0,"evicted-while-pinned resources still draw exactly");
     Options budget;budget.cacheBudgetBytes=64*64*4;Renderer bounded(budget);capabilityReport(bounded,ok);need(ok,"budget renderer usable");
     Collector bc;auto g=renderNow(bounded,bc,list,1);auto kb=bounded.counters();
     need(kb.uncachedUploads>=1&&kb.retainedBytes<=budget.cacheBudgetBytes,"oversized valid sprite takes bounded uncached path");
     need(compare(rasterizeSceneDrawListCPU(*list).pixelsBGRA,readback(*g)).pixels==0,"uncached path exact");
     std::cout<<"{\"test\":\"cache_budget\",\"evictions\":"<<k.evictions<<",\"retained_entries\":"<<k.retainedEntries<<",\"retained_bytes\":"<<k.retainedBytes
         <<",\"budget\":"<<small.cacheBudgetBytes<<",\"uncached_uploads\":"<<kb.uncachedUploads<<",\"passed\":true}\n";}

    // 4. Frame survives input release, eviction, invalidation, shutdown and renderer destruction.
    {std::shared_ptr<const Frame> kept;std::vector<uint32_t> expected;
     {Options small;small.cacheBudgetBytes=200*1024;Renderer local(small);bool ok=false;capabilityReport(local,ok);need(ok,"local renderer");Collector lc;
      auto list=share(randomList(21,160,120,10,300));expected=rasterizeSceneDrawListCPU(*list).pixelsBGRA;
      kept=renderNow(local,lc,list,1);list.reset();
      for(uint64_t n=2;n<8;++n)renderNow(local,lc,share(randomList(uint32_t(100+n),160,120,10,300)),n); // evicts the first list's textures
      need(local.counters().evictions>0,"later requests evicted earlier textures");
      need(compare(expected,readback(*kept)).pixels==0,"frame unchanged after input release and eviction");
      local.invalidateResources();local.shutdown();need(local.waitIdleForTesting(10),"idle after shutdown");
      need(compare(expected,readback(*kept)).pixels==0,"frame unchanged after invalidate+shutdown");
      const auto k=local.counters();need(k.inFlight==0&&k.pinnedTextures==0&&k.pinnedBytes==0&&k.retainedBytes==0,"no pins/residency after shutdown");}
     need(compare(expected,readback(*kept)).pixels==0,"frame unchanged after renderer destruction");
     std::cout<<"{\"test\":\"frame_survival\",\"checks\":[\"input_release\",\"eviction\",\"invalidate\",\"shutdown\",\"destruction\"],\"passed\":true}\n";}

    // 5. Backpressure: two accepted, third Busy, Busy never calls back.
    {auto list=share(randomList(31,816,620,32,4000));Collector bp;
     renderer.injectFaultForTesting(TestFault::DelayEncodeMs50);
     const auto a=renderer.submit(list,ticket(1001,816,620),bp.handler());const auto b=renderer.submit(list,ticket(1002,816,620),bp.handler());
     const auto x=renderer.submit(list,ticket(1003,816,620),bp.handler());const auto inflight=renderer.counters().inFlight;
     need(a.status==SubmitStatus::Accepted&&b.status==SubmitStatus::Accepted,"first two accepted");
     need(x.status==SubmitStatus::Busy&&x.reason=="max_in_flight"&&inflight==2,"third is Busy max_in_flight with 2 in flight");
     need(renderer.waitIdleForTesting(10),"idle");need(bp.total==2&&bp.bySerial.count(1003)==0,"Busy produced no completion");
     need(bp.one(1001).status==CompletionStatus::Ready&&bp.one(1002).status==CompletionStatus::Ready,"both accepted ready");
     std::cout<<"{\"test\":\"backpressure\",\"accepted\":2,\"busy\":1,\"completions\":"<<bp.total<<",\"passed\":true}\n";}

    // 6. Async failures and refusals.
    {Collector fc;auto list=share(randomList(41,64,48,4,40));
     renderer.injectFaultForTesting(TestFault::FailBeforeCommit);
     need(renderer.submit(list,ticket(2001,64,48),fc.handler()).status==SubmitStatus::Accepted,"fault request accepted");
     renderer.injectFaultForTesting(TestFault::FailAtCompletion);need(renderer.waitIdleForTesting(10),"idle");
     need(renderer.submit(list,ticket(2002,64,48),fc.handler()).status==SubmitStatus::Accepted,"completion-fault request accepted");
     auto bad=*list;bad.sprites[0]=own(1,1,{0x10ff0000});bad.items[0]={0,0,0,1,1};
     need(renderer.submit(share(bad),ticket(2003,64,48),fc.handler()).status==SubmitStatus::Accepted,"invalid pixels detected asynchronously");
     const auto mismatch=renderer.submit(list,ticket(2004,65,48),fc.handler());const auto null=renderer.submit(nullptr,ticket(2005,64,48),fc.handler());
     need(renderer.waitIdleForTesting(10),"idle");
     need(mismatch.status==SubmitStatus::Failed&&mismatch.reason=="ticket_target_mismatch","target mismatch refused");
     need(null.status==SubmitStatus::Failed&&null.reason=="null_draw_list","null list refused");
     const auto f1=fc.one(2001),f2=fc.one(2002),f3=fc.one(2003);
     need(f1.status==CompletionStatus::Failed&&f1.reason=="injected_failure_before_commit"&&!f1.frame,"failure before commit");
     need(f2.status==CompletionStatus::Failed&&f2.reason=="injected_failure_at_completion"&&!f2.frame,"failure at completion");
     need(f3.status==CompletionStatus::Failed&&f3.reason=="draw_list_invalid:sprite_not_premultiplied","non-premultiplied refused, never drawn");
     need(fc.total==3&&!fc.bySerial.count(2004)&&!fc.bySerial.count(2005),"refusals produce no completion");
     const auto k=renderer.counters();need(k.inFlight==0&&k.pinnedTextures==0,"failures release pins");
     std::cout<<"{\"test\":\"async_failures\",\"failed_completions\":3,\"refusals_without_callback\":2,\"passed\":true}\n";}

    // 7. Shutdown races: every accepted request gets exactly one completion.
    {size_t accepted=0,ready=0,cancelled=0,failed=0,closed=0,iterations=40;
     for(size_t it=0;it<iterations;++it){
        Collector sc;std::vector<uint64_t> serials;
        {Renderer r;need(r.waitForSelfTestForTesting(30)&&r.available(),"race renderer");
         auto list=share(randomList(uint32_t(500+it),320,240,16,1500));
         if(it%3==0)r.injectFaultForTesting(TestFault::DelayEncodeMs50);
         for(uint64_t n=1;n<=3;++n){const auto s=r.submit(list,ticket(n,320,240),sc.handler());if(s.status==SubmitStatus::Accepted)serials.push_back(n);}
         if(it%4==1)std::this_thread::sleep_for(std::chrono::microseconds(200*it));
         if(it%5!=4)r.shutdown(); // otherwise destruction performs the shutdown
         if(it%5!=4){const auto s=r.submit(list,ticket(9,320,240),sc.handler());need(s.status==SubmitStatus::Closed,"submit after shutdown is Closed");++closed;}
         if(it%5!=4)need(r.waitIdleForTesting(10),"race idle");}
        need(sc.waitTotal(serials.size(),10),"all accepted completed");std::this_thread::sleep_for(std::chrono::milliseconds(20));
        need(sc.total==serials.size(),"no extra completion");
        for(auto n:serials){const auto d=sc.one(n);accepted++;ready+=d.status==CompletionStatus::Ready;cancelled+=d.status==CompletionStatus::Cancelled;failed+=d.status==CompletionStatus::Failed;
            if(d.status==CompletionStatus::Cancelled)need(d.reason=="backend_shutdown"&&!d.frame,"cancel reason");}
        need(!sc.bySerial.count(9),"Closed produces no completion");
     }
     std::cout<<"{\"test\":\"shutdown_races\",\"iterations\":"<<iterations<<",\"accepted\":"<<accepted<<",\"exactly_once\":"<<accepted<<",\"ready\":"<<ready<<",\"cancelled\":"<<cancelled
         <<",\"failed\":"<<failed<<",\"closed_refusals\":"<<closed<<",\"passed\":true}\n";
     need(cancelled>0,"race exercised cancellation");}

    // 8. Nullable encoder factories: nil -> named Failed exactly once, no Ready/Frame, pins released.
    {const std::pair<TestFault,const char*> faults[]={{TestFault::NilSpriteEncoder,"render_encoder_unavailable:sprite_layer"},
        {TestFault::NilBackgroundBlit,"blit_encoder_unavailable:background"},{TestFault::NilCompositeEncoder,"render_encoder_unavailable:composite"}};
     auto list=share(randomList(61,160,120,8,200));const auto cpu=rasterizeSceneDrawListCPU(*list);size_t failedOnce=0,attempts=0;
     for(const auto& [fault,reason]:faults)for(int rep=0;rep<3;++rep){
        Collector nc;const auto before=renderer.counters();renderer.injectFaultForTesting(fault);++attempts;
        need(renderer.submit(list,ticket(3000+attempts,160,120),nc.handler()).status==SubmitStatus::Accepted,"nil-encoder request accepted");
        need(renderer.waitIdleForTesting(10),"idle after nil encoder");
        const auto d=nc.one(3000+attempts);const auto after=renderer.counters();
        need(d.status==CompletionStatus::Failed&&d.reason==reason&&!d.frame,std::string("named nil-encoder failure ")+reason+" got "+d.reason);
        need(nc.total==1&&after.ready==before.ready&&after.failed==before.failed+1,"exactly one Failed, no Ready");
        need(after.inFlight==0&&after.pinnedTextures==0&&after.pinnedBytes==0,"nil-encoder failure releases in-flight/pinned");
        need(after.liveFrames==before.liveFrames,"no partial Frame created");++failedOnce;}
     auto healthy=renderNow(renderer,c,list,++serial);need(compare(cpu.pixelsBGRA,readback(*healthy)).pixels==0,"renderer healthy after nil-encoder failures");
     size_t raceCancelled=0;
     for(int rep=0;rep<5;++rep){Collector rc;
        {Renderer r;need(r.waitForSelfTestForTesting(30)&&r.available(),"race renderer");
         r.injectFaultForTesting(TestFault::DelayEncodeMs50);r.injectFaultForTesting(TestFault::NilBackgroundBlit);
         need(r.submit(list,ticket(1,160,120),rc.handler()).status==SubmitStatus::Accepted,"race nil request accepted");
         r.shutdown();need(r.waitIdleForTesting(10),"race idle");const auto k2=r.counters();
         need(k2.inFlight==0&&k2.pinnedTextures==0&&k2.pinnedBytes==0&&k2.ready==0,"race releases pins, no Ready");}
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const auto d=rc.one(1);need(rc.total==1&&d.status==CompletionStatus::Cancelled&&d.reason=="backend_shutdown"&&!d.frame,"shutdown wins: exactly one Cancelled");++raceCancelled;}
     std::cout<<"{\"test\":\"nil_encoders\",\"named_failed_exactly_once\":"<<failedOnce<<",\"attempts\":"<<attempts<<",\"shutdown_race_cancelled_once\":"<<raceCancelled<<",\"shutdown_race_attempts\":5,\"passed\":true}\n";}

    // 9. Cache admission/eviction use actual allocatedSize, not w*h*4.
    {id<MTLDevice> device=MTLCreateSystemDefaultDevice();
     MTLTextureDescriptor* d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:64 height:64 mipmapped:NO];
     d.usage=MTLTextureUsageShaderRead;d.storageMode=device.hasUnifiedMemory?MTLStorageModeShared:MTLStorageModeManaged;
     const size_t actual64=[device newTextureWithDescriptor:d].allocatedSize,logical64=64*64*4;
     need(actual64>logical64+1,"device allocates more than w*h*4 for 64x64 (demonstrable)");
     SceneDrawList l;l.width=64;l.height=64;l.background={64,64,std::vector<uint32_t>(4096,0xff101010)};
     for(uint32_t n=0;n<10;++n){l.sprites.push_back(own(64,64,std::vector<uint32_t>(4096,bgra(n*20,n*10,200,255))));l.items.push_back({n,0,0,64,64});}
     auto list=share(l);const auto cpu=rasterizeSceneDrawListCPU(*list);
     Options three;three.cacheBudgetBytes=3*actual64;Renderer r3(three);bool ok=false;capabilityReport(r3,ok);need(ok,"actual-budget renderer");Collector a3;
     auto f=renderNow(r3,a3,list,1);const auto k3=r3.counters();
     const size_t logicalWouldAdmit=three.cacheBudgetBytes/logical64;
     need(k3.retainedEntries==3&&k3.retainedBytes==3*actual64&&k3.retainedLogicalBytes==3*logical64&&k3.evictions==7,"admission/eviction by allocatedSize");
     need(logicalWouldAdmit>3&&k3.peakRetainedBytes<=three.cacheBudgetBytes,"logical accounting would have over-admitted");
     need(f->diagnostics().uploadAllocatedBytes==10*actual64&&f->diagnostics().uploadBytes==10*logical64,"upload bytes actual vs logical");
     need(compare(cpu.pixelsBGRA,readback(*f)).pixels==0,"actual-budget pixels exact");
     Options under;under.cacheBudgetBytes=actual64-1;Renderer ru(under);capabilityReport(ru,ok);need(ok,"under-budget renderer");Collector au;
     auto g=renderNow(ru,au,list,1);const auto ku=ru.counters();
     need(under.cacheBudgetBytes>=logical64&&ku.retainedEntries==0&&ku.retainedBytes==0&&ku.uncachedUploads==10&&ku.uncachedBytes==10*actual64,"logical fits but actual does not -> uncached");
     need(compare(cpu.pixelsBGRA,readback(*g)).pixels==0,"uncached pixels exact");
     MTLTextureDescriptor* rt=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:64 height:64 mipmapped:NO];
     rt.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;rt.storageMode=MTLStorageModePrivate;
     const size_t actualTarget=[device newTextureWithDescriptor:rt].allocatedSize;
     need(f->allocatedBytes()==actualTarget&&g->allocatedBytes()==actualTarget,"Frame bytes are the output's allocatedSize");
     need(f->diagnostics().pinnedTextures==13&&f->diagnostics().pinnedBytes>=10*actual64+actual64+2*actualTarget,"pinned counts sprites+background+layer+output by actual size");
     need(r3.counters().liveFrameBytes==actualTarget&&ru.counters().liveFrameBytes==actualTarget,"liveFrameBytes per held Frame");
     std::cout<<"{\"test\":\"allocated_size_budget\",\"logical_64x64\":"<<logical64<<",\"allocated_64x64\":"<<actual64<<",\"budget\":"<<three.cacheBudgetBytes
         <<",\"retained_entries\":"<<k3.retainedEntries<<",\"logical_would_admit\":"<<logicalWouldAdmit<<",\"retained_bytes\":"<<k3.retainedBytes<<",\"retained_logical\":"<<k3.retainedLogicalBytes
         <<",\"evictions\":"<<k3.evictions<<",\"under_budget\":"<<under.cacheBudgetBytes<<",\"uncached\":"<<ku.uncachedUploads<<",\"uncached_bytes\":"<<ku.uncachedBytes
         <<",\"frame_bytes\":"<<f->allocatedBytes()<<",\"pinned_bytes\":"<<f->diagnostics().pinnedBytes<<",\"passed\":true}\n";

     // 10. Per-request pin budget: bounded, named failure, no Ready, pins released.
     Options pinned;pinned.requestPinBudgetBytes=8*actual64; /* full list: 13 textures; 2-item list: 5 + buffer */Renderer rp(pinned);capabilityReport(rp,ok);need(ok,"pin-budget renderer");Collector pc;
     need(rp.submit(list,ticket(1,64,64),pc.handler()).status==SubmitStatus::Accepted,"over-budget request accepted");need(rp.waitIdleForTesting(10),"idle");
     const auto over=pc.one(1);const auto kp=rp.counters();
     need(over.status==CompletionStatus::Failed&&over.reason=="request_pin_budget_exceeded"&&!over.frame&&kp.ready==0,"pin budget refusal named, no Ready");
     need(kp.inFlight==0&&kp.pinnedBytes==0&&kp.peakPinnedBytes==0&&kp.liveFrames==0,"pin budget failure pins nothing");
     SceneDrawList small=l;small.items.resize(2);auto smallList=share(small);
     auto okFrame=renderNow(rp,pc,smallList,2);need(okFrame->diagnostics().pinnedBytes<=pinned.requestPinBudgetBytes,"within-budget request pinned <= budget");
     need(compare(rasterizeSceneDrawListCPU(*smallList).pixelsBGRA,readback(*okFrame)).pixels==0,"within-budget pixels exact");
     need(rp.waitIdleForTesting(10)&&rp.counters().peakPinnedBytes<=2*pinned.requestPinBudgetBytes,"peak pinned bounded by 2 x request budget");
     std::cout<<"{\"test\":\"request_pin_budget\",\"budget\":"<<pinned.requestPinBudgetBytes<<",\"refusal\":\""<<over.reason<<"\",\"within_budget_pinned\":"<<okFrame->diagnostics().pinnedBytes
         <<",\"peak_pinned\":"<<rp.counters().peakPinnedBytes<<",\"self_test_transient_bytes\":"<<rp.capabilities().selfTestTransientBytes<<",\"passed\":true}\n";}

    need(renderer.waitIdleForTesting(10),"final idle");const auto k=renderer.counters();
    need(k.inFlight==0&&k.pinnedTextures==0&&k.pinnedBytes==0,"no pins at rest");
    std::cout<<"{\"counters\":{\"accepted\":"<<k.accepted<<",\"busy\":"<<k.busy<<",\"refused\":"<<k.refused<<",\"ready\":"<<k.ready<<",\"failed\":"<<k.failed
        <<",\"cancelled\":"<<k.cancelled<<",\"uploads\":"<<k.uploads<<",\"hits\":"<<k.hits<<",\"uncached\":"<<k.uncachedUploads<<",\"evictions\":"<<k.evictions
        <<",\"invalidated\":"<<k.invalidatedEntries<<",\"stale_replacements\":"<<k.staleReplacements<<",\"upload_bytes\":"<<k.uploadBytes<<",\"retained_bytes\":"<<k.retainedBytes
        <<",\"retained_entries\":"<<k.retainedEntries<<",\"retained_logical\":"<<k.retainedLogicalBytes<<",\"peak_retained\":"<<k.peakRetainedBytes<<",\"upload_allocated\":"<<k.uploadAllocatedBytes<<",\"in_flight\":"<<k.inFlight<<",\"pinned\":"<<k.pinnedTextures<<",\"peak_pinned\":"<<k.peakPinnedBytes
        <<",\"live_frames\":"<<k.liveFrames<<"}}\n";
    need(callbacksTotal>0&&callbacksOnMain==0,"completions never run on the main thread");
    std::cout<<"{\"callback_queue\":{\"completions\":"<<callbacksTotal<<",\"on_main_thread\":"<<callbacksOnMain<<"}}\n";
    std::cout<<"{\"authored_checks\":"<<checks<<",\"failures\":"<<failures<<",\"passed\":true}\n";
}

// Same sources/scenarios/cameras as stage45 scene_draw_list_tests.mm --metal;
// the existing CPU renderer image is the oracle (stage45 proved list replay == it).
void realPath(const char* env,BlendPath path){
    Options options;options.forceBlendForTesting=path;Renderer renderer(options);bool usable=false;capabilityReport(renderer,usable);
    if(!usable){std::cout<<"{\"RESULT\":\"MISSING_CAPABILITY\"}\n";std::exit(2);}
    const auto root=ResourceRoot::fromAbsolute(env);std::string error;auto source=loadNativeSessionSource(root,"Save/2026102301/202610230101.sav",error);need(bool(source),error);
    auto native=startNativeSession(source->seed,5);std::vector<NativeSessionState> nativeStates{native};
    for(auto cmd:std::vector<NativeSessionCommand>{{NativeSessionAction::Road,0,81,56},{NativeSessionAction::Road,0,81,58},{NativeSessionAction::Road,0,85,57},{NativeSessionAction::House,0,82,56},{NativeSessionAction::Well,0,82,58},{NativeSessionAction::Hunter,0,84,58}}){
        cmd.expectedRevision=native.revision;need(transactNativeSession(source->seed,native,cmd).committed,"native construction");}
    nativeStates.push_back(native);
    const std::vector<std::string> saves={"Save/2026102301/320111.sav","Save/2026102401/240103.sav"};
    Collector c;uint64_t serial=0;size_t frames=0,pixels=0,diffPixels=0,survived=0,warmHits=0,warmResources=0,overlay=0,clipped=0;
    std::vector<double> gpuMs,encodeMs,completeMs;
    for(size_t scenario=0;scenario<4;++scenario){
        OriginalMapTerrain terrain;OriginalBuildingProfile buildings;OriginalSavedGround ground;std::optional<OriginalFigureProfile> figures;double x=83,y=58;
        if(scenario<2){auto scene=nativeSessionScene(*source,nativeStates[scenario]);need(bool(scene),"native projection");terrain=scene->terrain;buildings=scene->buildings;ground=scene->ground;}
        else{auto save=loadOriginalSave(root.resolve(saves[scenario-2]));need(save.status==OriginalSaveStatus::LoadedSnapshot&&bool(save.layers.ground),"real saved scene");
            terrain=save.terrain;buildings=save.buildings;ground=*save.layers.ground;figures=save.layers.figures;if(!buildings.footprints.empty()){x=buildings.footprints.front().x;y=buildings.footprints.front().y;}}
        auto cache=std::make_shared<SG3SpriteCache>();
        for(unsigned view=0;view<(scenario>=2?6u:4u);++view){
            OriginalMapCamera camera;camera.width=(view==3||view==5)?127:816;camera.height=(view==3||view==5)?95:620;camera.scale=view<4?view+1:view==4?2:4;
            camera.centerX=std::min(double(terrain.activeSide)-1,x+(view==3?.375:0));camera.centerY=std::min(double(terrain.activeSide)-1,y+(view==3?.625:0));
            if(view>=4){need(figures.has_value(),"figure focus source");auto selected=std::find_if(figures->live.begin(),figures->live.end(),[&](const auto& f){return f.image&&f.x>=0&&f.y>=0&&unsigned(f.x)<terrain.activeSide&&unsigned(f.y)<terrain.activeSide;});
                need(selected!=figures->live.end(),"saved figure focus");camera.centerX=selected->x;camera.centerY=selected->y;}
            SceneDrawCapture capture;OriginalBuildingSceneView reference;
            {AppMapSurface surface(root,cache);need(surface.selectSavedBuildings(terrain,buildings,&ground,0,figures?&*figures:nullptr),"prepare original scene");
             reference=surface.renderSavedBuildings(camera,-1,&capture);need(capture.complete(),"draw capture: "+capture.error);need(reference.image.valid(),"CPU reference valid");}
            auto list=share(std::move(capture.list));
            need(compare(reference.image.pixelsBGRA,rasterizeSceneDrawListCPU(*list).pixelsBGRA).pixels==0,"stage45 oracle: list replay equals CPU renderer");
            for(const auto& item:list->items)clipped+=item.left<0||item.top<0||int64_t(item.left)+item.width>camera.width||int64_t(item.top)+item.height>camera.height;
            overlay+=reference.overlayDrawn;
            auto frame=renderNow(renderer,c,list,++serial);
            auto warm=renderNow(renderer,c,list,++serial);warmHits+=warm->diagnostics().cacheHits;warmResources+=warm->diagnostics().resourcesDrawn;
            // Release every input: list, CPU sprite cache, backend cache; the frame must not change.
            list.reset();cache->clear();renderer.invalidateResources();
            const auto d=compare(reference.image.pixelsBGRA,readback(*frame));const auto dw=compare(reference.image.pixelsBGRA,readback(*warm));
            need(d.pixels==0&&dw.pixels==0,"original renderer/persistent Metal pixels differ");++survived;
            const auto& g=frame->diagnostics();gpuMs.push_back(g.gpuMs);encodeMs.push_back(g.encodeMs);completeMs.push_back(g.acceptToCompleteMs);
            std::cout<<"{\"scenario\":"<<scenario<<",\"view\":"<<view<<",\"scale\":"<<camera.scale<<",\"size\":["<<camera.width<<","<<camera.height<<"],\"blend\":\""<<blendPathName(g.blend)
                <<"\",\"items\":"<<g.items<<",\"drawn\":"<<g.drawnItems<<",\"culled\":"<<g.culledItems<<",\"resources\":"<<g.resources<<",\"uploads\":"<<g.uploads<<",\"warm_hits\":"<<warm->diagnostics().cacheHits
                <<",\"warm_uploads\":"<<warm->diagnostics().uploads<<",\"draw_calls\":"<<g.drawCalls<<","<<diffJson(d,reference.image.pixelsBGRA.size())<<",\"warm_diff_pixels\":"<<dw.pixels
                <<",\"encode_ms\":"<<g.encodeMs<<",\"gpu_ms\":"<<g.gpuMs<<",\"accept_to_complete_ms\":"<<g.acceptToCompleteMs<<"}\n";
            ++frames;pixels+=reference.image.pixelsBGRA.size()*2;diffPixels+=d.pixels+dw.pixels;
        }
    }
    need(overlay>0&&clipped>0,"real overlay and clipping exercised");need(warmHits==warmResources,"warm real requests hit every drawn resource");
    need(renderer.waitIdleForTesting(10),"idle");const auto k=renderer.counters();need(k.inFlight==0&&k.pinnedTextures==0,"no pins at rest");
    auto med=[](std::vector<double> v){std::sort(v.begin(),v.end());return v.empty()?0:v[v.size()/2];};
    std::cout<<"{\"blend\":\""<<blendPathName(path)<<"\",\"real_frames\":"<<frames<<",\"compared_frames\":"<<frames*2<<",\"pixels_compared\":"<<pixels<<",\"diff_pixels\":"<<diffPixels<<",\"survived_release\":"<<survived
        <<",\"warm_hits\":"<<warmHits<<",\"warm_resources\":"<<warmResources<<",\"overlay_draws\":"<<overlay<<",\"clipped_items\":"<<clipped
        <<",\"median_encode_ms\":"<<med(encodeMs)<<",\"median_gpu_ms\":"<<med(gpuMs)<<",\"median_accept_to_complete_ms\":"<<med(completeMs)
        <<",\"uploads\":"<<k.uploads<<",\"hits\":"<<k.hits<<",\"evictions\":"<<k.evictions<<",\"peak_retained\":"<<k.peakRetainedBytes<<",\"checks\":"<<checks<<",\"passed\":true}\n";
}
void real(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
    for(auto path:{BlendPath::FixedFunction,BlendPath::Programmable})realPath(env,path);
}
}
int main(int argc,char** argv){
    @autoreleasepool{
        try{if(argc==1)authored();else if(std::string(argv[1])=="--real")real();else{std::cerr<<"usage: [--real]\n";return 1;}return 0;}
        catch(const std::exception& e){std::cerr<<e.what()<<"\n";std::cout<<"{\"failed\":\""<<e.what()<<"\",\"checks\":"<<checks<<"}\n";return 1;}
    }
}
