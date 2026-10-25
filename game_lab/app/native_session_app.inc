- (void)updateNativeMinimap {
    if(!_nativeMinimap||!_nativeSource||!_nativeState)return;
    if(_nativeOverviewSource!=_nativeSource){
        std::shared_ptr<const game_lab::NativeTerrainOverview> overview;
        try{overview=game_lab::makeNativeTerrainOverview(_nativeSource->save.terrain);}catch(const std::bad_alloc&){}
        NSImage* image=nil;if(overview){game_lab::DecodedImage p;p.width=p.height=overview->pixels;p.pixelsBGRA=overview->bgra;image=[self nativeCPUImage:p];}
        if(!image){[_nativeMinimap installOverview:{} image:nil];_nativeMinimap.accessibilityLabel=@"地形概览暂不可用";_nativeOverviewSource=_nativeSource;return;}
        [_nativeMinimap installOverview:overview image:image];_nativeOverviewSource=_nativeSource;
        std::fprintf(stderr,"native_minimap base_generated side=%u active=%zu pixels=%u\n",overview->side,overview->activeCells,overview->pixels);
    }
    [_nativeMinimap updateCamera:_nativeCamera objects:_nativeState->objects];
}
// SPDX-License-Identifier: MIT
// Versioned native UI implementation included inside EmperorAppDelegate. This uses
// original scene rendering and the separate native session transaction API.
- (NSImage*)nativeUI:(unsigned)physicalID {
    if(!_nativeUIImages){_nativeUIImages=[NSMutableDictionary dictionary];const auto archive=_media->sg3("DATA/China_Interface.sg3");
        for(unsigned id:{1221u,1222u,1223u,1224u,1319u,1323u,1327u,1331u,1335u,1339u,1343u,1347u,1351u,1355u,1359u,1491u,1551u,1512u}){if(id>archive.sprites.size())continue;
            const auto result=_media->sprite(archive,id-1,"DATA/China_Interface.555");
            if(result.image.valid()){NSImage* im=imageFromPixels(result.image.width,result.image.height,result.image.pixelsBGRA);if(im)_nativeUIImages[@(id)]=im;}}
    }return _nativeUIImages[@(physicalID)];
}
- (const game_lab::NativeCityFeedback&)nativeFeedback {
    const auto epoch=_nativeFrameGate.sessionEpoch,revision=_nativeState->revision;
    if(!_nativeFeedback||_nativeFeedbackSource.lock()!=_nativeSource||_nativeFeedbackEpoch!=epoch||_nativeFeedbackRevision!=revision){
        _nativeFeedback=game_lab::queryNativeCityFeedback(_nativeSource->seed,*_nativeState);_nativeFeedbackEpoch=epoch;_nativeFeedbackRevision=revision;_nativeFeedbackSource=_nativeSource;
    }
    return *_nativeFeedback;
}
- (const game_lab::NativeHouseEvolutionReport&)nativeHouseEvolution {
    static std::weak_ptr<const game_lab::NativeSessionSource> modelSource;
    static game_lab::NativeHouseModels models;
    static bool modelReady=false;
    static std::string modelMissing;
    static std::weak_ptr<const game_lab::NativeSessionSource> reportSource;
    static uint64_t reportEpoch=0,reportRevision=UINT64_MAX;
    static std::optional<game_lab::NativeHouseEvolutionReport> report;
    static game_lab::NativeHouseEvolutionReport empty;
    static uint64_t loggedRevision=UINT64_MAX;
    if(!_nativeSource||!_nativeState)return empty;
    const double queryStarted=CACurrentMediaTime();
    if(modelSource.lock()!=_nativeSource){
        modelReady=false;modelMissing.clear();
        const double started=CACurrentMediaTime();
        if(auto loaded=game_lab::loadNativeHouseModels(_resourceRoot,_nativeSource->seed,modelMissing)){
            models=std::move(*loaded);modelReady=true;
        }
        std::fprintf(stderr,"native_house_models elapsed_ms=%.3f ready=%d missing=%s\n",(CACurrentMediaTime()-started)*1000,modelReady?1:0,modelMissing.c_str());
        modelSource=_nativeSource;reportSource.reset();reportRevision=UINT64_MAX;loggedRevision=UINT64_MAX;
    }
    if(reportSource.lock()!=_nativeSource||reportEpoch!=_nativeFrameGate.sessionEpoch||reportRevision!=_nativeState->revision){
        const double started=CACurrentMediaTime();
        if(modelReady)report=game_lab::queryNativeHouseEvolution(*_nativeSource,*_nativeState,models);
        else {report=game_lab::NativeHouseEvolutionReport{};report->error=modelMissing.empty()?"models_unparsed":modelMissing;}
        std::fprintf(stderr,"native_house_query first_ms=%.3f query_ms=%.3f houses=%zu error=%s\n",(CACurrentMediaTime()-queryStarted)*1000,(CACurrentMediaTime()-started)*1000,report->houses.size(),report->error.c_str());
        reportSource=_nativeSource;reportEpoch=_nativeFrameGate.sessionEpoch;reportRevision=_nativeState->revision;
    }else if(loggedRevision!=reportRevision){
        loggedRevision=reportRevision;const auto& cached=report?*report:empty;
        std::fprintf(stderr,"native_house_query cached_ms=%.3f houses=%zu revision=%llu\n",(CACurrentMediaTime()-queryStarted)*1000,cached.houses.size(),(unsigned long long)reportRevision);
    }
    return report?*report:empty;
}
- (NSString*)nativeHouseConditionsText {
    if(!_nativeState||!_nativeSource||!_nativeInspectionId)return nil;
    const auto* object=[self nativeFeedback].find(_nativeInspectionId);
    if(!object||object->kind!=game_lab::NativeObjectKind::House)return nil;
    const auto& report=[self nativeHouseEvolution];
    if(const auto* view=report.find(_nativeInspectionId))
        return [NSString stringWithUTF8String:game_lab::nativeHouseEvolutionText(*view).detail.c_str()];
    return [NSString stringWithFormat:@"住宅条件未算\n%@",[NSString stringWithUTF8String:game_lab::nativeHouseEvolutionErrorText(report.error).c_str()]];
}
- (void)popoverDidShow:(NSNotification*)note {
    if(note.object!=_nativeHousePopover)return;
    _nativeHouseConditions.accessibilityValue=@"已展开";
    std::fprintf(stderr,"native_house_popover shown=1 id=%llu\n",(unsigned long long)_nativeHousePopoverHouseId);
}
- (void)popoverDidClose:(NSNotification*)note {
    if(note.object!=_nativeHousePopover)return;
    _nativeHouseConditions.accessibilityValue=@"已收起";
    std::fprintf(stderr,"native_house_popover shown=0 id=%llu\n",(unsigned long long)_nativeHousePopoverHouseId);
}
- (void)onNativeHouseConditions:(id)sender {
    (void)sender;NSString* text=[self nativeHouseConditionsText];if(!text.length)return;
    if(!_nativeHousePopover){
        NSPopover* popover=[NSPopover new];popover.behavior=NSPopoverBehaviorTransient;popover.contentSize=NSMakeSize(340,300);
        NSView* content=[[NSView alloc]initWithFrame:NSMakeRect(0,0,340,300)];
        NSScrollView* scroll=[[NSScrollView alloc]initWithFrame:content.bounds];
        scroll.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;scroll.hasVerticalScroller=YES;scroll.borderType=NSNoBorder;
        scroll.accessibilityIdentifier=@"native-house-conditions-scroll";
        NSTextView* textView=[[NSTextView alloc]initWithFrame:NSMakeRect(0,0,320,300)];
        textView.editable=NO;textView.selectable=YES;textView.drawsBackground=NO;textView.font=[NSFont systemFontOfSize:12];
        textView.textContainerInset=NSMakeSize(8,8);textView.verticallyResizable=YES;textView.horizontallyResizable=NO;
        textView.autoresizingMask=NSViewWidthSizable;textView.textContainer.widthTracksTextView=YES;
        textView.accessibilityIdentifier=@"native-house-conditions-text";textView.accessibilityLabel=@"住宅完整条件";
        scroll.documentView=textView;[content addSubview:scroll];
        NSViewController* controller=[NSViewController new];controller.view=content;popover.contentViewController=controller;
        popover.delegate=self;_nativeHousePopover=popover;_nativeHousePopoverText=textView;
    }
    _nativeHousePopoverText.string=text;
    if(_nativeHousePopover.shown){[_nativeHousePopover close];return;}
    _nativeHousePopoverHouseId=_nativeInspectionId;
    [_nativeHousePopover showRelativeToRect:_nativeDetails.bounds ofView:_nativeDetails preferredEdge:NSRectEdgeMaxY];
}
- (void)updateNativeInspection {
    if(!_nativeState||!_nativeSource)return;
    const auto& seed=_nativeSource->seed;const NSInteger tool=_nativeTool.indexOfSelectedItem;
    const auto& feedback=[self nativeFeedback];
    NSString* summary=[NSString stringWithUTF8String:game_lab::nativeCategorySummary(seed,*_nativeState,_nativeCategoryIndex,feedback).c_str()];
    _nativeServiceSummary.stringValue=summary;_nativeServiceSummary.accessibilityLabel=summary;
    NSString* description=nil;NSString* tooltip=nil;
    if(tool>0){NSArray* selection=@[@"浏览",[NSString stringWithFormat:@"道路 · %u 金 / 格\n拖动预览整条路径，松开一次提交。",seed.roadCost],[NSString stringWithFormat:@"普通住宅 · %u 金 / 2×2\n容量 %u 人；入住按当前版本的道路、供水及食物条件。",seed.houseCost,seed.houseCapacity],[NSString stringWithFormat:@"水井 · %u 金 / 2×2\n邻路启用；原生供水范围外扩4格。",seed.wellCost],[NSString stringWithFormat:@"猎人营地 · %u 金 / 2×2\n邻路且生产开启，每2更新最多产4肉。",seed.hunterCost],@"拆除 · 无退款\n先选自己的对象，再确认；可取消。"];
        description=tool<NSInteger(selection.count)?[selection[tool] stringByAppendingString:@"\n\n选择只启用工具，不扣费。地图预览后单击施工；右键取消。"]:[NSString stringWithUTF8String:game_lab::nativeCategoryHint(_nativeCategoryIndex).c_str()];
    }else if(_nativeInspectionId){description=[NSString stringWithUTF8String:game_lab::nativeObjectDescription(seed,*_nativeState,_nativeInspectionId,feedback).c_str()];
        const auto* inspected=feedback.find(_nativeInspectionId);
        if(inspected&&inspected->kind==game_lab::NativeObjectKind::House){
            const auto& report=[self nativeHouseEvolution];
            if(const auto* view=report.find(_nativeInspectionId)){
                const auto text=game_lab::nativeHouseEvolutionText(*view);
                if(!text.summary.empty())description=[description stringByAppendingFormat:@"\n%@",[NSString stringWithUTF8String:text.summary.c_str()]];
                tooltip=[description stringByAppendingFormat:@"\n\n%@",[NSString stringWithUTF8String:text.detail.c_str()]];
            }else{
                NSString* reason=[NSString stringWithUTF8String:game_lab::nativeHouseEvolutionErrorText(report.error).c_str()];
                description=[description stringByAppendingFormat:@"\n\n住宅条件未算\n%@",reason];tooltip=description;
            }
        }
    }else{description=[NSString stringWithFormat:@"%@\n当前为浏览；分类切换不会施工。",[NSString stringWithUTF8String:game_lab::nativeCategoryHint(_nativeCategoryIndex).c_str()]];}
    if(!tooltip)tooltip=description;
    _nativeDetails.stringValue=description;_nativeDetails.toolTip=tooltip;
    unsigned icon=0;const auto*object=[self nativeFeedback].find(_nativeInspectionId);
    if(tool==0&&object)icon=object->kind==game_lab::NativeObjectKind::House?1491:object->kind==game_lab::NativeObjectKind::Well?1551:object->kind==game_lab::NativeObjectKind::Hunter?1512:0;
    _nativeDetailIcon.image=icon?[self nativeUI:icon]:nil;_nativeDetailIcon.hidden=!_nativeDetailIcon.image;
    if(_nativeDetailIcon.image){NSMutableAttributedString* styled=[[NSMutableAttributedString alloc]initWithString:description attributes:@{NSFontAttributeName:_nativeDetails.font,NSForegroundColorAttributeName:_nativeDetails.textColor}];
        NSMutableParagraphStyle* paragraph=[NSMutableParagraphStyle new];paragraph.firstLineHeadIndent=20;
        const auto newline=[description rangeOfString:@"\n"].location;[styled addAttribute:NSParagraphStyleAttributeName value:paragraph range:NSMakeRange(0,newline==NSNotFound?description.length:newline)];_nativeDetails.attributedStringValue=styled;}
    const auto*selected=tool==0&&_nativeInspectionId?[self nativeFeedback].find(_nativeInspectionId):nullptr;
    if(_nativeDetailDemolish){
        const BOOL armed=_nativeDemolition.has_value();
        _nativeDetailDemolish.enabled=selected&&_nativeState->rulesVersion>=4&&_nativeState->paused;
        _nativeDetailDemolish.state=armed?NSControlStateValueOn:NSControlStateValueOff;
        _nativeDetailDemolish.accessibilityLabel=armed?@"拆除所选（已选目标，等待确认）":@"拆除所选…";
        _nativeDetailDemolish.toolTip=!selected?@"先在浏览模式点击自建对象，再拆除。":_nativeState->rulesVersion<4?@"当前规则版本不支持拆除；不会自动升级。":!_nativeState->paused?@"暂停后才能拆除；此按钮只打开预览。":@"打开拆除预览；仍需“确认拆除”才执行，无退款。";
        _nativeDetailDemolish.accessibilityHelp=_nativeDetailDemolish.toolTip;_nativeDetailDemolish.needsDisplay=YES;
    }
    const bool houseSelected=selected&&selected->kind==game_lab::NativeObjectKind::House;
    _nativeDetails.accessibilityHelp=houseSelected?@"点击展开完整住宅条件":nil;
    if(_nativeHouseConditions){
        _nativeHouseConditions.enabled=houseSelected;
        _nativeHouseConditions.toolTip=houseSelected?@"查看完整原版条件、阈值与未接通原因（只读，不改变城市）。":@"先选择住宅；此按钮只读。";
        _nativeHouseConditions.accessibilityHelp=_nativeHouseConditions.toolTip;
    }
    if(_nativeHousePopover.shown){
        NSString* text=houseSelected?[self nativeHouseConditionsText]:nil;
        if(text.length)_nativeHousePopoverText.string=text;else [_nativeHousePopover close];
    }
}
- (void)updateNativeToolPalette {
    if(!_nativeState||!_nativeSource)return;const auto version=_nativeState->rulesVersion;
    const auto layout=game_lab::nativeGameLayout(_nativePanel.contentView.bounds.size.width,_nativePanel.contentView.bounds.size.height);
    NSArray* names=@[@"查看建筑 / 拖动地图",[NSString stringWithFormat:@"道路 · %u 金 / 格",_nativeSource->seed.roadCost],[NSString stringWithFormat:@"普通住宅 · %u 金 / 2×2",_nativeSource->seed.houseCost],[NSString stringWithFormat:@"水井 · %u 金 / 2×2",_nativeSource->seed.wellCost],[NSString stringWithFormat:@"猎人营地 · %u 金 / 2×2",_nativeSource->seed.hunterCost],@"拆除 · 先预览后确认"];
    NSArray* categoryNames=@[@"住宅",@"食物",@"公共服务：水井",@"道路与拆除"];
    for(NSButton* b in _nativeCategories){const auto tag=unsigned(b.tag);NSString* reason=[NSString stringWithUTF8String:game_lab::nativeCategoryUnavailableReason(tag,version).c_str()];
        b.enabled=game_lab::nativeCategoryAvailable(tag,version);b.state=tag==_nativeCategoryIndex?NSControlStateValueOn:NSControlStateValueOff;
        b.toolTip=reason.length?[NSString stringWithFormat:@"%@\n%@",categoryNames[tag],reason]:[NSString stringWithFormat:@"%@\n点击查看分类摘要。",categoryNames[tag]];
        b.accessibilityHelp=reason.length?reason:@"点击查看分类摘要。";b.needsDisplay=YES;}
    for(NSButton* b in _nativeChoices){const auto tag=unsigned(b.tag);const bool visible=b.tag==0||game_lab::nativeToolCategory(tag)==_nativeCategoryIndex;
        NSString* reason=[NSString stringWithUTF8String:game_lab::nativeToolUnavailableReason(tag,version).c_str()];
        b.accessibilityLabel=names[b.tag];b.toolTip=reason.length?[NSString stringWithFormat:@"%@\n%@",names[b.tag],reason]:names[b.tag];b.accessibilityHelp=reason;
        b.hidden=!visible;b.enabled=game_lab::nativeToolAvailable(tag,version);b.state=b.tag==_nativeTool.indexOfSelectedItem?NSControlStateValueOn:NSControlStateValueOff;
        b.frame=NativeNSRect(layout.toolFor(tag));b.needsDisplay=YES;}
}
- (void)onNativeCategory:(NSButton*)sender {
    if(!_nativeState||!game_lab::nativeCategoryAvailable(unsigned(sender.tag),_nativeState->rulesVersion))return;
    _nativeCategoryIndex=unsigned(sender.tag);_nativeInspectionId=0;
    [_nativeTool selectItemAtIndex:0];[self onNativeTool:nil];[self updateNativeInspection];
}
- (void)onNativePaletteTool:(NSButton*)sender {if(!_nativeState||sender.tag<0||sender.tag>5||!game_lab::nativeToolAvailable(unsigned(sender.tag),_nativeState->rulesVersion)||(sender.tag!=0&&game_lab::nativeToolCategory(unsigned(sender.tag))!=_nativeCategoryIndex))return;if(sender.tag!=0)_nativeInspectionId=0;[_nativeTool selectItemAtIndex:sender.tag];[self onNativeTool:nil];[self updateNativeInspection];}
- (void)onNativeRules:(id)sender {(void)sender;[self nativeError:_nativeRulesLabel.stringValue];}
- (void)onNativeDiagnostics:(id)sender {(void)sender;if(!_nativeState)return;
    [self nativeError:[NSString stringWithFormat:@"%@\n\n%@\n%@\n%@",_nativeStatus.stringValue,_nativeWaterInfo.stringValue,_nativeFoodInfo.stringValue,_nativeTaxInfo.stringValue]];
}
- (void)syncNativeSpeedChoices {
    for(NSButton* b in _nativeSpeedChoices){const BOOL on=b.tag==NSInteger(_nativeSpeedIndex);b.state=on?NSControlStateValueOn:NSControlStateValueOff;b.accessibilityLabel=[NSString stringWithFormat:@"%@ 游戏速度%@",b.title,on?@"（当前）":@""];b.needsDisplay=YES;}
}
- (void)onNativeSpeed:(NSButton*)sender {
    if(sender.tag<0||sender.tag>2)return;_nativeSpeedIndex=unsigned(sender.tag);[self syncNativeSpeedChoices];
    if(_nativeState&&!_nativeState->paused){[self stopNativeSessionTimer];const unsigned speeds[]={1,2,4};_nativeTimer=[NSTimer scheduledTimerWithTimeInterval:game_lab::nativePresentationInterval(speeds[_nativeSpeedIndex]) target:self selector:@selector(onNativeTick:) userInfo:nil repeats:YES];}
}
- (void)stopNativeSessionTimer {[_nativeTimer invalidate];_nativeTimer=nil;}
- (void)updateNativeWaterOverlay {
    _nativeCanvas.waterPath=nil;_nativeCanvas.wetHousePath=nil;_nativeCanvas.dryHousePath=nil;
    if(!_nativeState||!_nativeSource)return;const auto& s=*_nativeState;const auto& seed=_nativeSource->seed;
    _nativeWaterToggle.enabled=s.rulesVersion>=2;
    if(s.rulesVersion==1){_nativeWaterInfo.stringValue=@"v1旧规则：供水不参与入住，井不可建；加载不会自动升级规则。";_nativeCanvas.needsDisplay=YES;return;}
    const auto roads=game_lab::nativeRoads(seed,s),water=game_lab::nativeWaterCoverage(seed,s);size_t wells=0,active=0,wet=0,dry=0;NSString* inspected=@"青色=有水住宅，橙色=无水住宅";
    NSBezierPath* coverage=[NSBezierPath bezierPath];NSBezierPath* wetPath=[NSBezierPath bezierPath];NSBezierPath* dryPath=[NSBezierPath bezierPath];
    const auto add=[&](NSBezierPath* path,unsigned x,unsigned y){const auto p=game_lab::nativeSessionProject(_nativeCamera,x,y);const double w=40*_nativeCamera.scale,h=20*_nativeCamera.scale;
        if(p.x+w<0||p.x-w>_nativeCamera.width||p.y+h<0||p.y-h>_nativeCamera.height)return;
        [path moveToPoint:NSMakePoint(p.x,p.y-h)];[path lineToPoint:NSMakePoint(p.x+w,p.y)];[path lineToPoint:NSMakePoint(p.x,p.y+h)];[path lineToPoint:NSMakePoint(p.x-w,p.y)];[path closePath];};
    const bool show=_nativeWaterToggle.state==NSControlStateValueOn;
    if(show)for(unsigned y=0;y<seed.side;++y)for(unsigned x=0;x<seed.side;++x)if(water[size_t(y)*seed.side+x])add(coverage,x,y);
    for(const auto& object:s.objects){
        const bool connected=game_lab::nativeHouseConnected(seed,object,roads);
        if(object.kind==game_lab::NativeObjectKind::Well){++wells;active+=connected;}
        if(object.kind==game_lab::NativeObjectKind::House){const bool supplied=game_lab::nativeHouseWatered(seed,object,water);wet+=supplied;dry+=!supplied;
            if(show)for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx)add(supplied?wetPath:dryPath,object.x+dx,object.y+dy);}
        if(object.kind!=game_lab::NativeObjectKind::Road&&_nativeHoverTile&&_nativeHoverTile->first>=int(object.x)&&_nativeHoverTile->first<int(object.x)+2&&_nativeHoverTile->second>=int(object.y)&&_nativeHoverTile->second<int(object.y)+2){
            if(object.kind==game_lab::NativeObjectKind::Well)inspected=[NSString stringWithFormat:@"井#%llu：%@",object.id,connected?@"邻路，已供水":@"未邻路，未供水"];
            else if(object.kind==game_lab::NativeObjectKind::Hunter)inspected=[NSString stringWithFormat:@"营地#%llu：%@ / %@",object.id,connected?@"邻路":@"未邻路",s.huntingEnabled?@"生产已开":@"生产已停"];
            else inspected=[NSString stringWithFormat:@"住宅#%llu：%@ / %@，入住%u/%u",object.id,game_lab::nativeHouseWatered(seed,object,water)?@"有水":@"无水",connected?@"邻路":@"未邻路",object.residents,seed.houseCapacity];}
    }
    _nativeWaterInfo.stringValue=[NSString stringWithFormat:@"井启用 %zu/%zu · 住宅有水 %zu / 无水 %zu · %@",active,wells,wet,dry,inspected];
    if(show){_nativeCanvas.waterPath=coverage;_nativeCanvas.wetHousePath=wetPath;_nativeCanvas.dryHousePath=dryPath;}_nativeCanvas.needsDisplay=YES;
}
- (void)onNativeWaterOverlay:(id)sender {(void)sender;[self updateNativeWaterOverlay];}
- (void)updateNativeDemolitionPreview {
    _nativeDemolitionConfirm.enabled=NO;
    if(!_nativeSource||!_nativeState)return;
    const auto& state=*_nativeState;
    _nativeDemolitionInfo.stringValue=state.rulesVersion>=4?@"拆除无退款；库存保留；住民移出。只拆自建对象；编号不复用，总建造上限3999。":@"旧版本不支持拆除；会话按原规则运行，不会自动升级。";
    if(_nativeTool.indexOfSelectedItem!=5)return;
    if(!state.paused){_nativeHover.stringValue=@"请先暂停，再点选自己的道路或建筑预览拆除；当前没有删除。";return;}
    if(_nativeDemolition&&_nativeDemolition->expectedRevision!=state.revision){_nativeDemolition.reset();_nativeHover.stringValue=@"状态已变化，旧选择取消；请重新点选。";return;}
    const auto id=_nativeDemolition?std::optional<uint64_t>{_nativeDemolition->objectId}:_nativeHoverTile?game_lab::nativeObjectAt(state,_nativeHoverTile->first,_nativeHoverTile->second):std::nullopt;
    if(!id){_nativeHover.stringValue=@"拆除：此处没有本会话新增对象；原图道路、建筑和地形不可拆。";return;}
    const auto plan=game_lab::planNativeDemolition(_nativeSource->seed,state,*id);
    if(!plan.candidate){_nativeHover.stringValue=[NSString stringWithUTF8String:plan.error.c_str()];return;}
    const auto& o=plan.object;NSBezierPath* path=[NSBezierPath bezierPath];
    for(unsigned dy=0;dy<game_lab::nativeObjectSide(o.kind);++dy)for(unsigned dx=0;dx<game_lab::nativeObjectSide(o.kind);++dx){
        const auto p=game_lab::nativeSessionProject(_nativeCamera,o.x+dx,o.y+dy);const double w=40*_nativeCamera.scale,h=20*_nativeCamera.scale;
        [path moveToPoint:NSMakePoint(p.x,p.y-h)];[path lineToPoint:NSMakePoint(p.x+w,p.y)];[path lineToPoint:NSMakePoint(p.x,p.y+h)];[path lineToPoint:NSMakePoint(p.x-w,p.y)];[path closePath];}
    _nativeCanvas.previewPath=path;_nativeCanvas.previewAllowed=NO;_nativeCanvas.needsDisplay=YES;
    NSString* name=o.kind==game_lab::NativeObjectKind::Road?@"道路":o.kind==game_lab::NativeObjectKind::House?@"住宅":o.kind==game_lab::NativeObjectKind::Well?@"水井":@"猎人营地";
    _nativeHover.stringValue=[NSString stringWithFormat:@"拆除 %@ #%llu · (%u,%u) %u×%u · 无退款 · 移出%u人 · %@",name,o.id,o.x,o.y,game_lab::nativeObjectSide(o.kind),game_lab::nativeObjectSide(o.kind),o.residents,_nativeDemolition?@"已选中；点下方确认才执行":@"单击仅选中，不删除"];
    const auto& b=plan.before;const auto& a=plan.after;
    _nativeDemolitionInfo.stringValue=[NSString stringWithFormat:@"启用井 %u→%u · 有水住宅 %u→%u · 邻路住宅 %u→%u · 生产营地 %u→%u；肉%u保留。",b.wells,a.wells,b.wateredHouses,a.wateredHouses,b.connectedHouses,a.connectedHouses,b.hunters,a.hunters,state.foodStock];
    _nativeDemolitionConfirm.enabled=_nativeDemolition.has_value();
    _nativeDetails.stringValue=[_nativeHover.stringValue stringByAppendingFormat:@"\n%@",_nativeDemolitionInfo.stringValue];
}
- (void)onNativeDemolitionConfirm:(id)sender {
    (void)sender;if(!_nativeDemolition)return;const auto command=*_nativeDemolition;_nativeDemolition.reset();_nativeInspectionId=0;[self nativeCommand:command];
}
- (void)onNativeDetailDemolish:(id)sender {
    (void)sender;if(!_nativeState||!_nativeSource||!_nativeInspectionId)return;
    if(_nativeState->rulesVersion<4){_nativeMessage="当前规则版本不支持拆除；不会自动升级。";[self updateNativeSessionLabels];return;}
    if(!_nativeState->paused){_nativeMessage="拆除前请先暂停；所选对象保留，尚未进入拆除。";[self updateNativeSessionLabels];return;}
    const auto plan=game_lab::planNativeDemolition(_nativeSource->seed,*_nativeState,_nativeInspectionId);
    if(!plan.candidate){_nativeMessage="该对象当前不可拆："+plan.error;[self updateNativeSessionLabels];return;}
    if(_nativeTool.indexOfSelectedItem!=5)[_nativeTool selectItemAtIndex:5];
    _nativeCategoryIndex=game_lab::nativeToolCategory(5);
    game_lab::NativeSessionCommand command{game_lab::NativeSessionAction::Demolish,_nativeState->revision};command.objectId=plan.object.id;_nativeDemolition=command;
    _nativeMessage="已就地选中拆除目标；点“确认拆除”才执行，取消不改动城市。";[self updateNativeSessionLabels];
}
- (void)updateNativeSelectionHighlight {
    _nativeCanvas.selectionPath=nil;
    if(!_nativeState||!_nativeSource||!_nativeInspectionId||_nativeTool.indexOfSelectedItem!=0)return;
    const auto found=std::find_if(_nativeState->objects.begin(),_nativeState->objects.end(),[&](const auto& o){return o.id==_nativeInspectionId;});
    if(found==_nativeState->objects.end())return;
    NSBezierPath* path=[NSBezierPath bezierPath];
    for(unsigned dy=0;dy<game_lab::nativeObjectSide(found->kind);++dy)for(unsigned dx=0;dx<game_lab::nativeObjectSide(found->kind);++dx){
        const auto p=game_lab::nativeSessionProject(_nativeCamera,found->x+dx,found->y+dy);const double w=40*_nativeCamera.scale,h=20*_nativeCamera.scale;
        [path moveToPoint:NSMakePoint(p.x,p.y-h)];[path lineToPoint:NSMakePoint(p.x+w,p.y)];[path lineToPoint:NSMakePoint(p.x,p.y+h)];[path lineToPoint:NSMakePoint(p.x-w,p.y)];[path closePath];}
    _nativeCanvas.selectionPath=path;
}
- (void)updateNativePreview {
    _nativeCanvas.previewPath=nil;_nativeCanvas.existingPath=nil;[self updateNativeWaterOverlay];[self updateNativeSelectionHighlight];
    if(!_nativeState||!_nativeSource)return;
    [self updateNativeDemolitionPreview];
    _nativeHover.hidden=(!_nativeHoverTile&&!_nativeDemolition)||_nativeTool.indexOfSelectedItem==0;
    _nativeCanvas.browseMode=_nativeTool.indexOfSelectedItem==0;_nativeCanvas.roadMode=_nativeTool.indexOfSelectedItem==1;
    if(_nativeTool.indexOfSelectedItem==5)return;
    if(!_nativeHoverTile||_nativeCanvas.browseMode){_nativeHover.stringValue=_nativeCanvas.browseMode?@"浏览模式 · 拖动平移；选择道路、住宅、水井或营地开始建造。":@"道路可按住拖动预览；绿=新增，蓝=已有免费，红=整条拒绝。";_nativeCanvas.needsDisplay=YES;return;}
    const auto reason=[](const std::string& error)->NSString* {
        if(error.empty())return @"可建";if(error=="insufficient_funds")return @"余额不足，整条不建";
        if(error=="outside_scene")return @"超出城市边界，已取消";
        if(error=="occupied_or_unsupported_terrain")return @"占用/地形不可建，整条不建";
        if(error=="object_limit")return @"已达3999次总建造上限（含已拆），不能继续建造";
        if(error=="road_path_already_present")return @"全为已有道路，无需施工";
        return [NSString stringWithUTF8String:error.c_str()];};
    NSBezierPath* path=[NSBezierPath bezierPath];NSBezierPath* existing=[NSBezierPath bezierPath];
    const auto add=[&](NSBezierPath* to,int x,int y){const auto p=game_lab::nativeSessionProject(_nativeCamera,x,y);const double w=40*_nativeCamera.scale,h=20*_nativeCamera.scale;
        [to moveToPoint:NSMakePoint(p.x,p.y-h)];[to lineToPoint:NSMakePoint(p.x+w,p.y)];[to lineToPoint:NSMakePoint(p.x,p.y+h)];[to lineToPoint:NSMakePoint(p.x-w,p.y)];[to closePath];};
    const auto [x,y]=*_nativeHoverTile;
    if(_nativeCanvas.roadMode){
        const auto start=_nativeRoadGesture.start.value_or(*_nativeHoverTile),end=_nativeRoadGesture.end.value_or(*_nativeHoverTile);
        const auto plan=game_lab::planNativeRoadPath(_nativeSource->seed,*_nativeState,start.first,start.second,end.first,end.second);
        for(const auto& cell:plan.cells)add(cell.existing?existing:path,cell.x,cell.y);
        _nativeCanvas.previewAllowed=plan.error.empty();
        _nativeHover.stringValue=[NSString stringWithFormat:@"道路 (%d,%d)→(%d,%d) · 先X后Y · 新增 %zu / 已有 %zu · 总计 %lld 金 · %@ · %@",
            start.first,start.second,end.first,end.second,plan.newCells,plan.cells.size()-plan.newCells,plan.cost,reason(plan.error),_nativeRoadGesture.start?(plan.error.empty()?@"松开整条提交":@"松开不提交"):@"按住拖动或单击"];
    }else{
        const bool well=_nativeTool.indexOfSelectedItem==3,hunter=_nativeTool.indexOfSelectedItem==4;const auto result=game_lab::nativeSessionPlacementPreview(_nativeSource->seed,*_nativeState,well?game_lab::NativeSessionAction::Well:hunter?game_lab::NativeSessionAction::Hunter:game_lab::NativeSessionAction::House,x,y);
        for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx)add(path,x+dx,y+dy);
        _nativeCanvas.previewAllowed=result.committed;
        _nativeHover.stringValue=[NSString stringWithFormat:@"%@ 2×2 (%d,%d) · %u 金 · %@ · 单击确认，暂停时可建",well?@"水井":hunter?@"猎人营地":@"住宅",x,y,well?_nativeSource->seed.wellCost:hunter?_nativeSource->seed.hunterCost:_nativeSource->seed.houseCost,reason(result.error)];
    }
    _nativeCanvas.previewPath=path;_nativeCanvas.existingPath=existing;_nativeCanvas.needsDisplay=YES;
}
- (void)onNativeTool:(id)sender {(void)sender;if(_nativeTool.indexOfSelectedItem>0)_nativeCategoryIndex=game_lab::nativeToolCategory(unsigned(_nativeTool.indexOfSelectedItem));[self updateNativeToolPalette];_nativeDemolition.reset();[_nativeCanvas cancelGesture];_nativeRoadGesture.cancel();[self updateNativePreview];[_nativeCanvas refreshPointer];}
- (void)onNativeCancel:(id)sender {(void)sender;_nativeDemolition.reset();[_nativeCanvas cancelGesture];_nativeRoadGesture.cancel();[_nativeTool selectItemAtIndex:0];[self updateNativeToolPalette];[self updateNativeInspection];[self updateNativePreview];}
- (void)nativeViewCamera:(const game_lab::OriginalMapCamera&)camera {
    [self flushNativeVisualForUI];if(!_nativeState)return;
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Camera).state;
    __weak EmperorAppDelegate* weakSelf=self;
    const auto result=[self beginNativeCandidate:*_nativeState source:_nativeSource camera:camera completion:^(BOOL ok){EmperorAppDelegate* me=weakSelf;if(!me)return;if(!ok)me->_nativeMessage="相机绘制失败，保留原画面。";[me updateNativeSessionLabels];[me->_nativeCanvas refreshPointer];}];
    if(result==NativeInstallResult::Failed)[self updateNativeSessionLabels];
}
- (void)onNativeZoomIn:(id)sender {(void)sender;[_nativeCanvas cancelGesture];if(_nativeSource)[self nativeViewCamera:game_lab::nativeSessionZoom(_nativeCamera,_nativeSource->seed.side,1,_nativeCamera.width/2.,_nativeCamera.height/2.)];}
- (void)onNativeZoomOut:(id)sender {(void)sender;[_nativeCanvas cancelGesture];if(_nativeSource)[self nativeViewCamera:game_lab::nativeSessionZoom(_nativeCamera,_nativeSource->seed.side,-1,_nativeCamera.width/2.,_nativeCamera.height/2.)];}
- (void)updateNativeSessionLabels {
    if(!_nativeState||!_nativeSource)return;
    const auto& s=*_nativeState;const auto& seed=_nativeSource->seed;size_t houses=0,roads=0,wells=0,hunters=0;
    for(const auto& o:s.objects){houses+=o.kind==game_lab::NativeObjectKind::House;roads+=o.kind==game_lab::NativeObjectKind::Road;wells+=o.kind==game_lab::NativeObjectKind::Well;hunters+=o.kind==game_lab::NativeObjectKind::Hunter;}
    _nativeStatus.stringValue=[NSString stringWithFormat:@"原生 v%u · %@ · 更新 %llu · 人口 %llu · 余额 %lld · 道路 %zu · 住宅 %zu · 水井 %zu · 营地 %zu\n%@",
        s.rulesVersion,s.paused?@"已暂停":@"运行中",s.ticks,game_lab::nativePopulation(s),s.funds,roads,houses,wells,hunters,[NSString stringWithUTF8String:_nativeMessage.c_str()]];
    _nativePanel.title=[NSString stringWithFormat:@"原始数据 · 原生可玩子集 v%u",s.rulesVersion];
    [_nativeTool itemAtIndex:3].enabled=s.rulesVersion>=2;[_nativeTool itemAtIndex:4].enabled=s.rulesVersion>=3;[_nativeTool itemAtIndex:5].enabled=s.rulesVersion>=4;
    if((s.rulesVersion<4&&_nativeTool.indexOfSelectedItem==5)||(s.rulesVersion==1&&_nativeTool.indexOfSelectedItem==3)||(s.rulesVersion<3&&_nativeTool.indexOfSelectedItem==4))[_nativeTool selectItemAtIndex:0];
    _nativeHuntingToggle.enabled=s.rulesVersion>=3;_nativeHuntingToggle.state=s.rulesVersion>=3&&s.huntingEnabled?NSControlStateValueOn:NSControlStateValueOff;
    if(s.rulesVersion>=3)_nativeFoodInfo.stringValue=[NSString stringWithFormat:@"肉 %u/%u · 累产 %llu / 累耗 %llu · 上轮产 %u / 老居民吃 %u（需%u，缺%u）/ 新入住 %u · %@",s.foodStock,game_lab::nativeFoodCapacity,s.foodProduced,s.foodConsumed,s.lastFood.produced,s.lastFood.fed,s.lastFood.demand,s.lastFood.demand-s.lastFood.fed,s.lastFood.arrivals,s.huntingEnabled?@"生产开启":@"生产暂停"];
    else _nativeFoodInfo.stringValue=@"当前旧版本不运行食物生产/消费，入住规则保持原样；不会自动升级。";
    NSString* policy=s.rulesVersion>=3?@"井邻路供水/外扩4格；住宅须邻路+有水。营地邻路每2更新产4肉，库存100。\n同轮先产后吃：每人1份，新住民另1份（按住宅编号）；缺餐停增，不迁出。":s.rulesVersion==2?@"v2：井邻路启用，占地外扩4格、不限户数；住宅四格有水且邻路，每2更新+1人。缺水停增，不迁出。":@"v1旧规则：邻路住宅每2更新+1人，不检查供水/食物；水井和营地不可建。";
    if(s.rulesVersion>=5)policy=[policy stringByAppendingString:@"\nv5原生税：每4更新按本轮开始时邻路住民每人1金；先税后餐，暂停不收，无税务建筑/劳动等价。视图不产生收入。"];
    if(s.rulesVersion>=6)policy=[policy stringByAppendingFormat:@"\nv6劳动力：住民×%u%%（原版来源存档的难度/工资/健康档）；营地%u人、水井%u人（原版模型第5列），按原版类别优先级分配。营地按工人比例产肉；新住民不另耗肉；营地缺工时缺粮不阻止入住，满员仍缺粮才停增。水井工人仅显示，不改供水范围。",seed.laborPercent,seed.hunterWorkers,seed.wellWorkers];
    _nativeRulesLabel.stringValue=[NSString stringWithFormat:@"原生规则 v%u · 来源 %@\n原始Normal价格：路%u/格、房%u/2×2（%u人）、井%u/2×2、营地%u/2×2；暂停可建。\n%@\n原生简化：无原任务/农时/猎人/运输/市场/劳动等价；无原版经济/灾害/贸易。旧档保留原规则。",s.rulesVersion,[NSString stringWithUTF8String:seed.source.c_str()],seed.roadCost,seed.houseCost,seed.houseCapacity,seed.wellCost,seed.hunterCost,policy];
    _nativeRulesLabel.stringValue=[_nativeRulesLabel.stringValue stringByAppendingFormat:@"\n原始界面装饰 %lu/18；中文使用 macOS 系统字体。缺少装饰时采用纯色控件，不改变玩法。",(unsigned long)_nativeUIImages.count];
    if(s.rulesVersion>=4)_nativeFoodInfo.stringValue=[_nativeFoodInfo.stringValue stringByAppendingFormat:@" · 餐后拆迁%u人",s.departedSinceMeal];
    if(s.rulesVersion>=6){const auto& q=[self nativeFeedback];const auto& l=q.labor;
        _nativeFoodInfo.stringValue=[_nativeFoodInfo.stringValue stringByAppendingFormat:@"\n劳力 %d（住民×%u%%）· 岗位 %d · 就业 %d · 缺工 %d%@",l.available,seed.laborPercent,l.demand,l.employed,l.shortage,q.food.laborOverride?@" · 营地缺工，缺粮仍入住":@""];}
    if(s.rulesVersion>=5){const auto& plan=[self nativeFeedback].tax;const auto& t=plan.cycle;
        NSString* reason=!plan.error.empty()?@"余额上限：下轮将拒绝":t.eligible?@"未到税期不收":t.disconnected?@"断路住民不收":@"无住民可收";
        _nativeTaxInfo.stringValue=[NSString stringWithFormat:@"税每%u更新 · 距下次%llu · 当前可税%u人/断路%u人/空房%u · 上次+%llu · 累计%llu（%llu期）· %@",game_lab::nativeTaxPeriod,game_lab::nativeTaxPeriod-s.ticks%game_lab::nativeTaxPeriod,t.eligible,t.disconnected,t.emptyHouses,s.lastTax.income,s.taxReceived,s.taxCycles,reason];
    }else _nativeTaxInfo.stringValue=@"旧版本不运行税收；保留旧资金账本，不自动升级或补发收入。";
    _nativeResume.enabled=s.paused;_nativePause.enabled=!s.paused;
    _nativeCanvas.accessibilityLabel=[NSString stringWithFormat:@"原生 v%u 城市，来源 %@，更新 %llu，人口 %llu，余额 %lld，图像 %u×%u，相机 %.2f,%.2f，缩放 %u",
        s.rulesVersion,[NSString stringWithUTF8String:seed.source.c_str()],s.ticks,game_lab::nativePopulation(s),s.funds,_nativeCamera.width,_nativeCamera.height,_nativeCamera.centerX,_nativeCamera.centerY,_nativeCamera.scale];
    _nativeStatus.stringValue=[_nativeStatus.stringValue stringByAppendingString:[self nativeHasUnsavedChanges]?@" · 未保存":@" · 无未保存变更"];
    _nativeResources.stringValue=[NSString stringWithFormat:@"资金 %lld     人口 %llu     肉 %u / 100     更新 %llu",s.funds,game_lab::nativePopulation(s),s.foodStock,s.ticks];
    if(_nativeMetalEnabled)_nativeStatus.stringValue=[_nativeStatus.stringValue stringByAppendingFormat:@" · %@ · 待执行%zu",[NSString stringWithUTF8String:_nativeRenderRoute.c_str()],_nativeCommands.size()+size_t(bool(_nativePending))];
    _nativeCompactState.stringValue=[NSString stringWithFormat:@"%@ · 原生 v%u · 更新 %llu",s.paused?@"已暂停":@"运行中",s.rulesVersion,s.ticks];
    if([self nativeHasUnsavedChanges])_nativeCompactState.stringValue=[_nativeCompactState.stringValue stringByAppendingString:@" · 未保存"];
    _nativeCompactState.toolTip=_nativeStatus.stringValue;
    _nativeNotice.stringValue=[NSString stringWithUTF8String:_nativeMessage.c_str()];_nativeNotice.toolTip=_nativeNotice.stringValue;
    const auto& feedback=[self nativeFeedback];
    _nativeServiceSummary.stringValue=[NSString stringWithUTF8String:game_lab::nativeCityFeedbackSummary(s,feedback).c_str()];

    [self updateNativeMinimap];[self updateNativeInspection];[self updateNativeToolPalette];[self updateNativePreview];
    _nativeServiceSummary.toolTip=[NSString stringWithUTF8String:game_lab::nativeCityFeedbackExplanation(s,feedback).c_str()];
}
- (BOOL)nativeCommand:(const game_lab::NativeSessionCommand&)command {
    if(_nativePending||(!_nativeCommands.empty()&&!_nativeProcessingQueued&&!_nativeFlushing)){
        if(_nativePending&&_nativePending->source!=_nativeSource){_nativeMessage="正在切换城市；本次建造未接收，请载入完成后再操作。";[self updateNativeSessionLabels];return NO;}
        if(_nativeCommands.size()>=64){_nativeMessage="待执行操作已达64项，请稍后重试；本次未扣费。";[self updateNativeSessionLabels];return NO;}
        _nativeCommands.push_back(command);_nativeMessage="操作已排队，画面完成后按顺序校验；尚未扣费。";[self updateNativeSessionLabels];return YES;
    }
    const auto action=command.action;_nativeDemolition.reset();_nativeExitApproved=NO;
    if(!_nativeSource||!_nativeState)return NO;
    const double operationStarted=CACurrentMediaTime();
    auto next=*_nativeState;const auto result=game_lab::transactNativeSession(_nativeSource->seed,next,command);
    if(!result.committed){_nativeMessage=result.error=="object_limit"?"已达3999次总建造上限（含已拆对象）；未执行，也未扣费。":result.error=="tax_overflow"?"税收将超过余额上限；本轮税收、供餐和人口均未提交，城市已暂停。":"未执行："+result.error;[self updateNativeSessionLabels];return NO;}
    const bool redraw=action==game_lab::NativeSessionAction::Demolish||action==game_lab::NativeSessionAction::Hunter||action==game_lab::NativeSessionAction::Well||action==game_lab::NativeSessionAction::RoadPath||action==game_lab::NativeSessionAction::Road||action==game_lab::NativeSessionAction::House||game_lab::nativePopulation(next)!=game_lab::nativePopulation(*_nativeState);
    __weak EmperorAppDelegate* weakSelf=self;
    void (^complete)(BOOL)=^(BOOL ok){EmperorAppDelegate* me=weakSelf;if(!me)return;
        if(ok)me->_nativeMessage=action==game_lab::NativeSessionAction::Tick?"原生规则计时；原版任务与事件未运行。":"操作已提交。";
        else if(action==game_lab::NativeSessionAction::Tick&&me->_nativeState){const auto reason=me->_nativeMessage;[me stopNativeSessionTimer];game_lab::transactNativeSession(me->_nativeSource->seed,*me->_nativeState,{game_lab::NativeSessionAction::Pause,me->_nativeState->revision});me->_nativeMessage=reason;}
        [me updateNativeSessionLabels];
        if(action==game_lab::NativeSessionAction::Tick)std::fprintf(stderr,"native_tick completed=%d redraw=%d elapsed_ms=%.3f tick=%llu\n",ok?1:0,redraw?1:0,(CACurrentMediaTime()-operationStarted)*1000,me->_nativeState?me->_nativeState->ticks:0);
    };
    if(redraw){const auto status=[self beginNativeCandidate:next source:_nativeSource camera:_nativeCamera completion:complete];
        if(status==NativeInstallResult::Failed)return NO;
        if(status==NativeInstallResult::Pending){_nativeMessage="画面准备中；操作尚未扣费或提交。";[self updateNativeSessionLabels];}return YES;}
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Cancel).state;_nativeState=std::move(next);complete(YES);return YES;
}

