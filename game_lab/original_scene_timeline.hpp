#pragma once
#include "original_building_scene.hpp"
#include "original_building_update.hpp"
#include "original_city_clocks.hpp"
#include "original_post_figure_pass.hpp"

namespace game_lab {
// Same owned-exe hash as original_city_clocks.hpp; all VAs map to file VA-400000.
// No member below is inferred from a SAV or implicitly zeroed by the library.
struct OriginalCombinedProcessInputs {
    std::optional<OriginalCityClocks> cityClocks;
    std::optional<OriginalFigureRandomHistory> randomHistory;
    std::optional<uint8_t> eventGate17,eventGate16; //53C720/53C710
    std::optional<uint32_t> cityMode; //88EC38; ordinary combined mode requires0
    std::optional<int32_t> schedulerGate16; //408AC0->4FAE50
    std::optional<std::array<uint32_t,8>> eventSlots; //A98370+259920
    std::optional<uint8_t> eventProgress; //A98370+259952
    std::optional<std::vector<uint8_t>> eventRecordModes; //C75090+10, stride268
    std::optional<uint8_t> managerGate8; //425250; only needed for mode5
    std::optional<uint32_t> wildlifeCounter,wildlifePeriod; //12A6E78/859DA0
    std::optional<uint8_t> wildlifePause,wildlifeMode; //10DE0D0/10DE045
    std::optional<std::array<int32_t,11>> serviceCoverage; //C82D58, preserved columns4..6
    std::optional<bool> shortageNotice,unemploymentNotice,religionBonusEnabled;
    std::optional<int32_t> religionPreviousRating,religionSelected,migrantSpacing;
    std::optional<bool> scenarioFigurePlacementCheck;
    std::optional<uint32_t> highestEverBuildingSlot;
    std::optional<std::array<int32_t,6>> producerTail,staffedProducerTail; //phase33 preserves resource30..35
    std::optional<bool> forceHouseEvolution,alternateMode; //10BFF78/10DE044
    std::optional<uint32_t> effectDecayCounter,wallClock; //101D12C/766DA0 caller observation
    std::optional<std::array<int32_t,14>> cropDispatchModel; //1031418
    std::optional<std::array<int32_t,2>> cropDispatchRadius; //84C558/54
    std::optional<bool> effectPlacementCulled; //417460 viewport observation
    std::optional<unsigned> visualEffectCount,windDirection; //8900E8/C82F2C
    std::optional<int32_t> previousHealthRating; //D6241C
    std::optional<uint32_t> crtRandom; //explicit7665FD state,independent of766DA0
    std::optional<bool> soundEffectsEnabled,buildingEditMode; //10DE0D3/D62408 caller inputs
    std::optional<uint8_t> annualDemandResponseMode; //C7EDD0->10DE164,4A3A9A..AC caller preference
};
struct OriginalCombinedBuilding {
    OriginalBuildingUpdateState state;
    OriginalBuildingUpdateContext context;
};
struct OriginalCombinedState {
    OriginalFigureUpdateContext figures;
    std::map<size_t,OriginalCombinedBuilding> buildings;
    OriginalCombinedProcessInputs process;
    std::optional<OriginalBuildingUpdateContext> buildingContext;
    std::vector<OriginalPostFigureRegistryObject> postFigureRegistry;
    std::optional<OriginalPostFigurePassResult> postFigureResult;
    std::optional<std::vector<uint32_t>> rebuiltMovementTerrain; //53D630 before41F400,never snapshot-restored flags
    std::optional<std::array<OriginalBuildingAnnualOffer,22>> annualOffers;
    std::optional<OriginalBuildingScenarioMessages> scenarioMessages;
    uint32_t phase=0;
};
//5AD440 B7C projection with no buildings after SAV load. 534E5B->41F400
//creates only byte18=1 objects (41F820->41F350->41F25F),which5AD7AF skips.
//Own river cells are conservatively withheld; a known path proves the exit.
//No distance or full movement mask is published from this partial proof.
inline std::optional<bool> originalCombinedEmptyCityRoadExit(const OriginalCombinedState& city){
    const auto& f=city.figures;if(!city.buildingContext||!city.buildings.empty()||!f.cropBase||!f.activeSide||
        !*f.activeSide||*f.activeSide>228||*f.cropBase!=((228-*f.activeSide)/2)*229||
        !f.buildings||!city.rebuiltMovementTerrain||city.rebuiltMovementTerrain->size()!=51984||f.buildingOwners.size()!=51984||
        std::any_of(f.buildingOwners.begin(),f.buildingOwners.end(),[](uint16_t v){return v!=0;})||
        std::any_of(f.buildings->begin(),f.buildings->end(),[](const auto& e){return e.second.type!=0;})||
        !city.buildingContext->roadEndpoints)return std::nullopt;
    std::vector<uint16_t> masks(51984);const auto& endpoints=*city.buildingContext->roadEndpoints;
    for(unsigned y=0;y<*f.activeSide;++y)for(unsigned x=0;x<*f.activeSide;++x){const size_t cell=*f.cropBase+228*y+x;
        if(cell>=51984)return std::nullopt;const uint32_t t=city.rebuiltMovementTerrain->at(cell),plain=t&0xafffde6f;
        if(t&0x100)continue;
        if(t&0x800){masks[cell]=t&0x40?4:0;continue;}
        if((t&0x80000)||(t&0x10004)==0x10004)continue;
        if((t&0x400)&&(t&0x40)&&!(t&0x800000))masks[cell]=0x100;
        else if(t&0x40)masks[cell]=4;
        else if(t&(0x400|0x20))masks[cell]=0x10;
        else if((t&0x800000)&&(t&0x200)&&!(t&0x400000))masks[cell]=0x400;
        else if(t&0x200)continue;
        else if(t&0x20000)continue; //road-distance / pointer projection not closed
        else if(t&0x10000000)masks[cell]=0x20;
        else if(plain&0x8008)return std::nullopt;
        else if(t&0x10000)continue;
        else if(!plain||((plain&0x80061001)&&(!(plain&0x4000)||(plain&0x1000))))masks[cell]=0x10;
    }
    const int64_t entry=int64_t(*f.cropBase)+endpoints[0]+228*int64_t(endpoints[1]);
    const int64_t exit=int64_t(*f.cropBase)+endpoints[2]+228*int64_t(endpoints[3]);
    if(entry<0||entry>=51984||exit<0||exit>=51984)return std::nullopt;
    const auto distances=originalBuildingRoadDistances(masks,uint32_t(entry));
    return distances?std::make_optional(distances->at(size_t(exit))!=0):std::nullopt;
}
// Checks the ENTIRE phase, including work outside the per-building API.
// 0/49 have no case; empty table cases target4AC4C8.50 tails into4AC650.
inline std::string originalCombinedPhaseBlocker(uint32_t phase){
    switch(phase){
    case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9:case 10:case 11:
    case 12:case 13:case 14:case 15:case 16:case 17:case 18:case 19:case 20:
    case 21:case 22:case 23:case 24:case 25:case 26:case 27:case 28:case 29:case 30:case 31:
    case 32:case 33:case 34:case 35:case 36:case 37:case 38:case 39:case 40:case 41:case 42:
    case 43:case 44:case 45:case 46:case 47:case 49:case 50:return {};
    case 48:return {}; //closed common byte52 countdown; no figure-link consumer
    default:return "unported_city_phase_"+std::to_string(phase)+"_4ac2b0";
    }
}
// Construct the city/building half from the same validated save. Process inputs
// remain caller-owned. In particular no city-clock initial value is supplied here.
inline std::optional<OriginalCombinedState> originalCombinedSavedState(const OriginalFigureUpdateContext& figures,
    const OriginalBuildingProfile& profile,const std::vector<uint8_t>& payload,size_t terrain,size_t side,
    const OriginalCombinedProcessInputs& process){
    if(payload.size()<2||detail::u16(payload.data())!=19)return std::nullopt;
    const auto inputs=originalBuildingUpdateInputs(payload,profile);
    const auto production=originalBuildingProductionInputs(payload,profile);if(!inputs||!production)return std::nullopt;
    OriginalCombinedState out;out.figures=figures;out.process=process;
    if(process.crtRandom)out.figures.crtRandom=process.crtRandom;
    if(process.soundEffectsEnabled)out.figures.soundEffectsEnabled=process.soundEffectsEnabled;
    if(process.buildingEditMode)out.figures.buildingEditMode=process.buildingEditMode;
    auto footprints=profile.footprints;
    for(const auto& object:profile.objectBuildings){OriginalBuildingFootprint f;f.type=object.type;f.slot=object.slot;
        f.size=object.size;f.x=object.x;f.y=object.y;f.recordOffset=object.recordOffset;
        f.imageInputs=OriginalBuildingImageInputs{};f.imageInputs->gridIndex=object.gridIndex;footprints.push_back(f);}
    if(footprints.size()!=inputs->size()||terrain>payload.size()||original_save19::c82ef8+4>payload.size()-terrain)return std::nullopt;
    out.phase=detail::u32(payload.data()+terrain+original_save19::c82ef8);
    if(out.phase>50)return std::nullopt;
    for(const auto& f:footprints){
        auto c=originalBuildingSavedUpdateContext(payload,terrain,side,f,OriginalBuildingUpdateScope::AdmittedTicks,true);
        if(!c||!c->phase||*c->phase>50)return std::nullopt;
        if(!out.buildings.empty()&&out.phase!=*c->phase)return std::nullopt;
        out.phase=*c->phase;
        OriginalBuildingUpdateState state{f,inputs->at(f.slot),std::nullopt};
        const auto found=production->find(f.slot);if(found!=production->end())state.production=found->second;
        if(!out.buildings.emplace(f.slot,OriginalCombinedBuilding{state,*c}).second)return std::nullopt;
    }
    return out;
}
// Complete saved city services are shared, not independently replayed for each
// record. Archive/model inputs are supplied by the caller; no file/environment IO.
inline bool originalCombinedBindBuildingServices(OriginalCombinedState& city,const OriginalBuildingProfile& profile,
    const OriginalFigureProfile& figures,const std::vector<uint8_t>& payload,size_t terrain,size_t side,
    const OriginalSavedGround& ground,const OriginalBuildingScene& scene,const SG3Archive& general,
    const SG3Archive& terrainArchive,const std::string& buildingModels,
    std::optional<std::array<uint32_t,359>> speechCounts=std::nullopt){
    if(!city.figures.services)return false;
    OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
    if(!originalBuildingSavedCityContext(payload,side,c)||!originalBuildingSavedNoticeCountdown(payload,terrain,c)||
        !originalBuildingSavedHookContext(payload,terrain,c)||!ground.valid()||!scene.accepted)return false;
    c.phase=city.phase;c.monthPart=detail::u32(payload.data()+terrain+original_save19::c82ef0);
    c.cycle=detail::u32(payload.data()+terrain+original_save19::c82ef4);c.month=detail::u32(payload.data()+side-176);
    c.formations=originalBuildingSavedFormations(payload,terrain);c.figureModels=city.figures.models;
    c.goodsRecords=originalBuildingSavedGoodsRecords(payload,figures.end);
    city.annualOffers=originalBuildingSavedAnnualOffers(payload,figures.end);if(!city.annualOffers)return false;
    city.scenarioMessages=originalBuildingSavedScenarioMessages(payload,side);if(!city.scenarioMessages)return false;
    c.houseTaxModel=originalFigureHouseModelColumn(buildingModels,18);c.houseModelCapacity=originalFigureHouseCapacities(buildingModels);
    c.houses=originalBuildingHouseCapacityInputs(payload,profile);c.employmentModel=parseOriginalBuildingEmployment(buildingModels);
    c.religion=originalBuildingSavedReligion(payload,side);
    const auto empire=originalBuildingSavedEmpireStates(payload,figures.end,&c);
    if(!c.formations||!c.goodsRecords||!c.houseTaxModel||!c.houseModelCapacity||!c.houses||!c.employmentModel||!c.religion||!empire)return false;
    c.figureSpeechCounters=originalBuildingSavedSpeechCounters(payload,terrain);
    c.figureSpeechCounts=speechCounts;
    const auto difficulty=originalFigureSavedDifficulty(payload,side);if(!difficulty)return false;
    c.buildingModels=originalBuildingModels(buildingModels,*difficulty);if(!c.buildingModels)return false;
    c.houseModels=originalBuildingHouseModels(buildingModels,*difficulty);if(!c.houseModels||!c.cycleTail)return false;
    c.evolutionDisabled=city.figures.buildingEditMode;c.forceHouseEvolution=city.process.forceHouseEvolution;
    c.cycleTail->alternateMode=city.process.alternateMode;c.cycleTail->cropQuality=ground.state.gEAC3F0;c.cycleTail->waterMode=ground.waterMode;
    c.effectDecayCounter=city.process.effectDecayCounter;c.effectGrid=city.figures.services->serviceGrid;
    c.wallClock=city.process.wallClock;c.cropDispatchModel=city.process.cropDispatchModel;c.cropDispatchRadius=city.process.cropDispatchRadius;
    c.stallSuppliers=true;c.monthlyServices=true;c.additionalWalkers=true;c.industryCarts=true;
    c.disasters.emplace();c.disasters->freeBuildingSlots=originalBuildingSavedInactiveSlots(payload,profile);
    if(!c.disasters->freeBuildingSlots||!originalBuildingSavedDisasterContext(payload,terrain,c))return false;
    c.disasters->effectPlacementCulled=city.process.effectPlacementCulled;
    c.disasters->visualEffectCount=city.process.visualEffectCount;c.disasters->windDirection=city.process.windDirection;
    c.cycleTail->previousHealthRating=city.process.previousHealthRating;
    size_t ruinIndex=0;if(!resolveGroundTileCode(terrainArchive,0x612,ruinIndex))return false;
    c.disasters->ruinImageBase=(3u<<14)|uint32_t(ruinIndex);
    c.cropServices=city.figures.crops;c.cropFestival=originalBuildingSavedCropFestival(payload);
    if(!originalBuildingSavedTradeContext(payload,side,c))return false;
    c.terrainOwners=ground.owner;c.terrainRandom=ground.state.randomByte;
    c.statistics->empireState=*empire;c.statistics->religionBonusEnabled=city.process.religionBonusEnabled;
    c.religion->previousRating=city.process.religionPreviousRating;c.religion->selected=city.process.religionSelected;
    c.cityEmployment->shortageNotice=city.process.shortageNotice;c.cityEmployment->unemploymentNotice=city.process.unemploymentNotice;
    c.serviceCoverage=city.process.serviceCoverage;if(city.process.migrantSpacing)c.labor->spacing=*city.process.migrantSpacing;
    if(city.process.producerTail&&city.process.staffedProducerTail){c.buildingTotals.emplace();
        std::copy(city.process.producerTail->begin(),city.process.producerTail->end(),c.buildingTotals->producers.begin()+30);
        std::copy(city.process.staffedProducerTail->begin(),city.process.staffedProducerTail->end(),c.buildingTotals->staffedProducers.begin()+30);}
    c.scenarioFigurePlacementCheck=city.process.scenarioFigurePlacementCheck;
    c.labor->populationNotices=std::array<bool,10>{};
    for(unsigned n=0;n<10;++n)c.labor->populationNotices->at(n)=payload.at(terrain+1363680+n)!=0; //53153A
    c.buildingInputs.emplace();c.buildings.emplace();c.productionInputs.emplace();
    c.poolRoadAccess.emplace();c.poolRoadReachable.emplace();c.poolRoadSelected.emplace();c.storageRoadStatus.emplace();c.poolHookRoadAccess.emplace();
    c.highestBuildingSlot=0;c.buildingRegistrySize=4000;c.highestEverBuildingSlot=city.process.highestEverBuildingSlot;
    //Highest-ever is not restored; phase10 requires its explicit caller value.
    c.cropBase=city.figures.cropBase;c.activeSide=city.figures.activeSide;c.terrain=city.figures.terrain;
    c.movement=city.figures.passability;c.roadNetworks=city.figures.roadNetworks;
    c.roadRefreshOrdinaryExit=false;
    std::optional<OriginalBuildingRoadInputs> roads;
    if(c.movement&&c.cropBase&&c.activeSide&&c.roadEndpoints){
        const int entry=int(*c.cropBase)+c.roadEndpoints->at(0)+228*c.roadEndpoints->at(1);
        const auto distance=entry>=0?originalBuildingRoadDistances(c.movement->cells,uint32_t(entry)):std::nullopt;
        const auto ranked=originalBuildingRankedRoadNetworks(c.roadNetworks);
        if(distance&&ranked){roads.emplace();roads->cropBase=*c.cropBase;roads->terrain=scene.rebuiltMovementTerrain;
            roads->distance=*distance;roads->owners=ground.owner;roads->networks=c.roadNetworks;roads->rankedNetworks=*ranked;roads->ownerTypes=profile.slotTypes;
            const int exit=int(*c.cropBase)+c.roadEndpoints->at(2)+228*c.roadEndpoints->at(3);
            c.roadRefreshOrdinaryExit=exit>=0&&exit<51984&&distance->at(size_t(exit))!=0;}}
    for(auto& e:city.buildings){auto& b=e.second;const auto& f=b.state.building;
        c.buildingInputs->emplace(e.first,b.state.inputs);c.buildings->emplace(e.first,f);
        if(b.state.production)c.productionInputs->emplace(e.first,*b.state.production);
        *c.highestBuildingSlot=std::max(*c.highestBuildingSlot,uint32_t(e.first));
        b.context.unavailable=originalBuildingSavedUnavailable(payload,side,b.state.inputs.output);
        const auto required=c.employmentModel->find(f.type);if(required!=c.employmentModel->end())b.context.requiredWorkers=required->second;
        if(b.state.production)b.context.festivalProductionBoost=originalBuildingSavedProductionBoost(payload,f.type);
        if(roads){const auto access=f.type==59||f.type==62||f.type==66||f.type==67?
                originalBuildingMultipartRoadAccess(f,profile,*roads,b.state.inputs.parentSlot):
                originalBuildingIsHouse(f.type)?originalBuildingHouseRoadAccess(f,*roads):originalBuildingRoadAccess(f,*roads);
            if(!access)return false;c.poolRoadAccess->emplace(e.first,*access);c.poolRoadReachable->emplace(e.first,access->distance!=0);
            if(originalBuildingIsHouse(f.type)&&!originalBuildingHouseRoadAccess(f,*roads))c.roadRefreshOrdinaryExit=false;
            const auto adjacent=originalBuildingHouseRoadAccess(f,*roads,false);
            if(originalBuildingIsHouse(f.type)&&adjacent)city.figures.workerRoadExits.emplace(e.first,
                adjacent->selected?std::make_optional(std::make_pair(adjacent->x,adjacent->y)):std::nullopt);
            if(adjacent){c.poolRoadSelected->emplace(e.first,adjacent->selected);c.poolHookRoadAccess->emplace(e.first,*adjacent);}
            if(f.type==53||f.type==54||f.type==58){if(!adjacent)return false;uint8_t status=adjacent->selected?1:0;
                if(!status&&f.type==53){auto wider=f;wider.size=5;const auto fallback=originalBuildingHouseRoadAccess(wider,*roads,false);if(!fallback)return false;if(fallback->selected)status=2;}
                c.storageRoadStatus->emplace(e.first,status);}}}
    c.resourceGrowth.emplace();auto& growth=*c.resourceGrowth;growth.surface=ground.state;growth.surface.image=scene.surface.image;
    growth.surface.subTile=scene.surface.subTile;growth.owners=ground.owner;
    for(uint16_t code:{uint16_t(0x620),uint16_t(0x624),uint16_t(0x625),uint16_t(0x627)}){size_t index=0;
        if(!resolveGroundTileCode(terrainArchive,code,index))return false;growth.imageBases.emplace(code,(3u<<14)|uint32_t(index));}
    if(general.imageIndex.size()<=0x35||general.imageIndex[0x35]<201)return false;
    c.irrigation.emplace();c.irrigation->image=scene.surface.image;c.irrigation->subTile=scene.surface.subTile;
    c.irrigation->owners=ground.owner;c.irrigation->ownerTypes=profile.slotTypes;c.irrigation->baseImage=(2u<<14)|uint32_t(general.imageIndex[0x35]-201);
    for(const auto& e:city.buildings)if(e.second.state.building.type==57)return false; //524360 registry needs its source descriptor
    //The post-pass registry is the complete building slot vector (413B40(1)).
    //Only non-whitelisted types can ignore missing live/word inputs; the
    //post-pass adapter below refuses a whitelisted slot absent from this pool.
    city.postFigureRegistry.resize(profile.parsedRecords);
    for(size_t n=0;n<profile.parsedRecords;++n){auto& o=city.postFigureRegistry[n];o.type0x14=int16_t(profile.slotTypes.at(n));}
    city.figures.migrationRoadEndpoints=c.roadEndpoints;
    city.rebuiltMovementTerrain=scene.rebuiltMovementTerrain;city.buildingContext=std::move(c);return true;
}
// Figure links are canonical for the fields figures can write. Bridge their
// overlapping common5/data1 fields before another building operation or publish.
// Wire positions are427430/517410/51C3A0, shared with the existing link readers.
inline std::string originalCombinedSyncBuildings(OriginalCombinedState& city){
    for(auto& entry:city.buildings){auto& s=entry.second.state;
        if(!city.figures.buildings||!city.figures.buildings->count(entry.first))return "combined_building_link_required";
        const auto& link=city.figures.buildings->at(entry.first);
        s.building.type=link.type;s.inputs.alive=link.alive;
        if(link.type>=2&&link.type<18){s.inputs.model=link.houseModel;s.inputs.residents=link.residents;s.inputs.free=link.freeSlots;}
        s.inputs.distance=link.roadDistance;s.inputs.roadX=link.roadX;s.inputs.roadY=link.roadY;
        s.inputs.migrant=link.worker;s.inputs.workers=link.workers;s.inputs.upgrade=link.upgrade;
        s.inputs.primaryFigure=link.primaryFigure;
        s.inputs.fireRisk=link.fireRisk;s.inputs.damageRisk=link.damageRisk;s.inputs.watchRisk=link.watchRisk;
        s.inputs.houseInUse=link.accepting;s.inputs.stock=link.industryInput;s.inputs.direction=link.roamDirection;
        s.inputs.operational=link.serviceCount;s.inputs.enabled=link.destinationEnabled;
        s.inputs.output=link.outputResource;s.inputs.available=link.roadAccess;s.inputs.network=link.category;
        if(link.type<2||link.type>=18)s.inputs.model=link.traderFaction;
        if(s.building.animationInputs)s.building.animationInputs->workers=link.workers;
        if(s.building.imageInputs){if(link.type>=2&&link.type<18){s.building.imageInputs->word20=link.residents;
                if(link.laborDisabled)s.building.imageInputs->houseByte3c=*link.laborDisabled;}
            if(link.storage)s.building.imageInputs->compartments=link.storage;}
        if(s.production){s.production->workers=link.workers;s.production->primaryStock=link.industryInput;
            s.production->secondaryStock=link.hunterReturnGoods;s.production->operational=link.serviceCount;
            s.production->enabled=link.destinationEnabled;}
        if(s.inputs.industry&&link.cropDelivery)originalBuildingSetCropFlags(s.inputs,link.cropDelivery->flags);
        if(s.inputs.cropWorkers){if(link.harvestFarmers)s.inputs.cropWorkers->at(0)=*link.harvestFarmers;
            if(link.suppliers)s.inputs.cropWorkers->at(1)=*link.suppliers;}
        if(s.inputs.hookIndustry){auto& data=*s.inputs.hookIndustry;
            if(link.delegatedCount)data.at(0x5c-4)=*link.delegatedCount;
            if(link.type==71&&link.entertainment)for(unsigned n=0;n<3;++n)data.at(0x5d-4+n)=link.entertainment->at(n);
            if(link.type==54||link.type==58||link.type==59||link.type==71||link.type==124)
                for(unsigned n=0;n<2;++n){const uint16_t id=uint16_t(link.secondaryFigures[n]);data.at(0x6a-4+2*n)=uint8_t(id);data.at(0x6b-4+2*n)=uint8_t(id>>8);}}
        if(!s.inputs.house)continue;
        if(!link.religion||!link.entertainment||!link.herbalist||!link.waterService||!link.houseGoods||!link.foodQuality)
            return "shared_house_service_fields_required";
        auto& h=*s.inputs.house;
        if(link.laborDisabled)h.at(55)=*link.laborDisabled;
        for(unsigned n=0;n<4;++n)h.at(13-4+n)=link.religion->at(n);
        for(unsigned n=0;n<12;++n){const uint16_t v=uint16_t(link.houseGoods->at(n));h.at(17-4+2*n)=uint8_t(v);h.at(18-4+2*n)=uint8_t(v>>8);}
        for(unsigned n=0;n<3;++n)h.at((n==2?45:42+n)-4)=link.entertainment->at(n);
        h.at(44-4)=*link.herbalist;h.at(49-4)=link.waterService->at(0);h.at(51-4)=link.waterService->at(1);
        h.at(53-4)=*link.foodQuality;
    }
    return {};
}
//5D63C0->5D5DB0 industrial supplier search. Warehouse+1A8
//is416B00 (common+18 network);5F2060 reserves100 per incoming cart before
//5D5EE6 compares networks.5D6461..6540 creates type22 and withdraws stock.
inline OriginalBuildingUpdateBlocker originalCombinedWarehouseHook(OriginalBuildingUpdateState& s,OriginalBuildingUpdateContext& c){
    if(!c.buildingLinks||!c.buildingInputs||!c.goods||!c.employmentModel||!c.employmentModel->count(54)||!s.inputs.hookIndustry)
        return OriginalBuildingUpdateBlocker::MissingContext;
    auto& b=c.buildingLinks->at(s.building.slot);if(!b.storage)return OriginalBuildingUpdateBlocker::MissingContext;
    const int required=s.inputs.laborDisabled?0:c.employmentModel->at(54),percent=required>0?s.inputs.workers*100/required:0;
    const int16_t exporter=detail::i16(s.inputs.hookIndustry->data()+0x6a-4);
    if(!exporter&&percent){bool mismatch=false;c.storageSupplierMismatch[s.building.slot]=0;
        for(const auto& e:*c.buildingInputs){const auto& target=e.second;const auto& link=c.buildingLinks->at(e.first);
            if((target.alive!=1&&target.alive!=3)||!target.workers||!target.enabled||link.type!=43)continue;
            for(const auto& bay:*b.storage){if(bay.resource!=18||!bay.amount||c.goods->unavailable[18])continue;
                if(target.stock>300||(link.incomingFigures>0&&400-link.incomingFigures*100-target.stock<=0))continue;
                if(link.category==b.category){
                    //19 and22 share constructor speech254 and identical common
                    //fields/table groups84E798;the API19 gate admits that common
                    //constructor,then5D6476 supplies22 before its caller writes.
                    const auto slot=originalBuildingAllocateService(c,19,s.inputs.faction,s.inputs.roadX,s.inputs.roadY,4);
                    if(!slot)return OriginalBuildingUpdateBlocker::MissingContext;
                    if(*slot){auto& figure=c.figures->at(*slot);figure.type=22;figure.action=6;figure.owner=int16_t(s.building.slot);figure.subtype=18;
                        unsigned amount=0;for(const auto& stock:*b.storage)if(stock.resource==18)amount+=stock.amount;
                        figure.amount=originalFigureWord(int(std::min(400u,amount)));figure.action=10; //4D6AC0 bulk18
                        const auto remaining=originalFigureStorageWithdraw(b,18,unsigned(figure.amount),true,*c.goods);
                        if(!remaining||*remaining)return OriginalBuildingUpdateBlocker::MissingContext;
                        s.inputs.hookIndustry->at(0x6a-4)=uint8_t(*slot);s.inputs.hookIndustry->at(0x6b-4)=uint8_t(*slot>>8);
                        if(s.building.imageInputs)s.building.imageInputs->compartments=b.storage;}
                    const auto output=originalBuildingStorageExport(b,*c.goods,percent,true);
                    if(!output)return OriginalBuildingUpdateBlocker::MissingContext;
                    return *output>=0?OriginalBuildingUpdateBlocker::BuildingVirtual20:OriginalBuildingUpdateBlocker::None;}
                mismatch=true;}}
        c.storageSupplierMismatch[s.building.slot]=mismatch?1:0;}
    const auto output=originalBuildingStorageExport(b,*c.goods,percent,true);if(!output)return OriginalBuildingUpdateBlocker::MissingContext;
    if(*output>=0)return OriginalBuildingUpdateBlocker::BuildingVirtual20;return OriginalBuildingUpdateBlocker::None;
}
inline std::string originalCombinedBuildingPhase(OriginalCombinedState& city){
    const auto blocker=originalCombinedPhaseBlocker(city.phase);if(!blocker.empty())return blocker;
    //Empty cases are dispatch targets4AC4C8;9 calls40FC20, a RET.
    const bool empty=city.phase==0||city.phase==2||city.phase==9||city.phase==14||city.phase==15||city.phase==19||
        city.phase==26||city.phase==34||city.phase==41||city.phase==42||city.phase==46||city.phase==49;
    if(!city.buildingContext){if(empty)return {};return "combined_shared_building_context_required";}
    auto& c=*city.buildingContext;auto& f=city.figures;
    c.createdFigureRecords.clear();c.removedFigureRecords.clear();const size_t allocatedBefore=c.allocatedFigures.size();
    const auto sync=originalCombinedSyncBuildings(city);if(!sync.empty())return sync;
    c.phase=city.phase;c.figureServices=f.services;c.buildingLinks=f.buildings;c.goods=f.goods;c.figures.emplace();c.figureStationary.emplace();
    c.figureOrdinals.clear(); //54CC60 writes only when called, not on intervening phases
    for(const auto& e:f.figures){if(e.first!=c.figures->size())return "combined_ordered_figure_pool_required";
        c.figures->push_back(e.second);c.figureStationary->operator[](e.first)=e.second.formationStationary;}
    c.figureModels=f.models;c.figureCellHeads=f.cellHeads;c.freeFigureSlots=f.freeSlots;c.highestFigureSlot=f.highestLiveSlot;
    c.crtRandom=f.crtRandom;c.crtRandomCalls=f.crtRandomCalls;c.soundEnabled=f.soundEffectsEnabled;
    if(f.speechCounters)c.figureSpeechCounters=f.speechCounters;
    if(c.disasters){c.disasters->visualEffectCount=f.visualEffectCount?f.visualEffectCount:city.process.visualEffectCount;
        c.disasters->figureEffects.clear();}
    c.cropServices=f.crops;c.effectGrid=f.services?std::make_optional(f.services->serviceGrid):std::nullopt;
    c.terrain=f.terrain;c.movement=f.passability;
    if(f.services&&f.services->population){c.populationTotal=f.services->population->total;c.populationPeak=f.services->population->peak;}
    for(const auto& e:city.buildings){c.buildingInputs->at(e.first)=e.second.state.inputs;c.buildings->at(e.first)=e.second.state.building;
        if(e.second.state.production)c.productionInputs->at(e.first)=*e.second.state.production;}
    OriginalBuildingUpdateBlocker result=OriginalBuildingUpdateBlocker::None;
    switch(city.phase){
    case 1:result=originalBuildingAccountingStep(c);break;
    case 3:case 30:c.citySignal=1;break; //4819F0/4AC42D store13F8E78=1
    case 4:if(!c.treasury)result=OriginalBuildingUpdateBlocker::CityPhase;else if(*c.treasury>=0)c.cityCountA=c.cityCountB=0;
        else return "negative_treasury_city_counters_required";break;
    case 5:case 29:result=originalBuildingFormationPhase(c,city.phase==5);break;
    case 6:result=originalBuildingLaborCountdown(c);break;
    case 7:if(c.movement)result=originalBuildingNetworkStep(c);
        else { //4AF350; a B0C projection suffices for the building-free river path.
            const auto networks=originalFigureRoadNetworks(f);
            const auto ranks=networks?originalBuildingRankedRoadNetworks(*networks):std::nullopt;
            if(!networks||!ranks)result=OriginalBuildingUpdateBlocker::RoadRefresh;
            else {c.roadNetworks=*networks;c.rankedNetworks=*ranks;++c.networkCalls;}}
        break;
    case 8:result=originalBuildingGoodsPoolStep(c);break;
    case 10:if(!c.highestEverBuildingSlot||!c.goodsRecords||c.goodsRecords->empty())result=OriginalBuildingUpdateBlocker::CityPhase;
        else{uint32_t highest=0;for(const auto& e:*c.buildingInputs)if(e.second.alive)highest=uint32_t(e.first);
            c.highestBuildingSlot=highest;*c.highestEverBuildingSlot=std::max(*c.highestEverBuildingSlot,highest);c.goodsRegistryLast=uint32_t(c.goodsRecords->size()-1);}break;
    case 11:if(!c.populationTotal||!c.populationPeak)result=OriginalBuildingUpdateBlocker::CityPhase;else *c.populationPeak=std::max(*c.populationPeak,*c.populationTotal);break;
    case 12:result=originalBuildingAvailabilityStep(c);break;
    case 13:result=originalBuildingStatisticsStep(c);break;
    case 16:result=originalBuildingStorageStep(c);break;
    case 17:result=originalBuildingFoodStep(c);break;
    case 18:result=originalBuildingReligionStep(c);break;
    case 21:if(city.buildings.empty())c.roadRefreshOrdinaryExit=originalCombinedEmptyCityRoadExit(city)==true;
        if(!c.roadRefreshOrdinaryExit||!*c.roadRefreshOrdinaryExit)result=OriginalBuildingUpdateBlocker::RoadRefresh;
        else result=originalBuildingPoolRoadStep(c);break;
    case 22:if(city.buildings.empty()){if(!c.houses||!c.houses->empty())result=OriginalBuildingUpdateBlocker::HousingCapacity;else c.cityHouseCapacity=c.cityHouseFree=0;}
        else result=originalBuildingHouseCapacityStep(city.buildings.begin()->second.state,c);break;
    case 23:if(!city.process.migrantSpacing)return "migrant_spacing_d62418_required";result=originalBuildingCityLaborStep(c);break;
    case 24:result=originalBuildingExcessResidents(c);break;
    case 25:result=originalBuildingEmploymentStep(c);break;
    case 27:result=originalBuildingIrrigationStep(c);break;
    case 28:result=originalBuildingWaterStep(c);break;
    case 31:result=originalBuildingHookPreamble(c);break;
    case 32:result=originalBuildingTradeStep(c);break;
    case 33:result=originalBuildingTotalsStep(c);break;
    case 37:result=originalBuildingDesirabilityStep(c);break;
    case 38:result=originalBuildingApplyDesirability(c);break;
    case 43:result=originalBuildingFireStep(c);break;
    case 44:result=originalBuildingRiskStep(c);break;
    case 45:result=originalBuildingMonumentStep(c);break;
    case 47:c.phase47Counters.fill(0);++c.counterCalls;break;
    case 50:result=originalBuildingCycleTailWithAnnualStep(c,city.annualOffers,city.scenarioMessages?&*city.scenarioMessages:nullptr,
        city.process.annualDemandResponseMode);break;
    default:break;
    }
    if(result!=OriginalBuildingUpdateBlocker::None)return originalBuildingUpdateBlockerName(result);
    //Local writes run once, in ascending slot order. Phase20's resource scan
    //follows ALL production records (4AC3BF then4AC3C4), never one scan per record.
    if(city.phase==20||city.phase==31||city.phase==35||city.phase==36||city.phase==39||city.phase==40||city.phase==48)for(auto& e:city.buildings){auto& b=e.second;
        b.state.inputs=c.buildingInputs->at(e.first);
        if(city.phase==20){c.unavailable=b.context.unavailable;c.requiredWorkers=b.context.requiredWorkers;
            c.festivalProductionBoost=b.context.festivalProductionBoost;c.terrainFlags=b.context.terrainFlags;c.typeDisabled=b.context.typeDisabled;
            result=originalBuildingProductionStep(b.state,c);}
        else if(city.phase==31){c.typeDisabled=b.context.typeDisabled;c.terrainFlags=b.context.terrainFlags;
            if(b.state.inputs.alive==1||b.state.inputs.alive==3){result=b.state.building.type==33?
                    originalBuildingHunterDispatchStep(b.state,c):originalBuildingHookStep(b.state,c);
                if(result==OriginalBuildingUpdateBlocker::BuildingVirtual20&&b.state.building.type==54)
                    result=originalCombinedWarehouseHook(b.state,c);}}
        else if(city.phase==35){if(originalBuildingIsHouse(b.state.building.type)&&(b.state.inputs.alive==1||b.state.inputs.alive==3))originalBuildingDecayHouseServices(b.state.inputs);}
        else if(city.phase==39)result=originalBuildingHouseEvolutionStep(b.state,c);
        else if(city.phase==40)result=originalBuildingLifecycleStep(b.state,c);
        else {auto local=b.context;local.phase=city.phase;local.month=c.month;local.monthPart=c.monthPart;local.cycle=c.cycle;
            const auto step=originalBuildingStateAfterUpdates(b.state,local,1);
            result=step.blocker;if(step.state)b.state=*step.state;}
        if(result!=OriginalBuildingUpdateBlocker::None)return originalBuildingUpdateBlockerName(result);
        c.buildingInputs->at(e.first)=b.state.inputs;c.buildings->at(e.first)=b.state.building;if(b.state.production)c.productionInputs->at(e.first)=*b.state.production;
    }
    if(city.phase==20){result=originalBuildingResourceGrowthStep(c);if(result!=OriginalBuildingUpdateBlocker::None)return originalBuildingUpdateBlockerName(result);}
    //42AAA0/4B7660 keep the original tombstone and create independent161
    //records. The public disaster service owns footprint replacement/masks.
    for(const auto& e:*c.buildingInputs)if(e.second.alive!=1&&e.second.alive!=3&&
        (!c.disasters||std::find(c.disasters->destroyedBuildings.begin(),c.disasters->destroyedBuildings.end(),e.first)==c.disasters->destroyedBuildings.end()))
        return "combined_building_removal_4b7660_not_closed";
    for(const auto& e:*c.buildingInputs)if(!city.buildings.count(e.first)){
        if(!c.disasters||c.buildings->at(e.first).type!=161)return "combined_created_building_kind_required";
        auto local=OriginalBuildingUpdateContext(OriginalBuildingUpdateScope::AdmittedTicks,true);
        local.phase=city.phase;local.month=c.month;local.monthPart=c.monthPart;local.cycle=c.cycle;
        local.disasters=c.disasters;
        city.buildings.emplace(e.first,OriginalCombinedBuilding{{c.buildings->at(e.first),e.second,std::nullopt},std::move(local)});}
    for(size_t n=allocatedBefore;n<c.allocatedFigures.size();++n)c.createdFigureRecords.push_back(c.figures->at(c.allocatedFigures[n]));
    if(!originalBuildingSynchronizePool(c))return "combined_building_pool_bridge_required";
    //427DE6/427D93: road network is common wire20; employment category
    //wire64 is a distinct field. Figures' category name refers to wire20.
    if(c.buildingLinks)for(const auto& e:*c.buildingInputs)c.buildingLinks->at(e.first).category=e.second.network;
    if(c.buildingLinks)for(const auto& e:*c.buildingInputs){auto& link=c.buildingLinks->at(e.first);
        const auto& i=e.second;link.serviceCount=i.operational;link.destinationEnabled=i.enabled;
        if(c.productionInputs&&c.productionInputs->count(e.first))link.hunterReturnGoods=c.productionInputs->at(e.first).secondaryStock;
        if(i.cropWorkers){link.harvestFarmers=i.cropWorkers->at(0);link.suppliers=i.cropWorkers->at(1);}
        if(!originalBuildingIsHouse(link.type))link.traderFaction=i.model;
        if(link.type==124)link.inspectorSecondary=i.model>0||(i.modelUpgrade&&*i.modelUpgrade!=0);
        auto& footprint=c.buildings->at(e.first);
        if(footprint.wellImageInputs){footprint.wellImageInputs->word16=i.model;
            if(i.modelUpgrade)footprint.wellImageInputs->byte6f=*i.modelUpgrade;}
        link.roamDirection=i.direction;link.accepting=i.houseInUse;link.industryInput=i.stock;
        if(i.house){if(!link.houseGoods||!link.foodQuality)return "combined_house_goods_bridge_required";
            for(unsigned n=0;n<12;++n)link.houseGoods->at(n)=detail::i16(i.house->data()+17-4+2*n);
            link.foodQuality=i.house->at(53-4);}
        if(i.industry&&link.cropDelivery)link.cropDelivery->flags=originalBuildingCropFlags(i);
        if(i.hookIndustry){const auto& data=*i.hookIndustry;
            if(link.delegatedCount)link.delegatedCount=data.at(0x5c-4);
            if(link.type==71&&link.entertainment)for(unsigned n=0;n<3;++n)link.entertainment->at(n)=data.at(0x5d-4+n);
            if(link.type==54||link.type==58||link.type==59||link.type==71||link.type==124)
                for(unsigned n=0;n<2;++n)link.secondaryFigures[n]=detail::i16(data.data()+0x6a-4+2*n);}}
    f.services=c.figureServices;f.buildings=c.buildingLinks;f.goods=c.goods;f.crtRandom=c.crtRandom;f.crtRandomCalls=c.crtRandomCalls;
    f.speechCounters=c.figureSpeechCounters;f.speechCounts=c.figureSpeechCounts;
    if(city.phase==43)f.services->fireCount=c.fireTotals[2]; //4AF9CE writes C82F28 before Inspector4E2E90.
    if(city.phase==45&&c.effectGrid)f.services->serviceGrid=*c.effectGrid;
    if(c.cropServices)f.crops=c.cropServices;
    if(f.crops&&c.month)f.crops->month=*c.month;
    f.roadNetworks=c.roadNetworks;
    if(f.hunterLinks)for(auto& e:*f.hunterLinks){const auto link=f.buildings->find(e.first);if(link==f.buildings->end())return "combined_hunter_link_required";
        e.second.alive=link->second.alive;e.second.enabled=link->second.destinationEnabled;e.second.workers=link->second.workers;}
    f.freeSlots=c.freeFigureSlots;f.highestLiveSlot=c.highestFigureSlot;f.cellHeads=*c.figureCellHeads;
    for(const auto& value:*c.figures)f.figures.at(value.slot)=value;
    for(const auto& record:c.createdFigureRecords)f.primaryImageInputs.erase(record.slot);
    //The common allocator exports its raw logical-cell chain.417390 then
    //initializes kind0;4EA3B0->418450 applies the effect draw-cell offset.
    for(const auto& record:c.createdFigureRecords)if(record.type==1){auto& effect=f.figures.at(record.slot);
        size_t previous=0,slot=f.cellHeads.at(effect.cell);std::set<size_t> seen;
        while(slot&&slot!=effect.slot){if(!seen.insert(slot).second)return "combined_effect_list_4ea3b0";
            previous=slot;slot=f.figures.at(slot).next;}
        if(!slot)return "combined_effect_list_4ea3b0";
        if(previous)f.figures.at(previous).next=effect.next;else f.cellHeads.at(effect.cell)=effect.next;
        effect.next=0;const auto cell=originalFigureDrawCell(effect);if(cell>=51984)return "combined_effect_list_4ea3b0";
        auto& head=f.cellHeads.at(size_t(cell));if(!head)head=uint16_t(effect.slot);else{size_t tail=head;seen.clear();
            while(f.figures.at(tail).next){if(!seen.insert(tail).second)return "combined_effect_list_4ea3b0";tail=f.figures.at(tail).next;}
            f.figures.at(tail).next=uint16_t(effect.slot);}}
    for(const auto& e:c.figureOrdinals)f.figures.at(e.first).formationOrdinal=e.second;
    for(size_t slot:c.allocatedFigures)f.allocatedSlots.insert(slot);
    f.allocationCount+=c.createdFigureRecords.size();f.buildingAllocationCount+=c.createdFigureRecords.size();
    f.farmerTasks=c.farmerTasks;
    for(const auto& e:*c.buildingInputs)f.buildingBurning[e.first]=e.second.burning;
    if(c.disasters){for(const auto& effect:c.disasters->figureEffects){auto& record=f.figures.at(effect.slot);
            record.effectAnimation=effect.animation;c.figures->at(effect.slot)=record;}
        f.visualEffectCount=c.disasters->visualEffectCount;
        if(c.terrainOwners)f.buildingOwners=*c.terrainOwners;
        if(c.terrain)f.terrain=*c.terrain;
        if(c.movement){f.passability=c.movement;f.movementMasks=c.movement->cells;}
        if(c.terrain&&!c.disasters->createdBuildings.empty())city.rebuiltMovementTerrain=*c.terrain;}
    for(auto& record:c.createdFigureRecords)record=f.figures.at(record.slot);
    //Removed records from an admitted public helper are applied as complete
    //slot replacements;the exported FIFO and cell heads already include unlink.
    for(const auto& record:c.removedFigureRecords){if(record.slot>=2000||record.alive)return "combined_removed_figure_record_required";
        f.figures.at(record.slot)=record;}
    for(auto& e:city.buildings){auto& b=e.second;b.state.inputs=c.buildingInputs->at(e.first);
        b.state.building=c.buildings->at(e.first);
        if(b.state.production)b.state.production=c.productionInputs->at(e.first);
        if(b.state.building.imageInputs&&b.state.inputs.house){b.state.building.imageInputs->word20=b.state.inputs.residents;
            b.state.building.imageInputs->houseByte3c=b.state.inputs.house->at(55);}
        b.context.phase=city.phase==50?0:city.phase+1;}
    city.process.randomHistory=f.services->randomHistory;
    return {};
}
// 466E90 consumes the live, loaded message table. Do not require a second
// caller copy of its modes: annual processing can append/change these rows.
inline std::string originalCombinedScenarioEvents(OriginalCombinedState& city){
    auto& p=city.process;
    if(city.scenarioMessages&&city.scenarioMessages->count>city.scenarioMessages->records.size())
        return "scenario_manager_492430_records_required";
    if((!p.managerGate8||*p.managerGate8)&&!city.scenarioMessages&&!p.eventRecordModes)
        return "scenario_manager_492430_records_required";
    if(p.eventRecordModes&&p.eventRecordModes->size()>150)return "scenario_manager_492430_records_required";
    // Preserve explicitly supplied mode constraints, but read the actual live
    // table too; an external empty list cannot hide an active saved mode5 row.
    if(p.eventRecordModes)for(uint8_t mode:*p.eventRecordModes)
        if(mode==5&&(!p.managerGate8||*p.managerGate8))return "scenario_manager_492430_4ed3b0";
    if(city.scenarioMessages)for(unsigned n=0;n<city.scenarioMessages->count;++n)
        if(city.scenarioMessages->records[n][2]==5&&(!p.managerGate8||*p.managerGate8))return "scenario_manager_492430_4ed3b0";
    // 466EA6 overwrites this byte before either slot loop. Its previous value
    // is not read here. The building stage earlier in this update must still
    // receive its prior value; only subsequent work sees this write.
    p.eventProgress=uint8_t{0};
    p.scenarioFigurePlacementCheck=false; // same A98370+0x3F770 byte
    if(city.buildingContext)city.buildingContext->scenarioFigurePlacementCheck=false;
    if(!p.eventSlots)return "scenario_events_466e90_slots_required";
    if(std::any_of(p.eventSlots->begin(),p.eventSlots->end(),[](uint32_t slot){return slot!=0;}))return "scenario_slot_467680_not_closed";
    return {};
}
// 5371A0 prefix, then exactly one scheduler phase. The only accepted event
// branches below are proven empty/no-op; positive branches keep their own names.
inline std::string originalCombinedBeforeFigures(OriginalCombinedState& city,std::vector<uint32_t>& order){
    auto& p=city.process;
    order.clear();order.push_back(0x4189b0);
    if(!p.cityClocks)return "city_clocks_529990_initial_values_required";
    if(!p.randomHistory||p.randomHistory->cursor>=100||p.randomHistory->previous7>127||!city.figures.services||
        std::any_of(p.randomHistory->ring.begin(),p.randomHistory->ring.end(),[](uint32_t v){return v>127;}))
        return "engine_rng_4189b0_history_required";
    city.figures.services->randomHistory=p.randomHistory;
    originalFigureRandomNext(*city.figures.services);
    p.randomHistory=city.figures.services->randomHistory;
    order.push_back(0x529990);advanceOriginalCityClocks(*p.cityClocks);
    order.push_back(0x5ef190);order.push_back(0x5eff20);
    //5EFF25->53C720(+17),5EFF34->53C710(+16): zero returns5F00A2
    //before any store. Unknown/positive is never accepted as an empty call.
    if(!p.eventGate17||(*p.eventGate17&&(!p.eventGate16||*p.eventGate16)))return "event_ui_5eff20_not_closed";
    if(!p.cityMode||*p.cityMode)return "ordinary_city_mode_88ec38_zero_required";
    order.push_back(0x4ac2b0);
    //408AC0 returns10BC348;4FAE50 reads+16. Zero short-circuits directly
    //to4AC2E4 without the two later, effectful scheduler prechecks.
    if(!p.schedulerGate16||*p.schedulerGate16)return "scheduler_precheck_4fae50_55b540_55cee0";
    if(city.phase>50)return "invalid_city_phase";
    const auto blocker=originalCombinedBuildingPhase(city);if(!blocker.empty())return blocker;
    city.phase=city.phase==50?0:city.phase+1; //4AC4C8..4E2;tail already completed through the public API.
    if(city.figures.goods)city.figures.goods->savedPhase=city.phase;
    order.push_back(0x466e90);
    return originalCombinedScenarioEvents(city);
}
inline std::string originalCombinedAfterFigures(OriginalCombinedState& city,std::vector<uint32_t>& order){
    auto& p=city.process;p.randomHistory=city.figures.services->randomHistory;
    order.push_back(0x564b50);
    OriginalPostFigurePassInputs inputs;inputs.gate0x88EC38=p.cityMode&&*p.cityMode<=255?std::optional<uint8_t>(uint8_t(*p.cityMode)):std::nullopt;
    inputs.pause0x10DE0D0=p.wildlifePause;inputs.mode0x10DE045=p.wildlifeMode;
    inputs.counterInitial0x12A6E78=p.wildlifeCounter;inputs.periodInitial0x859DA0=p.wildlifePeriod;
    if(!originalPostFigurePassInputsComplete(inputs))return originalPostFigurePassStatusName(OriginalPostFigurePassStatus::RefusedMissingSessionInput);
    for(const auto& e:city.buildings){if(e.first>=city.postFigureRegistry.size())return "post_figure_building_registry_required";
        auto& o=city.postFigureRegistry[e.first];o.type0x14=int16_t(e.second.state.building.type);o.word0x16=e.second.state.inputs.model;o.live0x04=e.second.state.inputs.alive;}
    const bool due=originalBuildingI32(*p.wildlifeCounter+1u)>originalBuildingI32(*p.wildlifePeriod);
    for(size_t n=0;n<city.postFigureRegistry.size();++n){const auto& o=city.postFigureRegistry[n];
        if(due&&n&&originalPostFigureWhitelisted(uint32_t(int32_t(o.type0x14)))&&!city.buildings.count(n))
            return "post_figure_registry_live_fields_required";
        inputs.registry.push_back(&o);}
    auto state=*originalPostFigurePassSessionState(inputs);city.postFigureResult=advanceOriginalPostFigurePass(state,inputs);
    p.wildlifeCounter=state.counter;p.wildlifePeriod=state.period;
    return city.postFigureResult->status==OriginalPostFigurePassStatus::Ok?"":originalPostFigurePassStatusName(city.postFigureResult->status);
}
inline std::optional<OriginalFigurePassability> originalSceneMovementPassability(const OriginalBuildingScene& scene,
    const OriginalBuildingProfile& buildings,const OriginalSavedGround& saved,const OriginalFigureSpawnServices& spawn){
    if(!scene.accepted||!scene.groundRebuilt||scene.rebuiltMovementTerrain.size()!=51984||!saved.valid()||
        std::any_of(saved.state.terrain.begin(),saved.state.terrain.end(),[](uint32_t t){return (t&0x10000)!=0;}))return std::nullopt;
    auto surface=scene.surface;surface.terrain=scene.rebuiltMovementTerrain;
    return originalFigureMovementPassability(surface,buildings,saved.owner,spawn);
}
// One frame-loop iteration: 536B4D captures animation time,418AA0 computes
// flags,53A130 separately samples admission,then0..10000 ordered updates,
// then the caller's explicitly observed building draws. No catch-up premise.
struct OriginalSceneTimelineFrame {
    uint32_t animationClock=0;
    OriginalFigureAdmissionInputs admission;
    std::vector<bool> continueAfterPass;
    std::vector<size_t> buildingDraws;
    std::vector<uint32_t> figureDrawCells;
    // App opt-in: only the first frame may carry initial combined process state.
    std::optional<OriginalCombinedProcessInputs> combinedProcess;
    std::optional<uint32_t> farmerWallClock; //766DA0 caller observation for this frame;absent retains explicit process input
};
struct OriginalSceneTimeline {
    bool complete=false;
    unsigned updates=0,refusedUpdate=0;
    size_t frames=0,buildingDraws=0,buildingAdvances=0;
    size_t buildingImageReselections=0;
    size_t preyDeaths=0,preyCollections=0;
    uint32_t admissionClock=0;
    OriginalAnimationClock animationClock;
    std::map<size_t,uint8_t> buildingPhases;
    std::optional<OriginalFigureUpdateContext> context;
    std::map<size_t,std::vector<OriginalStorageCompartment>> buildingStorage;
    std::map<size_t,std::pair<uint16_t,int16_t>> buildingHouses; //518DE0 type conversion and resident selector input.
    OriginalFigureOrderedPass lastPass;
    std::string refusal;
    std::optional<OriginalCombinedState> combinedState;
    std::vector<uint32_t> lastUpdateOrder;
    size_t buildingOutcomes=0;
    size_t createdBuildings=0,destroyedBuildings=0;
    size_t figureAllocations=0,buildingFigureAllocations=0;
    std::map<size_t,OriginalBuildingFootprint> buildingSelections;
};
//4711A9->469580 sorts and REWRITES +10/cell heads before drawing.
// Preserve that mutation for the next admitted pass; a draw is not read-only.
inline bool originalFigureSortDrawCell(OriginalFigureUpdateContext& context,uint32_t cell){
    if(context.cellHeads.size()!=51984||cell>=51984)return false;
    std::vector<std::pair<int,size_t>> order;std::set<size_t> visited;
    for(size_t slot=context.cellHeads[cell];slot;){const auto found=context.figures.find(slot);
        if(found==context.figures.end()||!visited.insert(slot).second||visited.size()>50||
            originalFigureDrawCell(found->second)!=cell)return false;
        const auto score=originalFigureDepthScore(found->second);if(!score)return false;
        order.emplace_back(*score,slot);slot=found->second.next;}
    std::sort(order.begin(),order.end());
    context.cellHeads[cell]=order.empty()?0:uint16_t(order.front().second);
    for(size_t n=0;n<order.size();++n)context.figures.at(order[n].second).next=n+1<order.size()?uint16_t(order[n+1].second):0;
    return true;
}
inline OriginalSceneTimeline originalSceneTimelineImpl(const OriginalFigureUpdateContext& initial,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalBuildingScene& scene,const SG3Archive& general,
    const OriginalBuildingEmployment* employment,uint32_t previousAdmission,OriginalAnimationClock animationClock,
    const std::vector<OriginalSceneTimelineFrame>& frames,const OriginalCombinedState* combined=nullptr){
    OriginalSceneTimeline out;out.admissionClock=previousAdmission;out.animationClock=animationClock;
    for(size_t n=0;n<frames.size();++n)if(frames[n].combinedProcess&&(!combined||n)){
        out.refusal="combined_initial_saved_state_required_on_first_frame";return out;}
    if(!scene.accepted||scene.animationTickMs||initial.figures.size()!=2000||initial.terrain.size()!=51984||initial.terrain!=scene.surface.terrain||
        frames.size()>10000){out.refusal="timeline_inputs";return out;}
    std::map<size_t,const SG3Sprite*> bases;
    // Market/service/crop-link writes do not feed a cited image selector.
    // Warehouse/station resource+amount DO: rebind their existing components
    // after every successful pass changing those inputs, then install atomically.
    for(const auto& p:scene.placed){if(p.components.size()!=1)continue;
        if(!p.binding.physicalId||p.binding.physicalId>general.sprites.size()){out.refusal="building_base";return out;}
        const auto& base=general.sprites[p.binding.physicalId-1];if(!base.animationSpriteCount)continue;
        const auto enabled=originalBuildingAnimationEnabled(p.footprint,employment);
        if(!enabled){out.refusal="building_animation_gate";return out;}
        if(!*enabled)continue;
        if(!p.footprint.animationInputs||!p.footprint.animationInputs->phase||
            !originalBuildingAnimationState(base,*p.footprint.animationInputs->phase)||
            !bases.emplace(p.footprint.slot,&base).second){out.refusal="building_animation_phase";return out;}
        out.buildingPhases.emplace(p.footprint.slot,*p.footprint.animationInputs->phase);
    }
    auto context=initial;std::optional<OriginalCombinedState> city;
    if(combined){city=*combined;context.workerRoadExits=combined->figures.workerRoadExits;
        context.migrationRoadEndpoints=combined->figures.migrationRoadEndpoints;
        if(combined->process.crtRandom)context.crtRandom=combined->process.crtRandom;
        if(combined->process.soundEffectsEnabled)context.soundEffectsEnabled=combined->process.soundEffectsEnabled;
        if(combined->process.buildingEditMode)context.buildingEditMode=combined->process.buildingEditMode;city->figures=context;
        if(!city->process.cityClocks){out.refusal="city_clocks_529990_initial_values_required";return out;}}
    for(const auto& frame:frames){
        if(city&&frame.farmerWallClock){city->process.wallClock=frame.farmerWallClock;
            if(city->buildingContext)city->buildingContext->wallClock=frame.farmerWallClock;}
        if(frame.buildingDraws.size()>40000||frame.figureDrawCells.size()>4*51984){out.refusal="draw_budget";return out;}
        out.animationClock=originalAnimationClockFrame(out.animationClock,frame.animationClock);
        auto input=frame.admission;input.last=out.admissionClock;
        const auto admitted=originalFigureAdmission(input);
        const auto count=originalFigureUpdatesBeforeDraw(admitted,frame.continueAfterPass);
        if(!count||*count>10000-out.updates){out.refusal="admitted_update_observations";return out;}
        out.admissionClock=admitted.last;
        for(unsigned step=0;step<*count;++step){
            if(city){city->figures=context;
                out.refusal=originalCombinedBeforeFigures(*city,out.lastUpdateOrder);
                out.figureAllocations=city->figures.allocationCount;out.buildingFigureAllocations=city->figures.buildingAllocationCount;
                if(!out.refusal.empty()){
                    //Diagnostic events include constructors reached before a
                    //later required process input refuses the atomic phase.
                    if(city->buildingContext&&city->buildingContext->allocatedFigures.size()>out.buildingFigureAllocations){
                        out.figureAllocations+=city->buildingContext->allocatedFigures.size()-out.buildingFigureAllocations;
                        out.buildingFigureAllocations=city->buildingContext->allocatedFigures.size();}
                    out.refusedUpdate=out.updates+1;out.lastPass={};return out;}
                OriginalBuildingProfile selectionProfile;
                for(const auto& e:city->buildings)selectionProfile.footprints.push_back(e.second.state.building);
                for(const auto& p:scene.placed){const auto& footprint=city->buildings.at(p.footprint.slot).state.building;
                    const auto alive=city->buildings.at(p.footprint.slot).state.inputs.alive;
                    if(alive!=1&&alive!=3){bases.erase(p.footprint.slot);out.buildingPhases.erase(p.footprint.slot);
                        out.buildingSelections.erase(p.footprint.slot);continue;}
                    const auto previous=out.buildingSelections.find(p.footprint.slot);
                    const auto before=bindOriginalBuildingComponents(previous==out.buildingSelections.end()?p.footprint:previous->second,"DATA/China_General.sg3",general,&selectionProfile);
                    const auto after=bindOriginalBuildingComponents(footprint,"DATA/China_General.sg3",general,&selectionProfile);
                    if(!before||!after){out.refusal="combined_building_image_inputs_slot_"+std::to_string(p.footprint.slot)+
                        "_type_"+std::to_string(footprint.type)+(before?"_after":"_before");out.refusedUpdate=out.updates+1;return out;}
                    bool changed=before->size()!=after->size();
                    if(!changed)for(size_t k=0;k<before->size();++k)changed|=before->at(k).binding.physicalId!=after->at(k).binding.physicalId;
                    if(changed){++out.buildingImageReselections;
                        const auto animated=bases.find(p.footprint.slot);
                        if(animated!=bases.end()){
                            if(after->size()!=1||!after->front().binding.physicalId||after->front().binding.physicalId>general.sprites.size()){
                                out.refusal="combined_reselected_animation_base";out.refusedUpdate=out.updates+1;return out;}
                            const auto& base=general.sprites[after->front().binding.physicalId-1];
                            if(!originalBuildingAnimationState(base,out.buildingPhases.at(p.footprint.slot))){
                                out.refusal="combined_reselected_animation_phase";out.refusedUpdate=out.updates+1;return out;}
                            animated->second=&base;
                        }}
                    out.buildingSelections[p.footprint.slot]=footprint;}
                context=city->figures;out.lastUpdateOrder.push_back(0x4e27e0);}
            context.animationFlags=out.animationClock.flags;
            out.lastPass=originalFigureOrderedNextPass(context,archives);
            if(city){out.figureAllocations=out.lastPass.allocationCount;out.buildingFigureAllocations=out.lastPass.buildingAllocationCount;}
            if(!out.lastPass.complete||!out.lastPass.context){out.refusedUpdate=out.updates+1;
                out.refusal=out.lastPass.firstBlockedSlot?originalFigureUpdateStatusName(out.lastPass.figures.at(out.lastPass.firstBlockedSlot).status):"ordered_pool_inputs";
                if(city&&out.lastPass.firstBlockedSlot){const auto& blocked=context.figures.at(out.lastPass.firstBlockedSlot);
                    out.refusal+="_slot_"+std::to_string(blocked.slot)+"_type_"+std::to_string(blocked.type)+"_action_"+std::to_string(blocked.action);}
                return out;}
            for(const auto& e:context.figures){const auto& before=e.second;const auto& after=out.lastPass.context->figures.at(e.first);
                if(before.type==76&&before.alive==1&&before.action!=10&&after.action==10)++out.preyDeaths;
                if(before.type==40&&before.action==9&&after.type==40&&after.action==8&&before.target>0){
                    const auto prey=context.figures.find(size_t(before.target));
                    if(prey!=context.figures.end()&&prey->second.type==76&&prey->second.action==10)++out.preyCollections;}}
            if(context.buildings&&out.lastPass.context->buildings)for(const auto& p:scene.placed){
                const auto before=context.buildings->find(p.footprint.slot),after=out.lastPass.context->buildings->find(p.footprint.slot);
                if(before==context.buildings->end()||after==out.lastPass.context->buildings->end())continue;
                if(before->second.type>=2&&before->second.type<18&&
                    before->second.type!=after->second.type){
                    auto footprint=p.footprint;if(!footprint.imageInputs){out.refusal="house_image_inputs";return out;}
                    footprint.type=after->second.type;footprint.imageInputs->word20=after->second.residents;
                    if(!bindOriginalBuildingComponents(footprint,"DATA/China_General.sg3",general)){out.refusal="house_image_reselection";return out;}
                    out.buildingHouses[p.footprint.slot]={after->second.type,after->second.residents};++out.buildingImageReselections;}
                if(!after->second.storage)continue;
                bool changed=!before->second.storage||before->second.storage->size()!=after->second.storage->size();
                if(!changed)for(size_t k=0;k<after->second.storage->size();++k){const auto& a=before->second.storage->at(k);const auto& b=after->second.storage->at(k);
                    changed|=a.resource!=b.resource||a.amount!=b.amount;}
                if(changed){auto footprint=p.footprint;if(!footprint.imageInputs){out.refusal="storage_image_inputs";return out;}
                    footprint.imageInputs->compartments=after->second.storage;
                    if(!bindOriginalBuildingComponents(footprint,"DATA/China_General.sg3",general)){out.refusal="storage_image_reselection";return out;}
                    out.buildingStorage[p.footprint.slot]=*after->second.storage;++out.buildingImageReselections;}}
            if(city&&out.lastPass.context->buildings)for(const auto& e:*out.lastPass.context->buildings){
                if(e.second.storage)out.buildingStorage[e.first]=*e.second.storage;}
            if(city){city->figures=*out.lastPass.context;
                out.refusal=originalCombinedSyncBuildings(*city);
                if(out.refusal.empty())out.refusal=originalCombinedAfterFigures(*city,out.lastUpdateOrder);
                if(!out.refusal.empty()){out.refusedUpdate=out.updates+1;out.lastPass={};return out;}
                for(const auto& p:scene.placed){const auto& b=city->buildings.at(p.footprint.slot).state;
                    if(b.inputs.alive==1||b.inputs.alive==3)out.buildingSelections[p.footprint.slot]=b.building;}}
            context=*out.lastPass.context;++out.updates;
        }
        // One supplied slot means one actual draw, repetitions are meaningful.
        for(size_t slot:frame.buildingDraws){const auto base=bases.find(slot);
            if(city&&city->buildings.count(slot)&&city->buildings.at(slot).state.inputs.alive!=1&&city->buildings.at(slot).state.inputs.alive!=3)continue;
            if(base==bases.end()){out.refusal="unbound_building_draw";return out;}
            const auto phase=originalBuildingAnimationDraw(*base->second,out.buildingPhases.at(slot),out.animationClock.flags);
            if(!phase){out.refusal="building_animation_draw";return out;}
            out.buildingAdvances+=out.animationClock.flags[base->second->animationSpeedId];
            out.buildingPhases.at(slot)=*phase;++out.buildingDraws;
        }
        for(uint32_t cell:frame.figureDrawCells)if(!originalFigureSortDrawCell(context,cell)){
            out.refusal="figure_draw_cell_sort";return out;}
        ++out.frames;
    }
    if(city){city->figures=context;city->process.crtRandom=context.crtRandom;
        for(auto& e:city->buildings){const auto phase=out.buildingPhases.find(e.first);
            if(phase!=out.buildingPhases.end()&&e.second.state.building.animationInputs)e.second.state.building.animationInputs->phase=phase->second;}
        if(city->buildingContext&&city->buildingContext->disasters){out.createdBuildings=city->buildingContext->disasters->createdBuildings.size();
            out.destroyedBuildings=city->buildingContext->disasters->destroyedBuildings.size();}
        out.buildingOutcomes=combined->buildings.size();out.combinedState=std::move(city);}
    out.context=std::move(context);out.complete=true;return out;
}
inline OriginalSceneTimeline originalSceneTimeline(const OriginalFigureUpdateContext& initial,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalBuildingScene& scene,const SG3Archive& general,
    const OriginalBuildingEmployment* employment,uint32_t previousAdmission,OriginalAnimationClock animationClock,
    const std::vector<OriginalSceneTimelineFrame>& frames,const OriginalCombinedState* combined=nullptr){
    auto out=originalSceneTimelineImpl(initial,archives,scene,general,employment,previousAdmission,animationClock,frames,combined);
    if(combined&&!out.complete){out.context.reset();out.combinedState.reset();out.lastPass={};
        out.buildingPhases.clear();out.buildingStorage.clear();out.buildingHouses.clear();out.buildingSelections.clear();}
    return out;
}
// Install an entirely completed timeline only. Figure placement and building
// phase replacement are atomic; failed later passes cannot paint a stale prefix.
inline bool installOriginalSceneTimeline(OriginalBuildingScene& scene,const OriginalMapTerrain& terrain,
    const OriginalFigureProfile& saved,const std::map<uint32_t,const SG3Archive*>& archives,
    const OriginalBuildingProfile& buildings,const SG3Archive& general,const OriginalSceneTimeline& timeline,
    const SG3Archive* terrainArchive=nullptr){
    if(!scene.accepted||!timeline.complete||!timeline.context||timeline.context->figures.size()!=2000||
        (!timeline.combinedState&&timeline.context->terrain!=terrain.flags))return false;
    auto updated=saved;updated.live.clear();updated.slots.clear();updated.cellHeads=timeline.context->cellHeads;
    for(const auto& e:timeline.context->figures){updated.slots.push_back(e.second);if(e.second.alive)updated.live.push_back(e.second);}
    const size_t outcomes=saved.live.size()+(timeline.combinedState?timeline.figureAllocations:timeline.context->allocatedSlots.size());
    if(updated.live.size()>outcomes)return false;
    auto next=scene;
    if(timeline.combinedState&&timeline.combinedState->buildingContext){const auto& c=*timeline.combinedState->buildingContext;
        if(c.disasters&&!c.disasters->destroyedBuildings.empty()){
            next.placed.erase(std::remove_if(next.placed.begin(),next.placed.end(),[&](const auto& p){
                const auto alive=timeline.combinedState->buildings.at(p.footprint.slot).state.inputs.alive;return alive!=1&&alive!=3;}),next.placed.end());
            next.overlays.erase(std::remove_if(next.overlays.begin(),next.overlays.end(),[&](const auto& p){
                return p.mode!=4&&std::find(c.disasters->destroyedBuildings.begin(),c.disasters->destroyedBuildings.end(),p.sourceSlot)!=c.disasters->destroyedBuildings.end();}),next.overlays.end());
            next.surface=c.resourceGrowth->surface;next.surface.terrain=timeline.context->terrain;
            next.rebuiltMovementTerrain=timeline.context->terrain;
            next.buildingOccupancy.assign(51984,0);for(size_t cell=0;cell<51984;++cell)next.buildingOccupancy[cell]=timeline.context->buildingOwners.at(cell)!=0;}
        if(c.resourceGrowth&&(c.resourceGrowth->calls||c.desirabilityCalls)){const auto& s=c.resourceGrowth->surface;
            if(s.image.size()!=51984||s.subTile.size()!=51984)return false;next.surface.image=s.image;next.surface.subTile=s.subTile;}
        if(c.irrigation&&c.irrigationCalls){if(c.irrigation->image.size()!=51984)return false;next.surface.image=c.irrigation->image;}
        for(auto& p:next.placed){const auto selected=timeline.buildingSelections.find(p.footprint.slot);if(selected==timeline.buildingSelections.end())return false;
            const auto components=bindOriginalBuildingComponents(selected->second,"DATA/China_General.sg3",general,&buildings);if(!components)return false;
            p.footprint=selected->second;p.components=*components;p.binding=components->front().binding;
            for(const auto& component:p.components){if(!timeline.context->cropBase||component.binding.physicalId<201)return false;
                for(unsigned y=0;y<component.side;++y)for(unsigned x=0;x<component.side;++x){
                    const size_t cell=*timeline.context->cropBase+(p.footprint.y+component.y+y)*228+p.footprint.x+component.x+x;
                    if(cell>=next.surface.image.size())return false;next.surface.image[cell]=(component.binding.tileset<<14)|(component.binding.physicalId-201);}}}}
    for(auto& p:next.placed){const auto house=timeline.buildingHouses.find(p.footprint.slot);if(house==timeline.buildingHouses.end())continue;
        if(!p.footprint.imageInputs||!timeline.context->cropBase)return false;
        p.footprint.type=house->second.first;p.footprint.imageInputs->word20=house->second.second;
        const auto components=bindOriginalBuildingComponents(p.footprint,"DATA/China_General.sg3",general);if(!components)return false;
        p.components=*components;p.binding=components->front().binding;
        if(p.binding.physicalId<201)return false;
        for(unsigned y=0;y<originalFootprintHeight(p.footprint);++y)for(unsigned x=0;x<originalFootprintWidth(p.footprint);++x){
            const size_t cell=*timeline.context->cropBase+(p.footprint.y+y)*228+p.footprint.x+x;
            if(cell>=next.surface.image.size())return false;
            next.surface.image[cell]=(p.binding.tileset<<14)|(p.binding.physicalId-201);}}
    for(auto& p:next.placed){const auto storage=timeline.buildingStorage.find(p.footprint.slot);if(storage==timeline.buildingStorage.end())continue;
        if(!p.footprint.imageInputs||!timeline.context->cropBase)return false;
        p.footprint.imageInputs->compartments=storage->second;
        const auto components=bindOriginalBuildingComponents(p.footprint,"DATA/China_General.sg3",general);if(!components)return false;
        p.components=*components;p.binding=components->front().binding;
        for(const auto& c:p.components){const size_t cell=*timeline.context->cropBase+(p.footprint.y+c.y)*228+p.footprint.x+c.x;
            if(cell>=next.surface.image.size()||c.binding.physicalId<201)return false;
            next.surface.image[cell]=(c.binding.tileset<<14)|(c.binding.physicalId-201);}}
    if(timeline.combinedState&&timeline.combinedState->buildingContext&&timeline.combinedState->buildingContext->disasters){
        const auto& c=*timeline.combinedState->buildingContext;
        for(const auto& ruin:c.disasters->createdBuildings){if(!terrainArchive||!ruin.imageInputs||ruin.type!=161||ruin.size!=1)return false;
            const auto& state=timeline.combinedState->buildings.at(ruin.slot).state;if(state.inputs.alive!=1&&state.inputs.alive!=3)continue;
            const uint32_t word=next.surface.image.at(ruin.imageInputs->gridIndex),id=(word&0x3fff)+201;
            if(word>>14!=3||id>terrainArchive->sprites.size())return false;
            const auto& sprite=terrainArchive->sprites[id-1];if(sprite.type!=30||sprite.width!=78||sprite.uncompressedLength!=3200)return false;
            OriginalBuildingImageBinding binding;binding.archive="DATA/China_Terrain.sg3";binding.tileset=3;binding.imageCode=0x612;
            binding.variant=uint16_t(*ruin.imageInputs->randomByte&7);binding.physicalId=id;binding.archiveIndex=id-1;
            next.placed.push_back({state.building,binding,ruin.x,ruin.y,{{binding,0,0,1}}});}
        next.liveRecords=scene.liveRecords+timeline.createdBuildings-timeline.destroyedBuildings;}
    next.overlays.erase(std::remove_if(next.overlays.begin(),next.overlays.end(),[](const auto& o){return o.mode==3;}),next.overlays.end());
    next.animationSelected=0;
    for(const auto& p:next.placed){const auto found=timeline.buildingPhases.find(p.footprint.slot);if(found==timeline.buildingPhases.end())continue;
        const unsigned frame=found->second&127;if(!frame)continue;
        const size_t id=p.binding.physicalId+frame;if(id>general.sprites.size())return false;
        const auto offset=originalBuildingAnimationOffset(general.sprites[p.binding.physicalId-1],general.sprites[id-1],p.footprint.size);
        if(!offset)return false;
        next.overlays.push_back({"DATA/China_General.sg3",uint32_t(id),2,uint16_t(p.drawX),uint16_t(p.drawY),offset->first,offset->second,
            {p.drawX+p.drawY,p.drawX,2,p.footprint.slot},0,3,p.footprint.slot});++next.animationSelected;
    }
    auto currentTerrain=terrain;currentTerrain.flags=timeline.context->terrain;
    installOriginalFigureOverlays(next,currentTerrain,updated,archives,0,OriginalFigureRenderSource::Saved,&buildings);
    next.figureRemoved=outcomes-updated.live.size();next.figureLive=outcomes;
    next.figurePostLoadK1=timeline.updates==1;next.timelineUpdates=timeline.updates;next.timelineFrames=timeline.frames;
    next.timelineCombined=timeline.combinedState.has_value();
    scene=std::move(next);return true;
}
}
