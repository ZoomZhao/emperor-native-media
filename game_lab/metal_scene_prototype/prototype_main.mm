// SPDX-License-Identifier: MIT
// Offscreen CPU-vs-Metal verification for authored sprite scenes.
// Exit codes: 0 every required check passed; 1 a required GPU path differs or is
// unavailable; 2 no usable Metal device/base shader (missing capability, not a
// pass); 3 the CPU reference failed its independent oracle; 4 usage error.
#include "metal_sprite_renderer.hpp"
#include "scenes.hpp"
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace msp;
namespace {
struct Diff { size_t pixels=0;size_t channel[4]={0,0,0,0};uint32_t maxAbs[4]={0,0,0,0};size_t firstIndex=SIZE_MAX; };
Diff compare(const std::vector<uint32_t>& a,const std::vector<uint32_t>& b){
    Diff d;
    for(size_t i=0;i<a.size()&&i<b.size();++i){if(a[i]==b[i])continue;++d.pixels;if(d.firstIndex==SIZE_MAX)d.firstIndex=i;
        for(unsigned c=0;c<4;++c){const int x=int(channel(a[i],c*8)),y=int(channel(b[i],c*8));
            if(x!=y){++d.channel[c];d.maxAbs[c]=std::max(d.maxAbs[c],uint32_t(std::abs(x-y)));}}}
    if(a.size()!=b.size())d.pixels=SIZE_MAX;
    return d;
}
size_t checkExpected(const AuthoredScene& s,const std::vector<uint32_t>& pixels,const char* who,bool verbose){
    size_t bad=0;
    for(const auto& e:s.expected){const uint32_t got=pixels[size_t(e.y)*s.scene.width+e.x];
        if(got!=e.pixel){++bad;if(verbose)printf("    %s expectation (%u,%u) %s: want %08x got %08x\n",who,e.x,e.y,e.what,e.pixel,got);}}
    return bad;
}
double median(std::vector<double> v){if(v.empty())return 0;std::sort(v.begin(),v.end());return v[v.size()/2];}

// CoreGraphics colorspace probe: draws authored opaque ramps from a DeviceRGB
// CGImage (the App's current conversion convention) into bitmap contexts of
// other colorspaces and counts changed bytes. Offscreen evidence only: it does
// not show what a window, CAMetalLayer or display finally emits.
void colorspaceProbe(){
    const uint32_t w=256,h=4;std::vector<uint32_t> src;
    for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x)src.push_back(y==0?bgra(x,x,x,255):y==1?bgra(0,0,x,255):y==2?bgra(0,x,0,255):bgra(x,0,0,255));
    CGColorSpaceRef device=CGColorSpaceCreateDeviceRGB();
    CFStringRef deviceName=CGColorSpaceCopyName(device);
    CFDataRef icc=CGColorSpaceCopyICCData(device);
    printf("colorspace_probe: DeviceRGB name=%s icc_bytes=%ld\n",deviceName?[(__bridge NSString*)deviceName UTF8String]:"(unnamed)",icc?long(CFDataGetLength(icc)):0L);
    if(deviceName)CFRelease(deviceName);if(icc)CFRelease(icc);
    CGDataProviderRef provider=CGDataProviderCreateWithData(nullptr,src.data(),src.size()*4,nullptr);
    CGImageRef image=CGImageCreate(w,h,8,32,w*4,device,kCGBitmapByteOrder32Little|kCGImageAlphaPremultipliedFirst,provider,nullptr,false,kCGRenderingIntentDefault);
    const std::pair<const char*,CGColorSpaceRef> targets[]={
        {"DeviceRGB",CGColorSpaceRetain(device)},{"sRGB",CGColorSpaceCreateWithName(kCGColorSpaceSRGB)},
        {"DisplayP3",CGColorSpaceCreateWithName(kCGColorSpaceDisplayP3)},{"LinearSRGB",CGColorSpaceCreateWithName(kCGColorSpaceLinearSRGB)},
        {"MainDisplay",CGDisplayCopyColorSpace(CGMainDisplayID())}};
    for(const auto& [name,space]:targets){
        if(!space){printf("  %-11s unavailable\n",name);continue;}
        std::vector<uint32_t> dst(src.size(),0);
        CGContextRef context=CGBitmapContextCreate(dst.data(),w,h,8,w*4,space,kCGBitmapByteOrder32Little|kCGImageAlphaPremultipliedFirst);
        if(!context){printf("  %-11s context unavailable\n",name);CGColorSpaceRelease(space);continue;}
        CGContextSetBlendMode(context,kCGBlendModeCopy);CGContextSetInterpolationQuality(context,kCGInterpolationNone);
        CGContextDrawImage(context,CGRectMake(0,0,w,h),image);CGContextRelease(context);
        const auto d=compare(src,dst);
        CFStringRef spaceName=CGColorSpaceCopyName(space);
        printf("  DeviceRGB->%-11s (%s): changed pixels %zu/%u, channel B/G/R %zu/%zu/%zu, max|d| %u/%u/%u\n",name,
            spaceName?[(__bridge NSString*)spaceName UTF8String]:"unnamed",d.pixels,w*h,d.channel[0],d.channel[1],d.channel[2],d.maxAbs[0],d.maxAbs[1],d.maxAbs[2]);
        if(spaceName)CFRelease(spaceName);CGColorSpaceRelease(space);
    }
    CGImageRelease(image);CGDataProviderRelease(provider);CGColorSpaceRelease(device);
}
}

