// SPDX-License-Identifier: MIT
// Fully authored, deterministic scenes. No original asset bytes are read.
#pragma once
#include "cpu_reference.hpp"
#include <functional>

namespace msp {
inline uint32_t bgra(uint32_t b,uint32_t g,uint32_t r,uint32_t a){return b|(g<<8)|(r<<16)|(a<<24);}
inline Sprite solid(uint32_t w,uint32_t h,uint32_t pixel){return {w,h,std::vector<uint32_t>(size_t(w)*h,pixel)};}

// A hand-checked expectation, independent of cpuCompose.
struct PixelExpectation { uint32_t x,y,pixel; const char* what; };
struct AuthoredScene {
    Scene scene;
    std::vector<PixelExpectation> expected;
    // Optional exhaustive formula check (alpha scene): returns mismatches.
    std::function<size_t(const std::vector<uint32_t>&)> oracle;
    std::function<std::string(uint32_t x,uint32_t y)> describe;
};

// 1. Painter order: depth sort, equal-depth ties keep submission order, and
// overlapping opaque/translucent sprites compose in that order.
inline AuthoredScene paintOrderScene(){
    AuthoredScene out;auto& s=out.scene;s.name="paint_order";s.width=48;s.height=32;s.scale=1;
    s.initial.assign(size_t(s.width)*s.height,bgra(10,20,30,255));
    s.sprites={solid(20,20,bgra(0,0,255,255)),    // 0 opaque red
               solid(20,20,bgra(0,255,0,255)),    // 1 opaque green
               solid(20,20,bgra(255,0,0,255)),    // 2 opaque blue
               solid(20,20,bgra(0,0,128,128))};   // 3 half red, premultiplied
    // Submitted out of depth order. Blue and green tie at depth 5: blue first.
    // Coverage: red d9 x4..23,y4..23; blue d5 x10..29,y8..27; green d5
    // x14..33,y10..29; half-red d7 x18..37,y6..25; red d1 x30..47(clipped),y2..21.
    s.items={{0,4,4,9},{2,10,8,5},{1,14,10,5},{3,18,6,7},{0,30,2,1}};
    const uint32_t red=bgra(0,0,255,255),green=bgra(0,255,0,255),blue=bgra(255,0,0,255);
    const uint32_t k=(255*127+127)/255; // 127: 255 scaled by (255-128)/255, rounded
    out.expected={
        {5,5,red,"red d9 alone"},
        {11,25,blue,"blue d5 alone"},
        {15,26,green,"equal-depth tie: green submitted after blue wins"},
        {30,27,green,"green d5 over background"},
        {28,15,bgra(0,k,128,128+k),"half-red d7 over green d5"},
        {35,10,bgra(0,0,128+k,128+k),"half-red d7 over red d1"},
        {40,3,red,"red d1 alone"},
        {20,20,red,"red d9 drawn last over all others"},
        {1,1,bgra(10,20,30,255),"background untouched"},
    };
    return out;
}

// 2. Exhaustive premultiplied blend: every (a,c<=a,d) for B/G/R and every
// (a,d) for A, drawn once at scale 1 over an authored destination.
inline AuthoredScene alphaExhaustiveScene(){
    AuthoredScene out;auto& s=out.scene;s.name="alpha_exhaustive";s.scale=1;
    std::vector<uint32_t> src,dst;
    for(uint32_t a=0;a<256;++a){
        std::vector<std::pair<uint32_t,uint32_t>> pairs;
        for(uint32_t c=0;c<=a;++c)for(uint32_t d=0;d<256;++d)pairs.push_back({c,d});
        for(size_t k=0;k<pairs.size();k+=3){
            const auto p0=pairs[k],p1=pairs[std::min(k+1,pairs.size()-1)],p2=pairs[std::min(k+2,pairs.size()-1)];
            const uint32_t dA=uint32_t((k/3)%256);
            src.push_back(bgra(p0.first,p1.first,p2.first,a));
            dst.push_back(bgra(p0.second,p1.second,p2.second,dA));
        }
    }
    s.width=2048;s.height=uint32_t((src.size()+s.width-1)/s.width);
    src.resize(size_t(s.width)*s.height,0);dst.resize(size_t(s.width)*s.height,0);
    s.initial=dst;s.sprites={{s.width,s.height,src}};s.items={{0,0,0,0}};
    const auto srcCopy=src,dstCopy=dst;
    out.oracle=[srcCopy,dstCopy](const std::vector<uint32_t>& result){
        size_t bad=0;
        for(size_t i=0;i<result.size();++i){const uint32_t a=srcCopy[i]>>24;
            for(unsigned shift:{0u,8u,16u,24u}){
                const uint32_t c=channel(srcCopy[i],shift),d=channel(dstCopy[i],shift);
                bad+=channel(result[i],shift)!=blendChannelOracle(c,d,a);
            }}
        return bad;
    };
    out.describe=[srcCopy,dstCopy,w=s.width](uint32_t x,uint32_t y){
        const size_t i=size_t(y)*w+x;char text[128];
        snprintf(text,sizeof text,"src=%08x dst=%08x",srcCopy[i],dstCopy[i]);return std::string(text);
    };
    return out;
}

// 3. Nearest integer scale, negative/edge clipping, integer pixel placement
// and texture orientation. Each texel of an asymmetric 5x3 sprite is unique;
// row 0 is the top row, column 0 the left column.
inline Sprite orientationSprite(){
    Sprite sp{5,3,{}};
    for(uint32_t y=0;y<3;++y)for(uint32_t x=0;x<5;++x){
        const uint32_t a=(x==4&&y==2)?128:255;          // one translucent texel
        const uint32_t b=std::min(a,x*50+5),g=std::min(a,y*100+7),r=std::min(a,uint32_t(200));
        sp.pixels.push_back(bgra(b,g,r,a));
    }
    sp.pixels[size_t(1)*5+2]=0;                          // one fully transparent texel
    return sp;
}
inline AuthoredScene scaleClipScene(uint32_t scale){
    AuthoredScene out;auto& s=out.scene;s.name="scale_clip_x"+std::to_string(scale);
    // Odd sizes on purpose; large enough that the three visible copies never
    // overlap at any scale 1..4 (inside copy spans at most x12..31,y8..19).
    s.width=61;s.height=41;s.scale=scale;
    s.initial.assign(size_t(s.width)*s.height,bgra(40,40,40,255));
    s.sprites={orientationSprite()};
    const int w=5*int(scale),h=3*int(scale);
    s.items={
        {0,-int(scale)*2-1,-int(scale),0},                // negative: left 2 texels+1px and top row clipped
        {0,int(s.width)-w+int(scale),int(s.height)-h+1,1},// right column and bottom pixel row clipped
        {0,12,8,2},                                       // fully inside
        {0,-w,0,3},{0,int(s.width),0,4},{0,0,-h,5},{0,0,int(s.height),6}, // exactly outside: no pixels
        {0,-4096,-4096,7},{0,4096,4096,8},                // far outside, at the validated bounds
    };
    const auto& sp=s.sprites[0];
    // Fully-inside copy at (12,8): every covered pixel equals its texel
    // (opaque texels) and the transparent texel leaves the background.
    for(uint32_t ty=0;ty<3;++ty)for(uint32_t tx=0;tx<5;++tx)for(uint32_t k=0;k<scale;++k){
        const uint32_t texel=sp.pixels[size_t(ty)*5+tx];
        if(texel>>24!=255&&texel)continue;                // translucent checked by the CPU comparison
        out.expected.push_back({12+tx*scale+k,8+ty*scale+(scale-1-k),texel?texel:bgra(40,40,40,255),
            ty==0&&tx==0?"top-left texel at origin (orientation)":texel?"nearest scaled texel":"transparent texel keeps background"});
    }
    // Negative copy: pixel (0,0) samples texel x=(0-left)/scale, y=(0-top)/scale.
    {const int left=-int(scale)*2-1,top=-int(scale);
     const uint32_t tx=uint32_t((0-left)/int(scale)),ty=uint32_t((0-top)/int(scale));
     const uint32_t texel=sp.pixels[size_t(ty)*5+tx]; // scale>=2 lands on the transparent texel (2,1)
     out.expected.push_back({0,0,texel?texel:bgra(40,40,40,255),"negative origin samples floor((0-left)/scale)"});}
    return out;
}

// 4. Larger randomized scene (seeded LCG): many overlapping sprites with ties,
// alpha 0/255/partial runs, scale 2, partially off-target positions.
inline AuthoredScene stressScene(uint32_t count=4000){
    AuthoredScene out;auto& s=out.scene;s.name="stress_"+std::to_string(count);
    s.width=816;s.height=620;s.scale=2;s.initial.assign(size_t(s.width)*s.height,0);
    uint64_t state=0x9E3779B97F4A7C15ull;
    const auto next=[&](uint32_t bound){state=state*6364136223846793005ull+1442695040888963407ull;return uint32_t((state>>33)%bound);};
    for(int i=0;i<32;++i){
        Sprite sp{8+next(57),8+next(89),{}};
        for(size_t p=0;p<size_t(sp.width)*sp.height;++p){
            const uint32_t kind=next(4);
            const uint32_t a=kind==0?0:kind==1?255:1+next(254);
            sp.pixels.push_back(bgra(a?next(a+1):0,a?next(a+1):0,a?next(a+1):0,a));
        }
        s.sprites.push_back(std::move(sp));
    }
    for(uint32_t i=0;i<count;++i)
        s.items.push_back({next(32),int32_t(next(s.width/2+120))-60,int32_t(next(s.height/2+120))-60,uint64_t(next(64))});
    return out;
}
} // namespace msp
