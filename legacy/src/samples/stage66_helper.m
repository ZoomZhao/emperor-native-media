// Own IPC helper. The initial HELLO check runs before any AppKit construction.
#import <AppKit/AppKit.h>
#import <Security/AuthSession.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
extern int sandbox_check(pid_t,const char *,int,...);
extern const int SANDBOX_CHECK_NO_REPORT;
@interface S66Window : NSWindow
@end
@implementation S66Window
- (BOOL)canBecomeKeyWindow{return NO;}
- (BOOL)canBecomeMainWindow{return NO;}
@end
@interface S66Canvas : NSView
@property(nonatomic,strong) NSImage *image;
@property(nonatomic) NSUInteger drawCalls;
@end
@implementation S66Canvas
- (BOOL)isFlipped{return YES;}
- (void)drawRect:(NSRect)rect {
    self.drawCalls++;
    [NSColor.blackColor setFill];NSRectFill(self.bounds);
    [self.image drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1.0 respectFlipped:YES hints:nil];
}
@end
static NSMutableData *pixels66;
static unsigned width66,height66;
static S66Window *window66;
static S66Canvas *view66;
static NSDictionary *surfaceState(void) {
    if(!pixels66)return @{@"error":@"surface not created"};
    return @{@"version":@1,@"surface_id":@1,@"refs":@1,@"width":@(width66),@"height":@(height66),
             @"pitch":@(width66*4),@"format":@"RGBA8",@"pixels_base64":[pixels66 base64EncodedStringWithOptions:0],
             @"window_shown":@(window66.visible),@"host_addresses_serialized":@NO};
}
static BOOL importSurface(NSDictionary *state) {
    unsigned w=[state[@"width"] unsignedIntValue],h=[state[@"height"] unsignedIntValue];
    if([state[@"version"] intValue]!=1||[state[@"surface_id"] intValue]!=1||[state[@"refs"] intValue]!=1||
       !w||!h||w>4096||h>4096||(uint64_t)w*h*4>32*1024*1024||[state[@"pitch"] unsignedIntValue]!=w*4||
       ![state[@"format"] isEqual:@"RGBA8"]||[state[@"host_addresses_serialized"] boolValue])return NO;
    NSData *body=[[NSData alloc]initWithBase64EncodedString:state[@"pixels_base64"] options:0];
    if(body.length!=(size_t)w*h*4)return NO;
    pixels66=[body mutableCopy];width66=w;height66=h;return YES;
}
static NSDictionary *createWindow(void) {
    if(!NSThread.isMainThread||!pixels66||window66)return @{@"error":@"invalid main-thread canvas state"};
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
    window66=[[S66Window alloc]initWithContentRect:NSMakeRect(120,120,384,256)
            styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
    window66.releasedWhenClosed=NO;window66.title=@"Stage66 own IPC canvas";
    view66=[[S66Canvas alloc]initWithFrame:NSMakeRect(0,0,384,256)];window66.contentView=view66;
    NSBitmapImageRep *bitmap=[[NSBitmapImageRep alloc]initWithBitmapDataPlanes:NULL pixelsWide:width66 pixelsHigh:height66
        bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
        bitmapFormat:0 bytesPerRow:width66*4 bitsPerPixel:32];
    memcpy(bitmap.bitmapData,pixels66.bytes,pixels66.length);
    NSImage *image=[[NSImage alloc]initWithSize:NSMakeSize(width66,height66)];[image addRepresentation:bitmap];view66.image=image;
    [window66 orderBack:nil];[view66 setNeedsDisplay:YES];[window66 display];
    NSBitmapImageRep *cache=[view66 bitmapImageRepForCachingDisplayInRect:view66.bounds];
    [view66 cacheDisplayInRect:view66.bounds toBitmapImageRep:cache];
    return @{@"window_created":@(window66!=nil),@"visible":@(window66.visible),@"key":@(window66.keyWindow),
             @"main":@(window66.mainWindow),@"NSApp_active":@(NSApp.active),@"main_thread":@(NSThread.isMainThread),
             @"draw_calls":@(view66.drawCalls),@"window_width":@384,@"window_height":@256,
             @"activation_requests":@0,@"canvas_pixel_bytes":@(pixels66.length),
             @"native_view_cache_bytes":@(cache.bytesPerRow*cache.pixelsHigh),
             @"global_input_hook_installed":@NO,@"screenshot_API_used":@NO};
}
static NSDictionary *closeWindow(void) {
    [window66 orderOut:nil];[window66 close];BOOL closed=!window66.visible;
    view66=nil;window66=nil;pixels66=nil;width66=height66=0;
    return @{@"closed":@(closed),@"window_visible":@NO};
}
static NSDictionary *permissions(NSString *root) {
    NSMutableDictionary *out=[NSMutableDictionary dictionary];
    out[@"sandbox_present"]=@(sandbox_check(getpid(),NULL,0));
    NSArray *cases=@[@[@"own_workspace_write",@"file-write-data",@1,[root stringByAppendingPathComponent:@"samples/stage66_helper.m"]],
                     @[@"original_workspace_write",@"file-write-data",@1,@"<LOCAL_ORIGINAL_PROJECT>"],
                     @[@"system_file_write",@"file-write-data",@1,@"/etc/hosts"],
                     @[@"launchservices_lookup",@"mach-lookup",@2,@"com.apple.coreservices.launchservicesd"],
                     @[@"windowserver_lookup",@"mach-lookup",@2,@"com.apple.windowserver.active"],
                     @[@"user_preferences_lookup",@"mach-lookup",@2,@"com.apple.cfprefsd.agent"]];
    for(NSArray *q in cases)out[q[0]]=@(sandbox_check(getpid(),[q[1] UTF8String],[q[2] intValue]|SANDBOX_CHECK_NO_REPORT,[q[3] UTF8String]));
    return out;
}
static NSDictionary *hello(NSString *root) {
    SecuritySessionId sid=0;SessionAttributeBits attrs=0;OSStatus status=SessionGetInfo(callerSecuritySession,&sid,&attrs);
    return @{@"protocol":@1,@"pid":@(getpid()),@"ppid":@(getppid()),@"uid":@(getuid()),@"euid":@(geteuid()),
             @"NSThread_isMainThread":@(NSThread.isMainThread),@"pthread_main_np":@(pthread_main_np()),
             @"session_status":@(status),@"session":@(sid),@"session_attributes":@(attrs),
             @"permissions":permissions(root),@"AppKit_constructed":@(NSApp!=nil),@"game_loaded":@NO};
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    alarm(45); // Independent hard bound for this own child, including blocked input.
    @autoreleasepool {
        NSString *root=[NSString stringWithUTF8String:argv[1]];char *line=NULL;size_t capacity=0;
        while(getline(&line,&capacity,stdin)>0) {
            @autoreleasepool {
                NSData *data=[[NSString stringWithUTF8String:line] dataUsingEncoding:NSUTF8StringEncoding];
                NSDictionary *command=[NSJSONSerialization JSONObjectWithData:data options:0 error:NULL];
                if(![command isKindOfClass:NSDictionary.class])break;
                NSDictionary *body;BOOL stop=NO;
                if([command[@"command"] isEqual:@"HELLO"])body=hello(root);
                else if([command[@"command"] isEqual:@"SURFACE_IMPORT"])body=@{@"imported":@(importSurface(command[@"state"]))};
                else if([command[@"command"] isEqual:@"SURFACE_EXPORT"])body=surfaceState();
                else if([command[@"command"] isEqual:@"WINDOW_CREATE"])body=createWindow();
                else if([command[@"command"] isEqual:@"CLOSE"]) {body=closeWindow();stop=YES;}
                else body=@{@"error":@"unsupported prototype command"};
                NSDictionary *response=@{@"id":command[@"id"]?:@0,@"body":body};
                NSData *bytes=[NSJSONSerialization dataWithJSONObject:response options:0 error:NULL];
                fwrite(bytes.bytes,1,bytes.length,stdout);fputc('\n',stdout);fflush(stdout);
                if(stop)break;
            }
        }
        closeWindow();free(line);return 0;
    }
}
