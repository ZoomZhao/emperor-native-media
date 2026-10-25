// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include "city_playback.hpp"
#include "../original_pak.hpp"
#include "../asset_bytes.hpp"
#include "../cg_image_conversion.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <tuple>
#include <sstream>
#include <limits>
#include <zlib.h>
namespace {
using Bytes=std::vector<uint8_t>;
size_t checks=0;
void need(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
using GroupKey=std::tuple<uint32_t,int,int>;
GroupKey storedGroupKey(const game_lab::OriginalTerrainState& state,size_t cell){
    return {state.image[cell],int(cell%228)-int(state.draw[cell]&7),
        int(cell/228)-int((state.draw[cell]>>3)&7)};
}
bool markerConstraint(const game_lab::OriginalTerrainState& state,size_t cell){
    const auto n=uint32_t(state.subTile[cell]&15)+1;
    return bool(state.draw[cell]&64)==((state.draw[cell]&7)==0&&uint32_t((state.draw[cell]>>3)&7)==n-1);
}
struct StoredGroupAudit {
    const game_lab::OriginalMapTerrain& terrain;
    const game_lab::OriginalTerrainState& state;
    uint32_t origin,side;
    std::map<GroupKey,size_t> members;
    StoredGroupAudit(const game_lab::OriginalMapTerrain& t,const game_lab::OriginalTerrainState& s):terrain(t),state(s){
        const auto activity=game_lab::analyzeOriginalMap(t);origin=activity.cropOrigin;side=activity.cropSide;
        for(uint32_t y=origin;y<origin+side;++y)for(uint32_t x=origin;x<origin+side;++x){
            const size_t cell=size_t(y)*228+x;
            if(!(terrain.flags[cell]&0x80000u)&&state.image[cell])++members[storedGroupKey(state,cell)];
        }
    }
    bool touchesInactive(size_t cell)const{
        const auto [word,ax,ay]=storedGroupKey(state,cell);(void)word;
        const int n=int(state.subTile[cell]&15)+1;
        for(int y=0;y<n;++y)for(int x=0;x<n;++x){
            const int px=ax+x,py=ay+y;
            if(px<int(origin)||py<int(origin)||px>=int(origin+side)||py>=int(origin+side)||
                (terrain.flags[size_t(py)*228+size_t(px)]&0x80000u))return true;
        }
        return false;
    }
    bool complete(size_t cell)const{
        const auto key=storedGroupKey(state,cell);const auto [word,ax,ay]=key;
        const uint32_t n=uint32_t(state.subTile[cell]&15)+1;
        if((state.draw[cell]&7)>=n||((state.draw[cell]>>3)&7)>=n||touchesInactive(cell))return false;
        const auto found=members.find(key);if(found==members.end()||found->second!=size_t(n)*n)return false;
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){
            const size_t next=size_t(ay+int(y))*228+size_t(ax+int(x));
            if(state.image[next]!=word||uint32_t(state.subTile[next]&15)+1!=n||
                (state.draw[next]&63)!=x+8*y)return false;
        }
        return true;
    }
};
void put(Bytes& b,size_t p,uint32_t n,unsigned width=4){for(unsigned i=0;i<width;++i)b.at(p+i)=uint8_t(n>>(8*i));}
void write(const std::filesystem::path& p,const Bytes& b){std::filesystem::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)throw std::runtime_error("owned fixture write");}
Bytes framed(const Bytes& b){Bytes out(4);put(out,0,0xfedcbaaa);for(size_t p=0;p<b.size();p+=32768){auto n=std::min(size_t(32768),b.size()-p);Bytes z(compressBound(n));uLongf length=z.size();if(compress2(z.data(),&length,b.data()+p,n,9)!=Z_OK)throw std::runtime_error("fixture compression");size_t q=out.size();out.resize(q+12+length);put(out,q,0xabcdefff);put(out,q+4,uint32_t(length));put(out,q+8,uint32_t(n));std::copy(z.begin(),z.begin()+length,out.begin()+q+12);}return out;}
void fixture(const std::filesystem::path& root){
    using game_lab::OriginalTerrainState;
    Bytes map(OriginalTerrainState::flatEnd);map[0]=5;map[2]=254;map[3]=202;map[6]=2;put(map,84,84);
    for(size_t i=0;i<51984;++i)put(map,OriginalTerrainState::terrainOffset+i*4,0x80000);
    const size_t cell=114*228+114;put(map,OriginalTerrainState::terrainOffset+cell*4,128);put(map,OriginalTerrainState::imageOffset+cell*4,3u<<14);
    write(root/"Cities/own.map",framed(map));
    constexpr size_t table=40680;Bytes sg(table+203*64);put(sg,0,uint32_t(sg.size()));put(sg,4,213);put(sg,12,203);put(sg,16,202);
    const size_t record=table+201*64;put(sg,record+4,3200);put(sg,record+8,3200);put(sg,record+20,78,2);put(sg,record+22,40,2);put(sg,record+50,30,2);sg[record+55]=1;
    write(root/"DATA/China_Terrain.sg3",sg);Bytes pixels(3200);for(size_t p=0;p<pixels.size();p+=2)put(pixels,p,0x03e0,2);write(root/"DATA/China_Terrain.555",pixels);
}
void candidateChecks(const std::filesystem::path& root){
    using namespace game_lab;
    auto base=loadOriginalMapTerrain((root/"Cities/own.map").string()).terrain;
    auto state=*loadOriginalTerrainState((root/"Cities/own.map").string());
    auto reset=[&]{std::fill(base.flags.begin(),base.flags.end(),0x80000u);std::fill(state.terrain.begin(),state.terrain.end(),0x80000u);
        std::fill(state.image.begin(),state.image.end(),0);std::fill(state.draw.begin(),state.draw.end(),0);std::fill(state.subTile.begin(),state.subTile.end(),0);};
    const uint32_t origin=analyzeOriginalMap(base).cropOrigin;
    auto cell=[&](uint32_t x,uint32_t y){return size_t(y+origin)*228+x+origin;};
    auto group=[&](uint32_t x,uint32_t y,uint32_t n,uint32_t word){for(uint32_t v=0;v<n;++v)for(uint32_t u=0;u<n;++u){
        const auto i=cell(x+u,y+v);base.flags[i]=state.terrain[i]=128;state.image[i]=word;state.subTile[i]=uint8_t(n-1);state.draw[i]=uint8_t(u+8*v);}};
    SG3Archive archive;archive.version=213;archive.numImages=1;archive.sprites.resize(1);
    auto& sprite=archive.sprites[0];sprite.type=0;sprite.width=sprite.height=2;sprite.length=8;
    const auto pixels=root/"candidate.555";write(pixels,Bytes{0xe0,3,0xe0,3,0xe0,3,0xe0,3});
    std::vector<MapImageGridTileset> tiles{{3,&archive,pixels.string(),1}};
    OriginalMapCamera camera;camera.width=256;camera.height=192;camera.centerX=camera.centerY=40.5;
    auto render=[&]{return renderMapSurfaceCandidateViewport(base,state,tiles,camera);};
    reset();group(40,40,2,3u<<14);auto full=render();
    need(full.verified.decodedCells==0&&full.verified.geometryUnsupported==4&&full.candidate.structuralCells==4&&full.candidate.decodedCells==4&&full.candidate.decodedGroups==1,"candidate retains strict failure but decodes one complete stored group");
    need(full.candidate.paintedPixels==4&&std::count(full.image.pixelsBGRA.begin(),full.image.pixelsBGRA.end(),0xff00ff00u)==4,"regular canvas is painted once,not once per member");
    group(42,40,2,3u<<14);auto adjacent=render();need(adjacent.candidate.decodedCells==8&&adjacent.candidate.decodedGroups==2&&adjacent.candidate.paintedPixels==8,"adjacent same-image groups keep distinct anchors");
    reset();group(40,40,2,3u<<14);base.flags[cell(41,41)]|=0x80000u;need(render().candidate.decodedCells==0,"incomplete inactive group remains unknown");
    reset();group(40,40,2,3u<<14);state.draw[cell(41,41)]=0;need(render().candidate.decodedCells==0,"duplicate offsets cannot manufacture a complete group");
    reset();group(40,40,2,3u<<14);state.subTile[cell(41,41)]=0;need(render().candidate.decodedCells==0,"mixed group sides fail closed");
    reset();group(40,40,2,3u<<14);state.draw[cell(41,41)]=7;need(render().candidate.decodedCells==0,"out-of-side offset is not clamped");
    reset();group(0,0,1,3u<<14);state.draw[cell(0,0)]=7;need(render().candidate.decodedCells==0,"negative inferred anchor is rejected before indexing");
    reset();group(83,83,1,3u<<14);state.subTile[cell(83,83)]=1;need(render().candidate.decodedCells==0,"footprint beyond active crop is rejected");
    reset();group(40,40,16,3u<<14);need(render().candidate.decodedCells==0,"four-bit size cannot bypass three-bit offset range");
    reset();group(40,40,2,3u<<14);const auto oldPath=tiles[0].internal555;tiles[0].internal555=(root/"missing-candidate.555").string();
    auto missing=render();need(missing.candidate.decodedCells==0&&missing.candidate.decodeFailureCells==4&&missing.candidate.paintedPixels==0,"candidate missing payload is not decoded coverage");
    const auto cut=root/"truncated-candidate.555";write(cut,Bytes{0xe0,3});tiles[0].internal555=cut.string();
    need(render().candidate.decodeFailureCells==4&&render().candidate.decodedCells==0,"truncated candidate payload never produces a partial success");
    tiles[0].internal555=(root/"missing-candidate.555").string();
    camera.centerX=camera.centerY=0;missing=render();need(missing.candidate.decodeFailureCells==4&&missing.candidate.decodedCells==0,"offscreen failed payload still validated");
    tiles[0].internal555=oldPath;auto offscreen=render();need(offscreen.candidate.decodedCells==4&&offscreen.candidate.drawnCells==0,"offscreen payload and visible paint counts stay separate");
    camera.centerX=camera.centerY=40.5;camera.centerX=std::numeric_limits<double>::quiet_NaN();need(!render().image.valid(),"invalid camera cannot allocate or paint candidates");camera.centerX=40.5;
    sprite.mirrorOffset=1;need(render().candidate.decodedCells==0,"forward/cyclic mirror is rejected");sprite.mirrorOffset=0;
    // The candidate path follows the logical mirror identity,not a physical-ID cache shortcut.
    SG3Archive mirrored=archive;mirrored.numImages=2;mirrored.sprites.push_back(SG3Sprite{});mirrored.sprites[1].mirrorOffset=-1;
    write(pixels,Bytes{0,0x7c,0xe0,3,0x1f,0,0xff,0x7f});tiles={{3,&mirrored,pixels.string(),1}};reset();group(40,40,1,(3u<<14)|1);
    auto reflected=render();need(reflected.candidate.decodedCells==1&&reflected.image.pixelsBGRA[94*256+127]==0xff00ff00u&&reflected.image.pixelsBGRA[94*256+128]==0xffff0000u,"backward mirror keeps exact reflected candidate pixels");
    tiles={{3,&archive,pixels.string(),1}};
    sprite.type=30;sprite.width=158;sprite.height=80;sprite.length=sprite.uncompressedLength=12800;sprite.tileSize=2;
    write(pixels,Bytes(12800,0));reset();group(40,40,3,3u<<14);auto different=render();
    need(different.candidate.structuralCells==9&&different.candidate.decodedCells==9&&different.candidate.decodedGroups==1&&different.verified.geometryUnsupported==9,"declared 2x2 base remains a full decoded canvas for a candidate 3x3 storage group");
    need(different.candidate.paintedPixels==6400,"SG3 base pixels are not stretched to fit storage group size");
    // Two namespaces share logical index zero but carry different pixels. Their
    // overlapping canvases exercise premultiplied alpha and deterministic order.
    SG3Archive green,red;green.version=red.version=214;green.numImages=red.numImages=1;green.sprites.resize(1);red.sprites.resize(1);
    auto& g=green.sprites[0];g.type=0;g.width=100;g.height=60;g.length=12000;
    auto& r=red.sprites[0];r=g;r.offset=12000;r.alphaLength=6000+24;
    Bytes data(24000);for(size_t i=0;i<6000;++i){put(data,2*i,0x03e0,2);put(data,12000+2*i,0x7c00,2);}
    for(size_t left=6000;left;){const size_t n=std::min(left,size_t(250));data.push_back(uint8_t(n));data.insert(data.end(),n,16);left-=n;}
    write(pixels,data);tiles={{3,&green,pixels.string(),1},{8,&red,pixels.string(),1}};
    reset();group(40,40,1,3u<<14);group(41,40,1,8u<<14);auto blended=render();
    need(blended.candidate.decodedGroups==2&&blended.candidate.drawnGroups==2&&blended.candidate.decodedCells==2,"candidate cache separates same logical ID in different archives");
    need(std::count(blended.image.pixelsBGRA.begin(),blended.image.pixelsBGRA.end(),0xff847b00u)>0,"candidate overlapping alpha uses premultiplied source-over");
    std::reverse(tiles.begin(),tiles.end());need(render().image.pixelsBGRA==blended.image.pixelsBGRA,"candidate painter order does not depend on archive input order");
}

