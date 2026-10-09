#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "controller.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <thread>

@interface MediaSeekSlider : NSSlider {BOOL _mouseTracking;}
@property(nonatomic,readonly) BOOL mouseTracking;
@end
@implementation MediaSeekSlider
- (BOOL)mouseTracking {return _mouseTracking;}
- (void)mouseDown:(NSEvent*)event {_mouseTracking=YES;[super mouseDown:event];_mouseTracking=NO;}
@end

@interface MediaDemoDelegate : NSObject <NSApplicationDelegate> {
    NSWindow* _window;NSImageView* _image;NSTextField* _status;NSTextField* _position;
    NSButton* _play;NSButton* _pause;NSButton* _stop;MediaSeekSlider* _seek;NSTextView* _log;
    NSMutableArray<NSString*>* _lines;NSTimer* _timer;dispatch_queue_t _worker;
    std::unique_ptr<media_demo::Controller> _controller;std::string _moviePath;
    std::unique_ptr<media_demo::DeviceMonitor> _devices;NSString* _testReport;NSFileHandle* _testTrace;
    std::unique_ptr<media_demo::DeviceEvent> _deferredDevice;
    BOOL _testInjectEvents,_testEventsScheduled;NSUInteger _staleDeliveries,_initialDeviceListeners;
    BOOL _testDelay,_testDelayDone,_testMetadataHold;
    std::shared_ptr<stream_lab::OpenControl> _loadControl;
    media_demo::State _state;uint64_t _revision,_ticks,_frameCount;NSUInteger _pending;
    BOOL _tickQueued,_closing,_mediaAvailable,_selecting,_switching;
}
- (void)play:(id)sender;
- (void)pause:(id)sender;
- (void)stop:(id)sender;
- (void)seek:(id)sender;
- (void)request:(media_demo::Action)action frame:(uint64_t)frame;
- (void)requestDevice:(media_demo::DeviceEvent)event;
@end

