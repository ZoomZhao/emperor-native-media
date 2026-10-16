#include "map_reference.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <limits>
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
 outputPathTests(root);
 std::filesystem::remove_all(root);std::cout<<"{\"passed\":true,\"assertions\":"<<checks<<",\"original_data\":false}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
