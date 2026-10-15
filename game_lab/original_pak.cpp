#include "original_pak.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <array>
#include <set>
#include <new>

namespace game_lab {
namespace {
constexpr size_t maxPackage=128u*1024u*1024u,maxDecoded=32u*1024u*1024u;
constexpr size_t footerBytes=68,offsetSlots=11,maxSourceFiles=512;
std::string lower(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return s;}
OriginalPakStatus pakFailure(OriginalMapStatus s){return s==OriginalMapStatus::BudgetExceeded?OriginalPakStatus::BudgetExceeded:s==OriginalMapStatus::UnsupportedContainer?OriginalPakStatus::UnsupportedContainer:OriginalPakStatus::InvalidContainer;}
OriginalMapStatus mapFailure(OriginalPakStatus s){return s==OriginalPakStatus::BudgetExceeded?OriginalMapStatus::BudgetExceeded:s==OriginalPakStatus::UnsupportedContainer?OriginalMapStatus::UnsupportedContainer:s==OriginalPakStatus::ReadFailure?OriginalMapStatus::ReadFailure:OriginalMapStatus::InvalidContainer;}
}
OriginalPakResult decodeOriginalPak(const std::vector<uint8_t>& input){
    OriginalPakResult result;result.inputBytes=input.size();
    if(input.size()>maxPackage){result.status=OriginalPakStatus::BudgetExceeded;return result;}
    if(input.size()<4+footerBytes||detail::u32(input.data())!=0xfedcbaaa){result.status=OriginalPakStatus::InvalidContainer;return result;}
    const size_t footer=input.size()-footerBytes;const auto* table=input.data()+footer;
    if(detail::u32(table)!=0xaaabcdef||detail::u32(table+4)!=0x3c||
       detail::u32(table+56)!=0xaaabcdef||detail::u32(table+60)!=4||detail::u32(table+64)!=0xaaaabbcc){result.status=OriginalPakStatus::UnsupportedContainer;return result;}
    const size_t lastSlot=detail::u32(table+52);
    if(lastSlot>=offsetSlots){result.status=OriginalPakStatus::InvalidContainer;return result;}
    try{
        std::vector<size_t> bounds{4};
        for(size_t i=0;i<=lastSlot;++i){const size_t offset=detail::u32(table+8+4*i);
            if(offset<4||offset>footer||offset<bounds.back()){result.status=OriginalPakStatus::InvalidContainer;return result;}
            if(offset!=bounds.back())bounds.push_back(offset);
        }
        if(bounds.back()!=footer)bounds.push_back(footer);
        if(bounds.size()<2){result.status=OriginalPakStatus::InvalidContainer;return result;}
        std::vector<OriginalPakPart> parts;size_t total=0,frames=0;
        for(size_t i=0;i+1<bounds.size();++i){
            const size_t start=bounds[i],end=bounds[i+1];
            if(end-start>4u*1024u*1024u-4){result.status=OriginalPakStatus::BudgetExceeded;return result;}
            std::vector<uint8_t> framed(input.begin(),input.begin()+4);
            framed.insert(framed.end(),input.begin()+start,input.begin()+end);
            auto decoded=decodeOriginalMapContainer(framed);
            if(!decoded.complete){result.status=pakFailure(decoded.failure);return result;}
            if(decoded.payload.size()>maxDecoded-total){result.status=OriginalPakStatus::BudgetExceeded;return result;}
            total+=decoded.payload.size();frames+=decoded.frames;
            OriginalPakPart part;part.index=i;
            constexpr std::array<uint8_t,8> signature{{5,0,0xfe,0xca,0,0,2,0}};
            part.cityCandidate=decoded.payload.size()>=signature.size()&&std::equal(signature.begin(),signature.end(),decoded.payload.begin());
            part.map=parseOriginalMapPayload(decoded.payload);
            part.map.inputBytes=framed.size();part.map.frames=decoded.frames;
            parts.push_back(std::move(part));
        }
        result.parts=std::move(parts);result.decodedBytes=total;result.frames=frames;result.status=OriginalPakStatus::Loaded;
    }catch(const std::bad_alloc&){result.status=OriginalPakStatus::BudgetExceeded;}
    return result;
}
OriginalPakResult loadOriginalPak(const std::string& path){std::vector<uint8_t> bytes;if(!detail::readFile(path,maxPackage,bytes))return {};return decodeOriginalPak(bytes);}
std::string describeOriginalPak(const OriginalPakResult& p){
    switch(p.status){
    case OriginalPakStatus::ReadFailure:return "战役包不可读取或文件预算超限";
    case OriginalPakStatus::InvalidContainer:return "战役包边界、压缩校验或有效偏移无效；不显示部分城市";
    case OriginalPakStatus::UnsupportedContainer:return "战役包容器/页脚结构未支持；不显示部分城市";
    case OriginalPakStatus::BudgetExceeded:return "战役包解码预算超限；不显示部分城市";
    case OriginalPakStatus::Loaded:break;
    }
    size_t candidates=0,loaded=0;for(const auto& part:p.parts){candidates+=part.cityCandidate;loaded+=part.cityCandidate&&part.map.status==OriginalMapStatus::LoadedTerrain;}
    return "战役包完整校验："+std::to_string(p.parts.size())+" 个分段；城市地形 "+std::to_string(loaded)+"/"+std::to_string(candidates)+"；其他分段语义未解释。选择包内城市查看数值地形；不是可玩战役或正确原版画面。";
}
OriginalMapCatalog enumerateOriginalMapSources(const ResourceRoot& root){
    OriginalMapCatalog catalog;
    if(!root.configured()){catalog.error="未配置数据目录";return catalog;}
    try{
        std::vector<std::string> maps,paks,saves;std::set<std::string> seen;std::error_code error;
        for(std::filesystem::recursive_directory_iterator it(root.configuredDirectory(),error),end;!error&&it!=end;it.increment(error)){
            const bool regular=it->is_regular_file(error);if(error)break;if(!regular)continue;
            const auto ext=lower(it->path().extension().string());
            const auto relative=it->path().lexically_relative(root.configuredDirectory()).generic_string();
            const bool pak=ext==".pak"&&lower(relative).rfind("campaigns/",0)==0;
            const auto name=lower(it->path().filename().string());
            const bool save=name.size()>=4&&name.compare(name.size()-4,4,".sav")==0&&lower(relative).rfind("save/",0)==0;
            if(ext!=".map"&&!pak&&!save)continue;
            const auto path=root.resolve(relative);
            if(path.empty()||!seen.insert(path).second){catalog.error="地图来源越出数据目录或有重复文件别名";return catalog;}
            (save?saves:pak?paks:maps).push_back(relative);
            if(maps.size()+paks.size()+saves.size()>maxSourceFiles){catalog.error="地图来源文件数量超限";return catalog;}
        }
        if(error){catalog.error="地图来源枚举失败";return catalog;}
        std::sort(maps.begin(),maps.end());std::sort(paks.begin(),paks.end());
        for(const auto& path:maps)catalog.sources.push_back({OriginalMapSourceKind::MapFile,path,path,0});
        catalog.looseMaps=maps.size();catalog.pakFiles=paks.size();
        for(const auto& path:paks){auto package=loadOriginalPak(root.resolve(path));size_t ordinal=0;
            catalog.sources.push_back({OriginalMapSourceKind::PakSummary,path,path+" · 战役包信息",0});
            if(package.status!=OriginalPakStatus::Loaded)continue;
            ++catalog.completePaks;catalog.pakParts+=package.parts.size();catalog.pakDecodedBytes+=package.decodedBytes;
            for(const auto& part:package.parts)if(part.cityCandidate){
                ++ordinal;++catalog.cityCandidates;
                const bool loaded=part.map.status==OriginalMapStatus::LoadedTerrain;catalog.cityMaps+=loaded;
                catalog.sources.push_back({OriginalMapSourceKind::PakCity,path,path+" · 包内城市 "+std::to_string(ordinal)+(loaded?"":"（字段不兼容）"),part.index});
            }
        }
        std::sort(saves.begin(),saves.end());catalog.saveFiles=saves.size();
        for(const auto& path:saves)catalog.sources.push_back({OriginalMapSourceKind::SaveFile,path,path+" · SAV 快照",0});
        catalog.complete=true;
    }catch(const std::bad_alloc&){catalog.error="地图来源内存预算超限";catalog.sources.clear();}
    return catalog;
}
OriginalMapSelection loadOriginalMapSource(const ResourceRoot& root,const OriginalMapSource& source){
    OriginalMapSelection selected;const auto path=root.resolve(source.relativePath);
    if(path.empty()){selected.sourceInfo="所选来源不可读取或越出数据目录";return selected;}
    if(source.kind==OriginalMapSourceKind::MapFile){selected.map=loadOriginalMapTerrain(path);return selected;}
    if(source.kind==OriginalMapSourceKind::SaveFile){
        auto save=loadOriginalSave(path);selected.sourceInfo=describeOriginalSave(save)+"\n";
        if(save.status!=OriginalSaveStatus::LoadedSnapshot){
            selected.map.status=save.status==OriginalSaveStatus::BudgetExceeded?OriginalMapStatus::BudgetExceeded:save.status==OriginalSaveStatus::UnsupportedRole?OriginalMapStatus::UnsupportedRole:save.status==OriginalSaveStatus::InvalidContainer?OriginalMapStatus::InvalidContainer:save.status==OriginalSaveStatus::ReadFailure||save.status==OriginalSaveStatus::ChangedDuringRead?OriginalMapStatus::ReadFailure:OriginalMapStatus::InvalidGrid;return selected;
        }
        selected.map.status=OriginalMapStatus::LoadedTerrain;selected.map.terrain=std::move(save.terrain);
        selected.map.inputBytes=save.inputBytes;selected.map.decodedBytes=save.decodedBytes;selected.map.frames=save.frames;
        selected.saveLayers=std::move(save.layers);return selected;
    }

    const auto package=loadOriginalPak(path);selected.sourceInfo=describeOriginalPak(package)+"\n";
    selected.packageSummary=source.kind==OriginalMapSourceKind::PakSummary;
    if(package.status!=OriginalPakStatus::Loaded){selected.map.status=mapFailure(package.status);return selected;}
    if(selected.packageSummary)return selected;
    if(source.partIndex>=package.parts.size()||!package.parts[source.partIndex].cityCandidate){selected.map.status=OriginalMapStatus::UnsupportedRole;selected.sourceInfo="所选包内城市已不存在或并非城市负载；旧图已清除。\n";return selected;}
    selected.sourceInfo="来源："+source.relativePath+" · 包内分段 "+std::to_string(source.partIndex+1)+"；战役/任务语义未解释。\n";
    selected.map=package.parts[source.partIndex].map;
    return selected;
}
}
