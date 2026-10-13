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
 std::map<uint32_t,size_t> sides;
 for(const auto& entry:std::filesystem::recursive_directory_iterator(root.configuredDirectory())){
  if(!entry.is_regular_file())continue;auto relative=entry.path().lexically_relative(root.configuredDirectory()).generic_string();
  const auto path=root.resolve(relative);if(path.empty())continue;
  auto name=entry.path().filename().string();for(char& c:name)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
  if(name=="chinamapproj.555")++candidates;
  auto ext=entry.path().extension().string();for(char& c:ext)if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(ext!=".map")continue;
  auto folded=relative;for(char& c:folded)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
  auto stem=entry.path().stem().string();for(char& c:stem)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
  ++files;cities+=folded.rfind("cities/",0)==0;empirePaths+=folded.rfind("campaigns/",0)==0&&!stem.empty()&&stem.back()=='p';
  auto result=game_lab::loadOriginalMapTerrain(path);
  complete+=result.decodedBytes>0;
  if(result.status==game_lab::OriginalMapStatus::LoadedTerrain){++loaded;++sides[result.terrain.activeSide];auto image=game_lab::renderOriginalMapFlags(result.terrain);
   auto native=game_lab::createCGImageFromPremultipliedBGRA(image.width,image.height,image.pixelsBGRA);if(native){++cg;CGImageRelease(native);}}
  else if(result.status==game_lab::OriginalMapStatus::UnsupportedRole)++unsupported;else ++failed;
 }
 std::cout<<"{\"skipped\":false,\"map_files\":"<<files<<",\"city_directory_maps\":"<<cities<<",\"empire_role_paths\":"<<empirePaths
   <<",\"complete_containers\":"<<complete<<",\"terrain_grids_loaded\":"<<loaded<<",\"native_images_created\":"<<cg<<",\"unsupported_roles\":"<<unsupported<<",\"failures\":"<<failed
   <<",\"storage_width\":228,\"storage_height\":228,\"active_sides\":{";
 bool first=true;for(const auto& [side,count]:sides){if(!first)std::cout<<',';first=false;std::cout<<'"'<<side<<"\":"<<count;}
 std::cout<<"},\"ChinaMapProj_companion_candidates\":"<<candidates<<",\"complete_world_parse\":\"unmeasured\",\"original_palette_or_projection\":false}\n";
 return failed||loaded!=cities||cg!=loaded?1:0;
}
