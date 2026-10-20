#include "original_save.hpp"
#include "original_pak.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <zlib.h>
namespace {
using Bytes=std::vector<uint8_t>;using namespace game_lab;size_t checks=0;
void need(bool b,const char* text){++checks;if(!b)throw std::runtime_error(text);}
void put(Bytes& b,size_t p,uint32_t value){for(unsigned k=0;k<4;++k)b.at(p+k)=uint8_t(value>>(8*k));}
void add(Bytes& b,uint32_t value){for(unsigned k=0;k<4;++k)b.push_back(uint8_t(value>>(8*k)));}
Bytes frame(const Bytes& body){Bytes out;add(out,0xfedcbaaa);for(size_t at=0;at<body.size();at+=32768){const size_t count=std::min(size_t(32768),body.size()-at);Bytes compressed(compressBound(count));uLongf n=compressed.size();if(compress2(compressed.data(),&n,body.data()+at,count,1)!=Z_OK)throw std::runtime_error("authored zlib");add(out,0xabcdefff);add(out,uint32_t(n));add(out,uint32_t(count));out.insert(out.end(),compressed.begin(),compressed.begin()+n);}return out;}
Bytes payload(size_t base=3003){Bytes body(base+2226670+17);body[0]=0x13;put(body,127,112);std::memcpy(body.data()+143,"Brief descriptio",16);
 for(size_t i=0;i<51984;++i){put(body,base+4*i,0x80000);put(body,base+207936+4*i,uint32_t(i)|0x80000000u);put(body,base+2018734+4*i,uint32_t(i)|0x40000000u);body[base+415872+i]=uint8_t(i);body[base+1510985+i]=uint8_t(255-i);body[base+1822996+i]=uint8_t(i%101);}
 put(body,base,0x80005);put(body,base+4,0x80005);put(body,base+8,0x80005);
 for(auto xy:{std::pair<uint32_t,uint32_t>{58,58},{169,58},{58,169},{169,169}})put(body,base+4*(xy.second*228+xy.first),0);
 put(body,base+4*(114*228+114),0x80000040u);return body;
}
void write(const std::filesystem::path& p,const Bytes& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)throw std::runtime_error("authored fixture write");}
}
int main(){try{
 constexpr size_t base=3003;auto body=payload();auto bytes=frame(body);auto saved=decodeOriginalSave(bytes);
 need(saved.status==OriginalSaveStatus::LoadedSnapshot&&saved.terrain.valid()&&saved.layers.valid(),"SAV13 complete snapshot fields");
 need(saved.terrainOffset==base&&saved.sizeOffset==127&&saved.frames>1&&saved.decodedBytes==body.size(),"SAV dynamic unaligned markers and complete framing");
 need(saved.terrain.activeSide==112&&saved.terrain.flags[114*228+114]==0x80000040u,"terrain signed high bits preserved");
 bool exact=true;for(size_t i=0;i<51984;++i)exact&=saved.layers.followingU32Unknown[i]==(uint32_t(i)|0x80000000u)&&saved.layers.mapperObjects[i]==(uint32_t(i)|0x40000000u)&&saved.layers.mapperRandom[i]==uint8_t(i)&&saved.layers.mapperStone[i]==uint8_t(255-i)&&saved.layers.mapperFertile[i]==uint8_t(i%101);
 need(exact,"every authored raw-layer value and both backing endpoints preserved");
 for(unsigned f=0;f<5;++f){auto values=originalSaveFieldValues(saved.layers,f);need(values.size()==51984&&values.back()==(f==0?(51983u|0x80000000u):f==1?(51983u|0x40000000u):f==2?uint8_t(51983):f==3?uint8_t(51983%101):uint8_t(255-51983)),"all raw field unsigned widenings exact");auto image=renderOriginalMapValues(saved.terrain,values,-1,true);need(image.valid()&&image.width==112&&image.height==112,"raw fields share terrain crop without using their own flag bits");}
 auto special=originalSaveFieldValues(saved.layers,1);special[114*228+114]=0x80000;auto raw=renderOriginalMapValues(saved.terrain,special,-1,true);need(raw.valid()&&raw.pixelsBGRA[56*112+56]!=0,"field bit19 is not mistaken for terrain off-map mask");
 special.pop_back();need(!renderOriginalMapValues(saved.terrain,special).valid(),"incomplete raw value raster rejected");need(originalSaveFieldValues(saved.layers,5).empty(),"unknown field selector rejected");
 auto cell=originalSaveCell(saved.terrain,saved.layers,56,56);
 need(cell&&cell->storageX==114&&cell->storageY==114&&cell->onMap&&cell->values[0]==0x80000040u&&cell->values[1]==saved.layers.followingU32Unknown[114*228+114]&&cell->values[2]==saved.layers.mapperObjects[114*228+114]&&cell->values[3]==saved.layers.mapperRandom[114*228+114]&&cell->values[4]==saved.layers.mapperFertile[114*228+114]&&cell->values[5]==saved.layers.mapperStone[114*228+114],"cell inspector reads six unsigned fields at same backing index");
 need(describeOriginalSaveCell(*cell).find("2147483712 / 0x80000040")!=std::string::npos,"cell description preserves unsigned decimal and hexadecimal");
 auto off=originalSaveCell(saved.terrain,saved.layers,1,1);need(off&&!off->onMap&&off->values[1]!=0,"off-map cell stays distinguishable and inspectable");
 for(auto xy:{std::pair<int64_t,int64_t>{-1,0},{0,-1},{112,0},{0,112},{INT64_MAX,0},{0,INT64_MIN}})need(!originalSaveCell(saved.terrain,saved.layers,xy.first,xy.second),"active coordinates reject all boundaries without wrapping");
 need(originalSaveCell(saved.terrain,saved.layers,0,0)&&originalSaveCell(saved.terrain,saved.layers,111,111),"both crop endpoints accepted");
 auto shortLayers=saved.layers;shortLayers.mapperStone.pop_back();need(!originalSaveCell(saved.terrain,shortLayers,0,0),"incomplete layer set never yields partial cell");
 auto invalidTerrain=saved.terrain;invalidTerrain.flags[0]=0;need(!originalSaveCell(invalidTerrain,saved.layers,0,0),"unverified crop never yields cell coordinates");
  auto moved=decodeOriginalSave(frame(payload(50003)));need(moved.status==OriginalSaveStatus::LoadedSnapshot&&moved.terrainOffset==50003&&moved.terrain.flags==saved.terrain.flags&&moved.layers.mapperObjects==saved.layers.mapperObjects,"marker offsets change without borrowing fixed MAP coordinates");
  // Literal SAV19 extents from serializer0x52FDA0/read0x5311DC. Distinct bytes
  // prevent aliases with the historical following-u32 or neighbouring arrays.
  constexpr size_t groundBase=200003;auto groundBody=payload(groundBase);
  put(groundBody,131,112);put(groundBody,135,58*229);
  groundBody[2]=6;groundBody[45]=2;groundBody[95]=3;groundBody[97]=4;
  for(size_t member=0;member<10;++member){groundBody[3355+6*member]=1;}
  groundBody[3415]=1; // channel singleton v1,+4=0,+8=0
  for(size_t i=0;i<51984;++i){groundBody[groundBase-155952+i]=uint8_t(i+17);groundBody[groundBase+311904+i]=uint8_t(i+23);groundBody[groundBase+467856+i]=uint8_t(i+31);groundBody[groundBase+1874984+i]=uint8_t(i+47);groundBody[groundBase-103968+2*i]=uint8_t(i);groundBody[groundBase-103968+2*i+1]=uint8_t(i>>8);}
  put(groundBody,groundBase-4,0); // required terrain marker's preceding owner bytes
  auto groundSave=decodeOriginalSave(frame(groundBody));need(groundSave.layers.ground&&groundSave.layers.ground->valid(),"complete SAV19 prior spans and independently serialized crop publish ground inputs");
  const auto& ground=*groundSave.layers.ground;exact=true;
  for(size_t i=0;i<51984;++i)exact&=ground.state.image[i]==0&&ground.state.draw[i]==uint8_t(i+17)&&ground.state.subTile[i]==uint8_t(i+23)&&ground.material[i]==uint8_t(i+31)&&ground.owner[i]==uint16_t(i<51982?i:0)&&ground.state.gBEBF3A[i]==uint8_t(i+47)&&ground.state.feature[i]==(uint32_t(i)|0x40000000u)&&ground.state.fertility[i]==uint8_t(i%101)&&ground.state.roadDistance[i]==uint8_t(255-i)&&ground.state.randomByte[i]==uint8_t(i);
  need(exact,"every SAV19 ground value reads the literal serializer span; image remains cleared");
  put(groundBody,135,0);need(!decodeOriginalSave(frame(groundBody)).layers.ground,"inconsistent serialized crop refuses rebuild inputs without inventing a crop");
  need(!saved.layers.ground,"small historical raw fixture cannot publish nonexistent preceding draw/owner grids");
  put(groundBody,135,58*229);groundBody[3421]=1;
  auto activeWater=decodeOriginalSave(frame(groundBody));need(activeWater.layers.ground&&activeWater.layers.ground->waterMode==1&&!activeWater.layers.ground->valid(),"saved active water mode is retained and unsupported rebuild refuses rather than using MAP zero premise");
  groundBody[3421]=0;groundBody[3415]=2;auto unknownWater=decodeOriginalSave(frame(groundBody));
  need(unknownWater.layers.ground&&!unknownWater.layers.ground->waterMode&&!unknownWater.layers.ground->valid(),"unknown singleton version withholds rebuild instead of assuming inactive");
  groundBody[3415]=1;groundBody[3355]=2;unknownWater=decodeOriginalSave(frame(groundBody));
  need(unknownWater.layers.ground&&!unknownWater.layers.ground->waterMode,"unknown intervening goal-list version cannot shift the singleton cursor");
  groundBody[3355]=1;put(groundBody,groundBase+1291324,2);auto otherView=decodeOriginalSave(frame(groundBody));
  need(otherView.layers.ground&&otherView.layers.ground->savedView==2&&!otherView.layers.ground->valid(),"saved nonzero view refuses view0 scene instead of borrowing incompatible corners");put(groundBody,groundBase+1291324,0);
  struct GoalSpan{const char* name;uint16_t version;size_t bytes;};
  for(const auto span:{GoalSpan{"cHousingGoal",1,12},{"cYearlyProductionGoal",1,12},{"cTradingPartnersGoal",1,4},{"cPopulationGoal",1,4},{"cTreasuryGoal",1,4},{"cHomageGoal",3,8},{"cAlliedCitiesGoal",3,8},{"cYearlyProfitGoal",1,8},{"cConquerGoal",2,8},{"cMonumentGoal",2,8},{"cMenagerieGoal",3,8}}){
   auto fixture=groundBody;std::fill(fixture.begin()+3355,fixture.begin()+5000,0);size_t at=3355;
   const auto u16=[&](uint16_t v){fixture[at++]=uint8_t(v);fixture[at++]=uint8_t(v>>8);};
   const auto u32=[&](uint32_t v){put(fixture,at,v);at+=4;};
   u16(1);u32(1);u16(65535);u16(0);u16(uint16_t(std::strlen(span.name)));
   std::copy_n(span.name,std::strlen(span.name),fixture.begin()+at);at+=std::strlen(span.name);
   u16(1);u32(0);u16(span.version);at+=span.bytes;
   for(unsigned member=1;member<10;++member){u16(1);u32(0);}u16(1);u32(0);fixture[at++]=0;
   auto closed=decodeOriginalSave(frame(fixture));need(closed.layers.ground&&closed.layers.ground->valid(),"each independently sized intervening goal payload reaches saved inactive singleton");
   fixture[at-1]=1;closed=decodeOriginalSave(frame(fixture));need(closed.layers.ground&&closed.layers.ground->waterMode==1&&!closed.layers.ground->valid(),"each intervening payload preserves active singleton refusal");
  }
 auto duplicate=body;put(duplicate,700,112);std::memcpy(duplicate.data()+716,"Brief descriptio",16);need(decodeOriginalSave(frame(duplicate)).status==OriginalSaveStatus::UnsupportedAnchors,"duplicate plausible size anchor refused");
 auto terrainDuplicate=body;std::memcpy(terrainDuplicate.data()+base+300000,body.data()+base-4,16);auto one=decodeOriginalSave(frame(terrainDuplicate));need(one.status==OriginalSaveStatus::LoadedSnapshot&&one.terrainOffset==base&&one.terrain.flags==saved.terrain.flags,"later marker without full known spans is not a second anchor");
 auto preceding=body;std::memcpy(preceding.data()+1000,body.data()+base-4,16);one=decodeOriginalSave(frame(preceding));need(one.status==OriginalSaveStatus::LoadedSnapshot&&one.terrainOffset==base&&one.terrain.flags==saved.terrain.flags,"earlier complete-span decoy fails crop; never choose first marker");
 auto ambiguous=body;const size_t secondBase=ambiguous.size()+4;ambiguous.resize(secondBase+2226670);std::copy_n(body.begin()+base,2226670,ambiguous.begin()+secondBase);put(ambiguous,secondBase+4*(114*228+114),0x80000080u);auto multiple=decodeOriginalSave(frame(ambiguous));need(multiple.status==OriginalSaveStatus::UnsupportedAnchors&&multiple.terrain.flags.empty()&&!multiple.layers.valid()&&multiple.decodedBytes==0,"two complete compatible candidates refuse without ranking or partial publication");
 ambiguous.pop_back();one=decodeOriginalSave(frame(ambiguous));need(one.status==OriginalSaveStatus::LoadedSnapshot&&one.terrainOffset==base,"truncated second candidate cannot override complete first candidate");
 auto missing=body;missing[143]='X';need(decodeOriginalSave(frame(missing)).status==OriginalSaveStatus::UnsupportedAnchors,"missing size marker not guessed");missing=body;put(missing,base,0x80000);need(decodeOriginalSave(frame(missing)).status==OriginalSaveStatus::UnsupportedAnchors,"missing terrain marker not guessed");
 for(uint32_t side:{0u,229u}){auto x=body;put(x,127,side);need(decodeOriginalSave(frame(x)).status==OriginalSaveStatus::UnsupportedAnchors,"invalid size anchor rejected");}
 auto badCrop=body;put(badCrop,127,140);need(decodeOriginalSave(frame(badCrop)).status==OriginalSaveStatus::InvalidGrid,"declared crop must match actual min/max flags");
 auto outside=body;put(outside,base+4*(227*228+227),0);need(decodeOriginalSave(frame(outside)).status==OriginalSaveStatus::InvalidGrid,"on-map flags outside declared crop rejected");
 auto shortBody=body;shortBody.resize(base+2226670-1);auto shortResult=decodeOriginalSave(frame(shortBody));need(shortResult.status==OriginalSaveStatus::InvalidGrid&&shortResult.terrain.flags.empty()&&!shortResult.layers.valid(),"last object-grid byte truncation never publishes earlier fields");
 auto badRole=body;badRole[0]=5;need(decodeOriginalSave(frame(badRole)).status==OriginalSaveStatus::UnsupportedRole,"MAP role not accepted as SAV");need(decodeOriginalSave(frame(Bytes{'E','M','P','1'})).status==OriginalSaveStatus::UnsupportedRole,"authored EMP1 is not SAV13 evidence");
 auto corruption=bytes;corruption.back()^=1;auto failed=decodeOriginalSave(corruption);need(failed.status==OriginalSaveStatus::InvalidContainer&&failed.terrain.flags.empty()&&failed.decodedBytes==0,"late checksum failure no partial data");
 for(int kind=0;kind<3;++kind){auto x=bytes;if(kind==0)x.resize(3);if(kind==1)x.pop_back();if(kind==2)x.push_back(0);need(decodeOriginalSave(x).status==OriginalSaveStatus::InvalidContainer,"framing truncation/tail failure strict");}
 auto declared=bytes;put(declared,12,32767);need(decodeOriginalSave(declared).status==OriginalSaveStatus::InvalidContainer,"declared frame length verified");
 need(decodeOriginalSave(Bytes(4u*1024u*1024u+1)).status==OriginalSaveStatus::BudgetExceeded,"encoded input budget");need(decodeOriginalSave(frame(Bytes(8u*1024u*1024u+1))).status==OriginalSaveStatus::BudgetExceeded,"decoded/frame count budget");
 auto many=payload(50003);for(size_t i=0;i<1025;++i){const size_t pos=100+32*i;put(many,pos,112);std::memcpy(many.data()+pos+16,"Brief descriptio",16);}need(decodeOriginalSave(frame(many)).status==OriginalSaveStatus::BudgetExceeded,"marker cardinality budget rejects excess matches");
 auto terrainMany=payload(50003);for(size_t i=0;i<1025;++i)std::memcpy(terrainMany.data()+1000+20*i,body.data()+base-4,16);auto excess=decodeOriginalSave(frame(terrainMany));need(excess.status==OriginalSaveStatus::BudgetExceeded&&excess.terrain.flags.empty()&&!excess.layers.valid(),"terrain marker budget enforced before candidate validation");
 auto rootPath=std::filesystem::temp_directory_path()/("emperor-sav-self-"+std::to_string(getpid()));need(std::filesystem::create_directory(rootPath),"fresh owned fixture root");std::filesystem::create_directories(rootPath/"Save/owner");write(rootPath/"Save/owner/.sav",bytes);
 auto file=loadOriginalSave((rootPath/"Save/owner/.sav").string());need(file.status==OriginalSaveStatus::LoadedSnapshot&&file.readStable&&file.layers.mapperObjects==saved.layers.mapperObjects,"two stable complete file reads publish same snapshot");
 need(loadOriginalSave(rootPath.string()).status==OriginalSaveStatus::ReadFailure,"directory not treated as save file");std::filesystem::create_symlink(rootPath/"Save/owner/.sav",rootPath/"link.sav");need(loadOriginalSave((rootPath/"link.sav").string()).status==OriginalSaveStatus::ReadFailure,"direct file API refuses leaf symlink");
 auto root=ResourceRoot::fromAbsolute(rootPath.string());auto catalog=enumerateOriginalMapSources(root);need(catalog.complete&&catalog.saveFiles==1&&catalog.sources.size()==1&&catalog.sources[0].kind==OriginalMapSourceKind::SaveFile,"catalog includes hidden .sav name inside Save root only");
 auto selected=loadOriginalMapSource(root,catalog.sources[0]);need(selected.map.terrain.flags==saved.terrain.flags&&selected.saveLayers&&selected.saveLayers->mapperRandom==saved.layers.mapperRandom,"native source selector returns retained save raw fields");
 write(rootPath/"Save/owner/.sav",corruption);auto changed=loadOriginalMapSource(root,catalog.sources[0]);need(!changed.map.terrain.valid()&&!changed.saveLayers,"corrupt replacement clears previous terrain and raw fields");
 auto escape=catalog.sources[0];escape.relativePath="../other.sav";need(!loadOriginalMapSource(root,escape).map.terrain.valid(),"configured-root source identity enforced");std::filesystem::remove_all(rootPath);
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"authored\":true,\"original_data\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
