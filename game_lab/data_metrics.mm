#include "emperor_assets.hpp"
#include "resource_root.hpp"
#include "cg_image_conversion.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <chrono>

int main() {
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!env || !*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
    auto root=game_lab::ResourceRoot::fromAbsolute(env);
    if(!root.configured()){std::cerr<<"Invalid EMPEROR_GAMEDATA_ROOT\n";return 2;}
    const auto start=std::chrono::steady_clock::now();
    size_t archives=0,parsedArchives=0,images=0,decoded=0,presentable=0,capacity=0,
        mirrored=0,alpha=0,external=0,unsupported=0,invalidRecord=0,invalidPayload=0,
        empty=0,missing=0,maps=0,mapPreviews=0,videos=0,
        animationRecordsCompared=0,animationRecordsMatched=0,animationFieldsMatched=0;
    struct Counts {size_t decoded=0,total=0;};std::map<uint16_t,Counts> types;
    bool binki=false;
    try {
        for(const auto& entry:std::filesystem::recursive_directory_iterator(root.configuredDirectory())) {
            if(!entry.is_regular_file())continue;
            auto relative=entry.path().lexically_relative(root.configuredDirectory()).generic_string();
            auto path=root.resolve(relative);if(path.empty())continue;
            auto extension=entry.path().extension().string();
            for(char& c:extension)if(c>='A' && c<='Z')c=char(c-'A'+'a');
            if(extension==".sg3") {
                ++archives;auto archive=game_lab::loadSG3(path);
                if(archive.sprites.empty())continue;
                ++parsedArchives;capacity+=archive.capacity;
                auto companion=std::filesystem::path(path);companion.replace_extension(".555");
                // Check all five named fields against independently read SG bytes.
                // Emit aggregates only; no per-record data or playback interpretation.
                std::ifstream metadata(path,std::ios::binary);
                const size_t recordSize=archive.version==213?64:72;
                for(size_t i=0;i<archive.sprites.size();++i) {
                    const auto& s=archive.sprites[i];++images;++types[s.type].total;
                    uint8_t raw[29]{};
                    metadata.seekg(std::streamoff(40680+(i+1)*recordSize+30));
                    metadata.read(reinterpret_cast<char*>(raw),sizeof(raw));
                    if(metadata.gcount()==sizeof(raw)) {
                        ++animationRecordsCompared;
                        auto u16=[&](size_t at){return uint16_t(uint16_t(raw[at])|(uint16_t(raw[at+1])<<8));};
                        size_t matched=(s.animationSpriteCount==u16(0))+
                            (uint16_t(s.animationOffsetX)==u16(4))+(uint16_t(s.animationOffsetY)==u16(6))+
                            (s.animationReversible==raw[18])+(s.animationSpeedId==raw[28]);
                        animationFieldsMatched+=matched;animationRecordsMatched+=matched==5;
                    }
                    const auto result=game_lab::decodeSG3Sprite(archive,i,companion.string());
                    if(result.status==game_lab::SG3DecodeStatus::Decoded && result.image.valid()) {
                        ++decoded;++types[s.type].decoded;mirrored+=result.mirrored;alpha+=s.alphaLength!=0;external+=s.external!=0;
                        auto image=game_lab::createCGImageFromPremultipliedBGRA(result.image.width,result.image.height,result.image.pixelsBGRA);
                        if(image){++presentable;CGImageRelease(image);}
                    }else if(result.status==game_lab::SG3DecodeStatus::EmptyRecord)++empty;
                    else if(result.status==game_lab::SG3DecodeStatus::Unsupported)++unsupported;
                    else if(result.status==game_lab::SG3DecodeStatus::MissingDependency)++missing;
                    else if(result.status==game_lab::SG3DecodeStatus::InvalidRecord)++invalidRecord;
                    else ++invalidPayload;
                }
            }else if(extension==".map"){++maps;mapPreviews+=game_lab::loadCityMap(path).valid();}
            else if(extension==".bik") {
                ++videos;char magic[4]{};std::ifstream f(path,std::ios::binary);f.read(magic,4);
                binki|=std::string(magic,4)=="BIKi";
            }
        }
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
    std::cout<<"{\"skipped\":false,\"sg3_archives\":"<<archives<<",\"sg3_parsed_archives\":"<<parsedArchives
      <<",\"in_use_logical_images\":"<<images<<",\"capacity_slots_not_denominator\":"<<capacity
      <<",\"animation_metadata_records_compared\":"<<animationRecordsCompared
      <<",\"animation_metadata_records_matched\":"<<animationRecordsMatched
      <<",\"animation_metadata_fields_per_record\":5,\"animation_metadata_fields_matched\":"<<animationFieldsMatched
      <<",\"animation_playback_semantics\":\"unmeasured\""
      <<",\"decoded_images\":"<<decoded<<",\"cg_images_created\":"<<presentable
      <<",\"decoded_mirrors\":"<<mirrored<<",\"decoded_alpha_records\":"<<alpha<<",\"decoded_external_records\":"<<external
      <<",\"unsupported\":"<<unsupported<<",\"empty_logical_records\":"<<empty
      <<",\"nonempty_logical_records\":"<<images-empty<<",\"invalid_record\":"<<invalidRecord
      <<",\"invalid_payload\":"<<invalidPayload<<",\"missing_dependencies\":"<<missing<<",\"by_type\":{";
    bool first=true;for(const auto& [type,c]:types){if(!first)std::cout<<',';first=false;
        std::cout<<'"'<<type<<"\":{\"decoded\":"<<c.decoded<<",\"total\":"<<c.total<<'}';}
    std::cout<<"},\"map_files\":"<<maps<<",\"map_preview_payloads\":"<<mapPreviews
      <<",\"full_map_parsing\":\"unmeasured\",\"video_files\":"<<videos<<",\"BIKi_found\":"<<(binki?"true":"false")
      <<",\"video_playback\":\"unmeasured\",\"original_visual_agreement\":\"unmeasured\",\"elapsed_seconds\":"
      <<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"}\n";
    // An inventory reports partial support; it is not an all-images pass claim.
    return animationRecordsMatched==images ? 0 : 1;
}
