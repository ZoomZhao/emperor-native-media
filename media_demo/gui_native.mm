#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
NSString* root(){NSURL* executable=[NSURL fileURLWithPath:NSProcessInfo.processInfo.arguments.firstObject];
    return executable.URLByResolvingSymlinksInPath.URLByDeletingLastPathComponent.URLByDeletingLastPathComponent.path;}
NSString* evidence(){return [root() stringByAppendingPathComponent:@"evidence/gui-session"];}
NSString* bundle(){return [root() stringByAppendingPathComponent:@"build/NativeMediaDemo.app"];}
NSString* recordPath(){return [evidence() stringByAppendingPathComponent:@"active-instance.json"];}
void require(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
void json(NSDictionary* value,NSString* path=nil){NSError* error=nil;NSData* data=[NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:&error];
    require(data!=nil&&!error,"JSON encoding");if(path)require([data writeToFile:path atomically:YES],"JSON write");
    else{fwrite(data.bytes,1,data.length,stdout);fputc('\n',stdout);}}
void pump(double seconds){const auto until=std::chrono::steady_clock::now()+std::chrono::duration<double>(seconds);
    while(std::chrono::steady_clock::now()<until)[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.02]];}
void waitCompletion(dispatch_semaphore_t signal){const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(dispatch_semaphore_wait(signal,DISPATCH_TIME_NOW)){require(std::chrono::steady_clock::now()<until,"async GUI API timeout");pump(0.02);}}
void permissions(){require(AXIsProcessTrusted(),"Accessibility not authorized; stop without requesting it");}
id attribute(AXUIElementRef element,CFStringRef name){CFTypeRef value=nullptr;const auto status=AXUIElementCopyAttributeValue(element,name,&value);
    if(status!=kAXErrorSuccess)return nil;return CFBridgingRelease(value);}
