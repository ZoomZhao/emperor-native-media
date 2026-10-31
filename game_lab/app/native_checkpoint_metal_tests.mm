// SPDX-License-Identifier: MIT
// Existing native ENCITY checkpoints + original SG3 sprites, real offscreen GPU.
// No original-game continuation, App/window or simulation update is claimed.
#import <AppKit/AppKit.h>
#include "map_surface.hpp"
#include "../native_session_source.hpp"
#include "../native_house_runtime_source.hpp"
#include "../native_session_io.hpp"
#include "../original_world_checkpoint_io.hpp"
#include "../metal_scene_renderer/metal_scene_renderer.hpp"
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace {
using namespace game_lab;
using namespace game_lab::metal_scene;
size_t checks = 0;
void need(bool value, const std::string& reason) {
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
std::string hex(const OriginalWorldDigest& digest) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (auto b : digest) { result += digits[b >> 4]; result += digits[b & 15]; }
    return result;
}
std::string imageHash(const DecodedImage& image) {
    need(image.valid(), "valid CPU/readback pixels");
    return hex(originalWorldDigest(image.pixelsBGRA.data(), image.pixelsBGRA.size() * 4));
}
struct Waiter {
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<Completion> results;
    bool onMain = false;
    CompletionHandler callback() {
        return [this](Completion result) {
            std::lock_guard<std::mutex> lock(mutex);
            onMain = onMain || bool(NSThread.isMainThread);
            results.push_back(std::move(result)); changed.notify_all();
        };
    }
    Completion wait() {
        std::unique_lock<std::mutex> lock(mutex);
        need(changed.wait_for(lock, std::chrono::seconds(20), [&]{ return !results.empty(); }), "bounded GPU completion");
        need(results.size() == 1 && !onMain, "one callback on backend queue");
        return results.front();
    }
};
Completion submit(Renderer& renderer, std::shared_ptr<const SceneDrawList> list,
                  SceneFrameTicket ticket, std::vector<std::shared_ptr<Waiter>>& retained) {
    // Callback storage outlives accepted work even if a bounded wait throws.
    auto waiter = std::make_shared<Waiter>();
    retained.push_back(waiter);
    auto result = renderer.submit(std::move(list), ticket, [waiter](Completion value) {
        auto callback = waiter->callback(); callback(std::move(value));
    });
    need(result.status == SubmitStatus::Accepted, "GPU submit: " + result.reason);
    auto completion = waiter->wait();
    need(completion.ticket == ticket, "unchanged completion identity");
    return completion;
}
void pixelsEqual(const Frame& frame, const DecodedImage& cpu) {
    DecodedImage gpu; std::string error;
    need(readFrameForTesting(frame, gpu, error), error);
    need(gpu.width == cpu.width && gpu.height == cpu.height, "GPU readback geometry");
    size_t different = 0;
    for (size_t i = 0; i < cpu.pixelsBGRA.size(); ++i)
        different += cpu.pixelsBGRA[i] != gpu.pixelsBGRA[i];
    need(different == 0, "GPU pixels differ from restored CPU oracle: " + std::to_string(different));
}
struct Loaded {
    NativeSessionSource source;
    NativeSessionState state;
    std::string canonical;
};
Loaded load(const ResourceRoot& root,const std::string& path) {
    std::string error;
    const auto bytes=readNativeSessionFile(path,root.configuredDirectory(),error);need(bool(bytes),error);
    const auto reference=nativeSessionReference(*bytes);need(bool(reference),"supported ENCITY reference");
    auto source=loadNativeSessionSource(root,reference->source,error);need(bool(source),error);
    if(reference->version==8)need(prepareNativeHouseRuntimeSource(root,*source,error),error);
    auto state=decodeNativeSession(source->seed,*bytes,error);need(bool(state),error);
    if(state->rulesVersion==8){need(bool(source->runtime),"v8 runtime resources");need(validateNativeHouseRuntimeReady(source->seed,*state,*source->runtime).empty(),"runtime ready");}
    need(state->paused,"selected historical checkpoint is paused; no simulation normalization");
    auto canonical=encodeNativeSession(source->seed,*state);need(!canonical.empty(),"canonical complete native state");
    return {std::move(*source),std::move(*state),std::move(canonical)};
}
struct Picture {
    DecodedImage cpu;
    std::shared_ptr<const SceneDrawList> list;
    size_t placedObjects=0,figureLive=0,figurePlaced=0,figurePixelDifferences=0,buildingResources=0;
};
Picture picture(const ResourceRoot& root,const Loaded& city) {
    const auto scene=nativeSessionScene(city.source,city.state);need(bool(scene),"existing native scene projection");
    need(scene->figures.has_value(),"actual native figure projection exists");
    auto chosen=std::find_if(scene->figures->live.begin(),scene->figures->live.end(),[&](const auto& f){return f.type==39&&f.image&&f.x>=0&&f.y>=0&&unsigned(f.x)<scene->terrain.activeSide&&unsigned(f.y)<scene->terrain.activeSide;});
    need(chosen!=scene->figures->live.end(),"existing visible patrol candidate, no actor construction");
    OriginalMapCamera camera;camera.width=816;camera.height=620;camera.centerX=chosen->x;camera.centerY=chosen->y;camera.outlines=false;
    auto cache=std::make_shared<SG3SpriteCache>();AppMapSurface surface(root,cache);
    need(surface.selectSavedBuildings(scene->terrain,scene->buildings,&scene->ground,0,&*scene->figures),surface.error());
    const size_t objects=std::count_if(city.state.objects.begin(),city.state.objects.end(),[](const auto& o){return o.kind!=NativeObjectKind::Road;});
    const auto cpu=surface.renderSavedBuildings(camera);
    need(nativeSceneReady(cpu,objects)&&cpu.figureLive>0&&cpu.figurePlaced>0&&!cpu.figureZeroImage&&!cpu.figureUnsupported,"all admitted buildings and figures represented");
    SceneDrawCapture capture;const auto deferred=surface.renderSavedBuildings(camera,-1,&capture,true);
    need(deferred.pixelsDeferred&&nativeSceneResourcesReady(deferred,objects)&&capture.complete(),"strict deferred capture: "+capture.error);
    need(!capture.list.items.empty()&&!capture.list.sprites.empty(),"nonempty original-resource draw list");
    need(deferred.bases.cpuSpriteDraws+deferred.overlayCpuDraws==0,"deferred capture did not CPU-rasterize sprites");
    need(imageHash(rasterizeSceneDrawListCPU(capture.list))==imageHash(cpu.image),"captured list exactly replays independent CPU compositor");
    size_t changedFromBackground=0;
    for(size_t i=0;i<cpu.image.pixelsBGRA.size();++i)changedFromBackground+=cpu.image.pixelsBGRA[i]!=capture.list.background.pixelsBGRA[i];
    need(changedFromBackground>0,"not a background-only GPU case");
    AppMapSurface noFigures(root,cache);need(noFigures.selectSavedBuildings(scene->terrain,scene->buildings,&scene->ground,0,nullptr),"same scene without figure layer");
    auto control=noFigures.renderSavedBuildings(camera);need(control.image.valid(),"figure visibility control");size_t figureDifference=0;
    for(size_t i=0;i<cpu.image.pixelsBGRA.size();++i)figureDifference+=cpu.image.pixelsBGRA[i]!=control.image.pixelsBGRA[i];
    need(figureDifference>0,"actual figure-layer pixels are visible, not merely decoded offscreen");
    // Match immutable cache handles for General sprites selected by the existing
    // building binder, rather than guessing a sprite from its colours or ID.
    auto general=loadSG3(root.resolve("DATA/China_General.sg3"));
    auto buildingScene=composeOriginalBuildingScene(scene->terrain,scene->buildings,general);
    need(buildingScene.accepted&&!buildingScene.placed.empty(),"existing building binder");
    size_t buildingResources=0;
    for(const auto& building:buildingScene.placed)for(const auto& component:building.components){
        if(component.binding.tileset!=2||!component.binding.physicalId||component.binding.physicalId>general.sprites.size())continue;
        auto resource=decodeSG3Resource(general,component.binding.physicalId-1,root.resolve("DATA/China_General.555"),cache.get());
        for(size_t i=0;i<capture.list.sprites.size();++i)if(capture.list.sprites[i].identity==resource.identity&&
            std::any_of(capture.list.items.begin(),capture.list.items.end(),[&](const auto& item){return item.sprite==i;})){++buildingResources;break;}
    }
    need(buildingResources>0,"bound building sprite resources actually present in clipped draw commands");
    return {cpu.image,std::make_shared<const SceneDrawList>(std::move(capture.list)),cpu.placedRecords,cpu.figureLive,cpu.figurePlaced,figureDifference,buildingResources};
}
std::shared_ptr<const Frame> ready(Renderer& renderer,const Picture& scene,SceneFrameTicket ticket,std::vector<std::shared_ptr<Waiter>>& waiters){
    auto result=submit(renderer,scene.list,ticket,waiters);need(result.status==CompletionStatus::Ready&&result.frame,"real nonempty Metal completion");
    const auto& d=result.frame->diagnostics();need(d.drawnItems>0&&d.resourcesDrawn>0&&d.drawCalls>1,"actual sprite GPU commands, not background-only");pixelsEqual(*result.frame,scene.cpu);return result.frame;
}
void prepare(const ResourceRoot& root,const std::filesystem::path& output,const std::vector<std::string>& inputs){
    need(output.is_absolute(),"absolute new output");const auto target=std::filesystem::weakly_canonical(output),data=std::filesystem::canonical(root.configuredDirectory());const auto relative=target.lexically_relative(data);
    need(target!=data&&!relative.empty()&&*relative.begin()==".."&&!std::filesystem::exists(output),"new output outside original data");
    need(inputs.size()==2&&!std::filesystem::equivalent(inputs[0],inputs[1]),"two distinct accepted checkpoint files");
    auto first=load(root,inputs[0]),second=load(root,inputs[1]);need(first.source.seed.source!=second.source.seed.source,"two original source identities, not repeated cities");
    std::filesystem::create_directories(output);std::vector<Loaded> cities;cities.push_back(std::move(first));cities.push_back(std::move(second));
    for(size_t i=0;i<cities.size();++i){auto& city=cities[i];auto scene=picture(root,city);Renderer renderer;need(renderer.waitForSelfTestForTesting(20)&&renderer.available(),"real Metal available");std::vector<std::shared_ptr<Waiter>> waiters;
        SceneFrameTicket ticket{{1,city.state.revision,1,0,0},1,scene.cpu.width,scene.cpu.height};auto frame=ready(renderer,scene,ticket,waiters);
        const auto file=output/("native"+std::to_string(i)+".encity");std::string error;need(writeNativeSessionNew(file.string(),root.configuredDirectory(),city.canonical,error),error);need(encodeNativeSession(city.source.seed,city.state)==city.canonical,"save/render do not advance state");
        renderer.shutdown();need(renderer.waitIdleForTesting(20),"prepare GPU drain");pixelsEqual(*frame,scene.cpu);
        std::cout<<"{\"case\":\""<<file.filename().string()<<"\",\"source_index\":"<<i<<",\"cpu_sha256\":\""<<imageHash(scene.cpu)<<"\",\"state_sha256\":\""<<hex(originalWorldDigest(city.canonical.data(),city.canonical.size()))<<"\",\"version\":"<<city.state.rulesVersion<<",\"ticks\":"<<city.state.ticks<<",\"pre_save_GPU\":true,\"scene_items\":"<<scene.list->items.size()<<",\"building_resources\":"<<scene.buildingResources<<",\"figure_pixel_differences\":"<<scene.figurePixelDifferences<<"}\n";
    }
}
void verify(const ResourceRoot& root,const std::string& file,const std::string& cpuHash,const std::string& stateHash){
    auto city=std::make_unique<Loaded>(load(root,file));need(hex(originalWorldDigest(city->canonical.data(),city->canonical.size()))==stateHash,"fresh-process all-state bytes equal");
    auto scene=picture(root,*city);need(imageHash(scene.cpu)==cpuHash,"before-save CPU equals after-restore CPU");auto renderer=std::make_unique<Renderer>();need(renderer->waitForSelfTestForTesting(20)&&renderer->available(),"real Metal available");const auto caps=renderer->capabilities();need(caps.registryId!=0,"actual device identity");std::vector<std::shared_ptr<Waiter>> waiters;
    SceneFrameTicket ticket{{1,city->state.revision,1,0,0},1,scene.cpu.width,scene.cpu.height};auto prior=ready(*renderer,scene,ticket,waiters);
    std::string error;need(!decodeNativeSession(city->source.seed,city->canonical+"trailing corrupt body",error)&&!error.empty(),"corrupt candidate refused by strict codec");
    need(encodeNativeSession(city->source.seed,city->state)==city->canonical,"decoder failure leaves previous complete state untouched");pixelsEqual(*prior,scene.cpu);
    renderer->injectFaultForTesting(TestFault::FailAtCompletion);++ticket.requestSerial;auto failure=submit(*renderer,scene.list,ticket,waiters);need(failure.status==CompletionStatus::Failed&&!failure.frame,"GPU failure cannot masquerade as a new frame");need(encodeNativeSession(city->source.seed,city->state)==city->canonical,"GPU failure leaves all native state unchanged");pixelsEqual(*prior,scene.cpu);
    renderer->invalidateResources();++ticket.requestSerial;++ticket.key.cacheGeneration;auto next=ready(*renderer,scene,ticket,waiters);
    const auto items=scene.list->items.size(),drawn=prior->diagnostics().drawnItems,resources=prior->diagnostics().resourcesDrawn,calls=prior->diagnostics().drawCalls;
    const auto objects=scene.placedObjects,figures=scene.figurePlaced,figPixels=scene.figurePixelDifferences,buildingResources=scene.buildingResources;
    city.reset();scene.list.reset();renderer->shutdown();need(renderer->waitIdleForTesting(20),"final GPU drain");renderer.reset();for(auto& waiter:waiters){std::lock_guard<std::mutex> lock(waiter->mutex);need(waiter->results.size()==1&&!waiter->onMain,"one completion after destruction");}pixelsEqual(*prior,scene.cpu);pixelsEqual(*next,scene.cpu);need(NSApp==nil,"no App/window");
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"GPU_executed\":true,\"GUI\":false,\"restored_ready_frames\":2,\"failed_frames\":1,\"scene_items\":"<<items<<",\"drawn_items\":"<<drawn<<",\"resources_drawn\":"<<resources<<",\"draw_calls\":"<<calls<<",\"placed_objects\":"<<objects<<",\"placed_figures\":"<<figures<<",\"visible_figure_pixels\":"<<figPixels<<",\"building_resources\":"<<buildingResources<<",\"different_pixels\":0,\"registry_id\":"<<caps.registryId<<"}\n";
}
}
int main(int argc,char**argv){@autoreleasepool{try{
 const char* configured=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!configured||!*configured){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}need(argc==5,"--prepare output checkpoint0 checkpoint1 OR --verify saved cpuHash stateHash");need(NSApp==nil,"no App");auto root=ResourceRoot::fromAbsolute(configured);if(std::string(argv[1])=="--prepare")prepare(root,argv[2],{argv[3],argv[4]});else if(std::string(argv[1])=="--verify")verify(root,argv[2],argv[3],argv[4]);else throw std::runtime_error("unknown mode");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}}
