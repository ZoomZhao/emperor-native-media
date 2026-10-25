// SPDX-License-Identifier: MIT
// AppKit overlay shared by CPU and Metal; no scene sampling or readback.
#include "../native_minimap.hpp"
@interface NativeMinimapView : NSView {
    std::shared_ptr<const game_lab::NativeTerrainOverview> _overview;
    NSImage* _terrainImage;
    std::vector<game_lab::NativeSessionObject> _objects;
    NSBezierPath* _roadMarks;NSBezierPath* _buildingMarks;
    game_lab::NativeCanvasRect _viewport;
    NSPoint _down;BOOL _pressed;
}
@property(nonatomic,copy) void (^onLocate)(double,double);
- (void)installOverview:(std::shared_ptr<const game_lab::NativeTerrainOverview>)overview image:(NSImage*)image;
- (void)updateCamera:(const game_lab::OriginalMapCamera&)camera objects:(const std::vector<game_lab::NativeSessionObject>&)objects;
@end
@implementation NativeMinimapView
- (BOOL)isFlipped{return YES;}
- (BOOL)acceptsFirstMouse:(NSEvent*)event{(void)event;return YES;}
- (void)installOverview:(std::shared_ptr<const game_lab::NativeTerrainOverview>)overview image:(NSImage*)image {
    _overview=std::move(overview);_terrainImage=image;_objects.clear();_roadMarks=nil;_buildingMarks=nil;_pressed=NO;self.needsDisplay=YES;
}
- (void)updateCamera:(const game_lab::OriginalMapCamera&)camera objects:(const std::vector<game_lab::NativeSessionObject>&)objects {
    if(!_overview)return;
    _viewport=game_lab::nativeOverviewViewport(*_overview,camera);
    bool same=objects.size()==_objects.size();if(same)for(size_t i=0;i<objects.size();++i){const auto&a=objects[i];const auto&b=_objects[i];if(a.id!=b.id||a.kind!=b.kind||a.x!=b.x||a.y!=b.y){same=false;break;}}
    if(!same){_objects=objects;_roadMarks=[NSBezierPath bezierPath];_buildingMarks=[NSBezierPath bezierPath];
        for(const auto&o:objects){const double s=game_lab::nativeObjectSide(o.kind);NSBezierPath* path=o.kind==game_lab::NativeObjectKind::Road?_roadMarks:_buildingMarks;
            const double xs[]={o.x-.5,o.x+s-.5,o.x+s-.5,o.x-.5},ys[]={o.y-.5,o.y-.5,o.y+s-.5,o.y+s-.5};
            for(unsigned i=0;i<4;++i){const auto p=_overview->project(xs[i],ys[i]);const NSPoint point=NSMakePoint(p.x,p.y);if(i==0)[path moveToPoint:point];else[path lineToPoint:point];}[path closePath];}}
    self.accessibilityLabel=[NSString stringWithFormat:@"地形概览 %u×%u；相机 %.2f,%.2f；缩放 %u；视窗 %.5f,%.5f,%.5f,%.5f",_overview->side,_overview->side,camera.centerX,camera.centerY,camera.scale,_viewport.x,_viewport.y,_viewport.width,_viewport.height];self.needsDisplay=YES;
}
- (void)drawRect:(NSRect)dirty {(void)dirty;[[NSColor colorWithRed:.20 green:.17 blue:.11 alpha:1]setFill];NSRectFill(self.bounds);
    if(!_overview||!_terrainImage)return;
    [_terrainImage drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];
    [NSGraphicsContext saveGraphicsState];NSRectClip(self.bounds);NSAffineTransform*t=[NSAffineTransform transform];[t scaleXBy:self.bounds.size.width yBy:self.bounds.size.height];
    [[NSColor colorWithRed:.9 green:.83 blue:.65 alpha:1]setFill];if(_roadMarks)[[t transformBezierPath:_roadMarks]fill];[[NSColor colorWithRed:.8 green:.35 blue:.23 alpha:1]setFill];if(_buildingMarks)[[t transformBezierPath:_buildingMarks]fill];
    if(_viewport.width>0&&_viewport.height>0){NSRect r=NSMakeRect(_viewport.x*self.bounds.size.width,_viewport.y*self.bounds.size.height,_viewport.width*self.bounds.size.width,_viewport.height*self.bounds.size.height);NSBezierPath*p=[NSBezierPath bezierPathWithRect:r];[NSColor.blackColor setStroke];p.lineWidth=3;[p stroke];[NSColor.whiteColor setStroke];p.lineWidth=1;[p stroke];}[NSGraphicsContext restoreGraphicsState];
}
- (void)mouseDown:(NSEvent*)event {_down=[self convertPoint:event.locationInWindow fromView:nil];_pressed=_overview&&NSPointInRect(_down,self.bounds);}
- (void)mouseUp:(NSEvent*)event {const BOOL pressed=_pressed;_pressed=NO;NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];if(!pressed||!_overview||!NSPointInRect(p,self.bounds)||std::hypot(p.x-_down.x,p.y-_down.y)>4)return;
    const auto point=game_lab::nativeOverviewPick(*_overview,p.x/self.bounds.size.width,p.y/self.bounds.size.height);if(point&&_onLocate)_onLocate(point->x,point->y);
}
- (void)rightMouseDown:(NSEvent*)event {(void)event;_pressed=NO;}
- (void)viewDidMoveToWindow {[super viewDidMoveToWindow];_pressed=NO;}
@end