NSString* text(AXUIElementRef element,CFStringRef name){id value=attribute(element,name);return [value isKindOfClass:NSString.class]?value:@"";}
void walk(AXUIElementRef element,NSMutableArray* elements,NSMutableArray* rows,unsigned depth=0){
    if(depth>16||elements.count>=200)return;[elements addObject:(__bridge id)element];
    NSMutableDictionary* row=[NSMutableDictionary dictionary];
    for(NSString* key in @[@"AXRole",@"AXTitle",@"AXValue",@"AXDescription",@"AXEnabled"]){
        id value=attribute(element,(__bridge CFStringRef)key);
        if([value isKindOfClass:NSString.class])row[key]=[value substringToIndex:std::min(NSUInteger(12000),[value length])];
        else if([value isKindOfClass:NSNumber.class])row[key]=value;
    }
    for(NSString* key in @[@"AXPosition",@"AXSize"]){id value=attribute(element,(__bridge CFStringRef)key);if(!value)continue;
        const auto ax=(__bridge AXValueRef)value;
        if(AXValueGetType(ax)==kAXValueCGPointType){CGPoint point{};AXValueGetValue(ax,static_cast<AXValueType>(kAXValueCGPointType),&point);row[key]=@[@(point.x),@(point.y)];}
        if(AXValueGetType(ax)==kAXValueCGSizeType){CGSize size{};AXValueGetValue(ax,static_cast<AXValueType>(kAXValueCGSizeType),&size);row[key]=@[@(size.width),@(size.height)];}
    }
    [rows addObject:row];NSArray* children=attribute(element,kAXChildrenAttribute);
    if([children isKindOfClass:NSArray.class])for(id child in children)walk((__bridge AXUIElementRef)child,elements,rows,depth+1);
}
NSRunningApplication* own(){NSError* error=nil;NSData* data=[NSData dataWithContentsOfFile:recordPath()];require(data!=nil,"no active owned launch record");
    NSDictionary* record=[NSJSONSerialization JSONObjectWithData:data options:0 error:&error];require(record&&!error,"launch record parse");
    auto* app=[NSRunningApplication runningApplicationWithProcessIdentifier:[record[@"pid"] intValue]];
    require(app&&!app.terminated,"owned test app is not running");
    require([app.bundleURL.path isEqualToString:bundle()]&&[app.bundleIdentifier isEqualToString:@"local.selfauthored.NativeMediaDemo"],"PID does not match owned bundle");
    require(std::abs(app.launchDate.timeIntervalSince1970-[record[@"launch_time"] doubleValue])<0.01,"PID launch time changed");return app;
}
AXUIElementRef window(NSRunningApplication* app){auto application=AXUIElementCreateApplication(app.processIdentifier);
    AXUIElementSetMessagingTimeout(application,3);NSArray* windows=attribute(application,kAXWindowsAttribute);CFRelease(application);
    require([windows isKindOfClass:NSArray.class]&&windows.count==1,"expected one owned demo window");
    auto result=(__bridge AXUIElementRef)windows.firstObject;require([text(result,kAXTitleAttribute) isEqualToString:@"静音媒体演示"],"owned window title mismatch");CFRetain(result);return result;
}
NSArray* inspect(NSString* name=nil){permissions();auto app=own();auto win=window(app);NSMutableArray* items=[NSMutableArray array];NSMutableArray* rows=[NSMutableArray array];
    walk(win,items,rows);CFRelease(win);NSDictionary* result=@{@"pid":@(app.processIdentifier),@"bundle":bundle(),@"window_elements":rows};
    if(name)json(result,[evidence() stringByAppendingPathComponent:[name stringByAppendingString:@".json"]]);else json(result);return rows;
}
AXUIElementRef find(AXUIElementRef win,NSString* role,NSString* title=nil){NSMutableArray* elements=[NSMutableArray array];NSMutableArray* rows=[NSMutableArray array];walk(win,elements,rows);
    for(id value in elements){auto element=(__bridge AXUIElementRef)value;
        if([text(element,kAXRoleAttribute) isEqualToString:role]&&(!title||[text(element,kAXTitleAttribute) isEqualToString:title])){CFRetain(element);return element;}}
    return nullptr;
}
NSString* status(){permissions();auto win=window(own());NSMutableArray* elements=[NSMutableArray array];NSMutableArray* rows=[NSMutableArray array];walk(win,elements,rows);CFRelease(win);
    for(NSDictionary* row in rows){NSString* value=row[@"AXValue"];if([row[@"AXRole"] isEqualToString:@"AXStaticText"]&&[value isKindOfClass:NSString.class]&&
        ([value containsString:@" · 帧 "]||[value hasPrefix:@"待机 · 未打开"]||[value isEqualToString:@"正在处理…"]))return value;}throw std::runtime_error("owned UI status missing");}
