// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include "../original_pak.hpp"
#include "../original_kernel_checkpoint.hpp"
#include "../original_world_checkpoint_codec.hpp"
#include "../../file_io/include/city19_goal_archive.hpp"
#include <filesystem>
#include <set>
#include <sstream>
#include <algorithm>

namespace game_lab {
void AppMapSurface::clear() {
    playback_.reset();
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
        for(const uint32_t slot:{14u,17u,38u,39u,40u}){
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
bool AppMapSurface::preparePostLoadInputs(const OriginalSaveResult& save,const std::vector<uint8_t>& payload,
    std::map<uint32_t,SG3Archive>& owned,std::map<uint32_t,const SG3Archive*>& refs,
    OriginalFigureUpdateContext& initial,OriginalBuildingEmployment& employment,std::string& buildingText){
    if(save.status!=OriginalSaveStatus::LoadedSnapshot||!save.layers.figures||!save.layers.ground||!save.layers.ground->valid())return false;
    const auto& figures=*save.layers.figures;const auto& ground=*save.layers.ground;
    std::vector<uint8_t> bytes;
    if(!detail::readFile(root_.resolve("Model/EmperorFigureModels.txt"),100000,bytes))return false;
    const auto difficulty=originalFigureSavedDifficulty(payload,save.sizeOffset);if(!difficulty)return false;
    const auto modelText=std::string(bytes.begin(),bytes.end());const auto models=originalFigureModels(modelText,difficulty);
    const auto modelSpeeds=originalFigureModelSpeeds(modelText);
    if(!detail::readFile(root_.resolve("Model/EmperorBuildingModels.txt"),1048576,bytes))return false;
    buildingText=std::string(bytes.begin(),bytes.end());
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
    //CRT holdrand is wall-clock seeded and absent from SAV;the caller supplies it.
    context.waterOverride=waterOverride;
    context.watchMaximum=0; //explicit fresh-process scratchD95080.
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
    for(unsigned slot:{14u,17u,38u,39u,40u}){owned.emplace(slot,loadSG3(root_.resolve(std::string("DATA/")+originalFigureArchiveName(slot))));refs[slot]=&owned.at(slot);}
    if(!selectSavedBuildings(save.terrain,save.buildings,&ground,0,&figures))return false;
    const auto fullPassability=originalSceneMovementPassability(*savedBuildings_,save.buildings,ground,*spawn);
    const auto* timelinePassability=fullPassability?&*fullPassability:passability?&*passability:nullptr;
    const auto loaded=originalFigurePostLoadContext(figures,refs,save.terrain.flags,*modelSpeeds,&*services,timelinePassability,
        &*hunters,&*buildings,*models,spawn->cropBase,fullPassability?fullPassability->cells:masks?*masks:std::vector<uint16_t>{},&context);
    if(!loaded.context)return false;
    employment=*buildingEmployment;initial=*loaded.context;
    return true;
}
bool AppMapSurface::selectPostLoadTimeline(const OriginalSaveResult& save,const std::vector<uint8_t>& payload,
    const std::vector<OriginalSceneTimelineFrame>& suppliedFrames,uint32_t previousAdmission,OriginalAnimationClock clocks){
    clear();
    std::map<uint32_t,SG3Archive> owned;std::map<uint32_t,const SG3Archive*> refs;
    OriginalFigureUpdateContext initial;OriginalBuildingEmployment employment;std::string buildingText;
    if(!preparePostLoadInputs(save,payload,owned,refs,initial,employment,buildingText))return false;
    const auto& figures=*save.layers.figures;const auto& ground=*save.layers.ground;
    std::optional<OriginalCombinedState> combined;
    for(size_t n=0;n<suppliedFrames.size();++n)if(suppliedFrames[n].combinedProcess){
        if(n){clear();error_="combined process inputs belong to the first frame";return false;}
        combined=originalCombinedSavedState(initial,save.buildings,payload,save.terrainOffset,save.sizeOffset,*suppliedFrames[n].combinedProcess);
        if(!combined){clear();error_="combined saved building inputs";return false;}
        std::vector<uint8_t> speechBytes;
        if(!detail::readFile(root_.resolve("EmperorText.eng"),8u*1024u*1024u,speechBytes)){clear();error_="combined speech groups";return false;}
        if(!originalCombinedBindBuildingServices(*combined,save.buildings,figures,payload,save.terrainOffset,save.sizeOffset,
            ground,*savedBuildings_,archives_.at(2).index,archives_.at(3).index,buildingText,originalBuildingSpeechCounts(speechBytes))){
            clear();error_="combined saved city services";return false;}}
    const auto timeline=originalSceneTimeline(initial,refs,*savedBuildings_,archives_.at(2).index,&employment,previousAdmission,clocks,suppliedFrames,combined?&*combined:nullptr);
    if(!timeline.complete||!installOriginalSceneTimeline(*savedBuildings_,save.terrain,figures,refs,save.buildings,archives_.at(2).index,timeline,&archives_.at(3).index)){
        clear();error_="post-load timeline: "+timeline.refusal+" (update "+std::to_string(timeline.refusedUpdate)+")";return false;}
    terrain_.flags=timeline.context->terrain;
    return true;
}
// One explicitly premised combined timeline replayed from the saved snapshot.
// The snapshot scene is never mutated: each step installs into a copy, so a
// refused frame keeps the saved or last admitted picture on screen.
struct AppMapSurface::Playback {
    OriginalSaveResult save;
    OriginalFigureProfile figures;
    OriginalBuildingProfile buildings;
    OriginalBuildingScene seed;
    OriginalFigureUpdateContext initial;
    OriginalCombinedState combined;
    AppCityNotifications notifications;
    std::map<uint32_t,SG3Archive> archives;
    std::map<uint32_t,const SG3Archive*> refs;
    OriginalBuildingEmployment employment;
    SG3Archive general,terrainArchive;
    std::vector<size_t> buildingDraws;
    std::vector<uint32_t> figureDrawCells;
    std::vector<OriginalSceneTimelineFrame> frames;
    bool recordReplay=false;
    OriginalCombinedProcessInputs process;
    AppCitySessionState session;
    OriginalKernelIdentities identities;
    std::optional<OriginalWorldSourceBinding> worldSource;
    std::optional<OriginalWorldPolicy> worldPolicy;
    uint64_t worldRevision=1;
    OriginalWorldControllerStatus worldController;
    std::string resourceRoot,buildingModelText,figureModelText;
    using ResourceStamp=std::pair<uintmax_t,std::filesystem::file_time_type>;
    std::map<std::string,ResourceStamp> resourceStamps;
    bool resourcesCurrent()const{for(const auto& [path,stamp]:resourceStamps){std::error_code error;
        const auto size=std::filesystem::file_size(path,error);if(error||size!=stamp.first)return false;
        const auto time=std::filesystem::last_write_time(path,error);if(error||time!=stamp.second)return false;}return true;}
    std::map<size_t,OriginalMarketKnownBase> marketBases;
    static std::unique_ptr<Playback> clone(const Playback& value){
        auto out=std::make_unique<Playback>(value);out->refs.clear();
        for(const auto& entry:out->archives)out->refs.emplace(entry.first,&entry.second);return out;
    }
    // Chained continuation: each admitted frame resumes the complete state of
    // the previous admitted prefix (figures, buildings, process scratch). The
    // snapshot replay above is the fallback for the first frame.
    bool chained=false;
    OriginalFigureUpdateContext chainInitial;
    OriginalCombinedState chainCombined;
    // Previous admitted building image selection per seed slot. The library's
    // reselection compares against its own previous selection, so a chained
    // step needs the same footprint/binding/components the installed scene had.
    std::map<size_t,OriginalBuildingPlacement> chainSelections;
    // Animation phases advanced by the draws of the previous admitted prefix.
    // installOriginalSceneTimeline captures buildingSelections before the draw
    // loop, so the installed placement phase lags the timeline by one draw and
    // cannot be used to resume a chained step.
    std::map<size_t,uint8_t> chainPhases;
    uint32_t chainAdmission=0;
    OriginalAnimationClock chainClocks;
    unsigned updates=0;
    size_t frames_=0,draws_=0,advances_=0,reselections_=0,preyDeaths_=0,preyCollections_=0;
};
// Opaque owner of every mutable continuation plus its archive lifetimes.
struct AppMapSurface::CityCheckpoint {
    std::shared_ptr<const Playback> playback;
    OriginalMapTerrain terrain;
    std::optional<OriginalTerrainState> state;
    std::optional<OriginalBuildingScene> scene;
    std::map<uint32_t,Archive> archives;
    std::optional<OriginalMapCamera> camera;
};
std::shared_ptr<const AppMapSurface::CityCheckpoint> AppMapSurface::cityCheckpoint()const{
    if(!playback_||!savedBuildings_)return {};
    const auto& world=playback_->chained?playback_->chainCombined:playback_->combined;
    const auto& figures=playback_->chained?playback_->chainInitial:playback_->initial;
    OriginalKernelCheckpoint represented;represented.sourceRevision=playback_->save.snapshotRevision;represented.modelRevision=1;
    represented.city=world;represented.marketBases=playback_->marketBases;
    for(auto& row:represented.marketBases)row.second=originalMarketKnownBaseAt(row.second,figures.figures.at(row.first));
    if(!originalKernelCheckpointIssue(represented).empty()||!playback_->identities.consistent(figures.figures))return {};
    auto checkpoint=std::make_shared<CityCheckpoint>();auto owned=Playback::clone(*playback_);owned->marketBases=std::move(represented.marketBases);checkpoint->playback=std::move(owned);
    checkpoint->terrain=terrain_;checkpoint->state=state_;checkpoint->scene=savedBuildings_;checkpoint->archives=archives_;checkpoint->camera=lastPlaybackCamera_;return checkpoint;
}
bool AppMapSurface::restoreCityCheckpoint(const CityCheckpoint& checkpoint,uint64_t expectedSourceRevision){
    try{
        if(!checkpoint.playback||!checkpoint.scene||!checkpoint.playback->save.snapshotRevision||
           checkpoint.playback->save.snapshotRevision!=expectedSourceRevision||checkpoint.playback->resourceRoot!=root_.resolve(""))return false;
        const auto& cp=*checkpoint.playback;if(!cp.resourcesCurrent())return false;std::vector<uint8_t> model;
        if(!detail::readFile(root_.resolve("Model/EmperorBuildingModels.txt"),1048576,model)||std::string(model.begin(),model.end())!=cp.buildingModelText)return false;
        if(!detail::readFile(root_.resolve("Model/EmperorFigureModels.txt"),100000,model)||std::string(model.begin(),model.end())!=cp.figureModelText)return false;
        const auto& world=cp.chained?cp.chainCombined:cp.combined;const auto& figures=cp.chained?cp.chainInitial:cp.initial;
        OriginalKernelCheckpoint represented;represented.sourceRevision=cp.save.snapshotRevision;represented.modelRevision=1;represented.city=world;represented.marketBases=cp.marketBases;
        if(!originalKernelCheckpointIssue(represented).empty()||!cp.identities.consistent(figures.figures))return false;
        AppMapSurface candidate(root_,spriteCache_);candidate.playback_=Playback::clone(cp);
        std::string issue;const auto identities=cp.identities.reloaded(issue);const auto notices=cp.notifications.reloaded();if(!identities||!notices)return false;
        candidate.playback_->identities=*identities;candidate.playback_->notifications=*notices;
        candidate.terrain_=checkpoint.terrain;candidate.state_=checkpoint.state;candidate.savedBuildings_=checkpoint.scene;candidate.archives_=checkpoint.archives;candidate.lastPlaybackCamera_=checkpoint.camera;
        auto camera=checkpoint.camera.value_or(OriginalMapCamera{});if(!checkpoint.camera){camera.width=256;camera.height=192;camera.centerX=camera.centerY=double(cp.save.terrain.activeSide)/2;}
        const auto view=candidate.renderSavedBuildings(camera);
        if(!view.image.valid()||view.bases.decodeFailures||view.overlayDecodeFailures||view.overlayInvalid||view.groundUnresolved||!cp.resourcesCurrent())return false;
        if(publicationProbe_)publicationProbe_("checkpoint_prepared");swapPrepared(candidate);error_.clear();return true;
    }catch(const std::exception&){return false;}
}

bool AppMapSurface::prepareBoundCityPlayback(const OriginalWorldBoundSource& source,const OriginalWorldPolicy& policy,const AppCitySessionState& session,bool replay){
    try{
        std::string error;if(source.binding.legacyRevision!=source.save.snapshotRevision||source.binding.payloadDigest!=originalWorldDigest(source.payload.data(),source.payload.size())||!verifyOriginalWorldBinding(root_,source.binding,error)){error_=error.empty()?"world_source_context_mismatch":error;return false;}
        AppMapSurface candidate(root_,spriteCache_);candidate.publicationProbe_=publicationProbe_;
        if(!candidate.prepareCityPlaybackCandidate(source.save,source.payload,policy.initial,&session,replay)){error_=candidate.error_;return false;}
        if(!verifyOriginalWorldBinding(root_,source.binding,error)){error_=error;return false;}
        candidate.playback_->worldSource=source.binding;candidate.playback_->worldPolicy=policy;
        if(publicationProbe_)publicationProbe_("bound_source_prepared");swapPrepared(candidate);error_.clear();return true;
    }catch(const std::exception& e){error_=e.what();return false;}
}
std::optional<OriginalWorldSnapshot> AppMapSurface::worldSnapshot(std::string& error)const{
    try{
        if(!playback_||!savedBuildings_||!playback_->worldSource||!playback_->worldPolicy){error="world_source_context_required";return std::nullopt;}
        const auto& p=*playback_;if(!verifyOriginalWorldBinding(root_,*p.worldSource,error))return std::nullopt;
        OriginalWorldSnapshot out;out.commitRevision=p.worldRevision;out.source=*p.worldSource;out.policy=*p.worldPolicy;
        out.city=p.chained?p.chainCombined:p.combined;
        const auto& actual=p.chained?p.chainInitial:p.initial;
        // The complete value copies must have the same authoritative pool.
        original_world_codec_detail::Writer a,b;a.value(actual.figures);b.value(out.city.figures.figures);if(a.bytes!=b.bytes){error="world_active_figure_alias";return std::nullopt;}
        out.marketBases=p.marketBases;for(auto& row:out.marketBases)row.second=originalMarketKnownBaseAt(row.second,out.city.figures.figures.at(row.first));
        out.terrain={terrain_,state_};out.scene.shown=*savedBuildings_;out.scene.selections=p.chainSelections;out.scene.phases=p.chainPhases;
        auto& c=out.continuation;c.controller=p.worldController;if(!p.chained)c.initialFigures=p.initial;c.chained=p.chained;c.admission=p.chained?p.chainAdmission:p.session.admissionClock;c.clocks=p.chained?p.chainClocks:p.session.animationClock;c.session=p.session;
        c.buildingDraws=p.buildingDraws;c.figureDrawCells=p.figureDrawCells;c.updates=p.updates;c.frames=p.frames_;c.draws=p.draws_;c.advances=p.advances_;c.reselections=p.reselections_;c.preyDeaths=p.preyDeaths_;c.preyCollections=p.preyCollections_;
        out.ledger=p.identities.persistent();out.notifications=p.notifications.persistent();out.camera=lastPlaybackCamera_;
        error=originalWorldCheckpointValueIssue(out);if(!error.empty())return std::nullopt;return out;
    }catch(const std::exception& e){error=e.what();return std::nullopt;}
}
uint64_t AppMapSurface::worldRevision()const{return playback_?playback_->worldRevision:0;}
bool AppMapSurface::restoreWorldSnapshot(const OriginalWorldSnapshot& s,std::string& error){
    try{
        error=originalWorldCheckpointValueIssue(s);if(!error.empty())return false;
        if(!verifyOriginalWorldBinding(root_,s.source,error))return false;
        const auto source=bindOriginalWorldSource(root_,s.source.source.relative,error);if(!source)return false;
        if(source->binding.source.digest!=s.source.source.digest||source->binding.payloadDigest!=s.source.payloadDigest){error="world_restore_source_changed";return false;}
        AppMapSurface candidate(root_,spriteCache_);const auto& save=source->save;
        // Pure saved-scene composition: no originalFigurePostLoadContext,
        // population recount, RNG, constructors, callbacks or date update.
        if(!candidate.selectSavedBuildings(save.terrain,save.buildings,save.layers.ground?&*save.layers.ground:nullptr,0,save.layers.figures?&*save.layers.figures:nullptr)){error="world_restore_seed_scene";return false;}
        auto p=std::make_unique<Playback>();p->save=save;p->figures=*save.layers.figures;p->buildings=save.buildings;p->seed=*candidate.savedBuildings_;
        p->general=candidate.archives_.at(2).index;p->terrainArchive=candidate.archives_.at(3).index;
        for(uint32_t slot:{14u,17u,38u,39u,40u}){const auto found=candidate.archives_.find(slot);if(found==candidate.archives_.end()){error="world_restore_archive";return false;}p->archives.emplace(slot,found->second.index);p->refs.emplace(slot,&p->archives.at(slot));}
        std::vector<uint8_t> text;if(!detail::readFile(root_.resolve("Model/EmperorBuildingModels.txt"),1048576,text)){error="world_restore_model";return false;}p->buildingModelText=std::string(text.begin(),text.end());const auto employment=parseOriginalBuildingEmployment(p->buildingModelText);if(!employment){error="world_restore_model";return false;}p->employment=*employment;
        if(!detail::readFile(root_.resolve("Model/EmperorFigureModels.txt"),100000,text)){error="world_restore_model";return false;}p->figureModelText=std::string(text.begin(),text.end());p->resourceRoot=root_.resolve("");
        for(const auto& row:candidate.archives_){const auto& name=row.second.companion;auto index=std::filesystem::path(name);index.replace_extension(".sg3");for(const auto& path:{name,index.string()}){std::error_code ec;const auto size=std::filesystem::file_size(path,ec);if(ec){error="world_restore_resource_stamp";return false;}const auto time=std::filesystem::last_write_time(path,ec);if(ec){error="world_restore_resource_stamp";return false;}p->resourceStamps.emplace(path,Playback::ResourceStamp{size,time});}}
        p->initial=s.continuation.initialFigures?*s.continuation.initialFigures:s.city.figures;p->combined=s.city;p->chained=s.continuation.chained;
        if(p->chained){p->chainInitial=s.city.figures;p->chainCombined=s.city;}
        const auto ids=OriginalKernelIdentities::restore(s.ledger,s.city.figures.figures,error);if(!ids)return false;const auto notices=AppCityNotifications::restore(s.notifications,*s.city.scenarioMessages);if(!notices){error="world_restore_notifications";return false;}
        p->identities=*ids;p->notifications=*notices;p->worldSource=s.source;p->worldPolicy=s.policy;p->worldRevision=s.commitRevision;p->process=s.policy.initial;p->marketBases=s.marketBases;
        const auto& c=s.continuation;p->worldController=c.controller;p->session=c.session;p->buildingDraws=c.buildingDraws;p->figureDrawCells=c.figureDrawCells;p->chainAdmission=c.admission;p->chainClocks=c.clocks;p->chainSelections=s.scene.selections;p->chainPhases=s.scene.phases;
        p->updates=c.updates;p->frames_=c.frames;p->draws_=c.draws;p->advances_=c.advances;p->reselections_=c.reselections;p->preyDeaths_=c.preyDeaths;p->preyCollections_=c.preyCollections;
        candidate.playback_=std::move(p);candidate.terrain_=s.terrain.map;candidate.state_=s.terrain.state;candidate.savedBuildings_=s.scene.shown;candidate.lastPlaybackCamera_=s.camera;
        auto camera=s.camera.value_or(OriginalMapCamera{});if(!s.camera){camera.width=256;camera.height=192;camera.centerX=camera.centerY=double(save.terrain.activeSide)/2;}
        const auto view=candidate.renderSavedBuildings(camera);if(!view.image.valid()||view.bases.decodeFailures||view.overlayDecodeFailures||view.overlayInvalid||view.groundUnresolved){error="world_restore_pixels";return false;}
        if(!verifyOriginalWorldBinding(root_,s.source,error))return false;
        candidate.lastPlaybackCamera_=s.camera;candidate.playback_->worldRevision=s.commitRevision;
        if(publicationProbe_)publicationProbe_("world_before_publish");swapPrepared(candidate);error_.clear();error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
std::optional<OriginalKernelHandle> AppMapSurface::cityFigureHandle(size_t slot)const{return playback_?playback_->identities.handle(slot):std::nullopt;}
bool AppMapSurface::cityFigureHandleCurrent(const OriginalKernelHandle& handle)const{return playback_&&playback_->identities.matches(handle);}
AppMapSurface::AppMapSurface(ResourceRoot root,std::shared_ptr<SG3SpriteCache> cache):root_(std::move(root)),spriteCache_(std::move(cache)){}
AppMapSurface::~AppMapSurface()=default;
void AppMapSurface::swapPrepared(AppMapSurface& other) noexcept {
    static_assert(std::is_nothrow_swappable<OriginalMapTerrain>::value,"terrain commit must not throw");
    static_assert(std::is_nothrow_swappable<decltype(savedBuildings_)>::value,"scene commit must not throw");
    using std::swap;swap(terrain_,other.terrain_);state_.swap(other.state_);savedBuildings_.swap(other.savedBuildings_);archives_.swap(other.archives_);playback_.swap(other.playback_);lastPlaybackCamera_.swap(other.lastPlaybackCamera_);
}
bool AppMapSurface::prepareCityPlayback(const OriginalSaveResult& save,const std::vector<uint8_t>& payload,
    const OriginalCombinedProcessInputs& process,const AppCitySessionState* session,bool replay){
    try {AppMapSurface candidate(root_,spriteCache_);candidate.publicationProbe_=publicationProbe_;
        if(!candidate.prepareCityPlaybackCandidate(save,payload,process,session,replay)){error_=candidate.error_;return false;}
        if(publicationProbe_)publicationProbe_("load_prepared");swapPrepared(candidate);error_.clear();return true;
    }catch(const std::exception&){return false;}
}
bool AppMapSurface::prepareCityPlaybackCandidate(const OriginalSaveResult& save,const std::vector<uint8_t>& payload,
    const OriginalCombinedProcessInputs& process,const AppCitySessionState* session,bool recordReplay){
    clear();
    auto playback=std::make_unique<Playback>();
    playback->recordReplay=recordReplay;
    std::string buildingText;
    if(!preparePostLoadInputs(save,payload,playback->archives,playback->refs,playback->initial,playback->employment,buildingText)){
        error_="SAV 城市时序输入不完整；播放未准备";return false;}
    if(!savedBuildings_||save.status!=OriginalSaveStatus::LoadedSnapshot){error_="SAV 快照场景不可用";return false;}
    const auto& figures=*save.layers.figures;const auto& ground=*save.layers.ground;
    if(session){
        playback->session=*session;
        auto& initial=playback->initial;
        initial.watchMaximum=session->watchMaximum;initial.inspectorMaximum=session->inspectorMaximum;
        initial.crtRandomCalls=session->crtRandomCalls;
        initial.goods->storageSearch=session->storageSearch;initial.goods->industrySearch=session->industrySearch;
    }
    auto combined=originalCombinedSavedState(playback->initial,save.buildings,payload,save.terrainOffset,save.sizeOffset,process);
    if(!combined){error_="combined saved building inputs";return false;}
    std::vector<uint8_t> speechBytes;
    if(!detail::readFile(root_.resolve("EmperorText.eng"),8u*1024u*1024u,speechBytes)){error_="combined speech groups";return false;}
    if(!originalCombinedBindBuildingServices(*combined,save.buildings,figures,payload,save.terrainOffset,save.sizeOffset,
        ground,*savedBuildings_,archives_.at(2).index,archives_.at(3).index,buildingText,originalBuildingSpeechCounts(speechBytes))){
        error_="combined saved city services";return false;}
    if(!combined->scenarioMessages){error_="app_scenario_messages_required";return false;}
    const auto notifications=AppCityNotifications::fromSaved(*combined->scenarioMessages);
    if(!notifications){error_="app_saved_notifications_invalid";return false;}
    playback->notifications=*notifications;
    playback->resourceRoot=root_.resolve("");playback->buildingModelText=buildingText;
    std::vector<uint8_t> figureModel;if(!detail::readFile(root_.resolve("Model/EmperorFigureModels.txt"),100000,figureModel))return false;
    playback->figureModelText=std::string(figureModel.begin(),figureModel.end());
    if(std::any_of(playback->initial.figures.begin(),playback->initial.figures.end(),[](const auto& row){return row.second.alive&&(row.second.type==24||row.second.type==25);})){
        emperor::file_io::research::GoalLimits limits;limits.enable_homage=limits.enable_trading=limits.enable_yearly=limits.enable_population=true;
        const auto goals=emperor::file_io::research::walk_city19_housing_goals({payload.data(),std::min(payload.size(),size_t(16384))},limits);
        if(goals.completed_members!=10||goals.pending.present){error_="kernel_market_base_goal_prefix";return false;}
        const auto prefix=evidenceFigurePrefix(payload,save.buildings.tableEnd,save.terrainOffset,goals.cache.size()+4000+save.buildings.declaredClasses);
        if(!prefix.complete||prefix.completed!=2000){error_="kernel_market_base_pool_prefix";return false;}
        for(const auto& row:prefix.records)if(row.field(22).value_or(0)&&(row.field(18)==24||row.field(18)==25)){
            OriginalMarketKnownBase base{save.snapshotRevision,row};error_=originalMarketKnownBaseIssue(base);if(!error_.empty())return false;
            playback->marketBases.emplace(row.slot,std::move(base));}
    }
    playback->process=process;playback->save=save;playback->figures=figures;playback->buildings=save.buildings;
    playback->seed=*savedBuildings_;playback->combined=std::move(*combined);
    playback->general=archives_.at(2).index;playback->terrainArchive=archives_.at(3).index;
    for(const auto& row:archives_){const auto& path=row.second.companion;auto indexPath=std::filesystem::path(path);indexPath.replace_extension(".sg3");
        for(const auto& name:{path,indexPath.string()}){std::error_code error;const auto bytes=std::filesystem::file_size(name,error);if(error){error_="kernel_resource_stamp_required";return false;}
            const auto time=std::filesystem::last_write_time(name,error);if(error){error_="kernel_resource_stamp_required";return false;}playback->resourceStamps.emplace(name,Playback::ResourceStamp{bytes,time});}}
    for(const auto& p:playback->seed.placed)if(p.components.size()==1&&p.binding.physicalId<=playback->general.sprites.size()&&
        playback->general.sprites[p.binding.physicalId-1].animationSpriteCount&&
        originalBuildingAnimationEnabled(p.footprint,&playback->employment)==true)playback->buildingDraws.push_back(p.footprint.slot);
    const unsigned origin=(228-save.terrain.activeSide)/2;
    for(unsigned diagonal=0;diagonal<2*save.terrain.activeSide-1;++diagonal)
        for(unsigned x=0;x<save.terrain.activeSide;++x)if(diagonal>=x&&diagonal-x<save.terrain.activeSide)
            playback->figureDrawCells.push_back((origin+diagonal-x)*228+origin+x);
    const auto identities=OriginalKernelIdentities::loaded(playback->initial.figures,error_);if(!identities)return false;playback->identities=*identities;
    playback_=std::move(playback);
    return true;
}
AppCityTimelineStep AppMapSurface::advanceCityPlayback(const OriginalSceneTimelineFrame& frame){
    try{return advanceCityPlaybackCandidate(frame);}catch(const std::exception&){AppCityTimelineStep out;out.updates=cityPlaybackUpdates();out.refusedUpdate=out.updates+1;out.refusal="kernel_publication_exception";return out;}
}
AppCityTimelineStep AppMapSurface::advanceCityPlaybackCandidate(const OriginalSceneTimelineFrame& supplied){
    AppCityTimelineStep out;
    if(!playback_){out.refusal="city_playback_not_prepared";return out;}
    auto& p=*playback_;
    if(p.worldRevision==UINT64_MAX){out.refusal="world_commit_revision_exhausted";out.updates=p.updates;return out;}
    auto frame=supplied;frame.buildingDraws=p.buildingDraws;frame.figureDrawCells=p.figureDrawCells;
    frame.combinedProcess.reset(); //the snapshot seed already carries the initial process state
    const bool chain=p.chained;
    // The batch path always replays from the saved snapshot scene, so a chained
    // step must present the same snapshot placement set (disaster ruins are
    // rebuilt by install from the carried combined state, never re-seeded here)
    // with the previous selection and carried draw phases applied to it.
    auto input=p.seed;
    if(chain){
        input.surface.terrain=p.chainInitial.terrain;
        for(auto& q:input.placed){
            const auto selected=p.chainSelections.find(q.footprint.slot);
            if(selected!=p.chainSelections.end()){q.footprint=selected->second.footprint;
                q.binding=selected->second.binding;q.drawX=selected->second.drawX;q.drawY=selected->second.drawY;
                q.components=selected->second.components;}
            const auto phase=p.chainPhases.find(q.footprint.slot);
            if(phase!=p.chainPhases.end()&&q.footprint.animationInputs)q.footprint.animationInputs->phase=phase->second;
        }
    }
    OriginalSceneTimeline result;
    if(chain)result=originalSceneTimeline(p.chainInitial,p.refs,input,p.general,&p.employment,
        p.chainAdmission,p.chainClocks,{frame},&p.chainCombined);
    else result=originalSceneTimeline(p.initial,p.refs,input,p.general,&p.employment,
        p.session.admissionClock,p.session.animationClock,{frame},&p.combined);
    if(!result.complete||!result.context||!result.combinedState){
        out.updates=p.updates;out.refusedUpdate=p.updates+1;
        out.refusal=result.refusal.empty()?"timeline_unknown_refusal":result.refusal;return out;
    }
    auto merged=std::move(result);
    if(chain){merged.updates+=p.updates;merged.frames+=p.frames_;merged.buildingDraws+=p.draws_;
        merged.buildingAdvances+=p.advances_;merged.buildingImageReselections+=p.reselections_;
        merged.preyDeaths+=p.preyDeaths_;merged.preyCollections+=p.preyCollections_;}
    // Capture only a complete candidate. Failed timeline/install leaves both
    // the rendered scene and the App process session at their committed state.
    auto session=p.session;
    const auto& f=*merged.context;const auto& process=merged.combinedState->process;
    if(!process.cityClocks||!process.randomHistory||!process.wildlifeCounter||!process.wildlifePeriod||
        !f.watchMaximum||!f.inspectorMaximum||!f.goods||!f.goods->storageSearch||!f.goods->industrySearch){
        out.updates=p.updates;out.refusedUpdate=p.updates+1;out.refusal="app_session_state_required";return out;
    }
    session.cityClocks=*process.cityClocks;session.randomHistory=*process.randomHistory;
    if(f.crtRandom)session.crtRandom=*f.crtRandom;
    session.crtRandomCalls=f.crtRandomCalls;session.wildlifeCounter=*process.wildlifeCounter;session.wildlifePeriod=*process.wildlifePeriod;
    session.watchMaximum=*f.watchMaximum;session.inspectorMaximum=*f.inspectorMaximum;
    session.storageSearch=*f.goods->storageSearch;session.industrySearch=*f.goods->industrySearch;
    session.admissionClock=merged.admissionClock;session.animationClock=merged.animationClock;
    auto notifications=p.notifications;
    if(!merged.combinedState->scenarioMessages||!notifications.observeCommitted(*merged.combinedState->scenarioMessages)){
        out.updates=p.updates;out.refusedUpdate=p.updates+1;out.refusal="app_generated_notifications_invalid";return out;
    }
    auto shown=input;
    if(!installOriginalSceneTimeline(shown,p.save.terrain,p.figures,p.refs,p.buildings,p.general,merged,&p.terrainArchive)){
        out.updates=p.updates;out.refusedUpdate=p.updates+1;out.refusal="timeline_install_refused";return out;
    }
    // Every allocation/copy/decode is completed before the first live write.
    auto nextInitial=*merged.context;auto nextCombined=*merged.combinedState;
    std::map<size_t,OriginalBuildingPlacement> nextSelections;
    for(const auto& q:shown.placed)nextSelections.emplace(q.footprint.slot,q);
    auto nextPhases=merged.buildingPhases;auto nextTerrain=merged.context->terrain;
    auto nextFrames=p.frames;if(p.recordReplay)nextFrames.push_back(std::move(frame));
    std::string identityError;const auto nextIdentities=p.identities.advanced(nextInitial.figures,nextInitial.allocationEvents,identityError);
    if(!nextIdentities){out.updates=p.updates;out.refusedUpdate=p.updates+1;out.refusal=identityError;return out;}
    auto identities=*nextIdentities;
    if(!p.resourcesCurrent()){out.updates=p.updates;out.refusedUpdate=p.updates+1;out.refusal="kernel_resource_binding_changed";return out;}
    if(publicationProbe_)publicationProbe_("before_pixels");
    OriginalMapCamera camera=lastPlaybackCamera_.value_or(OriginalMapCamera{});if(!lastPlaybackCamera_||camera.width<1||camera.height<1){camera.width=256;camera.height=192;camera.centerX=camera.centerY=double(p.save.terrain.activeSide)/2;}
    auto nextMap=terrain_;nextMap.flags=nextTerrain;
    std::vector<MapImageGridTileset> tiles;for(uint32_t slot:{3u,16u,14u,17u,38u,39u,40u})if(archives_.count(slot)){const auto& a=archives_.at(slot);tiles.push_back({slot,&a.index,a.companion,201});}
    const auto& a=archives_.at(2);if(spriteCache_)spriteCache_->beginFrame();
    const auto view=renderOriginalBuildingScene(nextMap,shown,a.index,a.companion,camera,-1,tiles,spriteCache_.get());
    if(!view.image.valid()||view.bases.decodeFailures||view.overlayDecodeFailures||view.overlayInvalid||view.groundUnresolved){out.updates=p.updates;out.refusedUpdate=p.updates+1;out.refusal="kernel_candidate_pixels_refused";return out;}
    if(!p.resourcesCurrent()){out.updates=p.updates;out.refusedUpdate=p.updates+1;out.refusal="kernel_resource_binding_changed";return out;}
    if(publicationProbe_)publicationProbe_("before_publish");
    static_assert(std::is_nothrow_copy_assignable<AppCitySessionState>::value,"session commit");
    static_assert(std::is_nothrow_move_assignable<OriginalFigureUpdateContext>::value,"figure commit");
    static_assert(std::is_nothrow_move_assignable<OriginalCombinedState>::value,"world commit");
    static_assert(std::is_nothrow_move_assignable<AppCityNotifications>::value,"notification commit");
    static_assert(std::is_nothrow_move_assignable<OriginalKernelIdentities>::value,"identity commit");
    static_assert(std::is_nothrow_move_assignable<OriginalBuildingScene>::value,"scene commit");
    p.chainInitial=std::move(nextInitial);p.chainCombined=std::move(nextCombined);p.chainSelections.swap(nextSelections);p.chainPhases.swap(nextPhases);p.frames.swap(nextFrames);
    p.chainAdmission=merged.admissionClock;p.chainClocks=merged.animationClock;p.chained=true;
    p.updates=merged.updates;p.frames_=merged.frames;p.draws_=merged.buildingDraws;p.advances_=merged.buildingAdvances;
    p.reselections_=merged.buildingImageReselections;p.preyDeaths_=merged.preyDeaths;p.preyCollections_=merged.preyCollections;
    p.session=session;p.notifications=std::move(notifications);p.identities=std::move(identities);
    terrain_.flags.swap(nextTerrain);*savedBuildings_=std::move(shown);
    ++p.worldRevision;out.complete=true;out.updates=p.updates;return out;
}
uint64_t AppMapSurface::installedSceneDigest()const{
    uint64_t h=1469598103934665603ull;
    const auto mix=[&](uint64_t v){for(int i=0;i<8;++i){h^=uint8_t(v>>(8*i));h*=1099511628211ull;}};
    // Placement animationInputs->phase is left out: in an installed timeline
    // scene it is a lagging copy that nothing draws; the drawn frame is the
    // mode-3 overlay built from the timeline's own phases, hashed below.
    for(const uint32_t f:terrain_.flags)mix(f);
    if(!savedBuildings_)return h;
    for(const auto& q:savedBuildings_->placed){
        mix(q.footprint.slot);mix(q.footprint.type);mix(q.binding.physicalId);mix(q.binding.variant);mix(q.drawX);mix(q.drawY);
        mix(q.components.size());for(const auto& c:q.components){mix(c.binding.physicalId);mix(c.x);mix(c.y);mix(c.side);}
    }
    mix(savedBuildings_->overlays.size());
    for(const auto& o:savedBuildings_->overlays){mix(o.physicalId);mix(o.anchorX);mix(o.anchorY);mix(uint32_t(o.offsetX));mix(uint32_t(o.offsetY));
        mix(o.role);mix(o.mode);mix(o.sourceSlot);}
    return h;
}
const AppCityNotifications* AppMapSurface::cityNotifications()const{return playback_?&playback_->notifications:nullptr;}
std::optional<AppCityTributeFinances> AppMapSurface::cityTributeFinances()const{
    if(!playback_)return std::nullopt;const auto& p=*playback_;
    const auto& city=p.chained?p.chainCombined:p.combined;
    if(!city.buildingContext)return std::nullopt;const auto& c=*city.buildingContext;
    if(!c.treasury||!c.accounting||!c.accounting->tributeIncomeComponent)return std::nullopt;
    return AppCityTributeFinances{*c.treasury,c.accounting->incomeOther,*c.accounting->tributeIncomeComponent};
}
AppCityTributeResult AppMapSurface::respondCityNotification(AppCityTributeId id,AppCityTributeDecision decision){
    if(!playback_||!playback_->chained||!playback_->chainCombined.buildingContext)return AppCityTributeResult::Stale;
    auto& p=*playback_;
    // The legacy frame-only replay journal cannot represent UI transactions.
    // Normal playback never enables it; refuse instead of silently losing an action.
    if(p.recordReplay)return AppCityTributeResult::ReplayRecording;
    if(p.worldRevision==UINT64_MAX)return AppCityTributeResult::Overflow;
    const auto result=p.notifications.respond(id,decision,*p.chainCombined.buildingContext);if(result==AppCityTributeResult::Applied)++p.worldRevision;return result;
}
unsigned AppMapSurface::cityPlaybackUpdates()const{return playback_?playback_->updates:0;}
std::optional<AppCitySessionState> AppMapSurface::cityPlaybackSession()const{
    return playback_?std::make_optional(playback_->session):std::nullopt;
}
// Preparation failures leave playback_ empty, so the reason must still be
// reported from error_ rather than hidden behind the prepared check.
const std::string& AppMapSurface::cityPlaybackError()const{return error_;}
void AppMapSurface::clearCityPlayback(){playback_.reset();}
// The admitted frames so far, with the initial process inputs on the first
// frame, so a caller can replay the same prefix through selectPostLoadTimeline.
std::vector<OriginalSceneTimelineFrame> AppMapSurface::cityPlaybackReplayFrames()const{
    if(!playback_)return {};
    auto frames=playback_->frames;
    if(!frames.empty())frames.front().combinedProcess=playback_->process;
    return frames;
}
OriginalBuildingSceneView AppMapSurface::renderSavedBuildings(const OriginalMapCamera& camera,int bit,SceneDrawCapture* capture,bool deferRaster)const{
    if(playback_){
        const auto& old=lastPlaybackCamera_;
        const bool changed=!old||old->centerX!=camera.centerX||old->centerY!=camera.centerY||old->width!=camera.width||old->height!=camera.height||old->scale!=camera.scale||old->outlines!=camera.outlines;
        if(changed){if(playback_->worldRevision==UINT64_MAX)return {};lastPlaybackCamera_=camera;++playback_->worldRevision;}
    }
    if(capture)*capture={};
    if(!savedBuildings_||!archives_.count(2))return {};
    const auto& archive=archives_.at(2);
    std::vector<MapImageGridTileset> groundTilesets;
    if(savedBuildings_->groundRebuilt)for(uint32_t slot:{3u,16u})if(archives_.count(slot)) {
        const auto& a=archives_.at(slot);groundTilesets.push_back({slot,&a.index,a.companion,201});
    }
    for(uint32_t slot:{14u,17u,38u,39u,40u})if(archives_.count(slot)){
        const auto& a=archives_.at(slot);groundTilesets.push_back({slot,&a.index,a.companion,201});
    }
    if(spriteCache_)spriteCache_->beginFrame();
    return renderOriginalBuildingScene(terrain_,*savedBuildings_,archive.index,archive.companion,camera,bit,groundTilesets,spriteCache_.get(),capture,deferRaster);
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
