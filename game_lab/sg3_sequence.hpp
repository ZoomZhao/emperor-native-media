#pragma once
#include "emperor_assets.hpp"
#include <charconv>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <new>

namespace game_lab {
// Caller-selected physical record IDs. No frame-chain, direction, count,
// reversible endpoint or speed-ID semantics are inferred from SG metadata.
inline std::optional<std::vector<uint32_t>> parseSG3SequenceIds(const std::string& text,size_t maxId){
    if(text.empty()||text.size()>4096||!maxId)return std::nullopt;
    std::vector<uint32_t> ids;size_t at=0;
    while(at<text.size()){
        while(at<text.size()&&std::isspace(static_cast<unsigned char>(text[at])))++at;
        const size_t start=at;while(at<text.size()&&text[at]>='0'&&text[at]<='9')++at;
        uint32_t id=0;const auto parsed=std::from_chars(text.data()+start,text.data()+at,id);
        if(start==at||parsed.ec!=std::errc{}||!id||id>maxId||ids.size()==256)return std::nullopt;
        ids.push_back(id);while(at<text.size()&&std::isspace(static_cast<unsigned char>(text[at])))++at;
        if(at==text.size())return ids;
        if(text[at++]!=','||at==text.size())return std::nullopt;
    }return std::nullopt;
}
struct SG3Sequence {
    std::vector<uint32_t> physicalIds;
    std::vector<DecodedImage> frames;
    bool offsetAligned=false;
    size_t cacheBytes=0;
    std::string error;
    bool valid() const {return !frames.empty()&&frames.size()==physicalIds.size()&&error.empty();}
};
inline size_t sg3SequenceStep(size_t current,size_t count,bool backwards=false){
    if(!count||current>=count)return 0;
    return backwards?(current?current-1:count-1):(current+1==count?0:current+1);
}
inline bool validSG3PreviewRate(double rate){return std::isfinite(rate)&&rate>=1&&rate<=30;}
// Preview convention only: place each natural canvas at (-raw offsetX,-raw
// offsetY), preserving a common bounding rectangle for the entire list. This
// is explicit asset alignment, not proof of an original world draw anchor.
inline SG3Sequence prepareSG3Sequence(const SG3Archive& archive,const std::vector<uint32_t>& ids,
                                     const std::string& companion,bool align){
    const auto failure=[](const char* error){SG3Sequence r;r.error=error;return r;};
    if(ids.empty()||ids.size()>256)return failure("sequence requires1..256 explicit physical IDs");
    try{
        std::vector<DecodedImage> decoded;decoded.reserve(ids.size());size_t sourceBytes=0;
        int minX=std::numeric_limits<int>::max(),minY=minX,maxX=std::numeric_limits<int>::min(),maxY=maxX;
        for(uint32_t id:ids){
            if(!id||id>archive.sprites.size())return failure("physical ID outside current archive");
            const auto& s=archive.sprites[id-1];auto image=decodeSG3Sprite(archive,id-1,companion).image;
            if(!image.valid())return failure("frame decode failed;no IDs skipped or compacted");
            const size_t bytes=image.pixelsBGRA.size()*4;
            if(bytes>32u*1024u*1024u-sourceBytes)return failure("source frame cache budget");
            sourceBytes+=bytes;const int x=align?-int(s.animationOffsetX):0,y=align?-int(s.animationOffsetY):0;
            minX=std::min(minX,x);minY=std::min(minY,y);maxX=std::max(maxX,x+int(image.width));maxY=std::max(maxY,y+int(image.height));decoded.push_back(std::move(image));
        }
        const int width=maxX-minX,height=maxY-minY;
        if(width<=0||height<=0||width>2048||height>2048||
            uint64_t(width)*height*4*ids.size()>32u*1024u*1024u)return failure("common canvas/frame cache budget");
        SG3Sequence result;result.physicalIds=ids;result.offsetAligned=align;
        result.cacheBytes=size_t(width)*height*4*ids.size();result.frames.reserve(ids.size());
        for(size_t i=0;i<ids.size();++i){
            const auto& s=archive.sprites[ids[i]-1];const auto& source=decoded[i];
            const int x=(align?-int(s.animationOffsetX):0)-minX,y=(align?-int(s.animationOffsetY):0)-minY;
            DecodedImage target;target.width=uint32_t(width);target.height=uint32_t(height);target.pixelsBGRA.resize(size_t(width)*height);
            for(size_t row=0;row<source.height;++row)std::copy_n(source.pixelsBGRA.data()+row*source.width,source.width,target.pixelsBGRA.data()+(row+size_t(y))*size_t(width)+size_t(x));
            result.frames.push_back(std::move(target));
        }return result;
    }catch(const std::bad_alloc&){return failure("sequence allocation budget");}
}
}