void waitState(NSString* state){for(unsigned i=0;i<250;++i){if([status() hasPrefix:state])return;pump(0.02);}throw std::runtime_error("UI state wait timeout");}
void press(NSString* title){permissions();auto app=own();auto win=window(app);auto button=find(win,@"AXButton",title);CFRelease(win);
    require(button!=nullptr,"owned button missing");NSNumber* enabled=attribute(button,kAXEnabledAttribute);
    for(unsigned i=0;i<150&&!enabled.boolValue;++i){pump(0.02);enabled=attribute(button,kAXEnabledAttribute);}
    require(enabled.boolValue,"owned button disabled");const auto result=AXUIElementPerformAction(button,kAXPressAction);CFRelease(button);
    require(result==kAXErrorSuccess,"owned button press failed");pump(0.12);json(@{@"action":@"AXPress",@"button":title,@"pid":@(app.processIdentifier),@"success":@YES});
}
void launch(NSString* reportName=nil,bool inject=false,NSString* fixture=nil){permissions();
    NSWorkspaceOpenConfiguration* config=NSWorkspaceOpenConfiguration.configuration;config.createsNewApplicationInstance=YES;config.activates=YES;
    if(reportName){require(reportName.length>0&&[reportName rangeOfCharacterFromSet:NSCharacterSet.alphanumericCharacterSet.invertedSet].location==NSNotFound,"report name must be alphanumeric");
        NSString* dir=[root() stringByAppendingPathComponent:fixture?@"evidence/reliability-gui":@"evidence/integration-gui"];require([NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil],"test report directory");
        NSMutableDictionary* environment=[@{@"NATIVE_MEDIA_DEMO_TEST_REPORT":[dir stringByAppendingPathComponent:[reportName stringByAppendingString:@".json"]],@"NATIVE_MEDIA_DEMO_TEST_EVENTS":inject?@"1":@"0"} mutableCopy];
        if(fixture){require([@[@"long",@"corrupt",@"truncated",@"normal",@"loading"] containsObject:fixture],"own reliability fixture tag only");
            environment[@"NATIVE_MEDIA_DEMO_TEST_FIXTURE"]=fixture;}
        if([fixture isEqualToString:@"long"])environment[@"NATIVE_MEDIA_DEMO_TEST_DELAY"]=@"1";
        config.environment=environment;}
    __block NSRunningApplication* launched=nil;__block NSError* failure=nil;dispatch_semaphore_t completed=dispatch_semaphore_create(0);
    [NSWorkspace.sharedWorkspace openApplicationAtURL:[NSURL fileURLWithPath:bundle()] configuration:config completionHandler:^(NSRunningApplication* app,NSError* error){
        launched=app;failure=error;dispatch_semaphore_signal(completed);}];
    waitCompletion(completed);require(!failure&&launched,"owned application launch failed/timed out");
    require([launched.bundleURL.path isEqualToString:bundle()],"unexpected launched bundle");
    NSDictionary* record=@{@"pid":@(launched.processIdentifier),@"launch_time":@(launched.launchDate.timeIntervalSince1970),@"bundle":bundle(),@"new_instance":@YES};
    json(record,recordPath());json(record,[evidence() stringByAppendingPathComponent:[NSString stringWithFormat:@"launch-%d.json",launched.processIdentifier]]);pump(0.8);json(record);
}
void seek(uint64_t target){permissions();auto app=own();auto win=window(app);auto slider=find(win,@"AXSlider");CFRelease(win);
    require(slider!=nullptr,"owned seek slider missing");CFArrayRef names=nullptr;
    NSNumber* maximum=attribute(slider,kAXMaxValueAttribute);require(maximum&&target<=maximum.unsignedLongLongValue,"seek outside owned slider range");
    const auto actionResult=AXUIElementCopyActionNames(slider,&names);NSArray* actions=names?CFBridgingRelease(names):@[];
    NSNumber* number=@(target);const auto result=AXUIElementSetAttributeValue(slider,kAXValueAttribute,(__bridge CFTypeRef)number);CFRelease(slider);
    require(result==kAXErrorSuccess,"owned slider AXValue set failed");pump(0.3);
    json(@{@"action":@"slider_AXValue",@"target_frame":@(target),@"pid":@(app.processIdentifier),@"attribute_set":@YES,
        @"supported_actions":actions,@"actions_query_code":@(actionResult),@"UI_status_after":status()});
}
void closeWindow(){permissions();auto app=own();auto win=window(app);id value=attribute(win,kAXCloseButtonAttribute);require(value!=nil,"close button missing");
    const auto result=AXUIElementPerformAction((__bridge AXUIElementRef)value,kAXPressAction);CFRelease(win);require(result==kAXErrorSuccess,"owned close press");
    for(unsigned i=0;i<500&&!app.terminated;++i)pump(0.02);require(app.terminated,"owned app did not exit after window close");
    json(@{@"action":@"window_close",@"pid":@(app.processIdentifier),@"terminated":@YES});
}
void resize(){permissions();auto app=own();auto win=window(app);const CGSize size{680,750};
    auto value=AXValueCreate(static_cast<AXValueType>(kAXValueCGSizeType),&size);require(value!=nullptr,"resize value");
    const auto result=AXUIElementSetAttributeValue(win,kAXSizeAttribute,value);CFRelease(value);CFRelease(win);
    require(result==kAXErrorSuccess,"owned window resize failed");pump(0.3);inspect(@"resized");json(@{@"action":@"window_resize",@"pid":@(app.processIdentifier),@"requested_size":@[@680,@750],@"success":@YES});
}
void verifyExited(){NSArray* files=[NSFileManager.defaultManager contentsOfDirectoryAtPath:evidence() error:nil];NSMutableArray* results=[NSMutableArray array];
    for(NSString* file in files){if(![file hasPrefix:@"launch-"]||![file hasSuffix:@".json"])continue;
        NSDictionary* record=[NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:[evidence() stringByAppendingPathComponent:file]] options:0 error:nil];
        auto app=[NSRunningApplication runningApplicationWithProcessIdentifier:[record[@"pid"] intValue]];
        const BOOL stillOwned=app&&!app.terminated&&[app.bundleURL.path isEqualToString:bundle()]&&std::abs(app.launchDate.timeIntervalSince1970-[record[@"launch_time"] doubleValue])<0.01;
        require(!stillOwned,"one of our launched demo instances remains running");[results addObject:@{@"pid":record[@"pid"],@"owned_instance_exited":@YES}];}
    json(@{@"all_test_instances_exited":@YES,@"instances":results,@"unrelated_processes_terminated":@NO},[evidence() stringByAppendingPathComponent:@"cleanup.json"]);
    json(@{@"all_test_instances_exited":@YES,@"instances":results});
}
void quit(){auto app=own();require([app terminate],"owned terminate request failed");
    for(unsigned i=0;i<500&&!app.terminated;++i)pump(0.02);
    if(!app.terminated){const bool forced=[app forceTerminate];json(@{@"action":@"cleanup",@"pid":@(app.processIdentifier),@"forced":@(forced)});throw std::runtime_error("owned app needed forced cleanup");}
    json(@{@"action":@"quit",@"pid":@(app.processIdentifier),@"terminated":@YES,@"forced":@NO});
}
void rapid(){
    if(![status() hasPrefix:@"待机"]){press(@"停止");waitState(@"待机");}
    for(uint64_t i=0;i<6;++i){press(@"播放");waitState(@"播放中");press(@"暂停");waitState(@"已暂停");
        seek(30+i*20);waitState(@"已暂停");press(@"播放");waitState(@"播放中");
        seek(12+i*10);waitState(@"播放中");press(@"停止");waitState(@"待机");}
    inspect(@"rapidFinal");json(@{@"scenario":@"rapid",@"cycles":@6,@"actions":@36,@"success":@YES,@"final_UI_status":status()});
}

}
int main(int argc,const char* argv[]){@autoreleasepool{try{
    // Establish the normal AppKit connection for scoped AX operations.
    // The helper owns no window and must never activate or appear as a GUI app.
    [NSApplication.sharedApplication setActivationPolicy:NSApplicationActivationPolicyProhibited];
    require(argc>=2,"mode required");NSError* error=nil;require([NSFileManager.defaultManager createDirectoryAtPath:evidence() withIntermediateDirectories:YES attributes:nil error:&error],"evidence directory");
    NSString* mode=[NSString stringWithUTF8String:argv[1]];
    if([mode isEqualToString:@"launch"])launch();
    else if([mode isEqualToString:@"launchIntegration"]){require(argc==3||argc==4,"report name required");launch([NSString stringWithUTF8String:argv[2]],argc==4&&std::string(argv[3])=="inject");}
    else if([mode isEqualToString:@"launchReliability"]){require(argc==4,"report name and fixture tag required");launch([NSString stringWithUTF8String:argv[2]],false,[NSString stringWithUTF8String:argv[3]]);}
    else if([mode isEqualToString:@"inspect"])inspect(argc==3?[NSString stringWithUTF8String:argv[2]]:nil);
    else if([mode isEqualToString:@"play"])press(@"播放");else if([mode isEqualToString:@"pause"])press(@"暂停");else if([mode isEqualToString:@"stop"])press(@"停止");
    else if([mode isEqualToString:@"seek"]){require(argc==3,"seek frame required");seek(std::stoull(argv[2]));}
    else if([mode isEqualToString:@"rapid"])rapid();
    else if([mode isEqualToString:@"resize"])resize();else if([mode isEqualToString:@"verifyExited"])verifyExited();
    else if([mode isEqualToString:@"waitEnded"]){waitState(@"播放结束");inspect(@"eof");json(@{@"action":@"wait_EOF",@"UI_status":status(),@"success":@YES});}
    else if([mode isEqualToString:@"close"])closeWindow();else if([mode isEqualToString:@"quit"])quit();else throw std::runtime_error("unknown mode");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}}}