Bytes authoredPackage(const std::vector<Bytes>& payloads){
    Bytes out(4);put(out,0,0xfedcbaaa);std::vector<uint32_t> offsets;
    for(size_t i=0;i<payloads.size();++i){if(i)offsets.push_back(uint32_t(out.size()));auto f=framed(payloads[i]);out.insert(out.end(),f.begin()+4,f.end());}
    if(offsets.empty())offsets.push_back(uint32_t(out.size()));
    const size_t footer=out.size();out.resize(footer+68);put(out,footer,0xaaabcdef);put(out,footer+4,0x3c);
    for(size_t i=0;i<offsets.size();++i)put(out,footer+8+4*i,offsets[i]);
    put(out,footer+52,uint32_t(offsets.size()-1));put(out,footer+56,0xaaabcdef);put(out,footer+60,4);put(out,footer+64,0xaaaabbcc);return out;
}
void pakChecks(const std::filesystem::path& root){
    using namespace game_lab;
    std::ifstream stream(root/"Cities/own.map",std::ios::binary);Bytes compressed((std::istreambuf_iterator<char>(stream)),{});
    auto decoded=decodeOriginalMapContainer(compressed);need(decoded.complete,"owned city has complete source payload");
    auto second=decoded.payload;put(second,OriginalTerrainState::imageOffset+4*(114*228+114),(3u<<14)|1);
    std::filesystem::create_directories(root/"Campaigns");const auto pakPath=root/"Campaigns/own.pak";
    write(pakPath,authoredPackage({Bytes{'x'},decoded.payload,second}));
    auto configured=ResourceRoot::fromAbsolute(root.string());auto source=OriginalMapSource{OriginalMapSourceKind::PakCity,"Campaigns/own.pak","owned package city",1};
    auto selected=loadOriginalMapSource(configured,source);need(selected.surfaceState&&selected.surfaceState->image[114*228+114]==(3u<<14),"PAK selected part keeps its stored image plane");
    AppMapSurface app(configured);need(app.select(source,selected.map.terrain),"App accepts actual package-city source");
    OriginalMapCamera c;c.width=256;c.height=192;c.centerX=c.centerY=42;auto image=app.renderPreview(c);
    need(image.verified.decodedCells==1&&std::count(image.image.pixelsBGRA.begin(),image.image.pixelsBGRA.end(),0xff00ff00u)==1600,"PAK stored surface reaches exact native pixels without substituting a loose MAP");
    source.partIndex=2;selected=loadOriginalMapSource(configured,source);need(selected.surfaceState&&selected.surfaceState->image[114*228+114]==((3u<<14)|1),"same-terrain parts retain distinct stored image identities");
    need(app.select(source,selected.map.terrain)&&app.render(c).stats.decodedCells==0,"part selection does not retain previous part pixels");
    source.partIndex=0;need(!app.select(source,selected.map.terrain)&&!app.available(),"non-city part clears previous PAK surface");
    source.partIndex=99;need(!app.select(source,selected.map.terrain)&&!app.available(),"stale PAK index fails closed");
    source.partIndex=1;source.relativePath="../outside.pak";need(!app.select(source,selected.map.terrain),"PAK source cannot escape root");
    source.relativePath="Campaigns/own.pak";source.kind=OriginalMapSourceKind::PakSummary;need(!app.select(source,selected.map.terrain),"package summary has no invented surface");
    source.kind=OriginalMapSourceKind::SaveFile;need(!app.select(source,selected.map.terrain),"SAV source cannot borrow PAK or MAP image fields");
    source.kind=OriginalMapSourceKind::PakCity;auto wrong=selected.map.terrain;wrong.flags[0]^=1;
    need(!app.select(source,wrong),"PAK surface rejects mismatched expected terrain");
    auto shortCity=decoded.payload;shortCity.resize(OriginalMapCellLayers::mapperFertileOffset+51984);write(pakPath,authoredPackage({Bytes{'x'},shortCity}));
    selected=loadOriginalMapSource(configured,source);need(selected.map.terrain.valid()&&!selected.surfaceState&&!app.select(source,selected.map.terrain),"numerical-only package city remains inspectable without invented surface state");
    auto corrupt=authoredPackage({Bytes{'x'},decoded.payload});corrupt[corrupt.size()-4]^=1;write(pakPath,corrupt);
    need(!app.select(source,wrong)&&!app.available()&&!app.render(c).image.valid(),"corrupt complete container cannot expose a stale or partial scene");
}

