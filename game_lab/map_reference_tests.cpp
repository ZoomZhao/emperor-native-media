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
 // determinate no-ground, counted apart from unknown.
 for(uint32_t flagged:{0x8u,0x4000u,0x8000u,0x80000u}) {
     const auto gated=selectGroundTileCode(GroundCodeRequest{flagged,0});
     need(gated.noGround&&!gated.known&&gated.code==0&&std::string(gated.branch)=="no-ground","the 0x8C008 gate skips every setter with no img write: determinate no-ground");
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
     need(!delegated.known&&!delegated.noGround&&delegated.code==0&&std::string(delegated.branch)==item.second,"runtime-dependent setter names its branch without inventing a code");
 }
 // Setter 4 shore: exactly one sub-case is fixed by MAP terrain + neighbours alone.
 {
     auto request=GroundCodeRequest{0x4000004u,0x0Au};request.hasNeighbours=true;
     for(uint32_t& flag:request.neighbourFlags)flag=0x4000000u;
     const auto shore=selectGroundTileCode(request);
     need(shore.known&&!shore.block&&!shore.noGround&&shore.code==uint16_t(0x61C)&&
          shore.variant==uint8_t(0x30u+(0x0Au%0x18u))&&
          std::string(shore.branch)=="0x4-shore-water-ring","the MAP-only shore ring writes T(0x61C)+0x30+random%0x18 with the offset kept out of the code");
     // A single neighbour without 0x4000000 leaves the value undetermined.
     request.neighbourFlags[3]&=~0x4000000u;
     const auto partial=selectGroundTileCode(request);
     need(!partial.known&&!partial.noGround&&std::string(partial.branch)=="0x4-shore","a shore cell with a neighbour lacking 0x4000000 stays unknown");
     // A neighbour carrying 0x10000 diverts to the runtime DAT_0101C540 arm.
     request.neighbourFlags[3]=0x10000u;
     need(!selectGroundTileCode(request).known,"a 0x10000 neighbour keeps the shore branch unknown");
     // hasNeighbours=false (no MAP neighbour data) must never be treated as all-zero.
     const auto noNeighbours=selectGroundTileCode(GroundCodeRequest{0x4000004u,0x0Au});
     need(!noNeighbours.known&&!noNeighbours.noGround&&std::string(noNeighbours.branch)=="0x4-shore","a shore request without neighbour data never guesses");
 }
 // Setter 5 road refuses the whole cell when a 0xC064 bit survives to it: a
 // determinate no-ground, not an unknown code.
 for(uint32_t flag:{0x1u|0x20u,0x1u|0x40u}) {
     const auto refused=selectGroundTileCode(GroundCodeRequest{flag,0x07u});
     need(refused.noGround&&!refused.known&&refused.code==0&&std::string(refused.branch)=="no-ground","a 0xC064 bit at setter 5 is a determinate no-ground");
 }
 // Setter 15 canal writes T(0x620)+random%9 from MAP bytes alone.
 for(uint8_t random:{0u,1u,8u,9u}) {
     const auto canal=selectGroundTileCode(GroundCodeRequest{0x40000u,random});
     need(canal.known&&!canal.block&&!canal.noGround&&canal.code==uint16_t(0x620)&&canal.variant==uint8_t(random%9u)&&std::string(canal.branch)=="0x40000-canal","terrain&0x40000 selects the pure 0x620 base with variant random%9");
 }
 // 0x20 survives to setter 15 and makes FUN_004BC800 refuse: determinate no ground.
 const auto canalRefused=selectGroundTileCode(GroundCodeRequest{0x40020u,0x05u});
 need(canalRefused.noGround&&!canalRefused.known&&std::string(canalRefused.branch)=="no-ground","terrain&0x20 makes the canal setter refuse without writing");
 // The final fall-through is determinate no ground: every setter ran, only
 // FUN_0053F090 was left and terrain&0xAEFEDE6F was nonzero.
 const auto exhausted=selectGroundTileCode(GroundCodeRequest{0x200u,0});
 need(exhausted.noGround&&!exhausted.known&&std::string(exhausted.branch)=="no-ground","a leftover 0xAEFEDE6F bit is a determinate no-ground");
 // Setter 17: terrain&0x80 with (terrain&0x10001)==0 is the farm plot. A MAP-only
 // load carries no runtime fertility, so the value is T(0x602)+random%9.
 for(const auto& item:std::vector<std::pair<uint8_t,uint8_t>>{{0u,0u},{1u,1u},{0x0Au,1u},{0x08u,8u},{0xF8u,5u},{0xFFu,3u}}) {
     const auto farm=selectGroundTileCode(GroundCodeRequest{0x80u,item.first});
     need(farm.known&&!farm.block&&farm.code==uint16_t(0x602)&&farm.variant==item.second&&std::string(farm.branch)=="0x80-farm","terrain&0x80 selects the pure 0x602 farm base with variant random%9");
 }
 // Setters ahead of 17 still win: road (0x1) and monument (0x10000).
 const auto roadFarm=selectGroundTileCode(GroundCodeRequest{0x81u,0x03u});
 need(!roadFarm.known&&std::string(roadFarm.branch)=="0x1-road","terrain&0x1 outranks the 0x80 farm branch");
 const auto monumentFarm=selectGroundTileCode(GroundCodeRequest{0x10080u,0x03u});
 need(!monumentFarm.known&&std::string(monumentFarm.branch)=="0x10000-monument","terrain&0x10000 keeps a farm bit out of branch 17");
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
 // The viewport needs exactly the parsed 228x228 random grid.
 OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0;
 OriginalMapCamera camera;camera.centerX=41.5;camera.centerY=41.5;camera.width=64;camera.height=48;camera.scale=1;
 need(!renderMapGroundViewport(terrain,std::vector<uint8_t>(51983,0),archive,"",camera).image.valid(),"random grid smaller than 228x228 refuses");
 need(!renderMapGroundViewport(terrain,std::vector<uint8_t>(51985,0),archive,"",camera).image.valid(),"random grid larger than 228x228 refuses");
 need(!renderMapGroundViewport(terrain,std::vector<uint8_t>(51984,0),{},"",camera).image.valid(),"empty archive refuses before counting");
 // Plain land resolves and counts; unsupported geometry stays explicit, never drawn.
 archive.sprites[0].type=30;archive.sprites[0].width=78;archive.sprites[0].height=40;
 archive.sprites[0].uncompressedLength=3200;archive.sprites[0].length=3200;
 const auto plain=renderMapGroundViewport(terrain,std::vector<uint8_t>(51984,0),archive,"",camera);
 need(plain.image.valid()&&plain.experimental,"ground selector viewport is a bounded candidate");
 need(plain.stats.activeCells==84*84&&plain.stats.knownCells==84*84&&plain.stats.unknownCells==0,"plain land is fully known");
 need(plain.stats.blockCells==84*84&&plain.stats.unresolvedCodes==0&&plain.stats.geometrySupported==84*84,"block flag and supported geometry counted");
 need(plain.stats.branchCells.size()==1&&plain.stats.branchCells[0].first=="plain-land"&&plain.stats.branchCells[0].second==84*84,"branch histogram names plain-land");
 // Gate-failed terrain is a determinate no-ground (setter 1 provably wrote
 // nothing), counted apart from unknown and never drawn.
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0x8;
 const auto gated=renderMapGroundViewport(terrain,std::vector<uint8_t>(51984,0),archive,"",camera);
 need(gated.image.valid()&&gated.stats.activeCells==84*84&&gated.stats.noGroundCells==84*84&&gated.stats.knownCells==0&&gated.stats.unknownCells==0&&gated.stats.drawnCells==0,"gate-failed cells are a determinate no-ground and never drawn");
 need(gated.stats.branchCells.size()==1&&gated.stats.branchCells[0].first=="no-ground","gate-failed cells name the no-ground branch");
 // A determinate no-ground cell is counted apart from unknown, never guessed, and
 // still names its branch.
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0x200;
 const auto noGround=renderMapGroundViewport(terrain,std::vector<uint8_t>(51984,0),archive,"",camera);
 need(noGround.image.valid()&&noGround.stats.activeCells==84*84&&noGround.stats.noGroundCells==84*84&&
      noGround.stats.knownCells==0&&noGround.stats.unknownCells==0&&noGround.stats.drawnCells==0,"determinate no-ground cells are counted apart from unknown and never drawn");
 need(noGround.stats.branchCells.size()==1&&noGround.stats.branchCells[0].first=="no-ground","no-ground cells name the no-ground branch");
 // The canal setter resolves from terrain flags alone: every cell is known.
 for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[size_t(y)*228+x]=0x40000;
 const auto canal=renderMapGroundViewport(terrain,std::vector<uint8_t>(51984,0),archive,"",camera);
 need(canal.image.valid()&&canal.stats.knownCells==84*84&&canal.stats.unknownCells==0&&canal.stats.noGroundCells==0,"terrain&0x40000 is known without any runtime state");
 need(canal.stats.branchCells.size()==1&&canal.stats.branchCells[0].first=="0x40000-canal","canal cells name the canal branch");
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
 need(loaded.status==OriginalMapStatus::LoadedTerrain&&loaded.cellLayers.has_value(),"original Anyang.map loads terrain and mapper cell layers");
 const auto& terrain=loaded.terrain;const auto& random=loaded.cellLayers->mapperRandom;
 need(terrain.valid()&&random.size()==OriginalMapCellLayers::cells,"original Anyang.map publishes the 228x228 grids");
 size_t active=0,known=0,unknown=0,noGround=0,farm=0,farmKnown=0;
 for(size_t cell=0;cell<terrain.flags.size();++cell) {
     if(terrain.flags[cell]&0x80000u)continue;
     ++active;
     const auto selection=selectGroundTileCode(groundCodeRequest(terrain.flags,random,cell));
     if(selection.noGround)++noGround;else if(selection.known)++known;else ++unknown;
     if(std::string(selection.branch)=="0x80-farm") {
         ++farm;
         if(selection.known&&!selection.block&&selection.code==uint16_t(0x602)&&selection.variant==uint8_t(random[cell]%9u))++farmKnown;
     }
 }
 // Measured on the user's own Anyang.map: 9940 active cells, 6212 farmland cells.
 need(active==9940&&farm==6212,"original Anyang.map active and farmland cell counts are stable");
 // Branch 17 is fully resolved from MAP terrain + mapperRandom alone: every
 // farmland cell is the 0x602 plot with the random%9 variant (>=90% of its domain).
 need(farmKnown==farm&&farmKnown*10>=farm*9,"the 0x80 farm branch resolves at least 90% of its own domain from MAP bytes alone");
 // The 0x4 shore-ring sub-case (384 cells) is now MAP-determinable, so the three
 // outcomes partition the active set: 6910 known, 2863 still runtime-unknown
 // (0x1-road / 0x4-shore / 0x40-road / 0x2000000 ...) and 167 a determinate
 // no-ground. Measured on the user's own Anyang.map.
 need(known==6910&&unknown==2863&&noGround==167&&known+unknown+noGround==active,"original Anyang.map MAP-only outcome counts are stable");
 // Overall MAP-only known coverage floor (~69.5%). It cannot reach the farmland
 // level until the 0x1-road and 0x4-shore general arms are modelled.
 need(known*100>=active*69,"MAP-only known coverage stays at or above the measured floor");
 // The setter variant resolves in the record/id space. With the user's own
 // China_Terrain.sg3 present, every MAP-known cell lands on a real single-cell
 // record: the folded-variant bug left 1940 of them unresolved (empty slots or
 // large composite records selected by the wrong header slot).
 const auto archivePath=std::filesystem::path(env)/"DATA"/"China_Terrain.sg3";
 if(std::filesystem::exists(archivePath)) {
     const auto archive=loadSG3(archivePath.string());
     need(archive.numImages&&archive.numImages==archive.sprites.size(),"original China_Terrain.sg3 loads with a consistent published image count");
     size_t knownCells=0,resolved=0,geometry=0;
     for(size_t cell=0;cell<terrain.flags.size();++cell) {
         if(terrain.flags[cell]&0x80000u)continue;
         const auto selection=selectGroundTileCode(groundCodeRequest(terrain.flags,random,cell));
         if(!selection.known)continue;
         ++knownCells;size_t index=0;
         if(!resolveGroundTileCode(archive,selection.code,selection.variant,index))continue;
         ++resolved;
         const auto record=sg3SingleCellGeometry(archive,index);
         if(record&&record->height<=4096)++geometry;
     }
     // Measured on the user's own Anyang.map + China_Terrain.sg3 after the fix.
     need(knownCells==6910&&resolved==6910&&geometry==6910,"every MAP-known ground cell resolves to a real single-cell China_Terrain.sg3 record");
     need(resolved*100>=knownCells*99,"MAP-known ground resolution stays at or above the measured floor");
 }
 originalData=true;
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
 registeredCellTests();
 outputPathTests(root);
 exclusiveOutputTests(root);
 groundSelectorTests();
 bool originalData=false;
 realDataGroundTests(originalData);
 std::filesystem::remove_all(root);std::cout<<"{\"passed\":true,\"assertions\":"<<checks<<",\"original_data\":"<<(originalData?"true":"false")<<"}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
