// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include "../cg_image_conversion.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <tuple>
#include <sstream>
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
void authored(const std::filesystem::path& root){
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
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"original_data\":false}\n";
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
    std::cout<<"{\"passed\":true,\"App_surface_sources\":"<<cities<<",\"native_CGImages\":"<<images<<",\"complete_storage_surfaces\":"<<complete<<",\"nonzero_cells\":"<<nonzero<<",\"decoded_cells\":"<<decoded<<",\"residue\":"<<residue<<",\"residual_classified_cells\":"<<residualClassified<<",\"residual_payload_decoded_cells\":"<<residualPayloadCells<<",\"residual_unique_decoded_records\":"<<decodedResidualWords.size()<<",\"residual_classes\":[";
    bool comma=false;for(const auto& entry:residualClasses){const auto& [slot,type,cols,side]=entry.first;
        if(comma)std::cout<<',';comma=true;
        std::cout<<"{\"tileset\":"<<slot<<",\"raw_type_flags\":"<<type<<",\"record_side\":"<<cols<<",\"subTile_side\":"<<side<<",\"cells\":"<<entry.second<<'}';}
    std::cout<<"],\"located_issue_cells\":"<<locatedIssues<<",\"unlocated_issue_cells\":"<<unlocatedIssues
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
    else if(argc==2)authored(std::filesystem::absolute(argv[1]));else throw std::runtime_error("fixture directory or --metrics required");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
