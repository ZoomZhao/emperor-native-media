// SPDX-License-Identifier: MIT
// Exercise actual App transaction methods without creating NSApplication/windows.
// Only the GPU submission and CPU image-allocation boundary are substituted.
#define main emperorAppEntryNotExecuted
#include "main.mm"
#undef main
#include <iostream>
#include <map>
#include <stdexcept>
namespace {std::string v6Summary;size_t checks=0;void need(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}}
@interface NativeVisualHarness:EmperorAppDelegate {
@public
    std::vector<std::shared_ptr<NativeVisualTransaction>> requests;
    bool failCPUImage;
}
- (instancetype)initRoot:(game_lab::ResourceRoot)root source:(std::shared_ptr<const game_lab::NativeSessionSource>)source;
- (void)submitNativeGPU:(std::shared_ptr<NativeVisualTransaction>)request;
- (int64_t)balance;
- (uint64_t)revision;
- (size_t)objects;
- (std::string)serializedState;
- (size_t)queued;
- (std::shared_ptr<NativeVisualTransaction>)pending;
- (void)failGPUAndFinishCPU:(std::shared_ptr<NativeVisualTransaction>)request;
- (void)closeHarness;
- (void)requestFailedReplacement;
- (void)verifyPixelsRetained;
- (void)verifyFeedbackReplacement;
- (void)verifyV6Labor;
@end
@implementation NativeVisualHarness
- (instancetype)initRoot:(game_lab::ResourceRoot)root source:(std::shared_ptr<const game_lab::NativeSessionSource>)source {
    if(!(self=[super init]))return nil;_resourceRoot=root;_nativeSource=source;
    _nativeFrameGate=game_lab::changeSceneFrame({},game_lab::SceneFrameChange::BeginSession).state;
    _nativeCanvas=[[NativeSessionCanvas alloc]initWithFrame:NSMakeRect(0,0,816,620)];
    _nativeCamera.width=816;_nativeCamera.height=620;_nativeCamera.centerX=83;_nativeCamera.centerY=58;_nativeCamera.outlines=false;
    const auto state=game_lab::startNativeSession(source->seed,5);__block unsigned calls=0;
    const auto result=[self beginNativeCandidate:state source:source camera:_nativeCamera completion:^(BOOL ok){need(ok,"initial real CPU candidate publishes");++calls;}];
    need(result==NativeInstallResult::Installed&&calls==1&&_nativeCanvas.image!=nil,"actual synchronous CPU path installed once");CGImageRef cg=[_nativeCanvas.image CGImageForProposedRect:nullptr context:nil hints:nil];CGColorSpaceRef srgb=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);need(cg&&CFEqual(CGImageGetColorSpace(cg),srgb),"actual native CPU image explicitly tags sRGB");CGColorSpaceRelease(srgb);_nativeMetalEnabled=YES;return self;
}
- (void)updateNativeSessionLabels {} // UI labels are GUI acceptance, not this harness.
- (void)submitNativeGPU:(std::shared_ptr<NativeVisualTransaction>)request {requests.push_back(std::move(request));}
- (NSImage*)nativeCPUImage:(const game_lab::DecodedImage&)pixels {return failCPUImage?nil:[super nativeCPUImage:pixels];}
- (int64_t)balance{return _nativeState->funds;}
- (uint64_t)revision{return _nativeState->revision;}
- (size_t)objects{return _nativeState->objects.size();}
- (std::string)serializedState{return game_lab::encodeNativeSession(_nativeSource->seed,*_nativeState);}
- (size_t)queued{return _nativeCommands.size();}
- (std::shared_ptr<NativeVisualTransaction>)pending{return _nativePending;}
- (void)failGPUAndFinishCPU:(std::shared_ptr<NativeVisualTransaction>)request {
    [self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::GPUFailed ticket:request->flow.ticket width:0 height:0];
    [self completeNativeCPU:request];
}
- (void)closeHarness {
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Close).state;
    if(_nativePending){_nativePending->settled=true;_nativePending->completion=nil;_nativePending.reset();}_nativeCommands.clear();
}
- (void)requestFailedReplacement {
    const auto previous=_nativeState;const auto previousBytes=[self serializedState];const auto source=_nativeSource;const auto epoch=_nativeFrameGate.sessionEpoch;
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::BeginSession).state;
    auto bad=*_nativeState;bad.funds=-1;__block unsigned calls=0;
    auto result=[self beginNativeCandidate:bad source:source camera:_nativeCamera completion:^(BOOL ok){need(!ok,"invalid replacement refuses");++calls;}];
    need(result==NativeInstallResult::Failed&&calls==1,"synchronous refusal invokes completion once");
    need(_nativeState->funds==previous->funds&&_nativeState->revision==previous->revision&&_nativeSource==source&&_nativeFrameGate.sessionEpoch>epoch,"failed replacement preserves city with newer epoch");
    need([self serializedState]==previousBytes,"failed replacement preserves the entire encoded native city");
}
- (void)verifyFeedbackReplacement {
    need(_nativeState->objects.size()==4&&_nativeState->revision==4,"feedback collision fixture has four committed roads");
    const auto before=game_lab::encodeNativeSession(_nativeSource->seed,*_nativeState);
    need([self nativeFeedback].objects.size()==4,"feedback initially describes committed city");
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::BeginSession).state;
    need([self nativeFeedback].objects.size()==4,"old city can be inspected during new epoch before publication");
    const auto replacementSource=std::make_shared<const game_lab::NativeSessionSource>(*_nativeSource);
    auto next=game_lab::startNativeSession(replacementSource->seed,5);
    for(auto action:{game_lab::NativeSessionAction::Resume,game_lab::NativeSessionAction::Pause,game_lab::NativeSessionAction::Resume,game_lab::NativeSessionAction::Pause})need(game_lab::transactNativeSession(replacementSource->seed,next,{action,next.revision}).committed,"legitimate same-revision empty replacement");
    const auto result=[self beginNativeCandidate:next source:replacementSource camera:_nativeCamera completion:nil];
    need(result==NativeInstallResult::Pending&&[self nativeFeedback].objects.size()==4,"pending replacement never changes feedback or state");
    need(game_lab::encodeNativeSession(_nativeSource->seed,*_nativeState)==before,"query is read-only during replacement");
    [self flushNativeVisualForUI];need(_nativeState->revision==4&&[self nativeFeedback].objects.empty(),"same epoch/revision new source invalidates old feedback");
}
// v6 through actual App transactions: installs a v6 city, places by nativeAction,
// advances by Tick and publishes each candidate through the real CPU path.
- (void)verifyV6Labor {
    const auto source=_nativeSource;const auto& seed=source->seed;need(seed.laborPercent>0&&seed.hunterWorkers>0,"real source supplies v6 labor inputs");
    __block unsigned calls=0;auto state=game_lab::startNativeSession(seed,game_lab::nativeLatestRulesVersion);need(state.rulesVersion==6,"App default creates v6");
    const auto installed=[self beginNativeCandidate:state source:source camera:_nativeCamera completion:^(BOOL ok){need(ok,"v6 initial publishes");++calls;}];
    if(installed==NativeInstallResult::Pending)[self flushNativeVisualForUI];
    need(installed!=NativeInstallResult::Failed&&calls==1&&_nativeState->rulesVersion==6,"v6 city installed by actual App path");
    unsigned sx=0,sy=0;bool found=false;
    for(unsigned y=50;y+5<seed.side&&!found;++y)for(unsigned x=70;x+12<seed.side&&!found;++x){bool flat=true;
        for(unsigned dy=0;dy<5&&flat;++dy)for(unsigned dx=0;dx<12;++dx)flat&=seed.buildable[size_t(y+dy)*seed.side+x+dx]!=0;if(flat){sx=x;sy=y;found=true;}}
    need(found,"real flat v6 site near camera");
    const auto act=[&](game_lab::NativeSessionAction a,unsigned x,unsigned y){need([self nativeAction:a x:x y:y],"v6 App action accepted");[self flushNativeVisualForUI];};
    for(unsigned dx=0;dx<12;++dx)act(game_lab::NativeSessionAction::Road,sx+dx,sy+2);
    act(game_lab::NativeSessionAction::House,sx,sy);act(game_lab::NativeSessionAction::House,sx+2,sy);
    act(game_lab::NativeSessionAction::Well,sx+4,sy);act(game_lab::NativeSessionAction::Hunter,sx+6,sy);
    auto q=[self nativeFeedback];need(q.error.empty()&&q.labor.available==0&&q.labor.demand==int32_t(seed.hunterWorkers+seed.wellWorkers)&&q.labor.shortage==q.labor.demand,"unstaffed v6 demand from model");
    act(game_lab::NativeSessionAction::Resume,0,0);
    for(int n=0;n<60;++n)act(game_lab::NativeSessionAction::Tick,0,0);
    act(game_lab::NativeSessionAction::Pause,0,0);
    q=[self nativeFeedback];const auto population=game_lab::nativePopulation(*_nativeState);
    need(_nativeState->rulesVersion==6&&population>0&&q.labor.available==int32_t(population*seed.laborPercent/100),"v6 App city grows and labor follows residents");
    need(q.labor.employed+q.labor.shortage==q.labor.demand&&_nativeState->foodProduced>0,"v6 App staffed hunter produces");
    need(_nativeCanvas.image!=nil&&_nativeCanvas.image.size.width==816,"v6 frames published by actual App renderer");
    const auto summary=game_lab::nativeCityFeedbackSummary(*_nativeState,q);need(summary.find("劳力")!=std::string::npos,"shared presentation names labor");
    const auto bytes=game_lab::encodeNativeSession(seed,*_nativeState);std::string error;const auto back=game_lab::decodeNativeSession(seed,bytes,error);
    need(back&&game_lab::encodeNativeSession(seed,*back)==bytes&&bytes.find("\nLABOR ")!=std::string::npos,"v6 App state roundtrips with labor record");
    v6Summary="\"v6_App\":{\"population\":"+std::to_string(population)+",\"labor\":"+std::to_string(q.labor.available)+",\"employed\":"+std::to_string(q.labor.employed)+
        ",\"shortage\":"+std::to_string(q.labor.shortage)+",\"meat_produced\":"+std::to_string(_nativeState->foodProduced)+"},";
}
- (void)verifyPixelsRetained {need(_nativeCanvas.image!=nil&&!_nativePresentedFrame,"CPU fallback retains a concrete image");}
@end
// Queue actual source-worker completions for deterministic late-delivery checks.
// No NSApplication, window, fake source data or background UI mutation.
@interface NativeStartHarness:EmperorAppDelegate {
    NSMutableArray* _deliveries;
}
- (instancetype)initRoot:(game_lab::ResourceRoot)root deliveries:(NSMutableArray*)deliveries;
- (void)runCatalogUntil:(std::shared_ptr<std::atomic<bool>>)done;
- (game_lab::NativeStartWorkPtr)work;
- (void)setupPreviews;
- (void)selectPreview:(NSInteger)index until:(std::shared_ptr<std::atomic<bool>>)done;
- (BOOL)previewReady;
- (NSString*)previewText;
- (BOOL)hasPreview;
@end
@implementation NativeStartHarness
- (instancetype)initRoot:(game_lab::ResourceRoot)root deliveries:(NSMutableArray*)deliveries {
    if(!(self=[super init]))return nil;_resourceRoot=root;_deliveries=deliveries;
    _nativeStartQueue=dispatch_queue_create("org.emperor.test-source-reader",DISPATCH_QUEUE_SERIAL);
    _nativeStartWork=std::make_shared<game_lab::NativeStartWork>();return self;
}
- (void)postNativeStartUI:(dispatch_block_t)block {@synchronized(_deliveries){[_deliveries addObject:[block copy]];}}
- (void)runCatalogUntil:(std::shared_ptr<std::atomic<bool>>)done {
    [self beginNativeStartCatalog];dispatch_async(_nativeStartQueue,^{done->store(true);});
}
- (game_lab::NativeStartWorkPtr)work {return _nativeStartWork;}
- (void)setupPreviews {
    _nativeStartCatalog=game_lab::nativeStartCatalog(_resourceRoot,{"Save/2026102301/202610230101.sav","Save/2026102301/202610230102.sav"});
    need(_nativeStartCatalog.entries.size()==2&&_nativeStartCatalog.entries[0].supported()&&_nativeStartCatalog.entries[1].supported(),"two real preview sources admitted");
    _nativeStartPicker=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(0,0,200,30) pullsDown:NO];[_nativeStartPicker addItemsWithTitles:@[@"first",@"second"]];
    _nativeStartDetail=[NSTextField labelWithString:@""];_nativeStartPreview=[NSImageView new];_nativeStartConfirm=[NSButton new];
}
- (void)selectPreview:(NSInteger)index until:(std::shared_ptr<std::atomic<bool>>)done {
    [_nativeStartPicker selectItemAtIndex:index];[self onNativeStartSelection:nil];
    dispatch_async(_nativeStartQueue,^{done->store(true);});
}
- (BOOL)previewReady {return _nativeStartConfirm.enabled;}
- (NSString*)previewText {return _nativeStartDetail.stringValue;}
- (BOOL)hasPreview {return _nativeStartPreview.image!=nil;}

