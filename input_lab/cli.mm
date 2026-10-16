// SPDX-License-Identifier: MIT
#include "final_dispatch.hpp"
#include <iostream>
#include <limits>
using namespace scoped_input;
static double number(NSDictionary* object, NSString* key) {
    id value=object[key];
    require([value isKindOfClass:NSNumber.class] && CFGetTypeID((__bridge CFTypeRef)value)!=CFBooleanGetTypeID(),"numeric request field required");
    double n=[value doubleValue]; require(std::isfinite(n),"finite request field required"); return n;
}
static uint64_t integer(NSDictionary* object,NSString* key,uint64_t maximum) {
    double n=number(object,key);
    require(n>=0 && n<=double(maximum) && std::floor(n)==n,"integer request field out of range");
    return uint64_t(n);
}
int main(int argc,char** argv) { @autoreleasepool {
    try {
        bool inspect = argc == 3 && std::string(argv[1]) == "--inspect";
        bool finalRoad=argc==3 && std::string(argv[1])=="--final-road";
        require(argc==2 || inspect || finalRoad,"usage: scoped-click [--inspect|--final-road] /absolute/local/request.json");
        if(finalRoad)refuseUnsupportedFinalRoad(); // Refuse before file reads, TCC, snapshots or event allocation.
        NSString* path=[NSString stringWithUTF8String:argv[inspect?2:1]];
        require(path.isAbsolutePath,"absolute request path required");
        NSData* bytes=[NSData dataWithContentsOfFile:path];
        require(bytes && bytes.length<=8192,"request unavailable or excessive");
        id value=[NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil];
        require([value isKindOfClass:NSDictionary.class],"JSON request object required");
        NSDictionary* object=value; id executable=object[@"executable"];
        require([executable isKindOfClass:NSString.class] && [executable length]>0,"executable request field required");
        Request r;
        r.owner={pid_t(integer(object,@"pid",std::numeric_limits<pid_t>::max())),
            uid_t(integer(object,@"uid",std::numeric_limits<uid_t>::max())),
            integer(object,@"birth_seconds",9007199254740991ULL),
            integer(object,@"birth_micros",999999),[executable UTF8String]};
        r.window=CGWindowID(integer(object,@"window_id",std::numeric_limits<CGWindowID>::max()));
        r.capturedFrame=CGRectMake(number(object,@"frame_x"),number(object,@"frame_y"),
            number(object,@"frame_width"),number(object,@"frame_height"));
        r.normalizedX=number(object,@"normalized_x"); r.normalizedY=number(object,@"normalized_y");
        r.deadlineUnixSeconds=number(object,@"deadline_unix_seconds");
        // No launching, activation, key events, permission requests or game paths.
        // Only an identified system Dock window may supply an in-memory alpha witness.
        [NSApplication.sharedApplication setActivationPolicy:NSApplicationActivationPolicyProhibited];
        if (inspect) {
            NSData* data=[NSJSONSerialization dataWithJSONObject:inspectionJSON(r,currentSnapshot(r))
                options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
            require(data!=nil,"inspection JSON failed");
            std::cout.write((const char*)data.bytes,data.length); std::cout << '\n'; return 0;
        }
        CGPoint point=postClick(r);
        NSDictionary* result=@{@"input_sent":@YES,@"visible_transition_verified":@NO,
            @"pid":@(r.owner.pid),@"window_id":@(r.window),@"point":@[@(point.x),@(point.y)]};
        NSData* data=[NSJSONSerialization dataWithJSONObject:result options:0 error:nil];
        std::cout.write((const char*)data.bytes,data.length); std::cout << '\n'; return 0;
    } catch (const std::exception& error) {std::cerr << error.what() << '\n'; return 1;}
}}
