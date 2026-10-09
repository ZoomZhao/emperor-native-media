#import <Foundation/Foundation.h>
#import <VideoToolbox/VideoToolbox.h>
#include <iostream>
int main(int argc,char** argv){@autoreleasepool{if(argc!=2)return 2;CFArrayRef encoders=nullptr;
    const OSStatus status=VTCopyVideoEncoderList(nullptr,&encoders);NSMutableArray* entries=[NSMutableArray array];
    if(encoders)for(NSDictionary* item in (__bridge NSArray*)encoders){NSNumber* number=item[(__bridge id)kVTVideoEncoderList_CodecType];
        const uint32_t code=number.unsignedIntValue;const char tag[5]={char(code>>24),char(code>>16),char(code>>8),char(code),0};
        id name=item[(__bridge id)kVTVideoEncoderList_CodecName],identifier=item[(__bridge id)kVTVideoEncoderList_EncoderID],hardware=item[(__bridge id)kVTVideoEncoderList_IsHardwareAccelerated];
        [entries addObject:@{@"codec_type":@(code),@"fourcc":[NSString stringWithUTF8String:tag],@"codec_name":name?name:@"",
            @"encoder_id":identifier?identifier:@"",@"hardware":hardware?hardware:@NO}];}
    NSDictionary* d=@{@"OSStatus":@(status),@"encoder_count":@(entries.count),@"encoders":entries,@"encoder_sessions_tested":@NO};
    NSData* json=[NSJSONSerialization dataWithJSONObject:d options:NSJSONWritingPrettyPrinted error:nil];
    const BOOL wrote=[json writeToFile:[NSString stringWithUTF8String:argv[1]] atomically:YES];
    std::cout<<"VideoToolbox inventory status="<<status<<", count="<<entries.count<<"; enumeration only\n";
    if(encoders)CFRelease(encoders);return wrote?0:1;}}
