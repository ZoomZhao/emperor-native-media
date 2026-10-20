#pragma once
#include "original_building_view.hpp"
#include "original_candidate_input.hpp"
#include <filesystem>
namespace game_lab {
// Snapshot revision token,not a security hash or an object/image binding.
inline uint64_t originalCandidateRevision(const uint8_t* data,size_t n){uint64_t h=14695981039346656037ull;for(size_t i=0;i<n;++i){h^=data[i];h*=1099511628211ull;}return h;}
inline uint64_t originalCandidateImageRevision(const DecodedImage& image){
    uint64_t h=14695981039346656037ull;const auto put=[&](uint32_t n){for(unsigned i=0;i<4;++i){h^=uint8_t(n>>(8*i));h*=1099511628211ull;}};
    put(image.width);put(image.height);for(auto p:image.pixelsBGRA)put(p);return h;
}
struct OriginalBuildingCandidate {
    std::string source,archive;uint64_t sourceRevision=0,imageRevision=0;uint32_t physicalId=0;
    OriginalBuildingFootprint target;DecodedImage pixels;
};
inline bool sameOriginalCandidateTarget(const OriginalBuildingFootprint& a,const OriginalBuildingFootprint& b){return a.slot==b.slot&&a.recordOffset==b.recordOffset&&a.type==b.type&&a.x==b.x&&a.y==b.y&&a.size==b.size&&originalFootprintWidth(a)==originalFootprintWidth(b)&&originalFootprintHeight(a)==originalFootprintHeight(b);}
inline bool relativeOriginalCandidatePath(const std::string& s){const std::filesystem::path p(s);if(s.empty()||s.size()>1024||s.find('\0')!=std::string::npos||p.is_absolute())return false;for(const auto& part:p)if(part==".."||part==".")return false;return true;}
inline bool validOriginalCandidate(const OriginalBuildingCandidate& c){if(c.pixels.valid())for(auto p:c.pixels.pixelsBGRA){const auto a=p>>24;if(((p>>16)&255)>a||((p>>8)&255)>a||(p&255)>a)return false;}return relativeOriginalCandidatePath(c.source)&&relativeOriginalCandidatePath(c.archive)&&std::filesystem::path(c.archive).extension()==".sg3"&&c.physicalId&&c.target.slot<4000&&c.target.recordOffset<8u*1024u*1024u&&c.pixels.valid()&&c.pixels.width<=2048&&c.pixels.height<=2048&&c.imageRevision==originalCandidateImageRevision(c.pixels);}
inline OriginalMapCamera originalCandidateCamera(const OriginalBuildingCandidate& c,OriginalMapCamera camera){
    if(!camera.width||!camera.height||camera.scale<1||camera.scale>4)return camera;
    camera.width=std::max(camera.width,std::min(2048u,(c.pixels.width+40)*camera.scale));
    camera.height=std::max(camera.height,std::min(2048u,(2*c.pixels.height+40)*camera.scale));return camera;
}
struct OriginalBuildingCandidateView {DecodedImage image;size_t paintedPixels=0;bool accepted=false;};
// Explicit manual convention:decoded canvas bottom-edge centre at the saved
// footprint centre. No static-origin/animation-offset ororiginal anchor claim.
inline OriginalBuildingCandidateView renderOriginalBuildingCandidate(const OriginalMapTerrain& terrain,
    const std::vector<OriginalBuildingFootprint>& footprints,const OriginalMapCamera& camera,
    const std::string& source,uint64_t revision,const OriginalBuildingCandidate& candidate,int bit=-1){
    OriginalBuildingCandidateView out;
    if(source!=candidate.source||revision!=candidate.sourceRevision||!validOriginalCandidate(candidate))return out;
    auto found=std::find_if(footprints.begin(),footprints.end(),[&](const auto& f){return sameOriginalCandidateTarget(f,candidate.target);});
    if(found==footprints.end()||!validOriginalBuildingFootprint(terrain,*found))return out;
    auto base=renderOriginalBuildingFootprints(terrain,footprints,camera,bit);if(!base.image.valid())return out;
    auto first=projectOriginalMapCell(terrain,camera,found->x,found->y);if(!first)return out;
    const double centreX=first->x+20.0*(int(originalFootprintWidth(*found))-int(originalFootprintHeight(*found)))*camera.scale;
    const double centreY=first->y+10.0*(int(originalFootprintWidth(*found))+int(originalFootprintHeight(*found))-2)*camera.scale;
    const int left=int(std::floor(centreX-candidate.pixels.width*camera.scale/2.0+.5)),top=int(std::floor(centreY-double(candidate.pixels.height)*camera.scale+.5));
    for(uint32_t y=0;y<candidate.pixels.height;++y)for(uint32_t x=0;x<candidate.pixels.width;++x){const uint32_t pixel=candidate.pixels.pixelsBGRA[size_t(y)*candidate.pixels.width+x],alpha=pixel>>24;if(!alpha)continue;
        for(uint32_t dy=0;dy<camera.scale;++dy)for(uint32_t dx=0;dx<camera.scale;++dx){const int px=left+int(x*camera.scale+dx),py=top+int(y*camera.scale+dy);if(px<0||py<0||px>=int(base.image.width)||py>=int(base.image.height))continue;
            auto& dst=base.image.pixelsBGRA[size_t(py)*base.image.width+size_t(px)];uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u})mixed|=(((pixel>>shift)&255)+(((dst>>shift)&255)*(255-alpha)+127)/255)<<shift;dst=mixed;++out.paintedPixels;
        }
    }out.image=std::move(base.image);out.accepted=true;return out;
}
struct OriginalWellBasePreview {
    std::optional<OriginalBuildingImageBinding> binding;
    OriginalBuildingCandidateView view;
};
// Exe-selected static base, using the existing explicit bottom-edge-centre
// preview placement. This closes image choice only; it does not establish the
// original anchor, shadows, occlusion, animation frame or complete SAV surface.
inline OriginalWellBasePreview renderOriginalWellBasePreview(const OriginalMapTerrain& terrain,
    const std::vector<OriginalBuildingFootprint>& footprints,const OriginalMapCamera& camera,
    const OriginalBuildingFootprint& target,const std::string& archivePath,
    const SG3Archive& archive,const std::string& companion555,int bit=-1){
    OriginalWellBasePreview out;
    const auto found=std::find_if(footprints.begin(),footprints.end(),[&](const auto& f){return sameOriginalCandidateTarget(f,target);});
    if(found==footprints.end()||!validOriginalBuildingFootprint(terrain,*found))return out;
    // Selection always uses the current profile's inputs, not a caller's stale
    // copy of the same geometry/slot.
    auto binding=bindOriginalWellBaseImage(*found,archivePath,archive);if(!binding)return out;
    auto decoded=decodeSG3Sprite(archive,binding->archiveIndex,companion555);if(decoded.status!=SG3DecodeStatus::Decoded||!decoded.image.valid())return out;
    OriginalBuildingCandidate c;c.source="snapshot";c.archive=archivePath;c.physicalId=binding->physicalId;c.target=*found;c.pixels=std::move(decoded.image);c.imageRevision=originalCandidateImageRevision(c.pixels);
    out.view=renderOriginalBuildingCandidate(terrain,footprints,camera,c.source,0,c,bit);
    if(out.view.accepted)out.binding=binding;
    return out;
}
// Candidate configuration is always local andmust stay outside both the
// supplied data root andany Git checkout,including symlink aliases/worktrees.
inline bool originalCandidateConfigPath(const std::string& path,const std::string& dataRoot){
    std::error_code ec;std::filesystem::path p(path);if(!p.is_absolute()||p.extension()!=".json")return false;p=std::filesystem::weakly_canonical(p,ec);if(ec)return false;
    const auto root=std::filesystem::weakly_canonical(dataRoot,ec);if(ec||root.empty())return false;
    auto a=root.begin(),b=p.begin();for(;a!=root.end()&&b!=p.end()&&*a==*b;++a,++b){}if(a==root.end())return false;
    for(auto parent=p.parent_path();!parent.empty();parent=parent.parent_path()){
        const bool present=std::filesystem::exists(parent,ec);if(ec)return false;
        if(present){if(std::filesystem::equivalent(parent,root,ec))return false;if(ec)return false;}
        if(std::filesystem::exists(parent/".git",ec)||ec)return false;if(parent==parent.parent_path())break;
    }return true;
}
}
