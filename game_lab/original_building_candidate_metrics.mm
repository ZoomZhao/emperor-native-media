#include "app/building_candidate_config.hpp"
#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv){@autoreleasepool{
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true}\n";return 0;}if(argc!=2||!game_lab::originalCandidateConfigPath(argv[1],env))return 2;
 NSData* bytes=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];if(!bytes||bytes.length>16384)return 3;id entries=[NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil];if(![entries isKindOfClass:NSArray.class]||![entries count]||[entries count]>6)return 4;
 auto root=game_lab::ResourceRoot::fromAbsolute(env);size_t accepted=0,native=0,pixels=0,roundtrips=0;
 for(NSDictionary* e in entries){if(![e isKindOfClass:NSDictionary.class]||e.count!=4)return 5;NSString* source=e[@"source"],*archive=e[@"archive"];NSNumber* id=e[@"physicalId"],*slot=e[@"slot"];if(![source isKindOfClass:NSString.class]||![archive isKindOfClass:NSString.class]||![id isKindOfClass:NSNumber.class]||![slot isKindOfClass:NSNumber.class])return 6;
  auto save=game_lab::loadOriginalSave(root.resolve(source.UTF8String));if(save.status!=game_lab::OriginalSaveStatus::LoadedSnapshot)return 7;auto found=std::find_if(save.buildings.footprints.begin(),save.buildings.footprints.end(),[&](const auto& f){return f.slot==slot.unsignedLongLongValue;});if(found==save.buildings.footprints.end())return 8;
  auto index=game_lab::loadSG3(root.resolve(archive.UTF8String));if(!id.unsignedLongLongValue||id.unsignedLongLongValue>index.sprites.size())return 9;auto companion=std::filesystem::path(root.resolve(archive.UTF8String));companion.replace_extension(".555");auto decoded=game_lab::decodeSG3Sprite(index,id.unsignedLongLongValue-1,companion.string());
  game_lab::OriginalBuildingCandidate c;c.source=source.UTF8String;c.archive=archive.UTF8String;c.sourceRevision=save.snapshotRevision;c.physicalId=id.unsignedIntValue;c.target=*found;c.pixels=std::move(decoded.image);c.imageRevision=game_lab::originalCandidateImageRevision(c.pixels);if(!game_lab::validOriginalCandidate(c))return 10;
  game_lab::OriginalMapCamera camera;camera.centerX=c.target.x+(game_lab::originalFootprintWidth(c.target)-1)/2.0;camera.centerY=c.target.y+(game_lab::originalFootprintHeight(c.target)-1)/2.0;camera=game_lab::originalBuildingFocusCamera(save.buildings.footprints,camera);
  camera=game_lab::originalCandidateCamera(c,camera);auto preview=game_lab::renderOriginalBuildingCandidate(save.terrain,save.buildings.footprints,camera,c.source,c.sourceRevision,c);accepted+=preview.accepted&&preview.paintedPixels>0;pixels+=preview.paintedPixels;auto cg=game_lab::createCGImageFromPremultipliedBGRA(preview.image.width,preview.image.height,preview.image.pixelsBGRA);if(cg){++native;CGImageRelease(cg);}
  auto reference=game_lab::decodeOriginalCandidate(game_lab::encodeOriginalCandidate(c));if(reference){reference->pixels=c.pixels;roundtrips+=game_lab::validOriginalCandidate(*reference)&&game_lab::sameOriginalCandidateTarget(reference->target,c.target);}
 }
 const bool ok=accepted==[entries count]&&native==accepted&&roundtrips==accepted;std::cout<<"{\"skipped\":false,\"explicit_cases\":"<<[entries count]<<",\"rendered_manual_candidates\":"<<accepted<<",\"CGImages\":"<<native<<",\"configuration_roundtrips\":"<<roundtrips<<",\"painted_pixels\":"<<pixels<<",\"confirmed_building_image_bindings\":0,\"original_anchor_verified\":false,\"acceptance_passed\":"<<(ok?"true":"false")<<"}\n";return ok?0:1;
}}
