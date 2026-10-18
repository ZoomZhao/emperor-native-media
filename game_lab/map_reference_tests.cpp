#include "map_reference.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <zlib.h>
#include <unistd.h>
namespace {using Bytes=std::vector<uint8_t>;size_t checks=0;void need(bool x,const char*s){++checks;if(!x)throw std::runtime_error(s);}void put(Bytes&b,size_t p,uint32_t n,unsigned w=4){for(unsigned i=0;i<w;++i)b[p+i]=uint8_t(n>>(8*i));}void write(const std::filesystem::path&p,const Bytes&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)throw std::runtime_error("fixture write");}Bytes body(size_t n){Bytes b(n);b[0]=5;b[2]=254;b[3]=202;b[6]=2;put(b,84,84);return b;}Bytes framed(const Bytes&b){Bytes out(4);put(out,0,0xfedcbaaa);for(size_t p=0;p<b.size();p+=32768){size_t n=std::min(size_t(32768),b.size()-p);Bytes compressed(compressBound(n));uLongf size=compressed.size();if(compress2(compressed.data(),&size,b.data()+p,n,9)!=Z_OK)throw std::runtime_error("zlib");size_t q=out.size();out.resize(q+12+size);put(out,q,0xabcdefff);put(out,q+4,uint32_t(size));put(out,q+8,uint32_t(n));std::copy(compressed.begin(),compressed.begin()+size,out.begin()+q+12);}return out;}}
void diagnosticSceneTests(const std::filesystem::path& root) {
 using namespace game_lab;
 SG3Sprite first;first.width=78;first.height=46;first.type=30;first.uncompressedLength=3200;first.length=3210;first.tileSize=1;
 auto second=first;second.height=40;second.offset=3210;second.length=3200;
 SG3Archive archive;archive.numImages=2;archive.sprites={first,second};
 Bytes bytes(6410);for(size_t i=0;i<1600;++i){put(bytes,i*2,0x7c00,2);put(bytes,3210+i*2,0x03e0,2);}
 const Bytes top={1,0xe0,3,255,255,255,250,1,31,0};std::copy(top.begin(),top.end(),bytes.begin()+3200);
 auto file=root/"scene-own.555";write(file,bytes);
 auto decodedTop=decodeSG3DiagnosticComponent(archive,0,file.string(),SG3DiagnosticComponent::Top);
 need(decodedTop.valid()&&decodedTop.pixelsBGRA[0]==0xff00ff00u&&decodedTop.pixelsBGRA[6*78+38]==0xff0000ffu,"complete original RLE top includes overlap with public base");
 need(std::count_if(decodedTop.pixelsBGRA.begin(),decodedTop.pixelsBGRA.end(),[](uint32_t p){return p>>24;})==2,"top does not include raw base or inferred visibility");
 SG3DiagnosticScene scene;scene.width=300;scene.height=200;scene.scale=2;
 scene.layers={{1,SG3DiagnosticComponent::Full,150,100},{1,SG3DiagnosticComponent::Top,150,100}};
 auto out=renderSG3DiagnosticScene(archive,file.string(),scene);
 need(out.image.valid()&&out.decodedLayers==2&&out.coveredPixels==6404,"native diagnostic full and top source coverage");
 need(out.visiblePerLayer==std::vector<size_t>({6396,8}),"exact last natural-alpha producer counts");
 need(out.image.pixelsBGRA.front()==0&&out.producer.front()==0,"unknown starts transparent without screenshot fill");
 need(out.producer[48*300+72]==2&&out.producer[100*300+150]==1,"2x bottom-base placement and provenance");
 scene.layers={{1,SG3DiagnosticComponent::Full,150,100},{2,SG3DiagnosticComponent::Full,150,100}};
 out=renderSG3DiagnosticScene(archive,file.string(),scene);need(out.image.pixelsBGRA[100*300+150]==0xff00ff00u,"declared later source paints over earlier source");
 std::reverse(scene.layers.begin(),scene.layers.end());out=renderSG3DiagnosticScene(archive,file.string(),scene);need(out.image.pixelsBGRA[100*300+150]==0xffff0000u,"input order preserved without score-based ordering");
 scene.layers={{2,SG3DiagnosticComponent::Top,150,100}};out=renderSG3DiagnosticScene(archive,file.string(),scene);
 need(out.image.valid()&&out.coveredPixels==0&&out.visiblePerLayer==std::vector<size_t>({0}),"empty natural top is entirely unknown,not invented base");
 need(!decodeSG3DiagnosticComponent(archive,1,(root/"absent.555").string(),SG3DiagnosticComponent::Top).valid(),"empty top still validates original source dependency");
 scene.layers[0].logicalID=0;out=renderSG3DiagnosticScene(archive,file.string(),scene);need(!out.image.valid()&&out.producer.empty(),"bad ID clears whole scene without stale image");
 scene.layers[0].logicalID=1;scene.layers[0].component=SG3DiagnosticComponent(8);need(!renderSG3DiagnosticScene(archive,file.string(),scene).image.valid(),"unknown component refused");
 scene.layers[0].component=SG3DiagnosticComponent::Full;scene.layers[0].centerX=std::numeric_limits<double>::quiet_NaN();need(!renderSG3DiagnosticScene(archive,file.string(),scene).image.valid(),"invalid scene centre refused");
 scene.layers[0].centerX=150;scene.width=2049;need(!renderSG3DiagnosticScene(archive,file.string(),scene).image.valid(),"scene canvas budget enforced");
 scene.width=300;scene.layers.resize(129,scene.layers.front());need(!renderSG3DiagnosticScene(archive,file.string(),scene).image.valid(),"scene layer budget enforced");
 archive.sprites[0].alphaLength=1;need(!decodeSG3DiagnosticComponent(archive,0,file.string(),SG3DiagnosticComponent::Full).valid(),"partial-alpha diagnostic source is unsupported,not silently made opaque");
}
void registeredCellTests() {
 using namespace game_lab;
 OriginalMapTerrain terrain;terrain.activeSide=112;terrain.flags.assign(51984,0x80000);
 for(unsigned y=58;y<170;++y)for(unsigned x=58;x<170;++x)terrain.flags[y*228+x]=128;
 const auto original=terrain.flags;
 SG3DiagnosticRegistration reg;reg.matrix={{{40,-40,190},{20,20,0}}};
 auto point=projectSG3DiagnosticCell(terrain,reg,2,3);
 need(point&&point->x==150&&point->y==100,"active SAV cell projects through explicit integer registration");
 reg.canvasOrigin={100,40};point=projectSG3DiagnosticCell(terrain,reg,2,3);
 need(point&&point->x==50&&point->y==60,"physical canvas origin subtracted once; scale is already in matrix");
 need(terrain.flags==original,"registration never alters terrain values");
 for(const auto& xy:std::vector<std::pair<int64_t,int64_t>>{{-1,0},{0,-1},{112,0},{0,112},{INT64_MAX,0}})
     need(!projectSG3DiagnosticCell(terrain,reg,xy.first,xy.second),"out-of-crop coordinates refused before indexing");
 terrain.flags[60*228+60]=0x80000;
 need(!projectSG3DiagnosticCell(terrain,reg,2,2),"internal off-map cell refused");terrain.flags=original;
 auto bad=reg;bad.matrix[1]=bad.matrix[0];need(!projectSG3DiagnosticCell(terrain,bad,2,3),"singular registration refused");
 for(int64_t huge:{INT64_MIN,INT64_MAX}) {
     bad=reg;bad.matrix[0][0]=huge;need(!projectSG3DiagnosticCell(terrain,bad,2,3),"linear arithmetic budget before product");
     bad=reg;bad.matrix[0][2]=huge;need(!projectSG3DiagnosticCell(terrain,bad,2,3),"translation budget before sum");
     bad=reg;bad.canvasOrigin[1]=huge;need(!projectSG3DiagnosticCell(terrain,bad,2,3),"canvas origin budget before subtraction");
 }
 bad=reg;bad.matrix[0][2]=1000000;bad.canvasOrigin[0]=-1000000;
 need(!projectSG3DiagnosticCell(terrain,bad,2,3),"projected centre budget enforced");
 terrain.flags[0]=0;need(!projectSG3DiagnosticCell(terrain,reg,2,3),"on-map flag outside crop refuses entire registration");
 terrain.flags=original;terrain.activeSide=113;need(!projectSG3DiagnosticCell(terrain,reg,2,3),"unsupported crop refused");
 terrain.flags.clear();need(!projectSG3DiagnosticCell(terrain,reg,2,3),"incomplete terrain refused");
}
void outputPathTests(const std::filesystem::path& root) {
 namespace fs=std::filesystem;
 const auto data=root/"protected-data",out=root/"path-outputs",link=root/"path-link";
 fs::create_directories(data);fs::create_directories(out/"child");fs::create_directory_symlink(out,link);
 const auto png=out/"fresh.png",producer=out/"fresh.u16le";
 auto normalized=game_lab::freshReferenceOutputs(data.string(),{png.string(),producer.string()});
 need(normalized==std::vector<std::string>({(fs::canonical(out)/"fresh.png").string(),(fs::canonical(out)/"fresh.u16le").string()}),"different fresh outputs return canonical-parent targets without creating them");
 need(!fs::exists(png)&&!fs::exists(producer),"output preflight has no creation side effects");
 auto refuses=[&](const std::vector<std::string>& paths,const char* expected) {
     std::string reason;try{game_lab::freshReferenceOutputs(data.string(),paths);}catch(const std::exception& error){reason=error.what();}
     need(reason.find(expected)!=std::string::npos,"unsafe output set refused with expected reason");
 };
 for(const auto& alias:std::vector<fs::path>{png,out/"."/"fresh.png",out/"child"/".."/"fresh.png",link/"fresh.png"}) {
     refuses({png.string(),alias.string()},"distinct");
     need(!fs::exists(png),"alias rejection occurs before either output is created");
 }
 const auto real=root/"real";fs::create_directories(real/"deep");fs::create_directory_symlink(real/"deep",out/"jump");
 refuses({(real/"combined.png").string(),(out/"jump"/".."/"combined.png").string()},"distinct");
 need(!fs::exists(real/"combined.png")&&!fs::exists(out/"combined.png"),"symlink then dotdot uses actual traversal,not lexical cancellation");
 const auto old=out/"old.png",hard=out/"hard.png",symbolic=out/"symbolic.png";
 const Bytes sentinel={137,80,78,71,13,10,26,10,42,43,44};write(old,sentinel);fs::create_hard_link(old,hard);fs::create_symlink(old,symbolic);
 refuses({old.string(),hard.string()},"distinct");
 refuses({old.string(),symbolic.string()},"distinct");
 refuses({png.string(),hard.string()},"fresh");need(!fs::exists(png),"existing producer refuses before PNG creation");
 refuses({old.string(),producer.string()},"fresh");need(!fs::exists(producer),"existing PNG refuses before producer creation");
 for(const auto& path:std::vector<fs::path>{old,hard,symbolic}) {
     std::ifstream file(path,std::ios::binary);Bytes bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
     need(bytes==sentinel,"existing PNG and same-inode aliases remain byte-identical after refusal");
 }
 const auto dangling=out/"dangling.png";fs::create_symlink(out/"absent-target",dangling);
 refuses({png.string(),dangling.string()},"fresh");need(fs::is_symlink(dangling)&&!fs::exists(png),"dangling final symlink preserved and no new output created");
 fs::create_directory_symlink(data,root/"data-link");
 refuses({(root/"data-link"/"no-write.png").string(),producer.string()},"inside data root");
 need(!fs::exists(data/"no-write.png")&&!fs::exists(producer),"protected root remains read-only through parent alias");
}
void exclusiveOutputTests(const std::filesystem::path& root) {
 namespace fs=std::filesystem;using game_lab::ExclusiveReferenceOutputs;
 const auto data=root/"exclusive-data",out=root/"exclusive-out";fs::create_directories(data);fs::create_directories(out);
 auto read=[](const fs::path& path){std::ifstream file(path,std::ios::binary);return Bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());};
 const Bytes png={137,80,78,71,13,10,26,10},producer={1,0,2,0};
 const auto a=out/"normal.png",b=out/"normal.u16le";
 {ExclusiveReferenceOutputs files(data.string(),{a.string(),b.string()});files.write(0,png.data(),png.size());files.write(1,producer.data(),producer.size());files.commit();}
 need(read(a)==png&&read(b)==producer,"different reserved targets retain separate committed payloads");
 const auto abandoned=out/"abandoned.png",abandoned2=out/"abandoned.u16le";
 {ExclusiveReferenceOutputs files(data.string(),{abandoned.string(),abandoned2.string()});files.write(0,png.data(),png.size());}
 need(!fs::exists(abandoned)&&!fs::exists(abandoned2),"uncommitted owned fresh outputs roll back together");
 try{ExclusiveReferenceOutputs files(data.string(),{abandoned.string(),abandoned2.string()});files.write(0,png.data(),png.size());files.write(9,nullptr,0);}catch(const std::exception&){}
 need(!fs::exists(abandoned)&&!fs::exists(abandoned2),"writer failure leaves no partial PNG or producer");
 const std::vector<std::pair<std::string,std::string>> aliases={{"Scene.PNG","scene.png"},{u8"Caf\u00e9.png",u8"Cafe\u0301.png"}};
 for(size_t n=0;n<aliases.size();++n) {
     const auto& names=aliases[n];const auto first=out/names.first,second=out/names.second;
     write(first,png);const bool same=fs::exists(second)&&fs::equivalent(first,second);fs::remove(first);
     bool refused=false;
     try{ExclusiveReferenceOutputs files(data.string(),{first.string(),second.string()});files.write(0,png.data(),png.size());files.write(1,producer.data(),producer.size());files.commit();}catch(const std::exception&){refused=true;}
     need(refused==same,"kernel filesystem semantics decide fresh case/Unicode aliases without text folding");
     if(same)need(!fs::exists(first)&&!fs::exists(second),"fresh filesystem alias refuses before payloads and rolls back reservation");
     else {need(read(first)==png&&read(second)==producer,"filesystem-distinct spellings remain valid outputs");fs::remove(first);fs::remove(second);}
     const auto sentinel=Bytes{17,23,41,59};write(first,sentinel);
     refused=false;try{ExclusiveReferenceOutputs files(data.string(),{abandoned.string(),second.string()});}catch(const std::exception&){refused=true;}
     if(same)need(refused&&!fs::exists(abandoned)&&read(first)==sentinel,"pre-existing case/Unicode alias is never truncated");
     else need(!refused&&read(first)==sentinel,"distinct spelling leaves existing unrelated file unchanged");
     fs::remove(first);
 }
 for(const auto& names:std::vector<std::pair<std::string,std::string>>{{"CaseData","casedata"},{u8"Caf\u00e9Data",u8"Cafe\u0301Data"}}) {
     const auto owned=root/names.first,alias=root/names.second;fs::create_directory(owned);
     if(!fs::exists(alias))fs::create_directory(alias);
     const bool same=fs::equivalent(owned,alias);bool refused=false;
     try{ExclusiveReferenceOutputs files(owned.string(),{(alias/"no-write.png").string()});}catch(const std::exception&){refused=true;}
     need(refused==same&&!fs::exists(alias/"no-write.png"),"data-root directory identities protect case/Unicode aliases under actual filesystem semantics");
 }
 const auto protectedInput=data/"input.bin",replaced=out/"replaced.png",other=out/"replaced.u16le";const Bytes sentinel={19,31,43};write(protectedInput,sentinel);
 bool identityRefused=false;
 {ExclusiveReferenceOutputs files(data.string(),{replaced.string(),other.string()});
  fs::remove(replaced);fs::create_hard_link(protectedInput,replaced);
  files.write(0,png.data(),png.size()); // held descriptor writes only its unlinked fresh inode
  try{files.commit();}catch(const std::exception&){identityRefused=true;}}
 need(identityRefused,"output-name replacement refuses final commit");
 need(read(protectedInput)==sentinel&&read(replaced)==sentinel,"held descriptor cannot truncate a replacement input inode");
 need(fs::exists(replaced)&&!fs::exists(other),"rollback preserves replacement entry and removes only the other owned fresh file");
}
// Fixture: a complete loader per-cell array set whose terrain grid is `flags`.
// Non-dispatch arrays are zero; the dispatch needs terrain/random/feature/fertility.
game_lab::OriginalTerrainState terrainState(const game_lab::OriginalMapTerrain& terrain) {
 const size_t n=game_lab::OriginalTerrainState::cells;game_lab::OriginalTerrainState state;
 state.terrain=terrain.flags;state.image.assign(n,0);state.draw.assign(n,0);state.feature.assign(n,0);
 state.subTile.assign(n,0);state.randomByte.assign(n,0);state.gF05160.assign(n,0);
 state.gEAC3F0.assign(n,0);state.roadDistance.assign(n,0);state.fertility.assign(n,0);
 state.gBEBF3A.assign(n,0);state.gBF8A4A.assign(n,0);return state;
}
// The FUN_0052E7C0 read order is a fixed flat layout over the decoded payload.
void originalTerrainTests(const std::filesystem::path& root) {
 using namespace game_lab;
 Bytes payload=body(OriginalTerrainState::flatEnd);
 auto words=[&](size_t offset){for(size_t i=0;i<OriginalTerrainState::cells;++i)put(payload,offset+4*i,uint32_t(i));};
 auto bytes=[&](size_t offset){for(size_t i=0;i<OriginalTerrainState::cells;++i)put(payload,offset+i,uint8_t(i),1);};
 words(OriginalTerrainState::imageOffset);words(OriginalTerrainState::terrainOffset);
 words(OriginalTerrainState::featureOffset);
 bytes(OriginalTerrainState::drawOffset);bytes(OriginalTerrainState::subTileOffset);
 bytes(OriginalTerrainState::randomByteOffset);bytes(OriginalTerrainState::gF05160Offset);
 bytes(OriginalTerrainState::gEAC3F0Offset);bytes(OriginalTerrainState::roadDistanceOffset);
 bytes(OriginalTerrainState::fertilityOffset);bytes(OriginalTerrainState::gBEBF3AOffset);
 bytes(OriginalTerrainState::gBF8A4AOffset);
 put(payload,OriginalTerrainState::g101D0ACOffset,0x11223344u);
 put(payload,OriginalTerrainState::g101D0B0Offset,0x55667788u);
 put(payload,OriginalTerrainState::g10C6F18Offset,0x99AABBCCu);
 for(size_t i=0;i<OriginalTerrainState::blob24Size;++i)put(payload,OriginalTerrainState::blob24Offset+i,uint8_t(0x40+i),1);
 for(size_t i=0;i<OriginalTerrainState::g10C6DB0Size;++i)put(payload,OriginalTerrainState::g10C6DB0Offset+i,uint8_t(0x80+i),1);
 const auto state=parseOriginalTerrainState(payload);
 need(state.has_value()&&state->valid(),"the loader read order parses into a complete per-cell state");
 need(state->image[0]==0u&&state->image[1]==1u&&state->image[OriginalTerrainState::cells-1]==OriginalTerrainState::cells-1,
      "the first u32 grid is a 4-byte-per-cell little-endian run");
 need(state->terrain[12345]==12345u&&state->feature[0xCAFE]==0xCAFEu,"each u32 grid keeps its own stride");
 need(state->draw[7]==7u&&state->randomByte[0x77]==0x77u&&state->subTile[1]==1u,"u8 grids are 1 byte per cell");
 need(state->fertility[OriginalTerrainState::cells-1]==uint8_t((OriginalTerrainState::cells-1)&0xFFu)&&
      state->roadDistance[3]==3u&&state->gBF8A4A[4]==4u,"later u8 grids land at their read-order offset");
 need(state->g101D0AC==0x11223344u&&state->g101D0B0==0x55667788u&&state->g10C6F18==0x99AABBCCu,
      "the scalar words between the grids are little-endian");
 need(state->blob24[0]==0x40u&&state->blob24[OriginalTerrainState::blob24Size-1]==uint8_t(0x40+OriginalTerrainState::blob24Size-1)&&
      state->g10C6DB0[0]==0x80u&&state->g10C6DB0[OriginalTerrainState::g10C6DB0Size-1]==uint8_t(0x80+OriginalTerrainState::g10C6DB0Size-1),
      "the trailing blob and 0x168-byte block are bounded and exact");
 auto truncated=payload;truncated.pop_back();
 need(!parseOriginalTerrainState(truncated),"a payload short of the flat end never publishes a partial state");
 auto wrong=payload;wrong[0]=2u;
 need(!parseOriginalTerrainState(wrong),"a non-city payload is refused before any field is read");
 // The exact same bytes reach the parser through the shared compressed container.
 write(root/"terrain-own.map",framed(payload));
 const auto loaded=loadOriginalTerrainState((root/"terrain-own.map").string());
 need(loaded&&loaded->valid()&&loaded->terrain[12345]==12345u&&loaded->fertility[OriginalTerrainState::cells-1]==
      uint8_t((OriginalTerrainState::cells-1)&0xFFu),"the file loader decodes the container and parses identically");
}
void groundSelectorTests() {
 using namespace game_lab;
 // Chain order: terrain==0 passes the 0x8C008 gate and ends on plain land.
 for(uint8_t random:{0u,1u,4u,7u,0x0Au,0xF8u}) {
     const auto plain=selectGroundTileCode(GroundCodeRequest{0u,random});
     need(plain.known&&plain.code==uint16_t(0x603)&&plain.variant==uint8_t(random&7u)&&std::string(plain.branch)=="plain-land","terrain==0 selects the pure plain-land base 0x603 with the random variant carried separately");
     need(plain.block==((random&0xF8u)==0u),"plain-land block flag follows random&0xF8");
 }
 // The 0x8C008 gate (bits 0x8|0x4000|0x8000|0x80000) lets later setters run
 // only when all four bits are clear. A cell reaching the gate has 0x100 clear,
 // so setter 1 (FUN_004EDC30) provably returned 0 without writing: the gate is a
 // determinate chain no-write, counted apart from unknown. It is not "no ground":
 // 0x00FE9880 is loaded from the `.map` payload, so the cell keeps its art.
 for(uint32_t flagged:{0x8u,0x4000u,0x8000u,0x80000u}) {
     const auto gated=selectGroundTileCode(GroundCodeRequest{flagged,0});
     need(gated.chainNoWrite&&!gated.known&&gated.code==0&&std::string(gated.branch)=="chain-declined","the 0x8C008 gate skips every setter with no img write: determinate chain no-write");
 }
 // 0x800 is setter 3 territory, not a gate bit; it must reach the 0x800 branch.
 const auto water=selectGroundTileCode(GroundCodeRequest{0x800u,0});
 need(!water.known&&std::string(water.branch)=="0x800-water","0x800 is handled by setter 3, never swallowed by the gate");
 // Setter 6: terrain&0x2 picks 0x606/0x607/0x608 from terrain&0x300002.
 for(const auto& item:std::vector<std::pair<uint32_t,uint16_t>>{{0x2u,0x606u},{0x100002u,0x607u},{0x200002u,0x608u}}) {
     const auto elevation=selectGroundTileCode(GroundCodeRequest{item.first,0});
     need(elevation.known&&elevation.code==item.second&&elevation.variant==0&&std::string(elevation.branch)=="0x2-land","terrain&0x2 picks the 0x606/0x607/0x608 base code");
 }
 // Setter 12: terrain&0x1000 -> 0x612 + (random&7) stored as T(code)+variant.
 const auto irrigated=selectGroundTileCode(GroundCodeRequest{0x1000u,0x05u});
 need(irrigated.known&&irrigated.code==uint16_t(0x612)&&irrigated.variant==5u&&std::string(irrigated.branch)=="0x1000-irrigation","terrain&0x1000 selects the pure 0x612 base with variant random&7");
 // Setters whose value needs runtime arrays report unknown instead of a guessed code.
 for(const auto& item:std::vector<std::pair<uint32_t,const char*>>{{0x100u,"0x100-special"},{0x1u,"0x1-road"},{0x4u,"0x4-shore"},
        {0x800u,"0x800-water"},{0x20000u,"0x20000-forest"},{0x2000000u,"0x2000000-delegated"},{0x40u,"0x40-road"},
        {0x10000u,"0x10000-monument"}}) {
     const auto delegated=selectGroundTileCode(GroundCodeRequest{item.first,0x07u});
     need(!delegated.known&&!delegated.chainNoWrite&&delegated.code==0&&std::string(delegated.branch)==item.second,"runtime-dependent setter names its branch without inventing a code");
 }
 // Setter 4 shore: exactly one sub-case is fixed by MAP terrain + neighbours alone.
 {
     auto request=GroundCodeRequest{0x4000004u,0x0Au};request.hasNeighbours=true;
     for(uint32_t& flag:request.neighbourFlags)flag=0x4000000u;
     const auto shore=selectGroundTileCode(request);
     need(shore.known&&!shore.block&&!shore.chainNoWrite&&shore.code==uint16_t(0x61C)&&
          shore.variant==uint8_t(0x30u+(0x0Au%0x18u))&&
          std::string(shore.branch)=="0x4-shore-water-ring","the MAP-only shore ring writes T(0x61C)+0x30+random%0x18 with the offset kept out of the code");
     // A single neighbour without 0x4000000 leaves the value undetermined.
     request.neighbourFlags[3]&=~0x4000000u;
     const auto partial=selectGroundTileCode(request);
     need(!partial.known&&!partial.chainNoWrite&&std::string(partial.branch)=="0x4-shore","a shore cell with a neighbour lacking 0x4000000 stays unknown");
     // A neighbour carrying 0x10000 diverts to the runtime DAT_0101C540 arm.
     request.neighbourFlags[3]=0x10000u;
     need(!selectGroundTileCode(request).known,"a 0x10000 neighbour keeps the shore branch unknown");
     // hasNeighbours=false (no MAP neighbour data) must never be treated as all-zero.
     const auto noNeighbours=selectGroundTileCode(GroundCodeRequest{0x4000004u,0x0Au});
     need(!noNeighbours.known&&!noNeighbours.chainNoWrite&&std::string(noNeighbours.branch)=="0x4-shore","a shore request without neighbour data never guesses");
 }
 // Setter 5 road refuses the whole cell when a 0xC064 bit survives to it: a
 // determinate chain no-write, not an unknown code.
 for(uint32_t flag:{0x1u|0x20u,0x1u|0x40u}) {
     const auto refused=selectGroundTileCode(GroundCodeRequest{flag,0x07u});
     need(refused.chainNoWrite&&!refused.known&&refused.code==0&&std::string(refused.branch)=="chain-declined","a 0xC064 bit at setter 5 is a determinate chain no-write");
 }
 // Setter 15 canal writes T(0x620)+random%9 from MAP bytes alone.
 for(uint8_t random:{0u,1u,8u,9u}) {
     const auto canal=selectGroundTileCode(GroundCodeRequest{0x40000u,random});
     need(canal.known&&!canal.block&&!canal.chainNoWrite&&canal.code==uint16_t(0x620)&&canal.variant==uint8_t(random%9u)&&std::string(canal.branch)=="0x40000-canal","terrain&0x40000 selects the pure 0x620 base with variant random%9");
 }
 // 0x20 survives to setter 15 and makes FUN_004BC800 refuse: the chain declines
 // to write and the cell keeps its loaded art.
 const auto canalRefused=selectGroundTileCode(GroundCodeRequest{0x40020u,0x05u});
 need(canalRefused.chainNoWrite&&!canalRefused.known&&std::string(canalRefused.branch)=="chain-declined","terrain&0x20 makes the canal setter refuse without writing");
 // The final fall-through is a chain no-write: every setter ran, only
 // FUN_0053F090 was left and terrain&0xAEFEDE6F was nonzero.
 const auto exhausted=selectGroundTileCode(GroundCodeRequest{0x200u,0});
 need(exhausted.chainNoWrite&&!exhausted.known&&std::string(exhausted.branch)=="chain-declined","a leftover 0xAEFEDE6F bit is a determinate chain no-write");
 // Setter 17: terrain&0x80 with (terrain&0x10001)==0 is the farm plot. Its value
 // needs the per-cell fertility (0x00E9F8E0) and random byte, both of which the
 // loader restores from the same `.map` payload. The stored id is
 // T(0x602) + 9*rank(fertility,random) + random%9 (FUN_004BD8C0 @0x004BD8C0).
 auto farmRequest=[](uint8_t fertility,uint8_t random) {
     GroundCodeRequest request;request.terrainFlags=0x80u;request.randomByte=random;
     request.hasNeighbours=true; // zeroed neighbours: no 0x10000 river arm
     request.hasLayers=true;request.fertility=fertility;return request;
 };
 for(const auto& item:std::vector<std::pair<std::pair<uint8_t,uint8_t>,uint8_t>>{
         {{0u,8u},8u},{{0u,9u},0u},{{100u,10u},37u},{{90u,9u},36u},
         {{80u,9u},27u},{{110u,12u},48u},{{255u,8u},116u}}) {
     const auto farm=selectGroundTileCode(farmRequest(item.first.first,item.first.second));
     need(farm.known&&!farm.block&&!farm.chainNoWrite&&farm.code==uint16_t(0x602)&&
          farm.variant==item.second&&std::string(farm.branch)=="0x80-farm",
          "terrain&0x80 selects the 0x602 farm base with the fertility-rank variant 9*rank+random%9");
 }
 // (90,9) is the regression for the corrected formula: the old random%9 model
 // gave 0; the executable's 9*FUN_004BD940+random%9 gives 36.
 // Every farm sub-case is named; none guesses a code.
 {
     auto river=farmRequest(0u,8u);river.neighbourFlags[0]=0x10000u;
     need(!selectGroundTileCode(river).known&&std::string(selectGroundTileCode(river).branch)=="0x80-farm-river",
          "a 0x10000 neighbour selects the farm river arm (runtime DAT_0101C540), never a guess");
     const auto block=selectGroundTileCode(farmRequest(0u,0u));
     need(!block.known&&!block.chainNoWrite&&std::string(block.branch)=="0x80-farm-block",
          "random&0xF8==0 selects the farm whole-plot block arm, left unknown");
     const auto noNeighbours=selectGroundTileCode(GroundCodeRequest{0x80u,0x08u});
     need(!noNeighbours.known&&std::string(noNeighbours.branch)=="0x80-farm-neighbours",
          "a farm request without neighbour data never treats missing bytes as zero");
     auto noLayers=farmRequest(0u,8u);noLayers.hasLayers=false;
     const auto variant=selectGroundTileCode(noLayers);
     need(!variant.known&&!variant.chainNoWrite&&std::string(variant.branch)=="0x80-farm-variant",
          "a farm request without the fertility layer stays unknown instead of a guessed variant");
 }
 // Setters ahead of 17 still win: road (0x1) and monument (0x10000).
 const auto roadFarm=selectGroundTileCode(GroundCodeRequest{0x81u,0x03u});
 need(!roadFarm.known&&std::string(roadFarm.branch)=="0x1-road","terrain&0x1 outranks the 0x80 farm branch");
 const auto monumentFarm=selectGroundTileCode(GroundCodeRequest{0x10080u,0x03u});
 need(!monumentFarm.known&&std::string(monumentFarm.branch)=="0x10000-monument","terrain&0x10000 keeps a farm bit out of branch 17");
 // Setter 10 (FUN_0053FA20 @0x0053FA20): terrain&0x2000000 with the reset image 0
 // selects the base code from the low byte of the per-cell feature array.
 auto featureRequest=[](uint32_t feature) {
     GroundCodeRequest request;request.terrainFlags=0x2000000u;request.hasLayers=true;
     request.featureFlags=feature;return request;
 };
 for(const auto& item:std::vector<std::pair<uint32_t,uint16_t>>{{8u,0x60Du},{0x10u,0x60Au},
         {0x20u,0x609u},{0x40u,0x619u},{0u,0x60Du},{0x108u,0x60Du}}) {
     const auto feature=selectGroundTileCode(featureRequest(item.first));
     need(feature.known&&feature.block&&!feature.chainNoWrite&&feature.code==item.second&&
          feature.variant==0&&std::string(feature.branch)=="0x2000000-feature",
          "terrain&0x2000000 picks the feature base (feat&8>feat&0x10>feat&0x20>feat&0x40>0x60D) as 5x5 block art");
 }
 // Bit priority matches the executable's fall-through order.
 need(selectGroundTileCode(featureRequest(8u|0x10u|0x20u|0x40u)).code==uint16_t(0x60Du),"feat&8 has the highest priority");
 need(selectGroundTileCode(featureRequest(0x10u|0x20u)).code==uint16_t(0x60Au),"feat&0x10 beats feat&0x20/0x40");
 need(selectGroundTileCode(featureRequest(0x20u|0x40u)).code==uint16_t(0x609u),"feat&0x20 beats feat&0x40");
 // Without the feature layer the branch names itself rather than assuming zeros.
 GroundCodeRequest noFeature;noFeature.terrainFlags=0x2000000u;
 const auto delegatedFeature=selectGroundTileCode(noFeature);
 need(!delegatedFeature.known&&!delegatedFeature.chainNoWrite&&std::string(delegatedFeature.branch)=="0x2000000-delegated",
      "a feature request without the layer stays unknown");

 // Tileset 2 (China_General) is the one modelled non-terrain ground code.
 const auto general=selectGroundTileCode(GroundCodeRequest{0x20u,0});
 need(general.known&&general.code==uint16_t(0x403)&&std::string(general.branch)=="0x20-general","terrain&0x20 selects China_General code 0x403");
 // Header slot k -> 1-based image id -> 0-based sprites index.
 SG3Archive archive;archive.numImages=3;archive.sprites.resize(3);archive.imageIndex[3]=1;archive.imageIndex[2]=3;
 size_t index=99;
 need(resolveGroundTileCode(archive,uint16_t(0x603),index)&&index==0,"0x600|k resolves through header slot k");
 need(resolveGroundTileCode(archive,uint16_t(0x602),index)&&index==2,"header slot value is a 1-based image id");
 need(!resolveGroundTileCode(archive,uint16_t(0x604),index)&&index==0,"empty header slot refuses and clears the index");
 need(!resolveGroundTileCode(archive,uint16_t(0x400),index),"tileset 2 is not the ground tileset");
 need(!resolveGroundTileCode(archive,uint16_t(0x000),index),"code with zero low bits refuses");
 need(!resolveGroundTileCode(archive,uint16_t(0x600u|300u),index),"slot 300 is outside the published index");
 need(!resolveGroundTileCode(archive,uint16_t(0x1000),index),"tileset 8 is not the ground tileset");
 // Regression for the folded-variant bug: the setter's offset moves the resolved
 // record id, it does not re-select a header slot. With imageIndex[3]=247 but
 // imageIndex[4]=336, folding +1 into 0x603 selected slot 4 and returned 335.
 SG3Archive ids;ids.numImages=2000;ids.sprites.resize(2000);
 ids.imageIndex[2]=202;ids.imageIndex[3]=247;ids.imageIndex[4]=336;ids.imageIndex[0x1C]=664;
 need(resolveGroundTileCode(ids,uint16_t(0x603),uint8_t(0),index)&&index==246,"0x603 base resolves imageIndex[3]-1");
 need(resolveGroundTileCode(ids,uint16_t(0x603),uint8_t(1),index)&&index==247,"0x603 with variant 1 resolves imageIndex[3]-1+1, not imageIndex[4]-1");
 for(uint8_t r:{0u,1u,8u})
     need(resolveGroundTileCode(ids,uint16_t(0x602),uint8_t(r%9u),index)&&index==201u+r%9u,"farm variant offsets the resolved record id");
 for(uint8_t r:{0u,0x17u})
     need(resolveGroundTileCode(ids,uint16_t(0x61C),uint8_t(0x30u+r%0x18u),index)&&index==663u+0x30u+r%0x18u,"shore ring variant offsets the resolved record id");
 need(!resolveGroundTileCode(ids,uint16_t(0x605),uint8_t(3),index)&&index==0,"empty slot refuses before the variant is added");
 SG3Archive small;small.numImages=3;small.sprites.resize(3);small.imageIndex[3]=2;
 need(resolveGroundTileCode(small,uint16_t(0x603),uint8_t(1),index)&&index==2,"the last in-range variant resolves");
 need(!resolveGroundTileCode(small,uint16_t(0x603),uint8_t(2),index)&&index==0,"an out-of-range variant is refused rather than clamped");
 // The viewport consumes the parsed 228x228 loader state (all per-cell arrays)
 // and refuses anything short of a complete state.
 OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0;
 OriginalMapCamera camera;camera.centerX=41.5;camera.centerY=41.5;camera.width=64;camera.height=48;camera.scale=1;
 auto state=terrainState(terrain);
 auto shortRandom=state;shortRandom.randomByte.pop_back();
 need(!renderMapGroundViewport(terrain,shortRandom,archive,"",camera).image.valid(),"a loader state with a short per-cell array refuses");
 auto longRandom=state;longRandom.randomByte.push_back(0);
 need(!renderMapGroundViewport(terrain,longRandom,archive,"",camera).image.valid(),"a loader state with an overlong per-cell array refuses");
 auto mismatched=state;mismatched.terrain.pop_back();
 need(!renderMapGroundViewport(terrain,mismatched,archive,"",camera).image.valid(),"a loader terrain grid of a different size than the MAP flags refuses");
 need(!renderMapGroundViewport(terrain,state,{},"",camera).image.valid(),"empty archive refuses before counting");
 // Plain land resolves and counts; unsupported geometry stays explicit, never drawn.
 archive.sprites[0].type=30;archive.sprites[0].width=78;archive.sprites[0].height=40;
 archive.sprites[0].uncompressedLength=3200;archive.sprites[0].length=3200;
 const auto plain=renderMapGroundViewport(terrain,state,archive,"",camera);
 need(plain.image.valid()&&plain.experimental,"ground selector viewport is a bounded candidate");
 need(plain.stats.activeCells==84*84&&plain.stats.knownCells==84*84&&plain.stats.unknownCells==0,"plain land is fully known");
 need(plain.stats.blockCells==84*84&&plain.stats.unresolvedCodes==0&&plain.stats.geometrySupported==84*84,"block flag and supported geometry counted");
 need(plain.stats.branchCells.size()==1&&plain.stats.branchCells[0].first=="plain-land"&&plain.stats.branchCells[0].second==84*84,"branch histogram names plain-land");
 // Gate-failed terrain is a determinate chain no-write (setter 1 provably wrote
 // nothing), counted apart from unknown and never drawn.
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0x8;
 state=terrainState(terrain);
 const auto gated=renderMapGroundViewport(terrain,state,archive,"",camera);
 need(gated.image.valid()&&gated.stats.activeCells==84*84&&gated.stats.chainNoWriteCells==84*84&&gated.stats.knownCells==0&&gated.stats.unknownCells==0&&gated.stats.drawnCells==0,"gate-failed cells are a determinate chain no-write and never drawn");
 need(gated.stats.branchCells.size()==1&&gated.stats.branchCells[0].first=="chain-declined","gate-failed cells name the chain-declined branch");
 // A chain no-write cell is counted apart from unknown, never guessed, and still
 // names its branch.
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0x200;
 state=terrainState(terrain);
 const auto chainNoWrite=renderMapGroundViewport(terrain,state,archive,"",camera);
 need(chainNoWrite.image.valid()&&chainNoWrite.stats.activeCells==84*84&&chainNoWrite.stats.chainNoWriteCells==84*84&&
      chainNoWrite.stats.knownCells==0&&chainNoWrite.stats.unknownCells==0&&chainNoWrite.stats.drawnCells==0,"chain no-write cells are counted apart from unknown and never drawn");
 need(chainNoWrite.stats.branchCells.size()==1&&chainNoWrite.stats.branchCells[0].first=="chain-declined","chain no-write cells name the chain-declined branch");
 // The canal setter resolves from terrain flags alone: every cell is known.
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0x40000;
 state=terrainState(terrain);
 const auto canal=renderMapGroundViewport(terrain,state,archive,"",camera);
 need(canal.image.valid()&&canal.stats.knownCells==84*84&&canal.stats.unknownCells==0&&canal.stats.chainNoWriteCells==0,"terrain&0x40000 is known without any runtime state");
 need(canal.stats.branchCells.size()==1&&canal.stats.branchCells[0].first=="0x40000-canal","canal cells name the canal branch");
}
// The stored surface-image grid (0x00FE9880) render and the chain score against
// it. A synthetic archive keeps the record-id base small so the two-cell grid
// exercises the exact mapping without 200 dummy records.
void imageGridTests(const std::filesystem::path& root) {
 using namespace game_lab;
 constexpr size_t table=40680;Bytes bytes(table+4*64);
 put(bytes,0,uint32_t(bytes.size()));put(bytes,4,213);put(bytes,12,20);put(bytes,16,3);put(bytes,20,1);
 bytes[680]='o';bytes[681]=0;
 // id1 single-cell type 30; id2 plain type 12; id3 composite type 30 width 158.
 auto record=[&](size_t id,uint32_t length,uint16_t width,uint16_t height,uint16_t type) {
     const size_t p=table+id*64;put(bytes,p,0u);put(bytes,p+4,length);put(bytes,p+8,length);
     put(bytes,p+20,width,2);put(bytes,p+22,height,2);put(bytes,p+50,type,2);bytes[p+55]=1;};
 record(1,3200,78,40,30);record(2,0,24,24,12);record(3,3200,158,40,30);
 write(root/"grid-own.sg3",bytes);
 Bytes pixels(3200);for(size_t i=0;i<1600;++i)put(pixels,i*2,0x7c00,2);
 write(root/"grid-own.555",pixels);
 const auto archive=loadSG3((root/"grid-own.sg3").string());
 need(archive.numImages==3&&archive.sprites.size()==3,"synthetic stored-grid archive loads");
 OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);
 auto at=[&](unsigned x,unsigned y){return size_t(y+72)*228+x+72;};
 terrain.flags[at(41,41)]=0;terrain.flags[at(42,41)]=0;terrain.flags[at(43,41)]=0;
 terrain.flags[at(44,41)]=0;terrain.flags[at(45,41)]=0;terrain.flags[at(46,41)]=0;
 auto state=terrainState(terrain);
 state.image[at(41,41)]=(uint32_t(3)<<14)|0u;  // recordId1 -> single-cell, drawn
 state.image[at(42,41)]=0u;                      // no stored ground code
 state.image[at(43,41)]=(uint32_t(3)<<14)|1u;   // recordId2 -> plain type 12
 state.image[at(44,41)]=(uint32_t(3)<<14)|2u;   // recordId3 -> composite width 158
 state.image[at(45,41)]=(uint32_t(3)<<14)|5u;   // recordId6 -> beyond numImages
 state.image[at(46,41)]=(uint32_t(16)<<14)|0u;  // tileset 16 with no supplied archive
 OriginalMapCamera camera;camera.centerX=43.5;camera.centerY=41.5;camera.width=600;camera.height=400;camera.scale=1;
 const std::vector<MapImageGridTileset> tilesets{{3,&archive,(root/"grid-own.555").string(),1}};
 const auto grid=renderMapImageGridViewport(terrain,state,tilesets,camera);
 need(grid.image.valid()&&grid.experimental,"stored-grid viewport is a bounded candidate");
 need(grid.stats.activeCells==6&&grid.stats.zeroCells==1&&grid.stats.nonzeroCells==5,
      "stored-grid active/zero/nonzero counts separate a missing ground code from data");
 need(grid.stats.decodedCells==1&&grid.stats.visibleCells==1&&grid.stats.drawnCells==1&&
      grid.stats.culled==0&&grid.stats.decodeFailures==0,"one stored single-cell record is drawn");
 need(grid.stats.unresolvedIndices==1&&grid.stats.geometryUnsupported==2&&
      grid.stats.archiveUnavailable==1,"out-of-range, composite and unmapped tilesets stay distinct");
 need(grid.stats.tilesetCells==std::vector<std::pair<uint32_t,size_t>>({{3,4},{16,1}})&&
      grid.stats.tilesetDrawn==std::vector<std::pair<uint32_t,size_t>>({{3,1}}),
      "per-tileset stored-cell and drawn histograms are exact");
 size_t covered=0;for(auto pixel:grid.image.pixelsBGRA)covered+=bool(pixel>>24);
 need(covered==1600,"the stored single-cell record paints its authored coverage only");
 // Composite (multi-tile) surface art. A record that structurally declares an s*s
 // block (type 30, width==s*80-2, height>=s*40, uncompressedLength==s*s*3200,
 // tileSize in {0,s}) is expanded once per block at the block's own anchor cell.
 // An irregular component is named as a mismatch, never absorbed as a guess.
 Bytes cbytes(table+5*64);
 put(cbytes,0,uint32_t(cbytes.size()));put(cbytes,4,213);put(cbytes,12,20);put(cbytes,16,4);put(cbytes,20,1);
 cbytes[680]='c';cbytes[681]=0;
 auto crecord=[&](size_t id,uint32_t length,uint16_t width,uint16_t height,uint16_t type,uint8_t tileSize) {
     const size_t p=table+id*64;put(cbytes,p,0u);put(cbytes,p+4,length);put(cbytes,p+8,length);
     put(cbytes,p+20,width,2);put(cbytes,p+22,height,2);put(cbytes,p+50,type,2);cbytes[p+55]=tileSize;};
 crecord(1,3200,78,40,30,1);        // single-cell control
 crecord(2,12800,158,80,30,2);      // cols 2 composite: 158==2*80-2, 4*3200
 crecord(3,12800,158,80,30,1);      // same geometry, tileSize disagrees: not a block
 crecord(4,12800,158,80,30,2);      // a second, distinct cols-2 block record
 write(root/"grid-comp.sg3",cbytes);
 Bytes cpixels(12800);for(size_t i=0;i<1600;++i)put(cpixels,i*2,0x7c00,2);
 write(root/"grid-comp.555",cpixels);
 const auto carchive=loadSG3((root/"grid-comp.sg3").string());
 need(carchive.numImages==4,"synthetic composite archive loads");
 OriginalMapTerrain cterrain;cterrain.activeSide=84;cterrain.flags.assign(51984,0x80000);
 for(unsigned y=41;y<45;++y)for(unsigned x=41;x<45;++x)cterrain.flags[at(x,y)]=0;
 auto cstate=terrainState(cterrain);
 // The per-cell draw-state offset names each cell's position in the 2x2 block
 // (column low 3 bits, row bits 3..5) and subTile low nibble + 1 is the group side.
 for(unsigned y=41;y<43;++y)for(unsigned x=41;x<43;++x) {
     cstate.image[at(x,y)]=(uint32_t(3)<<14)|1u;
     cstate.draw[at(x,y)]=uint8_t((y-41)*8+(x-41));
     cstate.subTile[at(x,y)]=1;   // group side 2 == record cols
 }
 cstate.image[at(43,43)]=(uint32_t(3)<<14)|1u;cstate.image[at(44,43)]=(uint32_t(3)<<14)|1u;
 cstate.image[at(43,44)]=(uint32_t(3)<<14)|1u;                                                    // irregular 3
 cstate.image[at(44,44)]=(uint32_t(3)<<14)|0u;                                                    // control
 OriginalMapCamera ccamera;ccamera.centerX=42.5;ccamera.centerY=42.5;ccamera.width=600;ccamera.height=400;ccamera.scale=1;
 const std::vector<MapImageGridTileset> ctilesets{{3,&carchive,(root/"grid-comp.555").string(),1}};
 const auto comp=renderMapImageGridViewport(cterrain,cstate,ctilesets,ccamera);
 need(comp.image.valid(),"composite stored-grid viewport renders");
 need(comp.stats.compositeBlocks==1&&comp.stats.compositeConfirmedCells==4&&
      comp.stats.compositeRecoveredCells==0&&comp.stats.compositePartialBlocks==0,
      "a 2x2 block with matching draw offsets and group side is one confirmed block");
 need(comp.stats.compositeMismatchedCells==3&&comp.stats.compositeMismatches.size()==1&&
      comp.stats.compositeMismatches[0].cells==3&&
      std::string(comp.stats.compositeMismatches[0].reason)=="subTile group side != record cols",
      "an irregular three-cell component is named as a mismatch, never absorbed");
 need(comp.stats.geometryUnsupported==3,"mismatched composite cells stay unrendered rather than guessed");
 need(comp.stats.decodedCells==5,"the confirmed block counts its four cells once, plus the control");
 need(comp.stats.decodedCells+comp.stats.geometryUnsupported+comp.stats.archiveUnavailable+
      comp.stats.decodeFailures==comp.stats.nonzeroCells,
      "stored-grid accounting is exact: every nonzero cell is decoded or named");
 // Per-cell offset placement also resolves a block whose component shape an
 // overwrite broke: three cells keep offsets (0,0),(1,0),(0,1); the fourth carries
 // a single-cell record. The anchor is still (41,41), the block is drawn once and
 // the overwritten cell decodes by its own record, so nothing is counted twice.
 auto partialState=terrainState(cterrain);
 for(unsigned y=41;y<43;++y)for(unsigned x=41;x<43;++x) {
     partialState.image[at(x,y)]=(uint32_t(3)<<14)|1u;
     partialState.draw[at(x,y)]=uint8_t((y-41)*8+(x-41));
     partialState.subTile[at(x,y)]=1;
 }
 partialState.image[at(42,42)]=(uint32_t(3)<<14)|0u;
 partialState.draw[at(42,42)]=0x40;partialState.subTile[at(42,42)]=0;
 const auto partial=renderMapImageGridViewport(cterrain,partialState,ctilesets,ccamera);
 need(partial.stats.compositeBlocks==1&&partial.stats.compositePartialBlocks==1&&
      partial.stats.compositePartialBlockCells==3&&partial.stats.compositeRecoveredCells==3,
      "a partially overwritten block is placed at the anchor its surviving offsets name");
 need(partial.stats.decodedCells==3+1&&partial.stats.geometryUnsupported==0&&
      partial.stats.nonzeroCells==4&&
      partial.stats.decodedCells+partial.stats.geometryUnsupported+partial.stats.archiveUnavailable+
      partial.stats.decodeFailures==partial.stats.nonzeroCells,
      "the overwritten cell decodes by its own record and the accounting stays exact");
 // An offset outside the record block is not absorbed: that cell stays named while
 // the three consistent cells still form a counted partial block.
 auto badOffset=terrainState(cterrain);
 for(unsigned y=41;y<43;++y)for(unsigned x=41;x<43;++x) {
     badOffset.image[at(x,y)]=(uint32_t(3)<<14)|1u;
     badOffset.draw[at(x,y)]=uint8_t((y-41)*8+(x-41));
     badOffset.subTile[at(x,y)]=1;
 }
 badOffset.draw[at(42,42)]=2; // column 2 of a cols-2 record
 const auto outside=renderMapImageGridViewport(cterrain,badOffset,ctilesets,ccamera);
 need(outside.stats.compositeBlocks==1&&outside.stats.compositePartialBlockCells==3&&
      outside.stats.compositeMismatchedCells==1&&outside.stats.geometryUnsupported==1&&
      outside.stats.compositeMismatches.size()==1&&
      std::string(outside.stats.compositeMismatches[0].reason)=="draw offset outside the record block",
      "an out-of-range draw offset is named, not silently accepted");
 need(outside.stats.decodedCells==3&&outside.stats.decodedCells+outside.stats.geometryUnsupported+
      outside.stats.archiveUnavailable+outside.stats.decodeFailures==outside.stats.nonzeroCells,
      "the out-of-range cell keeps the stored-grid accounting exact");
 // Four cells that all claim offset (0,0) would imply four overlapping copies of one
 // block. The footprint-overlap guard refuses every copy instead of drawing one.
 auto duplicate=terrainState(cterrain);
 for(unsigned y=41;y<43;++y)for(unsigned x=41;x<43;++x) {
     duplicate.image[at(x,y)]=(uint32_t(3)<<14)|1u;
     duplicate.draw[at(x,y)]=0;   // every cell claims to be the block anchor
     duplicate.subTile[at(x,y)]=1;
 }
 const auto duplicated=renderMapImageGridViewport(cterrain,duplicate,ctilesets,ccamera);
 need(duplicated.stats.compositeBlocks==0&&duplicated.stats.compositeMismatchedCells==4&&
      duplicated.stats.geometryUnsupported==4&&duplicated.stats.decodedCells==0&&
      std::string(duplicated.stats.compositeMismatches[0].reason)=="overlapping block footprints",
      "duplicated block offsets are refused rather than drawing overlapping copies");
 // A subTile group side that contradicts the record's declared block side is not a
 // block: all four cells stay unresolved and every one of them is named.
 auto wrongSide=terrainState(cterrain);
 for(unsigned y=41;y<43;++y)for(unsigned x=41;x<43;++x) {
     wrongSide.image[at(x,y)]=(uint32_t(3)<<14)|1u;
     wrongSide.draw[at(x,y)]=uint8_t((y-41)*8+(x-41));
     wrongSide.subTile[at(x,y)]=0; // group side 1, record cols 2
 }
 const auto contradiction=renderMapImageGridViewport(cterrain,wrongSide,ctilesets,ccamera);
 need(contradiction.stats.compositeBlocks==0&&contradiction.stats.compositeMismatchedCells==4&&
      contradiction.stats.geometryUnsupported==4&&contradiction.stats.decodedCells==0&&
      std::string(contradiction.stats.compositeMismatches[0].reason)=="subTile group side != record cols",
      "a contradictory subTile group side is refused, never guessed");
 // Two different block records whose offsets claim the same anchor are inconsistent:
 // the anchor is ambiguous, so neither is placed and both cells stay named.
 OriginalMapTerrain twoCells;twoCells.activeSide=84;twoCells.flags.assign(51984,0x80000);
 twoCells.flags[at(41,41)]=0;twoCells.flags[at(42,41)]=0;
 auto twoState=terrainState(twoCells);
 twoState.image[at(41,41)]=(uint32_t(3)<<14)|1u;twoState.draw[at(41,41)]=0;twoState.subTile[at(41,41)]=1;
 twoState.image[at(42,41)]=(uint32_t(3)<<14)|3u;twoState.draw[at(42,41)]=1;twoState.subTile[at(42,41)]=1;
 const auto ambiguous=renderMapImageGridViewport(twoCells,twoState,ctilesets,ccamera);
 need(ambiguous.stats.compositeBlocks==0&&ambiguous.stats.compositeMismatchedCells==2&&
      ambiguous.stats.geometryUnsupported==2&&ambiguous.stats.decodedCells==0&&
      std::string(ambiguous.stats.compositeMismatches[0].reason)=="conflicting draw offsets for one block anchor",
      "two records claiming one block anchor are inconsistent and are not accepted");
 auto badTile=cstate;
 for(unsigned y=41;y<43;++y)for(unsigned x=41;x<43;++x)badTile.image[at(x,y)]=(uint32_t(3)<<14)|2u;
 const auto notBlock=renderMapImageGridViewport(cterrain,badTile,ctilesets,ccamera);
 need(notBlock.stats.compositeBlocks==0&&notBlock.stats.compositeConfirmedCells==0&&
      notBlock.stats.compositeForeignCells>=4,
      "a multi-tile record whose tileSize contradicts its width is counted foreign, not expanded");
 // A tileset entry without an archive is a programming error, not data: refuse.
 std::vector<MapImageGridTileset> broken{{3,nullptr,"",1}};
 need(!renderMapImageGridViewport(terrain,state,broken,camera).image.valid(),
      "a tileset entry without an archive refuses the whole viewport");
 std::vector<MapImageGridTileset> mismatched{{3,&archive,"",1}};
 auto bad=archive;bad.sprites.pop_back();mismatched[0].archive=&bad;
 need(!renderMapImageGridViewport(terrain,state,mismatched,camera).image.valid(),
      "an archive whose count disagrees with its records refuses");
 auto shortState=state;shortState.image.pop_back();
 need(!renderMapImageGridViewport(terrain,shortState,tilesets,camera).image.valid(),
      "a loader state with a short image grid refuses before counting");
 // The exe's static tileset name table (FUN_00475B60 @0x00475B60) maps the four
 // stored tilesets the corpus still referenced to these archives. The mapping is
 // the name table, not a geometry fingerprint: several version-213 archives share
 // a 201-based type-30 run, so geometry alone cannot tell them apart.
 need(mapImageGridArchiveName(2)&&std::string(mapImageGridArchiveName(2))=="China_General.sg3"&&
      mapImageGridArchiveName(5)&&std::string(mapImageGridArchiveName(5))=="China_Mon_Tumulus.sg3"&&
      mapImageGridArchiveName(6)&&std::string(mapImageGridArchiveName(6))=="China_Mon_Grand_Canal.sg3"&&
      mapImageGridArchiveName(8)&&std::string(mapImageGridArchiveName(8))=="China_Mon_Earthen_Greatwall_1.sg3"&&
      mapImageGridArchiveName(3)&&std::string(mapImageGridArchiveName(3))=="China_Terrain.sg3"&&
      mapImageGridArchiveName(16)&&std::string(mapImageGridArchiveName(16))=="China_Elevation.sg3",
      "the executable's tileset name table maps every referenced tileset to its archive");
 need(!mapImageGridArchiveName(0)&&!mapImageGridArchiveName(1)&&!mapImageGridArchiveName(99),
      "tileset indices outside the referenced ground/object archives name no archive");
 // One hand-built archive per newly mapped tileset, with a single-cell type-30
 // record at recordId = low14 + 201 and a state cell that references it.
 Bytes mappingPixels(3200);for(size_t i=0;i<1600;++i)put(mappingPixels,i*2,0x7c00,2);
 write(root/"grid-mapping.555",mappingPixels);
 const auto mappingInternal=(root/"grid-mapping.555").string();
 for(const auto& item:std::vector<std::pair<uint32_t,uint32_t>>{{2u,7u},{5u,9u},{6u,11u},{8u,13u}}) {
     constexpr uint32_t count=220;
     const uint32_t rid=201u+item.second;
     Bytes mapping(table+(count+1)*64);
     put(mapping,0,uint32_t(mapping.size()));put(mapping,4,213);put(mapping,12,count+10);
     put(mapping,16,count);put(mapping,20,1);mapping[680]='o';mapping[681]=0;
     const size_t record=table+size_t(rid)*64;
     put(mapping,record,0u);put(mapping,record+4,3200u);put(mapping,record+8,3200u);
     put(mapping,record+20,78,2);put(mapping,record+22,40,2);put(mapping,record+50,30,2);
     mapping[record+55]=1;
     const auto archivePath=root/("grid-ts"+std::to_string(item.first)+".sg3");
     write(archivePath,mapping);
     const auto loaded=loadSG3(archivePath.string());
     need(loaded.numImages==count&&loaded.sprites.size()==count&&
          sg3SingleCellGeometry(loaded,size_t(rid-1)).has_value(),
          "each mapped tileset archive publishes the referenced single-cell record");
     OriginalMapTerrain mappingTerrain;mappingTerrain.activeSide=84;
     mappingTerrain.flags.assign(51984,0x80000);mappingTerrain.flags[at(41,41)]=0;
     auto mappingState=terrainState(mappingTerrain);
     mappingState.image[at(41,41)]=(item.first<<14)|item.second;
     OriginalMapCamera mappingCamera;mappingCamera.centerX=41;mappingCamera.centerY=41;
     mappingCamera.width=200;mappingCamera.height=200;mappingCamera.scale=1;
     const std::vector<MapImageGridTileset> mappingTilesets{{item.first,&loaded,mappingInternal,201}};
     const auto mappingGrid=renderMapImageGridViewport(mappingTerrain,mappingState,
         mappingTilesets,mappingCamera);
     need(mappingGrid.image.valid()&&mappingGrid.stats.activeCells==1&&
          mappingGrid.stats.nonzeroCells==1&&mappingGrid.stats.decodedCells==1&&
          mappingGrid.stats.visibleCells==1&&mappingGrid.stats.drawnCells==1&&
          mappingGrid.stats.archiveUnavailable==0&&mappingGrid.stats.geometryUnsupported==0&&
          mappingGrid.stats.unresolvedIndices==0&&mappingGrid.stats.decodeFailures==0,
          "a stored cell from each newly mapped tileset decodes through its own archive");
     size_t painted=0;for(auto pixel:mappingGrid.image.pixelsBGRA)painted+=bool(pixel>>24);
     need(painted==1600,"the mapped tileset record paints its authored coverage only");
 }
 // An absent archive keeps the tileset visible as data, never guessed from another
 // archive and never silently decoded.
 {
     OriginalMapTerrain absentTerrain;absentTerrain.activeSide=84;
     absentTerrain.flags.assign(51984,0x80000);absentTerrain.flags[at(41,41)]=0;
     auto absentState=terrainState(absentTerrain);
     absentState.image[at(41,41)]=(uint32_t(5)<<14)|9u;
     OriginalMapCamera absentCamera;absentCamera.centerX=41;absentCamera.centerY=41;
     absentCamera.width=200;absentCamera.height=200;absentCamera.scale=1;
     const auto absent=renderMapImageGridViewport(absentTerrain,absentState,{},absentCamera);
     need(absent.image.valid()&&absent.stats.activeCells==1&&absent.stats.nonzeroCells==1&&
          absent.stats.archiveUnavailable==1&&absent.stats.decodedCells==0&&
          absent.stats.drawnCells==0,"a newly mapped tileset whose archive is absent stays counted");
 }
}
// The chain score compares the reconstructed dispatch chain with the stored grid.
// Every bucket is asserted, and the three conservation identities are checked so a
// silent drop cannot pass.
void chainScoreTests() {
 using namespace game_lab;
 SG3Archive archive;archive.numImages=10;archive.sprites.resize(10);
 archive.imageIndex[2]=1; // slot 2 -> image id 1 -> base index 0
 archive.imageIndex[13]=0;// the feature base code has no slot: an unresolved emit
 OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);
 auto at=[&](unsigned x,unsigned y){return size_t(y+72)*228+x+72;};
 terrain.flags[at(41,41)]=0x80u;      // farm, stored matches the emitted record
 terrain.flags[at(42,41)]=0x80u;      // farm, stored differs
 terrain.flags[at(43,41)]=0x200u;     // chain no-write, stored zero
 terrain.flags[at(44,41)]=0x200u;     // chain no-write, stored nonzero
 terrain.flags[at(45,41)]=0x2000000u; // feature branch, base slot empty
 terrain.flags[at(46,41)]=0x80u;      // farm, stored tileset 16
 auto state=terrainState(terrain);
 for(size_t cell=0;cell<OriginalTerrainState::cells;++cell)state.randomByte[cell]=8u;
 state.feature[at(45,41)]=8u;
 state.image[at(41,41)]=(uint32_t(3)<<14)|8u;   // farm variant 8 -> stored index 8
 state.image[at(42,41)]=(uint32_t(3)<<14)|9u;
 state.image[at(43,41)]=0u;
 state.image[at(44,41)]=(uint32_t(3)<<14)|3u;
 state.image[at(45,41)]=0u;
 state.image[at(46,41)]=(uint32_t(16)<<14)|0u;
 const auto score=scoreGroundChainAgainstImageGrid(terrain,state,archive,1);
 need(score.activeCells==6&&score.storedZero==2&&score.chainNoWrite==2&&
      score.chainUnknown==0&&score.chainKnown==4,"chain score partitions every active cell");
 need(score.emitted==3&&score.matched==1&&score.mismatched==2&&
      score.emitted==score.matched+score.mismatched,"emitted splits into matched and mismatched");
 need(score.unresolved==1&&score.chainNoWriteVsStoredZero==1&&score.chainNoWriteVsStoredNonzero==1&&
      score.chainNoWrite==score.chainNoWriteVsStoredZero+score.chainNoWriteVsStoredNonzero,
      "a chain no-write prediction is checked against the stored zero");
 need(score.knownMatched==2&&score.storedZeroUnpredicted==1&&
      score.storedZero==score.chainNoWriteVsStoredZero+score.storedZeroUnpredicted,
      "stored zeros are all accounted for as agreed or unpredicted");
 bool sawFarm=false,sawChainNoWrite=false,sawFeature=false;
 for(const auto& branch:score.branches) {
     if(branch.branch=="0x80-farm"){sawFarm=true;need(branch.cells==3&&branch.emitted==3&&
         branch.matched==1&&branch.storedZero==0,"the farm branch carries its own score");}
     if(branch.branch=="chain-declined"){sawChainNoWrite=true;need(branch.cells==2&&branch.emitted==0&&
         branch.matched==0&&branch.storedZero==1,"the chain-declined branch carries its own score");}
     if(branch.branch=="0x2000000-feature"){sawFeature=true;need(branch.cells==1&&branch.emitted==0&&
         branch.storedZero==1,"the unresolved feature branch is still named");}
 }
 need(sawFarm&&sawChainNoWrite&&sawFeature,"every reached branch is reported");
 auto shortState=state;shortState.terrain.pop_back();
 need(scoreGroundChainAgainstImageGrid(terrain,shortState,archive,1).activeCells==0,
      "an incomplete loader state refuses before scoring");
}
// Optional original-data assertion. Reads only EMPEROR_GAMEDATA_ROOT and reports
// SKIP (leaving `originalData` false) when the variable or the city is absent, so
// the authored suite still passes on a machine without the user's assets.
void realDataGroundTests(bool& originalData) {
 using namespace game_lab;
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
 if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
 const auto city=std::filesystem::path(env)/"Cities"/"Anyang.map";
 if(!std::filesystem::exists(city)){std::cout<<"{\"skipped\":true,\"reason\":\"Anyang.map absent under EMPEROR_GAMEDATA_ROOT\"}\n";return;}
 const auto loaded=loadOriginalMapTerrain(city.string());
 need(loaded.status==OriginalMapStatus::LoadedTerrain&&loaded.cellLayers.has_value(),
      "original Anyang.map loads terrain and mapper cell layers");
 const auto& terrain=loaded.terrain;
 const auto state=loadOriginalTerrainState(city.string());
 need(state&&state->valid(),"original Anyang.map publishes the FUN_0052E7C0 read-order per-cell state");
 // The two parsers read the same payload: the terrain grid and the random/fertility
 // layers must agree cell for cell.
 need(state->terrain==terrain.flags&&state->randomByte==loaded.cellLayers->mapperRandom&&
      state->fertility==loaded.cellLayers->mapperFertile,
      "the read-order state and the container cell-layer parser agree on terrain/random/fertility");
 size_t active=0,known=0,unknown=0,chainNoWrite=0,farm=0,farmBlock=0,farmRiver=0;
 for(size_t cell=0;cell<terrain.flags.size();++cell) {
     if(terrain.flags[cell]&0x80000u)continue;
     ++active;
     const auto selection=selectGroundTileCode(groundCodeRequest(*state,cell));
     if(selection.chainNoWrite)++chainNoWrite;else if(selection.known)++known;else ++unknown;
     if(std::string(selection.branch)=="0x80-farm")++farm;
     else if(std::string(selection.branch)=="0x80-farm-block")++farmBlock;
     else if(std::string(selection.branch)=="0x80-farm-river")++farmRiver;
 }
 // Measured on the user's own Anyang.map: 9940 active cells, 6212 farmland cells.
 need(active==9940&&farm+farmBlock+farmRiver==6212,"original Anyang.map active and farmland cell counts are stable");
 // Branch 17 is resolved from MAP bytes: 5992 cells emit the 0x602 plot with the
 // fertility-rank variant; the 220 whole-plot cells (random&0xF8==0) use block art
 // that is not modelled and honestly stay unknown; no 0x10000 neighbour occurs.
 need(farm==5992&&farmBlock==220&&farmRiver==0,"the Anyang farm sub-cases are stable");
 // The 0x4 shore-ring sub-case and the farm formula are MAP-determinable, giving
 // 6690 known, 3083 still runtime-unknown (0x1-road / 0x4-shore / 0x40-road ... )
 // and 167 a determinate chain no-write. The 220 farm block cells move from the old
 // guessed 6910 known to unknown. Measured on the user's own Anyang.map.
 need(known==6690&&unknown==3083&&chainNoWrite==167&&known+unknown+chainNoWrite==active,
      "original Anyang.map MAP-only outcome counts are stable");
 // Overall MAP-only known coverage floor (~67%). It cannot reach the farmland level
 // until the 0x1-road and 0x4-shore general arms are modelled.
 need(known*100>=active*67,"MAP-only known coverage stays at or above the measured floor");
 // The setter variant resolves in the record/id space. With the user's own
 // China_Terrain.sg3 present, every MAP-known cell lands on a real single-cell
 // record: the folded-variant bug left 1940 of them unresolved (empty slots or
 // large composite records selected by the wrong header slot).
 const auto archivePath=std::filesystem::path(env)/"DATA"/"China_Terrain.sg3";
 if(std::filesystem::exists(archivePath)) {
     const auto archive=loadSG3(archivePath.string());
     need(archive.numImages&&archive.numImages==archive.sprites.size(),
          "original China_Terrain.sg3 loads with a consistent published image count");
     size_t knownCells=0,resolved=0,geometry=0;
     for(size_t cell=0;cell<terrain.flags.size();++cell) {
         if(terrain.flags[cell]&0x80000u)continue;
         const auto selection=selectGroundTileCode(groundCodeRequest(*state,cell));
         if(!selection.known)continue;
         ++knownCells;size_t index=0;
         if(!resolveGroundTileCode(archive,selection.code,selection.variant,index))continue;
         ++resolved;
         const auto record=sg3SingleCellGeometry(archive,index);
         if(record&&record->height<=4096)++geometry;
     }
     // Measured on the user's own Anyang.map + China_Terrain.sg3.
     need(knownCells==6690&&resolved==6690&&geometry==6690,
          "every MAP-known ground cell resolves to a real single-cell China_Terrain.sg3 record");
     // The stored surface-image grid (0x00FE9880) is the oracle for the chain. It
     // renders verbatim as recordId = low14 + 201 in the same archive; the chain
     // score then measures the reconstructed dispatch chain against it.
     const auto elevationPath=std::filesystem::path(env)/"DATA"/"China_Elevation.sg3";
     if(std::filesystem::exists(elevationPath)) {
         const auto elevation=loadSG3(elevationPath.string());
         need(elevation.numImages&&elevation.numImages==elevation.sprites.size(),
              "original China_Elevation.sg3 loads with a consistent published image count");
         auto internal=[](const std::filesystem::path& path){auto p=path;p.replace_extension(".555");return p.string();};
         std::vector<MapImageGridTileset> tilesets{{3,&archive,internal(archivePath),201},
             {16,&elevation,internal(elevationPath),201}};
         OriginalMapCamera camera;camera.centerX=69.5;camera.centerY=69.5;
         camera.width=1200;camera.height=900;camera.scale=1;
         const auto grid=renderMapImageGridViewport(terrain,*state,tilesets,camera);
         need(grid.image.valid()&&grid.stats.activeCells==9940&&grid.stats.zeroCells==0&&
              grid.stats.nonzeroCells==9940&&grid.stats.archiveUnavailable==0&&
              grid.stats.unresolvedIndices==0&&grid.stats.geometryUnsupported==0&&grid.stats.decodedCells==9940&&grid.stats.compositeBlocks==1&&grid.stats.compositeConfirmedCells==4&&
              grid.stats.decodeFailures==0,
              "original Anyang.map stored image grid decodes with the measured low-14 base");
         const auto score=scoreGroundChainAgainstImageGrid(terrain,*state,archive);
         need(score.activeCells==9940&&score.emitted==6690&&score.matched==5673&&
              score.mismatched==1017&&score.chainNoWrite==167&&score.chainUnknown==3083&&
              score.chainNoWriteVsStoredZero==0&&score.chainNoWriteVsStoredNonzero==167,
              "original Anyang.map chain score against the stored grid is stable");
     }
     // Branch 10 (MP48.map) exercises the new feature mapping: 331 active cells,
     // classes feat&8=100, feat&0x10=75, feat&0x20=100, feat&0x40=50 and six
     // feat-clear cells left by a neighbouring 5x5 block (stored-snapshot artefact).
     const auto mp48City=std::filesystem::path(env)/"Cities"/"MP48.map";
     if(std::filesystem::exists(mp48City)) {
         const auto mp48=loadOriginalMapTerrain(mp48City.string());
         const auto mp48State=loadOriginalTerrainState(mp48City.string());
         need(mp48.status==OriginalMapStatus::LoadedTerrain&&mp48State&&mp48State->valid()&&
              mp48State->terrain==mp48.terrain.flags,"original MP48.map publishes terrain and the loader state");
         size_t class8=0,class10=0,class20=0,class40=0,classClear=0,branch10=0;
         for(size_t cell=0;cell<mp48.terrain.flags.size();++cell) {
             if(mp48.terrain.flags[cell]&0x80000u)continue;
             const auto selection=selectGroundTileCode(groundCodeRequest(*mp48State,cell));
             if(std::string(selection.branch)!="0x2000000-feature")continue;
             ++branch10;
             need(selection.known&&selection.block&&selection.variant==0&&!selection.chainNoWrite,
                  "every MP48 branch-10 cell is the 5x5 feature block");
             const uint32_t feature=mp48State->feature[cell]&0xFFu;
             if(feature&8u){need(selection.code==uint16_t(0x60D),"feat&8 selects 0x60D");++class8;}
             else if(feature&0x10u){need(selection.code==uint16_t(0x60A),"feat&0x10 selects 0x60A");++class10;}
             else if(feature&0x20u){need(selection.code==uint16_t(0x609),"feat&0x20 selects 0x609");++class20;}
             else if(feature&0x40u){need(selection.code==uint16_t(0x619),"feat&0x40 selects 0x619");++class40;}
             else ++classClear;
         }
         need(branch10==331&&class8==100&&class10==75&&class20==100&&class40==50&&classClear==6,
              "original MP48.map branch-10 feature-class counts are stable");
         // The four base codes resolve to distinct archive records that are
         // consecutive in the executable's forwarded order 8,0x10,0x20,0x40. The
         // stored snapshot differs by +0/+1/+2/+3 across the four classes, which is
         // the same relative ordering in the in-exe header table.
         size_t i8=0,i10=0,i20=0,i40=0;
         need(resolveGroundTileCode(archive,uint16_t(0x60D),i8)&&resolveGroundTileCode(archive,uint16_t(0x60A),i10)&&
              resolveGroundTileCode(archive,uint16_t(0x609),i20)&&resolveGroundTileCode(archive,uint16_t(0x619),i40),
              "the four branch-10 base codes resolve in the published ground archive");
         need(i10==i8+1&&i20==i8+2&&i40==i8+3,
              "the four feature classes map to consecutive archive records in the executable's order");
         need(!sg3SingleCellGeometry(archive,i8),
              "branch-10 records are composite 5x5 art, not single-cell geometry, and are not expanded here");
     }
 }
 originalData=true;
}
void compositeSafetyTests(const std::filesystem::path& root) {
 using namespace game_lab;
 OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);
 auto at=[](unsigned x,unsigned y){return size_t(y+72)*228+x+72;};
 for(unsigned y=0;y<84;++y)for(unsigned x=0;x<84;++x)terrain.flags[at(x,y)]=0;
 SG3Sprite two;two.width=158;two.height=80;two.type=30;two.tileSize=2;two.length=two.uncompressedLength=12800;
 auto three=two;three.width=238;three.height=120;three.tileSize=3;three.length=three.uncompressedLength=28800;
 SG3Archive mixed;mixed.numImages=2;mixed.sprites={two,three};
 OriginalMapCamera camera;camera.centerX=camera.centerY=10;camera.width=640;camera.height=480;
 for(bool largeFirst:{false,true}) {
  auto state=terrainState(terrain);const unsigned offset=largeFirst?1:2;
  state.image[at(10,10)]=(3u<<14)|(largeFirst?1u:0u);state.subTile[at(10,10)]=largeFirst?2:1;
  state.image[at(10+offset,10+offset)]=(3u<<14)|(largeFirst?0u:1u);
  state.subTile[at(10+offset,10+offset)]=largeFirst?1:2;
  state.draw[at(10+offset,10+offset)]=uint8_t(offset*8+offset);
  const auto out=renderMapImageGridViewport(terrain,state,{{3,&mixed,"",1}},camera);
  need(out.image.valid()&&out.stats.nonzeroCells==2&&out.stats.geometryUnsupported==2,"mixed-size common anchors reject every member in both orders");
  need(out.stats.decodedCells==0&&out.stats.decodeFailures==0&&out.stats.compositeBlocks==0,"conflicting dimensions stop before indexing or decoding");
 }
 for(unsigned cols:{1u,2u}) {
  SG3Sprite sprite=two;sprite.width=int16_t(cols*80-2);sprite.height=int16_t(cols*40);sprite.tileSize=uint8_t(cols);sprite.length=sprite.uncompressedLength=cols*cols*3200;
  SG3Archive archive;archive.numImages=1;archive.sprites={sprite};auto state=terrainState(terrain);
  for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x){auto cell=at(10+x,10+y);state.image[cell]=3u<<14;state.subTile[cell]=uint8_t(cols-1);state.draw[cell]=uint8_t(y*8+x);}
  const auto truncated=root/("truncated-surface-"+std::to_string(cols)+".555");write(truncated,{0,0});
  const auto missing=root/("missing-surface-"+std::to_string(cols)+".555");
  const auto valid=root/("valid-surface-"+std::to_string(cols)+".555");Bytes data(sprite.length);for(size_t i=0;i<data.size();i+=2)put(data,i,0x03e0,2);write(valid,data);
  for(bool offscreen:{false,true}) {
   auto c=camera;if(offscreen){c.centerX=c.centerY=70;c.width=c.height=1;}
   for(const auto& input:{missing,truncated}) {
    const auto out=renderMapImageGridViewport(terrain,state,{{3,&archive,input.string(),1}},c);const auto& s=out.stats;
    need(out.image.valid()&&s.nonzeroCells==4&&s.decodedCells==0&&s.decodeFailures==4,"missing/truncated pixels fail before culling for single and composite sources");
    need(s.decodedCells+s.decodeFailures+s.geometryUnsupported+s.archiveUnavailable+s.unresolvedIndices==s.nonzeroCells,"stored nonzero accounting is exclusive");
    need(s.drawnCells==0&&std::all_of(out.image.pixelsBGRA.begin(),out.image.pixelsBGRA.end(),[](uint32_t p){return p==0;}),"blank canvas does not prove successful decode");
   }
   const auto good=renderMapImageGridViewport(terrain,state,{{3,&archive,valid.string(),1}},c);
   need(good.stats.decodedCells==4&&good.stats.decodeFailures==0&&good.stats.uniqueDecoded==1,"valid resources decode once even when entirely offscreen");
   need(offscreen?good.stats.culled==4&&good.stats.drawnCells==0:good.stats.culled==0&&good.stats.drawnCells==4,"culling counts successfully decoded cells only");
  }
 }
}
int main(){try{
 for(unsigned width:{1u,2u,4u}){auto b=body(512+51984u*width);const uint32_t maximum=width==4?UINT32_MAX:(1u<<(8*width))-1;put(b,512+227*width,maximum,width);put(b,512+228*width,maximum-1,width);put(b,512+51983*width,maximum-2,width);auto l=game_lab::parseOriginalMapRawLayer(b,{512,width});need(l&&l->valid()&&l->values[227]==maximum&&l->values[228]==maximum-1&&l->values.back()==maximum-2,"raw unsigned LE row boundaries/end");b.pop_back();need(!game_lab::parseOriginalMapRawLayer(b,{512,width}),"raw final byte truncation");}
 auto b=body(512+51984*4);need(!game_lab::parseOriginalMapRawLayer(b,{512,3}),"unsupported raw width");need(!game_lab::parseOriginalMapRawLayer(b,{std::numeric_limits<size_t>::max(),4}),"offset overflow before allocation");b[0]=2;need(!game_lab::parseOriginalMapRawLayer(b,{512,4}),"unsupported role");b[0]=5;put(b,84,0);need(!game_lab::parseOriginalMapRawLayer(b,{512,4}),"invalid side");
 auto root=std::filesystem::temp_directory_path()/("reference-own-"+std::to_string(getpid()));std::filesystem::create_directory(root);b=body(469391);auto container=framed(b);write(root/"own.map",container);auto loaded=game_lab::loadOriginalMapTerrain((root/"own.map").string(),game_lab::OriginalMapRawSpec{512,4});need(loaded.status==game_lab::OriginalMapStatus::LoadedTerrain&&loaded.diagnosticRawLayer,"raw published after full framing");container.back()^=1;write(root/"own.map",container);loaded=game_lab::loadOriginalMapTerrain((root/"own.map").string(),game_lab::OriginalMapRawSpec{512,4});need(loaded.status==game_lab::OriginalMapStatus::InvalidContainer&&!loaded.diagnosticRawLayer,"late corruption no stale raw layer");
 constexpr size_t table=40680;Bytes index(table+3*64);put(index,0,uint32_t(index.size()));put(index,4,213);put(index,12,20);put(index,16,2);put(index,20,1);index[680]='o';index[681]=0;size_t p=table+64;put(index,p,0);put(index,p+4,3200);put(index,p+8,3200);put(index,p+20,78,2);put(index,p+22,40,2);put(index,p+50,30,2);index[p+55]=1;p+=64;put(index,p,3200);put(index,p+4,2);put(index,p+20,1,2);put(index,p+22,1,2);put(index,p+50,1,2);Bytes pixels(3202);for(size_t i=0;i<1600;++i)put(pixels,i*2,0x7c00,2);write(root/"own.sg3",index);write(root/"own.555",pixels);auto archive=game_lab::loadSG3((root/"own.sg3").string());need(archive.sprites.size()==2,"authored SG metadata");
 game_lab::OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);game_lab::OriginalMapRawLayer raw;raw.spec={512,4};raw.values.assign(51984,0);auto cell=[](unsigned x,unsigned y){return size_t(y+72)*228+x+72;};for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x)terrain.flags[cell(x,y)]=0;raw.values[cell(0,0)]=1001;raw.values[cell(1,0)]=1001;raw.values[cell(0,1)]=1002;raw.values[cell(1,1)]=1003;
 game_lab::OriginalMapCamera cam;cam.centerX=.5;cam.centerY=.5;cam.width=300;cam.height=200;cam.scale=1;
 auto empty=game_lab::renderMapReferenceViewport(terrain,raw,{},"",1000,cam);need(!empty.image.valid()&&empty.stats.activeCells==0&&empty.stats.outsideArchive==0,"empty archive refuses before domain counting");
 write(root/"invalid.sg3",Bytes(8,0));auto invalid=game_lab::loadSG3((root/"invalid.sg3").string());need(!game_lab::renderMapReferenceViewport(terrain,raw,invalid,"",1000,cam).image.valid(),"damaged SG3 cannot publish native candidate");
 auto mismatch=archive;mismatch.sprites.pop_back();need(!game_lab::renderMapReferenceViewport(terrain,raw,mismatch,"",1000,cam).image.valid(),"archive count mismatch refused");
 auto outside=game_lab::renderMapReferenceViewport(terrain,raw,archive,(root/"own.555").string(),99999,cam);need(outside.image.valid()&&outside.stats.activeCells==4&&outside.stats.outsideArchive==4&&outside.stats.validIDs==0&&outside.stats.drawnCells==0,"valid archive all-outside remains a diagnostic");
 auto render=game_lab::renderMapReferenceViewport(terrain,raw,archive,(root/"own.555").string(),1000,cam);need(render.image.valid()&&render.experimental,"bounded candidate viewport");need(render.stats.activeCells==4&&render.stats.validIDs==3&&render.stats.geometrySupported==2&&render.stats.unknownGeometry==1&&render.stats.outsideArchive==1,"reference domains/geometry kept distinct");need(render.stats.uniqueDecoded==1&&render.stats.drawnCells==2&&render.stats.visibleCells==2,"unique decode cache and cell uses");size_t opaque=0;for(auto c:render.image.pixelsBGRA)opaque+=bool(c>>24);need(opaque==3200,"shared canvas has only two authored diamonds");need(render.image.pixelsBGRA[80*300+150]==0xffff0000u,"unsigned delta and projected natural pixel");
 auto missing=game_lab::renderMapReferenceViewport(terrain,raw,archive,(root/"missing.555").string(),1000,cam);need(missing.stats.decodeFailures==2&&missing.stats.drawnCells==0,"missing source never drawn");need(!game_lab::renderMapReferenceViewport(terrain,raw,archive,"",INT64_MAX,cam).image.valid(),"signed base bounded before arithmetic");cam.centerX=std::numeric_limits<double>::quiet_NaN();need(!game_lab::renderMapReferenceViewport(terrain,raw,archive,"",1000,cam).image.valid(),"invalid camera clears output");cam.centerX=.5;cam.width=2049;need(!game_lab::renderMapReferenceViewport(terrain,raw,archive,"",1000,cam).image.valid(),"viewport budget");
 diagnosticSceneTests(root);
 originalTerrainTests(root);
 registeredCellTests();
 outputPathTests(root);
 exclusiveOutputTests(root);
 groundSelectorTests();
 imageGridTests(root);
 compositeSafetyTests(root);
 chainScoreTests();
 bool originalData=false;
 realDataGroundTests(originalData);
 std::filesystem::remove_all(root);std::cout<<"{\"passed\":true,\"assertions\":"<<checks<<",\"original_data\":"<<(originalData?"true":"false")<<"}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
