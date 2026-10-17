#include "map_reference.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace game_lab {
MapReferenceResult renderMapReferenceViewport(const OriginalMapTerrain& terrain,const OriginalMapRawLayer& raw,
        const SG3Archive& archive,const std::string& internal555,int64_t base,const OriginalMapCamera& camera) {
    MapReferenceResult result;const auto activity=analyzeOriginalMap(terrain);size_t pixels=0;
    if(!raw.valid()||!activity.cropValid()||base < -int64_t(UINT32_MAX)||base>int64_t(UINT32_MAX)||
       !std::isfinite(camera.centerX)||!std::isfinite(camera.centerY)||camera.centerX<0||camera.centerY<0||
       camera.centerX>=activity.cropSide||camera.centerY>=activity.cropSide||camera.scale<1||camera.scale>4||
       camera.width>2048||camera.height>2048||!imagePixelCount(camera.width,camera.height,pixels)||pixels>2048u*2048u||
       !archive.numImages||archive.numImages!=archive.sprites.size())return result;
    try {
        result.image.width=camera.width;result.image.height=camera.height;result.image.pixelsBGRA.assign(pixels,0);
        std::unordered_map<size_t,DecodedImage> cache;const int scale=int(camera.scale);
        // Authored flat diagnostic painter order. This is not an original-game contract.
        for(uint32_t depth=0;depth<2*activity.cropSide-1;++depth)for(uint32_t x=0;x<activity.cropSide;++x) {
            if(depth<x||depth-x>=activity.cropSide)continue;const uint32_t y=depth-x;
            const size_t cell=size_t(y+activity.cropOrigin)*228+x+activity.cropOrigin;
            if(terrain.flags[cell]&0x80000u)continue;++result.stats.activeCells;
            const int64_t candidate=int64_t(raw.values[cell])-base;
            if(candidate<=0||uint64_t(candidate)>archive.numImages){++result.stats.outsideArchive;continue;}
            ++result.stats.validIDs;const size_t index=size_t(candidate-1);const auto geometry=sg3SingleCellGeometry(archive,index);
            if(!geometry||geometry->height>4096){++result.stats.unknownGeometry;continue;}
            ++result.stats.geometrySupported;
            const double dx=double(x)-camera.centerX,dy=double(y)-camera.centerY;
            const double px=(dx-dy)*40*scale+double(camera.width)/2,py=(dx+dy)*20*scale+double(camera.height)/2;
            const int left=int(std::floor(px+.5))-39*scale,top=int(std::floor(py+.5))-(int(geometry->height)-20)*scale;
            if(left>=int(camera.width)||top>=int(camera.height)||left+78*scale<=0||top+int(geometry->height)*scale<=0){++result.stats.culled;continue;}
            ++result.stats.visibleCells;
            auto found=cache.find(index);
            if(found==cache.end()) {
                auto decoded=decodeSG3Sprite(archive,index,internal555);
                if(!decoded.image.valid()){++result.stats.decodeFailures;continue;}
                const size_t bytes=decoded.image.pixelsBGRA.size()*4;
                if(bytes>64u*1024u*1024u-result.stats.cacheBytes){result.image={};return result;}
                result.stats.cacheBytes+=bytes;found=cache.emplace(index,std::move(decoded.image)).first;++result.stats.uniqueDecoded;
            }
            const auto& image=found->second;++result.stats.drawnCells;
            for(uint32_t row=0;row<image.height;++row)for(uint32_t col=0;col<image.width;++col) {
                const uint32_t source=image.pixelsBGRA[size_t(row)*image.width+col];const uint32_t alpha=source>>24;if(!alpha)continue;
                for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx) {
                    const int xx=left+int(col)*scale+sx,yy=top+int(row)*scale+sy;
                    if(xx<0||yy<0||xx>=int(camera.width)||yy>=int(camera.height))continue;
                    auto& target=result.image.pixelsBGRA[size_t(yy)*camera.width+size_t(xx)];
                    if(alpha==255){target=source;continue;}
                    uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u}) {
                        const uint32_t src=(source>>shift)&255,dst=(target>>shift)&255;
                        mixed|=(src+(dst*(255-alpha)+127)/255)<<shift;
                    }target=mixed;
                }
            }
        }
        return result;
    }catch(const std::bad_alloc&){return {};}
}
}

