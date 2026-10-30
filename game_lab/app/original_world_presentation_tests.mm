// SPDX-License-Identifier: MIT
// Actual App methods/controls and CPU images; no visible windows or event loop.
// AppKit may lazily initialize NSApplication when creating a menu.
#define main emperorAppEntryNotExecuted
#include "main.mm"
#undef main
#include <iostream>
#include <fstream>
#include <stdexcept>
namespace {
size_t checks=0;void need(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
std::string encoded(const game_lab::OriginalWorldSnapshot& s){std::string e;auto b=game_lab::encodeOriginalWorldCheckpoint(s,e);need(bool(b),e.c_str());return *b;}
std::string pixels(NSImage* image){CGImageRef cg=[image CGImageForProposedRect:nullptr context:nil hints:nil];need(cg!=nullptr,"concrete CG image");CFDataRef data=CGDataProviderCopyData(CGImageGetDataProvider(cg));need(data!=nullptr,"concrete image pixels");std::string bytes(reinterpret_cast<const char*>(CFDataGetBytePtr(data)),size_t(CFDataGetLength(data)));CFRelease(data);return bytes;}
struct Shown {NSImage* image=nil;NSString* label=nil;NSString* info=nil;NSString* detail=nil;NSString* picker=nil;NSString* filter=nil;std::string source,world,session;NSInteger plane=0;std::optional<uint64_t> receipt;};
}
@interface WorldPresentationHarness:EmperorAppDelegate {
@public
 NSURL* choice;
 bool failImage,throwImage;
}
- (instancetype)initRoot:(game_lab::ResourceRoot)root sources:(const std::vector<std::string>&)sources metal:(BOOL)metal;
- (Shown)shown;
- (void)assertShown:(const Shown&)old;
- (void)assertCurrent:(const game_lab::OriginalWorldSnapshot&)saved;
- (void)markSaved;
- (void)forceStoredPlane;
- (void)checkNextSessionOwner;
- (void)checkExperimentalSessionOwner;
@end
@implementation WorldPresentationHarness
- (instancetype)initRoot:(game_lab::ResourceRoot)root sources:(const std::vector<std::string>&)sources metal:(BOOL)metal {
 if(!(self=[super init]))return nil;_resourceRoot=root;_originalMapSurface=std::make_unique<game_lab::AppMapSurface>(root);_cityPlaybackEpochMs=appMonotonicMs();_citySession.crtRandom=999;_nativeMetalEnabled=metal;
 _originalMapPicker=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(0,0,300,30) pullsDown:NO];_originalMapSources.push_back({game_lab::OriginalMapSourceKind::MapFile,"Cities/ASSR-Baoji.map","Cities/ASSR-Baoji.map",0});
 for(const auto& source:sources)_originalMapSources.push_back({game_lab::OriginalMapSourceKind::SaveFile,source,source,0});
 _originalMapVisibleSources.push_back(0);[_originalMapPicker addItemWithTitle:@"Cities/ASSR-Baoji.map"];
 _originalMapFilter=[NSTextField new];_originalMapFilter.stringValue=@"old filtered sources";
 _originalMapBitPicker=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(0,0,300,30) pullsDown:NO];for(unsigned i=0;i<39;++i)[_originalMapBitPicker addItemWithTitle:[NSString stringWithFormat:@"plane%u",i]];[_originalMapBitPicker selectItemAtIndex:38];
 _originalMapInfo=[NSTextField labelWithString:@"old MAP 6384 statistics"];_originalMapUnknowns=[NSTextView new];_originalMapUnknowns.string=@"old MAP detail";_originalMapImage=[NSImageView new];_originalMapImage.image=imageFromPixels(1,1,{0xffff00ff});_originalMapImage.accessibilityLabel=@"Cities/ASSR-Baoji.map old surface";
 _originalMapSpatial=[NSButton new];_originalMapOutlines=[NSButton new];_originalMapCameraX=[NSTextField new];_originalMapCameraY=[NSTextField new];_originalMapCameraX.stringValue=@"old invalid x";_originalMapCameraY.stringValue=@"old invalid y";
 _originalFigureSource=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];[_originalFigureSource addItemsWithTitles:@[@"Saved snapshot",@"post-load k=1"]];[_originalFigureSource selectItemAtIndex:1];
 _originalBuildingSlot=[NSTextField new];_originalSaveCellX=[NSTextField new];_originalSaveCellY=[NSTextField new];_cityPlaybackStatus=[NSTextField labelWithString:@""];
 _originalSourceRelative="Cities/ASSR-Baoji.map";_originalMapPackageSummary=YES;_originalMapSourceInfo="old MAP source metadata";_originalMapResult.frames=123;_originalMapResult.decodedBytes=456;return self;
}
- (NSURL*)chooseWorldCheckpointURL {return choice;}
- (NSImage*)originalWorldCPUImage:(const game_lab::DecodedImage&)image {if(throwImage)throw std::bad_alloc();return failImage?nil:[super originalWorldCPUImage:image];}
- (Shown)shown {Shown s;s.image=_originalMapImage.image;s.label=[_originalMapImage.accessibilityLabel copy];s.info=[_originalMapInfo.stringValue copy];s.detail=[_originalMapUnknowns.string copy];s.picker=[_originalMapPicker.titleOfSelectedItem copy];s.filter=[_originalMapFilter.stringValue copy];s.source=_originalSourceRelative;s.plane=_originalMapBitPicker.indexOfSelectedItem;s.receipt=_worldReceipt.savedRevision;game_lab::original_world_codec_detail::Writer w;w.value(_citySession);s.session=w.bytes;if(_cityPlayback){std::string error;auto value=_cityPlayback->worldSnapshot(error);need(bool(value),error.c_str());s.world=encoded(*value);}return s;}
- (void)assertShown:(const Shown&)old {const auto now=[self shown];need(now.image==old.image&&[now.label isEqualToString:old.label]&&[now.info isEqualToString:old.info]&&[now.detail isEqualToString:old.detail],"failed/cancelled load retains exact old image andsource/statistics strings");need(now.source==old.source&&[now.picker isEqualToString:old.picker]&&[now.filter isEqualToString:old.filter]&&now.plane==old.plane,"failed load retains picker/filter/plane/source");need(now.world==old.world&&now.session==old.session&&now.receipt==old.receipt,"failed load retains entire world,external session andsave receipt");}
- (void)markSaved {_worldReceipt.savedRevision=_originalMapSurface->worldRevision();}
- (void)assertCurrent:(const game_lab::OriginalWorldSnapshot&)saved {
 need(_cityPlayback&&_cityPlayback->prepared()&&_cityPlayback->status().state==game_lab::AppCityPlaybackState::Paused,"new player installedpaused");need(_originalSourceRelative==saved.source.source.relative&&_originalMapBitPicker.indexOfSelectedItem==0,"restore selects supported full plane andcorrect source");
 need(!_originalMapPackageSummary&&_originalMapResult.status==game_lab::OriginalMapStatus::LoadedTerrain&&_originalMapResult.terrain.flags==saved.terrain.map.flags,"restored map status/current terrain replace oldMAP metadata");
 NSString* source=[NSString stringWithUTF8String:saved.source.source.relative.c_str()];need([_originalMapPicker.titleOfSelectedItem containsString:source]&&[_originalMapImage.accessibilityLabel containsString:source]&&[_originalMapInfo.stringValue containsString:source]&&[_originalMapUnknowns.string containsString:source],"picker/pixelsAX/info/detail allname same restored source");
 need(![_originalMapInfo.stringValue containsString:@"old MAP"]&&![_originalMapImage.accessibilityLabel containsString:@"ASSR-Baoji"],"old MAP identity/statistics gone aftersuccessful load");need(_originalMapFilter.stringValue.length==0&&_originalMapSpatial.state==NSControlStateValueOn&&![_originalMapBitPicker itemAtIndex:38].enabled&&!_originalFigureSource.enabled,"filter andincompatible source-only controls synchronized");
 auto camera=saved.camera.value_or(game_lab::originalBuildingFocusCamera(_originalSaveBuildings->footprints,game_lab::OriginalMapCamera{}));need(_originalMapCameraX.doubleValue==camera.centerX&&_originalMapCameraY.doubleValue==camera.centerY,"camera fields use saved/default source camera");
 game_lab::AppMapSurface expected(_resourceRoot);game_lab::AppCitySessionState session;std::string error;auto p=game_lab::AppCityPlayback::fromWorldSnapshot(expected,saved,0,&session,error);need(bool(p),error.c_str());const auto view=p->render(camera,-1);need(view.image.valid(),"expected actual current rendered pixels");
 const auto visible=pixels(_originalMapImage.image);const std::string wanted(reinterpret_cast<const char*>(view.image.pixelsBGRA.data()),view.image.pixelsBGRA.size()*4);need(visible==wanted,"installed actual CPU image bytes equal restoredworld render, not previous MAP");need([_originalMapInfo.stringValue containsString:[NSString stringWithFormat:@"已放置建筑 %zu/%zu",view.placedRecords,view.liveRecords]],"visible statistics equal installed current view");
 auto actual=_cityPlayback->worldSnapshot(error),target=p->worldSnapshot(error);need(bool(actual)&&bool(target)&&encoded(*actual)==encoded(*target),"complete world plusfirstview matches independent restored renderer");
}
- (void)forceStoredPlane {[_originalMapBitPicker selectItemAtIndex:38];_originalMapSpatial.state=NSControlStateValueOff;[self onOriginalMapPlaneChanged:nil];need(_originalMapBitPicker.indexOfSelectedItem==0&&_originalMapImage.image!=nil&&[_originalMapImage.accessibilityLabel containsString:[NSString stringWithUTF8String:_originalSourceRelative.c_str()]],"N0 redraw cannot route through old bit37 orsaved source plane");}
- (void)checkExperimentalSessionOwner {
 const auto previous=_cityPlayback->status().updates;need(_cityPlayback->status().experiment&&_cityPlayback->step([self cityPlaybackNowMs]),"onlylabelled explicitpremise step tests restoredsession owner");
 const auto session=_originalMapSurface->cityPlaybackSession();need(bool(session),"committed candidate session");game_lab::original_world_codec_detail::Writer a,b;a.value(_citySession);b.value(*session);need(a.bytes==b.bytes&&session->admissionClock!=0,"newcontroller writes surviving Appsession ratherthan expired preparationlocal");[self renderCityPlaybackFrame];need([_originalMapImage.accessibilityLabel containsString:[NSString stringWithFormat:@"N=%u",previous+1]],"labelled nextframe uses same matching presentation path");
}
- (void)checkNextSessionOwner {auto before=_citySession;need(!_cityPlayback->step([self cityPlaybackNowMs])&&_cityPlayback->status().refusal=="event_ui_5eff20_not_closed","strict next input remainsrefused");need(_citySession.crtRandom==before.crtRandom,"controller sessionowner remainslive anddoesnot reset/replay onnext input");}
@end
int main(int argc,char** argv){@autoreleasepool {try{
 const char* configured=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!configured||!*configured){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
 need(argc==4,"new output andtwoexplicit sources required");need(NSApp==nil,"no NSApplication/window launched");auto root=game_lab::ResourceRoot::fromAbsolute(configured);const auto out=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(out);std::vector<std::string> sources{argv[2],argv[3]};std::vector<game_lab::OriginalWorldSnapshot> states;std::vector<NSURL*> paths;
 std::string error;for(size_t i=0;i<2;++i){auto source=game_lab::bindOriginalWorldSource(root,sources[i],error);need(bool(source),error.c_str());game_lab::AppCityPlaybackPreferences prefs;need(game_lab::readAppCityPlaybackPreferences(root,prefs,error),error.c_str());game_lab::AppMapSurface surface(root);game_lab::AppCitySessionState session;session.crtRandom=uint32_t(17+i);game_lab::AppCityPlayback player(surface,source->save,source->payload,prefs,session.crtRandom,0,false,&session,false,&*source);need(player.prepared(),"source checkpoint preparation");auto s=player.worldSnapshot(error);need(bool(s),error.c_str());states.push_back(*s);const auto file=out/("source-"+std::to_string(i)+".emworld");auto result=game_lab::writeOriginalWorldCheckpointNew(file.string(),configured,*s);need(result.state!=game_lab::OriginalWorldPublication::NotPublished,result.error.c_str());paths.push_back([NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]]);}
 const auto invalid=out/"invalid.emworld";{std::ofstream f(invalid);f<<"authored invalid checkpoint";}NSURL* bad=[NSURL fileURLWithPath:[NSString stringWithUTF8String:invalid.c_str()]];
 for(bool metal:{false,true})for(size_t i=0;i<2;++i){WorldPresentationHarness* h=[[WorldPresentationHarness alloc]initRoot:root sources:sources metal:metal];const auto old=[h shown];h->choice=nil;[h onWorldOpen:nil];[h assertShown:old];h->choice=bad;[h onWorldOpen:nil];[h assertShown:old];h->failImage=true;h->choice=paths[i];[h onWorldOpen:nil];[h assertShown:old];h->failImage=false;
 h->choice=paths[i];[h onWorldOpen:nil];[h assertCurrent:states[i]];[h markSaved];const auto installed=[h shown];h->choice=nil;[h onWorldOpen:nil];[h assertShown:installed];h->choice=bad;[h onWorldOpen:nil];[h assertShown:installed];h->failImage=true;h->choice=paths[1-i];[h onWorldOpen:nil];[h assertShown:installed];h->failImage=false;h->throwImage=true;[h onWorldOpen:nil];[h assertShown:installed];h->throwImage=false;
 [h forceStoredPlane];[h assertCurrent:states[i]];h->choice=paths[1-i];[h onWorldOpen:nil];[h assertCurrent:states[1-i]];[h checkNextSessionOwner];h=nil;
 }
 // One labelled existing-premise case detects a dangling external session
 // owner after the private candidate returns; it never grants ordinary play.
 auto source=game_lab::bindOriginalWorldSource(root,sources[0],error);need(bool(source),error.c_str());game_lab::AppCityPlaybackPreferences prefs;need(game_lab::readAppCityPlaybackPreferences(root,prefs,error),error.c_str());game_lab::AppMapSurface surface(root);game_lab::AppCitySessionState session;session.crtRandom=17;game_lab::AppCityPlayback exp(surface,source->save,source->payload,prefs,17,0,true,&session,false,&*source);need(exp.step(100),"prepare known experimental N1");auto value=exp.worldSnapshot(error);need(bool(value),error.c_str());const auto file=out/"labelled-session-owner.emworld";need(game_lab::writeOriginalWorldCheckpointNew(file.string(),configured,*value).state!=game_lab::OriginalWorldPublication::NotPublished,"ownlabelled checkpoint");
 WorldPresentationHarness* h=[[WorldPresentationHarness alloc]initRoot:root sources:sources metal:NO];h->choice=[NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]];[h onWorldOpen:nil];[h assertCurrent:*value];[h checkExperimentalSessionOwner];h=nil;
 size_t visible=0;for(NSWindow* window in NSApp.windows)if(window.visible)++visible;need(visible==0,"no visible App windows in method tests");std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"AppKit_window_count\":"<<NSApp.windows.count<<",\"visible_windows\":"<<visible<<",\"event_loop_run\":false,\"NSApplication_created\":"<<(NSApp?"true":"false")<<",\"sources\":2,\"configurations\":2,\"actual_presenter\":\"CPU NSImageView\",\"GPU_executed\":false,\"GUI\":false}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
