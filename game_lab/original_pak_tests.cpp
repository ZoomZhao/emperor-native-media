#include "original_pak.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <array>
#include <unistd.h>
#include <zlib.h>
namespace {
using Bytes=std::vector<uint8_t>;size_t checks=0;
void need(bool b,const char* msg){++checks;if(!b)throw std::runtime_error(msg);}
void append(Bytes& b,uint32_t n){for(unsigned i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
void put(Bytes& b,size_t p,uint32_t n){for(unsigned i=0;i<4;++i)b[p+i]=uint8_t(n>>(i*8));}
uint32_t word(const Bytes& b,size_t p){return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);}
Bytes framed(const Bytes& body){Bytes b;for(size_t at=0;at<body.size();at+=32768){const size_t count=std::min(size_t(32768),body.size()-at);Bytes out(compressBound(count));uLongf n=out.size();if(compress2(out.data(),&n,body.data()+at,count,1)!=Z_OK)throw std::runtime_error("authored compression");append(b,0xabcdefff);append(b,uint32_t(n));append(b,uint32_t(count));b.insert(b.end(),out.begin(),out.begin()+n);}return b;}
Bytes city(){Bytes b(game_lab::OriginalMapCellLayers::mapperFertileOffset+51984,0);b[0]=5;b[2]=0xfe;b[3]=0xca;b[6]=2;put(b,84,84);for(size_t i=0;i<51984;++i){put(b,261455+4*i,0x80000);put(b,469391+4*i,uint32_t(i)|0x80000000);b[209471+i]=uint8_t(i*3);b[729311+i]=uint8_t(i);b[937255+i]=uint8_t(255-i);}put(b,261455+4*(114*228+114),0x80000040);return b;}
Bytes pak(const std::vector<Bytes>& payloads,const std::vector<uint32_t>& inactive={}){
 Bytes b;append(b,0xfedcbaaa);std::vector<uint32_t> starts;
 for(const auto& body:payloads){starts.push_back(uint32_t(b.size()));auto f=framed(body);b.insert(b.end(),f.begin(),f.end());}
 std::vector<uint32_t> active;if(starts.size()>1)active.assign(starts.begin()+1,starts.end());else active.push_back(uint32_t(b.size()));
 const uint32_t last=uint32_t(active.size()-1);auto slots=active;slots.insert(slots.end(),inactive.begin(),inactive.end());if(slots.size()>11)throw std::runtime_error("authored slot cap");slots.resize(11);
 append(b,0xaaabcdef);append(b,0x3c);for(auto x:slots)append(b,x);append(b,last);append(b,0xaaabcdef);append(b,4);append(b,0xaaaabbcc);return b;
}
void write(const std::filesystem::path& path,const Bytes& b){std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)throw std::runtime_error("authored fixture write");}
using namespace game_lab;
}
int main(){try{
 const Bytes a{'s','e','t'},b{'e','m','p'};const auto payload=city();auto bytes=pak({a,b,payload});auto result=decodeOriginalPak(bytes);
 need(result.status==OriginalPakStatus::Loaded&&result.parts.size()==3,"complete PAK framing");
 need(!result.parts[0].cityCandidate&&!result.parts[1].cityCandidate&&result.parts[2].cityCandidate,"roles separate settings/empire from city signature");
 const auto& map=result.parts[2].map;
 need(map.status==OriginalMapStatus::LoadedTerrain&&map.terrain.activeSide==84&&map.terrain.flags[114*228+114]==0x80000040,"shared city parser preserves unsigned original terrain");
 need(map.cellLayers&&map.cellLayers->valid()&&map.cellLayers->objects.back()==(51983u|0x80000000u)&&map.cellLayers->mapperRandom[256]==0,"complete existing raw layers reused");
 need(!result.parts[2].surfaceState,"short numerical city cannot invent missing surface fields");
 auto completeCity=payload;completeCity.resize(OriginalTerrainState::flatEnd);put(completeCity,OriginalTerrainState::imageOffset+4*(114*228+114),3u<<14);
 auto surfacePackage=decodeOriginalPak(pak({a,completeCity}));
 need(surfacePackage.status==OriginalPakStatus::Loaded&&surfacePackage.parts[1].surfaceState&&surfacePackage.parts[1].surfaceState->image[114*228+114]==(3u<<14)&&surfacePackage.parts[1].surfaceState->terrain==surfacePackage.parts[1].map.terrain.flags,"full embedded MAP surface fields bind the same numerical terrain");
 need(map.frames>1&&map.decodedBytes==payload.size()&&result.decodedBytes==payload.size()+a.size()+b.size(),"all compressed frames and decoded lengths counted");
 const size_t footer=bytes.size()-68;const uint32_t third=word(bytes,footer+12);auto ghost=pak({a,b,payload},{third+13});
 need(decodeOriginalPak(ghost).status==OriginalPakStatus::Loaded,"count-excluded interior offset never selected");put(ghost,ghost.size()-68+52,2);
 need(decodeOriginalPak(ghost).status==OriginalPakStatus::InvalidContainer&&decodeOriginalPak(ghost).parts.empty(),"same interior offset declared active fails without partial city");
 const uint32_t inner=third+12+word(bytes,third+4);auto validInactive=decodeOriginalPak(pak({a,b,payload},{inner}));
 need(validInactive.status==OriginalPakStatus::Loaded&&validInactive.parts.size()==3&&validInactive.parts[2].map.terrain.flags==map.terrain.flags,"inactive frame boundary cannot split a city");
 for(uint32_t n:{11u,0xffffffffu}){auto x=bytes;put(x,footer+52,n);need(decodeOriginalPak(x).status==OriginalPakStatus::InvalidContainer,"invalid last-slot index no fallback");}
 for(size_t off:{56u,60u,64u}){auto x=bytes;put(x,footer+off,0);need(decodeOriginalPak(x).status==OriginalPakStatus::UnsupportedContainer,"terminal profile required");}
 for(uint32_t off:{0u,1u,uint32_t(footer+1),0xffffffffu}){auto x=bytes;put(x,footer+8,off);need(decodeOriginalPak(x).status==OriginalPakStatus::InvalidContainer,"active pointer bounds strict");}
 auto reverse=bytes;put(reverse,footer+8,uint32_t(footer));need(decodeOriginalPak(reverse).status==OriginalPakStatus::InvalidContainer,"EOF cannot precede a later interior pointer");
 auto repeat=pak({a,b,payload},{third,uint32_t(footer)});put(repeat,footer+52,3);need(decodeOriginalPak(repeat).parts.size()==3,"duplicate and EOF active offsets are empty spans");
 auto corrupt=bytes;corrupt[footer-1]^=1;auto failed=decodeOriginalPak(corrupt);need(failed.status==OriginalPakStatus::InvalidContainer&&failed.parts.empty()&&failed.decodedBytes==0,"late checksum failure publishes no partial data");
 auto later=pak({a,b,payload,Bytes{'b','a','d'}});later[later.size()-69]^=1;need(decodeOriginalPak(later).parts.empty(),"earlier valid city withheld on later noncity corruption");
 for(int kind=0;kind<4;++kind){auto x=bytes;if(kind==0)x.resize(3);if(kind==1)x.pop_back();if(kind==2)x.push_back(0);if(kind==3)x[0]=0;need(decodeOriginalPak(x).status!=OriginalPakStatus::Loaded,"truncated/header/trailing bytes not accepted");}
 auto shortCity=payload;shortCity.resize(88);auto partial=decodeOriginalPak(pak({a,b,shortCity}));need(partial.status==OriginalPakStatus::Loaded&&partial.parts[2].cityCandidate&&partial.parts[2].map.status==OriginalMapStatus::InvalidGrid,"complete framed but incompatible city explicitly distinct");
 auto tooLargeChunk=bytes;put(tooLargeChunk,12,32769);need(decodeOriginalPak(tooLargeChunk).status==OriginalPakStatus::BudgetExceeded,"existing per-frame budget retained");
 auto tooManyFrames=pak({Bytes(8u*1024u*1024u+1)});need(decodeOriginalPak(tooManyFrames).status==OriginalPakStatus::BudgetExceeded,"existing frame/decoded budget retained");
 auto totalBudget=pak(std::vector<Bytes>(5,Bytes(8u*1024u*1024u)));need(decodeOriginalPak(totalBudget).status==OriginalPakStatus::BudgetExceeded&&decodeOriginalPak(totalBudget).parts.empty(),"32MiB aggregate package budget retained");
 auto maximum=decodeOriginalPak(pak(std::vector<Bytes>(12,Bytes{1})));need(maximum.status==OriginalPakStatus::Loaded&&maximum.parts.size()==12,"maximum active offset index accepted");
 need(decodeOriginalPak(pak({})).status==OriginalPakStatus::InvalidContainer,"empty framed body rejected");
 auto rootPath=std::filesystem::temp_directory_path()/("emperor-pak-self-"+std::to_string(getpid()));need(std::filesystem::create_directory(rootPath),"fresh authored root");std::filesystem::create_directories(rootPath/"Cities");std::filesystem::create_directories(rootPath/"Campaigns");
 auto loose=framed(payload);loose.insert(loose.begin(),{0xaa,0xba,0xdc,0xfe});write(rootPath/"Cities/own.map",loose);write(rootPath/"Campaigns/own.PAK",bytes);
 auto root=ResourceRoot::fromAbsolute(rootPath.string());auto catalog=enumerateOriginalMapSources(root);
 need(catalog.complete&&catalog.looseMaps==1&&catalog.pakFiles==1&&catalog.completePaks==1&&catalog.cityCandidates==1&&catalog.cityMaps==1&&catalog.sources.size()==3,"catalog contains loose map, package summary and embedded city");
 need(catalog.sources[0].label=="Cities/own.map"&&catalog.sources[2].label.find("包内城市 1")!=std::string::npos,"existing loose labels retained; package city label distinct");
 auto selected=loadOriginalMapSource(root,catalog.sources[2]);need(selected.map.terrain.flags==map.terrain.flags&&selected.map.cellLayers->objects==map.cellLayers->objects,"actual source selection reloads and parses original same city values");
 auto summary=loadOriginalMapSource(root,catalog.sources[1]);need(summary.packageSummary&&!summary.map.terrain.valid()&&summary.sourceInfo.find("3 个分段")!=std::string::npos,"package summary never displays a stale city");
 auto invalid=catalog.sources[2];invalid.partIndex=0;need(!loadOriginalMapSource(root,invalid).map.terrain.valid(),"noncity part cannot become city by caller request");invalid.partIndex=99;need(!loadOriginalMapSource(root,invalid).map.terrain.valid(),"stale part index rejected");invalid.relativePath="../outside.pak";need(!loadOriginalMapSource(root,invalid).map.terrain.valid(),"source cannot escape selected root");
 write(rootPath/"Campaigns/own.PAK",corrupt);need(!loadOriginalMapSource(root,catalog.sources[2]).map.terrain.valid(),"changed corrupt package clears prior valid selection");
 auto rejected=enumerateOriginalMapSources(root);need(rejected.complete&&rejected.pakFiles==1&&rejected.completePaks==0&&rejected.sources.size()==2,"failed package still has explicit summary, no partial city entries");
 std::filesystem::create_symlink(rootPath/"Campaigns/own.PAK",rootPath/"Campaigns/alias.pak");auto alias=enumerateOriginalMapSources(root);need(!alias.complete&&alias.sources.empty(),"duplicate canonical file alias refuses catalog");
 std::filesystem::remove(rootPath/"Campaigns/alias.pak");std::filesystem::create_symlink("/etc/hosts",rootPath/"Campaigns/escape.pak");auto escaped=enumerateOriginalMapSources(root);need(!escaped.complete&&escaped.sources.empty(),"outside-root package symlink refused");
 need(!enumerateOriginalMapSources(ResourceRoot{}).complete,"no implicit data root");std::filesystem::remove_all(rootPath);
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"authored\":true,\"original_data\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
