// Own minimal main-thread AppKit construction. No windows, game data, or adapters.
#import <AppKit/AppKit.h>
#import <Security/AuthSession.h>
#include <bsm/audit.h>
#include <errno.h>
#include <libproc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void emit(NSDictionary *record) {
    NSData *body=[NSJSONSerialization dataWithJSONObject:record options:0 error:NULL];
    fwrite(body.bytes,1,body.length,stdout);fputc('\n',stdout);fflush(stdout);
}
int main(void) {
    @autoreleasepool {
        emit(@{@"kind":@"IDENTITY",@"pid":@(getpid()),@"ppid":@(getppid()),
               @"uid":@(getuid()),@"euid":@(geteuid()),@"gid":@(getgid()),
               @"sid":@(getsid(0)),@"NSThread_isMainThread":@(NSThread.isMainThread),
               @"pthread_main_np":@(pthread_main_np()),@"isatty_stdin":@(isatty(0)),
               @"isatty_stdout":@(isatty(1)),@"original_game_loaded":@NO});
        char own[PROC_PIDPATHINFO_MAXSIZE]={0},parent[PROC_PIDPATHINFO_MAXSIZE]={0};
        errno=0;int own_status=proc_pidpath(getpid(),own,sizeof(own));int own_errno=errno;
        errno=0;int parent_status=proc_pidpath(getppid(),parent,sizeof(parent));int parent_errno=errno;
        emit(@{@"kind":@"OWN_PROCESS_PATHS",@"own_status":@(own_status),@"own_errno":@(own_errno),
               @"own_path":[NSString stringWithUTF8String:own],@"parent_status":@(parent_status),
               @"parent_errno":@(parent_errno),@"parent_path":[NSString stringWithUTF8String:parent]});
        struct auditinfo_addr info={0};errno=0;int audit_status=getaudit_addr(&info,sizeof(info));int audit_errno=errno;
        emit(@{@"kind":@"OWN_AUDIT_SESSION",@"status":@(audit_status),@"errno":@(audit_errno),
               @"audit_uid":@(info.ai_auid),@"audit_session_id":@(info.ai_asid),
               @"audit_session_flags":@(info.ai_flags)});
        SecuritySessionId session=0;SessionAttributeBits attributes=0;
        OSStatus status=SessionGetInfo(callerSecuritySession,&session,&attributes);
        emit(@{@"kind":@"OWN_SECURITY_SESSION",@"status":@(status),@"session":@(session),
               @"attributes":@(attributes),@"graphic_access":@((attributes&sessionHasGraphicAccess)!=0),
               @"has_tty":@((attributes&sessionHasTTY)!=0),@"remote":@((attributes&sessionIsRemote)!=0),
               @"root_session":@((attributes&sessionIsRoot)!=0)});
        emit(@{@"kind":@"BEFORE_NSAPPLICATION",@"NSThread_isMainThread":@(NSThread.isMainThread)});
        NSApplication *application=[NSApplication sharedApplication];
        emit(@{@"kind":@"AFTER_NSAPPLICATION",@"created":@(application!=nil),
               @"NSThread_isMainThread":@(NSThread.isMainThread),@"activation_policy":@(application.activationPolicy),
               @"active":@(application.active),@"running":@(application.running),@"windows":@(application.windows.count),
               @"activation_requests":@0,@"event_loop_started":@NO});
        return application?0:1;
    }
}
