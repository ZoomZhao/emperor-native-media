#include "original_save.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <new>
#include <cstdio>

namespace game_lab {
namespace {
constexpr size_t cells=228u*228u,maxInput=4u*1024u*1024u,maxMarkers=1024;
constexpr size_t following=207936,randomOffset=415872,stoneOffset=1510985,fertileOffset=1822996,objectsOffset=2018734;
constexpr std::array<uint8_t,16> brief{{'B','r','i','e','f',' ','d','e','s','c','r','i','p','t','i','o'}};
constexpr std::array<uint8_t,16> terrainMarker{{0,0,0,0,5,0,8,0,5,0,8,0,5,0,8,0}};
bool positions(const std::vector<uint8_t>& data,const std::array<uint8_t,16>& needle,size_t from,std::vector<size_t>& hits){
    if(from>data.size())return true;
    auto cursor=data.begin()+from;
    while(cursor!=data.end()){
        const auto found=std::search(cursor,data.end(),needle.begin(),needle.end());if(found==data.end())break;
        if(hits.size()==maxMarkers)return false;hits.push_back(size_t(found-data.begin()));cursor=found+1;
    }return true;
}
// The final objects span bounds all six published raw-field extents.
// Marker bytes alone are not an anchor: a complete compatible grid is required.
bool compatibleTerrain(const std::vector<uint8_t>& data,size_t base,uint32_t side){
    constexpr size_t needed=objectsOffset+4*cells;
    if(base>data.size()||needed>data.size()-base)return false;
    const uint32_t origin=(228-side)/2,end=origin+side-1;
    uint32_t minX=228,minY=228,maxX=0,maxY=0;bool active=false;
    for(size_t i=0;i<cells;++i){
        if(detail::u32(data.data()+base+4*i)&0x80000u)continue;
        const auto x=uint32_t(i%228),y=uint32_t(i/228);
        if(x<origin||y<origin||x>end||y>end)return false;
        active=true;minX=std::min(minX,x);minY=std::min(minY,y);maxX=std::max(maxX,x);maxY=std::max(maxY,y);
    }
    return active&&minX==origin&&minY==origin&&maxX==end&&maxY==end;
}
struct File {int fd;explicit File(const std::string& p):fd(open(p.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW)){}~File(){if(fd>=0)close(fd);}};
bool same(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_size==b.st_size&&a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec;}
bool readExact(int fd,std::vector<uint8_t>& bytes){
    size_t offset=0;while(offset<bytes.size()){const auto n=read(fd,bytes.data()+offset,bytes.size()-offset);if(n<0&&errno==EINTR)continue;if(n<=0)return false;offset+=size_t(n);}
    uint8_t tail=0;ssize_t extra;do{extra=read(fd,&tail,1);}while(extra<0&&errno==EINTR);return extra==0;
}
}
OriginalSaveResult decodeOriginalSave(const std::vector<uint8_t>& input){
    OriginalSaveResult result;result.inputBytes=input.size();auto container=decodeOriginalMapContainer(input);
    if(!container.complete){result.status=container.failure==OriginalMapStatus::BudgetExceeded?OriginalSaveStatus::BudgetExceeded:OriginalSaveStatus::InvalidContainer;return result;}
    const auto& data=container.payload;
    if(data.size()<2||data[0]!=0x13||data[1]!=0){result.status=OriginalSaveStatus::UnsupportedRole;return result;}
    try{
        std::vector<size_t> sizes;
        if(!positions(data,brief,0,sizes)){result.status=OriginalSaveStatus::BudgetExceeded;return result;}
        size_t sizeOffset=0;uint32_t side=0;size_t valid=0;
        for(auto position:sizes)if(position>=16){const auto candidate=detail::u32(data.data()+position-16);if(candidate>=1&&candidate<=228){sizeOffset=position-16;side=candidate;++valid;}}
        if(valid!=1){result.status=OriginalSaveStatus::UnsupportedAnchors;return result;}
        std::vector<size_t> terrainHits;
        if(!positions(data,terrainMarker,sizeOffset+4,terrainHits)){result.status=OriginalSaveStatus::BudgetExceeded;return result;}
        if(terrainHits.empty()){result.status=OriginalSaveStatus::UnsupportedAnchors;return result;}
        size_t base=0,validTerrain=0;
        // Check every bounded hit before accepting a unique structural candidate.
        // No first-hit, content ranking, MAP lookup or city-specific exception.
        for(auto hit:terrainHits)if(compatibleTerrain(data,hit+4,side)){base=hit+4;++validTerrain;}
        if(validTerrain!=1){result.status=validTerrain?OriginalSaveStatus::UnsupportedAnchors:OriginalSaveStatus::InvalidGrid;return result;}
        OriginalMapTerrain terrain;terrain.activeSide=side;terrain.flags.resize(cells);
        for(size_t i=0;i<cells;++i)terrain.flags[i]=detail::u32(data.data()+base+4*i);
        OriginalSaveCellLayers layers;layers.followingU32Unknown.resize(cells);layers.mapperObjects.resize(cells);
        for(size_t i=0;i<cells;++i){layers.followingU32Unknown[i]=detail::u32(data.data()+base+following+4*i);layers.mapperObjects[i]=detail::u32(data.data()+base+objectsOffset+4*i);}
        const auto copy=[&](size_t relative,std::vector<uint8_t>& values){values.assign(data.begin()+base+relative,data.begin()+base+relative+cells);};
        copy(randomOffset,layers.mapperRandom);copy(stoneOffset,layers.mapperStone);copy(fertileOffset,layers.mapperFertile);
        result.buildings=parseOriginalSavedBuildings(data,base,terrain);
        result.terrain=std::move(terrain);result.layers=std::move(layers);result.frames=container.frames;result.decodedBytes=data.size();result.sizeOffset=sizeOffset;result.terrainOffset=base;result.status=OriginalSaveStatus::LoadedSnapshot;result.snapshotRevision=14695981039346656037ull;for(uint8_t byte:input){result.snapshotRevision^=byte;result.snapshotRevision*=1099511628211ull;}
    }catch(const std::bad_alloc&){result.status=OriginalSaveStatus::BudgetExceeded;}
    return result;
}
OriginalSaveResult loadOriginalSave(const std::string& path){
    OriginalSaveResult failed;
    File file(path);struct stat before{},middle{},after{},named{};
    if(file.fd<0||fstat(file.fd,&before)||!S_ISREG(before.st_mode)||before.st_size<0)return failed;
    if(uint64_t(before.st_size)>maxInput){failed.status=OriginalSaveStatus::BudgetExceeded;return failed;}
    try{
        std::vector<uint8_t> first(size_t(before.st_size)),second(first.size());
        if(!readExact(file.fd,first)||fstat(file.fd,&middle))return failed;
        if(!same(before,middle)||lseek(file.fd,0,SEEK_SET)<0){failed.status=OriginalSaveStatus::ChangedDuringRead;return failed;}
        if(!readExact(file.fd,second)||fstat(file.fd,&after)||stat(path.c_str(),&named))return failed;
        if(first!=second||!same(before,after)||!same(before,named)){failed.status=OriginalSaveStatus::ChangedDuringRead;return failed;}
        auto result=decodeOriginalSave(first);result.readStable=true;return result;
    }catch(const std::bad_alloc&){failed.status=OriginalSaveStatus::BudgetExceeded;return failed;}
}
std::string describeOriginalSave(const OriginalSaveResult& save){
    switch(save.status){
    case OriginalSaveStatus::ReadFailure:return "SAV 读取失败；未显示旧快照";
    case OriginalSaveStatus::ChangedDuringRead:return "SAV 读取期间已改变；未发布混合快照";
    case OriginalSaveStatus::InvalidContainer:return "SAV 压缩帧校验或边界失败；未发布部分快照";
    case OriginalSaveStatus::UnsupportedRole:return "不是已支持的 SAV13 数据角色";
    case OriginalSaveStatus::UnsupportedAnchors:return "SAV 尺寸/有效地形锚点缺失或不唯一；没有按 MAP 相似度挑选";
    case OriginalSaveStatus::InvalidGrid:return "SAV 原始字段范围或活动框不兼容；未发布部分快照";
    case OriginalSaveStatus::BudgetExceeded:return "SAV 输入、标记或解码预算超限";
    case OriginalSaveStatus::LoadedSnapshot:break;
    }
    return "SAV13 已保存快照；"+std::string(save.readStable?"两次读取一致":"内存字节输入")+"；5 组原始字段已保留，含未知 following_u32。非当前内存；空间视图单独绑定静态建筑基座，不恢复规则；"+describeOriginalBuildingProfile(save.buildings)+"。";
}
std::optional<OriginalSaveCell> originalSaveCell(const OriginalMapTerrain& terrain,
    const OriginalSaveCellLayers& layers,int64_t x,int64_t y){
    if(!terrain.valid()||!layers.valid()||!analyzeOriginalMap(terrain).cropValid()||
        x<0||y<0||x>=terrain.activeSide||y>=terrain.activeSide)return std::nullopt;
    OriginalSaveCell cell;const uint32_t origin=(228-terrain.activeSide)/2;
    cell.storageX=origin+uint32_t(x);cell.storageY=origin+uint32_t(y);
    const size_t i=size_t(cell.storageY)*228+cell.storageX;
    cell.values={{terrain.flags[i],layers.followingU32Unknown[i],layers.mapperObjects[i],
        layers.mapperRandom[i],layers.mapperFertile[i],layers.mapperStone[i]}};
    cell.onMap=(cell.values[0]&0x80000u)==0;return cell;
}
std::string describeOriginalSaveCell(const OriginalSaveCell& cell){
    std::string text="存储坐标("+std::to_string(cell.storageX)+","+std::to_string(cell.storageY)+")；"+
        (cell.onMap?"地图内":"地图外（仍显示原值）")+"；只读快照，字段用途未扩展。\n";
    for(unsigned f=0;f<6;++f){
        char value[80];std::snprintf(value,sizeof(value),": %u / 0x%08X\n",cell.values[f],cell.values[f]);
        text+=(f?originalSaveFieldLabel(f-1):"SAV terrain flags")+std::string(value);
    }return text;
}
std::vector<uint32_t> originalSaveFieldValues(const OriginalSaveCellLayers& layers,unsigned field){
    if(!layers.valid()||field>4)return {};
    try{
    if(field==0)return layers.followingU32Unknown;
    if(field==1)return layers.mapperObjects;
    const auto& bytes=field==2?layers.mapperRandom:field==3?layers.mapperFertile:layers.mapperStone;
    return {bytes.begin(),bytes.end()};
    }catch(const std::bad_alloc&){return {};}
}
std::string originalSaveFieldLabel(unsigned field){
    static constexpr std::array<const char*,5> labels{{"SAV 未知 following_u32（仅原值）","SAV objects（公开变量名，仅原值）","SAV random（公开变量名，仅原值）","SAV fertile（公开变量名，仅原值）","SAV stone（公开变量名，仅原值）"}};
    return field<labels.size()?labels[field]:"无效 SAV 字段";
}
}
