// SPDX-License-Identifier: MIT
#include "scoped_click.hpp"
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <ImageIO/ImageIO.h>
#include <iostream>
using namespace scoped_input;
static NSString* directory;
static NSString* file(NSString* name) {return [directory stringByAppendingPathComponent:name];}
static void writeJSON(NSDictionary* value, NSString* name) {
    NSData* data = [NSJSONSerialization dataWithJSONObject:value
        options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
    require(data && [data writeToFile:file(name) atomically:YES], "evidence write failed");
}
static void pump(double seconds) {
    auto until = std::chrono::steady_clock::now()+std::chrono::duration<double>(seconds);
    while (std::chrono::steady_clock::now()<until)
        [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.02]];
}
static NSArray* records() {
    NSString* text = [NSString stringWithContentsOfFile:file(@"events.jsonl") encoding:NSUTF8StringEncoding error:nil];
    NSMutableArray* values = [NSMutableArray array];
    for (NSString* line in [text componentsSeparatedByString:@"\n"]) {
        if (!line.length) continue;
        id value = [NSJSONSerialization JSONObjectWithData:[line dataUsingEncoding:NSUTF8StringEncoding] options:0 error:nil];
        require([value isKindOfClass:NSDictionary.class], "malformed fixture event"); [values addObject:value];
    }
    return values;
}
static NSUInteger countKind(NSString* kind) {
    NSUInteger n=0; for (NSDictionary* row in records()) if ([row[@"kind"] isEqual:kind]) ++n; return n;
}
static id attribute(AXUIElementRef element, CFStringRef key) {
    CFTypeRef value=nullptr;
    if (AXUIElementCopyAttributeValue(element,key,&value)!=kAXErrorSuccess) return nil;
    return CFBridgingRelease(value);
}
static AXUIElementRef findButton(AXUIElementRef element, unsigned depth=0) {
    if (depth>12) return nullptr;
    if ([attribute(element,kAXIdentifierAttribute) isEqual:@"probe-button"]) {CFRetain(element); return element;}
    for (id child in attribute(element,kAXChildrenAttribute)) {
        auto found=findButton((__bridge AXUIElementRef)child,depth+1); if (found) return found;
    }
    return nullptr;
}
static SCShareableContent* content() {
    __block SCShareableContent* result=nil; __block NSError* failure=nil;
    auto signal=dispatch_semaphore_create(0);
    [SCShareableContent getShareableContentExcludingDesktopWindows:YES onScreenWindowsOnly:YES
        completionHandler:^(SCShareableContent* value,NSError* error) {
        result=value; failure=error; dispatch_semaphore_signal(signal);
    }];
    auto until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
    while (dispatch_semaphore_wait(signal,DISPATCH_TIME_NOW)) {
        require(std::chrono::steady_clock::now()<until,"window enumeration timeout"); pump(.02);
    }
    require(result && !failure,"window enumeration failed"); return result;
}
struct FixtureProcess {
    NSTask* task=[NSTask new];
    ~FixtureProcess() {
        [@"stop" writeToFile:file(@"command.txt") atomically:YES encoding:NSUTF8StringEncoding error:nil];
        for (unsigned i=0;i<100 && task.running;++i) pump(.03);
        if (task.running) std::cerr << "fixture normal exit pending; 60s automatic stop remains\n";
    }
};
int main(int argc,char** argv) { @autoreleasepool {
    if (argc!=4) return 2;
    directory=[NSString stringWithUTF8String:argv[2]];
    [NSApplication.sharedApplication setActivationPolicy:NSApplicationActivationPolicyProhibited];
    if (!(AXIsProcessTrusted() && CGPreflightPostEventAccess() && CGPreflightScreenCaptureAccess())) {
        writeJSON(@{@"outcome":@"SKIP",@"reason":@"existing UI permissions unavailable",
            @"permissions_requested":@NO,@"fixture_launched":@NO},@"result.json");
        std::cout << "SKIP: existing UI permissions unavailable; no requests or launch\n"; return 0;
    }
    try {
        require(![NSFileManager.defaultManager fileExistsAtPath:file(@"events.jsonl")],"fresh output directory required");
        require([NSFileManager.defaultManager createFileAtPath:file(@"events.jsonl") contents:nil attributes:nil],"event log creation failed");
        FixtureProcess fixture;
        fixture.task.executableURL=[NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]];
        fixture.task.arguments=@[directory];
        require([fixture.task launchAndReturnError:nil],"fixture launch failed");
        Identity owner=processIdentity(fixture.task.processIdentifier);
        writeJSON(@{@"pid":@(owner.pid),@"uid":@(owner.uid),@"birth_seconds":@(owner.birthSeconds),
            @"birth_micros":@(owner.birthMicros),@"executable":[NSString stringWithUTF8String:owner.executable.c_str()]},@"owner.json");
        NSRunningApplication* app=nil;
        auto until=std::chrono::steady_clock::now()+std::chrono::seconds(6);
        while ((!app || countKind(@"ready")==0) && std::chrono::steady_clock::now()<until) {
            pump(.1); app=[NSRunningApplication runningApplicationWithProcessIdentifier:owner.pid];
        }
        require(app && countKind(@"ready")==1 && [app activateWithOptions:0],"fixture readiness or activation failed");
        pump(.3);
        SCWindow* window=nil;
        for (SCWindow* candidate in content().windows)
            if (candidate.owningApplication.processID==owner.pid && [candidate.title isEqual:@"Codex Authored Input Fixture"])
                window=candidate;
        require(window!=nil,"authored window absent");
        auto root=AXUIElementCreateApplication(owner.pid); auto button=findButton(root); CFRelease(root);
        require(button!=nullptr,"authored button absent");
        CGPoint position{}; CGSize size{};
        id pos=attribute(button,kAXPositionAttribute),dim=attribute(button,kAXSizeAttribute);
        require(pos && dim && AXValueGetValue((__bridge AXValueRef)pos,(AXValueType)kAXValueCGPointType,&position) &&
            AXValueGetValue((__bridge AXValueRef)dim,(AXValueType)kAXValueCGSizeType,&size),"AX bounds unavailable");
        CGPoint centre=CGPointMake(position.x+size.width/2,position.y+size.height/2);
        Request request{owner,window.windowID,window.frame,
            (centre.x-window.frame.origin.x)/window.frame.size.width,
            (centre.y-window.frame.origin.y)/window.frame.size.height,NSDate.date.timeIntervalSince1970+30};
        require(CGPointEqualToPoint(target(request),centre),"AX and normalized target differ");
        auto filter=[[SCContentFilter alloc] initWithDesktopIndependentWindow:window];
        NSTask* shot=[NSTask new]; shot.executableURL=[NSURL fileURLWithPath:@"/usr/sbin/screencapture"];
        shot.arguments=@[@"-x",@"-o",[NSString stringWithFormat:@"-l%u",request.window],file(@"own-window.png")];
        require([shot launchAndReturnError:nil],"own screenshot launch failed");
        for (unsigned i=0;i<150 && shot.running;++i) pump(.02);
        require(!shot.running && shot.terminationStatus==0,"own screenshot failed");
        auto source=CGImageSourceCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:file(@"own-window.png")],nullptr);
        require(source!=nullptr,"own image unavailable");
        NSDictionary* properties=CFBridgingRelease(CGImageSourceCopyPropertiesAtIndex(source,0,nullptr)); CFRelease(source);
        double width=[properties[(__bridge NSString*)kCGImagePropertyPixelWidth] doubleValue];
        double height=[properties[(__bridge NSString*)kCGImagePropertyPixelHeight] doubleValue];
        require(width==window.frame.size.width*filter.pointPixelScale && height==window.frame.size.height*filter.pointPixelScale,
            "point/pixel scale mismatch");
        writeJSON(@{@"frame_points":@[@(window.frame.origin.x),@(window.frame.origin.y),@(window.frame.size.width),@(window.frame.size.height)],
            @"pixel_size":@[@(width),@(height)],@"scale":@(filter.pointPixelScale),
            @"target_points":@[@(centre.x),@(centre.y)],@"target_pixels":@[@(request.normalizedX*width),@(request.normalizedY*height)],
            @"window_id":@(request.window)},@"geometry.json");
        NSMutableArray* pidCases=[NSMutableArray array];
        for (unsigned mode=0;mode<3;++mode) {
            auto snapshot=currentSnapshot(request);
            writeJSON(@{@"expected_pid":@(owner.pid),@"expected_window":@(request.window),
                @"hit_pid":@(snapshot.hitPid),@"hit_window":@(snapshot.hitWindow),
                @"active":@(snapshot.active),@"frontmost_pid":@(snapshot.frontmostPid),
                @"hit_layer":@(snapshot.hitLayer),@"hit_frame":@[@(snapshot.hitFrame.origin.x),
                    @(snapshot.hitFrame.origin.y),@(snapshot.hitFrame.size.width),@(snapshot.hitFrame.size.height)]},@"hit-diagnostic.json");
            validate(request,snapshot); // Applies to diagnostic PID path too.
            NSUInteger before=countKind(@"action");
            Event down(CGEventCreateMouseEvent(nullptr,kCGEventLeftMouseDown,centre,kCGMouseButtonLeft));
            Event up(CGEventCreateMouseEvent(nullptr,kCGEventLeftMouseUp,centre,kCGMouseButtonLeft));
            require(down && up,"PID event allocation failed");
            for (CGEventRef event:{down.get(),up.get()}) {
                CGEventSetIntegerValueField(event,kCGMouseEventWindowUnderMousePointer,request.window);
                if (mode) CGEventSetIntegerValueField(event,kCGMouseEventClickState,1);
                if (mode==2) CGEventSetIntegerValueField(event,kCGMouseEventWindowUnderMousePointerThatCanHandleThisEvent,request.window);
            }
            auto downCount=CGEventGetIntegerValueField(down.get(),kCGMouseEventClickState);
            auto upCount=CGEventGetIntegerValueField(up.get(),kCGMouseEventClickState);
            CGEventPostToPid(owner.pid,down.get()); pump(mode?.25:.06); CGEventPostToPid(owner.pid,up.get()); pump(.3);
            [pidCases addObject:@{@"variant":@(mode),@"actions":@(countKind(@"action")-before),
                @"click_counts":@[@(downCount),@(upCount)]}];
        }
        unsigned rejections=0;
        auto rejects=[&](Request bad,const char* reason) {
            NSUInteger before=countKind(@"mouse"); bool rejected=false;
            try {postClick(bad);} catch (const std::runtime_error& error) {rejected=std::string(error.what())==reason;}
            pump(.1); require(rejected && countKind(@"mouse")==before,"guard failed or emitted mouse event"); ++rejections;
        };
        Request bad=request; bad.owner.birthMicros++; rejects(bad,"owner identity changed");
        bad=request; bad.deadlineUnixSeconds=NSDate.date.timeIntervalSince1970-1; rejects(bad,"expired or excessive deadline");
        bad=request; bad.capturedFrame.origin.x++; rejects(bad,"captured window moved or resized");
        [@"cover" writeToFile:file(@"command.txt") atomically:YES encoding:NSUTF8StringEncoding error:nil];
        for (unsigned i=0;i<50 && countKind(@"cover")==0;++i) pump(.02);
        require(countKind(@"cover")==1,"cover readiness failed"); pump(.1);
        rejects(request,"selected point obscured");
        [@"clear" writeToFile:file(@"command.txt") atomically:YES encoding:NSUTF8StringEncoding error:nil];
        for (unsigned i=0;i<50 && countKind(@"clear")==0;++i) pump(.02);
        require(countKind(@"clear")==1,"cover did not clear"); pump(.1);
        NSUInteger before=countKind(@"action"), first=records().count;
        writeJSON(@{@"pid":@(owner.pid),@"uid":@(owner.uid),@"birth_seconds":@(owner.birthSeconds),
            @"birth_micros":@(owner.birthMicros),@"executable":[NSString stringWithUTF8String:owner.executable.c_str()],
            @"window_id":@(request.window),@"frame_x":@(request.capturedFrame.origin.x),
            @"frame_y":@(request.capturedFrame.origin.y),@"frame_width":@(request.capturedFrame.size.width),
            @"frame_height":@(request.capturedFrame.size.height),@"normalized_x":@(request.normalizedX),
            @"normalized_y":@(request.normalizedY),@"deadline_unix_seconds":@(request.deadlineUnixSeconds)},@"request.json");
        NSTask* sender=[NSTask new]; sender.executableURL=[NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[3]]];
        sender.arguments=@[@"--inspect",file(@"request.json")];
        [NSFileManager.defaultManager createFileAtPath:file(@"inspection.json") contents:nil attributes:nil];
        NSFileHandle* inspectionOutput=[NSFileHandle fileHandleForWritingAtPath:file(@"inspection.json")];
        sender.standardOutput=inspectionOutput;
        require([sender launchAndReturnError:nil],"shared inspection launch failed");
        for (unsigned i=0;i<100 && sender.running;++i) pump(.03);
        require(!sender.running && sender.terminationStatus==0,"shared inspection failed");
        [inspectionOutput closeFile];
        NSDictionary* inspection=[NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:file(@"inspection.json")] options:0 error:nil];
        require([inspection[@"read_only"] boolValue] && ![inspection[@"input_sent"] boolValue] &&
            [inspection[@"selected_window_id"] unsignedIntValue]==request.window &&
            [inspection[@"selected_alpha"] isKindOfClass:NSNumber.class] &&
            [inspection[@"candidate_alpha"] isKindOfClass:NSNumber.class] &&
            [inspection[@"public_display_shield_window_id"] unsignedIntValue]==0 &&
            countKind(@"action")==before,"inspection fabricated metadata or submitted input");
        sender=[NSTask new]; sender.executableURL=[NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[3]]];
        sender.arguments=@[file(@"request.json")];
        require([sender launchAndReturnError:nil],"shared CLI launch failed");
        for (unsigned i=0;i<100 && sender.running;++i) pump(.03);
        require(!sender.running && sender.terminationStatus==0,"shared CLI submission failed"); pump(.3);
        require(countKind(@"action")==before+1,"guarded session click did not trigger exactly one action");
        NSMutableArray* pair=[NSMutableArray array]; NSArray* all=records();
        for (NSUInteger i=first;i<all.count;++i) if ([all[i][@"kind"] isEqual:@"mouse"]) [pair addObject:all[i]];
        require(pair.count==2,"guarded session mouse pair absent");
        CGPoint local=CGPointMake(centre.x-window.frame.origin.x,CGRectGetMaxY(window.frame)-centre.y);
        for (NSDictionary* row in pair) require([row[@"window_number"] unsignedIntValue]==request.window &&
            [row[@"click_count"] intValue]==1 && [row[@"active"] boolValue] &&
            [row[@"location_in_window"][0] doubleValue]==local.x && [row[@"location_in_window"][1] doubleValue]==local.y,
            "session event geometry or routing mismatch");
        require([pair[0][@"combined_button_down"] boolValue] && ![pair[1][@"combined_button_down"] boolValue],"session button state mismatch");
        before=countKind(@"action"); require(AXUIElementPerformAction(button,kAXPressAction)==kAXErrorSuccess,"AX control failed");
        CFRelease(button); pump(.1); require(countKind(@"action")==before+1,"AX control action absent");
        [@"stop" writeToFile:file(@"command.txt") atomically:YES encoding:NSUTF8StringEncoding error:nil];
        for (unsigned i=0;i<100 && fixture.task.running;++i) pump(.03);
        require(!fixture.task.running && fixture.task.terminationStatus==0,"fixture did not exit normally0");
        writeJSON(@{@"outcome":@"PASS",@"pid_diagnostics":pidCases,@"live_guard_rejections":@(rejections),
            @"rejection_events":@0,@"session_actions":@1,@"session_window_events":@2,@"AX_actions":@1,
            @"normal_exit_status":@0,@"forced_signals":@0,@"Wine_verified":@NO,@"inspection_passed":@YES},@"result.json");
        std::cout << "PASS: guarded session 1/1, four live guard rejections emitted zero events; Wine unverified\n";
        return 0;
    } catch (const std::exception& error) {
        writeJSON(@{@"outcome":@"FAIL",@"error":[NSString stringWithUTF8String:error.what()]},@"result.json");
        std::cerr << error.what() << '\n'; return 1;
    }
}}
