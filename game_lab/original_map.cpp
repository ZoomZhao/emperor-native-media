#include "original_map.hpp"
#include "asset_bytes.hpp"
#include <array>
#include <algorithm>
#include <new>
#include <zlib.h>

namespace game_lab {
namespace {
constexpr size_t maxInput=4u*1024u*1024u,maxDecoded=8u*1024u*1024u,maxChunk=32768,maxFrames=256;
// Published city-map layout facts, independently checked on the local corpus.
constexpr size_t sideOffset=84,terrainOffset=261455,terrainCells=228u*228u;
constexpr std::array<uint8_t,8> citySignature{{5,0,0xfe,0xca,0,0,2,0}};
}
OriginalMapResult loadOriginalMapTerrain(const std::string& path) {
    OriginalMapResult result;
    std::vector<uint8_t> input;
    if(!detail::readFile(path,maxInput,input))return result;
    result.inputBytes=input.size();
    if(input.size()<4 || detail::u32(input.data())!=0xfedcbaaa) {
        result.status=OriginalMapStatus::InvalidContainer;return result;
    }
    try {
        std::vector<uint8_t> decoded;
        size_t cursor=4;
        while(cursor<input.size()) {
            if(result.frames==maxFrames){result.status=OriginalMapStatus::BudgetExceeded;return result;}
            if(input.size()-cursor<12){result.status=OriginalMapStatus::InvalidContainer;return result;}
            const auto* record=input.data()+cursor;
            if(detail::u32(record)!=0xabcdefff){result.status=OriginalMapStatus::UnsupportedContainer;return result;}
            const size_t encoded=detail::u32(record+4),declared=detail::u32(record+8);
            if(!encoded || encoded>2*maxChunk || !declared || declared>maxChunk || declared>maxDecoded-decoded.size()) {
                result.status=OriginalMapStatus::BudgetExceeded;return result;
            }
            if(encoded>input.size()-cursor-12){result.status=OriginalMapStatus::InvalidContainer;return result;}
            std::array<uint8_t,maxChunk+1> output{};
            z_stream stream{};
            stream.next_in=const_cast<Bytef*>(record+12);stream.avail_in=uInt(encoded);
            stream.next_out=output.data();stream.avail_out=output.size();
            if(inflateInit(&stream)!=Z_OK){result.status=OriginalMapStatus::InvalidContainer;return result;}
            const int status=inflate(&stream,Z_FINISH);
            const bool good=status==Z_STREAM_END && stream.total_out==declared && stream.total_in==encoded;
            inflateEnd(&stream);
            if(!good){result.status=OriginalMapStatus::InvalidContainer;return result;}
            decoded.insert(decoded.end(),output.begin(),output.begin()+declared);
            cursor+=12+encoded;++result.frames;
        }
        result.decodedBytes=decoded.size();
        if(decoded.size()<citySignature.size() || !std::equal(citySignature.begin(),citySignature.end(),decoded.begin())) {
            result.status=OriginalMapStatus::UnsupportedRole;return result;
        }
        if(decoded.size()<terrainOffset+terrainCells*4) {result.status=OriginalMapStatus::InvalidGrid;return result;}
        const uint32_t side=detail::u32(decoded.data()+sideOffset);
        if(!side || side>OriginalMapTerrain::storageSide) {result.status=OriginalMapStatus::InvalidGrid;return result;}
        OriginalMapTerrain grid;grid.activeSide=side;grid.flags.resize(terrainCells);
        for(size_t index=0;index<terrainCells;++index)grid.flags[index]=detail::u32(decoded.data()+terrainOffset+index*4);
        result.terrain=std::move(grid);result.status=OriginalMapStatus::LoadedTerrain;
    } catch(const std::bad_alloc&) {result.status=OriginalMapStatus::BudgetExceeded;}
    return result;
}
DecodedImage renderOriginalMapFlags(const OriginalMapTerrain& grid,int bit) {
    if(!grid.valid() || bit<-1 || bit>31)return {};
    try {
        DecodedImage image;image.width=image.height=OriginalMapTerrain::storageSide;image.pixelsBGRA.reserve(grid.flags.size());
        for(uint32_t value:grid.flags) {
            if(bit>=0)image.pixelsBGRA.push_back((value&(uint32_t(1)<<bit))?0xff54b9e8u:0xff24272bu);
            else if(!value)image.pixelsBGRA.push_back(0xffc9c5b7u);
            else {
                // Independent numerical palette. No GPL/game colors or flag rules.
                const uint32_t mixed=value*2654435761u;
                const uint32_t r=48+((mixed>>16)&127),g=48+((mixed>>8)&127),b=48+(mixed&127);
                image.pixelsBGRA.push_back(0xff000000u|(r<<16)|(g<<8)|b);
            }
        }
        return image;
    }catch(const std::bad_alloc&){return {};}
}
std::string describeOriginalMap(const OriginalMapResult& result) {
    switch(result.status) {
    case OriginalMapStatus::LoadedTerrain:return "原始城市地图地形网格：存储228×228，活动边长字段 "+std::to_string(result.terrain.activeSide)+"；只读，无城市状态绑定";
    case OriginalMapStatus::ReadFailure:return "地图文件不可读取或超出输入限制";
    case OriginalMapStatus::InvalidContainer:return "地图压缩容器不完整、校验失败或长度不符";
    case OriginalMapStatus::UnsupportedContainer:return "当前只支持已验证的单段压缩 MAP 容器";
    case OriginalMapStatus::UnsupportedRole:return "非已验证城市地图头；帝国地图等内部布局尚未支持";
    case OriginalMapStatus::InvalidGrid:return "城市地图尺寸或地形网格范围无效";
    case OriginalMapStatus::BudgetExceeded:return "地图读取/解码预算超限或分配失败";
    }
    return "地图载入失败";
}
} // namespace game_lab
