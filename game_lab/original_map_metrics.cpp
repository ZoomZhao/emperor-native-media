#include "original_map.hpp"
#include "resource_root.hpp"
#include "cg_image_conversion.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
int main(){
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
 if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
 auto root=game_lab::ResourceRoot::fromAbsolute(env);if(!root.configured())return 2;
 size_t files=0,cities=0,empirePaths=0,loaded=0,unsupported=0,failed=0,cg=0,candidates=0,complete=0;
 size_t crops=0,cropCG=0,activeCells=0,outsideCells=0,unknownCells=0,spatialMaps=0,spatialCG=0;uint32_t unknownBits=0;std::array<size_t,32> unknownCounts{};
 size_t empireRejected=0,roleMismatches=0,unclassified=0;
 std::map<uint32_t,size_t> sides;
 for(const auto& entry:std::filesystem::recursive_directory_iterator(root.configuredDirectory())){
  if(!entry.is_regular_file())continue;auto relative=entry.path().lexically_relative(root.configuredDirectory()).generic_string();
  const auto path=root.resolve(relative);if(path.empty())continue;
  auto name=entry.path().filename().string();for(char& c:name)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
  if(name=="chinamapproj.555")++candidates;
  auto ext=entry.path().extension().string();for(char& c:ext)if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(ext!=".map")continue;
  auto folded=relative;for(char& c:folded)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
  auto stem=entry.path().stem().string();for(char& c:stem)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
  const bool cityPath=folded.rfind("cities/",0)==0;
  const bool empirePath=folded.rfind("campaigns/",0)==0&&!stem.empty()&&stem.back()=='p';
  ++files;cities+=cityPath;empirePaths+=empirePath;unclassified+=!cityPath&&!empirePath;
  auto result=game_lab::loadOriginalMapTerrain(path);
  const bool completeContainer=result.decodedBytes>0;complete+=completeContainer;
  const bool cityGrid=result.status==game_lab::OriginalMapStatus::LoadedTerrain;
  const bool otherRole=result.status==game_lab::OriginalMapStatus::UnsupportedRole;
  roleMismatches+=cityPath?!cityGrid:empirePath?!(otherRole&&completeContainer):false;
  if(cityPath&&cityGrid){++loaded;++sides[result.terrain.activeSide];auto image=game_lab::renderOriginalMapFlags(result.terrain);
   auto native=game_lab::createCGImageFromPremultipliedBGRA(image.width,image.height,image.pixelsBGRA);if(native){++cg;CGImageRelease(native);}
   const auto analysis=game_lab::analyzeOriginalMap(result.terrain);crops+=analysis.cropValid();activeCells+=analysis.activeCells;outsideCells+=analysis.outsideCropCells;unknownCells+=analysis.unknownFlagCells;unknownBits|=analysis.unknownBits;
   for(unsigned bit=0;bit<32;++bit)unknownCounts[bit]+=analysis.unknownBitOccurrences[bit];
   auto cropped=game_lab::renderOriginalMapFlags(result.terrain,-1,true);auto cropImage=game_lab::createCGImageFromPremultipliedBGRA(cropped.width,cropped.height,cropped.pixelsBGRA);if(cropImage){++cropCG;CGImageRelease(cropImage);}
   game_lab::OriginalMapCamera camera;camera.centerX=camera.centerY=double(result.terrain.activeSide)/2;
   auto spatial=game_lab::renderOriginalMapSpatialFlags(result.terrain,camera,6);spatialMaps+=spatial.valid();
   auto spatialImage=game_lab::createCGImageFromPremultipliedBGRA(spatial.width,spatial.height,spatial.pixelsBGRA);if(spatialImage){++spatialCG;CGImageRelease(spatialImage);}}
  else if(otherRole)++unsupported;else if(!cityGrid)++failed;
  empireRejected+=empirePath&&otherRole&&completeContainer;
 }
 const bool accepted=files>0&&cities>0&&complete==files&&loaded==cities&&cg==loaded&&
   empireRejected==empirePaths&&!failed&&!roleMismatches&&!unclassified&&crops==loaded&&cropCG==loaded&&spatialMaps==loaded&&spatialCG==loaded;
 std::cout<<"{\"skipped\":false,\"map_files\":"<<files<<",\"city_directory_maps\":"<<cities<<",\"empire_role_paths\":"<<empirePaths
   <<",\"complete_containers\":"<<complete<<",\"terrain_grids_loaded\":"<<loaded<<",\"native_images_created\":"<<cg<<",\"unsupported_roles\":"<<unsupported<<",\"failures\":"<<failed
   <<",\"empire_roles_rejected\":"<<empireRejected<<",\"role_mismatches\":"<<roleMismatches<<",\"unclassified_map_paths\":"<<unclassified<<",\"acceptance_passed\":"<<(accepted?"true":"false")
   <<",\"storage_width\":228,\"storage_height\":228,\"active_sides\":{";
 bool first=true;for(const auto& [side,count]:sides){if(!first)std::cout<<',';first=false;std::cout<<'"'<<side<<"\":"<<count;}
 std::cout<<"},\"valid_center_crops\":"<<crops<<",\"native_crop_images\":"<<cropCG<<",\"non_offmap_cells\":"<<activeCells<<",\"non_offmap_outside_crops\":"<<outsideCells
  <<",\"unknown_flag_cells\":"<<unknownCells<<",\"unknown_bits_mask\":"<<unknownBits<<",\"unknown_bit_occurrences\":{";
 first=true;for(unsigned bit=0;bit<32;++bit)if(unknownCounts[bit]){if(!first)std::cout<<',';first=false;std::cout<<'"'<<bit<<"\":"<<unknownCounts[bit];}
 std::cout<<"},\"spatial_MAP_views\":"<<spatialMaps<<",\"spatial_native_images\":"<<spatialCG
  <<",\"spatial_scope\":\"flat numerical fields with configurable camera,notSG3artwork\",\"ChinaMapProj_companion_candidates\":"<<candidates<<",\"verified_SG3_tile_joins\":0,\"SG3_tile_mapping\":\"unverified\",\"complete_world_parse\":\"unmeasured\",\"original_palette_or_projection\":false}\n";
 return accepted?0:1;
}
