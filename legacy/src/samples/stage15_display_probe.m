#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <CoreGraphics/CoreGraphics.h>
int main(void) {@autoreleasepool {
 NSScreen *screen=NSScreen.mainScreen;if(!screen)return 2;
 NSNumber *number=screen.deviceDescription[@"NSScreenNumber"];
 CGDirectDisplayID display=number.unsignedIntValue;CGDisplayModeRef mode=CGDisplayCopyDisplayMode(display);if(!mode)return 3;
 NSImage *icon=[NSWorkspace.sharedWorkspace iconForContentType:UTTypeFolder];NSSize size=icon.size;
 NSSize resolution=[screen.deviceDescription[NSDeviceResolution] sizeValue];CGSize mm=CGDisplayScreenSize(display);
 NSDictionary *record=@{@"main_thread":@(NSThread.isMainThread),@"display_id":@(display),@"screen_width_points":@(screen.frame.size.width),@"screen_height_points":@(screen.frame.size.height),@"backing_scale":@(screen.backingScaleFactor),@"mode_width":@(CGDisplayModeGetWidth(mode)),@"mode_height":@(CGDisplayModeGetHeight(mode)),@"mode_pixel_width":@(CGDisplayModeGetPixelWidth(mode)),@"mode_pixel_height":@(CGDisplayModeGetPixelHeight(mode)),@"physical_width_mm":@(mm.width),@"physical_height_mm":@(mm.height),@"device_resolution_x":@(resolution.width),@"device_resolution_y":@(resolution.height),@"system_icon_width_points":@(size.width),@"system_icon_height_points":@(size.height),@"legacy_scroller_points":@([NSScroller scrollerWidthForControlSize:NSControlSizeRegular scrollerStyle:NSScrollerStyleLegacy]),@"device_description":screen.deviceDescription.description};
 NSData *json=[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingPrettyPrinted error:NULL];fwrite(json.bytes,1,json.length,stdout);fputc('\n',stdout);CGDisplayModeRelease(mode);return 0;
}}
