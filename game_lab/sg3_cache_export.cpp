// SPDX-License-Identifier: MIT
// Explicit read-only native export for complete offline cache verification.
// Public-format parser only; no original code or scene-selection semantics.
#include "emperor_assets.hpp"
#include "resource_root.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
void writePixel(std::ostream& stream,uint32_t pixel) {
    const char bytes[]={char(pixel),char(pixel>>8),char(pixel>>16),char(pixel>>24)};
    stream.write(bytes,4);
}
}
int main(int argc,char** argv) {
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!env || !*env){std::cout<<"{\"skipped\":true}\n";return 0;}
    if(argc!=3)return 2;
    const auto root=game_lab::ResourceRoot::fromAbsolute(env);
    if(!root.configured())return 2;
    const std::string relative=argv[1],path=root.resolve(relative);
    if(path.empty())return 2;
    namespace fs=std::filesystem;
    const fs::path out=argv[2];std::error_code ec;
    if(!out.is_absolute() || fs::exists(fs::symlink_status(out,ec)))return 2;
    const auto parent=fs::canonical(out.parent_path(),ec);if(ec)return 2;
    const auto relation=parent.lexically_relative(root.configuredDirectory());
    if(relation.empty() || *relation.begin()!="..")return 2;
    auto archive=game_lab::loadSG3(path);
    if(archive.sprites.empty())return 2;
    auto rawArchive=archive;
    for(auto& sprite:rawArchive.sprites) {
        if(sprite.type==30 && sprite.width==78 && sprite.height>=40 && sprite.uncompressedLength==3200) {
            sprite.length=sprite.uncompressedLength;sprite.alphaLength=0;
        }
    }
    if(!fs::create_directory(out,ec) || ec)return 2;
    std::ofstream full(out/"candidates.bgra",std::ios::binary);
    std::ofstream raw(out/"raw-base.bgra",std::ios::binary),meta(out/"candidates.tsv");
    auto companion=fs::path(path);companion.replace_extension(".555");
    size_t count=0,bytes=0,rawBytes=0;
    for(size_t i=0;i<archive.sprites.size();++i) {
        if(!game_lab::sg3SingleCellGeometry(archive,i))continue;
        size_t source=i;bool mirror=false;
        while(archive.sprites[source].mirrorOffset) {
            const int64_t delta=archive.sprites[source].mirrorOffset;
            if(delta>=0 || int64_t(source)+delta<0)return 2;
            source=size_t(int64_t(source)+delta);mirror=!mirror;
        }
        const auto& s=archive.sprites[source];
        const auto decoded=game_lab::decodeSG3Sprite(archive,i,companion.string());
        const auto base=game_lab::decodeSG3Sprite(rawArchive,i,companion.string());
        if(!decoded.image.valid() || !base.image.valid())return 1;
        const auto& image=decoded.image;
        if(image.width!=78 || image.height!=uint32_t(s.height) ||
           base.image.width!=image.width || base.image.height!=image.height)return 1;
        const size_t size=image.pixelsBGRA.size()*4;
        if(size>512u*1024u*1024u-bytes || rawBytes>512u*1024u*1024u-12480)return 2;
        const auto opaque=std::count_if(image.pixelsBGRA.begin(),image.pixelsBGRA.end(),
                                       [](uint32_t p){return(p>>24)==255;});
        const auto partial=std::count_if(image.pixelsBGRA.begin(),image.pixelsBGRA.end(),
                                        [](uint32_t p){return(p>>24)>0 && (p>>24)<255;});
        meta<<relative<<'\t'<<i+1<<'\t'<<image.width<<'\t'<<image.height<<'\t'<<bytes
            <<'\t'<<opaque<<'\t'<<partial<<'\t'<<mirror<<'\t'<<source+1
            <<'\t'<<s.animationSpriteCount<<'\t'<<unsigned(s.animationSpeedId)<<'\n';
        for(const auto pixel:image.pixelsBGRA)writePixel(full,pixel);
        for(uint32_t y=image.height-40;y<image.height;++y)
            for(uint32_t x=0;x<78;++x)writePixel(raw,base.image.pixelsBGRA[size_t(y)*78+x]);
        bytes+=size;rawBytes+=12480;++count;
    }
    full.close();raw.close();meta.close();
    if(!full || !raw || !meta)return 1;
    std::cout<<"{\"decoded_candidates\":"<<count<<",\"full_canvas_bytes\":"<<bytes
             <<",\"raw_base_bytes\":"<<rawBytes<<"}\n";
    return 0;
}