- (BOOL)nativeAction:(game_lab::NativeSessionAction)action x:(int64_t)x y:(int64_t)y {
    if(!_nativeState)return NO;const game_lab::NativeSessionCommand command{action,_nativeState->revision,x,y};return [self nativeCommand:command];
}
- (void)finishNativeRoadX:(double)px y:(double)py {
    if(!_nativeState||!_nativeSource)return;const auto endpoints=_nativeRoadGesture.finish(game_lab::nativeSessionTile(_nativeCamera,_nativeSource->seed.side,px,py));
    if(!endpoints){[self updateNativePreview];return;}
    const auto [start,end]=*endpoints;_nativeX.integerValue=end.first;_nativeY.integerValue=end.second;
    const game_lab::NativeSessionCommand command{game_lab::NativeSessionAction::RoadPath,_nativeState->revision,start.first,start.second,end.first,end.second};[self nativeCommand:command];
}
- (void)onNativeTick:(NSTimer*)timer {
    (void)timer;if(_nativePending||!_nativeCommands.empty())return;
    if(!_nativePanel.visible||!_nativeState||_nativeState->paused){[self stopNativeSessionTimer];return;}
    if(![self nativeAction:game_lab::NativeSessionAction::Tick x:0 y:0]){
        const auto reason=_nativeMessage;[self nativeAction:game_lab::NativeSessionAction::Pause x:0 y:0];_nativeMessage=reason;[self stopNativeSessionTimer];[self updateNativeSessionLabels];}
}
- (void)onNativeResume:(id)sender {
    (void)sender;[self flushNativeVisualForUI];[_nativeCanvas cancelGesture];if(![self nativeAction:game_lab::NativeSessionAction::Resume x:0 y:0])return;
    [self stopNativeSessionTimer];const unsigned speeds[]={1,2,4};_nativeTimer=[NSTimer scheduledTimerWithTimeInterval:game_lab::nativePresentationInterval(speeds[_nativeSpeedIndex]) target:self selector:@selector(onNativeTick:) userInfo:nil repeats:YES];
}
- (void)onNativePause:(id)sender {(void)sender;[self flushNativeVisualForUI];[_nativeCanvas cancelGesture];[self stopNativeSessionTimer];[self nativeAction:game_lab::NativeSessionAction::Pause x:0 y:0];}
- (void)onNativeHunting:(id)sender {
    (void)sender;[_nativeCanvas cancelGesture];[self nativeAction:_nativeHuntingToggle.state==NSControlStateValueOn?game_lab::NativeSessionAction::HuntingResume:game_lab::NativeSessionAction::HuntingPause x:0 y:0];
}
- (void)onNativePlace:(id)sender {
    (void)sender;[_nativeCanvas cancelGesture];if(_nativeTool.indexOfSelectedItem==5){_nativeMessage="请在地图点选自己的对象，再用下方按钮确认；坐标建造按钮不执行拆除。";[self updateNativeSessionLabels];return;}if(_nativeTool.indexOfSelectedItem==0){_nativeMessage="请先选择道路或住宅。";[self updateNativeSessionLabels];return;}const std::string xs=_nativeX.stringValue.UTF8String,ys=_nativeY.stringValue.UTF8String;int64_t x=0,y=0;
    const auto a=std::from_chars(xs.data(),xs.data()+xs.size(),x),b=std::from_chars(ys.data(),ys.data()+ys.size(),y);
    if(a.ec!=std::errc{}||a.ptr!=xs.data()+xs.size()||b.ec!=std::errc{}||b.ptr!=ys.data()+ys.size()){
        _nativeMessage="坐标需要非负整数。";[self updateNativeSessionLabels];return;}
    if(!_nativeState)return;const game_lab::NativeSessionCommand command{_nativeTool.indexOfSelectedItem==1?game_lab::NativeSessionAction::RoadPath:_nativeTool.indexOfSelectedItem==3?game_lab::NativeSessionAction::Well:_nativeTool.indexOfSelectedItem==4?game_lab::NativeSessionAction::Hunter:game_lab::NativeSessionAction::House,_nativeState->revision,x,y,x,y};[self nativeCommand:command];
}
- (void)onNativeFocus:(id)sender {
    (void)sender;[_nativeCanvas cancelGesture];if(!_nativeSource||!_nativeState)return;
    const double x=_nativeX.doubleValue,y=_nativeY.doubleValue;
    if(!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=_nativeSource->seed.side||y>=_nativeSource->seed.side){_nativeMessage="相机坐标超出场景。";[self updateNativeSessionLabels];return;}
    auto camera=_nativeCamera;camera.centerX=x;camera.centerY=y;[self nativeViewCamera:camera];
}
- (void)nativeError:(NSString*)message {
    NSAlert* alert=[[NSAlert alloc]init];alert.messageText=@"原生城市";alert.informativeText=message;[alert addButtonWithTitle:@"知道了"];[alert runModal];
}
- (BOOL)nativeHasUnsavedChanges {
    if(!_nativeSource||!_nativeState)return NO;
    const auto bytes=game_lab::nativeSessionContent(_nativeSource->seed,*_nativeState);
    return bytes.empty()||bytes!=_nativeSavedContent;
}
- (BOOL)saveNativeAtPath:(NSString*)path {
    [self flushNativeVisualForUI];
    if(!_nativeSource||!_nativeState)return NO;
    _nativeDemolition.reset();[_nativeCanvas cancelGesture];
    if(!_nativeState->paused)[self onNativePause:nil];
    const auto bytes=game_lab::encodeNativeSession(_nativeSource->seed,*_nativeState);std::string error;
    if(!game_lab::writeNativeSessionNew(path.UTF8String,_resourceRoot.configuredDirectory(),bytes,error)){
        _nativeMessage=error;[self updateNativeSessionLabels];return NO;
    }
    _nativeFile.stringValue=path;_nativeSavedContent=game_lab::nativeSessionContent(_nativeSource->seed,*_nativeState);
    _nativeMessage="会话已保存，城市保持暂停；源 SAV 和已有存档均未改动。";[self updateNativeSessionLabels];return YES;
}
- (BOOL)saveNativeWithChooser {
    [self flushNativeVisualForUI];
    if(!_nativeSource||!_nativeState)return YES;
    if(!_nativeState->paused)[self onNativePause:nil];
    NSSavePanel* panel=[NSSavePanel savePanel];panel.title=@"保存原生城市";panel.message=@"保存独立 .encity 会话。使用新名称，已有文件不会覆盖；取消后城市仍暂停。";
    panel.nameFieldStringValue=[NSString stringWithFormat:@"城市-v%u-%@.encity",_nativeState->rulesVersion,NSUUID.UUID.UUIDString];
    panel.directoryURL=[NSURL fileURLWithPath:[_nativeFile.stringValue stringByDeletingLastPathComponent] isDirectory:YES];
    if([panel runModal]!=NSModalResponseOK)return NO;
    if([self saveNativeAtPath:panel.URL.path])return YES;
    [self nativeError:[NSString stringWithUTF8String:_nativeMessage.c_str()]];return NO;
}
- (void)onNativeSaveAs:(id)sender {(void)sender;[self saveNativeWithChooser];}
// Kept for explicit local/debug callers. Normal users use the standard chooser.
- (void)onNativeSave:(id)sender {(void)sender;[self saveNativeAtPath:_nativeFile.stringValue];}
- (BOOL)confirmNativeReplacement:(NSString*)operation {
    [self flushNativeVisualForUI];
    if(![self nativeHasUnsavedChanges])return YES;
    if(!_nativeState->paused)[self onNativePause:nil];
    NSAlert* alert=[[NSAlert alloc]init];alert.messageText=@"城市有未保存的进度";
    alert.informativeText=[NSString stringWithFormat:@"%@前要保存吗？城市已暂停。取消会保留当前城市。",operation];
    [alert addButtonWithTitle:@"保存后继续"].accessibilityIdentifier=@"native-unsaved-save";
    [alert addButtonWithTitle:@"取消"].accessibilityIdentifier=@"native-unsaved-cancel";
    [alert addButtonWithTitle:@"不保存继续"].accessibilityIdentifier=@"native-unsaved-discard";
    const auto answer=[alert runModal];if(answer==NSAlertFirstButtonReturn)return [self saveNativeWithChooser];return answer==NSAlertThirdButtonReturn;
}
- (void)onNativeReturn:(id)sender {
    (void)sender;if(_nativePanel){[_nativePanel makeKeyAndOrderFront:nil];[self updateNativeSessionLabels];}
    else [self nativeError:@"当前没有打开的城市。请选择“新建原生城市”或“继续游戏”。"];
}
- (void)cancelNativeStartWork {
    if(_nativeStartWork)_nativeStartWork->cancelled=true;
    if(_nativeStartPreviewWork)_nativeStartPreviewWork->cancelled=true;
    _nativeStartWork.reset();_nativeStartPreviewWork.reset();
    [_nativeStartSpinner stopAnimation:nil];
}
- (void)postNativeStartUI:(dispatch_block_t)block {
    // Service completions in the alert's modal loop as well as ordinary UI.
    CFRunLoopPerformBlock(CFRunLoopGetMain(),(__bridge CFTypeRef)@[NSRunLoopCommonModes,NSModalPanelRunLoopMode],block);
    CFRunLoopWakeUp(CFRunLoopGetMain());
}
- (void)onNativeStartMenuItem:(NSMenuItem*)sender {
    if(!_nativeStartPicker||sender.tag<0||size_t(sender.tag)>=_nativeStartCatalog.entries.size())return;
    [_nativeStartPicker selectItemAtIndex:sender.tag];[self onNativeStartSelection:sender];
}
- (void)setNativeStartDetail:(const game_lab::NativeStartEntry&)entry status:(NSString*)status {
    NSString* size=entry.metadata.side?[NSString stringWithFormat:@"%u × %u 格（原始活动方阵）",entry.metadata.side,entry.metadata.side]:@"尺寸未确认";
    NSString* funds=entry.metadata.funds?[NSString stringWithFormat:@"原始资金：%lld",*entry.metadata.funds]:@"资金未确认";
    _nativeStartDetail.stringValue=[NSString stringWithFormat:@"文件：%@\n%@\n%@\n\n%@",[NSString stringWithUTF8String:entry.source.c_str()],size,funds,status];
    _nativeStartDetail.toolTip=_nativeStartDetail.stringValue;
}
- (void)onNativeStartSelection:(id)sender {
    (void)sender;if(_nativeStartPreviewWork)_nativeStartPreviewWork->cancelled=true;
    _nativeStartPreviewWork=std::make_shared<game_lab::NativeStartWork>();
    _nativeStartConfirm.enabled=NO;_nativeStartPreview.image=nil;
    _nativeStartPreview.accessibilityLabel=@"所选来源预览尚未完成。";
    const NSInteger index=_nativeStartPicker.indexOfSelectedItem;
    if(index<0||size_t(index)>=_nativeStartCatalog.entries.size())return;
    const auto entry=_nativeStartCatalog.entries[size_t(index)];
    if(!entry.supported()){
        [_nativeStartSpinner stopAnimation:nil];[self setNativeStartDetail:entry status:[NSString stringWithFormat:@"暂不可创建\n%@",[NSString stringWithUTF8String:entry.reason.c_str()]]];return;
    }
    [self setNativeStartDetail:entry status:@"正在后台生成预览…可以换选或取消。"];
    [_nativeStartSpinner startAnimation:nil];
    const auto root=_resourceRoot;const auto generation=_nativeStartWork,selection=_nativeStartPreviewWork;
    __weak EmperorAppDelegate* weakSelf=self;
    dispatch_async(_nativeStartQueue,^{@autoreleasepool {
        if(generation->cancelled||selection->cancelled)return;
        auto pixels=std::make_shared<game_lab::DecodedImage>();std::string error;
        try {
            const auto source=game_lab::reloadNativeStartSource(root,entry,error);
            if(generation->cancelled||selection->cancelled)return;
            if(source){const auto overview=game_lab::makeNativeTerrainOverview(source->save.terrain,480);
                if(overview){pixels->width=pixels->height=overview->pixels;pixels->pixelsBGRA=overview->bgra;}}
            if(error.empty()&&!pixels->valid())error="地形预览不可用，暂不创建；当前城市不受影响。";
        }catch(const std::exception&){error="预览读取失败；当前城市未改变，可以取消或重新选择。";}
        if(generation->cancelled||selection->cancelled)return;
        // The worker owns data only. UI and the live city are main-thread-only.
        [weakSelf postNativeStartUI:^{EmperorAppDelegate* me=weakSelf;
            if(!me||!game_lab::nativeStartWorkCurrent(generation,me->_nativeStartWork)||!game_lab::nativeStartWorkCurrent(selection,me->_nativeStartPreviewWork)||!me->_nativeStartPicker)return;
            [me->_nativeStartSpinner stopAnimation:nil];
            if(error.empty())me->_nativeStartPreview.image=[me nativeCPUImage:*pixels];
            me->_nativeStartConfirm.enabled=me->_nativeStartPreview.image!=nil;
            NSString* status=me->_nativeStartConfirm.enabled?@"数据条件通过 · 可创建原生 v6\n创建时还会核验原图素材；非原版任务或原版续玩。": [NSString stringWithFormat:@"暂不可创建\n%@",error.empty()?@"预览图像创建失败。":[NSString stringWithUTF8String:error.c_str()]];
            [me setNativeStartDetail:entry status:status];
            me->_nativeStartPreview.accessibilityLabel=me->_nativeStartPreview.image?@"所选SAV的真实地形概览，原生配色，不是原版关卡截图。":@"所选来源没有可用预览。";
        }];
    }});
}
- (void)beginNativeStartCatalog {
    const auto root=_resourceRoot;const auto generation=_nativeStartWork;__weak EmperorAppDelegate* weakSelf=self;
    dispatch_async(_nativeStartQueue,^{@autoreleasepool {
        if(generation->cancelled)return;
        std::optional<game_lab::NativeStartCatalog> result;std::string error;
        try {
            const auto original=game_lab::enumerateOriginalMapSources(root);
            if(generation->cancelled)return;
            error=original.error;
            if(error.empty()){
                std::vector<std::string> paths;for(const auto& entry:original.sources)if(entry.kind==game_lab::OriginalMapSourceKind::SaveFile)paths.push_back(entry.relativePath);
                result=game_lab::loadNativeStartCatalog(root,std::move(paths),generation,[weakSelf,generation](size_t done,size_t total){
                    [weakSelf postNativeStartUI:^{EmperorAppDelegate* me=weakSelf;
                        if(!me||!game_lab::nativeStartWorkCurrent(generation,me->_nativeStartWork)||!me->_nativeStartPicker)return;
                        me->_nativeStartProgress.stringValue=[NSString stringWithFormat:@"后台核验 %zu / %zu 份 · 可随时取消",done,total];
                    }];
                });
            }
        }catch(const std::exception&){error="来源读取失败；当前城市未改变，可以取消后重试。";}
        if(generation->cancelled)return;
        [weakSelf postNativeStartUI:^{EmperorAppDelegate* me=weakSelf;
            if(!me||!game_lab::nativeStartWorkCurrent(generation,me->_nativeStartWork)||!me->_nativeStartPicker)return;
            [me->_nativeStartSpinner stopAnimation:nil];
            if(!error.empty()||!result||result->entries.empty()){
                me->_nativeStartProgress.stringValue=@"未找到可列出的来源";
                me->_nativeStartDetail.stringValue=error.empty()?@"当前数据目录没有SAV来源。MAP/PAK暂不能开局；继续游戏可选择已有.encity会话。":[NSString stringWithUTF8String:error.c_str()];return;
            }
            me->_nativeStartCatalog=*result;
            for(const auto& entry:me->_nativeStartCatalog.entries){
                NSString* name=[NSString stringWithUTF8String:entry.label.c_str()]?:@"无法显示文件名；请查看来源详情";
                NSMenuItem* item=[[NSMenuItem alloc]initWithTitle:[NSString stringWithFormat:@"%@ · %@",entry.supported()?@"数据通过":@"暂不可用",name] action:@selector(onNativeStartMenuItem:) keyEquivalent:@""];
                item.target=me;item.tag=me->_nativeStartPicker.numberOfItems;[me->_nativeStartPicker.menu addItem:item];
            }
            const auto supported=std::count_if(result->entries.begin(),result->entries.end(),[](const auto& entry){return entry.supported();});
            me->_nativeStartProgress.stringValue=[NSString stringWithFormat:@"已核验 %zu / %zu 份 · %zu 份数据通过%@",result->entries.size(),result->entries.size(),size_t(supported),result->omitted?[NSString stringWithFormat:@" · 另%zu份未检查",result->omitted]:@""];
            me->_nativeStartPicker.enabled=YES;
            const auto first=std::find_if(result->entries.begin(),result->entries.end(),[](const auto& entry){return entry.supported();});
            if(first!=result->entries.end())[me->_nativeStartPicker selectItemAtIndex:NSInteger(first-result->entries.begin())];
            [me onNativeStartSelection:nil];
        }];
    }});
}
- (void)onNativeNew:(id)sender {
    (void)sender;if(_nativeStartPicker)return;[self flushNativeVisualForUI];
    if(!_nativeStartQueue)_nativeStartQueue=dispatch_queue_create("org.emperor.native-source-reader",DISPATCH_QUEUE_SERIAL);
    _nativeStartWork=std::make_shared<game_lab::NativeStartWork>();_nativeStartCatalog={};
    NSAlert* alert=[NSAlert new];alert.messageText=@"选择开局来源 · 原生 v6";
    alert.informativeText=@"后台逐份核验，最多128份。按真实文件名选择，不支持项可查看原因；可随时取消。源文件只读，MAP/PAK开局与原版任务尚不兼容。";
    NSView* content=[[NSView alloc]initWithFrame:NSMakeRect(0,0,620,350)];
    _nativeStartProgress=[NSTextField labelWithString:@"正在后台查找SAV来源…可随时取消"];_nativeStartProgress.frame=NSMakeRect(26,318,594,24);_nativeStartProgress.font=[NSFont systemFontOfSize:12];_nativeStartProgress.accessibilityIdentifier=@"native-new-progress";[content addSubview:_nativeStartProgress];
    _nativeStartSpinner=[[NSProgressIndicator alloc]initWithFrame:NSMakeRect(0,321,18,18)];_nativeStartSpinner.style=NSProgressIndicatorStyleSpinning;[_nativeStartSpinner startAnimation:nil];[content addSubview:_nativeStartSpinner];
    _nativeStartPicker=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(0,282,620,28) pullsDown:NO];_nativeStartPicker.font=[NSFont systemFontOfSize:13];_nativeStartPicker.accessibilityIdentifier=@"native-new-source";_nativeStartPicker.enabled=NO;
    _nativeStartPicker.target=self;_nativeStartPicker.action=@selector(onNativeStartSelection:);[content addSubview:_nativeStartPicker];
    _nativeStartPreview=[[NSImageView alloc]initWithFrame:NSMakeRect(0,28,240,240)];_nativeStartPreview.imageScaling=NSImageScaleProportionallyUpOrDown;_nativeStartPreview.accessibilityIdentifier=@"native-new-preview";[content addSubview:_nativeStartPreview];
    NSTextField* caption=[NSTextField labelWithString:@"真实地形概览 · 原生配色"];caption.frame=NSMakeRect(0,0,260,24);caption.font=[NSFont systemFontOfSize:12];[content addSubview:caption];
    _nativeStartDetail=[NSTextField wrappingLabelWithString:@"尚未完成核验，当前城市保持不变。"];_nativeStartDetail.frame=NSMakeRect(260,10,360,258);_nativeStartDetail.font=[NSFont systemFontOfSize:13];_nativeStartDetail.accessibilityIdentifier=@"native-new-detail";[content addSubview:_nativeStartDetail];
    alert.accessoryView=content;_nativeStartConfirm=[alert addButtonWithTitle:@"创建 v6 城市"];_nativeStartConfirm.accessibilityIdentifier=@"native-new-confirm";_nativeStartConfirm.enabled=NO;[alert addButtonWithTitle:@"取消"].accessibilityIdentifier=@"native-new-cancel";
    [self beginNativeStartCatalog];
    const auto answer=[alert runModal];const NSInteger index=_nativeStartPicker.indexOfSelectedItem;
    const auto selected=index>=0&&size_t(index)<_nativeStartCatalog.entries.size()?std::make_optional(_nativeStartCatalog.entries[size_t(index)]):std::nullopt;
    const BOOL previewReady=_nativeStartConfirm.enabled;
    [self cancelNativeStartWork];
    _nativeStartPicker=nil;_nativeStartDetail=nil;_nativeStartPreview=nil;_nativeStartConfirm=nil;_nativeStartProgress=nil;_nativeStartSpinner=nil;_nativeStartCatalog={};
    if(answer!=NSAlertFirstButtonReturn||!previewReady||!selected||!selected->supported()||![self confirmNativeReplacement:@"新建另一座城市"])return;
    // Re-read after all modal confirmations; never silently start a changed seed.
    std::string error;auto source=game_lab::reloadNativeStartSource(_resourceRoot,*selected,error);
    if(!source){[self nativeError:[NSString stringWithUTF8String:error.c_str()]];return;}
    auto state=game_lab::startNativeSession(source->seed,game_lab::nativeLatestRulesVersion);[self presentNativeSource:std::move(*source) state:std::move(state) checkpoint:nil];
}
- (void)onNativeContinue:(id)sender {
    [self flushNativeVisualForUI];
    (void)sender;NSOpenPanel* panel=[NSOpenPanel openPanel];panel.title=@"继续原生城市";panel.message=@"选择独立 .encity 会话。会自动核对原始来源和规则版本，载入后暂停；不会运行离线补算。";panel.canChooseFiles=YES;panel.canChooseDirectories=NO;panel.allowsMultipleSelection=NO;
    if(_nativeFile.stringValue.length)panel.directoryURL=[NSURL fileURLWithPath:[_nativeFile.stringValue stringByDeletingLastPathComponent] isDirectory:YES];
    if([panel runModal]!=NSModalResponseOK)return;
    std::string error;const auto bytes=game_lab::readNativeSessionFile(panel.URL.path.UTF8String,_resourceRoot.configuredDirectory(),error);
    const auto reference=bytes?game_lab::nativeSessionReference(*bytes):std::nullopt;
    if(!reference){[self nativeError:bytes?@"不是受支持的原生会话；当前城市未替换。":[NSString stringWithUTF8String:error.c_str()]];return;}
    auto source=game_lab::loadNativeSessionSource(_resourceRoot,reference->source,error);
    auto state=source?game_lab::decodeNativeSession(source->seed,*bytes,error):std::nullopt;
    if(!source||!state){[self nativeError:[NSString stringWithFormat:@"会话未载入：%@。请使用保存时的原始数据目录；当前城市未替换。",[NSString stringWithUTF8String:error.c_str()]]];return;}
    NSAlert* info=[[NSAlert alloc]init];info.messageText=[NSString stringWithFormat:@"继续原生 v%u",state->rulesVersion];info.informativeText=[NSString stringWithFormat:@"原始来源：%@\n更新%llu · 人口%llu · 余额%lld\n保留该版本规则，载入后暂停。",[NSString stringWithUTF8String:source->seed.source.c_str()],state->ticks,game_lab::nativePopulation(*state),state->funds];[info addButtonWithTitle:@"继续此会话"].accessibilityIdentifier=@"native-continue-confirm";[info addButtonWithTitle:@"取消"];
    if([info runModal]!=NSAlertFirstButtonReturn||![self confirmNativeReplacement:@"载入另一份会话"])return;
    source=game_lab::reloadNativeStartSource(_resourceRoot,game_lab::nativeStartIdentity(*source),error);
    if(!source){[self nativeError:[NSString stringWithFormat:@"会话未载入：%@。当前城市未替换。",[NSString stringWithUTF8String:error.c_str()]]];return;}
    if(!state->paused){const auto result=game_lab::transactNativeSession(source->seed,*state,{game_lab::NativeSessionAction::Pause,state->revision});if(!result.committed){[self nativeError:@"会话无法安全暂停，未替换当前城市。"];return;}}
    [self presentNativeSource:std::move(*source) state:std::move(*state) checkpoint:panel.URL.path];
}
- (void)onNativeLoad:(id)sender {
    [self flushNativeVisualForUI];
    (void)sender;_nativeDemolition.reset();[_nativeCanvas cancelGesture];if(!_nativeSource||!_nativeState)return;
    std::string error;const auto bytes=game_lab::readNativeSessionFile(_nativeFile.stringValue.UTF8String,_resourceRoot.configuredDirectory(),error);
    if(!bytes){_nativeMessage=error;[self updateNativeSessionLabels];return;}
    const auto state=game_lab::decodeNativeSession(_nativeSource->seed,*bytes,error);
    if(!state){_nativeMessage="会话拒绝："+error;[self updateNativeSessionLabels];return;}
    if(![self confirmNativeReplacement:@"重载会话"])return;
    [self presentNativeSource:*_nativeSource state:*state checkpoint:_nativeFile.stringValue];
}
- (void)onNativeClose:(id)sender {
    [self flushNativeVisualForUI];
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::Cancel).state;
    (void)sender;[_nativeCanvas cancelGesture];if(_nativeState&&!_nativeState->paused)[self onNativePause:nil];[self stopNativeSessionTimer];[_nativePanel orderOut:nil];[self showTab:0];[_window makeKeyAndOrderFront:nil];
}
- (void)onNativeSession:(id)sender {
    (void)sender;
    if(_nativePanel){[_nativePanel makeKeyAndOrderFront:nil];[self updateNativeSessionLabels];return;}
    std::string error;auto source=game_lab::loadNativeSessionSource(_resourceRoot,_originalSourceRelative,error);
    if(!source){_cityPlaybackMessage="原生子集未建立："+error;[self updateCityPlaybackStatusText];
        NSAlert* alert=[[NSAlert alloc]init];alert.messageText=@"原生子集暂不支持该来源";alert.informativeText=[NSString stringWithUTF8String:error.c_str()];[alert addButtonWithTitle:@"知道了"];[alert runModal];return;}
    auto state=game_lab::startNativeSession(source->seed,game_lab::nativeLatestRulesVersion);
    [self presentNativeSource:std::move(*source) state:std::move(state) checkpoint:nil];
}
- (NativeInstallResult)presentNativeSource:(game_lab::NativeSessionSource)source state:(game_lab::NativeSessionState)state checkpoint:(NSString*)checkpoint {
    [self flushNativeVisualForUI];if(_nativeState&&!_nativeState->paused)[self onNativePause:nil];
    _nativeFrameGate=game_lab::changeSceneFrame(_nativeFrameGate,game_lab::SceneFrameChange::BeginSession).state;
    if(!state.paused){const auto paused=game_lab::transactNativeSession(source.seed,state,{game_lab::NativeSessionAction::Pause,state.revision});if(!paused.committed){[self nativeError:@"会话不能安全暂停，未替换当前城市。"];return NativeInstallResult::Failed;}}
    const BOOL created=_nativePanel==nil;
    auto candidateSource=std::make_shared<const game_lab::NativeSessionSource>(std::move(source));
    auto camera=game_lab::OriginalMapCamera{};camera.width=798;camera.height=720;camera.outlines=false;
    camera.centerX=candidateSource->seed.side/2;camera.centerY=candidateSource->seed.side/2;
    double distance=1e30;
    for(unsigned y=0;y+5<=candidateSource->seed.side;++y)for(unsigned x=0;x+5<=candidateSource->seed.side;++x){
        bool available=true;for(unsigned dy=0;dy<5;++dy)for(unsigned dx=0;dx<5;++dx)available&=candidateSource->seed.buildable[size_t(y+dy)*candidateSource->seed.side+x+dx]!=0;
        const double d=std::abs(double(x)-candidateSource->seed.side/2)+std::abs(double(y)-candidateSource->seed.side/2);
        if(available&&d<distance){distance=d;camera.centerX=x+2;camera.centerY=y+2;}
    }
    if(!state.objects.empty()){const auto& first=state.objects.front();camera.centerX=first.x;camera.centerY=first.y;}
    if(created){
    _nativePanel=[[NSPanel alloc]initWithContentRect:NSMakeRect(0,0,1024,768) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
    _nativePanel.title=@"原始数据 · 原生可玩子集 v6";_nativePanel.releasedWhenClosed=NO;_nativePanel.hidesOnDeactivate=NO;_nativePanel.contentMinSize=NSMakeSize(1024,768);_nativePanel.contentMaxSize=NSMakeSize(2048,2048);_nativePanel.acceptsMouseMovedEvents=YES;
    NativeGameChrome* chrome=[[NativeGameChrome alloc]initWithFrame:NSMakeRect(0,0,1024,768)];chrome.topTexture=[self nativeUI:1221];chrome.bottomTexture=[self nativeUI:1222];chrome.sideTexture=[self nativeUI:1223];chrome.lowerSideTexture=[self nativeUI:1224];_nativePanel.contentView=chrome;
    const auto layout=game_lab::nativeGameLayout(1024,768);
    auto label=[&](NSString* text,NSRect frame,CGFloat size){NSTextField* v=[NSTextField wrappingLabelWithString:text];v.frame=frame;v.font=[NSFont systemFontOfSize:size];v.textColor=[NSColor colorWithRed:1 green:.94 blue:.79 alpha:1];v.autoresizingMask=frame.origin.x>=798?NSViewMinXMargin|NSViewMinYMargin:NSViewWidthSizable|NSViewMinYMargin;[chrome addSubview:v];return v;};
    auto button=[&](NSString* title,SEL action,NSString* ident,NSRect frame){NativeGameButton* b=[NativeGameButton buttonWithTitle:title target:self action:action];b.frame=frame;b.font=[NSFont systemFontOfSize:12];b.autoresizingMask=(frame.origin.x>=798?NSViewMinXMargin:0)|NSViewMinYMargin;b.accessibilityIdentifier=ident;b.accessibilityLabel=title;[chrome addSubview:b];return b;};
    // Direct file actions replace the pull-down. Continue/save/home stay as
    // dedicated side buttons; no gameplay function is hidden by the change.
    button(@"新建城市…",@selector(onNativeNew:),@"native-file-new",NativeNSRect(layout.fileNew())).toolTip=@"选择原始 SAV 来源，新建原生 v6 城市。";
    button(@"退出",@selector(terminate:),@"native-file-quit",NativeNSRect(layout.fileQuit())).toolTip=@"正常退出；有未保存进度时会先询问。";
    _nativeResources=label(@"",NativeNSRect(layout.resources()),14);_nativeResources.accessibilityIdentifier=@"native-resources";
    _nativeCompactState=label(@"",NativeNSRect(layout.compactState()),12);_nativeCompactState.accessibilityIdentifier=@"native-session-state";
    NSMutableArray* categories=[NSMutableArray array];NSArray* names=@[@"住宅",@"食物",@"公共服务：水井",@"道路与拆除"];
    const unsigned rows[]={0,1,4,10};
    const unsigned allIDs[]={1323,1327,1331,1335,1339,1343,1347,1351,1355,1359,1319};
    for(unsigned row=0;row<11;++row){int active=-1;for(unsigned i=0;i<4;++i)if(rows[i]==row)active=int(i);
        NativeCategoryButton* b=[NativeCategoryButton buttonWithTitle:@"" target:self action:active>=0?@selector(onNativeCategory:):nullptr];b.frame=NativeNSRect(layout.category(row));b.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;b.image=[self nativeUI:allIDs[row]];b.font=[NSFont systemFontOfSize:12];b.enabled=active>=0;
        if(active>=0){b.tag=active;b.accessibilityIdentifier=[NSString stringWithFormat:@"native-category-%d",active];b.accessibilityLabel=names[active];b.toolTip=names[active];[categories addObject:b];}
        else {b.accessibilityIdentifier=[NSString stringWithFormat:@"native-unavailable-category-%u",row];b.accessibilityLabel=[NSString stringWithFormat:@"原版分类 %u · 尚未实现",row+1];b.toolTip=@"此原版类别尚未实现，不能建造。";b.accessibilityHelp=b.toolTip;}
        [chrome addSubview:b];
    }_nativeCategories=categories;
    _nativeTool=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];[_nativeTool addItemsWithTitles:@[@"浏览 / 取消",@"道路",@"住宅 2×2",@"水井 2×2",@"猎人营地 2×2",@"拆除（预览）"]];_nativeTool.menu.autoenablesItems=NO;[_nativeTool selectItemAtIndex:0];_nativeCategoryIndex=3;
    NSMutableArray* choices=[NSMutableArray array];NSArray* titles=@[@"查看建筑 / 拖动地图",@"道路",@"普通住宅",@"水井",@"猎人营地",@"拆除"];
    const unsigned icons[]={0,0,1491,1551,1512,0};NSArray* shortTitles=@[@"查看",@"道路",@"住宅",@"水井",@"狩猎",@"拆除"];
    for(unsigned i=0;i<6;++i){NativeBuildButton* b=[NativeBuildButton buttonWithTitle:shortTitles[i] target:self action:@selector(onNativePaletteTool:)];b.frame=NativeNSRect(layout.toolFor(i));b.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;b.font=[NSFont systemFontOfSize:11];b.accessibilityIdentifier=[NSString stringWithFormat:@"native-tool-%u",i];b.accessibilityLabel=titles[i];if(icons[i])b.image=[self nativeUI:icons[i]];b.tag=i;[chrome addSubview:b];[choices addObject:b];}_nativeChoices=choices;
    NativeBuildButton* cancel=[NativeBuildButton buttonWithTitle:@"取消" target:self action:@selector(onNativeCancel:)];cancel.frame=NativeNSRect(layout.toolCancel());cancel.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;cancel.font=[NSFont systemFontOfSize:11];cancel.accessibilityIdentifier=@"native-v4-demolish-cancel";cancel.accessibilityLabel=@"取消当前操作";cancel.toolTip=@"取消当前工具、拆除选择或道路预览，回到浏览；不修改城市。";[chrome addSubview:cancel];
    _nativeDetails=label(@"",NativeNSRect(layout.details()),12);_nativeDetails.accessibilityIdentifier=@"native-building-details";
    NSClickGestureRecognizer* houseConditions=[[NSClickGestureRecognizer alloc]initWithTarget:self action:@selector(onNativeHouseConditions:)];
    [_nativeDetails addGestureRecognizer:houseConditions];
    _nativeDetailIcon=[[NSImageView alloc]initWithFrame:NativeNSRect(layout.detailIcon())];_nativeDetailIcon.imageScaling=NSImageScaleProportionallyUpOrDown;_nativeDetailIcon.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;_nativeDetailIcon.hidden=YES;[chrome addSubview:_nativeDetailIcon];
    _nativeServiceSummary=label(@"",NativeNSRect(layout.summary()),12);_nativeServiceSummary.accessibilityIdentifier=@"native-services-summary";
    _nativeHouseConditions=button(@"条件…",@selector(onNativeHouseConditions:),@"native-house-conditions",NativeNSRect(layout.houseConditions()));
    _nativeHouseConditions.toolTip=@"先选择住宅；查看完整原版条件、阈值与未接通原因（只读，不改变城市）。";
    _nativeHouseConditions.accessibilityLabel=@"住宅完整条件";_nativeHouseConditions.enabled=NO;_nativeHouseConditions.accessibilityValue=@"已收起";
    _nativeDetailDemolish=button(@"拆除所选…",@selector(onNativeDetailDemolish:),@"native-detail-demolish",NativeNSRect(layout.detailDemolish()));_nativeDetailDemolish.toolTip=@"先在浏览模式点击自建对象。";_nativeDetailDemolish.enabled=NO;
    _nativeNotice=label(@"",NativeNSRect(layout.notice()),12);_nativeNotice.accessibilityIdentifier=@"native-notice";
    _nativeStatus=[NSTextField wrappingLabelWithString:@""];_nativeStatus.accessibilityIdentifier=@"native-v1-status";
    _nativeFoodInfo=[NSTextField wrappingLabelWithString:@""];_nativeFoodInfo.accessibilityIdentifier=@"native-v3-food-status";
    _nativeWaterInfo=[NSTextField wrappingLabelWithString:@""];_nativeWaterInfo.accessibilityIdentifier=@"native-v2-water-status";
    _nativeTaxInfo=[NSTextField wrappingLabelWithString:@""];_nativeTaxInfo.accessibilityIdentifier=@"native-v5-tax-status";
    _nativeHover=label(@"",NSMakeRect(8,12,782,24),12);_nativeHover.autoresizingMask=NSViewWidthSizable|NSViewMaxYMargin;_nativeHover.accessibilityIdentifier=@"native-v1-hover";
    _nativeRulesLabel=[NSTextField wrappingLabelWithString:@""];
    _nativeX=[[NSTextField alloc]initWithFrame:NSZeroRect];_nativeY=[[NSTextField alloc]initWithFrame:NSZeroRect];
    _nativeCanvas=[[NativeSessionCanvas alloc]initWithFrame:NativeNSRect(layout.map)];_nativeCanvas.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
    _nativeCanvas.accessibilityElement=YES;_nativeCanvas.accessibilityRole=NSAccessibilityImageRole;_nativeCanvas.accessibilityIdentifier=@"native-v1-canvas";
    __weak EmperorAppDelegate* weakSelf=self;
    _nativeCanvas.onGeometryChange=^{[weakSelf nativeCanvasGeometryChanged];};
    _nativeCanvas.onHover=^(double px,double py){EmperorAppDelegate* me=weakSelf;if(!me||!me->_nativeSource)return;const auto tile=game_lab::nativeSessionTile(me->_nativeCamera,me->_nativeSource->seed.side,px,py);if(tile==me->_nativeHoverTile)return;me->_nativeHoverTile=tile;[me updateNativePreview];};
    _nativeCanvas.onPoint=^(double px,double py){EmperorAppDelegate* me=weakSelf;if(!me||!me->_nativeSource)return;const auto p=game_lab::nativeSessionTile(me->_nativeCamera,me->_nativeSource->seed.side,px,py);if(!p)return;if(me->_nativeTool.indexOfSelectedItem==0){me->_nativeInspectionId=game_lab::nativeObjectAt(*me->_nativeState,p->first,p->second).value_or(0);me->_nativeDemolition.reset();[me updateNativeInspection];[me updateNativePreview];return;}if(me->_nativeTool.indexOfSelectedItem==5){me->_nativeDemolition.reset();const auto id=game_lab::nativeObjectAt(*me->_nativeState,p->first,p->second);
        if(id){const auto plan=game_lab::planNativeDemolition(me->_nativeSource->seed,*me->_nativeState,*id);if(plan.candidate){game_lab::NativeSessionCommand command{game_lab::NativeSessionAction::Demolish,me->_nativeState->revision};command.objectId=*id;me->_nativeDemolition=command;}}[me updateNativePreview];return;}
        me->_nativeX.integerValue=p->first;me->_nativeY.integerValue=p->second;[me onNativePlace:nil];};
    _nativeCanvas.onFocusLost=^{EmperorAppDelegate* me=weakSelf;if(me){me->_nativeDemolition.reset();[me updateNativePreview];}};
    _nativeCanvas.onGestureCancel=^{EmperorAppDelegate* me=weakSelf;if(!me)return;me->_nativeRoadGesture.cancel();[me updateNativePreview];};
    _nativeCanvas.onRoadBegin=^(double px,double py){EmperorAppDelegate* me=weakSelf;if(!me||!me->_nativeSource)return;const auto p=game_lab::nativeSessionTile(me->_nativeCamera,me->_nativeSource->seed.side,px,py);me->_nativeRoadGesture.begin(p);if(!p)[me->_nativeCanvas cancelGesture];[me updateNativePreview];};
    _nativeCanvas.onRoadMove=^(double px,double py){EmperorAppDelegate* me=weakSelf;if(!me||!me->_nativeSource)return;const auto p=game_lab::nativeSessionTile(me->_nativeCamera,me->_nativeSource->seed.side,px,py);me->_nativeRoadGesture.move(p);if(!p)[me->_nativeCanvas cancelGesture];[me updateNativePreview];};
    _nativeCanvas.onRoadFinish=^(double px,double py){[weakSelf finishNativeRoadX:px y:py];};
    _nativeCanvas.onCancel=^{[weakSelf onNativeCancel:nil];};
    _nativeCanvas.onPan=^(double dx,double dy){EmperorAppDelegate* me=weakSelf;if(me&&me->_nativeSource)[me nativeViewCamera:game_lab::nativeSessionPan(me->_nativeCamera,me->_nativeSource->seed.side,dx,dy)];};
    _nativeCanvas.onZoom=^(int direction,double px,double py){EmperorAppDelegate* me=weakSelf;if(me&&me->_nativeSource)[me nativeViewCamera:game_lab::nativeSessionZoom(me->_nativeCamera,me->_nativeSource->seed.side,direction,px,py)];};
    [chrome addSubview:_nativeCanvas positioned:NSWindowBelow relativeTo:_nativeHover];
    // Existing native controls remain explicitly labelled; no radar/advisor
    // entry is fabricated for an unimplemented original function.
    _nativePause=button(@"暂停",@selector(onNativePause:),@"native-v1-pause",NativeNSRect(layout.pause()));
    _nativeResume=button(@"恢复",@selector(onNativeResume:),@"native-v1-resume",NativeNSRect(layout.resume()));
    NSMutableArray* speedChoices=[NSMutableArray array];NSArray* speedTitles=@[@"1×",@"2×",@"4×"];const unsigned speedValues[]={1,2,4};
    for(unsigned i=0;i<3;++i){NativeGameButton* b=button(speedTitles[i],@selector(onNativeSpeed:),[NSString stringWithFormat:@"native-speed-%u",speedValues[i]],NativeNSRect(layout.speed(i)));b.tag=i;b.toolTip=@"直接选择计时速度；运行中切换立即生效。";[speedChoices addObject:b];}
    _nativeSpeedChoices=speedChoices;_nativeSpeedIndex=0;[self syncNativeSpeedChoices];
    _nativeWaterToggle=[NSButton checkboxWithTitle:@"供水" target:self action:@selector(onNativeWaterOverlay:)];_nativeWaterToggle.frame=NativeNSRect(layout.waterToggle());_nativeWaterToggle.state=NSControlStateValueOn;_nativeWaterToggle.accessibilityIdentifier=@"native-v2-water-overlay";_nativeWaterToggle.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;[chrome addSubview:_nativeWaterToggle];
    _nativeHuntingToggle=[NSButton checkboxWithTitle:@"狩猎生产" target:self action:@selector(onNativeHunting:)];_nativeHuntingToggle.frame=NativeNSRect(layout.huntingToggle());_nativeHuntingToggle.accessibilityIdentifier=@"native-v3-hunting-enabled";_nativeHuntingToggle.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;[chrome addSubview:_nativeHuntingToggle];
    _nativeDemolitionConfirm=button(@"确认拆除",@selector(onNativeDemolitionConfirm:),@"native-v4-demolish-confirm",NativeNSRect(layout.demolitionConfirm()));
    button(@"−",@selector(onNativeZoomOut:),@"native-zoom-out",NativeNSRect(layout.zoomOut())).accessibilityLabel=@"缩小地图";
    button(@"+",@selector(onNativeZoomIn:),@"native-zoom-in",NativeNSRect(layout.zoomIn())).accessibilityLabel=@"放大地图";
    button(@"存档…",@selector(onNativeSaveAs:),@"native-v1-save",NativeNSRect(layout.save()));
    button(@"继续…",@selector(onNativeContinue:),@"native-v1-load",NativeNSRect(layout.load()));
    button(@"首页",@selector(onNativeClose:),@"native-v1-close",NativeNSRect(layout.home()));
    _nativeMinimap=[[NativeMinimapView alloc]initWithFrame:NativeNSRect(layout.minimap())];_nativeMinimap.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;_nativeMinimap.accessibilityElement=YES;_nativeMinimap.accessibilityRole=NSAccessibilityImageRole;_nativeMinimap.accessibilityIdentifier=@"native-minimap";
    _nativeMinimap.toolTip=@"原生地形概览：蓝为水、深绿为林、灰为岩石、浅色为道路、红为建筑。白框是当前视窗，点击定位；边界就近夹紧。配色不保证原版一致。";
    _nativeMinimap.onLocate=^(double x,double y){EmperorAppDelegate* me=weakSelf;if(!me||!me->_nativeSource)return;[me->_nativeCanvas cancelGesture];me->_nativeRoadGesture.cancel();me->_nativeHoverTile.reset();auto next=me->_nativeCamera;next.centerX=x;next.centerY=y;[me nativeViewCamera:next];};[chrome addSubview:_nativeMinimap];_nativeOverviewSource.reset();
    _nativeFile=[[NSTextField alloc]initWithFrame:NSZeroRect];NSString* folder=[NSBundle.mainBundle.bundlePath.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"NativeSessions"];[NSFileManager.defaultManager createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:nil];_nativeFile.stringValue=[folder stringByAppendingPathComponent:@"未保存.encity"];
    _nativeDemolitionInfo=[NSTextField wrappingLabelWithString:@""];_nativeDemolitionInfo.accessibilityIdentifier=@"native-v4-demolition-info";
    }
    camera.width=unsigned(_nativeCanvas.bounds.size.width);camera.height=unsigned(_nativeCanvas.bounds.size.height);
    __weak EmperorAppDelegate* weakOwner=self;
    void (^complete)(BOOL)=^(BOOL ok){EmperorAppDelegate* me=weakOwner;if(!me)return;
        if(!ok){if(created){[me->_nativePanel orderOut:nil];me->_nativePanel=nil;}else [me updateNativeSessionLabels];[me nativeError:[NSString stringWithUTF8String:me->_nativeMessage.c_str()]];return;}
        [me resetCityPlayback];[me stopNativeSessionTimer];me->_nativeDemolition.reset();me->_nativeHoverTile.reset();me->_nativeRoadGesture.cancel();[me->_nativeCanvas cancelGesture];[me->_nativeTool selectItemAtIndex:0];me->_nativeCategoryIndex=3;
        me->_nativeFile.stringValue=checkpoint?:[[me->_nativeFile.stringValue stringByDeletingLastPathComponent]stringByAppendingPathComponent:[NSString stringWithFormat:@"城市-v6-%@.encity",NSUUID.UUID.UUIDString]];
        me->_nativeSavedContent=game_lab::nativeSessionContent(me->_nativeSource->seed,*me->_nativeState);me->_nativeExitApproved=NO;me->_nativeInspectionId=0;
        me->_nativeX.integerValue=int(me->_nativeCamera.centerX);me->_nativeY.integerValue=int(me->_nativeCamera.centerY);
        me->_nativeMessage=checkpoint?"会话已按原规则载入并暂停；点恢复继续。":"新v6空城已暂停。先放道路和住宅，再接通水井与营地；营地需要住民当工人，缺工时缺粮仍会入住。鼠标拖路，右键取消。";
        [me updateNativeSessionLabels];if(created)[me->_nativePanel center];[me->_nativePanel makeKeyAndOrderFront:nil];
    };
    const auto result=[self beginNativeCandidate:state source:candidateSource camera:camera completion:complete];
    return result;
}
