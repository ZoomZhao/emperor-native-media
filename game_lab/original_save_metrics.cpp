#include "original_save.hpp"
#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
int main(){
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return 0;}
 auto root=game_lab::ResourceRoot::fromAbsolute(env);if(!root.configured())return 2;const auto catalog=game_lab::enumerateOriginalMapSources(root);
 size_t inspected=0,cellQueries=0,cellValues=0;
 size_t entries=0,loaded=0,stable=0,sets=0,selected=0,views=0,native=0,hidden=0;std::map<uint32_t,size_t> sides;
 for(const auto& source:catalog.sources){if(source.kind!=game_lab::OriginalMapSourceKind::SaveFile)continue;++entries;hidden+=std::filesystem::path(source.relativePath).filename()==".sav";
  const auto snapshot=game_lab::loadOriginalSave(root.resolve(source.relativePath));if(snapshot.status!=game_lab::OriginalSaveStatus::LoadedSnapshot)continue;++loaded;stable+=snapshot.readStable;sets+=snapshot.layers.valid();++sides[snapshot.terrain.activeSide];
  const auto selection=game_lab::loadOriginalMapSource(root,source);if(!selection.saveLayers)return 1;selected+=selection.map.terrain.flags==snapshot.terrain.flags&&selection.saveLayers&&selection.saveLayers->mapperObjects==snapshot.layers.mapperObjects&&selection.saveLayers->followingU32Unknown==snapshot.layers.followingU32Unknown;
  bool cellsOK=true;const int64_t side=snapshot.terrain.activeSide;const size_t origin=(228-side)/2;
  for(auto xy:{std::pair<int64_t,int64_t>{0,0},{side-1,0},{0,side-1},{side-1,side-1},{side/2,side/2}}){
   const auto cell=game_lab::originalSaveCell(selection.map.terrain,*selection.saveLayers,xy.first,xy.second);
   const size_t index=(origin+size_t(xy.second))*228+origin+size_t(xy.first);
   const std::array<uint32_t,6> expected{{snapshot.terrain.flags[index],snapshot.layers.followingU32Unknown[index],snapshot.layers.mapperObjects[index],snapshot.layers.mapperRandom[index],snapshot.layers.mapperFertile[index],snapshot.layers.mapperStone[index]}};
   const bool exact=cell&&cell->values==expected&&cell->storageX==origin+xy.first&&cell->storageY==origin+xy.second&&cell->onMap==((expected[0]&0x80000u)==0);
   cellsOK&=exact;cellQueries+=exact;cellValues+=exact?6:0;
  }
  inspected+=cellsOK;
  for(unsigned field=0;field<6;++field){auto values=field==0?snapshot.terrain.flags:game_lab::originalSaveFieldValues(snapshot.layers,field-1);const auto image=game_lab::renderOriginalMapValues(snapshot.terrain,values,-1,true);views+=image.valid();auto cg=game_lab::createCGImageFromPremultipliedBGRA(image.width,image.height,image.pixelsBGRA);if(cg){++native;CGImageRelease(cg);}}
 }
 const bool accepted=catalog.complete&&entries>0&&entries==catalog.saveFiles&&loaded==entries&&stable==entries&&sets==entries&&selected==entries&&inspected==entries&&cellQueries==5*entries&&views==entries*6&&native==views;
 std::cout<<"{\"skipped\":false,\"native_SAV_entries\":"<<entries<<",\"snapshots_loaded\":"<<loaded<<",\"stable_two_reads\":"<<stable<<",\"complete_saved_layer_sets\":"<<sets<<",\"source_selections_verified\":"<<selected<<",\"inspected_sources\":"<<inspected<<",\"cell_queries_verified\":"<<cellQueries<<",\"cell_values_verified\":"<<cellValues<<",\"hidden_sav_names\":"<<hidden<<",\"numerical_field_grids\":"<<views<<",\"native_images_created\":"<<native<<",\"values_per_grid\":51984,\"verified_SG3_tile_joins\":0,\"active_sides\":{";
 bool first=true;for(const auto& item:sides){if(!first)std::cout<<',';first=false;std::cout<<'"'<<item.first<<"\":"<<item.second;}
 std::cout<<"},\"acceptance_passed\":"<<(accepted?"true":"false")<<",\"scope\":\"saved raw snapshot only, not current memory, buildings, walkers or game rules\"}\n";return accepted?0:1;
}
