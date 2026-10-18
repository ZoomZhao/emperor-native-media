// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#import <ImageIO/ImageIO.h>
#include "map_reference.hpp"
#include "original_save.hpp"
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
void writePNG(game_lab::ExclusiveReferenceOutputs& outputs,size_t index,CGImageRef image) {
    NSMutableData* bytes=[NSMutableData data];
    CGImageDestinationRef destination=CGImageDestinationCreateWithData((__bridge CFMutableDataRef)bytes,CFSTR("public.png"),1,nullptr);
    if(!destination)throw std::runtime_error("PNG destination failed");
    CGImageDestinationAddImage(destination,image,nullptr);
    const bool good=CGImageDestinationFinalize(destination);CFRelease(destination);
    if(!good)throw std::runtime_error("PNG finalization failed");
    outputs.write(index,static_cast<const uint8_t*>(bytes.bytes),bytes.length);
}
void savePNG(const std::string& path,const std::string& root,CGImageRef image) {
    game_lab::ExclusiveReferenceOutputs outputs(root,{path});writePNG(outputs,0,image);outputs.commit();
}
long long jsonInteger(id value,long long low,long long high) {
    if(![value isKindOfClass:[NSNumber class]] || CFGetTypeID((__bridge CFTypeRef)value)==CFBooleanGetTypeID())throw std::runtime_error("scene integer required");
    double n=[value doubleValue];if(!std::isfinite(n)||std::floor(n)!=n||n<low||n>high)throw std::runtime_error("scene integer bounds");
    return static_cast<long long>(n);
}
void keys(NSDictionary* value,NSArray* expected) {
    if(![[NSSet setWithArray:value.allKeys]isEqualToSet:[NSSet setWithArray:expected]])throw std::runtime_error("scene fields must be exact;no background/pixel input");
}
std::string sceneRelativePath(id value) {
    if(![value isKindOfClass:[NSString class]])throw std::runtime_error("root-relative scene path required");
    NSData* utf8=[value dataUsingEncoding:NSUTF8StringEncoding];
    if(!utf8 || !utf8.length)return {};
    // Preserve embedded NUL so ResourceRoot rejects it rather than accepting a
    // truncated prefix through NSString's null-terminated UTF8String accessor.
    return std::string(static_cast<const char*>(utf8.bytes),utf8.length);
}
int diagnosticScene(const std::map<std::string,std::string>& args,bool show) {
    for(const auto&[key,value]:args){(void)value;if(key!="--data-root"&&key!="--scene-manifest"&&key!="--output"&&key!="--producer-output")throw std::runtime_error("scene mode only accepts root/manifest/output");}
    if(!args.count("--data-root"))throw std::runtime_error("explicit data root required");
    auto root=game_lab::ResourceRoot::fromAbsolute(args.at("--data-root"));if(!root.configured())throw std::runtime_error("existing absolute data root required");
    std::vector<std::string> requested;
    if(args.count("--output"))requested.push_back(args.at("--output"));
    if(args.count("--producer-output"))requested.push_back(args.at("--producer-output"));
    auto path=std::filesystem::path(args.at("--scene-manifest"));if(!path.is_absolute()||std::filesystem::file_size(path)>65536)throw std::runtime_error("bounded absolute scene manifest required");
    NSData* data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
    NSError* error=nil;id object=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:&error]:nil;
    if(error||![object isKindOfClass:[NSDictionary class]])throw std::runtime_error("scene JSON dictionary required");
    NSDictionary* doc=object;
    const bool savedCells=[doc[@"schema"]isEqual:@"sg3-diagnostic-sav-scene-v1"];
    if(savedCells)keys(doc,@[@"schema",@"diagnostic",@"archive",@"width",@"height",@"scale",@"save",@"registration",@"layers"]);
    else keys(doc,@[@"schema",@"diagnostic",@"archive",@"width",@"height",@"scale",@"layers"]);
    id diagnostic=doc[@"diagnostic"];
    if((!savedCells && ![doc[@"schema"]isEqual:@"sg3-diagnostic-scene-v1"])||![diagnostic isKindOfClass:[NSNumber class]]||CFGetTypeID((__bridge CFTypeRef)diagnostic)!=CFBooleanGetTypeID()||![diagnostic boolValue])throw std::runtime_error("explicit diagnostic schema required");
    const auto archivePath=root.resolve(sceneRelativePath(doc[@"archive"]));if(archivePath.empty())throw std::runtime_error("scene archive outside data root or missing");
    game_lab::OriginalSaveResult saved;
    game_lab::SG3DiagnosticRegistration registration;
    if(savedCells) {
        const auto savePath=root.resolve(sceneRelativePath(doc[@"save"]));if(savePath.empty())throw std::runtime_error("scene SAV outside data root or missing");
        saved=game_lab::loadOriginalSave(savePath);
        if(saved.status!=game_lab::OriginalSaveStatus::LoadedSnapshot || !saved.readStable || !saved.layers.valid())throw std::runtime_error("complete stable SAV snapshot required");
        id value=doc[@"registration"];if(![value isKindOfClass:[NSDictionary class]])throw std::runtime_error("registration dictionary required");
        NSDictionary* reg=value;keys(reg,@[@"matrix",@"canvas_origin"]);
        id matrix=reg[@"matrix"];if(![matrix isKindOfClass:[NSArray class]]||[matrix count]!=2)throw std::runtime_error("registration matrix needs2 rows");
        for(size_t row=0;row<2;++row) {
            id entries=matrix[row];if(![entries isKindOfClass:[NSArray class]]||[entries count]!=3)throw std::runtime_error("registration matrix needs3 columns");
            for(size_t col=0;col<3;++col)registration.matrix[row][col]=jsonInteger(entries[col],col==2?-1000000:-4096,col==2?1000000:4096);
        }
        id origin=reg[@"canvas_origin"];if(![origin isKindOfClass:[NSArray class]]||[origin count]!=2)throw std::runtime_error("canvas origin needs2 integers");
        for(size_t row=0;row<2;++row)registration.canvasOrigin[row]=jsonInteger(origin[row],-1000000,1000000);
    }
    game_lab::SG3DiagnosticScene scene;scene.width=uint32_t(jsonInteger(doc[@"width"],1,2048));scene.height=uint32_t(jsonInteger(doc[@"height"],1,2048));scene.scale=uint32_t(jsonInteger(doc[@"scale"],1,4));
    id layers=doc[@"layers"];if(![layers isKindOfClass:[NSArray class]]||[layers count]<1||[layers count]>128)throw std::runtime_error("scene needs1..128 explicit layers");
    for(id item in layers) {
        if(![item isKindOfClass:[NSDictionary class]])throw std::runtime_error("layer dictionary required");
        NSDictionary* layer=item;keys(layer,savedCells?@[@"id",@"component",@"cell"]:@[@"id",@"component",@"centre"]);
        game_lab::SG3DiagnosticLayer spec;spec.logicalID=uint32_t(jsonInteger(layer[@"id"],1,UINT32_MAX));
        id component=layer[@"component"];if([component isEqual:@"full"])spec.component=game_lab::SG3DiagnosticComponent::Full;
        else if([component isEqual:@"top"])spec.component=game_lab::SG3DiagnosticComponent::Top;else throw std::runtime_error("component must be full or natural top");
        if(savedCells) {
            id cell=layer[@"cell"];if(![cell isKindOfClass:[NSArray class]]||[cell count]!=2)throw std::runtime_error("layer cell needs2 integers");
            const auto point=game_lab::projectSG3DiagnosticCell(saved.terrain,registration,jsonInteger(cell[0],0,227),jsonInteger(cell[1],0,227));
            if(!point)throw std::runtime_error("unsupported registration or SAV cell outside active on-map crop");
            spec.centerX=point->x;spec.centerY=point->y;
        }else {
            id centre=layer[@"centre"];if(![centre isKindOfClass:[NSArray class]]||[centre count]!=2)throw std::runtime_error("layer centre needs2 integers");
            spec.centerX=double(jsonInteger(centre[0],-1000000,1000000));spec.centerY=double(jsonInteger(centre[1],-1000000,1000000));
        }
        scene.layers.push_back(spec);
    }
    auto archive=game_lab::loadSG3(archivePath);auto internal=std::filesystem::path(archivePath);internal.replace_extension(".555");
    auto rendered=game_lab::renderSG3DiagnosticScene(archive,internal.string(),scene);if(!rendered.image.valid())throw std::runtime_error("diagnostic source/scene refused;no fallback image");
    CGImageRef cg=game_lab::createCGImageFromPremultipliedBGRA(scene.width,scene.height,rendered.image.pixelsBGRA);if(!cg)throw std::runtime_error("scene CGImage failed");
    game_lab::ExclusiveReferenceOutputs outputs(root.configuredDirectory(),requested);
    if(args.count("--output"))writePNG(outputs,0,cg);
    if(args.count("--producer-output")) {
        std::array<uint8_t,8192> bytes{};
        for(size_t offset=0;offset<rendered.producer.size();offset+=bytes.size()/2) {
            const auto count=std::min(bytes.size()/2,rendered.producer.size()-offset);
            for(size_t p=0;p<count;++p){const auto value=rendered.producer[offset+p];bytes[2*p]=uint8_t(value);bytes[2*p+1]=uint8_t(value>>8);}
            outputs.write(args.count("--output")?1:0,bytes.data(),2*count);
        }
    }
    outputs.commit();
    std::cout<<"{\"sav_coordinates\":"<<(savedCells?"true":"false")<<",\"validated_sav_cells\":"<<(savedCells?scene.layers.size():0)<<",\"selector_status\":\"unsupported\",\"diagnostic_scene\":true,\"native_image_created\":true,\"screenshot_inputs\":0,\"source_rule_confirmed\":false,\"decoded_layers\":"<<rendered.decodedLayers<<",\"covered\":"<<rendered.coveredPixels<<",\"unknown\":"<<rendered.image.pixelsBGRA.size()-rendered.coveredPixels<<",\"denominator\":"<<rendered.image.pixelsBGRA.size()<<",\"visible_per_layer\":[";
    for(size_t i=0;i<rendered.visiblePerLayer.size();++i){if(i)std::cout<<',';std::cout<<rendered.visiblePerLayer[i];}std::cout<<"]}\n"<<std::flush;
    if(!show){CGImageRelease(cg);return 0;}
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];ReferenceDelegate*delegate=[ReferenceDelegate new];NSApp.delegate=delegate;
    const CGFloat w=CGFloat(scene.width)/scene.scale,h=CGFloat(scene.height)/scene.scale;
    NSWindow*window=[[NSWindow alloc]initWithContentRect:NSMakeRect(80,80,w,h+54)styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];window.title=@"DIAGNOSTIC SG3 scene — unconfirmed source/order";
    NSTextField*label=[NSTextField wrappingLabelWithString:@"Explicit SG3 hypotheses; transparent pixels are unknown. No screenshot background; no MAP/SAV automatic selector."];label.frame=NSMakeRect(8,h+4,w-16,46);label.font=[NSFont systemFontOfSize:11];[window.contentView addSubview:label];
    ReferenceImageView*view=[[ReferenceImageView alloc]initWithFrame:NSMakeRect(0,0,w,h)];view.candidate=[[NSImage alloc]initWithCGImage:cg size:NSMakeSize(w,h)];[window.contentView addSubview:view];CGImageRelease(cg);[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];[NSApp run];return 0;
}
int groundSelectorView(const std::map<std::string,std::string>& args,bool show) {
    for(const auto&[key,value]:args){(void)value;if(key!="--data-root"&&key!="--map"&&key!="--sg3"&&key!="--camera-x"&&key!="--camera-y"&&key!="--width"&&key!="--height"&&key!="--scale"&&key!="--output")throw std::runtime_error("ground selector mode only accepts root/map/archive/camera/size/output");}
    for(const auto&k:{"--data-root","--map","--sg3","--camera-x","--camera-y","--width","--height","--scale"})if(!args.count(k))throw std::runtime_error("ground selector needs data root/map/archive/camera/size");
    auto root=game_lab::ResourceRoot::fromAbsolute(args.at("--data-root"));if(!root.configured())throw std::runtime_error("existing absolute data root required");
    const auto vw=integer(args.at("--width")),vh=integer(args.at("--height")),scale=integer(args.at("--scale"));
    if(vw<1||vw>2048||vh<1||vh>2048||scale<1||scale>4)throw std::runtime_error("camera integer bounds");
    const auto mapPath=root.resolve(args.at("--map")),archivePath=root.resolve(args.at("--sg3"));if(mapPath.empty()||archivePath.empty())throw std::runtime_error("relative map/archive path outside root or missing");
    // loadOriginalMapTerrain runs the container decode + terrain grid; the sibling
    // loader parses the full FUN_0052E7C0 read-order per-cell array set from the
    // same payload, which the selector needs for feature bits and fertility.
    auto map=game_lab::loadOriginalMapTerrain(mapPath);
    if(map.status!=game_lab::OriginalMapStatus::LoadedTerrain||!map.terrain.valid())throw std::runtime_error("complete MAP terrain required");
    auto state=game_lab::loadOriginalTerrainState(mapPath);
    if(!state||!state->valid())throw std::runtime_error("complete MAP per-cell loader state required");
    auto archive=game_lab::loadSG3(archivePath);if(!archive.numImages||archive.numImages!=archive.sprites.size())throw std::runtime_error("empty or invalid SG3 archive");
    auto internal=std::filesystem::path(archivePath);internal.replace_extension(".555");
    game_lab::OriginalMapCamera camera;camera.centerX=real(args.at("--camera-x"));camera.centerY=real(args.at("--camera-y"));camera.width=uint32_t(vw);camera.height=uint32_t(vh);camera.scale=uint32_t(scale);camera.outlines=false;
    const auto rendered=game_lab::renderMapGroundViewport(map.terrain,*state,archive,internal.string(),camera);if(!rendered.image.valid())throw std::runtime_error("experimental ground selector viewport rejected");
    CGImageRef cg=game_lab::createCGImageFromPremultipliedBGRA(rendered.image.width,rendered.image.height,rendered.image.pixelsBGRA);if(!cg)throw std::runtime_error("CGImage failed");
    const auto&s=rendered.stats;size_t plainIndex=0;const bool plainResolved=game_lab::resolveGroundTileCode(archive,0x603u,plainIndex);
    std::cout<<"{\"selector\":\"original-exe-dispatch-chain\",\"experimental\":true,\"cell_fields\":\"MAP terrain flags + FUN_0052E7C0 read-order layers (random/feature/fertility)\",\"random_grid_cells\":"<<state->randomByte.size()<<",\"active_cells\":"<<s.activeCells<<",\"known_cells\":"<<s.knownCells<<",\"unknown_cells\":"<<s.unknownCells<<",\"chain_no_write_cells\":"<<s.chainNoWriteCells<<",\"block_cells\":"<<s.blockCells<<",\"unresolved_codes\":"<<s.unresolvedCodes<<",\"geometry_supported_cells\":"<<s.geometrySupported<<",\"culled\":"<<s.culled<<",\"visible_cells\":"<<s.visibleCells<<",\"drawn_cells\":"<<s.drawnCells<<",\"decode_failures\":"<<s.decodeFailures<<",\"unique_decoded_IDs\":"<<s.uniqueDecoded<<",\"cache_bytes\":"<<s.cacheBytes<<",\"branch_cells\":[";
    for(size_t i=0;i<s.branchCells.size();++i){if(i)std::cout<<',';std::cout<<"{\"branch\":\""<<s.branchCells[i].first<<"\",\"cells\":"<<s.branchCells[i].second<<"}";}
    std::cout<<"]"<<",\"plain_land_code_0x603_archive_index\":"<<(plainResolved?std::to_string(plainIndex):std::string("null"))<<",\"plain_land_code_0x603_width\":"<<(plainResolved?int(archive.sprites[plainIndex].width):-1)<<",\"native_image_created\":true,\"pixel_width\":"<<vw<<",\"pixel_height\":"<<vh<<"}\n"<<std::flush;
    if(args.count("--output"))savePNG(args.at("--output"),root.configuredDirectory(),cg);
    if(!show){CGImageRelease(cg);return 0;}
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];ReferenceDelegate*delegate=[ReferenceDelegate new];NSApp.delegate=delegate;
    const CGFloat displayW=CGFloat(vw)/scale,displayH=CGFloat(vh)/scale;NSWindow*window=[[NSWindow alloc]initWithContentRect:NSMakeRect(80,80,displayW,displayH+54)styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];window.title=@"EXPERIMENTAL — original ground dispatch chain — unverified";
    NSTextField*label=[NSTextField wrappingLabelWithString:[NSString stringWithFormat:@"Original-dispatch ground selector · %@ · loader cells %zu\n%zu drawn / %zu visible; known %zu, unknown %zu, unresolved %zu.",[NSString stringWithUTF8String:args.at("--sg3").c_str()],state->randomByte.size(),s.drawnCells,s.visibleCells,s.knownCells,s.unknownCells,s.unresolvedCodes]];label.frame=NSMakeRect(8,displayH+4,displayW-16,46);label.font=[NSFont systemFontOfSize:11];[window.contentView addSubview:label];
    ReferenceImageView*view=[[ReferenceImageView alloc]initWithFrame:NSMakeRect(0,0,displayW,displayH)];view.candidate=[[NSImage alloc]initWithCGImage:cg size:NSMakeSize(displayW,displayH)];[window.contentView addSubview:view];CGImageRelease(cg);[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];[NSApp run];return 0;
}
int imageGridView(const std::map<std::string,std::string>& args,bool show) {
    for(const auto&[key,value]:args){(void)value;if(key!="--data-root"&&key!="--map"&&key!="--sg3"&&key!="--sg3-elevation"&&key!="--sg3-tilesets"&&key!="--camera-x"&&key!="--camera-y"&&key!="--width"&&key!="--height"&&key!="--scale"&&key!="--output")throw std::runtime_error("image grid mode only accepts root/map/archive/tilesets/camera/size/output");}
    for(const auto&k:{"--data-root","--map","--sg3","--camera-x","--camera-y","--width","--height","--scale"})if(!args.count(k))throw std::runtime_error("image grid needs data root/map/archive/camera/size");
    auto root=game_lab::ResourceRoot::fromAbsolute(args.at("--data-root"));if(!root.configured())throw std::runtime_error("existing absolute data root required");
    const auto vw=integer(args.at("--width")),vh=integer(args.at("--height")),scale=integer(args.at("--scale"));
    if(vw<1||vw>2048||vh<1||vh>2048||scale<1||scale>4)throw std::runtime_error("camera integer bounds");
    const auto mapPath=root.resolve(args.at("--map")),archivePath=root.resolve(args.at("--sg3"));if(mapPath.empty()||archivePath.empty())throw std::runtime_error("relative map/archive path outside root or missing");
    auto map=game_lab::loadOriginalMapTerrain(mapPath);
    if(map.status!=game_lab::OriginalMapStatus::LoadedTerrain||!map.terrain.valid())throw std::runtime_error("complete MAP terrain required");
    auto state=game_lab::loadOriginalTerrainState(mapPath);
    if(!state||!state->valid())throw std::runtime_error("complete MAP per-cell loader state required");
    // The stored grid names a tileset index; the original executable's static
    // name table (FUN_00475B60) maps it to an archive. Tileset 3 is the supplied
    // --sg3, tileset 16 --sg3-elevation, and the remaining indices the 167-city
    // corpus references (2/5/6/8) are looked up next to --sg3 by their table name.
    // --sg3-tilesets overrides/extends the set explicitly. firstRecordId 201 is
    // the version-213 low-14 base (map_reference.hpp MapImageGridTileset).
    struct LoadedGridArchive { game_lab::SG3Archive archive; std::string relative, absolute; };
    std::map<uint32_t,LoadedGridArchive> loaded;
    const std::filesystem::path archiveRelative(args.at("--sg3"));
    const auto archiveDirectory=archiveRelative.parent_path();
    auto internalFor=[](const std::string& path){auto p=std::filesystem::path(path);p.replace_extension(".555");return p.string();};
    auto loadRelative=[&](uint32_t tileset,const std::string& relative,bool required) {
        const auto path=root.resolve(relative);
        if(path.empty()) {
            if(required)throw std::runtime_error("relative per-tileset archive path outside root or missing");
            return false;
        }
        auto archive=game_lab::loadSG3(path);
        if(!archive.numImages||archive.numImages!=archive.sprites.size())
            throw std::runtime_error("empty or invalid per-tileset SG3 archive");
        loaded.erase(tileset);
        loaded.emplace(tileset,LoadedGridArchive{std::move(archive),relative,path});
        return true;
    };
    loadRelative(3,args.at("--sg3"),true);
    if(args.count("--sg3-elevation"))loadRelative(16,args.at("--sg3-elevation"),true);
    else loadRelative(16,(archiveDirectory/"China_Elevation.sg3").generic_string(),false);
    for(uint32_t tileset:{2u,5u,6u,8u}) {
        const char* leaf=game_lab::mapImageGridArchiveName(tileset);
        if(leaf)loadRelative(tileset,(archiveDirectory/leaf).generic_string(),false);
    }
    if(args.count("--sg3-tilesets")) {
        const auto& list=args.at("--sg3-tilesets");
        size_t start=0;
        while(start<=list.size()) {
            const auto end=list.find(',',start);
            const auto item=list.substr(start,end==std::string::npos?std::string::npos:end-start);
            const auto equals=item.find('=');
            if(equals==std::string::npos||equals==0||equals+1>=item.size())
                throw std::runtime_error("--sg3-tilesets entries must be index=relative-path");
            const auto indexText=item.substr(0,equals);
            size_t used=0;unsigned long index=0;
            try{index=std::stoul(indexText,&used,10);}catch(const std::exception&){throw std::runtime_error("--sg3-tilesets index must be an integer");}
            if(used!=indexText.size()||index==0||index>40)
                throw std::runtime_error("--sg3-tilesets index out of range");
            loadRelative(uint32_t(index),item.substr(equals+1),true);
            if(end==std::string::npos)break;
            start=end+1;
        }
    }
    std::vector<game_lab::MapImageGridTileset> tilesets;
    for(const auto& entry:loaded)
        tilesets.push_back({entry.first,&entry.second.archive,internalFor(entry.second.absolute),201});
    const auto& archive=loaded.at(3).archive;
    game_lab::OriginalMapCamera camera;camera.centerX=real(args.at("--camera-x"));camera.centerY=real(args.at("--camera-y"));camera.width=uint32_t(vw);camera.height=uint32_t(vh);camera.scale=uint32_t(scale);camera.outlines=false;
    const auto rendered=game_lab::renderMapImageGridViewport(map.terrain,*state,tilesets,camera);if(!rendered.image.valid())throw std::runtime_error("stored image-grid viewport rejected");
    const auto score=game_lab::scoreGroundChainAgainstImageGrid(map.terrain,*state,archive);
    CGImageRef cg=game_lab::createCGImageFromPremultipliedBGRA(rendered.image.width,rendered.image.height,rendered.image.pixelsBGRA);if(!cg)throw std::runtime_error("CGImage failed");
    const auto&s=rendered.stats;
    const bool exact=!s.unresolvedIndices&&!s.geometryUnsupported&&!s.archiveUnavailable&&!s.decodeFailures;
    std::cout<<"{\"mode\":\"stored-image-grid\",\"experimental\":true,\"grid_cells\":"<<s.activeCells<<",\"zero_cells\":"<<s.zeroCells<<",\"nonzero_cells\":"<<s.nonzeroCells<<",\"decoded_cells\":"<<s.decodedCells<<",\"drawn_cells\":"<<s.drawnCells<<",\"culled\":"<<s.culled<<",\"visible_cells\":"<<s.visibleCells<<",\"unresolved_indices\":"<<s.unresolvedIndices<<",\"geometry_unsupported\":"<<s.geometryUnsupported<<",\"archive_unavailable\":"<<s.archiveUnavailable<<",\"decode_failures\":"<<s.decodeFailures<<",\"unique_decoded_IDs\":"<<s.uniqueDecoded<<",\"cache_bytes\":"<<s.cacheBytes<<",\"composite_blocks\":"<<s.compositeBlocks<<",\"composite_confirmed_cells\":"<<s.compositeConfirmedCells<<",\"composite_mismatched_cells\":"<<s.compositeMismatchedCells<<",\"composite_foreign_cells\":"<<s.compositeForeignCells<<",\"drawn_blocks\":"<<s.drawnBlocks<<",\"tileset_cells\":[";
    for(size_t i=0;i<s.tilesetCells.size();++i){if(i)std::cout<<',';size_t drawn=0;for(const auto& d:s.tilesetDrawn)if(d.first==s.tilesetCells[i].first)drawn=d.second;std::cout<<"{\"tileset\":"<<s.tilesetCells[i].first<<",\"cells\":"<<s.tilesetCells[i].second<<",\"drawn\":"<<drawn<<"}";}
    std::cout<<"]"<<",\"composite_mismatches\":[";
    for(size_t i=0;i<s.compositeMismatches.size();++i){if(i)std::cout<<',';const auto& m=s.compositeMismatches[i];std::cout<<"{\"tileset\":"<<m.tileset<<",\"record_id\":"<<m.recordId<<",\"width\":"<<m.width<<",\"height\":"<<m.height<<",\"cols\":"<<m.cols<<",\"bbox_width\":"<<m.bboxWidth<<",\"bbox_height\":"<<m.bboxHeight<<",\"cells\":"<<m.cells<<",\"full_rect\":"<<(m.fullRect?"true":"false")<<",\"divisible\":"<<(m.divisible?"true":"false")<<"}";}
    std::cout<<"]"<<",\"chain_score\":{\"cells\":"<<score.activeCells<<",\"emitted\":"<<score.emitted<<",\"matched\":"<<score.matched<<",\"mismatched\":"<<score.mismatched<<",\"known_matched\":"<<score.knownMatched<<",\"chain_no_write_vs_stored_zero\":"<<score.chainNoWriteVsStoredZero<<",\"chain_no_write_vs_stored_nonzero\":"<<score.chainNoWriteVsStoredNonzero<<",\"stored_zero\":"<<score.storedZero<<",\"stored_zero_unpredicted\":"<<score.storedZeroUnpredicted<<",\"chain_known\":"<<score.chainKnown<<",\"chain_unknown\":"<<score.chainUnknown<<",\"chain_no_write\":"<<score.chainNoWrite<<",\"unresolved\":"<<score.unresolved<<",\"branches\":[";
    for(size_t i=0;i<score.branches.size();++i){if(i)std::cout<<',';const auto& b=score.branches[i];std::cout<<"{\"branch\":\""<<b.branch<<"\",\"cells\":"<<b.cells<<",\"emitted\":"<<b.emitted<<",\"matched\":"<<b.matched<<",\"stored_zero\":"<<b.storedZero<<"}";}
    std::cout<<"]}"<<",\"tilesets\":[";
    {size_t i=0;for(const auto& entry:loaded) {
        if(i++)std::cout<<',';
        std::cout<<"{\"tileset\":"<<entry.first<<",\"archive\":\""<<entry.second.relative
            <<"\",\"records\":"<<entry.second.archive.numImages<<",\"first_record_id\":201}";
    }}
    std::cout<<"]"<<",\"exact\":"<<(exact?"true":"false")<<",\"native_image_created\":true,\"pixel_width\":"<<vw<<",\"pixel_height\":"<<vh<<"}\n"<<std::flush;
    if(args.count("--output"))savePNG(args.at("--output"),root.configuredDirectory(),cg);
    if(!show){CGImageRelease(cg);return 0;}
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];ReferenceDelegate*delegate=[ReferenceDelegate new];NSApp.delegate=delegate;
    const CGFloat displayW=CGFloat(vw)/scale,displayH=CGFloat(vh)/scale;NSWindow*window=[[NSWindow alloc]initWithContentRect:NSMakeRect(80,80,displayW,displayH+54)styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];window.title=@"STORED image grid — 0x00FE9880 read verbatim";
    NSTextField*label=[NSTextField wrappingLabelWithString:[NSString stringWithFormat:@"Stored surface-image grid · tilesets %zu\n%zu drawn / %zu nonzero; archive-unavailable %zu, confirmed blocks %zu, mismatched %zu, unresolved %zu.",tilesets.size(),s.drawnCells,s.nonzeroCells,s.archiveUnavailable,s.compositeBlocks,s.compositeMismatchedCells,s.unresolvedIndices]];label.frame=NSMakeRect(8,displayH+4,displayW-16,46);label.font=[NSFont systemFontOfSize:11];[window.contentView addSubview:label];
    ReferenceImageView*view=[[ReferenceImageView alloc]initWithFrame:NSMakeRect(0,0,displayW,displayH)];view.candidate=[[NSImage alloc]initWithCGImage:cg size:NSMakeSize(displayW,displayH)];[window.contentView addSubview:view];CGImageRelease(cg);[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];[NSApp run];return 0;
}
}
int main(int argc,char**argv){@autoreleasepool{try{
    std::map<std::string,std::string> args;bool show=true;bool groundSelector=false;bool imageGrid=false;
    for(int i=1;i<argc;++i){std::string key=argv[i];if(key=="--no-window"){show=false;continue;}if(key=="--ground-selector"){groundSelector=true;continue;}if(key=="--image-grid"){imageGrid=true;continue;}if(key.rfind("--",0)!=0||i+1>=argc||args.count(key))throw std::runtime_error("unique named option/value required");args[key]=argv[++i];}
    if(args.count("--scene-manifest"))return diagnosticScene(args,show);
    if(imageGrid)return imageGridView(args,show);
    if(groundSelector)return groundSelectorView(args,show);
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
