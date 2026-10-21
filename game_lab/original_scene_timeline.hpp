#pragma once
#include "original_building_scene.hpp"
#include "original_building_update.hpp"
#include "original_city_clocks.hpp"

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
};
struct OriginalCombinedBuilding {
    OriginalBuildingUpdateState state;
    OriginalBuildingUpdateContext context;
};
struct OriginalCombinedState {
    OriginalFigureUpdateContext figures;
    std::map<size_t,OriginalCombinedBuilding> buildings;
    OriginalCombinedProcessInputs process;
    uint32_t phase=0;
};
// Checks the ENTIRE phase, including work outside the per-building API.
// 0/49 have no case; empty table cases target4AC4C8.50 tails into4AC650.
inline std::string originalCombinedPhaseBlocker(uint32_t phase){
    switch(phase){
    case 0:case 2:case 14:case 15:case 19:case 26:case 34:case 41:case 42:case 46:case 49:return {};
    case 48:return {}; //closed common byte52 countdown; no figure-link consumer
    case 35:return "shared_house_service_bridge_517b40_required";
    case 1:return "city_accounting_4f1560_4ac2fa";
    case 20:return "terrain_resource_growth_4bc440_after_51e4a0";
    case 50:return "monthly_city_4ac650";
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
        s.inputs.output=link.outputResource;s.inputs.available=link.roadAccess;s.inputs.network=link.category;
        if(s.building.wellImageInputs)s.building.wellImageInputs->byte6f=link.upgrade;
        if(s.building.animationInputs)s.building.animationInputs->workers=link.workers;
        if(s.building.imageInputs){if(link.type>=2&&link.type<18)s.building.imageInputs->word20=link.residents;
            if(link.storage)s.building.imageInputs->compartments=link.storage;}
        if(s.production){s.production->workers=link.workers;s.production->primaryStock=link.industryInput;
            s.production->secondaryStock=link.hunterReturnGoods;s.production->operational=link.serviceCount;
            s.production->enabled=link.destinationEnabled;}
        if(s.inputs.industry&&link.cropDelivery)originalBuildingSetCropFlags(s.inputs,link.cropDelivery->flags);
        if(!s.inputs.house)continue;
        if(!link.religion||!link.entertainment||!link.herbalist||!link.waterService||!link.houseGoods||!link.foodQuality)
            return "shared_house_service_fields_required";
        auto& h=*s.inputs.house;
        for(unsigned n=0;n<4;++n)h.at(13-4+n)=link.religion->at(n);
        for(unsigned n=0;n<12;++n){const uint16_t v=uint16_t(link.houseGoods->at(n));h.at(17-4+2*n)=uint8_t(v);h.at(18-4+2*n)=uint8_t(v>>8);}
        for(unsigned n=0;n<3;++n)h.at((n==2?45:42+n)-4)=link.entertainment->at(n);
        h.at(44-4)=*link.herbalist;h.at(49-4)=link.waterService->at(0);h.at(51-4)=link.waterService->at(1);
        h.at(53-4)=*link.foodQuality;
    }
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
    const auto blocker=originalCombinedPhaseBlocker(city.phase);if(!blocker.empty())return blocker;
    for(auto& e:city.buildings){auto& b=e.second;
        if(e.first!=b.state.building.slot||b.context.scope!=OriginalBuildingUpdateScope::AdmittedTicks)
            return "combined_building_pool_inputs";
        if(!city.figures.buildings||!city.figures.buildings->count(e.first))return "combined_building_link_required";
        const auto& link=city.figures.buildings->at(e.first);
        if(link.type!=b.state.building.type)return "shared_building_type_bridge_required";
        b.state.inputs.alive=link.alive;
        // The accepted phases read no figure-written fields. The per-record
        // public API is isolated only for this single, ordered operation.
        b.context.phase=city.phase;
        const auto result=originalBuildingStateAfterUpdates(b.state,b.context,1);
        if(result.blocker!=OriginalBuildingUpdateBlocker::None||!result.state||!result.context)
            return originalBuildingUpdateBlockerName(result.blocker);
        b.state=*result.state;b.context=*result.context;
    }
    ++city.phase; //50 was refused before4AC650; no invented wrap/month writes.
    if(city.figures.goods)city.figures.goods->savedPhase=city.phase;
    order.push_back(0x466e90);
    if((!p.managerGate8||*p.managerGate8)&&!p.eventRecordModes)return "scenario_manager_492430_records_required";
    if(p.eventRecordModes&&p.eventRecordModes->size()>150)return "scenario_manager_492430_records_required";
    //49244D skips every non-mode5 row;425250 false skips mode5 as well.
    if(p.eventRecordModes)for(uint8_t mode:*p.eventRecordModes)if(mode==5&&(!p.managerGate8||*p.managerGate8))return "scenario_manager_492430_4ed3b0";
    if(!p.eventSlots||!p.eventProgress)return "scenario_events_466e90_slots_required";
    if(std::any_of(p.eventSlots->begin(),p.eventSlots->end(),[](uint32_t slot){return slot!=0;}))return "scenario_slot_467680_not_closed";
    *p.eventProgress=0; //466EA6; both8-slot loops do no calls for null slots.
    return {};
}
inline std::string originalCombinedAfterFigures(OriginalCombinedState& city,std::vector<uint32_t>& order){
    auto& p=city.process;p.randomHistory=city.figures.services->randomHistory;
    order.push_back(0x564b50);
    if(!p.wildlifeCounter||!p.wildlifePeriod)return "wildlife_564b50_counter_period_required";
    const uint32_t next=*p.wildlifeCounter+1u;*p.wildlifeCounter=next;
    //564B5C/63 uses signed JLE, and stores the increment even on early return.
    if(originalBuildingI32(next)>originalBuildingI32(*p.wildlifePeriod))return "wildlife_scan_564b50_not_closed";
    return {};
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
    if(combined){city=*combined;city->figures=initial;
        if(!city->process.cityClocks){out.refusal="city_clocks_529990_initial_values_required";return out;}}
    for(const auto& frame:frames){
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
                if(!out.refusal.empty()){out.refusedUpdate=out.updates+1;out.lastPass={};return out;}
                context=city->figures;out.lastUpdateOrder.push_back(0x4e27e0);}
            context.animationFlags=out.animationClock.flags;
            out.lastPass=originalFigureOrderedNextPass(context,archives);
            if(!out.lastPass.complete||!out.lastPass.context){out.refusedUpdate=out.updates+1;
                out.refusal=out.lastPass.firstBlockedSlot?originalFigureUpdateStatusName(out.lastPass.figures.at(out.lastPass.firstBlockedSlot).status):"ordered_pool_inputs";
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
            if(city){city->figures=*out.lastPass.context;
                out.refusal=originalCombinedSyncBuildings(*city);
                if(out.refusal.empty())out.refusal=originalCombinedAfterFigures(*city,out.lastUpdateOrder);
                if(!out.refusal.empty()){out.refusedUpdate=out.updates+1;out.lastPass={};return out;}}
            context=*out.lastPass.context;++out.updates;
        }
        // One supplied slot means one actual draw, repetitions are meaningful.
        for(size_t slot:frame.buildingDraws){const auto base=bases.find(slot);
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
    if(city){city->figures=context;
        for(auto& e:city->buildings){const auto phase=out.buildingPhases.find(e.first);
            if(phase!=out.buildingPhases.end()&&e.second.state.building.animationInputs)e.second.state.building.animationInputs->phase=phase->second;}
        out.buildingOutcomes=city->buildings.size();out.combinedState=std::move(city);}
    out.context=std::move(context);out.complete=true;return out;
}
inline OriginalSceneTimeline originalSceneTimeline(const OriginalFigureUpdateContext& initial,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalBuildingScene& scene,const SG3Archive& general,
    const OriginalBuildingEmployment* employment,uint32_t previousAdmission,OriginalAnimationClock animationClock,
    const std::vector<OriginalSceneTimelineFrame>& frames,const OriginalCombinedState* combined=nullptr){
    auto out=originalSceneTimelineImpl(initial,archives,scene,general,employment,previousAdmission,animationClock,frames,combined);
    if(combined&&!out.complete){out.context.reset();out.combinedState.reset();out.lastPass={};
        out.buildingPhases.clear();out.buildingStorage.clear();out.buildingHouses.clear();}
    return out;
}
// Install an entirely completed timeline only. Figure placement and building
// phase replacement are atomic; failed later passes cannot paint a stale prefix.
inline bool installOriginalSceneTimeline(OriginalBuildingScene& scene,const OriginalMapTerrain& terrain,
    const OriginalFigureProfile& saved,const std::map<uint32_t,const SG3Archive*>& archives,
    const OriginalBuildingProfile& buildings,const SG3Archive& general,const OriginalSceneTimeline& timeline){
    if(!scene.accepted||!timeline.complete||!timeline.context||timeline.context->figures.size()!=2000||
        timeline.context->terrain!=terrain.flags)return false;
    auto updated=saved;updated.live.clear();updated.slots.clear();updated.cellHeads=timeline.context->cellHeads;
    for(const auto& e:timeline.context->figures){updated.slots.push_back(e.second);if(e.second.alive)updated.live.push_back(e.second);}
    const size_t outcomes=saved.live.size()+timeline.context->allocatedSlots.size();
    if(updated.live.size()>outcomes)return false;
    auto next=scene;
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
    installOriginalFigureOverlays(next,terrain,updated,archives,0,OriginalFigureRenderSource::Saved,&buildings);
    next.figureRemoved=outcomes-updated.live.size();next.figureLive=outcomes;
    next.figurePostLoadK1=timeline.updates==1;next.timelineUpdates=timeline.updates;next.timelineFrames=timeline.frames;
    next.timelineCombined=timeline.combinedState.has_value();
    scene=std::move(next);return true;
}
}
