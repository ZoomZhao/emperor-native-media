#include "emperor_assets.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
namespace {
using Bytes=std::vector<uint8_t>;
size_t checks=0;
void need(bool b,const char* message){++checks;if(!b)throw std::runtime_error(message);}
void put16(Bytes& b,size_t p,uint16_t n){b[p]=n&255;b[p+1]=n>>8;}
void put32(Bytes& b,size_t p,uint32_t n){for(size_t i=0;i<4;++i)b[p+i]=(n>>(8*i))&255;}
void save(const std::filesystem::path& p,const Bytes& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());need(bool(f),"self fixture write");}
Bytes index(uint32_t version,const std::vector<game_lab::SG3Sprite>& sprites,const std::string& bitmap="own.bmp") {
    const size_t step=version==214?72:64;Bytes b(40680+(sprites.size()+1)*step);
    put32(b,0,b.size());put32(b,4,version);put32(b,12,sprites.size()+1);put32(b,16,sprites.size());put32(b,20,1);
    std::copy(bitmap.begin(),bitmap.end(),b.begin()+680);
    // A poisoned dummy ensures the loader really skips logical image ID0.
    put16(b,40680+20,999);put16(b,40680+50,999);
    for(size_t i=0;i<sprites.size();++i) {
        const auto& s=sprites[i];size_t p=40680+(i+1)*step;
        put32(b,p,s.offset);put32(b,p+4,s.length);put32(b,p+8,s.uncompressedLength);put32(b,p+16,uint32_t(s.mirrorOffset));
        put16(b,p+20,s.width);put16(b,p+22,s.height);put16(b,p+50,s.type);b[p+52]=s.external;b[p+55]=s.tileSize;
        if(version==214){put32(b,p+64,s.alphaOffset);put32(b,p+68,s.alphaLength);}
    }
    return b;
}
}
int main() {
    try {
        auto root=std::filesystem::temp_directory_path()/ ("emperor_sg3_self_"+std::to_string(getpid()));
        need(std::filesystem::create_directory(root),"fresh synthetic root");
        auto idx=root/"own.sg3",data=root/"own.555";
        game_lab::SG3Sprite s;s.width=2;s.height=1;s.type=1;s.length=4;
        save(data,{0x1f,0xf8,0,0x7c});save(idx,index(213,{s}));
        auto a=game_lab::loadSG3(idx.string());need(a.version==213 && a.capacity==2 && a.numImages==1 && a.sprites[0].width==2 && a.sprites[0].type==1,"real published offsets and dummy exclusion");
        auto d=game_lab::decodeSG3Sprite(a,0,data.string());need(d.image.valid() && d.image.pixelsBGRA[0]==0xfff700ff && d.image.pixelsBGRA[1]==0xffff0000,"555 literal colors preserved");
        s.type=0;save(idx,index(213,{s}));a=game_lab::loadSG3(idx.string());d=game_lab::decodeSG3Sprite(a,0,data.string());need(d.image.pixelsBGRA[0]==0,"old type0 color key only");
        s.width=2;s.height=2;s.type=256;s.length=8;
        save(data,{1,0x1f,0xf8,255,2,1,0xe0,3});save(idx,index(214,{s}));a=game_lab::loadSG3(idx.string());
        d=game_lab::decodeSG3Sprite(a,0,data.string());need(d.image.valid() && d.image.pixelsBGRA[0]==0xfff700ff && d.image.pixelsBGRA[1]==0 && d.image.pixelsBGRA[3]==0xff00ff00,"RLE preserves F81F literal and transparent run");
        save(data,{3,0,0});d=game_lab::decodeSG3Sprite(a,0,data.string());need(!d.image.valid(),"truncated literal rejected without fallback");
        s.length=2;save(data,{255,5});save(idx,index(214,{s}));a=game_lab::loadSG3(idx.string());need(!game_lab::decodeSG3Sprite(a,0,data.string()).image.valid(),"RLE output overflow rejected");
        s.type=276;s.length=9;s.alphaLength=5;s.alphaOffset=18;
        save(data,{4,0,0x7c,0xe0,3,0x1f,0,0xff,0x7f,4,0,15,31,31});
        auto mirror=s;mirror.mirrorOffset=-1;save(idx,index(214,{s,mirror}));a=game_lab::loadSG3(idx.string());
        d=game_lab::decodeSG3Sprite(a,0,data.string());need(d.image.valid() && d.image.pixelsBGRA[0]==0 && d.image.pixelsBGRA[1]==0x7b007b00,"214 adjacent mask becomes premultiplied BGRA");
        auto m=game_lab::decodeSG3Sprite(a,1,data.string());need(m.mirrored && m.image.pixelsBGRA[0]==d.image.pixelsBGRA[1] && m.image.pixelsBGRA[3]==d.image.pixelsBGRA[2],"signed mirror keeps logical ID and flips rows");
        mirror.mirrorOffset=1;save(idx,index(214,{s,mirror}));a=game_lab::loadSG3(idx.string());need(!game_lab::decodeSG3Sprite(a,1,data.string()).image.valid(),"forward mirror and cycles rejected");
        s={};s.type=30;s.width=78;s.height=40;s.length=s.uncompressedLength=3200;s.tileSize=1;
        Bytes diamond(3200);for(size_t i=1;i<diamond.size();i+=2)diamond[i]=0x7c;
        save(data,diamond);save(idx,index(213,{s}));a=game_lab::loadSG3(idx.string());d=game_lab::decodeSG3Sprite(a,0,data.string());
        size_t opaque=0;for(auto p:d.image.pixelsBGRA)opaque+=bool(p>>24);
        need(d.image.valid() && opaque==1600 && d.image.pixelsBGRA[38]==0xffff0000 && d.image.pixelsBGRA[0]==0,"Emperor 78x40 diamond placement");
        s.width=238;s.height=120;s.tileSize=0;s.length=s.uncompressedLength=28800;diamond.resize(28800);save(data,diamond);save(idx,index(213,{s}));a=game_lab::loadSG3(idx.string());need(game_lab::decodeSG3Sprite(a,0,data.string()).image.valid(),"3x3 footprint avoids 58x30 heuristic ambiguity");
        s.tileSize=4;save(idx,index(213,{s}));a=game_lab::loadSG3(idx.string());need(!game_lab::decodeSG3Sprite(a,0,data.string()).image.valid(),"wrong explicit footprint size rejected");
        s={};s.type=1;s.width=s.height=1;s.length=2;s.offset=1;s.external=1;save(root/"external.555",{0,0x7c});save(idx,index(213,{s},"external.bmp"));a=game_lab::loadSG3(idx.string());
        need(game_lab::decodeSG3Sprite(a,0,data.string()).image.pixelsBGRA[0]==0xffff0000,"external bitmap resolution and one-byte correction");
        save(idx,index(213,{s},"absent.bmp"));a=game_lab::loadSG3(idx.string());need(game_lab::decodeSG3Sprite(a,0,data.string()).status==game_lab::SG3DecodeStatus::MissingDependency,"missing file is explicit dependency failure");
        save(idx,index(213,{s},"../external.bmp"));a=game_lab::loadSG3(idx.string());need(game_lab::decodeSG3Sprite(a,0,data.string()).status==game_lab::SG3DecodeStatus::MissingDependency,"external traversal rejected");
        save(idx,index(215,{s}));need(game_lab::loadSG3(idx.string()).sprites.empty(),"unknown version rejected");
        auto cut=index(214,{s});cut.pop_back();save(idx,cut);need(game_lab::loadSG3(idx.string()).sprites.empty(),"incomplete record table rejected");
        std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"self_authored\":true,\"real_data\":false}\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
