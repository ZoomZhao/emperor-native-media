// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#import <ImageIO/ImageIO.h>
#include "map_reference.hpp"
#include "cg_image_conversion.hpp"
#include "resource_root.hpp"
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <cmath>
#include <fstream>

@interface ReferenceImageView : NSView
@property(nonatomic,strong) NSImage* candidate;
@end
@implementation ReferenceImageView
- (void)drawRect:(NSRect)dirty { (void)dirty;[[NSColor colorWithWhite:0.18 alpha:1]setFill];NSRectFill(self.bounds);[NSGraphicsContext currentContext].imageInterpolation=NSImageInterpolationNone;[self.candidate drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}]; }
@end
@interface ReferenceDelegate : NSObject<NSApplicationDelegate>
@end
@implementation ReferenceDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app {(void)app;return YES;}
@end
namespace {
long long integer(const std::string&s){size_t n=0;auto v=std::stoll(s,&n,10);if(n!=s.size())throw std::runtime_error("whole integer required");return v;}
double real(const std::string&s){size_t n=0;auto v=std::stod(s,&n);if(n!=s.size())throw std::runtime_error("whole number required");return v;}
void savePNG(const std::string&path,const std::string&root,CGImageRef image){
    const auto target=game_lab::freshReferenceOutputs(root,{path}).front();
    NSURL*url=[NSURL fileURLWithPath:[NSString stringWithUTF8String:target.c_str()]];CGImageDestinationRef d=CGImageDestinationCreateWithURL((__bridge CFURLRef)url,CFSTR("public.png"),1,nullptr);if(!d)throw std::runtime_error("PNG destination failed");CGImageDestinationAddImage(d,image,nullptr);bool good=CGImageDestinationFinalize(d);CFRelease(d);if(!good)throw std::runtime_error("PNG finalization failed");
}
long long jsonInteger(id value,long long low,long long high) {
    if(![value isKindOfClass:[NSNumber class]] || CFGetTypeID((__bridge CFTypeRef)value)==CFBooleanGetTypeID())throw std::runtime_error("scene integer required");
    double n=[value doubleValue];if(!std::isfinite(n)||std::floor(n)!=n||n<low||n>high)throw std::runtime_error("scene integer bounds");
    return static_cast<long long>(n);
}
void keys(NSDictionary* value,NSArray* expected) {
    if(![[NSSet setWithArray:value.allKeys]isEqualToSet:[NSSet setWithArray:expected]])throw std::runtime_error("scene fields must be exact;no background/pixel input");
}
int diagnosticScene(const std::map<std::string,std::string>& args,bool show) {
    for(const auto&[key,value]:args){(void)value;if(key!="--data-root"&&key!="--scene-manifest"&&key!="--output"&&key!="--producer-output")throw std::runtime_error("scene mode only accepts root/manifest/output");}
    if(!args.count("--data-root"))throw std::runtime_error("explicit data root required");
    auto root=game_lab::ResourceRoot::fromAbsolute(args.at("--data-root"));if(!root.configured())throw std::runtime_error("existing absolute data root required");
    std::vector<std::string> requested;
    if(args.count("--output"))requested.push_back(args.at("--output"));
    if(args.count("--producer-output"))requested.push_back(args.at("--producer-output"));
    const auto outputs=game_lab::freshReferenceOutputs(root.configuredDirectory(),requested);
    const std::string pngOutput=args.count("--output")?outputs.front():std::string{};
    const std::string producerOutput=args.count("--producer-output")?outputs.back():std::string{};
    auto path=std::filesystem::path(args.at("--scene-manifest"));if(!path.is_absolute()||std::filesystem::file_size(path)>65536)throw std::runtime_error("bounded absolute scene manifest required");
    NSData* data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
    NSError* error=nil;id object=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:&error]:nil;
    if(error||![object isKindOfClass:[NSDictionary class]])throw std::runtime_error("scene JSON dictionary required");
    NSDictionary* doc=object;keys(doc,@[@"schema",@"diagnostic",@"archive",@"width",@"height",@"scale",@"layers"]);
    id diagnostic=doc[@"diagnostic"];
    if(![doc[@"schema"]isEqual:@"sg3-diagnostic-scene-v1"]||![diagnostic isKindOfClass:[NSNumber class]]||CFGetTypeID((__bridge CFTypeRef)diagnostic)!=CFBooleanGetTypeID()||![diagnostic boolValue])throw std::runtime_error("explicit diagnostic schema required");
    if(![doc[@"archive"]isKindOfClass:[NSString class]])throw std::runtime_error("relative SG3 archive required");
    const auto archivePath=root.resolve([doc[@"archive"]UTF8String]);if(archivePath.empty())throw std::runtime_error("scene archive outside data root or missing");
    game_lab::SG3DiagnosticScene scene;scene.width=uint32_t(jsonInteger(doc[@"width"],1,2048));scene.height=uint32_t(jsonInteger(doc[@"height"],1,2048));scene.scale=uint32_t(jsonInteger(doc[@"scale"],1,4));
    id layers=doc[@"layers"];if(![layers isKindOfClass:[NSArray class]]||[layers count]<1||[layers count]>128)throw std::runtime_error("scene needs1..128 explicit layers");
    for(id item in layers) {
        if(![item isKindOfClass:[NSDictionary class]])throw std::runtime_error("layer dictionary required");
        NSDictionary* layer=item;keys(layer,@[@"id",@"component",@"centre"]);
        game_lab::SG3DiagnosticLayer spec;spec.logicalID=uint32_t(jsonInteger(layer[@"id"],1,UINT32_MAX));
        id component=layer[@"component"];if([component isEqual:@"full"])spec.component=game_lab::SG3DiagnosticComponent::Full;
        else if([component isEqual:@"top"])spec.component=game_lab::SG3DiagnosticComponent::Top;else throw std::runtime_error("component must be full or natural top");
        id centre=layer[@"centre"];if(![centre isKindOfClass:[NSArray class]]||[centre count]!=2)throw std::runtime_error("layer centre needs2 integers");
        spec.centerX=double(jsonInteger(centre[0],-1000000,1000000));spec.centerY=double(jsonInteger(centre[1],-1000000,1000000));scene.layers.push_back(spec);
    }
    auto archive=game_lab::loadSG3(archivePath);auto internal=std::filesystem::path(archivePath);internal.replace_extension(".555");
    auto rendered=game_lab::renderSG3DiagnosticScene(archive,internal.string(),scene);if(!rendered.image.valid())throw std::runtime_error("diagnostic source/scene refused;no fallback image");
    CGImageRef cg=game_lab::createCGImageFromPremultipliedBGRA(scene.width,scene.height,rendered.image.pixelsBGRA);if(!cg)throw std::runtime_error("scene CGImage failed");
    if(!pngOutput.empty())savePNG(pngOutput,root.configuredDirectory(),cg);
    if(!producerOutput.empty()) {
        std::ofstream file(producerOutput,std::ios::binary);
        for(auto producer:rendered.producer){const char bytes[]={char(producer),char(producer>>8)};file.write(bytes,2);}
        file.close();if(!file)throw std::runtime_error("producer output failed");
    }
    std::cout<<"{\"diagnostic_scene\":true,\"native_image_created\":true,\"screenshot_inputs\":0,\"source_rule_confirmed\":false,\"decoded_layers\":"<<rendered.decodedLayers<<",\"covered\":"<<rendered.coveredPixels<<",\"unknown\":"<<rendered.image.pixelsBGRA.size()-rendered.coveredPixels<<",\"denominator\":"<<rendered.image.pixelsBGRA.size()<<",\"visible_per_layer\":[";
    for(size_t i=0;i<rendered.visiblePerLayer.size();++i){if(i)std::cout<<',';std::cout<<rendered.visiblePerLayer[i];}std::cout<<"]}\n"<<std::flush;
    if(!show){CGImageRelease(cg);return 0;}
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];ReferenceDelegate*delegate=[ReferenceDelegate new];NSApp.delegate=delegate;
    const CGFloat w=CGFloat(scene.width)/scene.scale,h=CGFloat(scene.height)/scene.scale;
    NSWindow*window=[[NSWindow alloc]initWithContentRect:NSMakeRect(80,80,w,h+54)styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];window.title=@"DIAGNOSTIC SG3 scene — unconfirmed source/order";
    NSTextField*label=[NSTextField wrappingLabelWithString:@"Explicit SG3 hypotheses; transparent pixels are unknown. No screenshot background; no MAP/SAV automatic selector."];label.frame=NSMakeRect(8,h+4,w-16,46);label.font=[NSFont systemFontOfSize:11];[window.contentView addSubview:label];
    ReferenceImageView*view=[[ReferenceImageView alloc]initWithFrame:NSMakeRect(0,0,w,h)];view.candidate=[[NSImage alloc]initWithCGImage:cg size:NSMakeSize(w,h)];[window.contentView addSubview:view];CGImageRelease(cg);[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];[NSApp run];return 0;
}
}
int main(int argc,char**argv){@autoreleasepool{try{
    std::map<std::string,std::string> args;bool show=true;
    for(int i=1;i<argc;++i){std::string key=argv[i];if(key=="--no-window"){show=false;continue;}if(key.rfind("--",0)!=0||i+1>=argc||args.count(key))throw std::runtime_error("unique named option/value required");args[key]=argv[++i];}
    if(args.count("--scene-manifest"))return diagnosticScene(args,show);
    for(const auto&k:{"--data-root","--map","--raw-offset","--raw-width","--sg3","--id-base","--camera-x","--camera-y","--width","--height","--scale"})if(!args.count(k))throw std::runtime_error("all raw/archive/base/camera options must be explicit");
    for(const auto&[k,v]:args){(void)v;if(k!="--data-root"&&k!="--map"&&k!="--raw-offset"&&k!="--raw-width"&&k!="--sg3"&&k!="--id-base"&&k!="--camera-x"&&k!="--camera-y"&&k!="--width"&&k!="--height"&&k!="--scale"&&k!="--output")throw std::runtime_error("unknown option");}
    auto root=game_lab::ResourceRoot::fromAbsolute(args["--data-root"]);if(!root.configured())throw std::runtime_error("existing absolute data root required");
    const auto offset=integer(args["--raw-offset"]),width=integer(args["--raw-width"]),vw=integer(args["--width"]),vh=integer(args["--height"]),scale=integer(args["--scale"]);
    if(offset<0||width<1||width>4||vw<1||vw>2048||vh<1||vh>2048||scale<1||scale>4)throw std::runtime_error("raw/camera integer bounds");
    const auto mapPath=root.resolve(args["--map"]),archivePath=root.resolve(args["--sg3"]);if(mapPath.empty()||archivePath.empty())throw std::runtime_error("relative map/archive path outside root or missing");
    auto map=game_lab::loadOriginalMapTerrain(mapPath,game_lab::OriginalMapRawSpec{size_t(offset),unsigned(width)});if(map.status!=game_lab::OriginalMapStatus::LoadedTerrain||!map.diagnosticRawLayer)throw std::runtime_error("complete MAP/raw descriptor unavailable");
    auto archive=game_lab::loadSG3(archivePath);if(!archive.numImages||archive.numImages!=archive.sprites.size())throw std::runtime_error("empty or invalid SG3 archive");auto internal=std::filesystem::path(archivePath);internal.replace_extension(".555");game_lab::OriginalMapCamera camera;camera.centerX=real(args["--camera-x"]);camera.centerY=real(args["--camera-y"]);camera.width=uint32_t(vw);camera.height=uint32_t(vh);camera.scale=uint32_t(scale);camera.outlines=false;
    const auto base=integer(args["--id-base"]);auto rendered=game_lab::renderMapReferenceViewport(map.terrain,*map.diagnosticRawLayer,archive,internal.string(),base,camera);if(!rendered.image.valid())throw std::runtime_error("experimental viewport rejected");
    CGImageRef cg=game_lab::createCGImageFromPremultipliedBGRA(rendered.image.width,rendered.image.height,rendered.image.pixelsBGRA);if(!cg)throw std::runtime_error("CGImage failed");const auto&s=rendered.stats;
    std::cout<<"{\"experimental\":true,\"raw_cells\":"<<map.diagnosticRawLayer->values.size()<<",\"active_cells\":"<<s.activeCells<<",\"valid_ID_cells\":"<<s.validIDs<<",\"geometry_supported_cells\":"<<s.geometrySupported<<",\"outside_archive\":"<<s.outsideArchive<<",\"unknown_geometry\":"<<s.unknownGeometry<<",\"culled\":"<<s.culled<<",\"visible_cells\":"<<s.visibleCells<<",\"drawn_cells\":"<<s.drawnCells<<",\"decode_failures\":"<<s.decodeFailures<<",\"unique_decoded_IDs\":"<<s.uniqueDecoded<<",\"cache_bytes\":"<<s.cacheBytes<<",\"native_image_created\":true,\"pixel_width\":"<<vw<<",\"pixel_height\":"<<vh<<",\"verified_full_city_joins\":0}\n"<<std::flush;
    if(args.count("--output"))savePNG(args["--output"],root.configuredDirectory(),cg);
    if(!show){CGImageRelease(cg);return 0;}
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];ReferenceDelegate*delegate=[ReferenceDelegate new];NSApp.delegate=delegate;
    const CGFloat displayW=CGFloat(vw)/scale,displayH=CGFloat(vh)/scale;NSWindow*window=[[NSWindow alloc]initWithContentRect:NSMakeRect(80,80,displayW,displayH+54)styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];window.title=@"EXPERIMENTAL — MAP raw reference candidate — original agreement unverified";
    NSTextField*label=[NSTextField wrappingLabelWithString:[NSString stringWithFormat:@"Experimental · explicit byte %lld / width %lld · %@ · ID base %lld\n%zu drawn / %zu visible; decoded IDs %zu. Flat bottom-base placement; source order/layers unverified; full city joins 0.",offset,width,[NSString stringWithUTF8String:args["--sg3"].c_str()],base,s.drawnCells,s.visibleCells,s.uniqueDecoded]];label.frame=NSMakeRect(8,displayH+4,displayW-16,46);label.font=[NSFont systemFontOfSize:11];label.accessibilityIdentifier=@"map-reference-info";[window.contentView addSubview:label];
    ReferenceImageView*view=[[ReferenceImageView alloc]initWithFrame:NSMakeRect(0,0,displayW,displayH)];view.candidate=[[NSImage alloc]initWithCGImage:cg size:NSMakeSize(displayW,displayH)];view.accessibilityIdentifier=@"map-reference-candidate";[window.contentView addSubview:view];CGImageRelease(cg);[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];[NSApp run];return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}}
