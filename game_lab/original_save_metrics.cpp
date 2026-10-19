#include "original_save.hpp"
#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include "original_building_view.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
int main(){
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return 0;}
 auto root=game_lab::ResourceRoot::fromAbsolute(env);if(!root.configured())return 2;const auto catalog=game_lab::enumerateOriginalMapSources(root);
 size_t inspected=0,cellQueries=0,cellValues=0;
 size_t buildingBound=0,buildingSources=0,footprints=0,footprintCells=0,footprintViews=0,footprintNative=0,outlinePixels=0;std::map<uint16_t,size_t> footprintTypes;size_t prefixRecords=0,completeBuildingTables=0,liveSized=0,shapeUnsupported=0,rejectedFootprints=0,cropFields=0;
 size_t entries=0,loaded=0,stable=0,sets=0,selected=0,views=0,native=0,hidden=0;std::map<uint32_t,size_t> sides;
 for(const auto& source:catalog.sources){if(source.kind!=game_lab::OriginalMapSourceKind::SaveFile)continue;++entries;hidden+=std::filesystem::path(source.relativePath).filename()==".sav";
  const auto snapshot=game_lab::loadOriginalSave(root.resolve(source.relativePath));if(snapshot.status!=game_lab::OriginalSaveStatus::LoadedSnapshot)continue;++loaded;stable+=snapshot.readStable;sets+=snapshot.layers.valid();++sides[snapshot.terrain.activeSide];
  const auto selection=game_lab::loadOriginalMapSource(root,source);if(!selection.saveLayers)return 1;selected+=selection.map.terrain.flags==snapshot.terrain.flags&&selection.saveLayers&&selection.saveLayers->mapperObjects==snapshot.layers.mapperObjects&&selection.saveLayers->followingU32Unknown==snapshot.layers.followingU32Unknown;
  if(!selection.saveBuildings)return 1;prefixRecords+=snapshot.buildings.parsedRecords;liveSized+=snapshot.buildings.liveSizedRecords;shapeUnsupported+=snapshot.buildings.unsupportedFootprintRecords;rejectedFootprints+=snapshot.buildings.rejectedTerrainFootprints;completeBuildingTables+=snapshot.buildings.status==game_lab::OriginalBuildingStatus::Complete;
  buildingBound+=selection.saveBuildings->status==snapshot.buildings.status&&selection.saveBuildings->parsedRecords==snapshot.buildings.parsedRecords&&selection.saveBuildings->footprints.size()==snapshot.buildings.footprints.size();
  if(!snapshot.buildings.footprints.empty()){
   ++buildingSources;footprints+=snapshot.buildings.footprints.size();
   for(const auto& f:snapshot.buildings.footprints)footprintCells+=size_t(game_lab::originalFootprintWidth(f))*game_lab::originalFootprintHeight(f);
   for(const auto& f:selection.saveBuildings->footprints){
    ++footprintTypes[f.type];cropFields+=f.type>=0xc2&&f.type<=0xc7;game_lab::OriginalMapCamera camera;camera.width=320;camera.height=240;camera.centerX=f.x+(game_lab::originalFootprintWidth(f)-1)/2.0;camera.centerY=f.y+(game_lab::originalFootprintHeight(f)-1)/2.0;
    camera=game_lab::originalBuildingFocusCamera({f},camera);auto view=game_lab::renderOriginalBuildingFootprints(selection.map.terrain,{f},camera);footprintViews+=view.image.valid()&&view.visible==1&&view.valid==1&&view.outlinePixels==80u*(game_lab::originalFootprintWidth(f)+game_lab::originalFootprintHeight(f));outlinePixels+=view.outlinePixels;
    auto cg=game_lab::createCGImageFromPremultipliedBGRA(view.image.width,view.image.height,view.image.pixelsBGRA);if(cg){++footprintNative;CGImageRelease(cg);}
   }
  }
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
 const bool accepted=catalog.complete&&entries>0&&entries==catalog.saveFiles&&loaded==entries&&stable==entries&&sets==entries&&selected==entries&&inspected==entries&&cellQueries==5*entries&&views==entries*6&&native==views&&buildingBound==entries&&footprintViews==footprints&&footprintNative==footprints;
 std::cout<<"{\"skipped\":false,\"native_SAV_entries\":"<<entries<<",\"snapshots_loaded\":"<<loaded<<",\"stable_two_reads\":"<<stable<<",\"complete_saved_layer_sets\":"<<sets<<",\"source_selections_verified\":"<<selected<<",\"inspected_sources\":"<<inspected<<",\"cell_queries_verified\":"<<cellQueries<<",\"cell_values_verified\":"<<cellValues<<",\"hidden_sav_names\":"<<hidden<<",\"numerical_field_grids\":"<<views<<",\"native_images_created\":"<<native<<",\"validated_building_prefix_records\":"<<prefixRecords<<",\"complete_building_tables\":"<<completeBuildingTables<<",\"live_sized_records\":"<<liveSized<<",\"unsupported_footprint_shape_records\":"<<shapeUnsupported<<",\"rejected_terrain_footprints\":"<<rejectedFootprints<<",\"building_source_bindings\":"<<buildingBound<<",\"building_snapshot_sources\":"<<buildingSources<<",\"validated_building_footprints\":"<<footprints<<",\"crop_field_record_occurrences\":"<<cropFields<<",\"other_structure_record_occurrences\":"<<footprints-cropFields<<",\"footprint_cell_checks\":"<<footprintCells<<",\"footprint_views\":"<<footprintViews<<",\"footprint_CGImages\":"<<footprintNative<<",\"unique_outline_pixels_across_views\":"<<outlinePixels<<",\"values_per_grid\":51984,\"verified_SG3_tile_joins\":0,\"active_sides\":{";
 bool first=true;for(const auto& item:sides){if(!first)std::cout<<',';first=false;std::cout<<'"'<<item.first<<"\":"<<item.second;}
 std::cout<<"},\"footprint_types\":{";first=true;for(const auto& item:footprintTypes){if(!first)std::cout<<',';first=false;std::cout<<'"'<<item.first<<"\":"<<item.second;}
 std::cout<<"},\"acceptance_passed\":"<<(accepted?"true":"false")<<",\"scope\":\"saved raw snapshot only, bounded published-type square/market-rectangle footprint annotation only; not building artwork, current memory, walkers or game rules\"}\n";return accepted?0:1;
}
