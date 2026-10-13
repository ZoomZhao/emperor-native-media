#import <AppKit/AppKit.h>
#include "movie_session.hpp"
#include "cg_image_conversion.hpp"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>

@interface BinkTestView:NSView {
    CGImageRef _frame;uint64_t _version,_drawnVersion;
}
@property(nonatomic) uint64_t uniqueDraws;
@property(nonatomic) uint64_t visibleDraws;
- (void)setFrameImage:(CGImageRef)image;
@end
@implementation BinkTestView
- (void)dealloc {if(_frame)CGImageRelease(_frame);}
- (void)setFrameImage:(CGImageRef)image {
    if(image)CGImageRetain(image);if(_frame)CGImageRelease(_frame);_frame=image;
    if(image)++_version;self.needsDisplay=YES;
}
- (void)drawRect:(NSRect)dirty {
    (void)dirty;[[NSColor blackColor] setFill];NSRectFill(self.bounds);
    if(!_frame)return;
    const double scale=std::min(self.bounds.size.width/CGImageGetWidth(_frame),self.bounds.size.height/CGImageGetHeight(_frame));
    const NSRect rect=NSMakeRect((self.bounds.size.width-CGImageGetWidth(_frame)*scale)/2,
        (self.bounds.size.height-CGImageGetHeight(_frame)*scale)/2,CGImageGetWidth(_frame)*scale,CGImageGetHeight(_frame)*scale);
    CGContextDrawImage(NSGraphicsContext.currentContext.CGContext,NSRectToCGRect(rect),_frame);
    if(_drawnVersion!=_version){_drawnVersion=_version;++self.uniqueDraws;
        if(self.window.isVisible&&(self.window.occlusionState&NSWindowOcclusionStateVisible))++self.visibleDraws;}
}
@end