@implementation MediaDemoDelegate
- (void)appendLog:(NSString*)message {
    if(!message.length)return;
    [_lines addObject:message];if(_lines.count>200)[_lines removeObjectAtIndex:0];
    _log.string=[_lines componentsJoinedByString:@"\n"];
    [_log scrollRangeToVisible:NSMakeRange(_log.string.length,0)];
}
- (NSButton*)button:(NSString*)title action:(SEL)action {
    NSButton* button=[NSButton buttonWithTitle:title target:self action:action];
    button.bezelStyle=NSBezelStyleRounded;return button;
}
- (void)menus {
    NSMenu* menu=[[NSMenu alloc] init];NSMenuItem* appItem=[[NSMenuItem alloc] init];[menu addItem:appItem];
    NSMenu* appMenu=[[NSMenu alloc] initWithTitle:@"媒体演示"];
    [appMenu addItemWithTitle:@"关于静音媒体演示" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"退出静音媒体演示" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu=appMenu;
    NSMenuItem* fileItem=[[NSMenuItem alloc] init];[menu addItem:fileItem];
    NSMenu* fileMenu=[[NSMenu alloc] initWithTitle:@"文件"];
    [fileMenu addItemWithTitle:@"打开文件…" action:@selector(openDocument:) keyEquivalent:@"o"];
    fileItem.submenu=fileMenu;
    NSApp.mainMenu=menu;
}
// File selection is an ordinary user action. It is not evidence of successful
// reads or of the cause of the historical package-external open wait.
- (BOOL)validateMenuItem:(NSMenuItem*)item {
    if(item.action==@selector(openDocument:))return !_closing&&!_selecting&&!_switching&&!_pending;
    return YES;
}
- (void)openDocument:(id)sender {
    (void)sender;if(_closing||_pending||_selecting||_switching)return;
    _selecting=YES;[self updateControls];
    NSOpenPanel* panel=[NSOpenPanel openPanel];
    panel.allowedContentTypes=@[UTTypeQuickTimeMovie];
    panel.canChooseDirectories=NO;panel.allowsMultipleSelection=NO;
    panel.message=@"选择 MOV 文件（格式须匹配 64×48 30fps 48kHz S16LE 双声道）";
    [panel beginSheetModalForWindow:_window completionHandler:^(NSModalResponse result){
        self->_selecting=NO;if(self->_closing)return;
        NSURL* selected=[panel.URL copy];
        if(result!=NSModalResponseOK||!selected){[self updateControls];return;}
        if(self->_loadControl)self->_loadControl->cancelled.store(true,std::memory_order_release);
        const auto revision=++self->_revision;self->_switching=YES;++self->_pending;[self invalidateTimer];
        self->_status.stringValue=@"正在切换文件…";[self updateControls];
        dispatch_async(self->_worker,^{@autoreleasepool{
            NSString* closeError=nil;
            if(self->_controller){try{self->_controller->close();
                if(!self->_controller->snapshot().resourcesZero())throw std::runtime_error("media owners remain after close");
                self->_controller.reset();}catch(const std::exception& error){closeError=[NSString stringWithUTF8String:error.what()];}}
            dispatch_async(dispatch_get_main_queue(),^{
                if(self->_closing||revision!=self->_revision)return;
                self->_switching=NO;if(self->_pending)--self->_pending;
                if(closeError){self->_state=media_demo::State::Failed;self->_status.stringValue=@"发生错误 · 无法释放上一媒体";
                    [self appendLog:closeError];[self updateControls];return;}
                self->_moviePath=selected.path.UTF8String;
                self->_mediaAvailable=YES;self->_state=media_demo::State::Ready;
                self->_tickQueued=NO;self->_ticks=0;self->_frameCount=240;
                self->_image.image=nil;self->_seek.maxValue=239;self->_seek.doubleValue=0;
                self->_position.stringValue=[NSString stringWithFormat:@"0.00 / %.2f 秒",double(self->_frameCount)/30];
                self->_status.stringValue=[NSString stringWithFormat:@"待机 · 已选择文件 · %@",selected.lastPathComponent];
                [self appendLog:@"已选择文件，点击播放后检查媒体。"];
                [self updateControls];
                if(self->_deferredDevice){const auto event=*self->_deferredDevice;self->_deferredDevice.reset();[self requestDevice:event];}
            });
        }});
    }];
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;_state=media_demo::State::Ready;_revision=1;_frameCount=240;
    _worker=dispatch_queue_create("local.media-demo.serial",DISPATCH_QUEUE_SERIAL);_lines=[NSMutableArray array];
    NSURL* asset=[NSBundle.mainBundle URLForResource:@"self_authored_8s" withExtension:@"mov"];
    _mediaAvailable=asset!=nil;if(asset)_moviePath=asset.path.UTF8String;
    NSString* report=NSProcessInfo.processInfo.environment[@"NATIVE_MEDIA_DEMO_TEST_REPORT"];
    NSURL* moduleURL=NSBundle.mainBundle.bundleURL.URLByDeletingLastPathComponent.URLByDeletingLastPathComponent;
    NSString* base=[[moduleURL.path stringByAppendingPathComponent:@"evidence"] stringByAppendingString:@"/"];
    const BOOL testReport=([report.stringByStandardizingPath hasPrefix:[base stringByAppendingString:@"integration-gui/"]]||
        [report.stringByStandardizingPath hasPrefix:[base stringByAppendingString:@"reliability-gui/"]])&&[report.pathExtension isEqualToString:@"json"];
    if(testReport){_testReport=report;NSString* fixture=NSProcessInfo.processInfo.environment[@"NATIVE_MEDIA_DEMO_TEST_FIXTURE"];
        _testMetadataHold=[fixture isEqualToString:@"loading"];
        if(fixture.length&&[@[@"long",@"corrupt",@"truncated"] containsObject:fixture]){
            NSString* name=[fixture isEqualToString:@"long"]?@"self_authored_32s":fixture;
            NSURL* testAsset=[NSBundle.mainBundle URLForResource:name withExtension:@"mov" subdirectory:@"TestMedia"];
            _mediaAvailable=testAsset!=nil;if(testAsset)_moviePath=testAsset.path.UTF8String;
            if([fixture isEqualToString:@"long"])_frameCount=960;}}
    [self menus];
    _window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,860,690)
        styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    _window.title=@"静音媒体演示";_window.contentMinSize=NSMakeSize(660,690);
    NSView* content=_window.contentView;
    NSTextField* title=[NSTextField labelWithString:@"静音媒体演示"];
    title.font=[NSFont systemFontOfSize:25 weight:NSFontWeightSemibold];
    NSTextField* subtitle=[NSTextField labelWithString:[NSString stringWithFormat:@"独立媒体组件 · 自造%.0f秒色条短片 · 非游戏",double(_frameCount)/30]];
    subtitle.textColor=NSColor.secondaryLabelColor;
    NSTextField* sound=[NSTextField labelWithString:@"声音：静音（未接入输出设备）"];
    sound.textColor=NSColor.secondaryLabelColor;
    _image=[[NSImageView alloc] init];_image.imageScaling=NSImageScaleProportionallyUpOrDown;
    _image.wantsLayer=YES;_image.layer.backgroundColor=NSColor.blackColor.CGColor;
    [_image.heightAnchor constraintGreaterThanOrEqualToConstant:230].active=YES;
    [_image setContentHuggingPriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationVertical];
    _play=[self button:@"播放" action:@selector(play:)];_pause=[self button:@"暂停" action:@selector(pause:)];
    _stop=[self button:@"停止" action:@selector(stop:)];
    _position=[NSTextField labelWithString:[NSString stringWithFormat:@"0.00 / %.2f 秒",double(_frameCount)/30]];
    _position.font=[NSFont monospacedDigitSystemFontOfSize:13 weight:NSFontWeightRegular];
    NSStackView* controls=[NSStackView stackViewWithViews:@[_play,_pause,_stop,_position]];
    controls.orientation=NSUserInterfaceLayoutOrientationHorizontal;controls.spacing=12;
    _seek=[[MediaSeekSlider alloc] initWithFrame:NSZeroRect];_seek.minValue=0;_seek.maxValue=double(_frameCount-1);
    _seek.doubleValue=0;_seek.target=self;_seek.action=@selector(seek:);
    _seek.continuous=NO;_seek.toolTip=@"跳转到所选帧；待机或暂停时保持暂停。";
    _status=[NSTextField labelWithString:@"待机 · 未打开媒体 · 点击播放开始"];
    _status.font=[NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];
    _status.lineBreakMode=NSLineBreakByTruncatingTail;
    NSTextField* logTitle=[NSTextField labelWithString:@"状态日志"];
    NSScrollView* scroll=[[NSScrollView alloc] init];scroll.hasVerticalScroller=YES;scroll.borderType=NSBezelBorder;
    _log=[[NSTextView alloc] initWithFrame:NSZeroRect];_log.editable=NO;_log.selectable=YES;
    _log.font=[NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    _log.verticallyResizable=YES;_log.horizontallyResizable=NO;
    _log.minSize=NSMakeSize(0,130);_log.maxSize=NSMakeSize(CGFLOAT_MAX,CGFLOAT_MAX);
    _log.autoresizingMask=NSViewWidthSizable;_log.textContainer.widthTracksTextView=YES;
    _log.textContainer.containerSize=NSMakeSize(CGFLOAT_MAX,CGFLOAT_MAX);
    scroll.documentView=_log;[scroll.heightAnchor constraintEqualToConstant:130].active=YES;
    NSStackView* layout=[NSStackView stackViewWithViews:@[title,subtitle,sound,_image,controls,_seek,_status,logTitle,scroll]];
    layout.orientation=NSUserInterfaceLayoutOrientationVertical;layout.alignment=NSLayoutAttributeLeading;layout.spacing=10;
    layout.translatesAutoresizingMaskIntoConstraints=NO;[content addSubview:layout];
    [NSLayoutConstraint activateConstraints:@[
        [layout.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:20],
        [layout.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-20],
        [layout.topAnchor constraintEqualToAnchor:content.topAnchor constant:20],
        [layout.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-20],
        [_image.widthAnchor constraintEqualToAnchor:layout.widthAnchor],[_seek.widthAnchor constraintEqualToAnchor:layout.widthAnchor],
        [_status.widthAnchor constraintEqualToAnchor:layout.widthAnchor],[scroll.widthAnchor constraintEqualToAnchor:layout.widthAnchor]
    ]];
    [self appendLog:@"待机：默认静音；尚未打开解码器或创建GPU后端。"];
    if(!_mediaAvailable)[self appendLog:@"错误：应用包缺少自造短片，请重新构建。"];
    [self updateControls];[_window center];[_window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
    // No timer, play, decoder, GPU backend, or audio sink is started here.
    __weak MediaDemoDelegate* weakOwner=self;
    _devices=std::make_unique<media_demo::DeviceMonitor>([weakOwner](media_demo::DeviceEvent event){
        dispatch_async(dispatch_get_main_queue(),^{MediaDemoDelegate* owner=weakOwner;if(owner&&!owner->_closing)[owner requestDevice:event];});
    });
    _initialDeviceListeners=_devices->stats().activeListeners;
    if(_testReport){
        _testDelay=[NSProcessInfo.processInfo.environment[@"NATIVE_MEDIA_DEMO_TEST_DELAY"] isEqualToString:@"1"];
        _testInjectEvents=[NSProcessInfo.processInfo.environment[@"NATIVE_MEDIA_DEMO_TEST_EVENTS"] isEqualToString:@"1"];
        NSString* trace=[[report stringByDeletingPathExtension] stringByAppendingString:@".states.jsonl"];
        [NSFileManager.defaultManager createFileAtPath:trace contents:nil attributes:nil];_testTrace=[NSFileHandle fileHandleForWritingAtPath:trace];
        // Private regression launch only: hard stop the owned test instance before 60 seconds.
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,45*NSEC_PER_SEC),dispatch_get_main_queue(),^{if(!self->_closing)[NSApp terminate:nil];});
    }
}
- (void)updateControls {
    const BOOL enabled=!_closing&&!_selecting&&!_switching&&_pending==0&&_mediaAvailable;
    _play.enabled=enabled&&_state!=media_demo::State::Playing;
    _pause.enabled=enabled&&_state==media_demo::State::Playing;
    _stop.enabled=!_closing&&!_selecting&&!_switching&&_mediaAvailable&&(_pending>0||_state!=media_demo::State::Ready);_seek.enabled=enabled;
}
- (void)invalidateTimer {[_timer invalidate];_timer=nil;}
- (void)updateTimer {
    if(_closing||_switching||_state!=media_demo::State::Playing){[self invalidateTimer];return;}
    if(_timer)return;
    __weak MediaDemoDelegate* weakSelf=self;
    _timer=[NSTimer timerWithTimeInterval:1.0/30.0 repeats:YES block:^(NSTimer* timer){
        (void)timer;MediaDemoDelegate* owner=weakSelf;
        if(owner&&!owner->_closing&&!owner->_tickQueued&&!owner->_pending)[owner request:media_demo::Action::Tick frame:0];
    }];
    [NSRunLoop.mainRunLoop addTimer:_timer forMode:NSRunLoopCommonModes];
}
- (void)displayFrame:(const std::vector<uint8_t>&)bytes {
    if(bytes.empty())return;if(bytes.size()!=133*24){[self appendLog:@"显示错误：帧字节数不匹配。"];
        return;}
    NSBitmapImageRep* bitmap=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr pixelsWide:32 pixelsHigh:24
        bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace
        bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:128 bitsPerPixel:32];
    if(!bitmap){[self appendLog:@"显示错误：无法创建位图。"];
        return;}
    // NSBitmapImageRep's endianness flags describe sample storage; do not use them
    // as a BGRA pixel swizzle. Pack explicit non-premultiplied RGBA8 instead.
    for(size_t y=0;y<24;++y)for(size_t x=0;x<32;++x){const auto* src=bytes.data()+y*133+x*4;
        auto* dst=bitmap.bitmapData+y*size_t(bitmap.bytesPerRow)+x*4;
        dst[0]=src[2];dst[1]=src[1];dst[2]=src[0];dst[3]=src[3];}
    NSImage* image=[[NSImage alloc] initWithSize:NSMakeSize(32,24)];[image addRepresentation:bitmap];_image.image=image;
}
- (void)deliver:(std::shared_ptr<media_demo::Snapshot>)result revision:(uint64_t)revision tick:(BOOL)tick {
    if(tick)_tickQueued=NO;else if(_pending)--_pending;
    if(_closing)return;
    // User commands invalidate already queued UI deliveries before the worker handles seek/stop.
    if(revision!=_revision){++_staleDeliveries;[self updateControls];return;}
    _state=result->state;if(result->clearImage)_image.image=nil;[self displayFrame:result->bgra];
    if(!_seek.mouseTracking)_seek.doubleValue=double(result->frame);
    const double seconds=double(result->mediaPCM)/48000.0;
    _position.stringValue=[NSString stringWithFormat:@"%.2f / %.2f 秒",seconds,double(_frameCount)/30];
    _status.stringValue=[NSString stringWithFormat:@"%@ · 帧 %llu/%llu · 代际 %llu · 帧队列 %lu · PCM来源 %lu · GPU票据 %lu",
        [NSString stringWithUTF8String:media_demo::stateName(_state)],static_cast<unsigned long long>(result->frame),static_cast<unsigned long long>(_frameCount-1),static_cast<unsigned long long>(result->epoch),
        static_cast<unsigned long>(result->queuedVideo),static_cast<unsigned long>(result->pcmSources),static_cast<unsigned long>(result->gpuTickets)];
    [self appendLog:[NSString stringWithUTF8String:result->message.c_str()]];
    if(_testTrace){NSDictionary* row=@{@"state":[NSString stringWithUTF8String:media_demo::stateName(result->state)],@"frame":@(result->frame),@"media_pcm":@(result->mediaPCM),
        @"epoch":@(result->epoch),@"host_ns":@(result->clock.observedNS),@"UI_ns":@(media_demo::HostClock::nowNS()),@"resources_zero":@(result->resourcesZero()),
        @"device_blocked":@(result->deviceBlocked),@"tick":@(tick),@"message":[NSString stringWithUTF8String:result->message.c_str()]};
        NSMutableData* line=[[NSJSONSerialization dataWithJSONObject:row options:NSJSONWritingSortedKeys error:nil] mutableCopy];[line appendBytes:"\n" length:1];[_testTrace writeData:line];}
    if(tick&&++_ticks%30==0)[self appendLog:_status.stringValue];
    [self updateControls];[self updateTimer];
}
- (void)request:(media_demo::Action)action frame:(uint64_t)frame {
    if(_closing||_switching||!_mediaAvailable)return;const BOOL tick=action==media_demo::Action::Tick;
    if(_selecting&&!tick)return;
    if(action==media_demo::Action::Stop&&_loadControl)_loadControl->cancelled.store(true,std::memory_order_release);
    if(action==media_demo::Action::Play&&_testInjectEvents&&!_testEventsScheduled){_testEventsScheduled=YES;
        const auto original=_devices->stats().current;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            if(!self->_closing)self->_devices->inject({0,0,false,0,media_demo::DeviceOrigin::Injected});});
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2500*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
            if(!self->_closing)self->_devices->inject(original);});
    }
    if(tick){if(_tickQueued||_pending)return;_tickQueued=YES;}
    else{++_revision;++_pending;[self invalidateTimer];_status.stringValue=@"正在处理…";[self updateControls];}
    const auto control=std::make_shared<stream_lab::OpenControl>();control->holdMetadataCompletionForTest=_testMetadataHold;_loadControl=control;
    const uint64_t revision=_revision;
    const auto moviePath=_moviePath;const auto frameCount=_frameCount;
    dispatch_async(_worker,^{@autoreleasepool{
        std::shared_ptr<media_demo::Snapshot> output;
        try{if(!self->_controller)self->_controller=std::make_unique<media_demo::Controller>(moviePath,media_demo::Controller::PCMObserver{},frameCount);
            if(action==media_demo::Action::Tick&&self->_testDelay&&!self->_testDelayDone&&self->_controller->snapshot().mediaPCM>=576000){
                self->_testDelayDone=YES;std::this_thread::sleep_for(std::chrono::seconds(2));}
            output=std::make_shared<media_demo::Snapshot>(self->_controller->perform(action,frame,control));}
        catch(const std::exception& error){output=std::make_shared<media_demo::Snapshot>();output->state=media_demo::State::Failed;
            output->clearImage=true;output->message=std::string("错误：")+error.what();}
        dispatch_async(dispatch_get_main_queue(),^{[self deliver:output revision:revision tick:tick];});
    }});
}
- (void)play:(id)sender {(void)sender;[self request:media_demo::Action::Play frame:0];}
- (void)pause:(id)sender {(void)sender;[self request:media_demo::Action::Pause frame:0];}
- (void)stop:(id)sender {(void)sender;[self request:media_demo::Action::Stop frame:0];}
- (void)seek:(id)sender {(void)sender;const auto frame=uint64_t(std::clamp(std::llround(_seek.doubleValue),0LL,static_cast<long long>(_frameCount-1)));
    [self request:media_demo::Action::Seek frame:frame];}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)application {(void)application;return YES;}