namespace game_lab {
DecodedImage decodeSG3DiagnosticComponent(const SG3Archive& archive,size_t index,
        const std::string& internal,SG3DiagnosticComponent component) {
    if(archive.numImages!=archive.sprites.size() || !sg3SingleCellGeometry(archive,index))return {};
    const auto& record=archive.sprites[index];
    if(record.mirrorOffset || record.external || record.alphaLength)return {};
    auto full=decodeSG3Sprite(archive,index,internal).image;
    if(!full.valid())return {};
    if(component==SG3DiagnosticComponent::Full)return full;
    if(component!=SG3DiagnosticComponent::Top)return {};
    // Reuse the native public RLE reader at the original stream boundary.
    // No subtraction of base alpha: a natural top may overlap its own base.
    if(record.length==record.uncompressedLength) {
        DecodedImage empty;empty.width=78;empty.height=uint32_t(record.height);
        empty.pixelsBGRA.assign(size_t(empty.width)*empty.height,0);return empty;
    }
    if(uint64_t(record.offset)+record.uncompressedLength>UINT32_MAX)return {};
    auto topArchive=archive;auto& top=topArchive.sprites[index];
    top.type=256;top.offset+=top.uncompressedLength;top.length-=top.uncompressedLength;
    top.uncompressedLength=0;
    return decodeSG3Sprite(topArchive,index,internal).image;
}
std::optional<OriginalMapPoint> projectSG3DiagnosticCell(const OriginalMapTerrain& terrain,
        const SG3DiagnosticRegistration& registration,int64_t x,int64_t y) {
    const auto crop=analyzeOriginalMap(terrain);
    if(!crop.cropValid() || x<0 || y<0 || x>=crop.cropSide || y>=crop.cropSide)return std::nullopt;
    const auto cell=size_t(y+crop.cropOrigin)*OriginalMapTerrain::storageSide+size_t(x+crop.cropOrigin);
    if(terrain.flags[cell]&0x80000u)return std::nullopt;
    const auto& m=registration.matrix;
    for(size_t row=0;row<2;++row) {
        for(size_t col=0;col<2;++col)if(m[row][col]<-4096 || m[row][col]>4096)return std::nullopt;
        if(m[row][2]<-1000000 || m[row][2]>1000000 ||
           registration.canvasOrigin[row]<-1000000 || registration.canvasOrigin[row]>1000000)return std::nullopt;
    }
    // Operand bounds above make all products and sums safe in int64_t.
    if(m[0][0]*m[1][1]-m[0][1]*m[1][0]==0)return std::nullopt;
    const auto px=m[0][0]*x+m[0][1]*y+m[0][2]-registration.canvasOrigin[0];
    const auto py=m[1][0]*x+m[1][1]*y+m[1][2]-registration.canvasOrigin[1];
    if(px<-1000000 || px>1000000 || py<-1000000 || py>1000000)return std::nullopt;
    return OriginalMapPoint{double(px),double(py)};
}
SG3DiagnosticResult renderSG3DiagnosticScene(const SG3Archive& archive,const std::string& internal,
        const SG3DiagnosticScene& scene) {
    size_t count=0;
    if(scene.width>2048 || scene.height>2048 || !imagePixelCount(scene.width,scene.height,count) ||
       scene.scale<1 || scene.scale>4 || scene.layers.empty() || scene.layers.size()>128)return {};
    try {
        SG3DiagnosticResult out;out.image.width=scene.width;out.image.height=scene.height;
        out.image.pixelsBGRA.assign(count,0);out.producer.assign(count,0);
        out.visiblePerLayer.assign(scene.layers.size(),0);
        for(size_t layer=0;layer<scene.layers.size();++layer) {
            const auto& spec=scene.layers[layer];
            if(!spec.logicalID || spec.logicalID>archive.numImages)return {};
            auto source=decodeSG3DiagnosticComponent(archive,spec.logicalID-1,internal,spec.component);
            if(!source.valid())return {};
            for(auto pixel:source.pixelsBGRA)if((pixel>>24)!=0 && (pixel>>24)!=255)return {};
            SG3CanvasPlacement placement;placement.width=scene.width;placement.height=scene.height;
            placement.scale=scene.scale;placement.centerX=spec.centerX;placement.centerY=spec.centerY;
            auto canvas=renderSG3SingleCellCanvas(source,placement);
            if(!canvas.valid())return {};
            ++out.decodedLayers;
            for(size_t p=0;p<count;++p)if(canvas.pixelsBGRA[p]>>24) {
                out.image.pixelsBGRA[p]=canvas.pixelsBGRA[p];out.producer[p]=uint16_t(layer+1);
            }
        }
        for(auto producer:out.producer)if(producer){++out.coveredPixels;++out.visiblePerLayer[producer-1];}
        return out;
    }catch(const std::bad_alloc&){return {};}
}
}

