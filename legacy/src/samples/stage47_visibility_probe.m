#include "stage47_visibility.m"
#include <stdio.h>
#include <unistd.h>
@interface S47ProbePanel:NSPanel @end
@implementation S47ProbePanel
-(BOOL)canBecomeKeyWindow{return NO;}
-(BOOL)canBecomeMainWindow{return NO;}
@end
int main(void){@autoreleasepool{
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
 S47ProbePanel *win=[[S47ProbePanel alloc]initWithContentRect:NSMakeRect(250,200,320,240) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskNonactivatingPanel backing:NSBackingStoreBuffered defer:NO];win.title=@"Stage47 owned visibility probe";win.releasedWhenClosed=NO;win.hidesOnDeactivate=NO;win.floatingPanel=NO;
 if(!s47_watch((__bridge void*)win))return 2;[NSApp finishLaunching];[win orderFront:nil];
 NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:20];while(!NSRunningApplication.currentApplication.finishedLaunching&&deadline.timeIntervalSinceNow>0){NSEvent *event=[NSApp nextEventMatchingMask:NSEventMaskAppKitDefined untilDate:[NSDate dateWithTimeIntervalSinceNow:0.02] inMode:NSDefaultRunLoopMode dequeue:YES];if(event)[NSApp sendEvent:event];[NSApp updateWindows];if(NSApp.isActive||win.isKeyWindow)return 5;}
 if(!NSRunningApplication.currentApplication.finishedLaunching){puts("{\"finished_launching\":false,\"passed\":false}");[win close];return 3;}
 [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];[win orderFront:nil];[NSApp updateWindows];
 uint32_t front=(uint32_t)NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier;
 /* Deliberately hide this authored panel to exercise the diagnosed repair branch. */
 [win orderOut:nil];uint32_t v[12]={0},life[2]={0};int status=s47_recover((__bridge void*)win,0,v);BOOL stayedHidden=!win.isVisible;
 status&=s47_recover((__bridge void*)win,1,v);s47_lifecycle(life);
 BOOL passed=status&&stayedHidden&&v[0]==0&&v[3]==1&&v[4]==1&&v[5]==0&&v[6]==0&&v[8]==1&&v[9]==(uint32_t)getpid()&&v[11]==front&&front!=(uint32_t)getpid()&&life[0]==0;
 printf("{\"passed\":%s,\"authored_host_visibility_fault_injected\":true,\"guest_hidden_remains_hidden\":%s,\"recovery\":[",passed?"true":"false",stayedHidden?"true":"false");for(int i=0;i<12;i++)printf("%s%u",i?",":"",v[i]);printf("],\"close_notifications_before_cleanup\":%u,",life[0]);[win close];s47_lifecycle(life);printf("\"close_notifications_after_cleanup\":%u}\n",life[0]);return passed&&life[0]==1?0:4;
}}
