#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
int main(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return 0;}
    auto root=game_lab::ResourceRoot::fromAbsolute(env);if(!root.configured())return 2;
    const auto catalog=game_lab::enumerateOriginalMapSources(root);
    size_t summaryEntries=0,cityEntries=0,selected=0,layers=0,images=0,crops=0,spatial=0;std::map<uint32_t,size_t> sides;
    for(const auto& source:catalog.sources){
        if(source.kind==game_lab::OriginalMapSourceKind::PakSummary){++summaryEntries;continue;}
        if(source.kind!=game_lab::OriginalMapSourceKind::PakCity)continue;
        ++cityEntries;const auto selection=game_lab::loadOriginalMapSource(root,source);const auto& map=selection.map;
        if(map.status!=game_lab::OriginalMapStatus::LoadedTerrain||!map.terrain.valid())continue;
        ++selected;++sides[map.terrain.activeSide];layers+=map.cellLayers&&map.cellLayers->valid();
        const auto image=game_lab::renderOriginalMapFlags(map.terrain);
        auto cg=game_lab::createCGImageFromPremultipliedBGRA(image.width,image.height,image.pixelsBGRA);if(cg){++images;CGImageRelease(cg);}
        const auto analysis=game_lab::analyzeOriginalMap(map.terrain);crops+=analysis.cropValid();
        game_lab::OriginalMapCamera camera;camera.centerX=camera.centerY=double(map.terrain.activeSide)/2;camera.outlines=false;
        auto scene=game_lab::renderOriginalMapSpatialFlags(map.terrain,camera,6);spatial+=scene.valid();
    }
    const bool accepted=catalog.complete&&catalog.pakFiles>0&&catalog.completePaks==catalog.pakFiles&&summaryEntries==catalog.pakFiles&&
        cityEntries>0&&cityEntries==catalog.cityCandidates&&selected==cityEntries&&layers==selected&&images==selected;
    std::cout<<"{\"skipped\":false,\"native_PAK_files\":"<<catalog.pakFiles<<",\"complete_PAKs\":"<<catalog.completePaks
        <<",\"complete_parts\":"<<catalog.pakParts<<",\"decoded_bytes\":"<<catalog.pakDecodedBytes<<",\"package_summary_entries\":"<<summaryEntries
        <<",\"package_city_entries\":"<<cityEntries<<",\"city_grids_loaded\":"<<selected<<",\"city_raw_layer_sets\":"<<layers
        <<",\"native_images_created\":"<<images<<",\"valid_crops\":"<<crops<<",\"spatial_diagnostics\":"<<spatial
        <<",\"non_city_parts_uninterpreted\":"<<catalog.pakParts-catalog.cityCandidates<<",\"loose_MAP_entries\":"<<catalog.looseMaps
        <<",\"catalog_complete\":"<<(catalog.complete?"true":"false")<<",\"acceptance_passed\":"<<(accepted?"true":"false")<<",\"active_sides\":{";
    bool first=true;for(const auto& item:sides){if(!first)std::cout<<',';first=false;std::cout<<'"'<<item.first<<"\":"<<item.second;}
    std::cout<<"},\"verified_SG3_tile_joins\":0,\"scope\":\"native package selection and numerical terrain only; campaign semantics and original artwork selection unproved\"}\n";
    return accepted?0:1;
}
