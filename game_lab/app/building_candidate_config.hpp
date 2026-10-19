#pragma once
#import <Foundation/Foundation.h>
#include "../original_building_candidate.hpp"
#include <charconv>
#include <cstring>
namespace game_lab {
inline NSString* candidateHex(uint64_t value){return [NSString stringWithFormat:@"%016llx",(unsigned long long)value];}
inline NSDictionary* encodeOriginalCandidate(const OriginalBuildingCandidate& c){
    if(!validOriginalCandidate(c))return nil;
    NSString* source=[NSString stringWithUTF8String:c.source.c_str()];NSString* archive=[NSString stringWithUTF8String:c.archive.c_str()];if(!source||!archive)return nil;
    return @{@"schema":@1,@"status":@"manual-unverified",@"anchor":@"canvas-bottom-centre-to-footprint-centre",
        @"source":source,@"sourceRevision":candidateHex(c.sourceRevision),
        @"archive":archive,@"physicalId":@(c.physicalId),@"imageRevision":candidateHex(c.imageRevision),
        @"slot":@(c.target.slot),@"recordOffset":@(c.target.recordOffset),@"type":@(c.target.type),@"x":@(c.target.x),@"y":@(c.target.y),@"size":@(c.target.size),
        @"width":@(originalFootprintWidth(c.target)),@"height":@(originalFootprintHeight(c.target))};
}
// Metadata only. Caller must reselect an unchanged saved snapshot anddecode the
// referenced original image before validOriginalCandidate can accept this.
inline std::optional<OriginalBuildingCandidate> decodeOriginalCandidate(id object){
    if(![object isKindOfClass:NSDictionary.class]||[object count]!=16)return std::nullopt;NSDictionary* d=object;
    const auto number=[&](NSString* key,uint64_t max)->std::optional<uint64_t>{id n=d[key];if(![n isKindOfClass:NSNumber.class]||CFGetTypeID((__bridge CFTypeRef)n)==CFBooleanGetTypeID()||CFNumberIsFloatType((__bridge CFNumberRef)n)||[n longLongValue]<0||[n unsignedLongLongValue]>max)return std::nullopt;return [n unsignedLongLongValue];};
    const auto text=[&](NSString* key)->std::optional<std::string>{id s=d[key];if(![s isKindOfClass:NSString.class]||[s length]>1024||![s UTF8String])return std::nullopt;NSData* bytes=[s dataUsingEncoding:NSUTF8StringEncoding];if(!bytes||bytes.length>4096||std::memchr(bytes.bytes,0,bytes.length))return std::nullopt;return std::string((const char*)bytes.bytes,bytes.length);};
    const auto hex=[&](NSString* key)->std::optional<uint64_t>{auto s=text(key);if(!s||s->size()!=16)return std::nullopt;uint64_t n=0;const auto v=std::from_chars(s->data(),s->data()+s->size(),n,16);if(v.ec!=std::errc{}||v.ptr!=s->data()+s->size())return std::nullopt;return n;};
    auto schema=number(@"schema",1),id=number(@"physicalId",1000000),slot=number(@"slot",3999),offset=number(@"recordOffset",8u*1024u*1024u-1),type=number(@"type",65535),x=number(@"x",227),y=number(@"y",227),size=number(@"size",8),width=number(@"width",8),height=number(@"height",8);
    auto status=text(@"status"),anchor=text(@"anchor"),source=text(@"source"),archive=text(@"archive");auto revision=hex(@"sourceRevision"),pixels=hex(@"imageRevision");
    if(!schema||*schema!=1||!id||!*id||!slot||!offset||!type||!x||!y||!size||!*size||!width||!*width||!height||!*height||!status||*status!="manual-unverified"||!anchor||*anchor!="canvas-bottom-centre-to-footprint-centre"||!source||!archive||!revision||!pixels||!relativeOriginalCandidatePath(*source)||!relativeOriginalCandidatePath(*archive))return std::nullopt;
    OriginalBuildingCandidate c;c.source=*source;c.archive=*archive;c.sourceRevision=*revision;c.imageRevision=*pixels;c.physicalId=uint32_t(*id);c.target.slot=*slot;c.target.recordOffset=*offset;c.target.type=uint16_t(*type);c.target.x=uint16_t(*x);c.target.y=uint16_t(*y);c.target.size=uint8_t(*size);c.target.width=uint8_t(*width);c.target.height=uint8_t(*height);return c;
}
}
