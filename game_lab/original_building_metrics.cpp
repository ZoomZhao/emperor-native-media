#include "original_pak.hpp"
#include "asset_bytes.hpp"
#include <cstdlib>
#include <iostream>
int main(){
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return 0;}
 const auto root=game_lab::ResourceRoot::fromAbsolute(env);const auto catalog=game_lab::enumerateOriginalMapSources(root);if(!catalog.complete)return 2;
 size_t maps=0,embedded=0,complete=0,slots=0,cleared=0,refs=0,failed=0;
 const auto add=[&](const game_lab::OriginalBuildingProfile& p){complete+=p.status==game_lab::OriginalBuildingStatus::Complete;slots+=p.parsedRecords;cleared+=p.clearedClassRecords;refs+=p.classReferencesVerified;failed+=p.status!=game_lab::OriginalBuildingStatus::Complete;};
 for(const auto& source:catalog.sources){
  if(source.kind==game_lab::OriginalMapSourceKind::MapFile){std::vector<uint8_t> raw;if(!game_lab::detail::readFile(root.resolve(source.relativePath),4u*1024u*1024u,raw))return 3;auto container=game_lab::decodeOriginalMapContainer(raw);if(!container.complete)return 4;auto map=game_lab::parseOriginalMapPayload(container.payload);if(map.status==game_lab::OriginalMapStatus::UnsupportedRole)continue;if(map.status!=game_lab::OriginalMapStatus::LoadedTerrain)return 5;++maps;add(game_lab::parseOriginalMapBuildings(container.payload,map.terrain));}
  else if(source.kind==game_lab::OriginalMapSourceKind::PakSummary){auto pak=game_lab::loadOriginalPak(root.resolve(source.relativePath));if(pak.status!=game_lab::OriginalPakStatus::Loaded)return 6;for(const auto& part:pak.parts)if(part.cityCandidate){++embedded;if(!part.buildingTable)return 7;add(*part.buildingTable);}}
 }
 const size_t entries=maps+embedded;const bool pass=entries&&complete==entries&&slots==entries*4000&&failed==0;
 std::cout<<"{\"skipped\":false,\"loose_city_tables\":"<<maps<<",\"embedded_city_tables\":"<<embedded<<",\"complete_tables\":"<<complete<<",\"source_entries\":"<<entries<<",\"closed_slots\":"<<slots<<",\"declared_slots\":"<<entries*4000<<",\"class_references_verified\":"<<refs<<",\"cleared_class_records\":"<<cleared<<",\"failures\":"<<failed<<",\"acceptance_passed\":"<<(pass?"true":"false")<<",\"scope\":\"table spans only;cleared records are not live buildings or image bindings\"}\n";return pass?0:1;
}
