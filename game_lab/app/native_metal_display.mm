// SPDX-License-Identifier: MIT
#include "native_metal_display.hpp"
#import <QuartzCore/CATransaction.h>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
@implementation NativeMetalDisplay {
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commands;
    dispatch_queue_t _worker;
    CAMetalLayer* _layer;
    NSUInteger _inFlight;
    BOOL _closed;
    uint64_t _geometrySerial;
    uint32_t _width,_height;
}
- (instancetype)initWithDevice:(id<MTLDevice>)device {
    if(!(self=[super init]))return nil;
    _device=device;_commands=[device newCommandQueue];
    _worker=dispatch_queue_create("org.emperor.native.display-copy",DISPATCH_QUEUE_SERIAL);
    _layer=[CAMetalLayer layer];_layer.device=device;_layer.pixelFormat=MTLPixelFormatBGRA8Unorm;
    _layer.framebufferOnly=NO; // Blit destination, not a render-pass attachment.
    // Three display buffers avoid waiting for the compositor to recycle the
    // previous image. Accepted copies stay capped at two below. The process-local
    // diagnostic override keeps the old pool available for controlled comparisons.
    const char* pool=std::getenv("EMPEROR_NATIVE_DRAWABLES");
    _layer.maximumDrawableCount=pool&&std::strcmp(pool,"2")==0?2:3;
    _layer.allowsNextDrawableTimeout=YES;
    _layer.presentsWithTransaction=YES;_layer.opaque=NO;_layer.hidden=YES;
    _layer.contentsScale=1;_layer.minificationFilter=kCAFilterNearest;_layer.magnificationFilter=kCAFilterNearest;
    _layer.contentsGravity=kCAGravityResize;
    // Native CPU CGImages also declare sRGB, independent of layer backing.
    CGColorSpaceRef space=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);_layer.colorspace=space;CGColorSpaceRelease(space);
    return self;
}
- (CAMetalLayer*)layer{return _layer;}
- (NSUInteger)inFlight{return _inFlight;}
- (BOOL)closed{return _closed;}
- (BOOL)configureFrame:(CGRect)frame width:(uint32_t)width height:(uint32_t)height {
    NSAssert(NSThread.isMainThread,@"display geometry is main-thread owned");
    if(_closed)return NO;
    // Every new intent, including invalid input, invalidates acquired old drawables.
    if(_geometrySerial==std::numeric_limits<uint64_t>::max()){[self shutdown];return NO;}
    ++_geometrySerial;
    if(!width||!height||width>2048||height>2048||!std::isfinite(frame.origin.x)||!std::isfinite(frame.origin.y)||
        !std::isfinite(frame.size.width)||!std::isfinite(frame.size.height)||frame.size.width<=0||frame.size.height<=0){_width=_height=0;return NO;}
    _width=width;_height=height;
    [CATransaction begin];[CATransaction setDisableActions:YES];_layer.frame=frame;_layer.drawableSize=CGSizeMake(width,height);[CATransaction commit];
    return YES;
}
- (id<CAMetalDrawable>)acquireDrawable{return [_layer nextDrawable];}
- (void)finishTicket:(game_lab::SceneFrameTicket)ticket geometry:(uint64_t)geometry
           drawable:(id<CAMetalDrawable>)drawable error:(NSString*)error
          admission:(NativeMetalDisplayAdmission)admission completion:(NativeMetalDisplayCompletion)completion {
    NSAssert(NSThread.isMainThread,@"display completion must run on main");
    NSAssert(_inFlight>0,@"completion belongs to one accepted request");--_inFlight;
    if(_closed){completion(ticket,NativeMetalDisplayResult::Cancelled,@"display_closed");return;}
    if(geometry!=_geometrySerial){completion(ticket,NativeMetalDisplayResult::Stale,@"display_geometry_changed");return;}
    if(error){completion(ticket,NativeMetalDisplayResult::Failed,error);return;}
    if(!admission(ticket)){completion(ticket,NativeMetalDisplayResult::Stale,@"display_ticket_stale");return;}
    // GPU copy has completed. Admission, candidate publication and scheduling are
    // one main-thread turn; no waitUntilCompleted or main-thread nextDrawable.
    [CATransaction begin];[CATransaction setDisableActions:YES];[drawable present];[CATransaction commit];
    completion(ticket,NativeMetalDisplayResult::Scheduled,@"");
}
- (NativeMetalDisplaySubmit)submitTexture:(id<MTLTexture>)texture
                                  ticket:(const game_lab::SceneFrameTicket&)ticket
                               admission:(NativeMetalDisplayAdmission)admission
                              completion:(NativeMetalDisplayCompletion)completion {
    NSAssert(NSThread.isMainThread,@"display submissions are main-thread owned");
    if(_closed)return NativeMetalDisplaySubmit::Closed;
    if(!_device||!_commands)return NativeMetalDisplaySubmit::Unavailable;
    if(!texture||!admission||!completion||!_width||!_height||texture.device!=_device||texture.framebufferOnly||
        texture.textureType!=MTLTextureType2D||texture.sampleCount!=1||texture.pixelFormat!=MTLPixelFormatBGRA8Unorm||
        texture.width!=ticket.width||texture.height!=ticket.height||ticket.width!=_width||ticket.height!=_height)
        return NativeMetalDisplaySubmit::Invalid;
    if(_inFlight>=2)return NativeMetalDisplaySubmit::Busy;
    ++_inFlight;const auto identity=ticket;const auto geometry=_geometrySerial;
    const double submittedAt=CACurrentMediaTime();const auto poolSize=_layer.maximumDrawableCount;
    // ARC/block captures strongly retain texture, drawable and bridge until the
    // accepted request's one completion; caller may release its Frame immediately.
    dispatch_async(_worker,^{@autoreleasepool {
        const double workerAt=CACurrentMediaTime();
        id<CAMetalDrawable> drawable=[self acquireDrawable];
        const double acquiredAt=CACurrentMediaTime();
        NSString* error=nil;
        if(!drawable)error=@"display_drawable_unavailable";
        else if(drawable.texture.device!=self->_device||drawable.texture.framebufferOnly||
            drawable.texture.pixelFormat!=MTLPixelFormatBGRA8Unorm||drawable.texture.width!=identity.width||
            drawable.texture.height!=identity.height||drawable.texture.textureType!=MTLTextureType2D||drawable.texture.sampleCount!=1)
            error=@"display_drawable_mismatch";
        id<MTLCommandBuffer> buffer=error?nil:[self->_commands commandBuffer];
        if(!error&&!buffer)error=@"display_command_buffer_unavailable";
        id<MTLBlitCommandEncoder> blit=error?nil:[buffer blitCommandEncoder];
        if(!error&&!blit)error=@"display_blit_encoder_unavailable";
        if(error){dispatch_async(dispatch_get_main_queue(),^{[self finishTicket:identity geometry:geometry drawable:nil error:error admission:admission completion:completion];});return;}
        [blit copyFromTexture:texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
                   sourceSize:MTLSizeMake(identity.width,identity.height,1) toTexture:drawable.texture destinationSlice:0
             destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[blit endEncoding];
        [buffer addCompletedHandler:^(id<MTLCommandBuffer> completed){
            const double copiedAt=CACurrentMediaTime();
            NSString* failure=completed.status==MTLCommandBufferStatusCompleted?nil:@"display_gpu_copy_failed";
            dispatch_async(dispatch_get_main_queue(),^{
                std::fprintf(stderr,"native_display_timing request=%llu pool=%lu queue_ms=%.3f acquire_ms=%.3f copy_ms=%.3f main_hop_ms=%.3f pixels=%ux%u\n",identity.requestSerial,(unsigned long)poolSize,(workerAt-submittedAt)*1000,(acquiredAt-workerAt)*1000,(copiedAt-acquiredAt)*1000,(CACurrentMediaTime()-copiedAt)*1000,identity.width,identity.height);
                [self finishTicket:identity geometry:geometry drawable:drawable error:failure admission:admission completion:completion];
            });
        }];[buffer commit];
    }});
    return NativeMetalDisplaySubmit::Accepted;
}
- (void)shutdown {
    NSAssert(NSThread.isMainThread,@"display shutdown is main-thread owned");
    _closed=YES;[CATransaction begin];[CATransaction setDisableActions:YES];_layer.hidden=YES;[CATransaction commit];
    // No blocking GPU drain. Accepted requests finish once as Cancelled. The host
    // removes its layer and weak App capture prevents messaging a destroyed owner.
}
@end
