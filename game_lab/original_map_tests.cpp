#include "original_map.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <limits>
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
 need(!loaded.cellLayers,"terrain-only authored payload never claims complete extra layers");
 Bytes layerPayload(game_lab::OriginalMapCellLayers::mapperFertileOffset+51984,0);
 layerPayload[0]=5;layerPayload[2]=0xfe;layerPayload[3]=0xca;layerPayload[6]=2;put(layerPayload,84,140);
 for(size_t i=0;i<51984;++i){put(layerPayload,game_lab::OriginalMapCellLayers::objectsOffset+4*i,uint32_t(i)|0x80000000u);
  layerPayload[game_lab::OriginalMapCellLayers::edgesOffset+i]=uint8_t(i*3);
  layerPayload[game_lab::OriginalMapCellLayers::mapperRandomOffset+i]=uint8_t(i);
  layerPayload[game_lab::OriginalMapCellLayers::mapperFertileOffset+i]=uint8_t(255-i);}
 auto layers=game_lab::parseOriginalMapCellLayers(layerPayload);
 need(layers&&layers->valid()&&layers->objects[227]==(227u|0x80000000u)&&layers->objects[228]==(228u|0x80000000u)&&layers->objects.back()==(51983u|0x80000000u),"objects retain unsigned LE high bits and row boundaries");
 need(layers->mapperRandom[0]==0&&layers->mapperRandom[255]==255&&layers->mapperRandom[256]==0&&layers->mapperRandom.back()==uint8_t(51983),"mapper random preserves every unsigned byte and backing endpoint");
 need(layers->mapperFertile[0]==255&&layers->mapperFertile[255]==0&&layers->edges[228]==uint8_t(228*3),"mapper fertile/edges keep raw values without semantics or palette");
 auto truncatedLayers=layerPayload;truncatedLayers.pop_back();need(!game_lab::parseOriginalMapCellLayers(truncatedLayers),"final byte truncation publishes no partial layer set");
 for(int bad=0;bad<4;++bad){auto invalid=layerPayload;if(bad==0)invalid[0]=2;if(bad==1)put(invalid,84,0);if(bad==2)put(invalid,84,229);if(bad==3)invalid.resize(8u*1024u*1024u+1);need(!game_lab::parseOriginalMapCellLayers(invalid),"extra layers reject role,side and payload budget independently");}
 auto completeLayerFile=encode(layerPayload);write(root/"layers.map",completeLayerFile);auto completeLayers=game_lab::loadOriginalMapTerrain((root/"layers.map").string());
 need(completeLayers.status==game_lab::OriginalMapStatus::LoadedTerrain&&completeLayers.cellLayers&&completeLayers.cellLayers->mapperRandom==layers->mapperRandom,"complete container publishes all extra layers after framing validation");
 completeLayerFile.back()^=1;write(root/"layers.map",completeLayerFile);auto corruptLayers=game_lab::loadOriginalMapTerrain((root/"layers.map").string());
 need(corruptLayers.status==game_lab::OriginalMapStatus::InvalidContainer&&!corruptLayers.cellLayers,"late compressed corruption never exposes extra layers");
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
 // Authored flat-camera fixtures. These are not original-renderer passes.
 game_lab::OriginalMapTerrain spatial;spatial.activeSide=84;spatial.flags.assign(228u*228u,0x80000u);
 auto setCell=[&](uint32_t x,uint32_t y,uint32_t value){spatial.flags[size_t(y+72)*228+x+72]=value;};
 setCell(41,41,0xc0);setCell(41,42,0x80);setCell(42,41,0xc0);setCell(42,42,0x400080);
 game_lab::OriginalMapCamera camera;camera.centerX=camera.centerY=41.5;camera.width=160;camera.height=80;camera.outlines=false;
 auto p0=game_lab::projectOriginalMapCell(spatial,camera,41,41),px=game_lab::projectOriginalMapCell(spatial,camera,42,41),py=game_lab::projectOriginalMapCell(spatial,camera,41,42);
 need(p0&&px&&py&&p0->x==80&&p0->y==20&&px->x-p0->x==40&&px->y-p0->y==20&&py->x-p0->x==-40&&py->y-p0->y==20,"canonical cell centres and both axes come from configurable active-crop camera");
 auto spatialImage=game_lab::renderOriginalMapSpatialFlags(spatial,camera,6);size_t colored=0,roadPixels=0;
 for(uint32_t pixel:spatialImage.pixelsBGRA){colored+=bool(pixel>>24);roadPixels+=pixel==0xff54b9e8u;}
 need(spatialImage.valid()&&colored==6400&&roadPixels==3200&&spatialImage.pixelsBGRA[40*160+40]==0xff24272bu,"four published diamond masks preserve road/clear fields without overlap or invented artwork");
 auto unknownSpatial=game_lab::renderOriginalMapSpatialFlags(spatial,camera,22);
 need(unknownSpatial.valid()&&unknownSpatial.pixelsBGRA[60*160+80]==0xff54b9e8u&&unknownSpatial.pixelsBGRA[20*160+80]==0xff24272bu,"unknown high flag survives spatial projection as an explicitly unassigned bit plane");
 auto panned=camera;panned.centerX+=.5;auto moved=game_lab::projectOriginalMapCell(spatial,panned,41,41);
 need(moved&&moved->x==p0->x-20&&moved->y==p0->y-10,"camera movement is a planar translation, not a new orientation fit");
 auto doubled=camera;doubled.scale=2;doubled.width=320;doubled.height=160;auto scaledImage=game_lab::renderOriginalMapSpatialFlags(spatial,doubled,6);
 bool replicated=scaledImage.valid();for(size_t y=0;y<80&&replicated;++y)for(size_t x=0;x<160&&replicated;++x)for(size_t sy=0;sy<2;++sy)for(size_t sx=0;sx<2;++sx)
  replicated&=scaledImage.pixelsBGRA[(2*y+sy)*320+2*x+sx]==spatialImage.pixelsBGRA[y*160+x];
 need(replicated,"scale2 checks every replicated pixel and does not change source cell data");
 auto outlined=camera;outlined.outlines=true;auto outlines=game_lab::renderOriginalMapSpatialFlags(spatial,outlined,6);
 need(outlines.valid()&&outlines.pixelsBGRA!=spatialImage.pixelsBGRA&&spatial.flags[size_t(113)*228+113]==0xc0,"optional cell outlines change only authored diagnostic pixels, never MAP flags");
 // Fractional-camera raster translation across zero: every shared pixel must
 // move by the exact integer basis step, including half-pixel ties.
 game_lab::OriginalMapTerrain phaseGrid;phaseGrid.activeSide=84;phaseGrid.flags.assign(228u*228u,0x80000u);
 phaseGrid.flags[72u*228u+72]=0xc0;
 for(uint32_t scale=1;scale<=4;++scale)for(bool outlines:{false,true})for(int axis=0;axis<4;++axis) {
  game_lab::OriginalMapCamera before;before.width=before.height=80*scale;before.scale=scale;before.outlines=outlines;
  int cameraDX=0,cameraDY=0;
  if(axis==0){before.centerX=.0125;cameraDX=1;}
  if(axis==1){before.centerX=1.0125;cameraDY=1;}
  if(axis>=2){before.centerX=before.centerY=1.0125;if(axis==2)cameraDX=-1;else cameraDY=-1;}
  auto after=before;after.centerX+=cameraDX;after.centerY+=cameraDY;
  const auto first=game_lab::renderOriginalMapSpatialFlags(phaseGrid,before,6),second=game_lab::renderOriginalMapSpatialFlags(phaseGrid,after,6);
  const int dx=(cameraDY-cameraDX)*40*int(scale),dy=-(cameraDX+cameraDY)*20*int(scale);
  size_t common=0,colored=0,differences=0;
  for(int y=0;y<int(after.height);++y)for(int x=0;x<int(after.width);++x) {
   const int ax=x-dx,ay=y-dy;if(ax<0||ay<0||ax>=int(before.width)||ay>=int(before.height))continue;
   const auto oldPixel=first.pixelsBGRA[size_t(ay)*before.width+size_t(ax)],newPixel=second.pixelsBGRA[size_t(y)*after.width+size_t(x)];
   ++common;colored+=bool(oldPixel>>24);differences+=oldPixel!=newPixel;
  }
  need(first.valid()&&second.valid()&&common==2400*scale*scale&&colored>0&&differences==0,
       "fractional half-pixel raster crosses zero without phase jumps under integer camera translation");
 }
 auto clipped=camera;clipped.centerX=clipped.centerY=0;auto clippedImage=game_lab::renderOriginalMapSpatialFlags(spatial,clipped,6);
 need(clippedImage.valid()&&std::all_of(clippedImage.pixelsBGRA.begin(),clippedImage.pixelsBGRA.end(),[](uint32_t p){return p==0;}),"offscreen cells clip safely and are not wrapped to visible coordinates");
 for(int test=0;test<7;++test){auto invalidCamera=camera;
  if(test==0)invalidCamera.centerX=std::numeric_limits<double>::quiet_NaN();if(test==1)invalidCamera.centerY=-.1;
  if(test==2)invalidCamera.centerX=84;if(test==3)invalidCamera.scale=0;if(test==4)invalidCamera.scale=5;
  if(test==5)invalidCamera.width=0;if(test==6)invalidCamera.width=2049;
  need(!game_lab::renderOriginalMapSpatialFlags(spatial,invalidCamera,6).valid(),"invalid/nonfinite camera or image budget fails without stale scene");}
 need(!game_lab::projectOriginalMapCell(spatial,camera,84,41)&&!game_lab::renderOriginalMapSpatialFlags(spatial,camera,32).valid(),"out-of-grid cell and invalid spatial bit plane rejected");
 need(!game_lab::renderOriginalMapSpatialFlags(other,camera).valid(),"unverified activity crop cannot supply a spatial grid");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"self_authored\":true,\"original_data\":false}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}
