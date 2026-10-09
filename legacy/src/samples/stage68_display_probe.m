// One bounded, metadata-only native probe; no capture, activation, or input.
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Security/AuthSession.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
extern int sandbox_check(pid_t,const char *,int,...);
extern const int SANDBOX_CHECK_NO_REPORT;
static void emit68(NSDictionary *record) {
    NSData *body=[NSJSONSerialization dataWithJSONObject:record options:0 error:NULL];
    fwrite(body.bytes,1,body.length,stdout);fputc('\n',stdout);fflush(stdout);
}
static NSDictionary *permissions68(NSString *root) {
    NSMutableDictionary *out=[NSMutableDictionary dictionary];
    out[@"sandbox_present"]=@(sandbox_check(getpid(),NULL,0));
    NSArray *queries=@[@[@"own_workspace_write",@"file-write-data",@1,[root stringByAppendingPathComponent:@"samples/stage66_helper.m"]],
        @[@"original_workspace_write",@"file-write-data",@1,@"<LOCAL_ORIGINAL_PROJECT>"],
        @[@"system_file_write",@"file-write-data",@1,@"/etc/hosts"],
        @[@"launchservices_lookup",@"mach-lookup",@2,@"com.apple.coreservices.launchservicesd"],
        @[@"windowserver_lookup",@"mach-lookup",@2,@"com.apple.windowserver.active"],
        @[@"user_preferences_lookup",@"mach-lookup",@2,@"com.apple.cfprefsd.agent"]];
    for(NSArray *q in queries)out[q[0]]=@(sandbox_check(getpid(),[q[1] UTF8String],[q[2] intValue]|SANDBOX_CHECK_NO_REPORT,[q[3] UTF8String]));
    return out;
}
static NSDictionary *rect68(CGRect r) {
    return @{@"x":@(r.origin.x),@"y":@(r.origin.y),@"width":@(r.size.width),@"height":@(r.size.height)};
}
static NSDictionary *displays68(BOOL active) {
    CGDirectDisplayID ids[16]={0};uint32_t count=0;
    CGError status=active?CGGetActiveDisplayList(16,ids,&count):CGGetOnlineDisplayList(16,ids,&count);
    NSMutableArray *rows=[NSMutableArray array];
    if(status==kCGErrorSuccess)for(unsigned i=0;i<count&&i<16;i++) {
        CGDirectDisplayID d=ids[i];[rows addObject:@{@"id":@(d),@"online":@(CGDisplayIsOnline(d)),
            @"active":@(CGDisplayIsActive(d)),@"asleep":@(CGDisplayIsAsleep(d)),@"bounds":rect68(CGDisplayBounds(d))}];
    }
    return @{@"status":@(status),@"count":@(count),@"capacity":@16,@"rows":rows};
}
static void snapshot68(NSString *kind) {
    SecuritySessionId sid=0;SessionAttributeBits attrs=0;OSStatus status=SessionGetInfo(callerSecuritySession,&sid,&attrs);
    CFDictionaryRef session=CGSessionCopyCurrentDictionary();NSMutableDictionary *selected=[NSMutableDictionary dictionary];
    if(session)for(NSString *key in @[(__bridge NSString *)kCGSessionUserIDKey,(__bridge NSString *)kCGSessionOnConsoleKey,(__bridge NSString *)kCGSessionLoginDoneKey]) {
        id value=[(__bridge NSDictionary *)session objectForKey:key];selected[key]=value?:NSNull.null;
    }
    emit68(@{@"kind":kind,@"main_thread":@(NSThread.isMainThread),@"NSScreen_count":@(NSScreen.screens.count),
        @"mainScreen_available":@(NSScreen.mainScreen!=nil),@"active_displays":displays68(YES),@"online_displays":displays68(NO),
        @"CG_session_dictionary_available":@(session!=NULL),@"CG_session_selected_keys":selected,
        @"security_session_status":@(status),@"security_session":@(sid),@"security_attributes":@(attrs),
        @"graphic_access_flag":@((attrs&sessionHasGraphicAccess)!=0),@"NSApp_active":@(NSApp.active),
        @"NSApp_running":@(NSApp.running),@"windows":@(NSApp.windows.count),@"activation_requests":@0});
    if(session)CFRelease(session);
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;alarm(15);
    @autoreleasepool {
        NSString *root=[NSString stringWithUTF8String:argv[1]];
        SecuritySessionId sid=0;SessionAttributeBits attrs=0;OSStatus status=SessionGetInfo(callerSecuritySession,&sid,&attrs);
        emit68(@{@"kind":@"IDENTITY",@"pid":@(getpid()),@"ppid":@(getppid()),@"uid":@(getuid()),@"euid":@(geteuid()),
            @"NSThread_isMainThread":@(NSThread.isMainThread),@"pthread_main_np":@(pthread_main_np()),
            @"session_status":@(status),@"session":@(sid),@"session_attributes":@(attrs),@"permissions":permissions68(root),
            @"original_game_loaded":@NO,@"global_input_hooks":@0,@"screen_capture_calls":@0});
        emit68(@{@"kind":@"BEFORE_NSAPPLICATION"});
        [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
        snapshot68(@"BEFORE_STARTUP_EVENT_LOOP");
        emit68(@{@"kind":@"BEFORE_FINISH_LAUNCHING"});[NSApp finishLaunching];
        NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:0.35];unsigned iterations=0;
        while(deadline.timeIntervalSinceNow>0&&iterations<128) {
            [[NSRunLoop currentRunLoop]runMode:NSDefaultRunLoopMode beforeDate:deadline];iterations++;
        }
        emit68(@{@"kind":@"BOUNDED_STARTUP_LOOP_RETURNED",@"iterations":@(iterations),@"user_events_dispatched":@0});
        snapshot68(@"AFTER_STARTUP_EVENT_LOOP");
        return 0;
    }
}
