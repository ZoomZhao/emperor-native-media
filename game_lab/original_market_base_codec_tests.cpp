// SPDX-License-Identifier: MIT
#include "original_market_base_codec.hpp"
#include "original_save.hpp"
#include "original_pak.hpp"
#include "../file_io/include/city19_goal_archive.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace{size_t checks=0;void need(bool x,const char* why){++checks;if(!x)throw std::runtime_error(why);}
OriginalMarketKnownBase sample(unsigned type){OriginalMarketKnownBase b;b.sourceRevision=19;b.figure.slot=1936;b.figure.recordComplete=true;
    for(const auto& f:emperor::file_io::research::figure7_fields){uint32_t v=f.bytes==1?255:f.bytes==2?65535:f.bytes==4?UINT32_MAX:0;
        if(f.receiver_offset==18)v=type;b.figure.fields.push_back(v);}for(size_t n=0;n<20;++n)b.figure.opaque380[n]=uint8_t(n*11);return b;}
void tests(){std::string error;for(unsigned type:{24u,25u}){auto b=sample(type);auto encoded=encodeOriginalMarketKnownBase(b,error);need(bool(encoded),"full known base encodes");
    const auto decoded=decodeOriginalMarketKnownBase(*encoded,19,error);need(bool(decoded),"base decodes");need(decoded->figure.fields==b.figure.fields&&decoded->figure.opaque380==b.figure.opaque380,"all raw wire widths including enum high bytes preserved");
    need(encodeOriginalMarketKnownBase(*decoded,error)==encoded,"canonical roundtrip");
    for(size_t n=0;n<encoded->size();++n)need(!decodeOriginalMarketKnownBase(encoded->substr(0,n),19,error),"every truncation refuses");
    need(!decodeOriginalMarketKnownBase(*encoded,20,error),"source mismatch");need(!decodeOriginalMarketKnownBase(*encoded+"x\n",19,error),"trailing data");
    auto fsa=*encoded;fsa.replace(fsa.find("extension 0"),11,"extension 1");need(!decodeOriginalMarketKnownBase(fsa,19,error)&&error=="market_base_unknown_fsa","unknown FSA not skipped");
    auto v=*encoded;v.replace(v.find("BASE 1"),6,"BASE 2");need(!decodeOriginalMarketKnownBase(v,19,error),"unknown version");
    v=*encoded;v.replace(v.find("slot 1936"),9,"slot 2000");need(!decodeOriginalMarketKnownBase(v,19,error),"slot limit");
    v=*encoded;v.replace(v.find("class 0"),7,"class 1");need(!decodeOriginalMarketKnownBase(v,19,error),"unknown class");
    v=*encoded;v.replace(v.find("18 1 "),5,"19 1 ");need(!decodeOriginalMarketKnownBase(v,19,error),"duplicate reordered receiver field");
    b.figure.alias=true;need(!encodeOriginalMarketKnownBase(b,error),"alias not own base");b.figure.alias=false;b.figure.recordComplete=false;need(!encodeOriginalMarketKnownBase(b,error),"partial FSA cannot encode");
    b=sample(type);b.figure.fields[0]=256;need(!encodeOriginalMarketKnownBase(b,error),"oversized byte");}
    need(!decodeOriginalMarketKnownBase(std::string(16385,'a'),19,error),"budget");}
}
int main(int argc,char** argv){try{tests();
    if(argc>1){const char* configured=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!configured||!*configured){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
        const auto root=ResourceRoot::fromAbsolute(configured);size_t sources=0,actors=0;
        for(int n=1;n<argc;++n){const std::string relative=argv[n];need(relative.rfind("Save/",0)==0&&relative.find("..") ==std::string::npos,"relative authorized save only");
            const auto save=loadOriginalSave(root.resolve(relative));std::vector<uint8_t> bytes;need(save.status==OriginalSaveStatus::LoadedSnapshot&&detail::readFile(root.resolve(relative),4194304,bytes),"real source read only");
            const auto d=decodeOriginalMapContainer(bytes);need(d.complete,"container complete");emperor::file_io::research::GoalLimits limits;limits.enable_homage=limits.enable_trading=limits.enable_yearly=limits.enable_population=true;
            const auto goals=emperor::file_io::research::walk_city19_housing_goals({d.payload.data(),std::min(size_t(16384),d.payload.size())},limits);
            need(goals.completed_members==10&&!goals.pending.present,"qualified goals");const auto prefix=evidenceFigurePrefix(d.payload,save.buildings.tableEnd,save.terrainOffset,goals.cache.size()+4000+save.buildings.declaredClasses);
            need(prefix.complete&&prefix.completed==2000,"known full FSA boundary");size_t pair=0;
            for(const auto& f:prefix.records)if(f.field(22).value_or(0)&&(f.field(18)==24||f.field(18)==25)){OriginalMarketKnownBase b{save.snapshotRevision,f};std::string error;need(originalMarketKnownBaseMatches(b,save.layers.figures->slots.at(f.slot)),"real projection and raw single authority");
                const auto encoded=encodeOriginalMarketKnownBase(b,error);need(bool(encoded),"real market base encodes");const auto decoded=decodeOriginalMarketKnownBase(*encoded,save.snapshotRevision,error);
                need(bool(decoded)&&decoded->figure.fields==f.fields&&decoded->figure.opaque380==f.opaque380,"real known base exact roundtrip");++actors;++pair;}
            need(pair>=1&&pair<=2,"authorized original actor domain");++sources;}
        std::cout<<"{\"real_market_base_sources\":"<<sources<<",\"real_market_base_actors\":"<<actors<<",\"passed\":true}\n";
    }
std::cout<<"{\"passed\":true,\"market_base_codec_checks\":"<<checks<<",\"original_data\":false}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
