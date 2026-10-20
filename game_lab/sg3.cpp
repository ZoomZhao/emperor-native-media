// Public SG format and algorithms adapted from Bianca van Schaik's MIT sgreader.
// See THIRD_PARTY_NOTICES.md and docs/SG3.md. No original-game code is used.
#include "emperor_assets.hpp"
#include "asset_bytes.hpp"
#include "resource_root.hpp"
#include <algorithm>
#include <filesystem>
#include <new>
#include <cmath>

namespace game_lab {
namespace {
constexpr size_t tableOffset = 80 + 600 + 200 * 200;
std::string lower(std::string s) {
    for(char& c:s) if(c>='A' && c<='Z')c=char(c-'A'+'a');
    return s;
}
uint32_t color555(uint16_t color, bool key) {
    if(key && color==0xf81f)return 0;
    auto expand=[](uint32_t n){return (n<<3)|(n>>2);};
    return 0xff000000u|(expand((color>>10)&31)<<16)|(expand((color>>5)&31)<<8)|expand(color&31);
}
// RLE omission at the end means transparent, not fabricated pixel content.
bool rle(DecodedImage& image,const uint8_t* data,size_t length,bool alpha) {
    size_t in=0,out=0;
    while(in<length) {
        const uint8_t n=data[in++];
        if(n==255) {
            if(in==length)return false;
            const size_t skip=data[in++];
            if(skip>image.pixelsBGRA.size()-out)return false;
            out+=skip;
        } else {
            const size_t stride=alpha?1:2;
            if(n>image.pixelsBGRA.size()-out || n>(length-in)/stride)return false;
            for(size_t j=0;j<n;++j,++out,in+=stride) {
                if(!alpha)image.pixelsBGRA[out]=color555(detail::u16(data+in),false);
                else {
                    const uint32_t a=((data[in]&31)<<3)|((data[in]&31)>>2);
                    const auto rgb=image.pixelsBGRA[out];
                    // CoreGraphics's shared converter requires premultiplied BGRA.
                    image.pixelsBGRA[out]=(a<<24)|((((rgb>>16)&255)*a/255)<<16)|
                        ((((rgb>>8)&255)*a/255)<<8)|((rgb&255)*a/255);
                }
            }
        }
    }
    return true;
}
bool isometric(DecodedImage& image,const SG3Sprite& s,const std::vector<uint8_t>& bytes) {
    // Emperor is always 78x40. Never infer 58x30 from an ambiguous 120px height.
    const uint32_t size=(image.width+2)/80;
    if(!size || image.width!=size*80-2 || image.height<size*40 ||
       (s.tileSize && s.tileSize!=size) || s.uncompressedLength!=uint64_t(size)*size*3200 ||
       s.uncompressedLength>bytes.size())return false;
    size_t input=0;
    const uint32_t baseY=image.height-size*40;
    for(uint32_t row=0;row<2*size-1;++row) {
        uint32_t tileX=(row<size?size-row-1:row-size+1)*40;
        const uint32_t columns=row<size?row+1:2*size-row-1;
        for(uint32_t col=0;col<columns;++col,tileX+=80) {
            for(uint32_t y=0;y<40;++y) {
                const uint32_t left=y<20?38-2*y:2*y-40;
                for(uint32_t x=left;x<78-left;++x) {
                    const uint32_t xx=tileX+x,yy=baseY+row*20+y;
                    if(xx>=image.width || yy>=image.height || input+2>s.uncompressedLength)return false;
                    image.pixelsBGRA[size_t(yy)*image.width+xx]=color555(detail::u16(bytes.data()+input),false);
                    input+=2;
                }
            }
        }
    }
    return input==s.uncompressedLength && rle(image,bytes.data()+input,bytes.size()-input,false);
}
SG3DecodeResult pixels(const SG3Sprite& s,const std::string& path) {
    SG3DecodeResult result;
    size_t count=0;
    if(s.width<=0 || s.height<=0 || !imagePixelCount(s.width,s.height,count) || !s.length ||
       s.external>1 || (s.external && !s.offset))return result;
    const bool plain=s.type==0 || s.type==1 || s.type==10 || s.type==12 || s.type==13 || s.type==20;
    const bool compressed=s.type==256 || s.type==257 || s.type==276;
    if(!plain && !compressed && s.type!=30){result.status=SG3DecodeStatus::Unsupported;return result;}
    std::vector<uint8_t> bytes;
    // The public external flag specifies a one-byte offset adjustment.
    if(!detail::readRange(path,size_t(s.offset)-s.external,s.length,bytes)) {
        result.status=SG3DecodeStatus::InvalidPayload;return result;
    }
    DecodedImage image;image.width=s.width;image.height=s.height;image.pixelsBGRA.assign(count,0);
    bool good=false;
    if(plain) {
        if(bytes.size()==count*2) {
            for(size_t i=0;i<count;++i)image.pixelsBGRA[i]=color555(detail::u16(bytes.data()+2*i),s.type==0);
            good=true;
        }
    } else if(compressed)good=rle(image,bytes.data(),bytes.size(),false);
    else good=isometric(image,s,bytes);
    if(!good){result.status=SG3DecodeStatus::InvalidPayload;return result;}
    if(s.alphaLength) {
        // Public sgreader reads alpha immediately after color. Corpus validation:
        // all 9,059 masks obey this RLE layout; interpreting the alpha field as
        // an absolute file offset fails 7,918. Preserve the field as metadata.
        std::vector<uint8_t> alpha;
        const uint64_t alphaPosition=uint64_t(s.offset)-s.external+s.length;
        if(alphaPosition>kMaxAssetBytes || !detail::readRange(path,size_t(alphaPosition),s.alphaLength,alpha) ||
           !rle(image,alpha.data(),alpha.size(),true)) {
            result.status=SG3DecodeStatus::InvalidPayload;return result;
        }
    }
    result.status=SG3DecodeStatus::Decoded;result.alphaApplied=s.alphaLength!=0;result.image=std::move(image);return result;
}
std::string companion(const SG3Archive& a,const SG3Sprite& s,const std::string& internal) {
    namespace fs=std::filesystem;
    fs::path name=fs::path(internal).filename();
    if(s.external) {
        if(s.bitmapId>=a.bitmaps.size())return {};
        name=fs::path(a.bitmaps[s.bitmapId].filename);
        if(name.empty() || name.has_root_path() || name.filename()!=name ||
           name.string().find_first_of("\\:")!=std::string::npos)return {};
        name.replace_extension(".555");
    }
    auto root=ResourceRoot::fromAbsolute(fs::path(internal).parent_path().string());
    if(!root.configured())return {};
    auto direct=root.resolve(name.string());if(!direct.empty())return direct;
    for(const auto& directory:std::vector<std::string>{"","555"}) {
        std::error_code error;
        auto dir=fs::path(root.configuredDirectory())/directory;
        for(fs::directory_iterator it(dir,error),end;!error && it!=end;it.increment(error)) {
            if(lower(it->path().filename().string())!=lower(name.string()))continue;
            auto relative=(fs::path(directory)/it->path().filename()).generic_string();
            auto resolved=root.resolve(relative);if(!resolved.empty())return resolved;
        }
    }
    return {};
}
}
SG3Archive loadSG3(const std::string& path) {
    std::vector<uint8_t> bytes;
    if(!detail::readFile(path,kMaxAssetBytes,bytes) || bytes.size()<tableOffset)return {};
    const uint32_t version=detail::u32(bytes.data()+4),capacity=detail::u32(bytes.data()+12),
        count=detail::u32(bytes.data()+16),bitmapCount=detail::u32(bytes.data()+20);
    const size_t recordSize=version==213?64:72;
    const uint32_t declaredSize=detail::u32(bytes.data());
    // Public compatibility value, present in two indexes in the tested corpus.
    if((version!=213 && version!=214) || (declaredSize!=74480 && declaredSize!=bytes.size()) ||
       !count || !capacity || count>=capacity || bitmapCount>200 ||
       uint64_t(count)+1>(bytes.size()-tableOffset)/recordSize)return {};
    try {
        SG3Archive a;a.fileSize=uint32_t(bytes.size());a.version=version;a.numFiles=bitmapCount;
        a.numImages=count;a.capacity=capacity;a.bitmaps.reserve(bitmapCount);a.sprites.reserve(count);
        for(size_t slot=0;slot<a.imageIndex.size();++slot)a.imageIndex[slot]=detail::u16(bytes.data()+80+2*slot);
        for(size_t i=0;i<bitmapCount;++i) {
            const char* name=reinterpret_cast<const char*>(bytes.data()+680+i*200);
            size_t n=0;while(n<65 && name[n])++n;
            if(n==65)return {};
            a.bitmaps.push_back({std::string(name,n)});
        }
        for(size_t id=1;id<=count;++id) {
            const auto* r=bytes.data()+tableOffset+id*recordSize;SG3Sprite s;
            s.offset=detail::u32(r);s.length=detail::u32(r+4);s.uncompressedLength=detail::u32(r+8);
            const auto mirror=detail::u32(r+16);
            s.mirrorOffset=mirror<=0x7fffffffu?int32_t(mirror):int32_t(int64_t(mirror)-0x100000000ll);
            s.width=detail::i16(r+20);s.height=detail::i16(r+22);
            s.animationSpriteCount=detail::u16(r+30);
            s.animationDirections=detail::u16(r+32);
            s.animationOffsetX=detail::i16(r+34);s.animationOffsetY=detail::i16(r+36);
            s.animationReversible=r[48];s.animationSpeedId=r[58];
            s.type=detail::u16(r+50);s.flags=detail::u16(r+52);s.external=r[52];s.tileSize=r[55];s.bitmapId=r[56];
            if(version==214){s.alphaOffset=detail::u32(r+64);s.alphaLength=detail::u32(r+68);}
            a.sprites.push_back(s);
        }
        return a;
    } catch(const std::bad_alloc&){return {};}
}
std::optional<size_t> sg3LogicalIndexForGroup(const SG3Archive& archive,size_t slot) {
    if(slot>=archive.imageIndex.size())return std::nullopt;
    const size_t id=archive.imageIndex[slot];
    if(!id || id>archive.numImages || id>archive.sprites.size())return std::nullopt;
    return id-1;
}
SG3DecodeResult decodeSG3Sprite(const SG3Archive& a,size_t index,const std::string& internal) {
    SG3DecodeResult result;
    try {
        if(index>=a.sprites.size())return result;
        bool mirrored=false;
        while(a.sprites[index].mirrorOffset) {
            const int64_t delta=a.sprites[index].mirrorOffset;
            // Only earlier logical IDs are permitted; forward references/cycles fail closed.
            if(delta>=0 || int64_t(index)+delta<0)return result;
            index=size_t(int64_t(index)+delta);mirrored=!mirrored;
        }
        const auto& s=a.sprites[index];
        // The public reader rejects zero dimensions/data as no image. Keep such
        // logical IDs enumerable, but never fabricate transparent decoded pixels.
        // Classify the exact empty shape before resolving pixel dependencies.
        if(s.width==0 && s.height==0 && s.length==0) {
            result.status=SG3DecodeStatus::EmptyRecord;result.mirrored=mirrored;return result;
        }
        auto path=companion(a,s,internal);
        if(path.empty()){result.status=SG3DecodeStatus::MissingDependency;return result;}
        result=pixels(s,path);result.mirrored=mirrored;
        if(result.image.valid() && mirrored)for(size_t y=0;y<result.image.height;++y) {
            auto first=result.image.pixelsBGRA.begin()+y*result.image.width;
            std::reverse(first,first+result.image.width);
        }
        return result;
    } catch(const std::bad_alloc&){return {};}
}
std::optional<SG3SingleCellGeometry> sg3SingleCellGeometry(const SG3Archive& a,size_t index) {
    if(index>=a.numImages||index>=a.sprites.size())return std::nullopt;
    while(a.sprites[index].mirrorOffset) {
        const int64_t delta=a.sprites[index].mirrorOffset;
        if(delta>=0||int64_t(index)+delta<0)return std::nullopt;
        index=size_t(int64_t(index)+delta);
    }
    const auto& s=a.sprites[index];
    if(s.type!=30||s.width!=78||s.height<40||s.uncompressedLength!=3200||s.length<s.uncompressedLength||
       (s.tileSize&&s.tileSize!=1))return std::nullopt;
    return SG3SingleCellGeometry{78,uint32_t(s.height),39,double(s.height)-20};
}
DecodedImage renderSG3SingleCellCanvas(const DecodedImage& source,const SG3CanvasPlacement& p) {
    size_t count=0;
    if(!source.valid()||source.width!=78||source.height<40||source.height>4096||p.scale<1||p.scale>4||
       p.width>2048||p.height>2048||!imagePixelCount(p.width,p.height,count)||count>2048u*2048u||
       !std::isfinite(p.centerX)||!std::isfinite(p.centerY)||std::abs(p.centerX)>1000000||std::abs(p.centerY)>1000000)return {};
    // Explicit bottom-base convention, supplied centre and sign-consistent ties.
    const int scale=int(p.scale),left=int(std::floor(p.centerX+.5))-39*scale,
        top=int(std::floor(p.centerY+.5))-(int(source.height)-20)*scale;
    try {
        DecodedImage out;out.width=p.width;out.height=p.height;out.pixelsBGRA.assign(count,0);
        for(uint32_t y=0;y<source.height;++y)for(uint32_t x=0;x<78;++x) {
            const auto pixel=source.pixelsBGRA[size_t(y)*78+x];if(!(pixel>>24))continue;
            for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx) {
                const int px=left+int(x)*scale+sx,py=top+int(y)*scale+sy;
                if(px>=0&&py>=0&&px<int(p.width)&&py<int(p.height))out.pixelsBGRA[size_t(py)*p.width+size_t(px)]=pixel;
            }
        }
        return out;
    }catch(const std::bad_alloc&){return {};}
}
DecodedImage extractSprite(const SG3Sprite& s,const std::string& path) {
    try {if(s.mirrorOffset || s.external)return {};return pixels(s,path).image;}
    catch(const std::bad_alloc&){return {};}
}
} // namespace game_lab
