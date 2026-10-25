// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#include "native_metal_display.hpp"
#include "../native_session_interaction.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "native_session_canvas.hpp"
// This drawable does NOT present a window. Its real Metal texture verifies the
// bridge's copy and callbacks independently of later real CAMetalLayer acceptance.
@interface TestDrawable : NSObject<CAMetalDrawable>
@property(nonatomic,strong) id<MTLTexture> texture;
@property(nonatomic) NSUInteger presents;
@end
@implementation TestDrawable
- (CAMetalLayer*)layer{return nil;}
- (void)present{++_presents;}
- (void)presentAtTime:(CFTimeInterval)t{(void)t;[self present];}
- (void)presentAfterMinimumDuration:(CFTimeInterval)t{(void)t;[self present];}
- (void)addPresentedHandler:(MTLDrawablePresentedHandler)handler{(void)handler;}
- (CFTimeInterval)presentedTime{return 0;}
- (NSUInteger)drawableID{return 1;}
@end
@interface TestDisplay : NativeMetalDisplay
@property(nonatomic,strong) id<CAMetalDrawable> supplied;
@end
@implementation TestDisplay
- (id<CAMetalDrawable>)acquireDrawable{return self.supplied;}
@end
namespace {
size_t checks=0,canvasChecks=0;
void need(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
void waitFor(BOOL (^done)(void)){
    const auto deadline=[NSDate dateWithTimeIntervalSinceNow:5];
    while(!done()&&deadline.timeIntervalSinceNow>0)[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.005]];
    need(done(),"accepted request callback deadline");
}
id<MTLTexture> texture(id<MTLDevice> device,unsigned w=4,unsigned h=3,MTLPixelFormat format=MTLPixelFormatBGRA8Unorm){
    auto desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:w height:h mipmapped:NO];
    desc.storageMode=MTLStorageModeShared;desc.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
    return [device newTextureWithDescriptor:desc];
}
void canvas(){
    const auto start=checks;
    NativeSessionCanvas* view=[[NativeSessionCanvas alloc]initWithFrame:NSMakeRect(0,0,816,620)];
    need(view.logicalImageSize.width==0,"empty canvas has no implied texture size");
    need(![view imagePoint:NSMakePoint(1,1)],"empty canvas cannot place");
    view.image=[[NSImage alloc]initWithSize:NSMakeSize(816,620)];
    auto fit=[view imageFit];auto point=[view imagePoint:NSMakePoint(408,310)];
    need(fit.width==816&&fit.height==620&&point&&point->x==408&&point->y==310,"CPU image populates logical pixels");
    view.image=nil;view.logicalImageSize=NSMakeSize(816,620);
    point=[view imagePoint:NSMakePoint(408,310)];
    need(!view.image&&point&&point->x==408&&point->y==310,"future texture input does not need a CPU image");
    __block unsigned geometry=0;view.onGeometryChange=^{++geometry;};
    for(auto size:std::vector<NSSize>{NSMakeSize(1632,1240),NSMakeSize(1200,620),NSMakeSize(400,800),NSMakeSize(817,621)}){
        const unsigned before=geometry;[view setFrameSize:size];fit=[view imageFit];
        need(geometry>before,"layout invalidates camera sequence");
        point=[view imagePoint:NSMakePoint(fit.x+fit.width/2,fit.y+fit.height/2)];
        need(point&&std::abs(point->x-408)<1e-8&&std::abs(point->y-310)<1e-8,"letterbox fit maps logical center at multiple scales");
        need(![view imagePoint:NSMakePoint(fit.x-1,fit.y-1)],"letterbox misses do not build");
    }
    const unsigned before=geometry;[view viewDidChangeBackingProperties];need(geometry>before,"backing scale invalidates pending view");
    for(auto bad:std::vector<NSSize>{NSMakeSize(0,4),NSMakeSize(4,0),NSMakeSize(NAN,4),NSMakeSize(4096,1)}){
        view.logicalImageSize=bad;need(![view imagePoint:NSMakePoint(1,1)],"invalid logical texture size never picks a tile");
    }
    view.onGeometryChange=nil;view.image=nil;canvasChecks=checks-start;
}
void display(id<MTLDevice> device){
    __block unsigned callbacks=0,admissions=0;
    __block game_lab::SceneFrameGateState gate=game_lab::changeSceneFrame({},game_lab::SceneFrameChange::BeginSession).state;
    auto issue=^game_lab::SceneFrameTicket(void){auto r=game_lab::requestSceneFrame(gate,1,5,4,3);gate=r.state;return *gate.pending;};
    NativeMetalDisplayAdmission admit=^BOOL(const game_lab::SceneFrameTicket& t){
        need(NSThread.isMainThread,"admission on main");++admissions;
        const auto r=game_lab::consumeSceneFrame(gate,t,4,3);gate=r.state;return r.status==game_lab::SceneFrameStatus::Current;
    };
    __block std::vector<NativeMetalDisplayResult> results;
    __block std::vector<std::string> errors;
    NativeMetalDisplayCompletion complete=^(const game_lab::SceneFrameTicket& t,NativeMetalDisplayResult r,NSString* error){
        need(NSThread.isMainThread,"completion on main");need(t.width==4&&t.height==3,"ticket echoed unchanged");
        ++callbacks;results.push_back(r);errors.push_back(error.UTF8String?:"");
    };
    TestDisplay* bridge=[[TestDisplay alloc]initWithDevice:device];
    need(bridge.layer.pixelFormat==MTLPixelFormatBGRA8Unorm&&!bridge.layer.framebufferOnly&&bridge.layer.maximumDrawableCount==3,"blit-compatible bounded layer");
    CGColorSpaceRef srgb=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);need(CFEqual(bridge.layer.colorspace,srgb),"explicit sRGB agrees with native CPU before/after layer backing");CGColorSpaceRelease(srgb);
    need(bridge.layer.allowsNextDrawableTimeout&&bridge.layer.hidden,"bounded acquisition; not implicitly shown");
    need(![bridge configureFrame:CGRectMake(0,0,1,1) width:0 height:3],"invalid logical dimensions refused");
    need(![bridge configureFrame:CGRectMake(0,0,NAN,1) width:4 height:3],"invalid layout refused");
    need([bridge configureFrame:CGRectMake(2,3,400,300) width:4 height:3],"logical draw size distinct from view points");
    need(bridge.layer.drawableSize.width==4&&bridge.layer.frame.size.width==400,"layer uses explicit pixel/point sizes");
    auto t=issue();auto source=texture(device);need(source!=nil,"authored Metal texture allocated");
    const uint32_t pixels[]={0xff000001,0xff020304,0xff050607,0xff08090a,0xff0b0c0d,0xff0e0f10,0xff111213,0xff141516,0xff171819,0xff1a1b1c,0xff1d1e1f,0xff202122};
    [source replaceRegion:MTLRegionMake2D(0,0,4,3) mipmapLevel:0 withBytes:pixels bytesPerRow:16];
    need([bridge submitTexture:nil ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Invalid,"nil texture rejected before enqueue");
    need([bridge submitTexture:texture(device,4,3,MTLPixelFormatBGRA8Unorm_sRGB) ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Invalid,"sRGB texture cannot silently replace unorm");
    need([bridge submitTexture:texture(device,5,3) ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Invalid,"texture dimensions checked");
    need([bridge submitTexture:source ticket:t admission:nil completion:complete]==NativeMetalDisplaySubmit::Invalid,"no admission callback rejected");
    need(callbacks==0&&bridge.inFlight==0,"nonaccepted submissions never callback or consume slots");
    // Nil drawable => current-ticket CPU fallback remains possible.
    need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Accepted,"nil-drawable case accepted asynchronously");
    waitFor(^BOOL(void){return callbacks==1;});
    need(results.back()==NativeMetalDisplayResult::Failed&&errors.back()=="display_drawable_unavailable"&&admissions==0,"drawable failure cannot publish city");
    need(game_lab::mayFallbackSceneFrame(gate,t),"current failed display may use CPU fallback");
    auto cpu=game_lab::consumeSceneFrame(gate,t,4,3);gate=cpu.state;
    need(cpu.status==game_lab::SceneFrameStatus::Current&&!gate.pending,"CPU fallback consumes one valid ticket");
    TestDrawable* drawable=[TestDrawable new];drawable.texture=texture(device);bridge.supplied=drawable;
    t=issue();need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Accepted,"texture copy request accepted");
    source=nil; // The bridge/command buffer must retain its source beyond submit.
    waitFor(^BOOL(void){return callbacks==2;});
    need(results.back()==NativeMetalDisplayResult::Scheduled&&drawable.presents==1&&!gate.pending,"successful copy admits/schedules once");
    uint32_t output[12]={};[drawable.texture getBytes:output bytesPerRow:16 fromRegion:MTLRegionMake2D(0,0,4,3) mipmapLevel:0];
    for(unsigned i=0;i<12;++i)need(output[i]==pixels[i],"display blit preserves exact BGRA bytes");
    source=texture(device);t=issue();
    need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Accepted,"first pending slot accepted");
    need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Accepted,"second pending slot accepted");
    need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Busy&&bridge.inFlight==2,"third slot busy without callback");
    waitFor(^BOOL(void){return callbacks==4;});
    need(drawable.presents==2&&bridge.inFlight==0,"duplicate callback cannot publish twice via frame gate");
    need(std::count(results.begin()+2,results.end(),NativeMetalDisplayResult::Stale)==1,"one duplicate refused at final admission");
    t=issue();const auto oldAdmits=admissions;
    need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Accepted,"geometry test accepted");
    [bridge configureFrame:CGRectMake(0,0,800,600) width:4 height:3];
    waitFor(^BOOL(void){return callbacks==5;});
    need(results.back()==NativeMetalDisplayResult::Stale&&admissions==oldAdmits&&drawable.presents==2,"resized view cannot publish acquired old drawable");
    t=issue();need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Accepted,"close test accepted");
    [bridge shutdown];need(bridge.closed&&bridge.layer.hidden,"shutdown hides layer immediately");
    waitFor(^BOOL(void){return callbacks==6;});
    need(results.back()==NativeMetalDisplayResult::Cancelled&&admissions==oldAdmits&&drawable.presents==2,"accepted shutdown callback once without publication");
    need([bridge submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Closed,"shutdown rejects new work");
    need(![bridge configureFrame:CGRectMake(0,0,4,3) width:4 height:3],"closed bridge cannot reopen");
    TestDisplay* bad=[[TestDisplay alloc]initWithDevice:device];[bad configureFrame:CGRectMake(0,0,4,3) width:4 height:3];
    TestDrawable* wrong=[TestDrawable new];wrong.texture=texture(device,1,1);bad.supplied=wrong;t=issue();
    need([bad submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Accepted,"wrong drawable test accepted");
    waitFor(^BOOL(void){return callbacks==7;});
    need(results.back()==NativeMetalDisplayResult::Failed&&errors.back()=="display_drawable_mismatch"&&wrong.presents==0,"wrong output prevents copy and admission");
    [bad shutdown];
    NativeMetalDisplay* unavailable=[[NativeMetalDisplay alloc]initWithDevice:nil];
    need([unavailable submitTexture:source ticket:t admission:admit completion:complete]==NativeMetalDisplaySubmit::Unavailable,"missing device retains CPU route");
    [unavailable shutdown];need(callbacks==7,"nonaccepted work still has no callback");
}
}
int main(){@autoreleasepool{try{
    canvas();id<MTLDevice> device=MTLCreateSystemDefaultDevice();
    if(!device){std::cout<<"{\"SKIP\":\"no Metal device\",\"canvas_checks\":"<<canvasChecks<<"}\n";return 0;}
    display(device);
    std::cout<<"{\"authored_checks\":"<<checks<<",\"canvas_checks\":"<<canvasChecks<<",\"passed\":true,\"real_window_presented\":false,\"App_executed\":false,\"offscreen_copy_pixels\":12}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
