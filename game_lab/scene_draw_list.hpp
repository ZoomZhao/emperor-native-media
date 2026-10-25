// SPDX-License-Identifier: MIT
#pragma once
#include "sg3_sprite_cache.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>
namespace game_lab {
// A synchronous, immutable-resource snapshot. Vector order IS painter order;
// no backend may re-sort it. Width/height are final integer destination sizes.
// Sprites first compose onto transparent black; that layer then goes over the
// numerical background. Combining these passes changes integer rounding.
struct SceneDrawItem {uint32_t sprite=0;int32_t left=0,top=0;uint32_t width=0,height=0;};
struct SceneDrawList {
    uint32_t width=0,height=0;
    DecodedImage background;
    std::vector<SG3DecodedSprite> sprites;
    std::vector<SceneDrawItem> items;
};
struct SceneDrawCapture {SceneDrawList list;std::string error="not_captured";bool complete()const{return error.empty();}};
constexpr size_t sceneDrawItemLimit=131072,sceneDrawResourceLimit=16384,sceneDrawPixelBudget=128u*1024u*1024u;
inline bool scenePremultiplied(uint32_t p){const auto a=p>>24;return (p&255)<=a&&((p>>8)&255)<=a&&((p>>16)&255)<=a;}
class SceneDrawListBuilder {
    std::map<uint64_t,uint32_t> indices_;size_t bytes_=0;
public:
    SceneDrawList list;std::string error;
    SceneDrawListBuilder(uint32_t width,uint32_t height){list.width=width;list.height=height;}
    void resetLayer(){list.items.clear();list.sprites.clear();indices_.clear();bytes_=0;}
    void append(const SG3DecodedSprite& resource,int left,int top,unsigned scale){
        if(!error.empty())return;
        if(!resource.identity||!resource.value||!resource.value->image.valid()){error="invalid_sprite_resource";return;}
        const auto& image=resource.value->image;
        if(scale<1||scale>4||image.width>4096||image.height>4096){error="sprite_geometry";return;}
        if(list.items.size()>=sceneDrawItemLimit){error="draw_item_budget";return;}
        auto found=indices_.find(resource.identity);
        if(found==indices_.end()){
            const size_t bytes=image.pixelsBGRA.capacity()*4;
            if(list.sprites.size()>=sceneDrawResourceLimit||bytes>sceneDrawPixelBudget-bytes_){error="draw_resource_budget";return;}
            for(auto p:image.pixelsBGRA)if(!scenePremultiplied(p)){error="sprite_not_premultiplied";return;}
            found=indices_.emplace(resource.identity,uint32_t(list.sprites.size())).first;
            list.sprites.push_back(resource);bytes_+=bytes;
        }else if(list.sprites[found->second].value!=resource.value){error="resource_identity_collision";return;}
        list.items.push_back({found->second,left,top,image.width*scale,image.height*scale});
    }
};
inline std::string validateSceneDrawList(const SceneDrawList& s){
    if(!s.width||!s.height||s.width>2048||s.height>2048||!s.background.valid()||s.background.width!=s.width||s.background.height!=s.height)return "draw_target";
    if(s.items.size()>sceneDrawItemLimit||s.sprites.size()>sceneDrawResourceLimit)return "draw_budget";
    size_t bytes=0;std::set<uint64_t> ids;
    for(const auto& r:s.sprites){
        if(!r.identity||!r.value||!r.value->image.valid()||!ids.insert(r.identity).second)return "draw_resource";
        const auto& im=r.value->image;const size_t n=im.pixelsBGRA.capacity()*4;
        if(im.width>4096||im.height>4096||n>sceneDrawPixelBudget-bytes)return "draw_resource_budget";bytes+=n;
        for(auto p:im.pixelsBGRA)if(!scenePremultiplied(p))return "sprite_not_premultiplied";
    }
    for(const auto& i:s.items){
        if(i.sprite>=s.sprites.size())return "draw_resource_index";
        const auto& im=s.sprites[i.sprite].value->image;
        if(!i.width||!i.height||i.width%im.width||i.height%im.height||i.width/im.width!=i.height/im.height||i.width/im.width>4)return "draw_scale";
        if(i.left < -1000000||i.left>1000000||i.top < -1000000||i.top>1000000)return "draw_origin";
    }
    return {};
}
inline uint32_t sceneBlendOver(uint32_t pixel,uint32_t dst){
    const uint32_t a=pixel>>24;if(!a)return dst;uint32_t mixed=0;
    for(unsigned shift:{0u,8u,16u,24u})mixed|=(((pixel>>shift)&255)+(((dst>>shift)&255)*(255-a)+127)/255)<<shift;
    return mixed;
}
inline DecodedImage rasterizeSceneDrawListCPU(const SceneDrawList& s){
    if(!validateSceneDrawList(s).empty())return {};
    DecodedImage layer;layer.width=s.width;layer.height=s.height;layer.pixelsBGRA.resize(size_t(s.width)*s.height);
    for(const auto& item:s.items){
        const auto& im=s.sprites[item.sprite].value->image;const int scale=int(item.width/im.width);
        if(item.left>=int(s.width)||item.top>=int(s.height)||int64_t(item.left)+item.width<=0||int64_t(item.top)+item.height<=0)continue;
        for(unsigned y=0;y<im.height;++y)for(unsigned x=0;x<im.width;++x){
            const uint32_t p=im.pixelsBGRA[size_t(y)*im.width+x];if(!(p>>24))continue;
            for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx){
                const int xx=item.left+int(x)*scale+sx,yy=item.top+int(y)*scale+sy;
                if(xx<0||yy<0||xx>=int(s.width)||yy>=int(s.height))continue;
                auto& dst=layer.pixelsBGRA[size_t(yy)*s.width+size_t(xx)];dst=sceneBlendOver(p,dst);
            }
        }
    }
    for(size_t i=0;i<layer.pixelsBGRA.size();++i)layer.pixelsBGRA[i]=sceneBlendOver(layer.pixelsBGRA[i],s.background.pixelsBGRA[i]);
    return layer;
}
} // namespace game_lab