namespace game_lab {
std::vector<std::string> freshReferenceOutputs(const std::string& dataRoot,
        const std::vector<std::string>& requested) {
    namespace fs=std::filesystem;
    if(!fs::path(dataRoot).is_absolute())throw std::runtime_error("absolute data root required");
    const auto root=fs::canonical(dataRoot);
    std::vector<fs::path> paths;std::vector<std::string> targets;
    for(const auto& name:requested) {
        fs::path path(name);
        if(name.find('\0')!=std::string::npos || !path.is_absolute() || path.filename().empty() ||
           path.filename()=="." || path.filename()=="..")throw std::runtime_error("fresh absolute output file required");
        // Canonicalize the parent before lexical operations: symlink/.. follows
        // filesystem traversal, not merely string cancellation.
        const auto parent=fs::canonical(path.parent_path());
        if(!fs::is_directory(parent))throw std::runtime_error("output parent must be a directory");
        // Existing directory identities also protect case/normalization aliases
        // of the data-root path; lexical prefix spelling is not the boundary.
        for(auto ancestor=parent;;ancestor=ancestor.parent_path()) {
            if(fs::equivalent(ancestor,root))throw std::runtime_error("output inside data root refused");
            if(ancestor==ancestor.parent_path())break;
        }
        const auto target=(parent/path.filename()).string();
        for(size_t i=0;i<targets.size();++i) {
            std::error_code error;
            const bool sameExistingFile=fs::equivalent(path,paths[i],error);
            if(target==targets[i] || (!error && sameExistingFile))
                throw std::runtime_error("distinct PNG/producer output targets required");
        }
        paths.push_back(path);targets.push_back(target);
    }
    // Check every destination before allowing either writer to create a file.
    // symlink_status also rejects dangling final symlinks.
    for(const auto& path:paths)
        if(fs::symlink_status(path).type()!=fs::file_type::not_found)
            throw std::runtime_error("fresh absolute output path required");
    return targets;
}
}

namespace game_lab {
ExclusiveReferenceOutputs::ExclusiveReferenceOutputs(const std::string& root,
        const std::vector<std::string>& requested) {
    const auto paths=freshReferenceOutputs(root,requested);
    files_.reserve(paths.size());
    try {
        for(const auto& name:paths) {
            const std::filesystem::path path(name);
            files_.emplace_back();auto& file=files_.back();file.leaf=path.filename().string();
            file.parent=::open(path.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);
            if(file.parent<0)throw std::runtime_error("output parent open failed");
            file.fd=::openat(file.parent,file.leaf.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
            if(file.fd<0)throw std::runtime_error("exclusive fresh output reservation refused;filesystem alias or existing target");
            struct stat identity{};
            if(::fstat(file.fd,&identity)!=0 || !S_ISREG(identity.st_mode))throw std::runtime_error("reserved output identity failed");
            file.device=uint64_t(identity.st_dev);file.inode=uint64_t(identity.st_ino);
        }
    }catch(...){cleanup();throw;}
}
ExclusiveReferenceOutputs::~ExclusiveReferenceOutputs(){cleanup();}
void ExclusiveReferenceOutputs::cleanup() noexcept {
    for(auto& file:files_) {
        if(!committed_ && file.parent>=0 && file.fd>=0) {
            struct stat current{};
            if(::fstatat(file.parent,file.leaf.c_str(),&current,AT_SYMLINK_NOFOLLOW)==0 &&
               uint64_t(current.st_dev)==file.device && uint64_t(current.st_ino)==file.inode)
                (void)::unlinkat(file.parent,file.leaf.c_str(),0);
        }
        if(file.fd>=0){(void)::close(file.fd);file.fd=-1;}
        if(file.parent>=0){(void)::close(file.parent);file.parent=-1;}
    }
}
void ExclusiveReferenceOutputs::write(size_t index,const uint8_t* bytes,size_t count) {
    if(committed_ || index>=files_.size() || (count && !bytes))throw std::runtime_error("invalid reserved output write");
    size_t offset=0;
    while(offset<count) {
        const auto amount=std::min(count-offset,size_t(65536));
        const auto written=::write(files_[index].fd,bytes+offset,amount);
        if(written<0 && errno==EINTR)continue;
        if(written<=0)throw std::runtime_error("reserved output write failed");
        offset+=size_t(written);
    }
}
void ExclusiveReferenceOutputs::commit() {
    if(committed_)throw std::runtime_error("output set already committed");
    for(const auto& file:files_) {
        struct stat current{};
        if(::fstatat(file.parent,file.leaf.c_str(),&current,AT_SYMLINK_NOFOLLOW)!=0 ||
           uint64_t(current.st_dev)!=file.device || uint64_t(current.st_ino)!=file.inode)
            throw std::runtime_error("reserved output name identity changed");
        int result;do{result=::fsync(file.fd);}while(result!=0 && errno==EINTR);
        if(result!=0)throw std::runtime_error("reserved output sync failed");
    }
    committed_=true;cleanup();
}
}
