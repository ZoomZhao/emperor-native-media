// SPDX-License-Identifier: MIT
#include "native_start_catalog.hpp"
#include "native_session_io.hpp"
#include "original_pak.hpp"
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {size_t checks=0;void need(bool b,const char*m){++checks;if(!b)throw std::runtime_error(m);}
void authored(){
 NativeSessionSource source;source.seed.source="Save/player/actual-name.sav";source.seed.sourceRevision=12;source.seed.modelRevision=34;
 auto e=nativeStartIdentity(source);need(nativeStartMatches(e,source),"unchanged fingerprint admitted");
 auto different=source;different.seed.sourceRevision++;need(!nativeStartMatches(e,different),"changed SAV refuses");different=source;different.seed.modelRevision++;need(!nativeStartMatches(e,different),"changed model refuses");different=source;different.seed.source="Save/other/same-name.sav";need(!nativeStartMatches(e,different),"path identity is not basename identity");e.reason="unsupported";need(!nativeStartMatches(e,source),"unsupported entry never admitted by matching hashes");e=nativeStartIdentity(source);e.sourceRevision=0;need(!nativeStartMatches(e,source),"unknown hash never defaults known");
 std::string error;need(!reloadNativeStartSource({},e,error)&&!error.empty(),"uninspected entry refuses before I/O");
 const std::vector<std::string> paths={"Save/b/a.sav","Save/a/a.sav","Save/b/a.sav","../escape.sav","/absolute.sav","Save/a/.sav"};auto c=nativeStartCatalog({},paths);
 need(c.entries.size()==5&&c.omitted==0,"deduplicate exact paths but retain unsupported entries");need(paths.size()==6,"catalog never mutates caller list");
 for(const auto&x:c.entries)need(!x.supported()&&!x.reason.empty()&&!x.metadata.funds&&x.metadata.side==0,"missing source has explicit refusal and unknown metadata");
 auto a=std::find_if(c.entries.begin(),c.entries.end(),[](const auto&x){return x.source=="Save/a/a.sav";});need(a!=c.entries.end()&&a->label=="a.sav · Save/a","actual filename and parent disambiguate without invented city name");
 std::vector<std::string> many;for(int n=0;n<130;++n)many.push_back("Save/p/"+std::to_string(n)+".sav");c=nativeStartCatalog({},many);need(c.entries.size()==128&&c.omitted==2,"bounded catalog explicitly reports unchecked remainder");
 auto generation=std::make_shared<NativeStartWork>();auto newer=std::make_shared<NativeStartWork>();
 need(nativeStartWorkCurrent(generation,generation),"same live generation may publish");need(!nativeStartWorkCurrent(generation,newer),"old dialog result cannot publish to reopened dialog");
 need(!nativeStartWorkCurrent(generation,{}),"closed owner rejects pending result");generation->cancelled=true;need(!nativeStartWorkCurrent(generation,generation),"cancelled result cannot publish");
 auto selected=std::make_shared<NativeStartWork>();auto nextSelection=std::make_shared<NativeStartWork>();need(!nativeStartWorkCurrent(selected,nextSelection),"old selection cannot overwrite new preview");
 size_t progressCalls=0;auto stopped=loadNativeStartCatalog({},paths,generation,[&](size_t,size_t){++progressCalls;});need(!stopped&&progressCalls==0,"cancel before start performs no progress or source work");
 generation=std::make_shared<NativeStartWork>();std::vector<size_t> progress;
 stopped=loadNativeStartCatalog({},paths,generation,[&](size_t done,size_t total){need(total==5,"deduplicated bounded progress denominator");progress.push_back(done);if(done==2)generation->cancelled=true;});
 need(!stopped&&progress==std::vector<size_t>({0,1,2}),"cancel between files stops work and returns no partial catalog");
 generation=std::make_shared<NativeStartWork>();stopped=loadNativeStartCatalog({},paths,generation,[&](size_t done,size_t total){if(done==total)generation->cancelled=true;});need(!stopped,"cancelling last progress still prevents publication");
 generation=std::make_shared<NativeStartWork>();progress.clear();auto complete=loadNativeStartCatalog({},paths,generation,[&](size_t done,size_t){progress.push_back(done);});need(complete&&complete->entries.size()==5&&progress==std::vector<size_t>({0,1,2,3,4,5}),"completed serial progress covers every checked row exactly once");
 std::cout<<"{\"passed\":true,\"authored_checks\":"<<checks<<",\"GUI\":false,\"original_data\":false}\n";
}
void real(int argc,char**argv){
 const char*env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}need(argc==3,"explicit external checkpoint folder");
 const auto root=ResourceRoot::fromAbsolute(env);auto original=enumerateOriginalMapSources(root);need(original.error.empty(),"existing original source enumeration");std::vector<std::string> paths;for(const auto&s:original.sources)if(s.kind==OriginalMapSourceKind::SaveFile)paths.push_back(s.relativePath);
 const auto c=nativeStartCatalog(root,paths);need(c.omitted==0&&c.entries.size()==paths.size(),"all supplied SAV entries included in bounded run");const auto old=nativeSessionChoices(root,paths);size_t accepted=0,previews=0,roundtrips=0;
 for(const auto&e:c.entries){
  if(!e.supported()){need(!e.reason.empty(),"unsupported SAV explains refusal");continue;}
  ++accepted;std::string error;auto source=reloadNativeStartSource(root,e,error);need(source&&error.empty(),"listed supported source revalidates");need(e.metadata.side==source->seed.side&&e.metadata.funds==source->seed.initialFunds,"displayed dimensions/funds come from admitted SAV");
  auto changed=e;changed.sourceRevision^=1;need(!reloadNativeStartSource(root,changed,error)&&error.find("改变")!=std::string::npos,"changed expected source refuses without altering input");changed=e;changed.modelRevision^=1;need(!reloadNativeStartSource(root,changed,error),"changed expected model refuses");
  auto state=startNativeSession(source->seed,5);const auto initial=encodeNativeSession(source->seed,state);const auto overview=makeNativeTerrainOverview(source->save.terrain,480);need(overview&&overview->side==e.metadata.side&&overview->activeCells>0&&overview->bgra.size()==480*480,"actual selected terrain produces bounded preview");++previews;need(encodeNativeSession(source->seed,state)==initial,"preview never changes city");
  bool found=false;unsigned x=0,y=0;for(unsigned py=0;py+2<source->seed.side&&!found;++py)for(unsigned px=1;px+2<source->seed.side&&!found;++px){bool okay=true;for(unsigned dy=0;dy<2;++dy)for(int dx=-1;dx<2;++dx)okay&=bool(source->seed.buildable[size_t(py+dy)*source->seed.side+px+dx]);if(okay){found=true;x=px;y=py;}}
  need(found,"listed source contains basic construction area");need(transactNativeSession(source->seed,state,{NativeSessionAction::Road,state.revision,int64_t(x)-1,y}).committed,"basic road accepted");need(transactNativeSession(source->seed,state,{NativeSessionAction::House,state.revision,x,y}).committed,"basic house accepted");need(state.funds==source->seed.initialFunds-source->seed.roadCost-source->seed.houseCost,"actual source/model costs only");
  const auto bytes=encodeNativeSession(source->seed,state);const auto path=std::filesystem::path(argv[2])/("catalog-"+std::to_string(accepted)+".encity");need(writeNativeSessionNew(path.string(),env,bytes,error),"unique independent file saved");const auto read=readNativeSessionFile(path.string(),env,error);need(read&&*read==bytes,"saved bytes read back");const auto restored=decodeNativeSession(source->seed,*read,error);need(restored&&encodeNativeSession(source->seed,*restored)==bytes,"restored state exact");need(!writeNativeSessionNew(path.string(),env,bytes,error),"existing checkpoint never overwritten");++roundtrips;
  std::cout<<"source "<<std::quoted(e.source)<<" side "<<e.metadata.side<<" funds "<<*e.metadata.funds<<" passed\n";
 }
 need(accepted==old.size(),"no expansion of original native admission");std::cout<<"{\"passed\":true,\"real_source_checks\":"<<checks<<",\"listed_SAV\":"<<c.entries.size()<<",\"supported\":"<<accepted<<",\"unsupported\":"<<c.entries.size()-accepted<<",\"previews\":"<<previews<<",\"build_save_reload\":"<<roundtrips<<",\"GUI\":false,\"original_behavior_claim\":false}\n";
}
}
int main(int argc,char**argv){try{if(argc==1)authored();else real(argc,argv);return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
