#pragma once
#include "original_building_scene.hpp"

namespace game_lab {
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
};
struct OriginalSceneTimeline {
    bool complete=false;
    unsigned updates=0,refusedUpdate=0;
    size_t frames=0,buildingDraws=0,buildingAdvances=0;
    size_t buildingImageReselections=0;
    uint32_t admissionClock=0;
    OriginalAnimationClock animationClock;
    std::map<size_t,uint8_t> buildingPhases;
    std::optional<OriginalFigureUpdateContext> context;
    std::map<size_t,std::vector<OriginalStorageCompartment>> buildingStorage;
    OriginalFigureOrderedPass lastPass;
    std::string refusal;
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
inline OriginalSceneTimeline originalSceneTimeline(const OriginalFigureUpdateContext& initial,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalBuildingScene& scene,const SG3Archive& general,
    const OriginalBuildingEmployment* employment,uint32_t previousAdmission,OriginalAnimationClock animationClock,
    const std::vector<OriginalSceneTimelineFrame>& frames){
    OriginalSceneTimeline out;out.admissionClock=previousAdmission;out.animationClock=animationClock;
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
    auto context=initial;
    for(const auto& frame:frames){
        if(frame.buildingDraws.size()>40000||frame.figureDrawCells.size()>4*51984){out.refusal="draw_budget";return out;}
        out.animationClock=originalAnimationClockFrame(out.animationClock,frame.animationClock);
        auto input=frame.admission;input.last=out.admissionClock;
        const auto admitted=originalFigureAdmission(input);
        const auto count=originalFigureUpdatesBeforeDraw(admitted,frame.continueAfterPass);
        if(!count||*count>10000-out.updates){out.refusal="admitted_update_observations";return out;}
        out.admissionClock=admitted.last;
        for(unsigned step=0;step<*count;++step){
            out.lastPass=originalFigureOrderedNextPass(context,archives);
            if(!out.lastPass.complete||!out.lastPass.context){out.refusedUpdate=out.updates+1;
                out.refusal=out.lastPass.firstBlockedSlot?originalFigureUpdateStatusName(out.lastPass.figures.at(out.lastPass.firstBlockedSlot).status):"ordered_pool_inputs";
                return out;}
            if(context.buildings&&out.lastPass.context->buildings)for(const auto& p:scene.placed){
                const auto before=context.buildings->find(p.footprint.slot),after=out.lastPass.context->buildings->find(p.footprint.slot);
                if(before==context.buildings->end()||after==out.lastPass.context->buildings->end()||!after->second.storage)continue;
                bool changed=!before->second.storage||before->second.storage->size()!=after->second.storage->size();
                if(!changed)for(size_t k=0;k<after->second.storage->size();++k){const auto& a=before->second.storage->at(k);const auto& b=after->second.storage->at(k);
                    changed|=a.resource!=b.resource||a.amount!=b.amount;}
                if(changed){auto footprint=p.footprint;if(!footprint.imageInputs){out.refusal="storage_image_inputs";return out;}
                    footprint.imageInputs->compartments=after->second.storage;
                    if(!bindOriginalBuildingComponents(footprint,"DATA/China_General.sg3",general)){out.refusal="storage_image_reselection";return out;}
                    out.buildingStorage[p.footprint.slot]=*after->second.storage;++out.buildingImageReselections;}}
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
    out.context=std::move(context);out.complete=true;return out;
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
    if(updated.live.size()>saved.live.size())return false;
    auto next=scene;
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
    next.figureRemoved=saved.live.size()-updated.live.size();next.figureLive=saved.live.size();
    next.figurePostLoadK1=timeline.updates==1;next.timelineUpdates=timeline.updates;next.timelineFrames=timeline.frames;
    scene=std::move(next);return true;
}
}
