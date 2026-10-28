// SPDX-License-Identifier: MIT
#pragma once
#include "native_session_source.hpp"
#include "native_house_runtime.hpp"
namespace game_lab {
// Explicit v8 source admission. Raw data stays local and immutable. The source
// scene is not resumed: a new native pool is constructed and labelled as such.
inline bool prepareNativeHouseRuntimeSource(const ResourceRoot& root,NativeSessionSource& source,std::string& error){
    error="runtime_source_unavailable (source SAV, Model/EmperorBuildingModels.txt, Model/EmperorFigureModels.txt, EmperorText.eng required)";std::vector<uint8_t> raw,buildingBytes,figureBytes,speechBytes;
    if(!detail::readFile(root.resolve(source.seed.source),4194304,raw)||
       !detail::readFile(root.resolve("Model/EmperorBuildingModels.txt"),1048576,buildingBytes)||
       !detail::readFile(root.resolve("Model/EmperorFigureModels.txt"),100000,figureBytes)||
       !detail::readFile(root.resolve("EmperorText.eng"),8388608,speechBytes))return false;
    const auto decoded=decodeOriginalSave(raw);const auto container=decodeOriginalMapContainer(raw);
    const std::string buildingText(buildingBytes.begin(),buildingBytes.end()),figureText(figureBytes.begin(),figureBytes.end());
    if(decoded.snapshotRevision!=source.seed.sourceRevision||nativeTextRevision(buildingText)!=source.seed.modelRevision||!container.complete){error="runtime_source_changed";return false;}
    const auto& save=source.save;const auto& payload=container.payload;
    if(!save.layers.ground||!save.layers.figures||!save.buildings.footprints.empty()){error="runtime_empty_source_required";return false;}
    const auto difficulty=originalFigureSavedDifficulty(payload,save.sizeOffset);
    const auto houseModels=difficulty?originalBuildingHouseModels(buildingText,*difficulty):std::nullopt;
    const auto figureModels=originalFigureModels(figureText,difficulty);const auto speeds=originalFigureModelSpeeds(figureText);
    const auto speech=originalBuildingSpeechCounts(speechBytes);const auto counters=originalBuildingSavedSpeechCounters(payload,save.terrainOffset);
    const auto employment=parseOriginalBuildingEmployment(buildingText);const auto spawn=originalFigureSpawnServices(payload,save.sizeOffset);
    auto figures=originalFigureUpdateContext(*save.layers.figures);auto services=originalFigureSavedServices(payload,save.terrainOffset);
    std::optional<bool> water;const auto festival=originalFigureSavedFestivalVenue(payload,save.terrainOffset,save.sizeOffset,&water);
    if(!houseModels||!figureModels||!speeds||!speech||!counters||!employment||!spawn||!figures||!services||!festival||!water){error="runtime_source_services_incomplete";return false;}
    const auto general=loadSG3(root.resolve("DATA/China_General.sg3")),terrain=loadSG3(root.resolve("DATA/China_Terrain.sg3")),elevation=loadSG3(root.resolve("DATA/China_Elevation.sg3"));
    const auto scene=composeOriginalSavedGroundScene(save.terrain,save.buildings,*save.layers.ground,general,terrain,elevation,0,&*employment);
    const auto passability=originalSceneMovementPassability(scene,save.buildings,*save.layers.ground,*spawn);
    if(!scene.accepted||!passability){error="runtime_source_scene_incomplete";return false;}
    figures->modelSpeeds=*speeds;figures->models=*figureModels;figures->terrain=save.terrain.flags;figures->cropBase=spawn->cropBase;figures->activeSide=save.terrain.activeSide;
    figures->passability=*passability;figures->movementMasks=passability->cells;figures->services=*services;
    figures->buildingOwners=save.layers.ground->owner;figures->randomByte=save.layers.ground->state.randomByte;figures->elevation=save.layers.ground->state.gBEBF3A;
    for(auto value:save.layers.mapperObjects)figures->feature.push_back(uint8_t(value));
    figures->buildings=originalFigureBuildingLinks(payload,save.buildings);figures->waterOverride=water;figures->festivalVenue=festival;
    figures->houseCapacities=originalFigureHouseCapacities(buildingText).value_or(std::map<int,int>{});
    figures->speechCounts=speech;figures->speechCounters=counters;figures->buildingEmployment=*employment;
    figures->inspectorReduction=originalFigureWatchtowerReduction(buildingText,124);
    figures->soundEffectsEnabled=false;figures->buildingEditMode=false;figures->roadblockGate=0;
    OriginalCombinedProcessInputs process;process.scenarioFigurePlacementCheck=false;process.forceHouseEvolution=false;process.soundEffectsEnabled=false;process.buildingEditMode=false;
    auto city=originalCombinedSavedState(*figures,save.buildings,payload,save.terrainOffset,save.sizeOffset,process);
    if(!city||!originalCombinedBindBuildingServices(*city,save.buildings,*save.layers.figures,payload,save.terrainOffset,save.sizeOffset,*save.layers.ground,scene,general,terrain,buildingText,speech)){error="runtime_building_services_incomplete";return false;}
    auto resources=std::make_shared<NativeHouseRuntimeResources>();auto& tpl=resources->strictTemplate;
    tpl.sourceRevision=source.seed.sourceRevision;tpl.modelRevision=source.seed.modelRevision;tpl.side=source.seed.side;
    tpl.cityGoodsSource=originalFigureSavedGoodsServices(payload,save.layers.figures->end,save.sizeOffset,save.terrainOffset);
    tpl.rebuiltSurface=save.layers.ground->state;tpl.rebuiltSurface.terrain=scene.rebuiltMovementTerrain;tpl.rebuiltSurface.image=scene.surface.image;tpl.rebuiltSurface.subTile=scene.surface.subTile;tpl.sourceProfile=save.buildings;
    for(uint16_t type:{2,3,4,5,7,33,72,124}){
        OriginalBuildingFootprint footprint;footprint.type=type;footprint.imageInputs.emplace();footprint.imageInputs->gridIndex=spawn->cropBase;
        const auto c=originalBuildingSavedUpdateContext(payload,save.terrainOffset,save.sizeOffset,footprint,OriginalBuildingUpdateScope::AdmittedTicks,true);
        if(!c){error="runtime_building_prototype_unavailable";return false;}tpl.contextPrototypes.emplace(type,*c);
    }
    // Fresh native pool, no retained original wildlife/routes. This is an
    // explicit adapter constructor, not a claim about original process reset.
    auto& f=city->figures;f.figures.clear();f.freeSlots.clear();f.cellHeads.assign(51984,0);f.highestLiveSlot=0;
    for(size_t slot=0;slot<2000;++slot){OriginalFigure dead;dead.slot=slot;f.figures.emplace(slot,dead);}
    for(size_t slot=1999;slot>0;--slot)f.freeSlots.push_back(slot);
    auto& sv=*f.services;sv.routeOwners.fill(0);sv.directions.assign(500000,0);sv.randomHistory=OriginalFigureRandomHistory{};
    sv.population=OriginalFigurePopulation{};sv.serviceGrid.assign(51984,0);sv.fireCount=0;sv.criminalCount=0;sv.allocationSerial=0;
    for(auto& impact:sv.formationImpacts){impact.alert=0;impact.source=0;}
    f.speechCounters=std::array<int32_t,359>{};city->buildingContext->figureSpeechCounters=f.speechCounters;
    city->buildingContext->figureServices=f.services;
    tpl.emptyCity=std::move(*city);
    for(unsigned slot:{14u,38u,39u,40u})resources->archives.emplace(slot,loadSG3(root.resolve(std::string("DATA/")+originalFigureArchiveName(slot))));
    auto seed=source.seed;const auto& c=*tpl.emptyCity.buildingContext;
    if(!c.cycle||!c.monthPart||!c.month||!c.buildingHookCountdown){error="runtime_source_clock_unknown";return false;}
    seed.sourceClock=NativeHouseClockSeed{tpl.emptyCity.phase,*c.cycle,*c.monthPart,*c.month,false};seed.waterOverride=water;seed.houseModels=*houseModels;
    const auto initial=startNativeHouseRuntime(seed,*resources,error);if(!initial)return false;
    source.seed=std::move(seed);source.runtime=std::move(resources);error.clear();return true;
}
} // namespace game_lab
