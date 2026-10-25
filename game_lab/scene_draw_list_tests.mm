// SPDX-License-Identifier: MIT
#include "native_session_source.hpp"
#include "app/map_surface.hpp"
#include "metal_scene_bridge.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool b,const std::string& why){++checks;if(!b)throw std::runtime_error(why);}
SG3DecodedSprite own(unsigned w,unsigned h,std::vector<uint32_t> pixels){auto p=std::make_shared<SG3DecodeResult>();p->status=SG3DecodeStatus::Decoded;p->image.width=w;p->image.height=h;p->image.pixelsBGRA=std::move(pixels);return identifySG3Sprite(p);}
void authored(){
 auto black=own(1,1,{0x01000000});SceneDrawListBuilder builder(1,1);builder.append(black,0,0,1);builder.append(black,0,0,1);builder.list.background={1,1,{0xff404040}};
 auto composed=rasterizeSceneDrawListCPU(builder.list);need(composed.valid()&&composed.pixelsBGRA[0]==0xff3f3f3f,"two-stage rounding oracle");need(sceneBlendOver(0x01000000,sceneBlendOver(0x01000000,0xff404040))==0xff404040,"flattening passes must differ");need(builder.list.sprites.size()==1&&builder.list.items.size()==2,"resource reuse does not merge ordered draws");
 auto red=own(2,2,{0xffff0000,0xff00ff00,0xff0000ff,0});auto blue=own(1,1,{0xff0000ff});
 for(unsigned scale=1;scale<=4;++scale){SceneDrawListBuilder b(7,7);b.list.background={7,7,std::vector<uint32_t>(49,0xff111111)};b.append(red,-int(scale),-int(scale),scale);b.append(blue,6,6,scale);auto image=rasterizeSceneDrawListCPU(b.list);need(image.valid(),"clipped scaled output");need(image.pixelsBGRA[0]==0xff111111&&image.pixelsBGRA[48]==0xff0000ff,"transparent corner and right/bottom clipping");}
 auto ordered=builder.list;ordered.sprites={red,blue};ordered.items={{0,0,0,2,2},{1,0,0,1,1}};need(rasterizeSceneDrawListCPU(ordered).pixelsBGRA[0]==0xff0000ff,"final vector order preserved");std::reverse(ordered.items.begin(),ordered.items.end());need(rasterizeSceneDrawListCPU(ordered).pixelsBGRA[0]==0xffff0000,"reversing order changes overlap");
 for(unsigned bad=0;bad<7;++bad){auto b=builder.list;if(bad==0)b.items[0].sprite=99;if(bad==1)b.items[0].width=0;if(bad==2)b.items[0].height=2;if(bad==3)b.sprites[0].identity=0;if(bad==4)b.sprites[0]=own(1,1,{0x010000ff});if(bad==5)b.background={};if(bad==6)b.items[0].left=INT32_MIN;need(!rasterizeSceneDrawListCPU(b).valid(),"invalid list fails closed");}
 char folder[]="/tmp/emperor-draw-owned-XXXXXX";need(mkdtemp(folder)!=nullptr,"owned temporary fixture");auto file=std::filesystem::path(folder)/"own.555";{std::ofstream f(file,std::ios::binary);f.put(0);f.put(0x7c);}
 SG3Archive archive;SG3Sprite sprite;sprite.width=sprite.height=1;sprite.type=1;sprite.length=2;archive.sprites={sprite};archive.numImages=1;SG3SpriteCache cache(4,1);cache.beginFrame();auto first=cache.decodeResource(archive,0,file.string());cache.beginFrame();auto hit=cache.decodeResource(archive,0,file.string());need(first.identity!=0&&hit.identity==first.identity&&hit.value==first.value,"warm stable resource identity");
 SceneDrawListBuilder held(1,1);held.list.background={1,1,{0xff000000}};held.append(first,0,0,1);cache.clear();auto second=cache.decodeResource(archive,0,file.string());need(second.identity!=first.identity,"redecode cannot recycle identity");std::filesystem::remove(file);need(rasterizeSceneDrawListCPU(held.list).pixelsBGRA[0]==0xffff0000,"held snapshot survives eviction and source removal");std::filesystem::remove(folder);
 std::cout<<"{\"authored_draw_checks\":"<<checks<<",\"passed\":true}\n";
}
size_t differences(const std::vector<uint32_t>&a,const std::vector<uint32_t>&b){need(a.size()==b.size(),"comparison dimensions");size_t n=0;for(size_t i=0;i<a.size();++i)n+=a[i]!=b[i];return n;}
void real(bool gpu){
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
 const auto root=ResourceRoot::fromAbsolute(env);std::string error;auto source=loadNativeSessionSource(root,"Save/2026102301/202610230101.sav",error);need(bool(source),error);
 std::unique_ptr<msp::MetalSpriteRenderer> renderer;if(gpu){renderer=std::make_unique<msp::MetalSpriteRenderer>();need(renderer->available(),"Metal device unavailable");std::cout<<"{\"metal_device\":\""<<renderer->report().name<<"\",\"offscreen_only\":true}\n";}
 auto native=startNativeSession(source->seed,5);std::vector<NativeSessionState> nativeStates{native};
 for(auto c:std::vector<NativeSessionCommand>{{NativeSessionAction::Road,0,81,56},{NativeSessionAction::Road,0,81,58},{NativeSessionAction::Road,0,85,57},{NativeSessionAction::House,0,82,56},{NativeSessionAction::Well,0,82,58},{NativeSessionAction::Hunter,0,84,58}}){c.expectedRevision=native.revision;need(transactNativeSession(source->seed,native,c).committed,"native construction");}nativeStates.push_back(native);
 size_t frames=0,gpuComparisons=0,overlayDraws=0,clipped=0,retained=0,totalPixels=0;
 const std::vector<std::string> saves={"Save/2026102301/320111.sav","Save/2026102401/240103.sav"};
 for(size_t scenario=0;scenario<4;++scenario){
  OriginalMapTerrain terrain;OriginalBuildingProfile buildings;OriginalSavedGround ground;std::optional<OriginalFigureProfile> figures;double x=83,y=58;
  if(scenario<2){auto scene=nativeSessionScene(*source,nativeStates[scenario]);need(bool(scene),"native projection");terrain=scene->terrain;buildings=scene->buildings;ground=scene->ground;}
  else{auto save=loadOriginalSave(root.resolve(saves[scenario-2]));need(save.status==OriginalSaveStatus::LoadedSnapshot&&bool(save.layers.ground),"real saved scene");terrain=save.terrain;buildings=save.buildings;ground=*save.layers.ground;figures=save.layers.figures;if(!buildings.footprints.empty()){x=buildings.footprints.front().x;y=buildings.footprints.front().y;}}
  auto cache=std::make_shared<SG3SpriteCache>();
  for(unsigned view=0;view<(scenario>=2?6u:4u);++view){
   OriginalMapCamera camera;camera.width=(view==3||view==5)?127:816;camera.height=(view==3||view==5)?95:620;camera.scale=view<4?view+1:view==4?2:4;camera.centerX=std::min(double(terrain.activeSide)-1,x+(view==3?.375:0));camera.centerY=std::min(double(terrain.activeSide)-1,y+(view==3?.625:0));
   if(view>=4){need(figures.has_value(),"figure focus source");auto selected=std::find_if(figures->live.begin(),figures->live.end(),[&](const auto& f){return f.image&&f.x>=0&&f.y>=0&&unsigned(f.x)<terrain.activeSide&&unsigned(f.y)<terrain.activeSide;});need(selected!=figures->live.end(),"saved figure focus");camera.centerX=selected->x;camera.centerY=selected->y;}
   SceneDrawCapture capture;OriginalBuildingSceneView reference;
   {AppMapSurface surface(root,cache);need(surface.selectSavedBuildings(terrain,buildings,&ground,0,figures?&*figures:nullptr),"prepare original scene");auto defaultCPU=surface.renderSavedBuildings(camera);reference=surface.renderSavedBuildings(camera,-1,&capture);need(defaultCPU.image.pixelsBGRA==reference.image.pixelsBGRA,"capture leaves default CPU path unchanged");need(capture.complete(),"draw capture: "+capture.error);need(reference.image.valid(),"CPU reference valid");
    SceneDrawCapture deferred;auto prepared=surface.renderSavedBuildings(camera,-1,&deferred,true);
    need(deferred.complete()&&prepared.pixelsDeferred&&!nativeSceneReady(prepared,prepared.placedRecords),"deferred resources cannot masquerade as a CPU city picture");
    need(prepared.bases.cpuSpriteDraws==0&&prepared.overlayCpuDraws==0,"deferred preparation executes zero CPU sprite draws");
    need(prepared.bases.decodedCells==reference.bases.decodedCells&&prepared.overlayDecoded==reference.overlayDecoded&&prepared.overlayInvalid==reference.overlayInvalid&&prepared.overlayDecodeFailures==reference.overlayDecodeFailures,"deferred validation retains every resource/object check");
    auto fallback=rasterizeSceneDrawListCPU(deferred.list);need(fallback.valid()&&fallback.pixelsBGRA==reference.image.pixelsBGRA,"deferred snapshot fallback is exactly the CPU reference");
    capture=std::move(deferred); // Remaining CPU/GPU checks use the list-only preparation.
    SceneDrawCapture warm;auto repeat=surface.renderSavedBuildings(camera,-1,&warm);need(warm.complete()&&warm.list.items.size()==capture.list.items.size(),"warm capture complete");need(warm.list.sprites.size()==capture.list.sprites.size(),"warm resource count");for(size_t n=0;n<warm.list.sprites.size();++n)need(warm.list.sprites[n].identity==capture.list.sprites[n].identity,"warm stable identity");need(repeat.image.pixelsBGRA==reference.image.pixelsBGRA,"capture does not alter CPU output");
    SceneDrawCapture refused;auto invalid=camera;invalid.width=0;need(!surface.renderSavedBuildings(invalid,-1,&refused).image.valid()&&!refused.complete()&&refused.list.items.empty(),"failed capture cannot publish previous list");}
   cache->clear();auto replay=rasterizeSceneDrawListCPU(capture.list);need(replay.valid()&&differences(reference.image.pixelsBGRA,replay.pixelsBGRA)==0,"original renderer/list CPU pixels differ");++retained;if(view>=4)need(reference.overlayDrawn>0,"focused original figure is actually drawn");overlayDraws+=reference.overlayDrawn;totalPixels+=size_t(camera.width)*camera.height;++frames;
   size_t clipping=0;for(const auto& item:capture.list.items)clipping+=item.left<0||item.top<0||int64_t(item.left)+item.width>camera.width||int64_t(item.top)+item.height>camera.height;clipped+=clipping;
   std::cout<<"{\"scenario\":"<<scenario<<",\"view\":"<<view<<",\"scale\":"<<camera.scale<<",\"pixels\":["<<camera.width<<","<<camera.height<<"],\"items\":"<<capture.list.items.size()<<",\"resources\":"<<capture.list.sprites.size()<<",\"overlay_drawn\":"<<reference.overlayDrawn<<",\"figure_placed\":"<<reference.figurePlaced<<",\"figure_live\":"<<reference.figureLive<<",\"clipped_items\":"<<clipping<<",\"cpu_diff_pixels\":0}\n";
   if(gpu)for(auto path:{msp::BlendPath::FixedFunction,msp::BlendPath::Programmable}){
    auto result=verifySceneDrawListMetal(capture.list,*renderer,path);need(result.complete,result.error);const auto diff=differences(reference.image.pixelsBGRA,result.composite.pixels);std::cout<<"{\"scenario\":"<<scenario<<",\"view\":"<<view<<",\"scale\":"<<camera.scale<<",\"path\":\""<<msp::blendPathName(path)<<"\",\"gpu_diff_pixels\":"<<diff<<",\"draws\":"<<result.layer.draws+result.composite.draws<<",\"upload_bytes\":"<<result.layer.uploadBytes+result.composite.uploadBytes<<",\"two_passes\":true}\n";need(diff==0,"original renderer/Metal pixels differ");++gpuComparisons;
   }
  }
 }
 need(overlayDraws>0&&clipped>0,"real overlay and clipping exercised");std::cout<<"{\"real_frames\":"<<frames<<",\"pixels_each_path_total\":"<<totalPixels<<",\"gpu_comparisons\":"<<gpuComparisons<<",\"overlay_draws\":"<<overlayDraws<<",\"retained_after_eviction\":"<<retained<<",\"passed\":true}\n";
}
}
int main(int argc,char**argv){@autoreleasepool{try{if(argc==1)authored();else real(std::string(argv[1])=="--metal");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}}
