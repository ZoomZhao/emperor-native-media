#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <iostream>
int main(int argc,char** argv){@autoreleasepool{if(argc!=3)return 2;
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();NSArray<id<MTLDevice>>* devices=MTLCopyAllDevices();
    NSMutableArray* names=[NSMutableArray array];for(id<MTLDevice> entry in devices)[names addObject:entry.name];
    struct utsname machine{};const int unameStatus=uname(&machine);const NSOperatingSystemVersion os=NSProcessInfo.processInfo.operatingSystemVersion;
    NSDictionary* d=@{@"requested_context_label":[NSString stringWithUTF8String:argv[2]],@"default_device_present":@(device!=nil),
        @"default_device_name":device?device.name:@"",@"enumerated_device_count":@(devices.count),@"enumerated_device_names":names,
        @"pid":@(getpid()),@"uid":@(getuid()),@"effective_uid":@(geteuid()),@"uname_status":@(unameStatus),@"architecture":[NSString stringWithUTF8String:machine.machine],
        @"macOS_major":@(os.majorVersion),@"macOS_minor":@(os.minorVersion),@"macOS_patch":@(os.patchVersion),
        @"command_buffers_created":@0,@"GPU_resources_allocated":@0,@"window_or_audio_device_used":@NO,@"settings_changed":@NO};
    NSData* json=[NSJSONSerialization dataWithJSONObject:d options:NSJSONWritingPrettyPrinted error:nil];
    const BOOL written=[json writeToFile:[NSString stringWithUTF8String:argv[1]] atomically:YES];
    std::cout<<argv[2]<<": default="<<(device?device.name.UTF8String:"nil")<<", enumerated="<<devices.count<<", uid="<<getuid()<<"/"<<geteuid()<<"\n";return written?0:1;}}
