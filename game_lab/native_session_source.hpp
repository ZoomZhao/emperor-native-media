// SPDX-License-Identifier: MIT
#pragma once
#include "native_session.hpp"
#include "native_session_interaction.hpp"
#include "resource_root.hpp"
#include "original_building_update.hpp"
#include "original_building_scene.hpp"

namespace game_lab {
struct NativeSessionSource { NativeSessionSeed seed; OriginalSaveResult save; };
// Metadata is observational only; it never relaxes admission or invents defaults.
struct NativeSourceMetadata {uint32_t side=0;std::optional<int64_t> funds;};
inline std::optional<NativeSessionSource> loadNativeSessionSource(const ResourceRoot& root,const std::string& relative,std::string& error,NativeSourceMetadata* metadata=nullptr) {
    if(metadata)*metadata={};
    error="文件名不受会话格式支持；需要有名称的小写 .sav 文件。";
    if(std::filesystem::path(relative).extension()!=".sav")return std::nullopt;
    std::vector<uint8_t> raw,modelBytes;
    error="来源不可读取、越出数据目录，或超过4 MiB上限。";
    if(!detail::readFile(root.resolve(relative),4194304,raw))return std::nullopt;
    error="缺少可读取的原始建筑模型，或模型超过1 MiB上限。";
    if(!detail::readFile(root.resolve("Model/EmperorBuildingModels.txt"),1048576,modelBytes))return std::nullopt;
    auto save=decodeOriginalSave(raw);const auto payload=decodeOriginalMapContainer(raw);
    error="SAV快照或容器不完整，暂不能创建原生会话。";
    if(save.status!=OriginalSaveStatus::LoadedSnapshot||!payload.complete)return std::nullopt;
    error="地形或地表状态不完整，暂不能创建原生会话。";
    if(!save.layers.ground||!save.layers.ground->valid()||!analyzeOriginalMap(save.terrain).cropValid())return std::nullopt;
    if(metadata)metadata->side=save.terrain.activeSide;
    OriginalBuildingUpdateContext city(OriginalBuildingUpdateScope::AdmittedTicks,true); // read saved scalars only
    error="存档的人口或资金字段无法确认。";
    if(!originalBuildingSavedCityContext(payload.payload,save.sizeOffset,city)||!city.populationTotal||!city.treasury)return std::nullopt;
    if(metadata)metadata->funds=*city.treasury;
    error="原始建筑表不完整，暂不能创建原生会话。";
    if(save.buildings.status!=OriginalBuildingStatus::Complete||save.buildings.parsedRecords!=4000)return std::nullopt;
    error="已有原版建筑或居民；本入口仅支持空城，原版续玩尚不兼容。";
    if(!save.buildings.footprints.empty()||!save.buildings.objectBuildings.empty()||!save.buildings.unplacedMarkers.empty()||
        !save.buildings.liveTypeCounts.empty()||*city.populationTotal)return std::nullopt;
    error="原始资金为负，不能建立有效原生会话。";
    if(*city.treasury<0)return std::nullopt;
    const std::string text(modelBytes.begin(),modelBytes.end());
    // Explicit native v1 policy: normal-difficulty model prices, row0 capacity.
    // These are data values; migration timing/road adjacency are authored rules.
    const auto models=originalBuildingModels(text,2);
    const auto caps=originalFigureHouseCapacities(text);
    error="原始模型缺少住宅、道路价格或住宅容量。";
    if(!models||!caps||!models->count(2)||!models->count(22)||!caps->count(0))return std::nullopt;
    NativeSessionSeed seed;seed.source=relative;seed.sourceRevision=save.snapshotRevision;seed.modelRevision=nativeTextRevision(text);
    seed.side=save.terrain.activeSide;seed.initialFunds=*city.treasury;seed.houseCost=models->at(2)[0];seed.roadCost=models->at(22)[0];seed.houseCapacity=caps->at(0);
    if(models->count(72))seed.wellCost=models->at(72)[0]; // named original Well row, Normal-price column
    if(models->count(33))seed.hunterCost=models->at(33)[0]; // original Hunter's Tent, Normal construction price
    // v6 labor inputs: saved difficulty/wage band/health (4AD850 share) and
    // saved category priorities; employment is model column5. Saved military
    // formation labor would reduce the pool, so any nonzero value leaves v6 unavailable.
    if(city.labor&&city.cityEmployment&&models->count(33)&&models->count(72)&&
       std::all_of(city.labor->formationLabor.begin(),city.labor->formationLabor.end(),[](int32_t v){return v==0;})){
        const auto percent=originalBuildingLaborPercent(city.labor->difficulty,city.labor->wageBand,city.labor->health);
        if(percent&&*percent>0&&models->at(33)[5]>0&&models->at(72)[5]>=0){seed.laborPercent=uint32_t(*percent);
            seed.hunterWorkers=uint32_t(models->at(33)[5]);seed.wellWorkers=uint32_t(models->at(72)[5]);
            for(unsigned n=0;n<10;++n)seed.laborPriority[n]=city.cityEmployment->rows[n].priority;}}
    const auto& ground=*save.layers.ground;const unsigned origin=(228-seed.side)/2;
    for(unsigned y=0;y<seed.side;++y)for(unsigned x=0;x<seed.side;++x){const size_t at=size_t(y+origin)*228+x+origin;const auto flags=save.terrain.flags[at];
        // Conservative native construction domain, not original placement rules:
        // flat unowned land/fertile land only; unknown bits, vegetation, water,
        // elevation, objects and all other original state are unavailable.
        seed.buildable.push_back((flags&~0x80u)==0&&ground.owner[at]==0&&save.layers.mapperObjects[at]==0&&ground.state.gBEBF3A[at]==0);
        seed.sourceRoad.push_back((flags&0x40u)&&!(flags&0x80000u));
    }
    if(!validNativeSeed(seed)){error="原模型价格/容量或源身份无效。";return std::nullopt;}
    error.clear();return NativeSessionSource{std::move(seed),std::move(save)};
}
struct NativeSessionChoice {std::string source;uint32_t side=0;int64_t funds=0;};
inline std::vector<NativeSessionChoice> nativeSessionChoices(const ResourceRoot& root,std::vector<std::string> paths){
    std::sort(paths.begin(),paths.end());paths.erase(std::unique(paths.begin(),paths.end()),paths.end());
    std::vector<NativeSessionChoice> result;std::string error;
    for(const auto& path:paths){const auto source=loadNativeSessionSource(root,path,error);
        if(source&&validateNativeSession(source->seed,startNativeSession(source->seed,5)).empty())result.push_back({path,source->seed.side,source->seed.initialFunds});}
    return result;
}
struct NativeSessionScene { OriginalMapTerrain terrain; OriginalBuildingProfile buildings; OriginalSavedGround ground; };
inline std::optional<NativeSessionScene> nativeSessionScene(const NativeSessionSource& source,const NativeSessionState& state) {
    if(!validateNativeSession(source.seed,state).empty()||!source.save.layers.ground)return std::nullopt;
    NativeSessionScene out{source.save.terrain,source.save.buildings,*source.save.layers.ground};
    const unsigned origin=(228-source.seed.side)/2;
    for(const auto& o:state.objects){const unsigned side=nativeObjectSide(o.kind);
        for(unsigned y=o.y;y<o.y+side;++y)for(unsigned x=o.x;x<o.x+side;++x){const size_t cell=size_t(y+origin)*228+x+origin;
            if(o.kind==NativeObjectKind::Road){out.terrain.flags[cell]|=0x40u;out.ground.state.roadDistance[cell]=255;}
            else{out.terrain.flags[cell]|=8u;out.ground.owner[cell]=uint16_t(o.id);}}
        if(o.kind!=NativeObjectKind::Road){
            OriginalBuildingFootprint f;f.type=o.kind==NativeObjectKind::Well?72:o.kind==NativeObjectKind::Hunter?33:o.residents?3:2;f.slot=size_t(o.id);f.x=o.x;f.y=o.y;f.size=2;
            // A new native-owned Well deliberately uses the basic Well image,
            // not an inferred original saved service/upgrade runtime state.
            if(o.kind==NativeObjectKind::Well){f.wellImageInputs=OriginalWellImageInputs{0,0};++out.buildings.namedWellRecords;}
            else if(o.kind==NativeObjectKind::House){
            f.imageInputs.emplace();f.imageInputs->gridIndex=(o.y+origin)*228+o.x+origin;f.imageInputs->randomByte=out.ground.state.randomByte[f.imageInputs->gridIndex];
            f.imageInputs->houseByte3c=o.residents?1:0;f.imageInputs->word20=int16_t(o.residents);f.imageInputs->savedView=0;}
            out.buildings.footprints.push_back(f);out.buildings.slotTypes[f.slot]=f.type;++out.buildings.liveTypeCounts[f.type];++out.buildings.liveSizedRecords;
        }
    }
    out.ground.state.terrain=out.terrain.flags;return out;
}
} // namespace game_lab