- (void)requestDevice:(media_demo::DeviceEvent)event {
    if(_closing||!_mediaAvailable)return;
    if(_switching){_deferredDevice=std::make_unique<media_demo::DeviceEvent>(event);return;}
    ++_revision;++_pending;[self invalidateTimer];[self updateControls];const auto revision=_revision;
    if(_loadControl)_loadControl->cancelled.store(true,std::memory_order_release);
    const auto control=std::make_shared<stream_lab::OpenControl>();_loadControl=control;
    const auto moviePath=_moviePath;const auto frameCount=_frameCount;
    dispatch_async(_worker,^{@autoreleasepool{
        if(!self->_controller)self->_controller=std::make_unique<media_demo::Controller>(moviePath,media_demo::Controller::PCMObserver{},frameCount);
        auto output=std::make_shared<media_demo::Snapshot>(self->_controller->deviceChanged(event,control));
        dispatch_async(dispatch_get_main_queue(),^{[self deliver:output revision:revision tick:NO];});
    }});
}
- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;_closing=YES;++_revision;[self invalidateTimer];
    if(_loadControl)_loadControl->cancelled.store(true,std::memory_order_release);
    if(_devices)_devices->close();const auto devices=_devices?_devices->stats():media_demo::DeviceMonitorStats{};
    __block media_demo::Snapshot final;__block NSString* closeError=@"";
    if(_worker)dispatch_sync(_worker,^{@autoreleasepool{if(self->_controller){try{self->_controller->close();}
        catch(const std::exception& error){closeError=[NSString stringWithUTF8String:error.what()];}
        final=self->_controller->snapshot();self->_controller.reset();}}});
    if(_testReport){NSDictionary* report=@{@"passed":@(closeError.length==0&&final.resourcesZero()&&devices.activeListeners==0&&devices.removeStatus==0),
        @"pid":@(NSProcessInfo.processInfo.processIdentifier),@"resources_zero":@(final.resourcesZero()),@"close_error":closeError,
        @"reader_count":@(final.readers),@"video_bytes":@(final.videoBytes),@"PCM_bytes":@(final.pcmBytes),@"GPU_tickets":@(final.gpuTickets),
        @"device_listeners":@(devices.activeListeners),@"initial_device_listeners":@(_initialDeviceListeners),@"native_device_events":@(devices.nativeEvents),@"injected_device_events":@(devices.injectedEvents),@"listener_remove_status":@(devices.removeStatus),
        @"device_recoveries":@(final.stats.deviceRecoveries),@"stale_UI_deliveries":@(_staleDeliveries),
        @"runtime_opens":@(final.stats.runtimeOpens),@"runtime_releases":@(final.stats.runtimeReleases),@"real_sound":@NO,
        @"metadata_timeouts":@(final.stats.metadataTimeouts),@"metadata_cancellations":@(final.stats.metadataCancellations),
        @"controlled_worker_delay_ms":@(self->_testDelayDone?2000:0),@"max_catchup_frames":@(final.stats.maxCatchupFrames),
        @"max_video_bytes":@(final.stats.maxVideoBytes),@"max_PCM_bytes":@(final.stats.maxPCMBytes),@"max_PCM_sources":@(final.stats.maxPCMSources),
        @"max_readers":@(final.stats.maxReaders),@"max_GPU_buffer_bytes":@(final.stats.maxGPUBytes)};
        NSData* data=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
        [data writeToFile:_testReport atomically:YES];}
    [_testTrace closeFile];_testTrace=nil;
    _devices.reset();
}
@end

int main(int argc,const char* argv[]){(void)argc;(void)argv;@autoreleasepool{
    NSApplication* application=NSApplication.sharedApplication;[application setActivationPolicy:NSApplicationActivationPolicyRegular];
    __attribute__((objc_precise_lifetime)) MediaDemoDelegate* delegate=[[MediaDemoDelegate alloc] init];
    application.delegate=delegate;[application run];return 0;
}}
