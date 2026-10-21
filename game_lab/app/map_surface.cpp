// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include "../original_pak.hpp"
#include <filesystem>
#include <set>
#include <sstream>
#include <algorithm>

namespace game_lab {
void AppMapSurface::clear() {
    state_.reset(); savedBuildings_.reset(); terrain_ = {}; error_.clear();
}
bool AppMapSurface::selectSavedBuildings(const OriginalMapTerrain& terrain,const OriginalBuildingProfile& profile,const OriginalSavedGround* ground,uint64_t tickMs,const OriginalFigureProfile* figures,OriginalFigureRenderSource figureSource,const OriginalFigureOrderedPass* ordered){
    clear();
    if(figureSource==OriginalFigureRenderSource::PostLoadK1&&(tickMs||!figures||!ordered||!ordered->complete||!ordered->context)){
        error_="post-load k=1 需要完整的一次有序人物更新";return false;}
    if(!archives_.count(2)){
        auto index=loadSG3(root_.resolve("DATA/China_General.sg3"));
        if((index.version!=213&&index.version!=214)||!index.numImages||index.numImages!=index.sprites.size()){
            error_="SAV 建筑图包不可用；保留数值占地";return false;
        }
        archives_.emplace(2,Archive{std::move(index),root_.resolve("DATA/China_General.555")});
    }
    std::vector<uint8_t> modelBytes;std::optional<OriginalBuildingEmployment> employment;
    if(detail::readFile(root_.resolve("Model/EmperorBuildingModels.txt"),1024u*1024u,modelBytes))
        employment=parseOriginalBuildingEmployment(std::string(modelBytes.begin(),modelBytes.end()));
    auto scene=composeOriginalBuildingScene(terrain,profile,archives_.at(2).index,tickMs,employment?&*employment:nullptr);
    if(ground) {
        for(uint32_t slot:{3u,16u})if(!archives_.count(slot)) {
            const std::string relative=std::string("DATA/")+mapImageGridArchiveName(slot);
            auto index=loadSG3(root_.resolve(relative));
            if(index.version!=213||!index.numImages||index.numImages!=index.sprites.size()) {
                error_="SAV 地面图包不可用";return false;
            }
            auto companion=std::filesystem::path(relative);companion.replace_extension(".555");
            archives_.emplace(slot,Archive{std::move(index),root_.resolve(companion.generic_string())});
        }
        scene=composeOriginalSavedGroundScene(terrain,profile,*ground,archives_.at(2).index,archives_.at(3).index,archives_.at(16).index,tickMs,employment?&*employment:nullptr);
    }
    if(!scene.accepted){error_="SAV 建筑表或活动框不完整；保留数值占地";return false;}
    if(figures){
        std::map<uint32_t,const SG3Archive*> figureArchives;
        // Startup slots, independent of saved image words. Forward selection
        // must have the same supplied archive domain when the oracle changes.
        for(const uint32_t slot:{14u,38u,39u,40u}){
            const char* name=originalFigureArchiveName(slot);
            if(!archives_.count(slot)){
                const std::string relative=std::string("DATA/")+name;auto index=loadSG3(root_.resolve(relative));
                if(index.version!=214||!index.numImages)continue;
                auto companion=std::filesystem::path(relative);companion.replace_extension(".555");
                archives_.emplace(slot,Archive{std::move(index),root_.resolve(companion.generic_string())});
            }
            figureArchives[slot]=&archives_.at(slot).index;
        }
        installOriginalFigureOverlays(scene,terrain,*figures,figureArchives,tickMs,figureSource,&profile,ordered);
        if(figureSource==OriginalFigureRenderSource::PostLoadK1&&!scene.figurePostLoadK1){error_="post-load k=1 人物池不完整";return false;}
    }
    terrain_=terrain;savedBuildings_=std::move(scene);return true;
}
bool AppMapSurface::selectPostLoadK1(const OriginalSaveResult& save,const std::vector<uint8_t>& payload){
    clear();
    // Explicit preview premise: speed80, previous admission0, fresh animation
    // clocks, one full-scene draw at the first admitted millisecond. This is
    // not inferred from SAV. Callers with observations use selectPostLoadTimeline.
    OriginalSceneTimelineFrame frame;frame.admission.speed=80;frame.continueAfterPass={false};
    do {++frame.admission.now;}while(!originalFigureAdmission(frame.admission).admitted);
    frame.animationClock=frame.admission.now;
    std::vector<uint8_t> bytes;if(!detail::readFile(root_.resolve("Model/EmperorBuildingModels.txt"),1048576,bytes))return false;
    const auto employment=parseOriginalBuildingEmployment(std::string(bytes.begin(),bytes.end()));if(!employment)return false;
    const auto general=loadSG3(root_.resolve("DATA/China_General.sg3"));
    const auto scene=composeOriginalBuildingScene(save.terrain,save.buildings,general,0,&*employment);if(!scene.accepted)return false;
    for(const auto& p:scene.placed)if(p.components.size()==1&&general.sprites[p.binding.physicalId-1].animationSpriteCount&&
        originalBuildingAnimationEnabled(p.footprint,&*employment)==true)frame.buildingDraws.push_back(p.footprint.slot);
    const unsigned origin=(228-save.terrain.activeSide)/2;
    for(unsigned diagonal=0;diagonal<2*save.terrain.activeSide-1;++diagonal)
        for(unsigned x=0;x<save.terrain.activeSide;++x)if(diagonal>=x&&diagonal-x<save.terrain.activeSide)
            frame.figureDrawCells.push_back((origin+diagonal-x)*228+origin+x);
    return selectPostLoadTimeline(save,payload,{frame},0,{});
}
bool AppMapSurface::selectPostLoadTimeline(const OriginalSaveResult& save,const std::vector<uint8_t>& payload,
    const std::vector<OriginalSceneTimelineFrame>& suppliedFrames,uint32_t previousAdmission,OriginalAnimationClock clocks){
    clear();
    if(save.status!=OriginalSaveStatus::LoadedSnapshot||!save.layers.figures||!save.layers.ground||!save.layers.ground->valid())return false;
    const auto& figures=*save.layers.figures;const auto& ground=*save.layers.ground;
    std::vector<uint8_t> bytes;
    if(!detail::readFile(root_.resolve("Model/EmperorFigureModels.txt"),100000,bytes))return false;
    const auto modelText=std::string(bytes.begin(),bytes.end());const auto models=originalFigureModels(modelText);
    const auto modelSpeeds=originalFigureModelSpeeds(modelText);
    if(!detail::readFile(root_.resolve("Model/EmperorBuildingModels.txt"),1048576,bytes))return false;
    const auto buildingText=std::string(bytes.begin(),bytes.end());
    const auto capacities=originalFigureHouseCapacities(buildingText),watchFloors=originalFigureHouseModelColumn(buildingText,15);
    const auto watchReduction=originalFigureWatchtowerReduction(buildingText);
    auto services=originalFigureSavedServices(payload,save.terrainOffset);const auto population=originalFigureSavedPopulation(payload,save.sizeOffset);
    const auto spawn=originalFigureSpawnServices(payload,save.sizeOffset);const auto contextSeed=originalFigureUpdateContext(figures);
    const auto buildings=originalFigureBuildingLinks(payload,save.buildings);const auto hunters=originalFigureHunterLinks(payload,save.buildings);
    const auto masks=originalFigureRoadMovementMasks(save.terrain,save.buildings);
    std::optional<bool> waterOverride;const auto festival=originalFigureSavedFestivalVenue(payload,save.terrainOffset,save.sizeOffset,&waterOverride);
    if(!models||!modelSpeeds||!capacities||!watchFloors||!watchReduction||!services||!population||!spawn||!contextSeed||!buildings||!hunters||!festival)return false;
    services->population=population;auto context=*contextSeed;context.houseCapacities=*capacities;context.buildingOwners=ground.owner;
    context.houseWatchFloors=*watchFloors;context.watchtowerReduction=watchReduction;context.elevation=ground.state.gBEBF3A;
    context.randomByte=ground.state.randomByte;
    //CRT holdrand is wall-clock seeded and absent from SAV;the default preview
    //supplies no state and refuses if a requested update reaches rand().
    context.waterOverride=waterOverride;
    context.watchMaximum=0; //explicit preview premise: fresh process scratchD95080.
    context.inspectorReduction=originalFigureWatchtowerReduction(buildingText,124);
    context.inspectorMaximum=std::array<int32_t,2>{0,0}; //explicit fresh-processD95084/88.
    context.crops=originalFigureSavedCropServices(payload,save.terrainOffset,save.sizeOffset);
    context.goods=originalFigureSavedGoodsServices(payload,figures.end,save.sizeOffset,save.terrainOffset);
    if(!context.goods)return false;
    context.goods->storageSearch=std::array<uint8_t,36>{}; //explicit fresh-process1E4EE08 premise.
    context.goods->industrySearch=std::array<uint8_t,36>{}; //explicit fresh-process10BC928 premise.
    const auto buildingEmployment=parseOriginalBuildingEmployment(buildingText);if(!buildingEmployment)return false;
    context.buildingEmployment=*buildingEmployment;
    for(unsigned col=9;col<=13;++col){const auto values=originalFigureHouseModelColumn(buildingText,col);if(!values)return false;context.houseMarketModels[col]=*values;}
    context.feature=ground.state.feature.empty()?std::vector<uint8_t>{}:std::vector<uint8_t>(ground.state.feature.size());
    for(size_t n=0;n<context.feature.size();++n)context.feature[n]=uint8_t(ground.state.feature[n]);
    context.activeSide=save.terrain.activeSide;context.festivalVenue=festival;context.entertainmentAvailable=spawn->entertainmentAvailable;
    const auto passability=originalFigurePheasantPassability(save.terrain,save.layers.mapperStone,save.buildings,*spawn);
    std::map<uint32_t,SG3Archive> owned;std::map<uint32_t,const SG3Archive*> refs;
    for(unsigned slot:{14u,38u,39u,40u}){owned.emplace(slot,loadSG3(root_.resolve(std::string("DATA/")+originalFigureArchiveName(slot))));refs[slot]=&owned.at(slot);}
    if(!selectSavedBuildings(save.terrain,save.buildings,&ground,0,&figures))return false;
    const auto fullPassability=originalSceneMovementPassability(*savedBuildings_,save.buildings,ground,*spawn);
    const auto* timelinePassability=fullPassability?&*fullPassability:passability?&*passability:nullptr;
    const auto initial=originalFigurePostLoadContext(figures,refs,save.terrain.flags,*modelSpeeds,&*services,timelinePassability,
        &*hunters,&*buildings,*models,spawn->cropBase,fullPassability?fullPassability->cells:masks?*masks:std::vector<uint16_t>{},&context);
    if(!initial.context)return false;
    const auto employment=parseOriginalBuildingEmployment(std::string(bytes.begin(),bytes.end()));
    if(!employment){clear();return false;}
    const auto timeline=originalSceneTimeline(*initial.context,refs,*savedBuildings_,archives_.at(2).index,&*employment,previousAdmission,clocks,suppliedFrames);
    if(!timeline.complete||!installOriginalSceneTimeline(*savedBuildings_,save.terrain,figures,refs,save.buildings,archives_.at(2).index,timeline)){
        clear();error_="post-load timeline: "+timeline.refusal+" (update "+std::to_string(timeline.refusedUpdate)+")";return false;}
    return true;
}
OriginalBuildingSceneView AppMapSurface::renderSavedBuildings(const OriginalMapCamera& camera,int bit)const{
    if(!savedBuildings_||!archives_.count(2))return {};
    const auto& archive=archives_.at(2);
    std::vector<MapImageGridTileset> groundTilesets;
    if(savedBuildings_->groundRebuilt)for(uint32_t slot:{3u,16u})if(archives_.count(slot)) {
        const auto& a=archives_.at(slot);groundTilesets.push_back({slot,&a.index,a.companion,201});
    }
    for(uint32_t slot:{14u,38u,39u,40u})if(archives_.count(slot)){
        const auto& a=archives_.at(slot);groundTilesets.push_back({slot,&a.index,a.companion,201});
    }
    return renderOriginalBuildingScene(terrain_,*savedBuildings_,archive.index,archive.companion,camera,bit,groundTilesets);
}
bool AppMapSurface::select(const std::string& relativeMap, const OriginalMapTerrain& expected) {
    clear();
    const auto path = root_.resolve(relativeMap);
    if (path.empty() || !expected.valid()) { error_ = "MAP 来源不可用"; return false; }
    const auto map = loadOriginalMapTerrain(path);
    if (map.status != OriginalMapStatus::LoadedTerrain || map.terrain.activeSide != expected.activeSide || map.terrain.flags != expected.flags) {
        error_ = "MAP 来源已变化或活动框不一致"; return false;
    }
    return adoptState(expected,loadOriginalTerrainState(path));
}
bool AppMapSurface::select(const OriginalMapSource& source,const OriginalMapTerrain& expected){
    if(source.kind==OriginalMapSourceKind::MapFile)return select(source.relativePath,expected);
    clear();
    if(source.kind!=OriginalMapSourceKind::PakCity){error_="该来源没有已确认的存储地表工作集";return false;}
    auto selected=loadOriginalMapSource(root_,source);
    if(selected.map.status!=OriginalMapStatus::LoadedTerrain||selected.map.terrain.activeSide!=expected.activeSide||selected.map.terrain.flags!=expected.flags||!selected.surfaceState){
        error_="包内城市地表缺失、来源已变化或活动框不一致";return false;
    }
    return adoptState(expected,std::move(selected.surfaceState));
}
bool AppMapSurface::adoptState(const OriginalMapTerrain& expected,std::optional<OriginalTerrainState> state){
    if (!expected.valid() || !state || !state->valid() || state->terrain != expected.flags) {
        error_ = "MAP 存储地表与当前地形来源不一致或数据不完整"; return false;
    }
    std::set<uint32_t> required;
    for (const auto word : state->image) if (word) required.insert(word >> 14);
    for (const auto slot : required) {
        if (archives_.count(slot)) continue;
        const auto leaf = mapImageGridArchiveName(slot);
        if (!leaf) continue; // Renderer counts unavailable namespaces explicitly.
        const std::filesystem::path relative = std::filesystem::path("DATA") / leaf;
        const auto archivePath = root_.resolve(relative.generic_string());
        if (archivePath.empty()) continue;
        auto index = loadSG3(archivePath);
        if (index.version != 213 || !index.numImages || index.numImages != index.sprites.size()) continue;
        auto companion = relative; companion.replace_extension(".555");
        archives_.emplace(slot, Archive{std::move(index), root_.resolve(companion.generic_string())});
    }
    terrain_ = expected; state_ = std::move(state); return true;
}
MapImageGridResult AppMapSurface::render(const OriginalMapCamera& camera) const {
    if (!state_) return {};
    std::vector<MapImageGridTileset> tilesets;
    for (const auto& entry : archives_) tilesets.push_back({entry.first, &entry.second.index, entry.second.companion, 201});
    auto pixels = camera; pixels.outlines = false;
    return renderMapImageGridViewport(terrain_, *state_, tilesets, pixels);
}
AppSurfaceDiagnostics AppMapSurface::diagnostics(const MapImageGridStats& stats) const {
    AppSurfaceDiagnostics out;
    out.totalUnknown=stats.geometryUnsupported+stats.archiveUnavailable+stats.unresolvedIndices+stats.decodeFailures;
    out.unlocated=out.totalUnknown;
    if(!state_||!out.totalUnknown)return out;
    const auto activity=analyzeOriginalMap(terrain_);
    if(!activity.cropValid())return out;
    std::map<uint32_t,uint32_t> geometry;
    for(const auto& mismatch:stats.compositeMismatches){
        if(mismatch.recordId<201)continue;
        const std::string reason=mismatch.reason;
        if(reason=="subTile group side != record cols"||reason=="record does not declare an s*s block")
            geometry.emplace((mismatch.tileset<<14)|(mismatch.recordId-201),mismatch.cols);
    }
    size_t locatedGeometry=0,locatedArchive=0,locatedIndex=0;
    for(uint32_t y=0;y<activity.cropSide;++y)for(uint32_t x=0;x<activity.cropSide;++x){
        const size_t cell=size_t(y+activity.cropOrigin)*228+x+activity.cropOrigin;
        if(terrain_.flags[cell]&0x80000u)continue;
        const auto word=state_->image[cell];if(!word)continue;
        const auto slot=word>>14;const auto archive=archives_.find(slot);
        AppSurfaceIssue issue{x,y,word,0,uint32_t(state_->subTile[cell]&15)+1,AppSurfaceIssueKind::BlockGeometryUnsupported};
        if(archive==archives_.end()){issue.kind=AppSurfaceIssueKind::ArchiveUnavailable;++locatedArchive;}
        else if(size_t((word&0x3fffu)+201)>archive->second.index.numImages){issue.kind=AppSurfaceIssueKind::IndexUnresolved;++locatedIndex;}
        else{
            const auto found=geometry.find(word);if(found==geometry.end())continue;
            issue.recordSide=found->second;
            if(issue.recordSide){
                if(issue.groupSide==issue.recordSide)continue; // same-record accepted cells stay absent
                issue.kind=AppSurfaceIssueKind::GroupSideConflict;
            }
            ++locatedGeometry;
        }
        out.cells.push_back(issue);
    }
    // A newer/unrecognised renderer reason must not acquire guessed coordinates.
    // Decode failures and other geometry remain explicitly unlocated.
    if(locatedGeometry>stats.geometryUnsupported||locatedArchive>stats.archiveUnavailable||locatedIndex>stats.unresolvedIndices){
        out.cells.clear();return out;
    }
    out.unlocated=out.totalUnknown-out.cells.size();return out;
}
std::string describeAppSurfaceDiagnostics(const AppSurfaceDiagnostics& diagnostics,size_t maxRows){
    std::ostringstream text;
    text<<"未覆盖 "<<diagnostics.totalUnknown<<" 格；已定位 "<<diagnostics.cells.size()<<" 格；未定位 "<<diagnostics.unlocated<<" 格。";
    if(!diagnostics.totalUnknown){text<<"\n本源非零存储格无遗漏；这不是完整原版画面对照。";return text.str();}
    text<<"\n下列为0基活动格坐标；将 X/Y 输入相机可定位。分类不证明原版对象语义或屏幕锚点。";
    const size_t shown=std::min(maxRows,diagnostics.cells.size());
    for(size_t i=0;i<shown;++i){const auto& issue=diagnostics.cells[i];
        const char* label=issue.kind==AppSurfaceIssueKind::GroupSideConflict?"组边长冲突":
            issue.kind==AppSurfaceIssueKind::BlockGeometryUnsupported?"不符合当前块几何":
            issue.kind==AppSurfaceIssueKind::ArchiveUnavailable?"缺少图包":"索引未解析";
        text<<"\n活动("<<issue.x<<','<<issue.y<<") · "<<label<<" · 图包 "<<(issue.storedWord>>14)<<" / 图像 "<<((issue.storedWord&0x3fffu)+201);
        if(issue.kind==AppSurfaceIssueKind::GroupSideConflict)text<<" · 存储组边长 "<<issue.groupSide<<" / 图像块边长 "<<issue.recordSide;
    }
    if(shown<diagnostics.cells.size())text<<"\n显示前 "<<shown<<'/'<<diagnostics.cells.size()<<" 格（列表预算上限）；其余仍未覆盖。";
    if(diagnostics.unlocated)text<<"\n另有 "<<diagnostics.unlocated<<" 格未定位（含解码失败或尚不支持的几何原因）；不能把它们当作已覆盖。";
    return text.str();
}
}