void preferenceChecks(const std::filesystem::path& path){
    using namespace game_lab;
    std::filesystem::create_directories(path);
    const auto root=ResourceRoot::fromAbsolute(path.string());
    AppCityPlaybackPreferences missing;std::string error;
    need(!missing.annualDemandResponseMode,"no default annual response mode");
    need(!readAppCityPlaybackPreferences(root,missing,error)&&!missing.annualDemandResponseMode,"missing preference file leaves mode unknown");
    Bytes bytes(0x32C);put(bytes,0x14,80);put(bytes,0x58,80);bytes[0x0B]=1;
    bytes[0x9B]=17;bytes[0x9D]=251;bytes[0x9E]=3;bytes[0x9F]=7;
    for(uint8_t mode:{uint8_t(0),uint8_t(1),uint8_t(2),uint8_t(3),uint8_t(255)}){
        bytes[0x9C]=mode;write(path/"Emperor.inf",bytes);
        AppCityPlaybackPreferences parsed;
        need(readAppCityPlaybackPreferences(root,parsed,error)&&parsed.annualDemandResponseMode==mode,
            "read exact annual response byte without widening or replacing unsupported values");
        need(parsed.speed==80&&parsed.personalSpeed==80&&parsed.soundEffects,"adjacent annual preference does not alter existing preferences");
    }
    bytes.pop_back();write(path/"Emperor.inf",bytes);AppCityPlaybackPreferences shortFile;
    need(!readAppCityPlaybackPreferences(root,shortFile,error)&&!shortFile.annualDemandResponseMode,"short preferences cannot supply an annual response mode");
}
void authored(const std::filesystem::path& root){
    preferenceChecks(root/"preferences");
    using namespace game_lab;fixture(root);const auto configured=ResourceRoot::fromAbsolute(root.string());AppMapSurface app(configured);
    auto map=loadOriginalMapTerrain((root/"Cities/own.map").string());need(map.status==OriginalMapStatus::LoadedTerrain,"owned MAP parses");
    need(app.select("Cities/own.map",map.terrain),"App selects same complete MAP");OriginalMapCamera c;c.centerX=c.centerY=42;c.width=256;c.height=192;
    auto image=app.render(c);need(image.image.valid()&&image.stats.nonzeroCells==1&&image.stats.decodedCells==1,"stored surface is rendered rather than numerical flags");
    need(app.diagnostics(image.stats).totalUnknown==0&&app.diagnostics(image.stats).cells.empty(),"complete source has no invented issue coordinates");
    need(std::count(image.image.pixelsBGRA.begin(),image.image.pixelsBGRA.end(),0xff00ff00u)==1600,"owned RGB555 art reaches exact native pixels");
    auto cg=createCGImageFromPremultipliedBGRA(image.image.width,image.image.height,image.image.pixelsBGRA);need(cg!=nullptr,"App CGImage creation");if(cg)CGImageRelease(cg);
    need(!app.select("../outside.map",map.terrain)&&!app.available()&&!app.render(c).image.valid(),"unsafe selection clears previous surface");
    need(app.select("Cities/own.map",map.terrain),"selection recovers after failure");auto wrong=map.terrain;wrong.activeSide=112;
    need(!app.select("Cities/own.map",wrong)&&!app.available(),"different crop cannot reuse old surface");wrong=map.terrain;wrong.flags[0]^=1;
    need(!app.select("Cities/own.map",wrong),"different field snapshot refused");
    AppMapSurface missing(configured);std::filesystem::rename(root/"DATA",root/"assets-absent");need(missing.select("Cities/own.map",map.terrain),"missing assets keep measured unknown surface");
    auto unknown=missing.render(c);need(unknown.image.valid()&&unknown.stats.archiveUnavailable==1&&unknown.stats.decodedCells==0,"missing archive is counted,not substituted");
    const auto missingIssues=missing.diagnostics(unknown.stats);
    need(missingIssues.cells.size()==1&&missingIssues.unlocated==0&&missingIssues.cells[0].x==42&&missingIssues.cells[0].y==42&&missingIssues.cells[0].kind==AppSurfaceIssueKind::ArchiveUnavailable,"missing archive has source-bound active coordinates and class");
    need(describeAppSurfaceDiagnostics(missingIssues).find("活动(42,42)")!=std::string::npos,"diagnostic text exposes zero-based coordinates");
    need(describeAppSurfaceDiagnostics(missingIssues,0).find("0/1")!=std::string::npos,"bounded issue list declares omitted rows");
    missing.clear();need(missing.diagnostics(unknown.stats).cells.empty()&&missing.diagnostics(unknown.stats).unlocated==1,"cleared source cannot reuse old coordinates");
    auto onlyFailure=image.stats;onlyFailure.decodedCells=0;onlyFailure.decodeFailures=1;
    need(app.diagnostics(onlyFailure).unlocated==1&&app.diagnostics(onlyFailure).cells.empty(),"decode failure without per-cell evidence remains explicitly unlocated");
    need(std::all_of(unknown.image.pixelsBGRA.begin(),unknown.image.pixelsBGRA.end(),[](uint32_t p){return p==0;}),"unknown surface remains transparent");
    auto state=*loadOriginalTerrainState((root/"Cities/own.map").string());auto terrain=map.terrain;
    const size_t anchor=114*228+114;
    for(size_t y=0;y<2;++y)for(size_t x=0;x<2;++x){const size_t i=anchor+y*228+x;
        terrain.flags[i]=128;state.image[i]=3u<<14;state.subTile[i]=1;state.draw[i]=uint8_t(x+8*y);}
    need(StoredGroupAudit(terrain,state).complete(anchor+229),"complete stored 2x2 group has one implied anchor");
    auto broken=state;broken.image[anchor+229]++;
    need(!StoredGroupAudit(terrain,broken).complete(anchor),"foreign image breaks stored group");
    broken=state;broken.subTile[anchor+229]=2;
    need(!StoredGroupAudit(terrain,broken).complete(anchor),"mixed stored sides rejected");
    broken=state;broken.draw[anchor+229]=0;
    need(!StoredGroupAudit(terrain,broken).complete(anchor),"duplicate offset rejects missing member");
    broken=state;auto extra=terrain;extra.flags[anchor+2]=128;broken.image[anchor+2]=3u<<14;broken.draw[anchor+2]=2;
    need(!StoredGroupAudit(extra,broken).complete(anchor),"extra cell sharing implied anchor rejected");
    auto inactive=terrain;inactive.flags[anchor+229]|=0x80000u;
    need(!StoredGroupAudit(inactive,state).complete(anchor)&&StoredGroupAudit(inactive,state).touchesInactive(anchor),"inactive footprint cannot be complete");
    broken=state;for(size_t y=0;y<2;++y)for(size_t x=0;x<2;++x)broken.draw[anchor+y*228+x]|=64;
    need(StoredGroupAudit(terrain,broken).complete(anchor),"uninterpreted upper draw bits do not change low-six group constraint");
    broken=state;broken.draw[anchor+228]|=64;
    need(markerConstraint(broken,anchor+228)&&markerConstraint(broken,anchor+229),"marker constraint distinguishes column-zero last row");
    broken=state;broken.draw[anchor+229]|=64;
    need(!markerConstraint(broken,anchor+228)&&!markerConstraint(broken,anchor+229),"bottom-right marker hypothesis is falsifiable");
    for(bool foreign:{false,true}){
        const auto own=root/(foreign?"foreign":"conflict");fixture(own);
        constexpr size_t table=40680,q=table+201*64;Bytes sg(table+203*64);
        put(sg,0,uint32_t(sg.size()));put(sg,4,213);put(sg,12,203);put(sg,16,202);
        put(sg,q+4,foreign?8:12800);put(sg,q+8,foreign?0:12800);put(sg,q+20,foreign?2:158,2);put(sg,q+22,foreign?2:80,2);put(sg,q+50,foreign?0:30,2);sg[q+55]=foreign?0:2;
        write(own/"DATA/China_Terrain.sg3",sg);write(own/"DATA/China_Terrain.555",Bytes(foreign?8:12800,0));
        AppMapSurface inspect(ResourceRoot::fromAbsolute(own.string()));need(inspect.select("Cities/own.map",map.terrain),"owned issue source selects");
        auto rejected=inspect.render(c);auto issues=inspect.diagnostics(rejected.stats);
        need(issues.totalUnknown==1&&issues.cells.size()==1&&issues.unlocated==0&&issues.cells[0].x==42&&issues.cells[0].y==42&&issues.cells[0].kind==(foreign?AppSurfaceIssueKind::BlockGeometryUnsupported:AppSurfaceIssueKind::GroupSideConflict),"geometry issue class and coordinates reflect the actual rejection");
    }
    const auto unused=root/"outside-index";fixture(unused);
    Bytes limited(40680+203*64);put(limited,0,uint32_t(limited.size()));put(limited,4,213);put(limited,12,203);put(limited,16,200);
    write(unused/"DATA/China_Terrain.sg3",limited);AppMapSurface outside(ResourceRoot::fromAbsolute(unused.string()));
    need(outside.select("Cities/own.map",map.terrain),"archive with absent referenced logical ID remains inspectable");
    const auto absent=outside.diagnostics(outside.render(c).stats);
    need(absent.cells.size()==1&&absent.unlocated==0&&absent.cells[0].kind==AppSurfaceIssueKind::IndexUnresolved&&absent.cells[0].x==42&&absent.cells[0].y==42,"invalid logical index is located without inventing geometry");
    candidateChecks(root);
    std::filesystem::rename(root/"assets-absent",root/"DATA");pakChecks(root);
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"original_data\":false}\n";
}
// App-path playback: every source save is driven through AppCityPlayback, the
// same controller the window uses. Strict premises must refuse by name at N=0;
// the labelled experiment premises must complete 100 admitted updates, keep
// pause/resume/step exact and redraw every admitted frame without decode
// failures. Host-clock values come from this process, never from the user.
void playback(const std::string& path){
    using namespace game_lab;
    const auto root=ResourceRoot::fromAbsolute(path);
    if(!root.configured())throw std::runtime_error("explicit existing data root required");
    AppCityPlaybackPreferences preferences;
    std::string error;
    need(readAppCityPlaybackPreferences(root,preferences,error),"Emperor.inf preference block");
    need(preferences.speed==80&&preferences.personalSpeed==80,"Emperor.inf speed 80");
    unsigned interval=0;
    need(AppCityPlayback::admissionIntervalMs(preferences.personalSpeed,interval)&&interval==37,"speed 80 cadence 37 ms");
    const uint32_t crtSeed=uint32_t(std::time(nullptr));
    const uint32_t farmerMs=0;
    const auto catalog=enumerateOriginalMapSources(root);
    size_t sessions=0,reached=0,strictRefusals=0,decodeFailures=0,replayChecks=0;
    unsigned maxN=0;size_t registryParsed=0,registryNoMode5=0,registryRecords=0,sessionReloads=0,continuousChecks=0;
    std::vector<std::string> rows;
    for(const auto& source:catalog.sources){
        if(source.kind!=OriginalMapSourceKind::SaveFile)continue;
        ++sessions;
        const auto save=loadOriginalSave(root.resolve(source.relativePath));
        std::vector<uint8_t> raw;
        need(detail::readFile(root.resolve(source.relativePath),4u*1024u*1024u,raw),"source bytes");
        const auto payload=decodeOriginalMapContainer(raw).payload;
        const auto messages=originalBuildingSavedScenarioMessages(payload,save.sizeOffset);
        need(messages.has_value(),"real saved scenario message table parsed");++registryParsed;registryRecords+=messages->count;
        OriginalCombinedState eventStage;eventStage.scenarioMessages=messages;
        // This isolates the registry stage, not a complete city update. Null
        // slots below are an explicit test premise, never supplied by the App.
        eventStage.process.eventSlots=std::array<uint32_t,8>{};
        const bool hasMode5=std::any_of(messages->records.begin(),messages->records.begin()+messages->count,
            [](const auto& row){return row[2]==5;});
        const auto eventRefusal=originalCombinedScenarioEvents(eventStage);
        need(hasMode5?eventRefusal=="scenario_manager_492430_4ed3b0":eventRefusal.empty(),"real registry determines manager branch without duplicate modes");
        if(!hasMode5)++registryNoMode5;
        OriginalMapCamera camera;camera.width=256;camera.height=192;camera.outlines=false;
        camera.centerX=camera.centerY=double(save.terrain.activeSide)/2;
        // Strict default: only documented categories, so the first update must
        // report the exact named missing input instead of inventing a value.
        AppMapSurface strictSurface(root);
        AppCitySessionState strictSession;strictSession.crtRandom=crtSeed;strictSession.admissionClock=1234;
        strictSession.cityClocks.value[0]=2;strictSession.wildlifeCounter=19;strictSession.randomHistory.cursor=7;
        AppCityPlayback strict(strictSurface,save,payload,preferences,crtSeed,farmerMs,false,&strictSession);
        need(strict.prepared(),"strict playback prepared");
        need(!strict.status().experiment,"strict default is not the experiment");
        strict.play();
        need(!strict.tick(1234),"load preserves admission clock before cadence");
        need(strict.status().state==AppCityPlaybackState::Playing,"too-early tick is not a refusal");
        need(!strict.tick(1234+interval),"strict premise refuses the first update");
        need(strict.status().state==AppCityPlaybackState::Refused,"strict state refused");
        need(strict.status().refusal=="event_ui_5eff20_not_closed","strict named refusal");
        need(strict.status().updates==0,"strict installs no update");
        need(strictSession.admissionClock==1234&&strictSession.cityClocks.value[0]==2&&strictSession.wildlifeCounter==19&&
            strictSession.randomHistory.cursor==7&&strictSession.crtRandom==crtSeed,"refused tick cannot commit partial process state");
        ++strictRefusals;
        // Labelled experiment: the same App code path plays the city.
        AppMapSurface surface(root);
        AppCitySessionState session;session.crtRandom=crtSeed;
        AppCityPlayback playback(surface,save,payload,preferences,crtSeed,farmerMs,true,&session,true);
        need(playback.prepared(),"experiment playback prepared");
        need(playback.status().experiment,"experiment premise labelled");
        need(playback.status().premise.find("实验前提")!=std::string::npos,"experiment premise shown");
        // One explicit step admits exactly one update.
        need(playback.step(0),"first step admitted");
        need(playback.status().updates==1&&playback.status().state==AppCityPlaybackState::Paused,"step is one update");
        auto view=playback.render(camera);
        need(view.image.valid(),"first admitted frame renders");
        need(view.timelineUpdates&&*view.timelineUpdates==1,"first frame shows N=1");
        decodeFailures+=playback.status().decodeFailures;
        // A pending play target must not turn a manual step back into Play.
        // The App stops its timer for Step; the controller must agree so that
        // both status and any later timer callback remain paused.
        playback.play(99);
        playback.pause();
        need(playback.step(interval),"step while an unfinished play target exists");
        need(playback.status().updates==2,"manual step advances exactly one update");
        need(playback.status().state==AppCityPlaybackState::Paused,"manual step stays paused before play target");
        need(!playback.tick(interval*2),"timer after manual step admits nothing");
        need(playback.status().updates==2,"timer after manual step keeps N");
        view=playback.render(camera);
        need(view.image.valid(),"manual step frame renders");
        // Pause keeps the picture and the timeline; ticks lose nothing.
        playback.play(18);
        uint64_t clock=interval;unsigned pausedAt=0;
        while(playback.status().state==AppCityPlaybackState::Playing&&playback.status().updates<20){
            clock+=interval;if(!playback.tick(clock))continue;
            view=playback.render(camera);need(view.image.valid(),"admitted frame renders");
        }
        playback.pause();
        pausedAt=playback.status().updates;
        need(pausedAt>=2,"playback advanced before pause");
        const auto pausedView=playback.render(camera);
        need(pausedView.image.valid(),"paused frame renders");
        for(unsigned extra=0;extra<5;++extra){clock+=interval;need(!playback.tick(clock),"paused tick admits nothing");}
        const auto stillView=playback.render(camera);
        need(playback.status().updates==pausedAt,"pause keeps N");
        need(stillView.image.width==pausedView.image.width&&stillView.image.height==pausedView.image.height&&
            stillView.image.pixelsBGRA==pausedView.image.pixelsBGRA,"pause keeps the picture");
        // Resume continues from the paused prefix, never from zero.
        need(playback.status().updates>=pausedAt,"resume keeps the prefix");
        playback.play(100-pausedAt);
        // Chained App playback must equal a full replay of the same admitted
        // frames through the existing selectPostLoadTimeline path.
        const auto checkReplay=[&](){
            const auto frames=playback.replayFrames();
            need(frames.size()==playback.status().updates,"one recorded frame per admitted update");
            AppMapSurface batch(root);
            need(batch.selectPostLoadTimeline(save,payload,frames,0,{}),"batch replay of the App frames");
            const auto chained=playback.render(camera);const auto replayed=batch.renderSavedBuildings(camera);
            need(chained.image.valid()&&replayed.image.valid()&&chained.image.width==replayed.image.width&&
                chained.image.height==replayed.image.height&&chained.image.pixelsBGRA==replayed.image.pixelsBGRA,
                "chained App frame equals batch replay pixels");
            need(surface.installedSceneDigest()==batch.installedSceneDigest(),"chained App scene equals batch replay scene");
            ++replayChecks;
        };
        checkReplay();
        while(playback.status().state==AppCityPlaybackState::Playing){
            clock+=interval;
            if(!playback.tick(clock))continue;
            view=playback.render(camera);need(view.image.valid(),"resumed frame renders");
            const unsigned n=playback.status().updates;
            if(n==22||n==51||n==100)checkReplay();
        }
        const auto finalStatus=playback.status();
        // Declared-input transport regression only. The normal adapter above
        // must still refuse; this does not establish original event semantics.
        AppMapSurface liveSurface(root);
        AppCityPlayback live(liveSurface,save,payload,preferences,crtSeed,farmerMs,true);
        need(live.prepared(),"continuous mechanics prepares");
        live.play();
        uint64_t liveClock=0;
        for(unsigned n=1;n<=98;++n){
            liveClock+=interval;
            if(!live.tick(liveClock)) throw std::runtime_error("continuous source="+std::to_string(sessions)+" expected_N="+std::to_string(n)+" actual_N="+std::to_string(live.status().updates)+" refusal="+live.status().refusal);
            need(live.status().updates==n&&live.status().state==AppCityPlaybackState::Playing,
                "continuous play remains active");
            need(live.replayFrames().empty(),"normal storage mode retains no frame journal");
        }
        live.pause();const auto digest=liveSurface.installedSceneDigest();
        liveClock+=10*interval;
        need(!live.tick(liveClock)&&live.status().updates==98&&liveSurface.installedSceneDigest()==digest,
            "continuous pause retains exact installed state");
        need(live.step(liveClock)&&live.status().updates==99&&live.status().state==AppCityPlaybackState::Paused,
            "step from continuous playback stays paused");
        liveClock+=interval;need(!live.tick(liveClock),"timer cannot resume after step");
        live.play();need(live.tick(liveClock)&&live.status().updates==100,"continuous resume keeps the prefix");
        liveClock+=interval;need(live.tick(liveClock)&&live.status().updates==101&&live.status().state==AppCityPlaybackState::Playing,
            "continuous play stays active beyond the old 100-update target");
        live.pause();need(live.replayFrames().empty(),"resume retains no replay history by default");
        need(live.render(camera).image.valid()&&live.status().decodeFailures==0,"continuous final frame renders");
        ++continuousChecks;
        const auto beforeReload=session;
        if(sessions==1){
            // Deliberately distinct process values make a reset-to-zero bug
            // observable. This is a declared-state wiring fixture only.
            auto marked=session;marked.watchMaximum=123;marked.inspectorMaximum={234,345};
            marked.storageSearch.fill(7);marked.industrySearch.fill(9);
            AppMapSurface heldSurface(root);
            AppCityPlayback held(heldSurface,save,payload,preferences,crtSeed,farmerMs,true,&marked);
            need(held.prepared(),"nonzero process fixture prepares");
            OriginalSceneTimelineFrame heldFrame;heldFrame.admission.now=marked.admissionClock;
            heldFrame.admission.speed=preferences.personalSpeed;heldFrame.animationClock=marked.admissionClock;
            const auto idle=heldSurface.advanceCityPlayback(heldFrame);const auto carried=heldSurface.cityPlaybackSession();
            need(idle.complete&&idle.updates==0&&carried&&carried->watchMaximum==123&&
                carried->inspectorMaximum==marked.inspectorMaximum&&carried->storageSearch==marked.storageSearch&&
                carried->industrySearch==marked.industrySearch&&carried->crtRandomCalls==marked.crtRandomCalls,
                "non-admitted load preserves actual maxima, caches and CRT count through context export");
        }
        AppMapSurface reloadSurface(root);
        AppCityPlayback reloaded(reloadSurface,save,payload,preferences,crtSeed,farmerMs,true,&session,true);
        need(reloaded.prepared(),"same process can load another saved city");
        need(reloaded.step(0),"reloaded city step uses carried admission clock");
        auto expectedClocks=beforeReload.cityClocks;advanceOriginalCityClocks(expectedClocks);
        need(session.cityClocks.value==expectedClocks.value&&session.admissionClock==beforeReload.admissionClock+interval,
            "successful reload preserves process city clocks and admission epoch");
        const auto expectedAnimation=originalAnimationClockFrame(beforeReload.animationClock,session.admissionClock);
        need(session.animationClock.last==expectedAnimation.last&&session.animationClock.flags==expectedAnimation.flags,
            "reload keeps nonzero animation clocks before applying current host time");
        need(session.crtRandomCalls>=beforeReload.crtRandomCalls,"reload never resets CRT consumption count");
        const auto reloadFrames=reloaded.replayFrames();
        need(reloadFrames.size()==1&&reloadFrames[0].combinedProcess->crtRandom==beforeReload.crtRandom&&
            reloadFrames[0].combinedProcess->randomHistory->ring==beforeReload.randomHistory.ring&&
            reloadFrames[0].combinedProcess->randomHistory->cursor==beforeReload.randomHistory.cursor&&
            reloadFrames[0].combinedProcess->wildlifeCounter==beforeReload.wildlifeCounter,
            "reload binds actual carried CRT, engine history and wildlife state");
        ++sessionReloads;
        need(finalStatus.state==AppCityPlaybackState::Paused||finalStatus.state==AppCityPlaybackState::Refused,"playback stopped");
        need(finalStatus.updates==100||!finalStatus.refusal.empty(),"playback reaches 100 or names its refusal");
        need(finalStatus.frames==finalStatus.updates,"one drawn frame per admitted update");
        need(playback.render(camera).image.valid(),"final frame renders");
        decodeFailures+=finalStatus.decodeFailures;
        need(finalStatus.decodeFailures==0,"no decode failures on the App path");
        maxN=std::max(maxN,finalStatus.updates);
        if(finalStatus.updates>=100&&finalStatus.refusal.empty())++reached;
        rows.push_back(std::string("{\"source\":")+std::string("\"")+source.relativePath+"\""+
            ",\"N\":"+std::to_string(finalStatus.updates)+",\"state\":\""+
            (finalStatus.state==AppCityPlaybackState::Paused?"paused":finalStatus.state==AppCityPlaybackState::Refused?"refused":"playing")+
            "\",\"refusal\":\""+finalStatus.refusal+"\",\"frames\":"+std::to_string(finalStatus.frames)+
            ",\"decode_failures\":"+std::to_string(finalStatus.decodeFailures)+"}");
    }
    need(sessions>0,"no SAV sources");
    need(reached==sessions,"every source save completes through the App path");
    need(strictRefusals==sessions,"every strict playback refuses by name");
    std::cout<<"{\"passed\":true,\"App_playback_sources\":"<<sessions
        <<",\"App_playback_reached_100\":"<<reached<<",\"App_playback_strict_refusals\":"<<strictRefusals
        <<",\"saved_scenario_registries\":"<<registryParsed<<",\"saved_scenario_records\":"<<registryRecords
        <<",\"saved_registry_no_mode5\":"<<registryNoMode5<<",\"mechanics_session_reloads\":"<<sessionReloads
        <<",\"continuous_transport_101\":"<<continuousChecks<<",\"continuous_retained_frames\":0"
        <<",\"App_playback_max_N\":"<<maxN<<",\"App_playback_decode_failures\":"<<decodeFailures
        <<",\"App_playback_replay_equal\":"<<replayChecks<<",\"App_playback_cadence_ms\":"<<interval
        <<",\"App_playback_rows\":[";
    for(size_t i=0;i<rows.size();++i){if(i)std::cout<<',';std::cout<<rows[i];}
    std::cout<<"]}\n";
}
void metrics(const std::string& path){
    using namespace game_lab;namespace fs=std::filesystem;const auto root=ResourceRoot::fromAbsolute(path);if(!root.configured())throw std::runtime_error("explicit existing data root required");
    AppMapSurface app(root);size_t cities=0,complete=0,images=0,nonzero=0,decoded=0,residue=0,excluded=0;
    std::string firstPartial;
    // Inspect unsupported geometry without assigning a guessed placement. Every
    // rejected cell is reclassified individually,not by a component's first reason.
    std::map<uint32_t,SG3Archive> residualArchives;
    std::set<uint32_t> decodedResidualWords;
    std::map<std::tuple<uint32_t,uint16_t,uint32_t,uint32_t>,size_t> residualClasses;
    size_t residualPayloadCells=0,residualClassified=0;
    size_t groupedCells=0,storedGroups=0,mirroredCells=0,cropTouchingCells=0;
    size_t markerMatches=0,controlCities=0,controlCells=0,controlMatches=0,controlGroups=0;
    size_t locatedIssues=0,unlocatedIssues=0,sideIssues=0,blockIssues=0;
    size_t previewCities=0,previewImages=0,candidateCells=0,candidateGroups=0,candidateDecoded=0,candidateFailed=0,combinedCells=0;
    std::map<std::pair<uint32_t,uint8_t>,size_t> controlMarkerFailures;
    std::string firstControlFailure;
    std::vector<std::string> cityConstraints;
    for(const auto& file:fs::directory_iterator(fs::path(root.configuredDirectory())/"Cities")){
        if(file.path().extension()!=".map")continue;
        const auto relative=fs::relative(file.path(),root.configuredDirectory()).generic_string();const auto absolute=root.resolve(relative);if(absolute.empty())throw std::runtime_error("MAP escapes root");
        auto map=loadOriginalMapTerrain(absolute);if(map.status==OriginalMapStatus::UnsupportedRole){++excluded;continue;}
        if(map.status!=OriginalMapStatus::LoadedTerrain||!app.select(relative,map.terrain))throw std::runtime_error("App MAP selection failed: "+relative);
        OriginalMapCamera c;c.centerX=c.centerY=double(map.terrain.activeSide)/2;c.width=256;c.height=192;
        auto out=app.render(c);if(!out.image.valid())throw std::runtime_error("App surface render failed: "+relative);
        auto cg=createCGImageFromPremultipliedBGRA(out.image.width,out.image.height,out.image.pixelsBGRA);if(!cg)throw std::runtime_error("CGImage failed: "+relative);CGImageRelease(cg);++images;++cities;
        const auto& s=out.stats;nonzero+=s.nonzeroCells;decoded+=s.decodedCells;residue+=s.geometryUnsupported+s.unresolvedIndices+s.archiveUnavailable+s.decodeFailures;
        const auto preview=app.renderPreview(c);const auto& candidate=preview.candidate;
        if(!preview.image.valid()||preview.verified.decodedCells!=s.decodedCells||preview.verified.nonzeroCells!=s.nonzeroCells)throw std::runtime_error("preview changed strict accounting");
        if(candidate.decodedCells+candidate.decodeFailureCells!=candidate.structuralCells||candidate.decodedCells>s.geometryUnsupported)throw std::runtime_error("candidate accounting overlaps strict acceptance");
        auto previewCG=createCGImageFromPremultipliedBGRA(preview.image.width,preview.image.height,preview.image.pixelsBGRA);if(!previewCG)throw std::runtime_error("preview CGImage failed");CGImageRelease(previewCG);++previewImages;
        candidateCells+=candidate.structuralCells;candidateGroups+=candidate.structuralGroups;candidateDecoded+=candidate.decodedCells;candidateFailed+=candidate.decodeFailureCells;
        combinedCells+=s.decodedCells+candidate.decodedCells;
        if(s.decodedCells+candidate.decodedCells==s.nonzeroCells)++previewCities;
        const auto diagnostics=app.diagnostics(s);locatedIssues+=diagnostics.cells.size();unlocatedIssues+=diagnostics.unlocated;
        if(diagnostics.cells.size()+diagnostics.unlocated!=diagnostics.totalUnknown)throw std::runtime_error("issue accounting is not exclusive");
        const auto active=analyzeOriginalMap(map.terrain);const auto issueState=diagnostics.cells.empty()?std::optional<OriginalTerrainState>():loadOriginalTerrainState(absolute);
        std::set<std::pair<uint32_t,uint32_t>> positions;
        for(const auto& issue:diagnostics.cells){
            if(!issueState||issue.x>=active.cropSide||issue.y>=active.cropSide||!positions.emplace(issue.x,issue.y).second)throw std::runtime_error("issue coordinate missing,duplicated or out of range");
            const size_t cell=size_t(issue.y+active.cropOrigin)*228+issue.x+active.cropOrigin;
            if((map.terrain.flags[cell]&0x80000u)||issueState->image[cell]!=issue.storedWord)throw std::runtime_error("issue coordinate does not bind the selected MAP");
            if(issue.kind==AppSurfaceIssueKind::GroupSideConflict)++sideIssues;if(issue.kind==AppSurfaceIssueKind::BlockGeometryUnsupported)++blockIssues;
        }
        if(s.decodedCells+s.geometryUnsupported+s.unresolvedIndices+s.archiveUnavailable+s.decodeFailures!=s.nonzeroCells)throw std::runtime_error("surface accounting is not exclusive: "+relative);
        if(s.decodedCells<s.nonzeroCells&&firstPartial.empty())firstPartial=relative;
        std::map<uint32_t,uint32_t> rejectedRecords;
        for(const auto& mismatch:s.compositeMismatches){
            const std::string reason=mismatch.reason;
            if(reason!="subTile group side != record cols"&&reason!="record does not declare an s*s block")
                throw std::runtime_error("residual classification needs an additional geometry reason");
            const auto word=(mismatch.tileset<<14)|(mismatch.recordId-201);
            const auto inserted=rejectedRecords.emplace(word,mismatch.cols);
            if(!inserted.second&&inserted.first->second!=mismatch.cols)throw std::runtime_error("inconsistent residual record geometry");
        }
        if(!rejectedRecords.empty()){
            const auto state=loadOriginalTerrainState(absolute);if(!state)throw std::runtime_error("residual state reload failed");
            const auto activity=analyzeOriginalMap(map.terrain);size_t classified=0;
            StoredGroupAudit groupAudit(map.terrain,*state);std::set<GroupKey> cityGroups;
            size_t cityGrouped=0,cityMirrored=0,cityCrop=0,cityConflict=0,citySingle=0,cityDrawBit6=0,cityMarkerMatches=0;
            for(uint32_t y=0;y<activity.cropSide;++y)for(uint32_t x=0;x<activity.cropSide;++x){
                const size_t cell=size_t(y+activity.cropOrigin)*228+x+activity.cropOrigin;
                if(map.terrain.flags[cell]&0x80000u)continue;
                const auto word=state->image[cell];const auto rejected=rejectedRecords.find(word);if(rejected==rejectedRecords.end())continue;
                const auto cols=rejected->second,side=uint32_t(state->subTile[cell]&15u)+1u;
                if(cols&&side==cols)continue; // accepted cells sharing the same record
                const auto slot=word>>14;auto found=residualArchives.find(slot);
                if(found==residualArchives.end()){
                    const auto leaf=mapImageGridArchiveName(slot);if(!leaf)throw std::runtime_error("unknown residual archive");
                    found=residualArchives.emplace(slot,loadSG3(root.resolve(std::string("DATA/")+leaf))).first;
                }
                const auto& archive=found->second;size_t index=(word&0x3fffu)+200;
                if(index>=archive.sprites.size())throw std::runtime_error("residual record out of bounds");
                if(archive.sprites[index].mirrorOffset)++cityMirrored;
                if(!decodedResidualWords.count(word)){
                    auto companion=fs::path("DATA")/mapImageGridArchiveName(slot);companion.replace_extension(".555");
                    const auto pixels=decodeSG3Sprite(archive,index,root.resolve(companion.generic_string()));
                    if(pixels.status!=SG3DecodeStatus::Decoded||!pixels.image.valid())throw std::runtime_error("residual payload does not decode");
                    decodedResidualWords.insert(word);
                }
                while(archive.sprites[index].mirrorOffset){const auto delta=archive.sprites[index].mirrorOffset;
                    if(delta>=0||int64_t(index)+delta<0)throw std::runtime_error("invalid residual mirror");index=size_t(int64_t(index)+delta);}
                ++residualClasses[{slot,archive.sprites[index].type,cols,cols?side:0}];++classified;++residualPayloadCells;
                if(groupAudit.complete(cell)){++cityGrouped;cityGroups.insert(storedGroupKey(*state,cell));}
                if(groupAudit.touchesInactive(cell))++cityCrop;
                if(cols)++cityConflict;
                if(side==1)++citySingle;
                if(state->draw[cell]&64)++cityDrawBit6;
                if(markerConstraint(*state,cell))++cityMarkerMatches;
            }
            if(classified!=s.geometryUnsupported)throw std::runtime_error("residual per-cell classification does not reconcile");
            residualClassified+=classified;
            groupedCells+=cityGrouped;storedGroups+=cityGroups.size();mirroredCells+=cityMirrored;cropTouchingCells+=cityCrop;
            markerMatches+=cityMarkerMatches;
            std::ostringstream row;row<<"{\"city\":"<<std::quoted(file.path().stem().string())<<",\"cells\":"<<classified
                <<",\"side_conflict_cells\":"<<cityConflict<<",\"complete_group_cells\":"<<cityGrouped<<",\"complete_groups\":"<<cityGroups.size()
                <<",\"mirrored_cells\":"<<cityMirrored<<",\"crop_touching_cells\":"<<cityCrop
                <<",\"singleton_cells\":"<<citySingle<<",\"draw_bit6_cells\":"<<cityDrawBit6
                <<",\"marker_constraint_cells\":"<<cityMarkerMatches<<'}';cityConstraints.push_back(row.str());
        }
        if(s.decodedCells==s.nonzeroCells){
            ++controlCities;const auto state=loadOriginalTerrainState(absolute);if(!state)throw std::runtime_error("control state reload failed");
            const auto activity=analyzeOriginalMap(map.terrain);StoredGroupAudit audit(map.terrain,*state);std::set<GroupKey> groups;
            for(uint32_t y=activity.cropOrigin;y<activity.cropOrigin+activity.cropSide;++y)
                for(uint32_t x=activity.cropOrigin;x<activity.cropOrigin+activity.cropSide;++x){const size_t cell=size_t(y)*228+x;
                    if((map.terrain.flags[cell]&0x80000u)||!state->image[cell]||!audit.complete(cell))continue;
                    ++controlCells;if(markerConstraint(*state,cell))++controlMatches;
                    else{++controlMarkerFailures[{uint32_t(state->subTile[cell]&15)+1,state->draw[cell]}];if(firstControlFailure.empty())firstControlFailure=relative;}
                    groups.insert(storedGroupKey(*state,cell));}
            controlGroups+=groups.size();
        }
        if(s.decodedCells==s.nonzeroCells&&!s.geometryUnsupported&&!s.unresolvedIndices&&!s.archiveUnavailable&&!s.decodeFailures)++complete;
    }
    if(!cities)throw std::runtime_error("no city MAP corpus");
    const auto catalog=enumerateOriginalMapSources(root);if(!catalog.complete)throw std::runtime_error("PAK catalog failed");
    size_t pakCities=0,pakStates=0,pakImages=0,pakNonzero=0,pakStrict=0,pakCandidate=0,pakComplete=0,pakStrictComplete=0,pakGroups=0;
    std::string firstPakLabel;size_t firstPakNonzero=0,firstPakStrict=0;
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::PakCity)continue;
        ++pakCities;const auto selected=loadOriginalMapSource(root,source);
        if(selected.surfaceState&&selected.surfaceState->terrain==selected.map.terrain.flags)++pakStates;
        if(!app.select(source,selected.map.terrain))throw std::runtime_error("PAK surface selection failed: "+source.label);
        OriginalMapCamera camera;camera.width=256;camera.height=192;camera.centerX=camera.centerY=double(selected.map.terrain.activeSide)/2;
        auto preview=app.renderPreview(camera);if(!preview.image.valid())throw std::runtime_error("PAK scene invalid");
        auto cg=createCGImageFromPremultipliedBGRA(preview.image.width,preview.image.height,preview.image.pixelsBGRA);if(!cg)throw std::runtime_error("PAK CGImage failed");CGImageRelease(cg);++pakImages;
        const auto& s=preview.verified;const auto& c=preview.candidate;
        if(s.decodedCells+s.geometryUnsupported+s.unresolvedIndices+s.archiveUnavailable+s.decodeFailures!=s.nonzeroCells||s.decodedCells+c.decodedCells>s.nonzeroCells)throw std::runtime_error("PAK scene accounting overlaps");
        pakNonzero+=s.nonzeroCells;pakStrict+=s.decodedCells;pakCandidate+=c.decodedCells;
        pakGroups+=c.structuralGroups;
        if(firstPakLabel.empty()){firstPakLabel=source.label;firstPakNonzero=s.nonzeroCells;firstPakStrict=s.decodedCells;}
        if(s.nonzeroCells&&s.decodedCells==s.nonzeroCells)++pakStrictComplete;
        if(s.nonzeroCells&&s.decodedCells+c.decodedCells==s.nonzeroCells)++pakComplete;
    }
    std::cout<<"{\"passed\":true,\"App_surface_sources\":"<<cities<<",\"native_CGImages\":"<<images<<",\"complete_storage_surfaces\":"<<complete<<",\"nonzero_cells\":"<<nonzero<<",\"decoded_cells\":"<<decoded<<",\"residue\":"<<residue<<",\"residual_classified_cells\":"<<residualClassified<<",\"residual_payload_decoded_cells\":"<<residualPayloadCells<<",\"residual_unique_decoded_records\":"<<decodedResidualWords.size()<<",\"residual_classes\":[";
    bool comma=false;for(const auto& entry:residualClasses){const auto& [slot,type,cols,side]=entry.first;
        if(comma)std::cout<<',';comma=true;
        std::cout<<"{\"tileset\":"<<slot<<",\"raw_type_flags\":"<<type<<",\"record_side\":"<<cols<<",\"subTile_side\":"<<side<<",\"cells\":"<<entry.second<<'}';}
    std::cout<<"],\"PAK_surface_sources\":"<<pakCities<<",\"PAK_surface_states\":"<<pakStates
        <<",\"PAK_scene_CGImages\":"<<pakImages<<",\"PAK_nonzero_cells\":"<<pakNonzero
        <<",\"PAK_strict_decoded_cells\":"<<pakStrict<<",\"PAK_candidate_decoded_cells\":"<<pakCandidate
        <<",\"PAK_complete_candidate_scenes\":"<<pakComplete<<",\"PAK_complete_strict_scenes\":"<<pakStrictComplete
        <<",\"PAK_candidate_groups\":"<<pakGroups<<",\"PAK_first_scene_label\":"<<std::quoted(firstPakLabel)
        <<",\"PAK_first_nonzero\":"<<firstPakNonzero<<",\"PAK_first_strict_decoded\":"<<firstPakStrict;
    std::cout<<",\"candidate_structural_cells\":"<<candidateCells<<",\"candidate_structural_groups\":"<<candidateGroups
        <<",\"candidate_decoded_cells\":"<<candidateDecoded<<",\"candidate_failed_cells\":"<<candidateFailed
        <<",\"combined_renderable_cells\":"<<combinedCells<<",\"complete_candidate_previews\":"<<previewCities
        <<",\"candidate_CGImages\":"<<previewImages<<",\"candidate_original_screen_accuracy_measured\":false"
        <<",\"located_issue_cells\":"<<locatedIssues<<",\"unlocated_issue_cells\":"<<unlocatedIssues
        <<",\"side_conflict_issue_cells\":"<<sideIssues<<",\"block_geometry_issue_cells\":"<<blockIssues
        <<",\"residual_complete_group_cells\":"<<groupedCells<<",\"residual_complete_groups\":"<<storedGroups
        <<",\"residual_mirrored_cells\":"<<mirroredCells<<",\"residual_crop_touching_cells\":"<<cropTouchingCells
        <<",\"residual_marker_constraint_cells\":"<<markerMatches
        <<",\"complete_city_marker_controls\":{\"cities\":"<<controlCities<<",\"complete_group_cells\":"<<controlCells
        <<",\"marker_constraint_cells\":"<<controlMatches<<",\"groups\":"<<controlGroups
        <<",\"first_failure_source\":"<<std::quoted(firstControlFailure)<<",\"failure_classes\":[";
    comma=false;for(const auto& entry:controlMarkerFailures){if(comma)std::cout<<',';comma=true;
        std::cout<<"{\"subTile_side\":"<<entry.first.first<<",\"draw\":"<<unsigned(entry.first.second)<<",\"cells\":"<<entry.second<<'}';}
    std::cout<<"]},\"residual_city_constraints\":[";
    std::sort(cityConstraints.begin(),cityConstraints.end());
    for(size_t i=0;i<cityConstraints.size();++i){if(i)std::cout<<',';std::cout<<cityConstraints[i];}
    std::cout<<"],\"excluded_non_city\":"<<excluded<<",\"first_partial_source\":"<<std::quoted(firstPartial)<<",\"selector_accuracy_measured\":false,\"original_screen_accuracy_measured\":false}\n";
}
}
int main(int argc,char** argv){try{if(argc==2&&std::string(argv[1])=="--metrics"){const char* root=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!root||!*root){std::cout<<"{\"skipped\":true}\n";return 0;}metrics(root);}
    else if(argc==2&&std::string(argv[1])=="--playback"){const char* root=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!root||!*root){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return 0;}playback(root);}
    else if(argc==2)authored(std::filesystem::absolute(argv[1]));else throw std::runtime_error("fixture directory or --metrics required");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
