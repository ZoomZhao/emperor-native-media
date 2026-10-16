// SPDX-License-Identifier: MIT
// In-process authored dry-run bridge. No OS snapshots or event posting here.
#include "final_dispatch.hpp"
#include <cstring>
#include <limits>
using namespace scoped_input;
using GateCallback=int(*)(double,double,void*,int64_t*);
using SendCallback=int(*)(double,double,void*);
using ClockCallback=int64_t(*)(void*);
static double numeric(NSDictionary* o,NSString* k) {
    id v=o[k];require([v isKindOfClass:NSNumber.class] && CFGetTypeID((__bridge CFTypeRef)v)!=CFBooleanGetTypeID(),"numeric fixture field required");
    double n=[v doubleValue];require(std::isfinite(n),"finite fixture field required");return n;
}
static uint64_t integer(NSDictionary* o,NSString* k,uint64_t bound) {
    double n=numeric(o,k);require(n>=0 && n<=double(bound) && std::floor(n)==n,"integer fixture field required");return uint64_t(n);
}
extern "C" int run_dry_final(const char* json,GateCallback check,SendCallback emit,ClockCallback clock,void* user,char* output,size_t capacity) {
    @autoreleasepool {
        unsigned gateCalls=0,senderCalls=0,snapshotReads=0;std::string reason;bool passed=false;
        try {
            require(json && check && emit && clock,"complete dry-run callbacks required");
            const size_t length=strnlen(json,8193);require(length<=8192,"fixture command budget");
            NSData* bytes=[NSData dataWithBytes:json length:length];
            id value=[NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil];
            require([value isKindOfClass:NSDictionary.class],"fixture object required");
            NSDictionary* object=value;require([object[@"mode"] isEqual:@"dry-run"],"only authored dry-run is supported");
            NSDictionary* f=object[@"request"];require([f isKindOfClass:NSDictionary.class],"fixture request required");
            NSString* path=f[@"executable"];require([path isKindOfClass:NSString.class],"fixture executable required");
            Request r;
            r.owner={pid_t(integer(f,@"pid",std::numeric_limits<pid_t>::max())),uid_t(integer(f,@"uid",std::numeric_limits<uid_t>::max())),
                integer(f,@"birth_seconds",9007199254740991ULL),integer(f,@"birth_micros",999999),path.UTF8String};
            r.window=CGWindowID(integer(f,@"window_id",std::numeric_limits<CGWindowID>::max()));
            r.capturedFrame=CGRectMake(numeric(f,@"frame_x"),numeric(f,@"frame_y"),numeric(f,@"frame_width"),numeric(f,@"frame_height"));
            r.normalizedX=numeric(f,@"normalized_x");r.normalizedY=numeric(f,@"normalized_y");r.deadlineUnixSeconds=1100;
            Snapshot s{r.owner,r.owner.uid,true,true,false,r.owner.pid,r.owner.pid,r.owner.pid,r.window,r.window,r.capturedFrame,1000};
            s.hitFrame=r.capturedFrame;s.candidateOwnerPid=r.owner.pid;
            NSString* fault=object[@"native_fault"];
            auto snapshot=[&] {
                ++snapshotReads;
                if(gateCalls>=2) {
                    if([fault isEqual:@"late_focus"])s.active=false;
                    if([fault isEqual:@"late_window"])s.window=r.window+1;
                    if([fault isEqual:@"late_frame"])s.frame.origin.x+=1;
                    if([fault isEqual:@"late_occlusion"])s.hitWindow=r.window+1;
                }
                if([fault isEqual:@"focus"])s.active=false;
                if([fault isEqual:@"frame"])s.frame.origin.x+=1;
                if([fault isEqual:@"owner"])s.owner.birthMicros+=1;
                if([fault isEqual:@"deadline"])s.nowUnixSeconds=1100;
                if([fault isEqual:@"dock_expiry"]) {
                    DockWitness witness;witness.captureRequestedAt=DockClock::time_point(std::chrono::nanoseconds(1150000000));s.transparentDock=witness;
                }
                return s;
            };
            auto now=[&] {const int64_t tick=clock(user);require(tick>0,"clock unavailable");return DockClock::time_point(std::chrono::nanoseconds(tick));};
            dispatchFinalChecked(r,snapshot,[&](CGPoint p) {
                ++gateCalls;int64_t requested=0;
                require(check(p.x,p.y,user,&requested)==0 && requested>0,"final validator refused");
                return FinalReceipt{p,DockClock::time_point(std::chrono::nanoseconds(requested))};
            },[&](CGPoint p) {
                ++senderCalls;require(emit(p.x,p.y,user)==0,"sender callback failed after entry; do not retry");
            },now);
            passed=true;
        }catch(const std::exception& error) {reason=error.what();}
        NSDictionary* report=@{@"passed":@(passed),@"mode":@"dry-run",@"gate_calls":@(gateCalls),@"sender_callback_calls":@(senderCalls),@"native_snapshot_reads":@(snapshotReads),
            @"status":passed?@"completed":senderCalls?@"uncertain_after_sender_entry":@"refused_before_sender",
            @"reason":[NSString stringWithUTF8String:reason.c_str()],@"native_event_posts":@0};
        NSData* result=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingSortedKeys error:nil];
        if(!result || !output || capacity<=result.length)return 2;
        std::memcpy(output,result.bytes,result.length);output[result.length]=0;return passed?0:1;
    }
}
