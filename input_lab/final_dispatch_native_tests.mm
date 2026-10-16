// SPDX-License-Identifier: MIT
// Standalone UBSan host: no system-Python sanitizer loading or UI operations.
#include "final_dispatch.hpp"
#include <iostream>
#include <cstring>
extern "C" int run_dry_final(const char*,int(*)(double,double,void*,int64_t*),int(*)(double,double,void*),int64_t(*)(void*),void*,char*,size_t);
using namespace scoped_input;
struct State {std::string kind;unsigned gates=0,sends=0,fakeEvents=0;int64_t tick=1400000000;};
static int gate(double x,double y,void* user,int64_t* requested) {
    auto& s=*static_cast<State*>(user);++s.gates;
    if(x!=530 || y!=489 || s.kind=="gate_first" || (s.kind=="gate_second" && s.gates==2))return 1;
    *requested=s.kind=="replaced_preview" && s.gates==2?1310000000:1300000000;
    if(s.kind=="late" && s.gates==2)s.tick=1550000001;
    if(s.kind=="dock_expiry" && s.gates==2)s.tick=1400000001;
    return 0;
}
static int send(double x,double y,void* user) {
    auto& s=*static_cast<State*>(user);++s.sends;
    if(x!=530 || y!=489)return 1;
    ++s.fakeEvents;if(s.kind=="sender_partial")return 1;++s.fakeEvents;return 0;
}
static int64_t clock(void* user) {auto& s=*static_cast<State*>(user);return s.kind=="clock_missing"?-1:s.tick;}
int main() { @autoreleasepool {
    try {
        unsigned passed=0;NSMutableArray* results=[NSMutableArray new];
        for(const char* name:{"positive","gate_first","gate_second","late","clock_missing","focus","frame","owner","deadline","replaced_preview","wrong_point","dock_expiry","late_focus","late_window","late_frame","late_occlusion","sender_partial"}) {
            State state{name};
            NSMutableDictionary* request=[@{@"pid":@123,@"uid":@501,@"birth_seconds":@42,@"birth_micros":@1234,@"executable":@"/synthetic/Fixture",@"window_id":@77,
                @"frame_x":@220,@"frame_y":@242,@"frame_width":@640,@"frame_height":@448,@"normalized_x":@0.484375,@"normalized_y":@(247.0/448.0)}mutableCopy];
            if(state.kind=="wrong_point")request[@"normalized_x"]=@0.5;
            NSDictionary* command=@{@"mode":@"dry-run",@"request":request,@"native_fault":[NSString stringWithUTF8String:name]};
            NSData* json=[NSJSONSerialization dataWithJSONObject:command options:0 error:nil];
            std::string text((const char*)json.bytes,json.length);char output[16384]{};
            int code=run_dry_final(text.c_str(),gate,send,clock,&state,output,sizeof(output));
            NSDictionary* report=[NSJSONSerialization JSONObjectWithData:[NSData dataWithBytes:output length:std::strlen(output)]options:0 error:nil];
            require(report!=nil && [report[@"native_event_posts"]intValue]==0,"bad native dry-run report");
            if(state.kind=="positive")require(code==0 && state.gates==2 && state.sends==1 && state.fakeEvents==2,"positive dispatch count failed");
            else if(state.kind=="sender_partial")require(code==1 && state.sends==1 && state.fakeEvents==1 && [report[@"status"]isEqual:@"uncertain_after_sender_entry"],"partial failure overclaimed");
            else require(code==1 && state.sends==0 && state.fakeEvents==0,"refusal invoked fake sender");
            if(state.kind.rfind("late_",0)==0)require([report[@"native_snapshot_reads"]intValue]==2,"latest snapshot was not read");
            [results addObject:@{@"case":[NSString stringWithUTF8String:name],@"gates":@(state.gates),@"sender_calls":@(state.sends),@"fake_events":@(state.fakeEvents)}];++passed;
        }
        NSDictionary* output=@{@"passed":@YES,@"mode":@"undefined",@"scope":@"standalone native boundary; Python FFI remains Release only",@"authored_cases_passed":@[@(passed),@17],@"cases":results,@"OS_event_posts":@0,@"captures":@0,@"original_game_launches":@0};
        NSData* bytes=[NSJSONSerialization dataWithJSONObject:output options:NSJSONWritingSortedKeys error:nil];
        std::cout.write((const char*)bytes.bytes,bytes.length);std::cout<<'\n';return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}}
