// SPDX-License-Identifier: MIT
// Included in EmperorAppDelegate. All methods/decisions here run on main.
- (void)scheduleNativeQueuedCommands {
    if(_nativeQueueDrainScheduled||_nativeFlushing||_nativeCommands.empty()||_nativePending)return;
    _nativeQueueDrainScheduled=YES;__weak EmperorAppDelegate* weakSelf=self;
    dispatch_async(dispatch_get_main_queue(),^{EmperorAppDelegate* me=weakSelf;if(!me)return;me->_nativeQueueDrainScheduled=NO;
        if(me->_nativeFrameGate.phase!=game_lab::SceneFramePhase::Open||me->_nativePending||me->_nativeCommands.empty())return;
        auto command=me->_nativeCommands.front();me->_nativeCommands.pop_front();
        // Placement intents retain their grid coordinates but validate against the
        // freshly committed city. Demolition retains its explicit selection token.
        if(me->_nativeState&&command.action!=game_lab::NativeSessionAction::Demolish)command.expectedRevision=me->_nativeState->revision;
        me->_nativeProcessingQueued=YES;[me nativeCommand:command];me->_nativeProcessingQueued=NO;[me scheduleNativeQueuedCommands];
    });
}
- (void)finishNativeVisual:(std::shared_ptr<NativeVisualTransaction>)request success:(BOOL)success metal:(BOOL)metal {
    if(request->settled)return;request->settled=true;
    const bool current=_nativePending==request;
    if(current)_nativePending.reset();
    _nativePublishing=YES;
    if(success&&current){request->installed=true;
        _nativeSource=request->source;_nativeState=request->candidate;_nativeCamera=request->camera;_nativeSurface=request->surface;
        if(metal){[_nativeCanvas showMetalLayer:_nativeMetalDisplay.layer logicalSize:NSMakeSize(request->flow.ticket.width,request->flow.ticket.height)];_nativePresentedFrame=request->frame;_nativeRenderRoute="Metal 测试";}
        else{_nativeCanvas.image=request->cpuImage;_nativePresentedFrame.reset();_nativeRenderRoute=_nativeMetalEnabled?"CPU 回退 / Metal 测试":"CPU";}
        const auto stats=request->frame?request->frame->diagnostics():game_lab::metal_scene::FrameDiagnostics{};
        std::fprintf(stderr,"native_timing request=%llu prepare_ms=%.3f install_ms=%.3f encode_ms=%.3f gpu_ms=%.3f backend_ms=%.3f hits=%zu uploads=%zu pinned_bytes=%zu pixels=%ux%u objects=%zu scale=%d cpu_sprite_draws=%zu fallback_raster_ms=%.3f\n",request->flow.ticket.requestSerial,request->prepareMs,(CACurrentMediaTime()-request->startedAt)*1000,stats.encodeMs,stats.gpuMs,stats.acceptToCompleteMs,stats.cacheHits,stats.uploads,stats.pinnedBytes,request->flow.ticket.width,request->flow.ticket.height,request->candidate.objects.size(),int(request->camera.scale),request->preparedCPUSpriteDraws,request->fallbackRasterMs);
        std::fprintf(stderr,"native_frame route=%s request=%llu epoch=%llu revision=%llu tick=%llu funds=%lld scheduled_only=%d\n",metal?"metal":"cpu",request->flow.ticket.requestSerial,request->flow.ticket.key.sessionEpoch,request->candidate.revision,request->candidate.ticks,request->candidate.funds,metal?1:0);
    }
    // No strong request->owner cycle, and obsolete requests cannot change UI.
    auto completion=request->completion;request->completion=nil;
    if(current&&completion)completion(success);
    if(current){[self updateNativeSessionLabels];[self scheduleNativeQueuedCommands];}
    if(current)std::fprintf(stderr,"native_publish_complete request=%llu success=%d elapsed_ms=%.3f\n",request->flow.ticket.requestSerial,success?1:0,(CACurrentMediaTime()-request->startedAt)*1000);
    _nativePublishing=NO;
}
- (BOOL)receiveNativeVisual:(std::shared_ptr<NativeVisualTransaction>)request signal:(game_lab::SceneFrameSignal)signal ticket:(const game_lab::SceneFrameTicket&)ticket width:(uint32_t)width height:(uint32_t)height {
    NSAssert(NSThread.isMainThread,@"native publication is main-thread owned");
    if(request->settled||_nativePending!=request)return NO;
    const auto transition=game_lab::reduceSceneFrameFlow(_nativeFrameGate,request->flow,signal,ticket,width,height);
    _nativeFrameGate=transition.gate;request->flow=transition.flow;
    if(transition.status==game_lab::SceneFrameFlowStatus::UnexpectedTicket){
        _nativeMessage="渲染返回身份不匹配，未提交。";_nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Cancel).state;[self finishNativeVisual:request success:NO metal:NO];return NO;
    }
    switch(transition.action){
        case game_lab::SceneFrameAction::StartDisplay:[self displayNativeVisual:request];break;
        case game_lab::SceneFrameAction::StartCPU:[self queueNativeCPU:request];break;
        case game_lab::SceneFrameAction::PublishGPU:[self finishNativeVisual:request success:YES metal:YES];return YES;
        case game_lab::SceneFrameAction::PublishCPU:[self finishNativeVisual:request success:YES metal:NO];return YES;
        case game_lab::SceneFrameAction::None:break;
    }
    if(request->flow.work==game_lab::SceneFrameWork::Finished&&!request->settled){_nativeMessage="候选画面取消或失败，城市未替换。";[self finishNativeVisual:request success:NO metal:NO];}
    return NO;
}
- (NSImage*)nativeCPUImage:(const game_lab::DecodedImage&)pixels {
    // A declared source space is stable before and after the view becomes layer-backed.
    // DeviceRGB is interpreted differently by AppKit's two backing paths.
    CGColorSpaceRef space=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);if(!space)return nil;
    CGImageRef image=game_lab::createCGImageFromPremultipliedBGRA(pixels.width,pixels.height,pixels.pixelsBGRA,space);CGColorSpaceRelease(space);
    if(!image)return nil;NSImage* result=[[NSImage alloc]initWithCGImage:image size:NSMakeSize(pixels.width,pixels.height)];CGImageRelease(image);return result;
}
- (void)completeNativeCPU:(std::shared_ptr<NativeVisualTransaction>)request {
    if(request->settled||_nativePending!=request||request->flow.work!=game_lab::SceneFrameWork::CPU)return;
    if(!game_lab::mayFallbackSceneFrame(_nativeFrameGate,request->flow.ticket)){
        [self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::CPUFailed ticket:request->flow.ticket width:0 height:0];return;}
    if(!request->cpuPixels){
        const double start=CACurrentMediaTime();
        if(request->drawList){auto pixels=game_lab::rasterizeSceneDrawListCPU(*request->drawList);if(pixels.valid())request->cpuPixels=std::make_shared<const game_lab::DecodedImage>(std::move(pixels));}
        request->fallbackRasterMs=(CACurrentMediaTime()-start)*1000;
        if(!request->cpuPixels){[self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::CPUFailed ticket:request->flow.ticket width:0 height:0];return;}
    }
    const auto& pixels=*request->cpuPixels;
    request->cpuImage=[self nativeCPUImage:pixels];
    [self receiveNativeVisual:request signal:request->cpuImage?game_lab::SceneFrameSignal::CPUReady:game_lab::SceneFrameSignal::CPUFailed ticket:request->flow.ticket width:pixels.width height:pixels.height];
}
- (void)queueNativeCPU:(std::shared_ptr<NativeVisualTransaction>)request {
    request->frame.reset();if(_nativeFlushing)return;__weak EmperorAppDelegate* weakSelf=self;
    dispatch_async(dispatch_get_main_queue(),^{EmperorAppDelegate* me=weakSelf;if(me)[me completeNativeCPU:request];});
}
- (void)displayNativeVisual:(std::shared_ptr<NativeVisualTransaction>)request {
    if(request->settled||_nativePending!=request)return;
    const auto texture=request->frame?request->frame->texture():nil;
    if(!texture){[self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::DisplayFailed ticket:request->flow.ticket width:0 height:0];return;}
    if(!_nativeMetalDisplay)_nativeMetalDisplay=[[NativeMetalDisplay alloc]initWithDevice:texture.device];
    [_nativeCanvas prepareMetalLayer:_nativeMetalDisplay.layer logicalSize:NSMakeSize(request->flow.ticket.width,request->flow.ticket.height)];
    if(request->settled||_nativePending!=request)return;
    const auto rect=[_nativeCanvas metalLocalFrame];
    if(![_nativeMetalDisplay configureFrame:rect width:request->flow.ticket.width height:request->flow.ticket.height]){
        [self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::DisplayDeclined ticket:request->flow.ticket width:0 height:0];return;}
    const uint32_t outputWidth=uint32_t(texture.width),outputHeight=uint32_t(texture.height);
    __weak EmperorAppDelegate* weakSelf=self;
    const auto submitted=[_nativeMetalDisplay submitTexture:texture ticket:request->flow.ticket
        admission:^BOOL(const game_lab::SceneFrameTicket& ticket){EmperorAppDelegate* me=weakSelf;
            return me?[me receiveNativeVisual:request signal:game_lab::SceneFrameSignal::DisplayAdmit ticket:ticket width:outputWidth height:outputHeight]:NO;
        } completion:^(const game_lab::SceneFrameTicket& ticket,NativeMetalDisplayResult result,NSString* reason){
            EmperorAppDelegate* me=weakSelf;if(!me)return;
            if(result==NativeMetalDisplayResult::Scheduled){std::fprintf(stderr,"native_display scheduled request=%llu (visibility not asserted)\n",ticket.requestSerial);return;}
            if(!request->settled)std::fprintf(stderr,"native_display failed request=%llu reason=%s\n",ticket.requestSerial,reason.UTF8String?:"unknown");
            if(!request->settled&&!reason.length)me->_nativeMessage="显示未完成，保留上一画面。";
            [me receiveNativeVisual:request signal:result==NativeMetalDisplayResult::Failed?game_lab::SceneFrameSignal::DisplayFailed:game_lab::SceneFrameSignal::DisplayCancelled ticket:ticket width:0 height:0];
        }];
    if(submitted!=NativeMetalDisplaySubmit::Accepted)[self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::DisplayDeclined ticket:request->flow.ticket width:0 height:0];
}
- (void)submitNativeGPU:(std::shared_ptr<NativeVisualTransaction>)request {
    if(!_nativeRenderer)_nativeRenderer=std::make_unique<game_lab::metal_scene::Renderer>();
    if(_nativeRenderer->available()&&_nativeMetalFault){
        _nativeRenderer->injectFaultForTesting(_nativeMetalFault==1?game_lab::metal_scene::TestFault::FailAtCompletion:game_lab::metal_scene::TestFault::DelayEncodeMs50);
        std::fprintf(stderr,"native_renderer explicit_test_fault=%u\n",_nativeMetalFault);if(_nativeMetalFault==1)_nativeMetalFault=0;
    }
    __weak EmperorAppDelegate* weakSelf=self;
    auto submitted=_nativeRenderer->submit(request->drawList,request->flow.ticket,[weakSelf,request](game_lab::metal_scene::Completion result){
        // Backend callback queue never touches App state. Completion/frame ownership
        // crosses to main; owner lifetime is checked only after that queue hop.
        dispatch_async(dispatch_get_main_queue(),^{EmperorAppDelegate* me=weakSelf;if(!me||request->settled||me->_nativePending!=request)return;
            if(result.status==game_lab::metal_scene::CompletionStatus::Ready&&result.frame){
                request->frame=result.frame;[me receiveNativeVisual:request signal:game_lab::SceneFrameSignal::GPUReady ticket:result.ticket width:result.frame->width() height:result.frame->height()];
            }else {std::fprintf(stderr,"native_renderer completion_failed request=%llu reason=%s\n",result.ticket.requestSerial,result.reason.c_str());[me receiveNativeVisual:request signal:result.status==game_lab::metal_scene::CompletionStatus::Cancelled?game_lab::SceneFrameSignal::GPUCancelled:game_lab::SceneFrameSignal::GPUFailed ticket:result.ticket width:0 height:0];}
        });
    });
    if(submitted.status!=game_lab::metal_scene::SubmitStatus::Accepted){
        std::fprintf(stderr,"native_renderer declined=%s reason=%s\n",game_lab::metal_scene::submitStatusName(submitted.status),submitted.reason.c_str());
        [self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::GPUDeclined ticket:request->flow.ticket width:0 height:0];
    }
}
- (NativeInstallResult)beginNativeCandidate:(const game_lab::NativeSessionState&)candidate source:(std::shared_ptr<const game_lab::NativeSessionSource>)source camera:(const game_lab::OriginalMapCamera&)camera completion:(void (^)(BOOL))completion {
    const double startedAt=CACurrentMediaTime();
    const auto reject=[&](){if(completion)completion(NO);return NativeInstallResult::Failed;};
    NSAssert(!_nativePending,@"caller must preserve or finish the preceding candidate");
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Cancel).state;
    if(!source)return reject();
    const auto projected=game_lab::nativeSessionScene(*source,candidate);
    const double projectedAt=CACurrentMediaTime();
    if(!projected){_nativeMessage="原生状态校验失败，未提交。";return reject();}
    if(!_nativeSpriteCache)_nativeSpriteCache=std::make_shared<game_lab::SG3SpriteCache>();
    auto surface=std::make_shared<game_lab::AppMapSurface>(_resourceRoot,_nativeSpriteCache);
    if(!surface->selectSavedBuildings(projected->terrain,projected->buildings,&projected->ground,0,projected->figures?&*projected->figures:nullptr)){_nativeMessage="绘制准备失败，未提交："+surface->error();return reject();}
    const double selectedAt=CACurrentMediaTime();
    const auto cacheBefore=_nativeSpriteCache->statistics();
    game_lab::SceneDrawCapture capture;
    const bool deferred=_nativeMetalEnabled&&!_nativeFlushing;
    auto view=surface->renderSavedBuildings(camera,-1,deferred?&capture:nullptr,deferred);
    if(candidate.rulesVersion==8){
        const size_t figures=candidate.figureRuntime->wellWalkerPayloads.size()+candidate.figureRuntime->migrantPayloads.size()+candidate.figureRuntime->immigrantPayloads.size()+(candidate.inspection?candidate.inspection->payloads.size():0);
        std::fprintf(stderr,"native_v8_figures expected=%zu live=%zu placed=%zu hidden=%zu zero=%zu unsupported=%zu decoded=%zu failures=%zu\n",figures,view.figureLive,view.figurePlaced,view.figureHidden,view.figureZeroImage,view.figureUnsupported,view.figureDecoded,view.overlayDecodeFailures);
        if(view.figureLive!=figures||view.figurePlaced+view.figureHidden!=figures||view.figureZeroImage||view.figureUnsupported){
            _nativeMessage="行人画面不完整，候选城市未提交。";return reject();
        }
    }
    const auto cacheAfter=_nativeSpriteCache->statistics();
    std::fprintf(stderr,"native_prepare project_ms=%.3f select_ms=%.3f draw_ms=%.3f cache_hits=%llu cache_requests=%llu path_resolutions=%llu path_hits=%llu resolve_ms=%.3f validate_ms=%.3f decode_ms=%.3f cache_bytes=%zu\n",(projectedAt-startedAt)*1000,(selectedAt-projectedAt)*1000,(CACurrentMediaTime()-selectedAt)*1000,cacheAfter.hits-cacheBefore.hits,cacheAfter.requests-cacheBefore.requests,cacheAfter.pathResolutions-cacheBefore.pathResolutions,cacheAfter.pathHits-cacheBefore.pathHits,(cacheAfter.resolveNs-cacheBefore.resolveNs)/1e6,(cacheAfter.validateNs-cacheBefore.validateNs)/1e6,(cacheAfter.decodeNs-cacheBefore.decodeNs)/1e6,cacheAfter.bytes);
    const auto invalidations=cacheAfter.invalidations;
    if(invalidations!=_nativeObservedCacheInvalidations){_nativeObservedCacheInvalidations=invalidations;_nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Resources).state;if(_nativeRenderer)_nativeRenderer->invalidateResources();}
    const size_t objects=std::count_if(candidate.objects.begin(),candidate.objects.end(),[](const auto&o){return o.kind!=game_lab::NativeObjectKind::Road;});
    if(!(deferred?game_lab::nativeSceneResourcesReady(view,objects)&&capture.complete():game_lab::nativeSceneReady(view,objects))){_nativeMessage="原资产图像绑定/解码失败，未扣费或提交。";return reject();}
    const auto issued=game_lab::requestSceneFrame(_nativeFrameGate,candidate.revision,surface->installedSceneDigest(),view.image.width,view.image.height);_nativeFrameGate=issued.state;
    if(!_nativeFrameGate.pending){_nativeMessage="画面请求拒绝，未提交。";return reject();}
    auto request=std::make_shared<NativeVisualTransaction>();request->startedAt=startedAt;request->prepareMs=(CACurrentMediaTime()-startedAt)*1000;request->source=std::move(source);request->candidate=candidate;request->camera=camera;request->surface=std::move(surface);
    request->preparedCPUSpriteDraws=view.bases.cpuSpriteDraws+view.overlayCpuDraws;
    if(!deferred)request->cpuPixels=std::make_shared<const game_lab::DecodedImage>(std::move(view.image));request->completion=[completion copy];request->flow.ticket=*_nativeFrameGate.pending;_nativePending=request;
    if(deferred){
        request->drawList=std::make_shared<const game_lab::SceneDrawList>(std::move(capture.list));[self submitNativeGPU:request];return NativeInstallResult::Pending;
    }
    request->flow.work=game_lab::SceneFrameWork::CPU;[self completeNativeCPU:request];
    return request->installed?NativeInstallResult::Installed:NativeInstallResult::Failed;
}
- (void)flushNativeVisualForUI {
    // Finish accepted user intents before a modal save/quit/new-source operation.
    // This uses the already validated CPU reference; never waits for GPU work.
    if(_nativeFlushing)return;_nativeFlushing=YES;
    while(_nativePending||!_nativeCommands.empty()){
        if(_nativePending){auto request=_nativePending;
            if(request->flow.work==game_lab::SceneFrameWork::GPU)[self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::GPUFailed ticket:request->flow.ticket width:0 height:0];
            else if(request->flow.work==game_lab::SceneFrameWork::Display)[self receiveNativeVisual:request signal:game_lab::SceneFrameSignal::DisplayFailed ticket:request->flow.ticket width:0 height:0];
            [self completeNativeCPU:request];
        }else{auto command=_nativeCommands.front();_nativeCommands.pop_front();if(_nativeState&&command.action!=game_lab::NativeSessionAction::Demolish)command.expectedRevision=_nativeState->revision;[self nativeCommand:command];}
    }
    _nativeFlushing=NO;
}
- (void)nativeCanvasGeometryChanged {
    if(_nativePublishing){
        if(!_nativeGeometryDeferred){_nativeGeometryDeferred=YES;__weak EmperorAppDelegate* weakSelf=self;
            dispatch_async(dispatch_get_main_queue(),^{EmperorAppDelegate* me=weakSelf;if(!me)return;me->_nativeGeometryDeferred=NO;if(me->_nativeFrameGate.phase==game_lab::SceneFramePhase::Open)[me nativeCanvasGeometryChanged];});}
        return;
    }
    // A resize never drops an accepted construction: finish it via CPU first,
    // then invalidate only the view. Logical viewport pixels follow Cocoa points,
    // not backing scale, so resizing reveals more city without stretching art.
    [self flushNativeVisualForUI];_nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Camera).state;
    if(_nativeState&&_nativeCanvas.bounds.size.width>=1&&_nativeCanvas.bounds.size.height>=1){
        auto camera=_nativeCamera;camera.width=unsigned(std::clamp(std::floor(_nativeCanvas.bounds.size.width),1.,2048.));camera.height=unsigned(std::clamp(std::floor(_nativeCanvas.bounds.size.height),1.,2048.));
        if(_nativePresentedFrame||camera.width!=_nativeCamera.width||camera.height!=_nativeCamera.height)[self nativeViewCamera:camera];
    }
}
