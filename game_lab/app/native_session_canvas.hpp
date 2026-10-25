// SPDX-License-Identifier: MIT
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CATransaction.h>
// Cocoa input surface. Logical view points -> fitted image pixels -> grid.
@class NativeSessionCanvas;
@interface NativeCanvasOverlay : NSView
@property(nonatomic,weak) NativeSessionCanvas* owner;
@end
@interface NativeMetalHostView : NSView
@end
@implementation NativeMetalHostView
- (BOOL)isFlipped{return YES;}
- (NSView*)hitTest:(NSPoint)p{(void)p;return nil;}
@end
@interface NativeSessionCanvas : NSView {
    NSTrackingArea* _tracking; NSPoint _down,_last; BOOL _pressed,_panning,_dragged,_roadGesture; id _resignObserver; NativeMetalHostView* _metalHost; NativeCanvasOverlay* _metalOverlay; CAMetalLayer* _cityLayer;
}
@property(nonatomic,strong) NSImage* image;
// Logical art pixels are independent of a CPU NSImage for the future texture path.
@property(nonatomic) NSSize logicalImageSize;
@property(nonatomic,copy) void (^onGeometryChange)(void);
@property(nonatomic,strong) NSBezierPath* previewPath;
@property(nonatomic,strong) NSBezierPath* existingPath;
@property(nonatomic,strong) NSBezierPath* waterPath;
@property(nonatomic,strong) NSBezierPath* wetHousePath;
@property(nonatomic,strong) NSBezierPath* dryHousePath;
@property(nonatomic,strong) NSBezierPath* selectionPath;
@property(nonatomic) BOOL previewAllowed;
@property(nonatomic,copy) void (^onPoint)(double,double);
@property(nonatomic,copy) void (^onHover)(double,double);
@property(nonatomic,copy) void (^onPan)(double,double);
@property(nonatomic,copy) void (^onZoom)(int,double,double);
@property(nonatomic,copy) void (^onCancel)(void);
@property(nonatomic,copy) void (^onFocusLost)(void);
@property(nonatomic) BOOL browseMode;
@property(nonatomic) BOOL roadMode;
@property(nonatomic,copy) void (^onRoadBegin)(double,double);
@property(nonatomic,copy) void (^onRoadMove)(double,double);
@property(nonatomic,copy) void (^onRoadFinish)(double,double);
@property(nonatomic,copy) void (^onGestureCancel)(void);
- (void)prepareMetalLayer:(CAMetalLayer*)layer logicalSize:(NSSize)size;
- (void)showMetalLayer:(CAMetalLayer*)layer logicalSize:(NSSize)size;
- (CGRect)metalLocalFrame;
- (void)layoutMetalFit;
- (void)drawNativeOverlays;
- (void)cancelGesture;
- (game_lab::NativeCanvasRect)imageFit;
- (void)refreshPointer;
@end
@implementation NativeSessionCanvas
- (BOOL)isFlipped{return YES;}
- (BOOL)acceptsFirstResponder{return YES;}
- (BOOL)acceptsFirstMouse:(NSEvent*)event{(void)event;return YES;}
- (game_lab::NativeCanvasRect)imageFit{return game_lab::nativeCanvasFit(self.bounds.size.width,self.bounds.size.height,self.logicalImageSize.width,self.logicalImageSize.height);}
- (std::optional<game_lab::OriginalMapPoint>)imagePoint:(NSPoint)p {return game_lab::nativeCanvasImagePoint([self imageFit],self.logicalImageSize.width,self.logicalImageSize.height,p.x,p.y);}
- (void)setLogicalImageSize:(NSSize)size {
    _logicalImageSize=std::isfinite(size.width)&&std::isfinite(size.height)&&size.width>0&&size.height>0&&size.width<=2048&&size.height<=2048?size:NSZeroSize;
    self.needsDisplay=YES;
}
- (void)setImage:(NSImage*)image {_image=image;self.logicalImageSize=image?image.size:NSZeroSize;_metalHost.hidden=YES;}
- (void)setNeedsDisplay:(BOOL)value {[super setNeedsDisplay:value];_metalOverlay.needsDisplay=value;}
- (void)prepareMetalLayer:(CAMetalLayer*)layer logicalSize:(NSSize)size {
    if(!_metalHost){self.wantsLayer=YES;_metalHost=[[NativeMetalHostView alloc]initWithFrame:NSZeroRect];_metalHost.wantsLayer=YES;[self addSubview:_metalHost];
        _metalOverlay=[[NativeCanvasOverlay alloc]initWithFrame:self.bounds];_metalOverlay.owner=self;_metalOverlay.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;[self addSubview:_metalOverlay];}
    if(_cityLayer!=layer){[_cityLayer removeFromSuperlayer];_cityLayer=layer;[_metalHost.layer addSublayer:layer];}
    if(_metalHost.hidden||self.image)layer.hidden=YES;
    const auto f=game_lab::nativeCanvasFit(self.bounds.size.width,self.bounds.size.height,size.width,size.height);
    _metalHost.frame=NSMakeRect(f.x,f.y,f.width,f.height);_metalHost.hidden=NO;_metalOverlay.frame=self.bounds;
}
- (void)layoutMetalFit {
    if(!_metalHost||self.logicalImageSize.width<=0)return;const auto f=[self imageFit];
    [CATransaction begin];[CATransaction setDisableActions:YES];_metalHost.frame=NSMakeRect(f.x,f.y,f.width,f.height);_cityLayer.frame=CGRectMake(0,0,f.width,f.height);_metalOverlay.frame=self.bounds;[CATransaction commit];
}
- (CGRect)metalLocalFrame {return CGRectMake(0,0,_metalHost.bounds.size.width,_metalHost.bounds.size.height);}
- (void)showMetalLayer:(CAMetalLayer*)layer logicalSize:(NSSize)size {
    _image=nil;self.logicalImageSize=size;_metalHost.hidden=NO;layer.hidden=NO;self.needsDisplay=YES;
}
- (void)drawRect:(NSRect)dirty {(void)dirty;[[NSColor colorWithWhite:.12 alpha:1]setFill];NSRectFill(self.bounds);
    if(self.logicalImageSize.width<=0||self.logicalImageSize.height<=0)return;
    const auto f=[self imageFit];if(self.image)[self.image drawInRect:NSMakeRect(f.x,f.y,f.width,f.height) fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];
    if(!_metalOverlay)[self drawNativeOverlays];
}
- (void)drawNativeOverlays {
    if(self.logicalImageSize.width<=0||self.logicalImageSize.height<=0)return;
    const auto f=[self imageFit];
    [NSGraphicsContext saveGraphicsState];NSRectClip(NSMakeRect(f.x,f.y,f.width,f.height));
    NSAffineTransform*t=[NSAffineTransform transform];[t translateXBy:f.x yBy:f.y];[t scaleXBy:f.width/self.logicalImageSize.width yBy:f.height/self.logicalImageSize.height];
    const auto paint=[&](NSBezierPath* raw,NSColor* color){if(!raw)return;NSBezierPath*p=[t transformBezierPath:raw];[[color colorWithAlphaComponent:.3]setFill];[p fill];[color setStroke];p.lineWidth=2;[p stroke];};
    if(self.waterPath){NSBezierPath* coverage=[t transformBezierPath:self.waterPath];[[NSColor.systemCyanColor colorWithAlphaComponent:.12]setFill];[coverage fill];}
    paint(self.wetHousePath,NSColor.systemCyanColor);paint(self.dryHousePath,NSColor.systemOrangeColor);
    paint(self.selectionPath,NSColor.systemYellowColor);
    paint(self.previewPath,self.previewAllowed?NSColor.systemGreenColor:NSColor.systemRedColor);paint(self.existingPath,NSColor.systemBlueColor);[NSGraphicsContext restoreGraphicsState];
}
- (void)updateTrackingAreas {[super updateTrackingAreas];if(_tracking)[self removeTrackingArea:_tracking];_tracking=[[NSTrackingArea alloc]initWithRect:NSZeroRect options:NSTrackingMouseEnteredAndExited|NSTrackingMouseMoved|NSTrackingActiveInKeyWindow|NSTrackingInVisibleRect owner:self userInfo:nil];[self addTrackingArea:_tracking];}
- (void)refreshPointer {if(!self.window)return;const auto p=[self imagePoint:[self convertPoint:self.window.mouseLocationOutsideOfEventStream fromView:nil]];if(_onHover)_onHover(p?p->x:NAN,p?p->y:NAN);}
- (void)mouseMoved:(NSEvent*)event {const auto p=[self imagePoint:[self convertPoint:event.locationInWindow fromView:nil]];if(_onHover)_onHover(p?p->x:NAN,p?p->y:NAN);}
- (void)mouseEntered:(NSEvent*)event {[self mouseMoved:event];}
- (void)mouseExited:(NSEvent*)event {(void)event;[self cancelGesture];if(_onHover)_onHover(NAN,NAN);}
- (void)cancelGesture {const BOOL had=_pressed||_roadGesture;_pressed=NO;_roadGesture=NO;_dragged=NO;if(had&&_onGestureCancel)_onGestureCancel();}
- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];if(_resignObserver)[NSNotificationCenter.defaultCenter removeObserver:_resignObserver];_resignObserver=nil;
    [self cancelGesture];if(self.window){__weak NativeSessionCanvas* me=self;_resignObserver=[NSNotificationCenter.defaultCenter addObserverForName:NSWindowDidResignKeyNotification object:self.window queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification* note){(void)note;[me cancelGesture];if(me.onFocusLost)me.onFocusLost();if(me.onHover)me.onHover(NAN,NAN);}];}
}
- (void)dealloc {if(_resignObserver)[NSNotificationCenter.defaultCenter removeObserver:_resignObserver];}
- (void)mouseDown:(NSEvent*)event {
    [self cancelGesture];[self.window makeFirstResponder:self];_down=_last=[self convertPoint:event.locationInWindow fromView:nil];
    const auto p=[self imagePoint:_down];_pressed=bool(p);_dragged=NO;_panning=self.browseMode||(event.modifierFlags&NSEventModifierFlagOption);
    _roadGesture=_pressed&&!_panning&&self.roadMode;if(_roadGesture&&_onRoadBegin)_onRoadBegin(p->x,p->y);[self mouseMoved:event];
}
- (void)mouseDragged:(NSEvent*)event {
    if(!_pressed)return;const NSPoint point=[self convertPoint:event.locationInWindow fromView:nil];const auto p=[self imagePoint:point];
    if(!p){[self cancelGesture];if(_onHover)_onHover(NAN,NAN);return;}
    if(std::hypot(point.x-_down.x,point.y-_down.y)>4)_dragged=YES;
    const auto f=[self imageFit];if(_panning&&_dragged&&f.width>0&&f.height>0&&_onPan)_onPan((point.x-_last.x)*self.logicalImageSize.width/f.width,(point.y-_last.y)*self.logicalImageSize.height/f.height);
    if(_roadGesture&&_onRoadMove)_onRoadMove(p->x,p->y);_last=point;[self mouseMoved:event];
}
- (void)mouseUp:(NSEvent*)event {
    const BOOL road=_pressed&&_roadGesture,place=_pressed&&!_dragged&&(!_panning||self.browseMode)&&!_roadGesture;
    const auto p=[self imagePoint:[self convertPoint:event.locationInWindow fromView:nil]];
    if(!p){[self cancelGesture];if(_onHover)_onHover(NAN,NAN);return;}
    _pressed=NO;_roadGesture=NO;if(road&&_onRoadFinish)_onRoadFinish(p->x,p->y);else if(place&&_onPoint)_onPoint(p->x,p->y);[self mouseMoved:event];
}
- (void)rightMouseDown:(NSEvent*)event {(void)event;[self cancelGesture];if(_onCancel)_onCancel();}
- (void)keyDown:(NSEvent*)event {if(event.keyCode==53){[self cancelGesture];if(_onCancel)_onCancel();return;}
    if(event.keyCode>=123&&event.keyCode<=126){[self cancelGesture];if(_onPan)_onPan(event.keyCode==123?80:event.keyCode==124?-80:0,event.keyCode==125?-40:event.keyCode==126?40:0);[self refreshPointer];return;}[super keyDown:event];}
- (void)scrollWheel:(NSEvent*)event {[self cancelGesture];const auto p=[self imagePoint:[self convertPoint:event.locationInWindow fromView:nil]];if(p&&_onZoom&&std::abs(event.scrollingDeltaY)>.01)_onZoom(event.scrollingDeltaY>0?1:-1,p->x,p->y);[self mouseMoved:event];}
- (void)setFrameSize:(NSSize)size {[self cancelGesture];[super setFrameSize:size];[self layoutMetalFit];if(_onGeometryChange)_onGeometryChange();[self refreshPointer];self.needsDisplay=YES;}
- (void)viewDidChangeBackingProperties {[super viewDidChangeBackingProperties];[self cancelGesture];if(_onGeometryChange)_onGeometryChange();[self refreshPointer];self.needsDisplay=YES;}
@end

@implementation NativeCanvasOverlay
- (BOOL)isFlipped{return YES;}
- (BOOL)isOpaque{return NO;}
- (NSView*)hitTest:(NSPoint)p{(void)p;return nil;}
- (void)drawRect:(NSRect)dirty{(void)dirty;[self.owner drawNativeOverlays];}
@end
