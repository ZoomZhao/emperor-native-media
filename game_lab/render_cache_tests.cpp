// SPDX-License-Identifier: MIT
#include "native_session_source.hpp"
#include "native_session_io.hpp"
#include "app/map_surface.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <mach/mach.h>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
void put(const std::filesystem::path&p,std::initializer_list<uint8_t>b){std::ofstream f(p,std::ios::binary);for(auto x:b)f.put(char(x));need(bool(f),"write authored fixture");}
bool same(const SG3DecodeResult&a,const SG3DecodeResult&b){return a.status==b.status&&a.mirrored==b.mirrored&&a.alphaApplied==b.alphaApplied&&a.image.width==b.image.width&&a.image.height==b.image.height&&a.image.pixelsBGRA==b.image.pixelsBGRA;}
uint64_t rss(){mach_task_basic_info_data_t i{};mach_msg_type_number_t n=MACH_TASK_BASIC_INFO_COUNT;return task_info(mach_task_self(),MACH_TASK_BASIC_INFO,reinterpret_cast<task_info_t>(&i),&n)==KERN_SUCCESS?i.resident_size:0;}
void authored(){
 char path[]="/tmp/emperor-cache-owned-XXXXXX";need(mkdtemp(path),"unique owned fixture");const auto root=std::filesystem::path(path),file=root/"own.555";put(file,{0,0x7c,0xe0,3,0x1f,0});
 SG3Sprite s;s.width=2;s.height=1;s.type=1;s.length=4;SG3Archive a;a.sprites={s};a.numImages=1;
 SG3SpriteCache cache(16,2);cache.beginFrame();auto first=cache.decode(a,0,file.string());need(same(*first,decodeSG3Sprite(a,0,file.string())),"cold exact pixels");cache.beginFrame();auto hit=cache.decode(a,0,file.string());need(first==hit&&cache.statistics().hits==1,"cross-frame shared immutable hit");
 a.sprites[0].offset=2;cache.beginFrame();auto shifted=cache.decode(a,0,file.string());need(same(*shifted,decodeSG3Sprite(a,0,file.string()))&&shifted->image.pixelsBGRA!=first->image.pixelsBGRA,"changed index parameters cannot alias");
 a.sprites.push_back(a.sprites[0]);a.sprites.back().mirrorOffset=-1;a.numImages=2;cache.beginFrame();auto mirror=cache.decode(a,1,file.string());need(same(*mirror,decodeSG3Sprite(a,1,file.string()))&&mirror->mirrored,"mirror parity key");need(cache.statistics().bytes<=16&&cache.statistics().entries<=2&&cache.statistics().evictions>0,"pixel and entry ceilings evict");need(first->image.pixelsBGRA[0]==0xffff0000,"eviction does not invalidate frame-held image");
 const auto originalTime=std::filesystem::last_write_time(file);put(root/"replacement.555",{0x1f,0,0,0x7c,0xe0,3});std::filesystem::last_write_time(root/"replacement.555",originalTime);std::filesystem::rename(root/"replacement.555",file);cache.beginFrame();auto replacement=cache.decode(a,0,file.string());need(same(*replacement,decodeSG3Sprite(a,0,file.string()))&&replacement->image.pixelsBGRA!=shifted->image.pixelsBGRA,"same size and mtime replacement invalidates by inode/ctime");need(cache.statistics().invalidations>0,"version invalidation counted");
 std::filesystem::rename(file,root/"held.555");cache.beginFrame();need(!cache.decode(a,0,file.string())->image.valid(),"missing warm asset never serves old pixels");std::filesystem::rename(root/"held.555",file);cache.beginFrame();need(cache.decode(a,0,file.string())->image.valid(),"restoration retries successfully");
 std::filesystem::create_directory(root/"other");put(root/"other/own.555",{0,0x7c,0,0x7c,0,0x7c});cache.beginFrame();auto other=cache.decode(a,0,(root/"other/own.555").string());need(same(*other,decodeSG3Sprite(a,0,(root/"other/own.555").string()))&&other->image.pixelsBGRA!=replacement->image.pixelsBGRA,"same name other root cannot alias");
 // Actual external-name resolution and adjacent premultiplied alpha remain unchanged.
 a.sprites[0]={};auto& p=a.sprites[0];p.width=p.height=2;p.type=276;p.length=9;p.alphaLength=5;p.alphaOffset=18;p.external=1;p.offset=1;a.bitmaps={{"external.bmp"}};
 put(root/"external.555",{4,0,0x7c,0xe0,3,0x1f,0,0xff,0x7f,4,0,15,31,31});SG3SpriteCache large;large.beginFrame();auto alpha=large.decode(a,0,file.string());need(alpha->alphaApplied&&same(*alpha,decodeSG3Sprite(a,0,file.string())),"external alpha pixels exact");large.beginFrame();need(large.decode(a,0,file.string())==alpha,"external alpha cross-frame hit");
 put(root/"external.555",{255,255});large.beginFrame();need(!large.decode(a,0,file.string())->image.valid(),"changed/truncated external asset refuses despite warm cache");need(!large.decode(a,0,file.string())->image.valid(),"failure is not turned into success");
 SG3SpriteCache tiny(1,1);tiny.beginFrame();a.sprites[0]=s;need(tiny.decode(a,0,file.string())->image.valid()&&tiny.statistics().bytes==0,"oversize decodes uncached without changing acceptance");
 SG3SpriteCache capped(1024,1);for(unsigned n=0;n<30;++n){a.sprites[0].offset=n%2?0:2;capped.beginFrame();need(capped.decode(a,0,file.string())->image.valid(),"bounded churn decode");need(capped.statistics().entries<=1&&capped.statistics().bytes<=1024,"metadata and payload remain capped");}
 // Multiple sprites in a frame share a confined path, not a metadata/address key.
 SG3SpriteCache paths;SG3Archive pa;SG3Sprite ps;ps.width=ps.height=1;ps.type=1;ps.length=2;pa.sprites={ps,ps};pa.sprites[1].offset=2;pa.numImages=2;
 put(root/"paths.555",{0,0x7c,0xe0,3});paths.beginFrame();auto p0=paths.decode(pa,0,(root/"paths.555").string());auto p1=paths.decode(pa,1,(root/"paths.555").string());
 need(p0->image.valid()&&p1->image.valid()&&p0->image.pixelsBGRA!=p1->image.pixelsBGRA,"path memo never aliases sprite offsets");
 need(paths.statistics().pathResolutions==1&&paths.statistics().pathHits==1,"one path resolution per dependency in a frame");
 paths.beginFrame();need(paths.decode(pa,0,(root/"paths.555").string())==p0&&paths.statistics().pathResolutions==2,"next frame re-resolves then pixel cache hits");
 std::filesystem::create_directory(root/"555");std::filesystem::rename(root/"paths.555",root/"555/paths.555");put(root/"555/paths.555",{0x1f,0,0xe0,3});paths.beginFrame();auto moved=paths.decode(pa,0,(root/"paths.555").string());
 need(same(*moved,decodeSG3Sprite(pa,0,(root/"paths.555").string()))&&moved->image.pixelsBGRA!=p0->image.pixelsBGRA,"next frame detects companion search-location change");
 std::filesystem::create_directory(root/"second");put(root/"second/paths.555",{0,0x7c,0xe0,3});auto otherPath=paths.decode(pa,0,(root/"second/paths.555").string());need(otherPath->image.pixelsBGRA==p0->image.pixelsBGRA&&otherPath!=p0,"same leaf across roots never shares resolved path");
 pa.sprites[0].external=1;pa.sprites[0].bitmapId=0;pa.bitmaps.resize(1);pa.bitmaps[0].filename="paths.bmp";paths.beginFrame();auto external=paths.decode(pa,0,(root/"own.555").string());need(same(*external,decodeSG3Sprite(pa,0,(root/"own.555").string())),"external bitmap resolution remains identical");
 pa.bitmaps[0].filename="own.bmp";auto changed=paths.decode(pa,0,(root/"own.555").string());need(same(*changed,decodeSG3Sprite(pa,0,(root/"own.555").string())),"changed external bitmap metadata resolves its own name");
 pa.bitmaps[0].filename="../paths.bmp";need(!paths.decode(pa,0,(root/"own.555").string())->image.valid(),"invalid external name cannot reuse previous valid path");
 SG3CompanionMemo cappedPaths;cappedPaths.limit=1;pa.sprites[0].external=0;resolveSG3Sprite(pa,0,(root/"555/paths.555").string(),&cappedPaths);resolveSG3Sprite(pa,0,(root/"second/paths.555").string(),&cappedPaths);need(cappedPaths.paths.size()==1&&cappedPaths.resolutions==2,"path metadata budget remains bounded");
 std::filesystem::remove(root/"555/paths.555");paths.beginFrame();need(!paths.decode(pa,0,(root/"paths.555").string())->image.valid(),"removed companion refuses despite previous path and pixel hits");
 std::filesystem::create_symlink("/etc/hosts",root/"paths.555");paths.beginFrame();need(!paths.decode(pa,0,(root/"paths.555").string())->image.valid(),"outside symlink refuses without reading target");
 OriginalBuildingSceneView view;view.image.width=view.image.height=1;view.image.pixelsBGRA={0xff000000};view.placedRecords=1;need(nativeSceneReady(view,1),"complete native picture admitted");
 for(unsigned failure=0;failure<5;++failure){auto bad=view;if(failure==0)bad.bases.decodeFailures=1;if(failure==1)bad.overlayDecodeFailures=1;if(failure==2)bad.overlayInvalid=1;if(failure==3)bad.groundUnresolved=1;if(failure==4)bad.withheldRecords=1;need(!nativeSceneReady(bad,1),"either-layer failure rejects publication");}
 need(!nativeSceneReady(view,2),"expected object count retains guard");view.image={};need(!nativeSceneReady(view,1),"invalid canvas rejects publication");

 // Exercise the actual object renderer, not just injected summary counters.
 OriginalMapTerrain terrain;terrain.activeSide=2;terrain.flags.resize(OriginalTerrainState::cells);
 OriginalBuildingScene scene;scene.surface.image.resize(OriginalTerrainState::cells);
 scene.overlays.push_back({"DATA/China_General.sg3",1,2,0,0,0,0,{0,0,1,0},0,1,0});
 SG3Archive overlay;SG3Sprite os;os.width=os.height=1;os.type=256;os.length=3;overlay.sprites={os};overlay.numImages=1;
 put(root/"overlay.555",{1,0,0x7c});
 const std::vector<MapImageGridTileset> tiles={{2,&overlay,(root/"overlay.555").string(),1}};
 OriginalMapCamera camera;camera.width=camera.height=80;camera.centerX=camera.centerY=0;
 SG3SpriteCache overlays;
 auto draw=[&](){OriginalBuildingSceneView result;result.image.width=result.image.height=80;result.image.pixelsBGRA.resize(6400);overlays.beginFrame();need(renderOriginalSceneOverlays(terrain,scene,tiles,camera,result,result.image,&overlays),"object pass completes");return result;};
 auto good=draw();need(good.overlayDecoded==1&&good.overlayDecodeFailures==0&&nativeSceneReady(good,0),"real object pass admitted");
 auto warm=draw();need(warm.image.pixelsBGRA==good.image.pixelsBGRA&&overlays.statistics().hits==1,"object warm pixels exact");
 put(root/"overlay.555",{255});auto broken=draw();need(broken.overlayDecodeFailures==1&&!nativeSceneReady(broken,0),"actual warm object decode failure rejects publication");
 put(root/"overlay.555",{1,0,0x7c});auto recovered=draw();need(nativeSceneReady(recovered,0)&&recovered.image.pixelsBGRA==good.image.pixelsBGRA,"restored object renderer recovers");
 scene.overlays[0].mode=0;auto invalid=draw();need(invalid.overlayInvalid==1&&!nativeSceneReady(invalid,0),"actual invalid object command rejects publication");
 std::cout<<"{\"authored_cache_checks\":"<<checks<<",\"passed\":true}\n";
}
struct Timing {double project=0,prepare=0,raster=0;};
void real(){const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
 const auto root=ResourceRoot::fromAbsolute(env);std::string error;auto source=loadNativeSessionSource(root,"Save/2026102301/202610230101.sav",error);need(bool(source),error.c_str());const auto& seed=source->seed;
 std::vector<NativeSessionState> states;states.push_back(startNativeSession(seed,5));auto city=states.front();
 auto command=[&](NativeSessionAction action,int x,int y){auto result=transactNativeSession(seed,city,{action,city.revision,x,y});need(result.committed,"real source construction admitted");};
 for(const auto& c:std::vector<std::pair<int,int>>{{81,56},{81,58},{85,57}})command(NativeSessionAction::Road,c.first,c.second);
 command(NativeSessionAction::House,82,56);command(NativeSessionAction::Well,82,58);command(NativeSessionAction::Hunter,84,58);states.push_back(city);
 // An occupied stress sample uses only validated native commands, no edited SAV.
 for(unsigned y=51;y<66&&y+2<seed.side&&city.objects.size()<60;++y)for(unsigned x=75;x<94&&x+2<seed.side&&city.objects.size()<60;++x){auto next=city;auto a=transactNativeSession(seed,next,{NativeSessionAction::Road,next.revision,int(x)-1,int(y)});if(!a.committed)continue;auto b=transactNativeSession(seed,next,{NativeSessionAction::House,next.revision,x,y});if(b.committed)city=std::move(next);}
 need(city.objects.size()>=30,"occupied stress sample available");states.push_back(city);using Clock=std::chrono::steady_clock;auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
 size_t comparisons=0,maxOverlayObjects=0;
 for(size_t scenario=0;scenario<states.size();++scenario)for(unsigned scale:{1u,2u}){
  auto cache=std::make_shared<SG3SpriteCache>();std::array<Timing,2> total{},firstTiming{};uint64_t startRSS=rss();SG3CacheStats cold{};
  for(unsigned n=0;n<20;++n){std::vector<uint32_t> reference;
   for(unsigned mode=0;mode<2;++mode){auto t0=Clock::now();auto scene=nativeSessionScene(*source,states[scenario]);need(bool(scene),"state projection");auto t1=Clock::now();AppMapSurface surface(root,mode?cache:nullptr);need(surface.selectSavedBuildings(scene->terrain,scene->buildings,&scene->ground),"source resource/scene preparation");auto t2=Clock::now();OriginalMapCamera camera;camera.width=816;camera.height=620;camera.centerX=83+(n%3);camera.centerY=58;camera.scale=scale;auto view=surface.renderSavedBuildings(camera);auto t3=Clock::now();const auto objects=std::count_if(states[scenario].objects.begin(),states[scenario].objects.end(),[](const auto&o){return o.kind!=NativeObjectKind::Road;});maxOverlayObjects=std::max(maxOverlayObjects,view.overlayObjects);need(nativeSceneReady(view,objects),"both renders admitted");
    total[mode].project+=ms(t0,t1);total[mode].prepare+=ms(t1,t2);total[mode].raster+=ms(t2,t3);if(!n)firstTiming[mode]={ms(t0,t1),ms(t1,t2),ms(t2,t3)};
    if(!mode)reference=view.image.pixelsBGRA;else {need(reference==view.image.pixelsBGRA,"every cached pixel equals uncached renderer");++comparisons;if(!n)cold=cache->statistics();}
   }
  }
  const auto st=cache->statistics();need(st.hits>0&&st.bytes<=cache->budget()&&st.entries<=2048,"real cross-frame hits and budget");need(st.decodes==cold.decodes,"unchanged resources never redecoded after cold frame");
  for(unsigned mode=0;mode<2;++mode)std::cout<<"{\"scenario\":"<<scenario<<",\"objects\":"<<states[scenario].objects.size()<<",\"scale\":"<<scale<<",\"pixels\":[816,620],\"mode\":\""<<(mode?"cached":"uncached")<<"\",\"frames\":20,\"project_mean_ms\":"<<total[mode].project/20<<",\"prepare_mean_ms\":"<<total[mode].prepare/20<<",\"raster_mean_ms\":"<<total[mode].raster/20<<",\"cold_raster_ms\":"<<firstTiming[mode].raster<<",\"subsequent_raster_mean_ms\":"<<(total[mode].raster-firstTiming[mode].raster)/19<<"}\n";
  std::cout<<"{\"scenario\":"<<scenario<<",\"scale\":"<<scale<<",\"requests\":"<<st.requests<<",\"hits\":"<<st.hits<<",\"cold_decodes\":"<<cold.decodes<<",\"warm_decodes\":"<<st.decodes-cold.decodes<<",\"cache_bytes\":"<<st.bytes<<",\"peak_bytes\":"<<st.peakBytes<<",\"entries\":"<<st.entries<<",\"resolve_ms\":"<<st.resolveNs/1e6<<",\"validate_ms\":"<<st.validateNs/1e6<<",\"decode_ms\":"<<st.decodeNs/1e6<<",\"rss_before\":"<<startRSS<<",\"rss_after\":"<<rss()<<"}\n";
 }
 // Save/reload does not change pixels or encoding, and shares only immutable assets.
 auto cache=std::make_shared<SG3SpriteCache>();const auto bytes=encodeNativeSession(seed,states[1]);auto restored=decodeNativeSession(seed,bytes,error);need(bool(restored)&&encodeNativeSession(seed,*restored)==bytes,"save/read state byte exact");std::vector<uint32_t> previous;
 for(const auto* s:{&states[1],&*restored}){auto scene=nativeSessionScene(*source,*s);AppMapSurface surface(root,cache);need(surface.selectSavedBuildings(scene->terrain,scene->buildings,&scene->ground),"reloaded scene preparation");OriginalMapCamera c;c.width=816;c.height=620;c.centerX=83;c.centerY=58;auto view=surface.renderSavedBuildings(c);if(previous.empty())previous=view.image.pixelsBGRA;else need(previous==view.image.pixelsBGRA,"reload same pixels");}
 std::cout<<"{\"real_pixel_comparisons\":"<<comparisons<<",\"maximum_overlay_objects\":"<<maxOverlayObjects<<",\"pixels_each\":505920,\"save_reload\":true,\"passed\":true}\n";
}
}
int main(int argc,char**){try{if(argc==1)authored();else real();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