namespace {
double now(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
struct WindowPresenter:game_lab::ImagePresenter {
    BinkTestView* __strong view;
    explicit WindowPresenter(BinkTestView* value):view(value){}
    void clear() override {[view setFrameImage:nullptr];}
    bool present(uint32_t width,uint32_t height,const std::vector<uint32_t>& pixels) override {
        CGImageRef image=game_lab::createCGImageFromPremultipliedBGRA(width,height,pixels);
        if(!image)return false;[view setFrameImage:image];CGImageRelease(image);return true;
    }
};
struct Run {
    NSWindow* __strong window;BinkTestView* __strong view;
    NSTimer* __strong timer;NSTimer* __strong watchdog;
    std::unique_ptr<game_lab::MovieSession> session;std::unique_ptr<WindowPresenter> presenter;
    media_demo::Snapshot latest;bool pending=true,finishing=false,ended=false,closed=false,smoke=false,smokeCompleted=false;
    double firstDelivery=0,elapsed=0;int progressBucket=-1;std::string failure;
};
void awaitSafeClose(std::shared_ptr<Run> state){
    state->session->requestTermination(2000,[state](const game_lab::TerminationResult& result){
        state->closed=result.permitsExit();
        if(!state->closed){
            if(state->failure.empty())state->failure="native session cleanup deadline expired; still waiting";
            std::cerr<<"BLOCKED: waiting for normal child/session cleanup; no force termination\n"<<std::flush;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,500*NSEC_PER_MSEC),dispatch_get_main_queue(),^{awaitSafeClose(state);});
            return;
        }
        [state->window orderOut:nil];[NSApp stop:nil];
        [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint
            modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
    });
}
void finish(std::shared_ptr<Run> state,const std::string& failure){
    if(state->finishing)return;state->finishing=true;state->failure=failure;
    state->elapsed=state->firstDelivery?now()-state->firstDelivery:0;
    [state->timer invalidate];state->timer=nil;[state->watchdog invalidate];state->watchdog=nil;
    // Let the last real frame receive a drawRect callback before closing.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
        awaitSafeClose(state);
    });
}
void delivery(const std::shared_ptr<Run>& state,const game_lab::MovieDelivery& value){
    state->pending=false;
    if(!value.accepted)return;
    state->latest=value.snapshot;std::vector<uint8_t>().swap(state->latest.bgra);
    if(value.snapshot.state==media_demo::State::Failed){finish(state,value.snapshot.message);return;}
    if(value.snapshot.state==media_demo::State::Playing&&!state->firstDelivery){
        if(!value.presented){finish(state,"original first frame was not presented");return;}
        state->firstDelivery=now();
        std::cout<<"native_window_open=1/1 width="<<value.width<<" height="<<value.height
                 <<" declared_frames="<<value.snapshot.videoFrames<<"\n"<<std::flush;
    }
    const int bucket=int(value.snapshot.positionSeconds/30);
    if(bucket!=state->progressBucket){state->progressBucket=bucket;
        std::cout<<"playback_seconds="<<value.snapshot.positionSeconds
                 <<" decoded_frames="<<value.snapshot.stats.decodedVideoFrames
                 <<" window_draws="<<state->view.uniqueDraws<<"\n"<<std::flush;}
    if(value.snapshot.state==media_demo::State::Ended){state->ended=true;finish(state,"");}
}
}
int main(int argc,char** argv){@autoreleasepool{
    const char* supplied=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!supplied||!*supplied){std::cout<<"SKIP: EMPEROR_GAMEDATA_ROOT is unset\n";return 0;}
    if(argc!=2&&(argc!=3||std::string(argv[2])!="--smoke")){std::cerr<<"usage: bink-window-test RELATIVE_BIK_PATH [--smoke]\n";return 2;}
    try{
        auto root=game_lab::ResourceRoot::fromAbsolute(supplied);
        if(root.resolve(argv[1]).empty())throw std::runtime_error("video is outside supplied data root");
        [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        auto state=std::make_shared<Run>();state->smoke=argc==3;
        state->window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,640,480)
            styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered defer:NO];
        state->window.title=@"Emperor original Bink · silent preview";
        state->window.level=NSFloatingWindowLevel;
        state->window.collectionBehavior=NSWindowCollectionBehaviorCanJoinAllSpaces|NSWindowCollectionBehaviorFullScreenAuxiliary;
        state->view=[[BinkTestView alloc] initWithFrame:NSMakeRect(0,0,640,480)];
        state->view.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
        state->window.contentView=state->view;
        [state->window center];[state->window makeKeyAndOrderFront:nil];[state->window orderFrontRegardless];
        dispatch_async(dispatch_get_main_queue(),^{[NSApp activate];[state->window orderFrontRegardless];});
        if(!state->window.screen||!state->window.isVisible)throw std::runtime_error("native window is unavailable");
        state->presenter=std::make_unique<WindowPresenter>(state->view);
        state->session=std::make_unique<game_lab::MovieSession>(root);
        state->session->openAndPlay(argv[1],[state](const game_lab::MovieDelivery& d){delivery(state,d);},state->presenter.get());
        state->timer=[NSTimer timerWithTimeInterval:1.0/120 repeats:YES block:^(NSTimer* timer){
            (void)timer;if(state->finishing||state->pending)return;
            if(!state->window.isVisible){finish(state,"native playback window was closed");return;}
            state->pending=true;
            state->session->request(media_demo::Action::Tick,0,[state](const game_lab::MovieDelivery& d){delivery(state,d);},state->presenter.get());
        }];
        [NSRunLoop.mainRunLoop addTimer:state->timer forMode:NSRunLoopCommonModes];
        state->watchdog=[NSTimer scheduledTimerWithTimeInterval:state->smoke?2:185 repeats:NO block:^(NSTimer* timer){
            (void)timer;if(state->smoke){state->smokeCompleted=true;finish(state,"");}
            else finish(state,"bounded playback watchdog expired");
        }];
        [NSApp run];
        const auto& s=state->latest;
        const bool accepted=state->closed&&state->failure.empty()&&state->view.visibleDraws>1&&
            (state->smoke?state->smokeCompleted&&state->elapsed>1.5:
             state->ended&&s.stats.decodedVideoFrames==s.videoFrames&&std::abs(state->elapsed-s.durationSeconds)<2);
        const auto stats=state->session->stats();
        std::cout<<"{\"native_playback_files\":"<<(state->smoke?"null":accepted?"1":"0")<<",\"files\":1,\"decoded_frames\":"<<s.stats.decodedVideoFrames
                 <<",\"header_frames\":"<<s.videoFrames<<",\"delivered_frames\":"<<s.stats.renders
                 <<",\"window_unique_draws\":"<<state->view.uniqueDraws<<",\"window_visible_draws\":"<<state->view.visibleDraws
                 <<",\"elapsed_seconds\":"<<state->elapsed<<",\"expected_seconds\":"<<s.durationSeconds
                 <<",\"window_draw_fps\":"<<(state->elapsed?state->view.uniqueDraws/state->elapsed:0)
                 <<",\"declared_fps\":"<<double(s.fpsNumerator)/s.fpsDenominator
                 <<",\"discarded_due_frames\":"<<s.stats.discardedVideo
                 <<",\"app_active\":"<<int(NSApp.isActive)<<",\"window_occlusion_state\":"<<state->window.occlusionState
                 <<",\"max_frame_bytes\":"<<s.stats.maxVideoBytes<<",\"max_active\":"<<stats->maxActive
                 <<",\"max_pending\":"<<stats->maxPending<<",\"max_delivery\":"<<stats->maxDelivery
                 <<",\"resources_zero\":"<<int(state->closed)<<",\"silent_video_only\":true,\"bounded_smoke\":"<<int(state->smoke)<<"}\n"<<std::flush;
        if(!state->failure.empty())std::cerr<<"BLOCKED: "<<state->failure<<"\n";
        return accepted?0:1;
    }catch(const std::exception& failure){std::cerr<<"BLOCKED: "<<failure.what()<<"\n";return 2;}
}}
