#include "emperor_assets.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <limits>
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
        put16(b,p+20,s.width);put16(b,p+22,s.height);put16(b,p+30,s.animationSpriteCount);
        put16(b,p+34,uint16_t(s.animationOffsetX));put16(b,p+36,uint16_t(s.animationOffsetY));
        b[p+48]=s.animationReversible;b[p+58]=s.animationSpeedId;
        put16(b,p+50,s.type);b[p+52]=s.external;b[p+55]=s.tileSize;
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
        // Names/widths from the public SG image table, independent of playback.
        for(uint32_t version : {213u,214u}) {
            auto metadata=s;metadata.animationSpriteCount=version==213?65535:0x1234;
            metadata.animationOffsetX=version==213?-32768:32767;
            metadata.animationOffsetY=version==213?32767:-32768;
            metadata.animationReversible=version==213?255:0x7d;metadata.animationSpeedId=version==213?0xa5:255;
            auto raw=index(version,{metadata});
            put16(raw,40680+30,1234); // Poison dummy animation metadata.
            const size_t record=40680+(version==213?64:72);
            put16(raw,record+24,0x1234);put16(raw,record+26,0x5678);
            save(idx,raw);auto loaded=game_lab::loadSG3(idx.string());
            need(loaded.sprites.size()==1,"animation metadata retains logical record extent");
            const auto& actual=loaded.sprites[0];
            need(actual.animationSpriteCount==metadata.animationSpriteCount && actual.animationOffsetX==metadata.animationOffsetX &&
                 actual.animationOffsetY==metadata.animationOffsetY && actual.animationReversible==metadata.animationReversible &&
                 actual.animationSpeedId==metadata.animationSpeedId,"animation fields use published widths, signedness, offsets and raw byte IDs");
            const auto staticImage=game_lab::decodeSG3Sprite(loaded,0,data.string());
            need(staticImage.image.pixelsBGRA==d.image.pixelsBGRA,
                 "animation metadata does not imply a frame range, speed table, static origin or changed pixels");
        }
        // Distinguish header slot, logical ID, dummy ID0, and capacity slots.
        auto groupFixture=index(214,{s,s});put32(groupFixture,12,10);
        put16(groupFixture,80,1);put16(groupFixture,80+2*299,2);put16(groupFixture,80+2*298,3);
        save(idx,groupFixture);a=game_lab::loadSG3(idx.string());
        need(a.imageIndex[0]==1 && a.imageIndex[299]==2,"300 uint16 header lookup slots and little-endian endpoints");
        need(game_lab::sg3LogicalIndexForGroup(a,0)==size_t(0) && game_lab::sg3LogicalIndexForGroup(a,299)==size_t(1),"lookup logical IDs excludes dummy and preserves zero-based sprite index");
        need(!game_lab::sg3LogicalIndexForGroup(a,1),"unused zero group never aliases dummy or ID1");
        need(!game_lab::sg3LogicalIndexForGroup(a,298),"in-capacity but unused image ID rejected by group lookup");
        need(!game_lab::sg3LogicalIndexForGroup(a,300),"out-of-range header slot rejected");
        a.sprites.resize(1);need(!game_lab::sg3LogicalIndexForGroup(a,299),"inconsistent archive cannot return an out-of-range vector index");
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
        game_lab::SG3Sprite empty;save(idx,index(213,{empty},"absent.bmp"));a=game_lab::loadSG3(idx.string());
        auto blank=game_lab::decodeSG3Sprite(a,0,(root/"missing.555").string());
        need(a.sprites.size()==1 && blank.status==game_lab::SG3DecodeStatus::EmptyRecord && !blank.image.valid(),"zero record stays enumerable and is never decoded or hidden by missing dependency");
        auto emptyMirror=empty;emptyMirror.type=256;emptyMirror.mirrorOffset=-1;
        save(idx,index(214,{empty,emptyMirror},"absent.bmp"));a=game_lab::loadSG3(idx.string());
        blank=game_lab::decodeSG3Sprite(a,1,(root/"missing.555").string());
        need(blank.status==game_lab::SG3DecodeStatus::EmptyRecord && blank.mirrored && blank.image.pixelsBGRA.empty(),"mirrored empty source remains empty without fallback pixels");
        auto invalid=empty;invalid.length=2;save(idx,index(213,{invalid}));a=game_lab::loadSG3(idx.string());
        need(game_lab::decodeSG3Sprite(a,0,data.string()).status==game_lab::SG3DecodeStatus::InvalidRecord,"positive payload with zero dimensions is still an invalid record");
        save(idx,index(215,{s}));need(game_lab::loadSG3(idx.string()).sprites.empty(),"unknown version rejected");
        auto cut=index(214,{s});cut.pop_back();save(idx,cut);need(game_lab::loadSG3(idx.string()).sprites.empty(),"incomplete record table rejected");
        // Authored base-centred placement, independent of image/MAP selection.
        game_lab::SG3Sprite cell;cell.type=30;cell.width=78;cell.height=49;cell.length=4000;cell.uncompressedLength=3200;
        cell.animationOffsetX=32767;cell.animationOffsetY=-32768;
        game_lab::SG3Archive cells;cells.numImages=2;cells.sprites={cell,cell};cells.sprites[1].mirrorOffset=-1;
        auto geometry=game_lab::sg3SingleCellGeometry(cells,1);
        need(geometry&&geometry->width==78&&geometry->height==49&&geometry->baseCenterX==39&&geometry->baseCenterY==29,"tall base geometry follows effective mirrored source, not animation offsets");
        need(!game_lab::sg3SingleCellGeometry(cells,2),"unused or out-of-array canvas ID rejected");
        cells.sprites[1].mirrorOffset=1;need(!game_lab::sg3SingleCellGeometry(cells,1),"forward canvas mirror rejected");
        cells.sprites[1].mirrorOffset=-1;
        for(int bad=0;bad<5;++bad){auto invalid=cells;
            if(bad==0)invalid.sprites[0].type=256;if(bad==1)invalid.sprites[0].height=39;if(bad==2)invalid.sprites[0].width=158;
            if(bad==3)invalid.sprites[0].uncompressedLength=3198;if(bad==4)invalid.sprites[0].tileSize=2;
            need(!game_lab::sg3SingleCellGeometry(invalid,1),"only declared single-cell isometric geometry qualifies");}
        game_lab::DecodedImage tile;tile.width=78;tile.height=49;tile.pixelsBGRA.assign(78*49,0);
        tile.pixelsBGRA[29*78+39]=0xff123456;tile.pixelsBGRA[39]=0x80800000;
        game_lab::SG3CanvasPlacement placement;auto placed=game_lab::renderSG3SingleCellCanvas(tile,placement);
        need(placed.valid()&&placed.pixelsBGRA[120*500+250]==0xff123456&&placed.pixelsBGRA[91*500+250]==0x80800000&&placed.pixelsBGRA[0]==0,"full tall canvas preserves transparent and premultiplied pixels with its base centre fixed");
        auto tall=tile;tall.height=58;tall.pixelsBGRA.assign(78*58,0);tall.pixelsBGRA[38*78+39]=0xff123456;
        auto taller=game_lab::renderSG3SingleCellCanvas(tall,placement);need(taller.pixelsBGRA[120*500+250]==0xff123456,"changing record height does not move its bottom-base centre");
        auto doubledPlacement=placement;doubledPlacement.scale=2;doubledPlacement.width=1000;doubledPlacement.height=480;doubledPlacement.centerX=500;doubledPlacement.centerY=240;
        auto doubledCanvas=game_lab::renderSG3SingleCellCanvas(tile,doubledPlacement);bool replicated=doubledCanvas.valid();
        for(size_t y=0;y<240&&replicated;++y)for(size_t x=0;x<500&&replicated;++x)for(size_t sy=0;sy<2;++sy)for(size_t sx=0;sx<2;++sx)
            replicated&=doubledCanvas.pixelsBGRA[(2*y+sy)*1000+2*x+sx]==placed.pixelsBGRA[y*500+x];
        need(replicated,"nearest canvas scaling checks every replica");
        auto clipping=placement;clipping.centerX=0;clipping.centerY=20;auto clipped=game_lab::renderSG3SingleCellCanvas(tile,clipping);
        need(clipped.valid()&&clipped.pixelsBGRA[20*500]==0xff123456,"offscreen top/left clip safely while retaining the base centre");
        for(int bad=0;bad<5;++bad){auto invalid=placement;if(bad==0)invalid.centerX=std::numeric_limits<double>::quiet_NaN();
            if(bad==1)invalid.centerY=1000001;if(bad==2)invalid.scale=0;if(bad==3)invalid.width=2049;if(bad==4)invalid.height=0;
            need(!game_lab::renderSG3SingleCellCanvas(tile,invalid).valid(),"invalid single-cell canvas budget or centre fails without stale image");}
        std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"self_authored\":true,\"real_data\":false}\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
