#include "original_map.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <zlib.h>
namespace {
using Bytes=std::vector<uint8_t>;size_t checks=0;
void need(bool b,const char* text){++checks;if(!b)throw std::runtime_error(text);}
void append(Bytes& b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
void put(Bytes& b,size_t p,uint32_t n){for(int i=0;i<4;++i)b[p+i]=uint8_t(n>>(i*8));}
void write(const std::filesystem::path& p,const Bytes& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());need(bool(f),"self fixture write");}
Bytes encode(const Bytes& body){Bytes result;append(result,0xfedcbaaa);
 for(size_t p=0;p<body.size();p+=32768){const size_t size=std::min(size_t(32768),body.size()-p);Bytes out(compressBound(size));uLongf n=out.size();
  need(compress2(out.data(),&n,body.data()+p,size,9)==Z_OK,"self zlib stream");out.resize(n);append(result,0xabcdefff);append(result,n);append(result,size);result.insert(result.end(),out.begin(),out.end());}
 return result;
}
}
int main(){try{
 auto root=std::filesystem::temp_directory_path()/("emperor-map-self-"+std::to_string(getpid()));need(std::filesystem::create_directory(root),"fresh own root");auto file=root/"own.map";
 Bytes body(261455+228u*228u*4+17,0);body[0]=5;body[2]=0xfe;body[3]=0xca;body[6]=2;put(body,84,140);
 for(size_t i=0;i<228u*228u;++i)put(body,261455+i*4,uint32_t(i));put(body,261455+51983u*4,0x80000001u);auto framed=encode(body);write(file,framed);
 auto loaded=game_lab::loadOriginalMapTerrain(file.string());need(loaded.status==game_lab::OriginalMapStatus::LoadedTerrain&&loaded.terrain.valid(),"complete city terrain load");
 need(loaded.terrain.activeSide==140&&loaded.terrain.flags[227]==227&&loaded.terrain.flags[228]==228&&loaded.terrain.flags.back()==0x80000001u,"active size and row-major 228 backing independent of 128 preview");
 need(loaded.decodedBytes==body.size()&&loaded.frames>1,"complete multi-chunk consumption");
 auto image=game_lab::renderOriginalMapFlags(loaded.terrain);need(image.valid()&&image.width==228&&image.height==228,"native diagnostic raster dimensions");
 auto bit=game_lab::renderOriginalMapFlags(loaded.terrain,0);need(bit.pixelsBGRA[0]!=bit.pixelsBGRA[1]&&bit.pixelsBGRA[0]==bit.pixelsBGRA[2],"bit-plane uses original values only");
 auto high=game_lab::renderOriginalMapFlags(loaded.terrain,31);need(high.pixelsBGRA.back()!=high.pixelsBGRA[0],"highest uint32 flag bit displayed without signed shift");
 need(!game_lab::renderOriginalMapFlags(loaded.terrain,32).valid(),"invalid bit plane rejected");
 framed.back()^=1;write(file,framed);auto bad=game_lab::loadOriginalMapTerrain(file.string());need(bad.status==game_lab::OriginalMapStatus::InvalidContainer&&bad.terrain.flags.empty(),"late bad checksum publishes no earlier grid");
 framed=encode(body);framed.push_back(0);write(file,framed);bad=game_lab::loadOriginalMapTerrain(file.string());need(bad.status==game_lab::OriginalMapStatus::InvalidContainer&&bad.terrain.flags.empty(),"file tail not silently ignored");
 framed=encode(body);put(framed,12,32767);write(file,framed);need(game_lab::loadOriginalMapTerrain(file.string()).status==game_lab::OriginalMapStatus::InvalidContainer,"declared decoded length checked");
 Bytes extra=encode(Bytes(body.begin(),body.begin()+32768));extra.push_back(0);put(extra,8,uint32_t(extra.size()-16));write(file,extra);
 need(game_lab::loadOriginalMapTerrain(file.string()).status==game_lab::OriginalMapStatus::InvalidContainer,"trailing bytes inside compressed record rejected");
 put(body,84,229);write(file,encode(body));need(game_lab::loadOriginalMapTerrain(file.string()).status==game_lab::OriginalMapStatus::InvalidGrid,"out of capacity active size rejected");
 body[0]=2;put(body,84,140);write(file,encode(body));need(game_lab::loadOriginalMapTerrain(file.string()).status==game_lab::OriginalMapStatus::UnsupportedRole,"empire/other header not treated as city");
 body[0]=5;body.resize(261456);write(file,encode(body));need(game_lab::loadOriginalMapTerrain(file.string()).status==game_lab::OriginalMapStatus::InvalidGrid,"partial terrain is not loaded");
 Bytes raw;append(raw,0xfedcbaaa);append(raw,0xaaabcdef);append(raw,60);raw.resize(72);write(file,raw);need(game_lab::loadOriginalMapTerrain(file.string()).status==game_lab::OriginalMapStatus::UnsupportedContainer,"unverified raw/multipart container refused");
 for(uint32_t side:{84u,112u,140u,170u,226u}){
  game_lab::OriginalMapTerrain grid;grid.activeSide=side;grid.flags.assign(228u*228u,0x80000u);
  const uint32_t origin=(228-side)/2;
  grid.flags[size_t(origin)*228+origin]=1;
  grid.flags[size_t(origin)*228+origin+side-1]=uint32_t(1)<<22;
  auto analysis=game_lab::analyzeOriginalMap(grid);
  need(analysis.cropValid()&&analysis.cropOrigin==origin&&analysis.cropSide==side&&analysis.activeCells==2,"five published crop sizes preserve all unmarked cells");
  need(analysis.unknownFlagCells==1&&analysis.unknownBits==(uint32_t(1)<<22)&&analysis.unknownBitOccurrences[22]==1,"unpublished bits explicitly counted without guessed meanings");
  auto cropped=game_lab::renderOriginalMapFlags(grid,-1,true);
  need(cropped.valid()&&cropped.width==side&&cropped.height==side&&cropped.pixelsBGRA[0]&&cropped.pixelsBGRA[side-1]&&!cropped.pixelsBGRA[side],"crop keeps corner cells and leaves off-map cells transparent");
  grid.flags[0]=0;analysis=game_lab::analyzeOriginalMap(grid);
  need(!analysis.cropValid()&&analysis.outsideCropCells==1&&!game_lab::renderOriginalMapFlags(grid,-1,true).valid(),"contradictory crop fails rather than losing real cells");
 }
 game_lab::OriginalMapTerrain other;other.activeSide=144;other.flags.assign(228u*228u,0x80000u);other.flags[114u*228u+114]=0;
 need(!game_lab::analyzeOriginalMap(other).cropSupported&&!game_lab::renderOriginalMapFlags(other,-1,true).valid()&&game_lab::renderOriginalMapFlags(other).valid(),"unpublished size remains raw diagnostic only");
 need(game_lab::originalMapFlagLabel(22).find("unknown")!=std::string::npos&&game_lab::originalMapFlagLabel(19).find("地图外")!=std::string::npos,"unknown and public bit labels distinguish evidence");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"self_authored\":true,\"original_data\":false}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}
