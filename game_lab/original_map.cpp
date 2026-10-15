#include "original_map.hpp"
#include "asset_bytes.hpp"
#include <array>
#include <algorithm>
#include <new>
#include <cmath>
#include <zlib.h>

namespace game_lab {
namespace {
constexpr size_t maxInput=4u*1024u*1024u,maxDecoded=8u*1024u*1024u,maxChunk=32768,maxFrames=256;
// Published city-map layout facts, independently checked on the local corpus.
constexpr size_t sideOffset=84,terrainOffset=261455,terrainCells=228u*228u;
constexpr std::array<uint8_t,8> citySignature{{5,0,0xfe,0xca,0,0,2,0}};
constexpr uint32_t outsideFlag=0x00080000u;
// Field annotations from public Emperor map data, not rendering priorities.
// Composite states, original colours, object grids and SG3 IDs remain separate.
constexpr std::array<const char*,32> flagLabels{{
    "树/竹相关", "岩石/矿石相关", "水相关", "建筑相关", nullptr,
    "花园相关", "道路相关", "肥沃相关", "洪水相关（组合）", "高程相关",
    nullptr, "灌溉渠相关", "废墟相关（组合）", nullptr, "城墙相关", nullptr,
    "沙滩相关", "采石相关", "盐沼相关", "地图外",
    "矿产/废墟相关（组合）", "矿产相关（组合）", nullptr, nullptr,
    nullptr, "尖峰相关", "深水相关（组合）", nullptr, "运河/纪念物相关",
    nullptr, nullptr, "沙地相关"
}};
uint32_t annotatedMask() {
    uint32_t mask=0;for(unsigned bit=0;bit<32;++bit)if(flagLabels[bit])mask|=uint32_t(1)<<bit;return mask;
}
uint32_t diagnosticColor(uint32_t value,int bit) {
    if(bit>=0)return (value&(uint32_t(1)<<bit))?0xff54b9e8u:0xff24272bu;
    if(!value)return 0xffc9c5b7u;
    const uint32_t mixed=value*2654435761u;
    const uint32_t r=48+((mixed>>16)&127),g=48+((mixed>>8)&127),b=48+(mixed&127);
    return 0xff000000u|(r<<16)|(g<<8)|b;
}
bool validCamera(const OriginalMapCamera& camera,const OriginalMapAnalysis& analysis,size_t& pixels) {
    return analysis.cropValid() && std::isfinite(camera.centerX)&&std::isfinite(camera.centerY)&&
        camera.centerX>=0&&camera.centerY>=0&&camera.centerX<analysis.cropSide&&camera.centerY<analysis.cropSide&&
        camera.scale>=1&&camera.scale<=4&&camera.width<=2048&&camera.height<=2048&&
        imagePixelCount(camera.width,camera.height,pixels)&&pixels<=2048u*2048u;
}
OriginalMapPoint projected(const OriginalMapCamera& camera,uint32_t x,uint32_t y) {
    const double dx=double(x)-camera.centerX,dy=double(y)-camera.centerY;
    // Canonical flat cell-centre basis, matching the public78x40 diamond.
    return {(dx-dy)*40*camera.scale+double(camera.width)/2,
            (dx+dy)*20*camera.scale+double(camera.height)/2};
}
}
std::optional<OriginalMapCellLayers> parseOriginalMapCellLayers(const std::vector<uint8_t>& payload) {
    const size_t end=OriginalMapCellLayers::mapperFertileOffset+OriginalMapCellLayers::cells;
    if(payload.size()<end||payload.size()>maxDecoded||
       !std::equal(citySignature.begin(),citySignature.end(),payload.begin()))return std::nullopt;
    const uint32_t side=detail::u32(payload.data()+sideOffset);
    if(!side||side>OriginalMapTerrain::storageSide)return std::nullopt;
    try {
        OriginalMapCellLayers layers;layers.objects.resize(terrainCells);
        for(size_t i=0;i<terrainCells;++i)layers.objects[i]=detail::u32(payload.data()+OriginalMapCellLayers::objectsOffset+4*i);
        auto copy=[&](size_t offset,std::vector<uint8_t>& target){target.assign(payload.begin()+offset,payload.begin()+offset+terrainCells);};
        copy(OriginalMapCellLayers::edgesOffset,layers.edges);
        copy(OriginalMapCellLayers::mapperRandomOffset,layers.mapperRandom);
        copy(OriginalMapCellLayers::mapperFertileOffset,layers.mapperFertile);
        return layers;
    }catch(const std::bad_alloc&){return std::nullopt;}
}
std::optional<OriginalMapRawLayer> parseOriginalMapRawLayer(const std::vector<uint8_t>& payload,OriginalMapRawSpec spec) {
    if((spec.width!=1&&spec.width!=2&&spec.width!=4)||payload.size()<88||payload.size()>maxDecoded||
       !std::equal(citySignature.begin(),citySignature.end(),payload.begin()))return std::nullopt;
    const auto side=detail::u32(payload.data()+sideOffset);
    const size_t bytes=terrainCells*spec.width;
    if(!side||side>228||spec.offset>payload.size()||bytes>payload.size()-spec.offset)return std::nullopt;
    try {
        OriginalMapRawLayer layer;layer.spec=spec;layer.values.resize(terrainCells);
        for(size_t i=0;i<terrainCells;++i){const auto* p=payload.data()+spec.offset+i*spec.width;
            layer.values[i]=spec.width==1?p[0]:spec.width==2?detail::u16(p):detail::u32(p);}
        return layer;
    }catch(const std::bad_alloc&){return std::nullopt;}
}
OriginalMapContainerResult decodeOriginalMapContainer(const std::vector<uint8_t>& input) {
    OriginalMapContainerResult result;result.inputBytes=input.size();
    if(input.size()>maxInput){result.failure=OriginalMapStatus::BudgetExceeded;return result;}
    if(input.size()<4||detail::u32(input.data())!=0xfedcbaaa)return result;
    try {
        std::vector<uint8_t> decoded;
        size_t cursor=4;
        while(cursor<input.size()) {
            if(result.frames==maxFrames){result.failure=OriginalMapStatus::BudgetExceeded;return result;}
            if(input.size()-cursor<12){result.failure=OriginalMapStatus::InvalidContainer;return result;}
            const auto* record=input.data()+cursor;
            if(detail::u32(record)!=0xabcdefff){result.failure=OriginalMapStatus::UnsupportedContainer;return result;}
            const size_t encoded=detail::u32(record+4),declared=detail::u32(record+8);
            if(!encoded || encoded>2*maxChunk || !declared || declared>maxChunk || declared>maxDecoded-decoded.size()) {
                result.failure=OriginalMapStatus::BudgetExceeded;return result;
            }
            if(encoded>input.size()-cursor-12){result.failure=OriginalMapStatus::InvalidContainer;return result;}
            std::array<uint8_t,maxChunk+1> output{};
            z_stream stream{};
            stream.next_in=const_cast<Bytef*>(record+12);stream.avail_in=uInt(encoded);
            stream.next_out=output.data();stream.avail_out=output.size();
            if(inflateInit(&stream)!=Z_OK){result.failure=OriginalMapStatus::InvalidContainer;return result;}
            const int status=inflate(&stream,Z_FINISH);
            const bool good=status==Z_STREAM_END && stream.total_out==declared && stream.total_in==encoded;
            inflateEnd(&stream);
            if(!good){result.failure=OriginalMapStatus::InvalidContainer;return result;}
            decoded.insert(decoded.end(),output.begin(),output.begin()+declared);
            cursor+=12+encoded;++result.frames;
        }
        if(!result.frames){result.failure=OriginalMapStatus::InvalidContainer;return result;}
        result.payload=std::move(decoded);result.complete=true;
    }catch(const std::bad_alloc&){result.failure=OriginalMapStatus::BudgetExceeded;}
    return result;
}
OriginalMapResult parseOriginalMapPayload(const std::vector<uint8_t>& decoded,std::optional<OriginalMapRawSpec> diagnosticSpec) {
    OriginalMapResult result;
    if(decoded.size()>maxDecoded){result.status=OriginalMapStatus::BudgetExceeded;return result;}
    result.decodedBytes=decoded.size();
    try {
        if(decoded.size()<citySignature.size() || !std::equal(citySignature.begin(),citySignature.end(),decoded.begin())) {
            result.status=OriginalMapStatus::UnsupportedRole;return result;
        }
        if(decoded.size()<terrainOffset+terrainCells*4) {result.status=OriginalMapStatus::InvalidGrid;return result;}
        const uint32_t side=detail::u32(decoded.data()+sideOffset);
        if(!side || side>OriginalMapTerrain::storageSide) {result.status=OriginalMapStatus::InvalidGrid;return result;}
        OriginalMapTerrain grid;grid.activeSide=side;grid.flags.resize(terrainCells);
        for(size_t index=0;index<terrainCells;++index)grid.flags[index]=detail::u32(decoded.data()+terrainOffset+index*4);
        result.cellLayers=parseOriginalMapCellLayers(decoded);
        if(diagnosticSpec)result.diagnosticRawLayer=parseOriginalMapRawLayer(decoded,*diagnosticSpec);
        result.terrain=std::move(grid);result.status=OriginalMapStatus::LoadedTerrain;
    } catch(const std::bad_alloc&) {result.status=OriginalMapStatus::BudgetExceeded;}
    return result;
}
OriginalMapResult loadOriginalMapTerrain(const std::string& path,std::optional<OriginalMapRawSpec> diagnosticSpec) {
    OriginalMapResult result;std::vector<uint8_t> input;
    if(!detail::readFile(path,maxInput,input))return result;
    auto container=decodeOriginalMapContainer(input);
    if(!container.complete){result.status=container.failure;result.inputBytes=container.inputBytes;result.frames=container.frames;return result;}
    result=parseOriginalMapPayload(container.payload,diagnosticSpec);
    result.inputBytes=container.inputBytes;result.frames=container.frames;return result;
}
OriginalMapAnalysis analyzeOriginalMap(const OriginalMapTerrain& grid) {
    OriginalMapAnalysis result;if(!grid.valid())return result;
    switch(grid.activeSide){case 84:case 112:case 140:case 170:case 226:result.cropSupported=true;break;default:break;}
    if(result.cropSupported){result.cropSide=grid.activeSide;result.cropOrigin=(OriginalMapTerrain::storageSide-grid.activeSide)/2;}
    const uint32_t known=annotatedMask();
    for(size_t index=0;index<grid.flags.size();++index){
        const uint32_t value=grid.flags[index];if(value&outsideFlag)continue;
        ++result.activeCells;
        const size_t x=index%OriginalMapTerrain::storageSide,y=index/OriginalMapTerrain::storageSide;
        if(!result.cropSupported || x<result.cropOrigin || y<result.cropOrigin || x>=result.cropOrigin+result.cropSide || y>=result.cropOrigin+result.cropSide)++result.outsideCropCells;
        const uint32_t unknown=value&~known;result.unknownBits|=unknown;
        if(unknown)++result.unknownFlagCells;
        for(unsigned bit=0;bit<32;++bit)if(unknown&(uint32_t(1)<<bit))++result.unknownBitOccurrences[bit];
    }
    return result;
}
std::string originalMapFlagLabel(unsigned bit) {
    if(bit>=32)return "无效位";
    return flagLabels[bit]?std::string("公开标注：")+flagLabels[bit]:"unknown：无公开标注";
}
DecodedImage renderOriginalMapFlags(const OriginalMapTerrain& grid,int bit,bool crop) {
    if(!grid.valid() || bit<-1 || bit>31)return {};
    const auto analysis=analyzeOriginalMap(grid);if(crop && !analysis.cropValid())return {};
    try {
        const uint32_t side=crop?analysis.cropSide:OriginalMapTerrain::storageSide;
        const uint32_t origin=crop?analysis.cropOrigin:0;
        DecodedImage image;image.width=image.height=side;image.pixelsBGRA.reserve(size_t(side)*side);
        for(uint32_t y=0;y<side;++y)for(uint32_t x=0;x<side;++x) {
            const uint32_t value=grid.flags[size_t(y+origin)*OriginalMapTerrain::storageSide+x+origin];
            if(crop && (value&outsideFlag)){image.pixelsBGRA.push_back(0);continue;}
            // Reuse the same independent numerical palette in both views.
            image.pixelsBGRA.push_back(diagnosticColor(value,bit));
        }
        return image;
    }catch(const std::bad_alloc&){return {};}
}
std::optional<OriginalMapPoint> projectOriginalMapCell(const OriginalMapTerrain& grid,const OriginalMapCamera& camera,
                                                     uint32_t activeX,uint32_t activeY) {
    const auto analysis=analyzeOriginalMap(grid);size_t count=0;
    if(!validCamera(camera,analysis,count)||activeX>=analysis.cropSide||activeY>=analysis.cropSide)return std::nullopt;
    return projected(camera,activeX,activeY);
}
DecodedImage renderOriginalMapSpatialFlags(const OriginalMapTerrain& grid,const OriginalMapCamera& camera,int bit) {
    if(bit<-1||bit>31)return {};
    const auto analysis=analyzeOriginalMap(grid);size_t count=0;if(!validCamera(camera,analysis,count))return {};
    try {
        DecodedImage image;image.width=camera.width;image.height=camera.height;image.pixelsBGRA.assign(count,0);
        const int scale=int(camera.scale);
        for(uint32_t y=0;y<analysis.cropSide;++y)for(uint32_t x=0;x<analysis.cropSide;++x) {
            const uint32_t value=grid.flags[size_t(y+analysis.cropOrigin)*OriginalMapTerrain::storageSide+x+analysis.cropOrigin];
            if(value&outsideFlag)continue;
            const auto center=projected(camera,x,y);
            // Half-pixel ties go toward +infinity on both sides of zero;
            // unlike lround, this preserves the phase under integer translations.
            const int left=int(std::floor(center.x+.5))-39*scale,top=int(std::floor(center.y+.5))-20*scale;
            if(left>=int(camera.width)||top>=int(camera.height)||left+78*scale<=0||top+40*scale<=0)continue;
            const uint32_t color=diagnosticColor(value,bit);
            for(int row=0;row<40;++row) {
                const int inset=row<20?38-2*row:2*row-40;
                for(int column=inset;column<78-inset;++column) {
                    const bool edge=column==inset||column+1==78-inset||row==0||row==39;
                    const uint32_t pixel=camera.outlines&&edge?0xff959ea3u:color;
                    for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx) {
                        const int px=left+column*scale+sx,py=top+row*scale+sy;
                        if(px>=0&&py>=0&&px<int(camera.width)&&py<int(camera.height))
                            image.pixelsBGRA[size_t(py)*camera.width+size_t(px)]=pixel;
                    }
                }
            }
        }
        return image;
    }catch(const std::bad_alloc&){return {};}
}
std::string describeOriginalMap(const OriginalMapResult& result) {
    switch(result.status) {
    case OriginalMapStatus::LoadedTerrain:return "原始城市地图地形网格：存储228×228，活动边长字段 "+std::to_string(result.terrain.activeSide)+"；只读，无城市状态绑定；额外原始逐格字段"+(result.cellLayers?std::string("已完整保留，选图关系未知"):std::string("未完整保留"));
    case OriginalMapStatus::ReadFailure:return "地图文件不可读取或超出输入限制";
    case OriginalMapStatus::InvalidContainer:return "地图压缩容器不完整、校验失败或长度不符";
    case OriginalMapStatus::UnsupportedContainer:return "未支持的压缩容器或记录结构";
    case OriginalMapStatus::UnsupportedRole:return "非已验证城市地图头；帝国地图等内部布局尚未支持";
    case OriginalMapStatus::InvalidGrid:return "城市地图尺寸或地形网格范围无效";
    case OriginalMapStatus::BudgetExceeded:return "地图读取/解码预算超限或分配失败";
    }
    return "地图载入失败";
}
} // namespace game_lab
