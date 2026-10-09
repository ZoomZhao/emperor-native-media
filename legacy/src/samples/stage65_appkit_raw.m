// Exact AppKit constructor prefix used by own adapter; no prior Security query.
#import <AppKit/AppKit.h>
#import <Security/AuthSession.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
static void emit65(NSDictionary *record) {
    NSData *body=[NSJSONSerialization dataWithJSONObject:record options:0 error:NULL];
    fwrite(body.bytes,1,body.length,stdout);fputc('\n',stdout);fflush(stdout);
}
int s65_construct(void) {
    @autoreleasepool {
        emit65(@{@"kind":@"BEFORE_RAW_NSAPPLICATION",@"pid":@(getpid()),@"ppid":@(getppid()),
                 @"uid":@(getuid()),@"euid":@(geteuid()),@"sid":@(getsid(0)),
                 @"NSThread_isMainThread":@(NSThread.isMainThread),@"pthread_main_np":@(pthread_main_np()),
                 @"prior_Security_query":@NO,@"game_loaded":@NO});
        if(!NSThread.isMainThread)return 2;
        NSApplication *app=[NSApplication sharedApplication];
        BOOL changed=[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
        emit65(@{@"kind":@"AFTER_RAW_NSAPPLICATION",@"created":@(app!=nil),
                 @"NSThread_isMainThread":@(NSThread.isMainThread),@"activation_policy":@(app.activationPolicy),
                 @"policy_change_succeeded":@(changed),@"active":@(app.active),@"running":@(app.running),
                 @"windows":@(app.windows.count),@"activation_requests":@0,@"event_loop_started":@NO});
        SecuritySessionId sid=0;SessionAttributeBits attrs=0;
        OSStatus status=SessionGetInfo(callerSecuritySession,&sid,&attrs);
        emit65(@{@"kind":@"SESSION_QUERY_AFTER_CONSTRUCTION",@"status":@(status),@"session":@(sid),
                 @"attributes":@(attrs),@"graphic_access":@((attrs&sessionHasGraphicAccess)!=0),
                 @"remote":@((attrs&sessionIsRemote)!=0)});
        return app?0:1;
    }
}
#ifndef S65_LIBRARY
int main(void){return s65_construct();}
#endif
