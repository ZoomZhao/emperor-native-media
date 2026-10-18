// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include "../cg_image_conversion.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <zlib.h>
namespace {
using Bytes=std::vector<uint8_t>;
size_t checks=0;
void need(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void put(Bytes& b,size_t p,uint32_t n,unsigned width=4){for(unsigned i=0;i<width;++i)b.at(p+i)=uint8_t(n>>(8*i));}
void write(const std::filesystem::path& p,const Bytes& b){std::filesystem::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)throw std::runtime_error("owned fixture write");}
Bytes framed(const Bytes& b){Bytes out(4);put(out,0,0xfedcbaaa);for(size_t p=0;p<b.size();p+=32768){auto n=std::min(size_t(32768),b.size()-p);Bytes z(compressBound(n));uLongf length=z.size();if(compress2(z.data(),&length,b.data()+p,n,9)!=Z_OK)throw std::runtime_error("fixture compression");size_t q=out.size();out.resize(q+12+length);put(out,q,0xabcdefff);put(out,q+4,uint32_t(length));put(out,q+8,uint32_t(n));std::copy(z.begin(),z.begin()+length,out.begin()+q+12);}return out;}
void fixture(const std::filesystem::path& root){
    using game_lab::OriginalTerrainState;
    Bytes map(OriginalTerrainState::flatEnd);map[0]=5;map[2]=254;map[3]=202;map[6]=2;put(map,84,84);
    for(size_t i=0;i<51984;++i)put(map,OriginalTerrainState::terrainOffset+i*4,0x80000);
    const size_t cell=114*228+114;put(map,OriginalTerrainState::terrainOffset+cell*4,128);put(map,OriginalTerrainState::imageOffset+cell*4,3u<<14);
    write(root/"Cities/own.map",framed(map));
    constexpr size_t table=40680;Bytes sg(table+203*64);put(sg,0,uint32_t(sg.size()));put(sg,4,213);put(sg,12,203);put(sg,16,202);
    const size_t record=table+201*64;put(sg,record+4,3200);put(sg,record+8,3200);put(sg,record+20,78,2);put(sg,record+22,40,2);put(sg,record+50,30,2);sg[record+55]=1;
    write(root/"DATA/China_Terrain.sg3",sg);Bytes pixels(3200);for(size_t p=0;p<pixels.size();p+=2)put(pixels,p,0x03e0,2);write(root/"DATA/China_Terrain.555",pixels);
}
void authored(const std::filesystem::path& root){
    using namespace game_lab;fixture(root);const auto configured=ResourceRoot::fromAbsolute(root.string());AppMapSurface app(configured);
    auto map=loadOriginalMapTerrain((root/"Cities/own.map").string());need(map.status==OriginalMapStatus::LoadedTerrain,"owned MAP parses");
    need(app.select("Cities/own.map",map.terrain),"App selects same complete MAP");OriginalMapCamera c;c.centerX=c.centerY=42;c.width=256;c.height=192;
    auto image=app.render(c);need(image.image.valid()&&image.stats.nonzeroCells==1&&image.stats.decodedCells==1,"stored surface is rendered rather than numerical flags");
    need(std::count(image.image.pixelsBGRA.begin(),image.image.pixelsBGRA.end(),0xff00ff00u)==1600,"owned RGB555 art reaches exact native pixels");
    auto cg=createCGImageFromPremultipliedBGRA(image.image.width,image.image.height,image.image.pixelsBGRA);need(cg!=nullptr,"App CGImage creation");if(cg)CGImageRelease(cg);
    need(!app.select("../outside.map",map.terrain)&&!app.available()&&!app.render(c).image.valid(),"unsafe selection clears previous surface");
    need(app.select("Cities/own.map",map.terrain),"selection recovers after failure");auto wrong=map.terrain;wrong.activeSide=112;
    need(!app.select("Cities/own.map",wrong)&&!app.available(),"different crop cannot reuse old surface");wrong=map.terrain;wrong.flags[0]^=1;
    need(!app.select("Cities/own.map",wrong),"different field snapshot refused");
    AppMapSurface missing(configured);std::filesystem::rename(root/"DATA",root/"assets-absent");need(missing.select("Cities/own.map",map.terrain),"missing assets keep measured unknown surface");
    auto unknown=missing.render(c);need(unknown.image.valid()&&unknown.stats.archiveUnavailable==1&&unknown.stats.decodedCells==0,"missing archive is counted,not substituted");
    need(std::all_of(unknown.image.pixelsBGRA.begin(),unknown.image.pixelsBGRA.end(),[](uint32_t p){return p==0;}),"unknown surface remains transparent");
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"original_data\":false}\n";
}
void metrics(const std::string& path){
    using namespace game_lab;namespace fs=std::filesystem;const auto root=ResourceRoot::fromAbsolute(path);if(!root.configured())throw std::runtime_error("explicit existing data root required");
    AppMapSurface app(root);size_t cities=0,complete=0,images=0,nonzero=0,decoded=0,residue=0,excluded=0;
    std::string firstPartial;
    for(const auto& file:fs::directory_iterator(fs::path(root.configuredDirectory())/"Cities")){
        if(file.path().extension()!=".map")continue;
        const auto relative=fs::relative(file.path(),root.configuredDirectory()).generic_string();const auto absolute=root.resolve(relative);if(absolute.empty())throw std::runtime_error("MAP escapes root");
        auto map=loadOriginalMapTerrain(absolute);if(map.status==OriginalMapStatus::UnsupportedRole){++excluded;continue;}
        if(map.status!=OriginalMapStatus::LoadedTerrain||!app.select(relative,map.terrain))throw std::runtime_error("App MAP selection failed: "+relative);
        OriginalMapCamera c;c.centerX=c.centerY=double(map.terrain.activeSide)/2;c.width=256;c.height=192;
        auto out=app.render(c);if(!out.image.valid())throw std::runtime_error("App surface render failed: "+relative);
        auto cg=createCGImageFromPremultipliedBGRA(out.image.width,out.image.height,out.image.pixelsBGRA);if(!cg)throw std::runtime_error("CGImage failed: "+relative);CGImageRelease(cg);++images;++cities;
        const auto& s=out.stats;nonzero+=s.nonzeroCells;decoded+=s.decodedCells;residue+=s.geometryUnsupported+s.unresolvedIndices+s.archiveUnavailable+s.decodeFailures;
        if(s.decodedCells+s.geometryUnsupported+s.unresolvedIndices+s.archiveUnavailable+s.decodeFailures!=s.nonzeroCells)throw std::runtime_error("surface accounting is not exclusive: "+relative);
        if(s.decodedCells<s.nonzeroCells&&firstPartial.empty())firstPartial=relative;
        if(s.decodedCells==s.nonzeroCells&&!s.geometryUnsupported&&!s.unresolvedIndices&&!s.archiveUnavailable&&!s.decodeFailures)++complete;
    }
    if(!cities)throw std::runtime_error("no city MAP corpus");
    std::cout<<"{\"passed\":true,\"App_surface_sources\":"<<cities<<",\"native_CGImages\":"<<images<<",\"complete_storage_surfaces\":"<<complete<<",\"nonzero_cells\":"<<nonzero<<",\"decoded_cells\":"<<decoded<<",\"residue\":"<<residue<<",\"excluded_non_city\":"<<excluded<<",\"first_partial_source\":"<<std::quoted(firstPartial)<<",\"selector_accuracy_measured\":false,\"original_screen_accuracy_measured\":false}\n";
}
}
int main(int argc,char** argv){try{if(argc==2&&std::string(argv[1])=="--metrics"){const char* root=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!root||!*root){std::cout<<"{\"skipped\":true}\n";return 0;}metrics(root);}
    else if(argc==2)authored(std::filesystem::absolute(argv[1]));else throw std::runtime_error("fixture directory or --metrics required");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
