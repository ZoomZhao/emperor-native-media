// SPDX-License-Identifier: MIT
// Original read-only art decorates Cocoa controls. All text uses the system font.
#include "native_game_layout.hpp"
static inline NSRect NativeNSRect(game_lab::NativeGameRect r){return NSMakeRect(r.x,r.y,r.width,r.height);}

// Nine-slice drawing keeps corner and edge pixels at their source size. Only the
// interior is resampled, so a decorative frame is never stretched as a whole image.
static inline void NativeDrawNineSlice(NSImage* image,NSRect destination,CGFloat inset) {
    if(!image)return;
    const NSSize source=image.size;
    if(source.width<=0||source.height<=0||destination.size.width<=0||destination.size.height<=0)return;
    CGFloat x=MIN(inset,source.width/2),y=MIN(inset,source.height/2);
    x=MIN(x,destination.size.width/2);y=MIN(y,destination.size.height/2);
    if(x<=0||y<=0){[image drawInRect:destination fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:NO hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];return;}
    const CGFloat sx[3]={0,x,source.width-x},sy[3]={0,y,source.height-y};
    const CGFloat sw[3]={x,source.width-2*x,x},sh[3]={y,source.height-2*y,y};
    const CGFloat dx[3]={NSMinX(destination),NSMinX(destination)+x,NSMaxX(destination)-x};
    const CGFloat dy[3]={NSMinY(destination),NSMinY(destination)+y,NSMaxY(destination)-y};
    const CGFloat dw[3]={x,destination.size.width-2*x,x},dh[3]={y,destination.size.height-2*y,y};
    NSDictionary* hints=@{NSImageHintInterpolation:@(NSImageInterpolationNone)};
    for(int row=0;row<3;++row)for(int column=0;column<3;++column) {
        if(sw[column]<=0||sh[row]<=0||dw[column]<=0||dh[row]<=0)continue;
        [image drawInRect:NSMakeRect(dx[column],dy[row],dw[column],dh[row])
                 fromRect:NSMakeRect(sx[column],sy[row],sw[column],sh[row])
                operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:NO hints:hints];
    }
}

// Icons keep their source aspect ratio and are never enlarged. At 1x the natural
// pixel size is used; only oversized icons are uniformly reduced to fit the box.
static inline NSRect NativeAspectFit(NSImage* image,NSRect box) {
    const NSSize source=image.size;
    if(source.width<=0||source.height<=0||box.size.width<=0||box.size.height<=0)return NSZeroRect;
    CGFloat scale=1;
    if(source.width>box.size.width||source.height>box.size.height)
        scale=MIN(box.size.width/source.width,box.size.height/source.height);
    const NSSize fit=NSMakeSize(source.width*scale,source.height*scale);
    return NSMakeRect(NSMidX(box)-fit.width/2,NSMidY(box)-fit.height/2,fit.width,fit.height);
}