@end
namespace {
std::map<std::string,double> pumpSeconds,pumpStall; // longest wait / longest main-runloop turn per labelled point
void pumpUntil(const char* label,double limit,BOOL(^done)(void)){
    NSDate* start=[NSDate date];NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:limit];
    double stall=0; // longest single main-runloop turn: main-thread blocking, not total work
    while(!done()&&deadline.timeIntervalSinceNow>0){NSDate* turn=[NSDate date];[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.005]];stall=std::max(stall,-turn.timeIntervalSinceNow);}
    const double elapsed=-start.timeIntervalSinceNow;auto& longest=pumpSeconds[label];longest=std::max(longest,elapsed);auto& turnMax=pumpStall[label];turnMax=std::max(turnMax,stall);
    if(!done())throw std::runtime_error(std::string("wait timed out: ")+label+" after "+std::to_string(elapsed)+"s");++checks;
    // Background work may scale with the supplied source count; the main thread may not block.
    if(stall>=.25)throw std::runtime_error(std::string("main runloop blocked during ")+label+" for "+std::to_string(stall)+"s");++checks;
}
void run(const game_lab::ResourceRoot& root,std::shared_ptr<const game_lab::NativeSessionSource> source){
    std::vector<std::pair<unsigned,unsigned>> cells;
    auto initial=game_lab::startNativeSession(source->seed,5);
    for(unsigned y=50;y<source->seed.side&&cells.size()<8;++y)for(unsigned x=75;x<source->seed.side&&cells.size()<8;++x)
        if(game_lab::nativeSessionPlacementPreview(source->seed,initial,game_lab::NativeSessionAction::Road,x,y).committed)cells.push_back({x,y});
    need(cells.size()==8,"real source has eight distinct buildable cells");
    NativeVisualHarness* owner=[[NativeVisualHarness alloc]initRoot:root source:source];__weak NativeVisualHarness* weakOwner=owner;const auto original=[owner balance];const auto cost=source->seed.roadCost;
    auto place=[&](size_t i){const auto[x,y]=cells[i];return [owner nativeAction:game_lab::NativeSessionAction::Road x:x y:y];};
    need(place(0)&&[owner pending]&&[owner balance]==original&&[owner objects]==0,"accepted GPU candidate does not charge before completion");
    auto first=[owner pending];need(!first->cpuPixels&&first->drawList&&first->preparedCPUSpriteDraws==0,"normal GPU preparation does not run CPU sprite raster");need(place(1)&&[owner queued]==1&&[owner balance]==original,"second actual construction queues without charging");
    [owner failGPUAndFinishCPU:first];need(first->cpuPixels&&first->cpuPixels->valid(),"only current fallback creates CPU raster pixels");need([owner balance]==original-cost&&[owner objects]==1,"current failure commits first candidate once via CPU");
    pumpUntil("rebased-queue",3,^BOOL{return [weakOwner pending]!=nullptr;});auto second=[owner pending];need(second!=first&&[owner queued]==0,"actual queue rebases next placement after commit");
    [owner failGPUAndFinishCPU:second];need([owner balance]==original-2*cost&&[owner objects]==2,"second queued placement charged exactly once");
    [owner failGPUAndFinishCPU:first];need([owner balance]==original-2*cost,"late GPU/CPU duplicates cannot charge again");
    need(place(2)&&place(3),"two more operations accepted");[owner flushNativeVisualForUI];
    need(![owner pending]&&[owner queued]==0&&[owner balance]==original-4*cost&&[owner objects]==4,"save/quit flush publishes all accepted intents before modal UI");
    need(place(4),"failure transaction accepted");auto bad=[owner pending];auto revision=[owner revision];const auto previousBytes=[owner serializedState];owner->failCPUImage=true;[owner failGPUAndFinishCPU:bad];owner->failCPUImage=false;
    need([owner balance]==original-4*cost&&[owner objects]==4&&[owner revision]==revision&&!bad->installed,"actual CPU image failure preserves public city fields");need([owner serializedState]==previousBytes,"actual presentation failure preserves the entire encoded native city");[owner verifyPixelsRetained];
    need(place(4),"retry accepted after allocation failure");[owner flushNativeVisualForUI];need([owner balance]==original-5*cost,"retry commits exactly once");
    need(place(5),"construction before camera change accepted");auto beforeCamera=[owner pending];game_lab::OriginalMapCamera camera=beforeCamera->camera;camera.centerX+=1;
    [owner nativeViewCamera:camera];need([owner balance]==original-6*cost&&[owner objects]==6,"camera change does not drop pending construction");
    auto view=[owner pending];need(view&&view!=beforeCamera&&view->camera.centerX==camera.centerX,"new camera owns distinct request");
    [owner failGPUAndFinishCPU:beforeCamera];need([owner pending]==view,"old callback cannot replace new view intent");[owner flushNativeVisualForUI];
    need([owner balance]==original-6*cost,"camera-only completion never charges");[owner requestFailedReplacement];
    need(place(6),"close case accepted");auto closing=[owner pending];[owner closeHarness];[owner failGPUAndFinishCPU:closing];need([owner balance]==original-6*cost&&!closing->installed,"closed actual owner never commits late candidate");
    NativeVisualHarness* feedback=[[NativeVisualHarness alloc]initRoot:root source:source];
    for(size_t i=0;i<4;++i){need([feedback nativeAction:game_lab::NativeSessionAction::Road x:cells[i].first y:cells[i].second],"feedback fixture construction");[feedback flushNativeVisualForUI];}
    [feedback verifyFeedbackReplacement];[feedback closeHarness];feedback=nil;
    // Actual stored completion closures are weak, including commands/camera.
    __weak NativeVisualHarness* weak=owner;owner=nil;need(weak==nil,"actual App completion closures do not retain owner");
    // New input arriving between completion and its queued drain must remain FIFO.
    NativeVisualHarness* fifo=[[NativeVisualHarness alloc]initRoot:root source:source];__weak NativeVisualHarness* weakFIFO=fifo;
    auto add=[&](size_t i){return [fifo nativeAction:game_lab::NativeSessionAction::Road x:cells[i].first y:cells[i].second];};
    need(add(0)&&add(1),"FIFO first and queued inputs accepted");[fifo failGPUAndFinishCPU:[fifo pending]];
    need(add(2)&&[fifo queued]==2&&![fifo pending],"fresh input cannot overtake queued predecessor");
    pumpUntil("fifo-first",3,^BOOL{return [weakFIFO pending]!=nullptr;});auto q=[fifo pending];
    need(q->candidate.objects.back().x==cells[1].first&&q->candidate.objects.back().y==cells[1].second,"oldest queued placement executes first");[fifo failGPUAndFinishCPU:q];
    pumpUntil("fifo-second",3,^BOOL{return [weakFIFO pending]!=nullptr;});q=[fifo pending];
    need(q->candidate.objects.back().x==cells[2].first&&q->candidate.objects.back().y==cells[2].second,"new input executes after predecessor");[fifo failGPUAndFinishCPU:q];
    need([fifo objects]==3&&[fifo balance]==original-3*cost,"FIFO cost and objects commit once each");[fifo closeHarness];fifo=nil;

    NSMutableArray* callbacks=[NSMutableArray new];auto done=std::make_shared<std::atomic<bool>>(false);
    NativeStartHarness* scan=[[NativeStartHarness alloc]initRoot:root deliveries:callbacks];__weak NativeStartHarness* weakScan=scan;auto generation=[scan work];
    [scan runCatalogUntil:done];pumpUntil("catalog-scan",120,^BOOL{return done->load();});
    need(callbacks.count>=3,"actual serial source worker reports progress and final delivery");
    need(!generation->cancelled,"completed worker leaves generation alive until dialog closes");
    [scan cancelNativeStartWork];need(generation->cancelled&&![scan work],"actual cancellation clears current generation");
    scan=nil;need(weakScan==nil,"queued actual catalog deliveries hold only a weak owner");
    for(dispatch_block_t callback in callbacks)callback();need(weakScan==nil,"late completions after owner destruction are safe");
    [callbacks removeAllObjects];scan=[[NativeStartHarness alloc]initRoot:root deliveries:callbacks];[scan setupPreviews];
    done=std::make_shared<std::atomic<bool>>(false);[scan selectPreview:0 until:done];pumpUntil("preview-first",3,^BOOL{return done->load();});
    need(callbacks.count==1&&![scan previewReady]&&![scan hasPreview],"decoded first preview waits for UI publication");
    dispatch_block_t late=[callbacks[0] copy];[callbacks removeAllObjects];
    done=std::make_shared<std::atomic<bool>>(false);[scan selectPreview:1 until:done];late();
    need(![scan previewReady]&&![scan hasPreview]&&[[scan previewText] containsString:@"202610230102.sav"],"late first preview cannot enable or paint second selection");
    pumpUntil("preview-second",3,^BOOL{return done->load();});need(callbacks.count==1,"second selection produces one delivery");((dispatch_block_t)callbacks[0])();
    need([scan previewReady]&&[scan hasPreview]&&[[scan previewText] containsString:@"202610230102.sav"],"current preview publishes matching image/source and enables creation");
    late();need([scan previewReady]&&[[scan previewText] containsString:@"202610230102.sav"],"old completion still cannot overwrite ready newer preview");
    [scan cancelNativeStartWork];scan=nil;[callbacks removeAllObjects];
    [callbacks removeAllObjects];done=std::make_shared<std::atomic<bool>>(false);
    scan=[[NativeStartHarness alloc]initRoot:root deliveries:callbacks];generation=[scan work];generation->cancelled=true;
    [scan runCatalogUntil:done];pumpUntil("catalog-cancelled",120,^BOOL{return done->load();});need(callbacks.count==0,"cancel before serial worker starts produces no callbacks");scan=nil;
    NativeVisualHarness* labor=[[NativeVisualHarness alloc]initRoot:root source:source];[labor verifyV6Labor];[labor closeHarness];labor=nil;

}
}
int main(){@autoreleasepool {try{
    const char* data=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!data){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
    auto root=game_lab::ResourceRoot::fromAbsolute(data);need(root.configured(),"verified process-local root");std::string error;
    auto source=game_lab::loadNativeSessionSource(root,"Save/2026102301/202610230101.sav",error);need(bool(source),"real empty source parsed");
    run(root,std::make_shared<const game_lab::NativeSessionSource>(std::move(*source)));
    std::string waits="\"waits_s\":{";for(const auto& [k,v]:pumpSeconds)waits+="\""+k+"\":["+std::to_string(v)+","+std::to_string(pumpStall[k])+"],";waits.back()='}';waits+=",";
    std::cout<<"{\"passed\":true,"<<waits<<v6Summary<<"\"actual_App_method_checks\":"<<checks<<",\"real_sources\":2,\"GPU_substitution\":true,\"GUI\":false,\"window_created\":false}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}}
