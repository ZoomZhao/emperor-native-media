// SPDX-License-Identifier: MIT
#pragma once
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include "../scene_frame_gate.hpp"

// Display-only bridge. It never renders sprites or reads pixels back to CPU.
// Used by the opt-in Metal route; the CPU route remains the default.
enum class NativeMetalDisplaySubmit { Accepted, Busy, Closed, Invalid, Unavailable };
enum class NativeMetalDisplayResult { Scheduled, Stale, Cancelled, Failed };
typedef BOOL (^NativeMetalDisplayAdmission)(const game_lab::SceneFrameTicket&);
typedef void (^NativeMetalDisplayCompletion)(const game_lab::SceneFrameTicket&,NativeMetalDisplayResult,NSString*);
@interface NativeMetalDisplay : NSObject
@property(nonatomic,readonly,strong) CAMetalLayer* layer;
@property(nonatomic,readonly) NSUInteger inFlight;
@property(nonatomic,readonly) BOOL closed;
- (instancetype)initWithDevice:(id<MTLDevice>)device;
// Main-thread calls. Host owns placement/overlay composition and may keep the
// layer hidden until admission. Logical texture pixels are not Retina points.
- (BOOL)configureFrame:(CGRect)frame width:(uint32_t)width height:(uint32_t)height;
// Main thread: Accepted => exactly one completion on main. Other statuses => none.
// Admission runs only after GPU copy success, immediately before present, on main.
// It must recheck/consume FrameTicket and atomically install the candidate. Weakly
// capture the App owner. Scheduled is NOT an acknowledgement of visible display.
- (NativeMetalDisplaySubmit)submitTexture:(id<MTLTexture>)texture
                                  ticket:(const game_lab::SceneFrameTicket&)ticket
                               admission:(NativeMetalDisplayAdmission)admission
                              completion:(NativeMetalDisplayCompletion)completion;
- (void)shutdown;
// Isolated-test seam; production acquires a drawable on its private worker.
- (id<CAMetalDrawable>)acquireDrawable;
@end