@interface NativeGameChrome : NSView
@property(nonatomic,strong) NSImage* topTexture;
@property(nonatomic,strong) NSImage* sideTexture;
@property(nonatomic,strong) NSImage* bottomTexture;
@property(nonatomic,strong) NSImage* lowerSideTexture;
@end
@implementation NativeGameChrome
- (void)drawRect:(NSRect)dirty {(void)dirty;[[NSColor colorWithRed:.17 green:.10 blue:.055 alpha:1]setFill];NSRectFill(self.bounds);
 const auto layout=game_lab::nativeGameLayout(self.bounds.size.width,self.bounds.size.height);
 if(_topTexture)NativeDrawNineSlice(_topTexture,NativeNSRect(layout.top),10);
 if(_bottomTexture)NativeDrawNineSlice(_bottomTexture,NativeNSRect(layout.bottom),4);
 if(_sideTexture)[_sideTexture drawInRect:NativeNSRect(layout.sideUpper) fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:NO hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];
 if(_lowerSideTexture)NativeDrawNineSlice(_lowerSideTexture,NativeNSRect(layout.sideLower),12);
}
@end
@interface NativeGameButton : NSButton
@end
@implementation NativeGameButton
- (void)drawRect:(NSRect)dirty {(void)dirty;const BOOL selected=self.state==NSControlStateValueOn;
 [[NSColor colorWithRed:selected?.44:.27 green:selected?.27:.16 blue:.07 alpha:1]setFill];NSRectFill(self.bounds);
 [[NSColor colorWithRed:.75 green:.54 blue:.27 alpha:self.enabled?1:.35]setStroke];[[NSBezierPath bezierPathWithRect:NSInsetRect(self.bounds,1,1)]stroke];
 const CGFloat opacity=self.enabled?1:.38;CGFloat inset=8;
 if(self.image){const NSRect icon=NativeAspectFit(self.image,NSMakeRect(0,0,48,NSHeight(self.bounds)));if(!NSIsEmptyRect(icon))[self.image drawInRect:icon fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:opacity respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];inset=48;}
 NSDictionary* attrs=@{NSFontAttributeName:self.font?:[NSFont systemFontOfSize:13],NSForegroundColorAttributeName:[[NSColor colorWithRed:1 green:.92 blue:.72 alpha:1]colorWithAlphaComponent:opacity]};
 [self.title drawInRect:NSMakeRect(inset,(self.bounds.size.height-20)/2,self.bounds.size.width-inset-4,24) withAttributes:attrs];
 if(self.highlighted){[[NSColor colorWithWhite:1 alpha:.13]setFill];NSRectFill(self.bounds);}
}
@end

// Original category art already contains its frame. Do not add a second wide
// text-button frame or distort the 46x38 reference slots. Text is system-font
// tooltip/accessibility metadata; unsupported categories are clearly disabled.
@interface NativeCategoryButton : NSButton
@end
@implementation NativeCategoryButton
- (void)drawRect:(NSRect)dirty {(void)dirty;
 const CGFloat opacity=self.enabled?1:.36;
 if(self.image)[self.image drawInRect:NativeAspectFit(self.image,self.bounds) fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:opacity respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];
 if(self.state==NSControlStateValueOn){[[NSColor colorWithRed:1 green:.83 blue:.3 alpha:1]setStroke];NSBezierPath* path=[NSBezierPath bezierPathWithRect:NSInsetRect(self.bounds,1,1)];path.lineWidth=2;[path stroke];}
 if(self.highlighted){[[NSColor colorWithWhite:1 alpha:.15]setFill];NSRectFill(self.bounds);}
}
@end

// Building selector slots use the measured original 54x53 grid. The known
// housing/well/food icons retain source pixels; native-only tools use system text.
@interface NativeBuildButton : NSButton
@end
@implementation NativeBuildButton
- (void)drawRect:(NSRect)dirty {(void)dirty;
 const CGFloat opacity=self.enabled?1:.36;
 if(self.image)[self.image drawInRect:NativeAspectFit(self.image,self.bounds) fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:opacity respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];
 else {[[NSColor colorWithRed:.27 green:.16 blue:.07 alpha:1]setFill];NSRectFill(self.bounds);[[NSColor colorWithRed:.75 green:.54 blue:.27 alpha:opacity]setStroke];[[NSBezierPath bezierPathWithRect:NSInsetRect(self.bounds,1,1)]stroke];
 NSMutableParagraphStyle* paragraph=[[NSMutableParagraphStyle alloc]init];paragraph.alignment=NSTextAlignmentCenter;
 [self.title drawInRect:NSInsetRect(self.bounds,3,13) withAttributes:@{NSFontAttributeName:[NSFont systemFontOfSize:11],NSForegroundColorAttributeName:[NSColor colorWithWhite:1 alpha:opacity],NSParagraphStyleAttributeName:paragraph}];}
 if(self.state==NSControlStateValueOn){[[NSColor colorWithRed:1 green:.83 blue:.3 alpha:1]setStroke];NSBezierPath* path=[NSBezierPath bezierPathWithRect:NSInsetRect(self.bounds,1,1)];path.lineWidth=2;[path stroke];}
 if(self.highlighted){[[NSColor colorWithWhite:1 alpha:.15]setFill];NSRectFill(self.bounds);}
}
@end