int main(int argc,char** argv){
    @autoreleasepool {
    bool requireFixed=true,requireProgrammable=true,verbose=false;unsigned iterations=20;
    for(int i=1;i<argc;++i){
        if(!strcmp(argv[i],"--require=both")){requireFixed=requireProgrammable=true;}
        else if(!strcmp(argv[i],"--require=fixed")){requireFixed=true;requireProgrammable=false;}
        else if(!strcmp(argv[i],"--require=programmable")){requireFixed=false;requireProgrammable=true;}
        else if(!strncmp(argv[i],"--iterations=",13)){iterations=unsigned(atoi(argv[i]+13));if(!iterations||iterations>1000)return 4;}
        else if(!strcmp(argv[i],"--verbose"))verbose=true;
        else{fprintf(stderr,"usage: %s [--require=both|fixed|programmable] [--iterations=N] [--verbose]\n",argv[0]);return 4;}
    }
    printf("metal_scene_prototype: authored scenes only; offscreen readback; not an App display test\n");
    printf("os: %s\n",NSProcessInfo.processInfo.operatingSystemVersionString.UTF8String);

    std::vector<AuthoredScene> scenes;
    scenes.push_back(paintOrderScene());scenes.push_back(alphaExhaustiveScene());
    for(uint32_t scale=1;scale<=4;++scale)scenes.push_back(scaleClipScene(scale));
    scenes.push_back(stressScene());

    // CPU reference self-checks against independent oracles first.
    bool cpuOk=true;std::vector<std::vector<uint32_t>> cpu;
    for(const auto& s:scenes){
        if(const auto invalid=validateScene(s.scene);!invalid.empty()){printf("cpu %s: invalid scene %s\n",s.scene.name.c_str(),invalid.c_str());return 3;}
        cpu.push_back(cpuCompose(s.scene));
        const size_t hand=checkExpected(s,cpu.back(),"cpu",true);
        const size_t oracle=s.oracle?s.oracle(cpu.back()):0;
        printf("cpu %-18s %ux%u scale%u draws %zu: hand %zu/%zu ok%s\n",s.scene.name.c_str(),s.scene.width,s.scene.height,s.scene.scale,
            s.scene.items.size(),s.expected.size()-hand,s.expected.size(),
            s.oracle?(std::string(", formula oracle channel mismatches ")+std::to_string(oracle)+"/"+std::to_string(cpu.back().size()*4)).c_str():"");
        cpuOk&=hand==0&&oracle==0;
    }
    {Scene bad;bad.width=bad.height=1;bad.initial={0};bad.sprites={{1,1,{bgra(200,0,0,100)}}};bad.items={{0,0,0,0}};
     const bool rejected=!validateScene(bad).empty();
     printf("cpu invalid-premultiplied input rejected: %s (CPU formula would carry: 200+%u=%u > 255)\n",rejected?"yes":"NO",(255u*155u+127u)/255u,200u+(255u*155u+127u)/255u);
     cpuOk&=rejected;}
    if(!cpuOk){printf("RESULT cpu_reference=FAIL exit=3\n");return 3;}

    colorspaceProbe();

    MetalSpriteRenderer renderer;const auto report=renderer.report();
    printf("device: %s name=\"%s\" registry=%llu unified=%d lowPower=%d removable=%d workingSet=%llu\n",report.device?"yes":"no",
        report.name.c_str(),report.registryId,report.unifiedMemory,report.lowPower,report.removable,report.recommendedWorkingSet);
    std::string families;for(const auto& f:report.families)families+=f+" ";
    printf("families: %s\nshader base compile: %s\nshader programmable compile: %s\n",families.c_str(),report.baseCompile.c_str(),report.programmableCompile.c_str());
    if(!renderer.available()){printf("RESULT metal=MISSING_CAPABILITY (%s) exit=2\n",report.reason.c_str());return 2;}

    struct Variant {BlendPath path;TargetFormat format;bool control;};
    const Variant variants[]={{BlendPath::FixedFunction,TargetFormat::BGRA8Unorm,false},{BlendPath::Programmable,TargetFormat::BGRA8Unorm,false},
        {BlendPath::FixedFunction,TargetFormat::BGRA8UnormSRGB,true}};
    bool fixedOk=true,programmableOk=true,controlDetects=true;
    for(const auto& v:variants){
        std::string why;
        if(!renderer.pathAvailable(v.path,v.format,why)){
            printf("variant %s/%s: UNAVAILABLE %s\n",blendPathName(v.path),targetFormatName(v.format),why.c_str());
            if(!v.control)(v.path==BlendPath::FixedFunction?fixedOk:programmableOk)=false;else controlDetects=false;
            continue;
        }
        for(size_t i=0;i<scenes.size();++i){
            const auto& s=scenes[i];
            // The sRGB control only needs scenes with blending to show detection power.
            if(v.control&&s.scene.name!="alpha_exhaustive"&&s.scene.name!="paint_order")continue;
            const auto gpu=renderer.render(s.scene,v.path,v.format,1);
            if(!gpu.ok){printf("variant %s/%s %s: GPU ERROR %s\n",blendPathName(v.path),targetFormatName(v.format),s.scene.name.c_str(),gpu.error.c_str());
                if(!v.control)(v.path==BlendPath::FixedFunction?fixedOk:programmableOk)=false;continue;}
            const auto d=compare(cpu[i],gpu.pixels);const size_t hand=checkExpected(s,gpu.pixels,"gpu",verbose&&!v.control);
            const size_t oracle=s.oracle?s.oracle(gpu.pixels):0;const auto& t=gpu.iterations.front();
            const bool exact=d.pixels==0&&hand==0&&oracle==0;
            printf("%s %s/%s %-18s diff_pixels %zu/%zu channels B/G/R/A %zu/%zu/%zu/%zu max|d| %u/%u/%u/%u hand %zu/%zu%s | upload %.3fms reset %.3fms encode %.3fms gpu %.3fms wait %.3fms readback %.3fms\n",
                v.control?"control":(exact?"PASS":"FAIL"),blendPathName(v.path),targetFormatName(v.format),s.scene.name.c_str(),
                d.pixels,cpu[i].size(),d.channel[0],d.channel[1],d.channel[2],d.channel[3],d.maxAbs[0],d.maxAbs[1],d.maxAbs[2],d.maxAbs[3],
                s.expected.size()-hand,s.expected.size(),s.oracle?(std::string(" oracle_mismatch ")+std::to_string(oracle)).c_str():"",
                gpu.uploadMs,t.targetResetMs,t.encodeMs,t.gpuMs,t.waitMs,t.readbackMs);
            if(!exact&&d.firstIndex!=SIZE_MAX&&(verbose||!v.control)){
                const uint32_t x=uint32_t(d.firstIndex%s.scene.width),y=uint32_t(d.firstIndex/s.scene.width);
                printf("    first diff (%u,%u): cpu %08x gpu %08x%s%s\n",x,y,cpu[i][d.firstIndex],gpu.pixels[d.firstIndex],
                    s.describe?" ":"",s.describe?s.describe(x,y).c_str():"");
            }
            if(v.control)controlDetects&=d.pixels>0;
            else if(!exact)(v.path==BlendPath::FixedFunction?fixedOk:programmableOk)=false;
        }
    }
    // Repeated stress timing for each available non-control path; textures are
    // uploaded once, then target reset/encode/GPU/wait/readback per iteration.
    const auto& stress=scenes.back();
    for(BlendPath path:{BlendPath::FixedFunction,BlendPath::Programmable}){
        std::string why;if(!renderer.pathAvailable(path,TargetFormat::BGRA8Unorm,why))continue;
        const auto gpu=renderer.render(stress.scene,path,TargetFormat::BGRA8Unorm,iterations);if(!gpu.ok)continue;
        std::vector<double> reset,encode,gpuMs,wait,readback;
        for(const auto& t:gpu.iterations){reset.push_back(t.targetResetMs);encode.push_back(t.encodeMs);gpuMs.push_back(t.gpuMs);wait.push_back(t.waitMs);readback.push_back(t.readbackMs);}
        const auto d=compare(cpu.back(),gpu.pixels);
        printf("timing %s %s draws %zu iterations %u (median ms): upload_once %.3f (%zu bytes) reset %.3f encode %.3f gpu %.3f wait %.3f readback %.3f; last diff_pixels %zu\n",
            blendPathName(path),stress.scene.name.c_str(),gpu.draws,iterations,gpu.uploadMs,gpu.uploadBytes,median(reset),median(encode),median(gpuMs),median(wait),median(readback),d.pixels);
    }
    const bool controlOk=controlDetects;
    const bool requiredOk=(!requireFixed||fixedOk)&&(!requireProgrammable||programmableOk);
    printf("RESULT fixed_function=%s programmable=%s srgb_control_detects=%s required=%s%s exit=%d\n",fixedOk?"EXACT":"DIFFERS_OR_UNAVAILABLE",
        programmableOk?"EXACT":"DIFFERS_OR_UNAVAILABLE",controlOk?"yes":"NO",requireFixed&&requireProgrammable?"both":requireFixed?"fixed":"programmable",
        controlOk?"":" (control failed to detect sRGB change: comparison lacks power)",requiredOk&&controlOk?0:1);
    return requiredOk&&controlOk?0:1;
    }
}
