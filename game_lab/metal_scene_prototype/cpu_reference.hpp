// SPDX-License-Identifier: MIT
// CPU reference for the isolated Metal sprite prototype. Self-contained: no
// game_lab headers, no original assets. The blend formula is copied verbatim
// from original_building_scene.hpp (overlay replay) and map_reference.cpp
// (grid renderer) and must not be changed to match any GPU result.
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace msp {
// Packed little-endian BGRA8, premultiplied: B bits0-7, G 8-15, R 16-23, A 24-31.
// In memory this is B,G,R,A bytes, i.e. MTLPixelFormatBGRA8Unorm and
// kCGBitmapByteOrder32Little|kCGImageAlphaPremultipliedFirst.
struct Sprite { uint32_t width=0,height=0; std::vector<uint32_t> pixels; };
struct DrawItem { uint32_t sprite=0; int32_t left=0,top=0; uint64_t depth=0; };
struct Scene {
    std::string name;
    uint32_t width=0,height=0,scale=1;
    std::vector<uint32_t> initial;          // target contents before drawing
    std::vector<Sprite> sprites;
    std::vector<DrawItem> items;            // submission order; stable-sorted by depth
};

inline uint32_t channel(uint32_t pixel,unsigned shift){return (pixel>>shift)&255u;}
inline bool validPremultiplied(uint32_t p){
    const uint32_t a=p>>24;return channel(p,0)<=a&&channel(p,8)<=a&&channel(p,16)<=a;
}
// Same bounds family as the existing renderers (+-4096 offsets, <=4096 sprites,
// <=4096 targets, scale 1..4). Invalid premultiplied input is rejected: the CPU
// formula would carry into the next channel there, while a GPU clamps.
inline std::string validateScene(const Scene& s){
    if(!s.width||!s.height||s.width>4096||s.height>4096)return "target size";
    if(s.scale<1||s.scale>4)return "scale";
    if(s.initial.size()!=size_t(s.width)*s.height)return "initial size";
    for(const auto& sprite:s.sprites){
        if(!sprite.width||!sprite.height||sprite.width>4096||sprite.height>4096)return "sprite size";
        if(sprite.pixels.size()!=size_t(sprite.width)*sprite.height)return "sprite pixels";
        for(uint32_t p:sprite.pixels)if(!validPremultiplied(p))return "sprite not premultiplied";
    }
    for(const auto& item:s.items){
        if(item.sprite>=s.sprites.size())return "sprite index";
        if(item.left<-4096||item.left>4096||item.top<-4096||item.top>4096)return "item offset";
    }
    return {};
}
inline std::vector<DrawItem> paintOrder(const Scene& s){
    auto order=s.items;
    std::stable_sort(order.begin(),order.end(),[](const DrawItem& a,const DrawItem& b){return a.depth<b.depth;});
    return order;
}
// Verbatim: mixed|=(((pixel>>shift)&255)+(((dst>>shift)&255)*(255-alpha)+127)/255)<<shift;
// alpha==0 is skipped by both existing renderers; alpha==255 reduces to src.
inline uint32_t blendOver(uint32_t src,uint32_t dst){
    const uint32_t alpha=src>>24;if(!alpha)return dst;
    uint32_t mixed=0;
    for(unsigned shift:{0u,8u,16u,24u})mixed|=(((src>>shift)&255)+(((dst>>shift)&255)*(255-alpha)+127)/255)<<shift;
    return mixed;
}
// Same loop structure as renderOriginalSceneOverlays: whole-sprite cull, then
// per-texel nearest integer scale with per-pixel target clipping.
inline std::vector<uint32_t> cpuCompose(const Scene& s){
    auto canvas=s.initial;const int scale=int(s.scale);
    for(const auto& item:paintOrder(s)){
        const auto& src=s.sprites[item.sprite];
        if(item.left>=int(s.width)||item.top>=int(s.height)||
           item.left+int(src.width)*scale<=0||item.top+int(src.height)*scale<=0)continue;
        for(uint32_t y=0;y<src.height;++y)for(uint32_t x=0;x<src.width;++x){
            const uint32_t pixel=src.pixels[size_t(y)*src.width+x];if(!(pixel>>24))continue;
            for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx){
                const int xx=item.left+int(x)*scale+sx,yy=item.top+int(y)*scale+sy;
                if(xx<0||yy<0||xx>=int(s.width)||yy>=int(s.height))continue;
                auto& dst=canvas[size_t(yy)*s.width+size_t(xx)];dst=blendOver(pixel,dst);
            }
        }
    }
    return canvas;
}
// Independent mathematical oracle for one channel: c + round_half_up(d*(255-a)/255).
inline uint32_t blendChannelOracle(uint32_t c,uint32_t d,uint32_t a){
    if(!a)return d;
    const uint64_t numerator=uint64_t(d)*(255-a);
    return c+uint32_t((2*numerator+255)/510);
}
} // namespace msp
