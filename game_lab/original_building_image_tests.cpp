#include "original_building_candidate.hpp"
#include "original_save.hpp"
#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool value,const char* text){++checks;if(!value)throw std::runtime_error(text);}
void word(std::vector<uint8_t>& b,size_t at,uint16_t value){b.at(at)=uint8_t(value);b.at(at+1)=uint8_t(value>>8);}
SG3Archive authoredArchive(){
    SG3Archive a;a.version=213;a.numImages=3;a.sprites.resize(3);a.imageIndex[130]=3;a.imageIndex[131]=1;
    for(auto& s:a.sprites){s.type=30;s.width=158;s.height=80;s.uncompressedLength=12800;s.tileSize=2;}return a;
}
void authored(){
    auto a=authoredArchive();OriginalBuildingFootprint f;f.type=72;f.size=2;f.wellImageInputs=OriginalWellImageInputs{};
    for(int16_t w:{int16_t(-32768),int16_t(-1),int16_t(0),int16_t(1),int16_t(32767)})for(uint8_t b:{uint8_t(0),uint8_t(1),uint8_t(128),uint8_t(255)}){
        f.wellImageInputs=OriginalWellImageInputs{w,b};const bool alternate=w>0||b;
        need(originalWellBaseImageCode(f)==(alternate?0x483:0x482),"signed selector truth table differs from exe predicate");
        auto image=bindOriginalWellBaseImage(f,"DATA/China_General.sg3",a);
        need(image&&image->tileset==2&&image->physicalId==(alternate?1u:3u)&&image->archiveIndex+1==image->physicalId,"selector must use archive header, preserving nonmonotonic physical IDs");
    }
    f.wellImageInputs=OriginalWellImageInputs{};
    need(!bindOriginalWellBaseImage(f,"DATA/China_Terrain.sg3",a),"wrong tileset archive refuses");
    for(unsigned kind=0;kind<12;++kind){auto bad=a;
        switch(kind){case 0:bad.imageIndex[130]=0;break;case 1:bad.imageIndex[130]=4;break;case 2:bad.numImages=2;break;case 3:bad.version=212;break;case 4:bad.sprites[2].type=256;break;case 5:bad.sprites[2].width=78;break;case 6:bad.sprites[2].height=79;break;case 7:bad.sprites[2].uncompressedLength=3200;break;case 8:bad.sprites[2].tileSize=3;break;case 9:bad.sprites[2].mirrorOffset=-1;break;case 10:bad.sprites.clear();break;case 11:bad.numImages=0;break;}
        need(!bindOriginalWellBaseImage(f,"DATA/China_General.sg3",bad),"invalid header or independent footprint geometry refuses");
    }
    for(unsigned kind=0;kind<5;++kind){auto bad=f;if(kind==0)bad.wellImageInputs.reset();if(kind==1)bad.type=73;if(kind==2)bad.size=1;if(kind==3)bad.width=3;if(kind==4)bad.height=1;need(!originalWellBaseImageCode(bad),"unsupported type/footprint or missing saved inputs cannot fabricate a binding");}
    OriginalMapTerrain t;t.activeSide=112;t.flags.assign(51984,8);
    // Literal independently assembled serialized offsets, with contradictory
    // adjacent bytes. A wrong v3/4-versus-v5 offset must fail this assertion.
    for(uint16_t version:{uint16_t(3),uint16_t(4),uint16_t(5)})for(unsigned variant=0;variant<3;++variant){
        const size_t common=version==3?179:version==4?181:182;std::vector<uint8_t> bytes{0x13,0};const size_t start=bytes.size();
        const std::vector<uint8_t> preamble{1,0,0xa0,0x0f,0,0,0xff,0xff,0,0,8,0,'B','u','i','l','d','i','n','g'};bytes.insert(bytes.end(),preamble.begin(),preamble.end());const size_t root=bytes.size();bytes.resize(root+common);word(bytes,root,version);
        bytes.push_back(1);bytes.push_back(0x80);const size_t body=bytes.size();bytes.resize(body+common+130);word(bytes,body,version);bytes[body+2]=1;bytes[body+5]=2;word(bytes,body+8,7);word(bytes,body+10,9);word(bytes,body+16,72);word(bytes,body+18,variant==1?1:0xffff);
        const size_t predicate=version==5?100:99;bytes[body+predicate]=variant==2?128:0;bytes[body+(version==5?99:100)]=variant==0?255:0;
        bytes.push_back(1);bytes.push_back(0x80);const size_t unknown=bytes.size();bytes.resize(unknown+common);word(bytes,unknown,version);word(bytes,unknown+16,0x1234);
        auto p=parseOriginalBuildingTable(bytes,start,bytes.size(),t);
        need(p.status==OriginalBuildingStatus::UnsupportedTail&&p.parsedRecords==2&&p.footprints.size()==1,"closed Well prefix survives unknown tail");
        need(p.footprints[0].wellImageInputs&&p.footprints[0].wellImageInputs->word16==(variant==1?1:-1)&&p.footprints[0].wellImageInputs->byte6f==(variant==2?128:0),"exe serialized fields use exact version offsets and signed word");
        need(originalWellBaseImageCode(p.footprints[0])==(variant?0x483:0x482),"parsed original-record inputs must drive code choice");
    }
    OriginalMapCamera camera;camera.width=320;camera.height=240;f.x=7;f.y=9;
    need(!renderOriginalWellBasePreview(t,{f},camera,f,"DATA/China_General.sg3",a,"").binding,"invalid payload never publishes a bound preview");
}
void original(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    auto root=ResourceRoot::fromAbsolute(env);need(root.configured(),"configured original root");
    const std::string archivePath="DATA/China_General.sg3";const auto archive=loadSG3(root.resolve(archivePath));
    // Independent on-disk contract from the exe-selected groups: both branches
    // must remain explicit. These are metadata assertions, not visual scoring.
    need(archive.imageIndex[130]==1559&&archive.imageIndex[131]==1538,"exe Well/fountain groups must resolve to original physical records1559/1538");
    size_t decodedBases=0;
    for(const auto& item:std::vector<std::pair<bool,std::pair<uint32_t,int>>>{{false,{1559,132}},{true,{1538,93}}}){
        OriginalBuildingFootprint f;f.type=72;f.size=2;f.wellImageInputs=OriginalWellImageInputs{0,uint8_t(item.first)};
        const auto binding=bindOriginalWellBaseImage(f,archivePath,archive);need(binding&&binding->physicalId==item.second.first,"both exe selector branches bind original data");
        const auto& s=archive.sprites[binding->archiveIndex];need(s.width==158&&s.height==item.second.second&&s.type==30&&s.tileSize==2&&s.uncompressedLength==12800&&s.bitmapId<archive.bitmaps.size()&&archive.bitmaps[s.bitmapId].filename=="China_Safety.bmp","record dimensions/type/base bytes/bitmap name agree with2x2 Well footprint");
        auto image=decodeSG3Sprite(archive,binding->archiveIndex,root.resolve("DATA/China_General.555"));need(image.status==SG3DecodeStatus::Decoded&&image.image.valid(),"both bound bases decode");++decodedBases;
    }
    const auto catalog=enumerateOriginalMapSources(root);need(catalog.complete,"complete catalog");size_t saves=0,wells=0,bindings=0,previews=0,native=0;
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::SaveFile)continue;++saves;auto snapshot=loadOriginalSave(root.resolve(source.relativePath));need(snapshot.status==OriginalSaveStatus::LoadedSnapshot&&snapshot.readStable,"every original save loads stably");
        for(const auto& f:snapshot.buildings.footprints){if(f.type!=72)continue;++wells;auto binding=bindOriginalWellBaseImage(f,archivePath,archive);need(binding&&binding->physicalId==1559&&f.wellImageInputs&&f.wellImageInputs->word16<=0&&!f.wellImageInputs->byte6f,"supplied saved Wells predict1559 from serialized predicate, not literal image id");++bindings;
            OriginalMapCamera camera;camera.width=400;camera.height=400;camera.centerX=f.x+.5;camera.centerY=f.y+.5;
            auto preview=renderOriginalWellBasePreview(snapshot.terrain,snapshot.buildings.footprints,camera,f,archivePath,archive,root.resolve("DATA/China_General.555"));need(preview.binding&&preview.binding->physicalId==1559&&preview.view.accepted&&preview.view.image.valid()&&preview.view.paintedPixels>0,"exe-selected base reaches existing contained scene-preview path");++previews;
            auto cg=createCGImageFromPremultipliedBGRA(preview.view.image.width,preview.view.image.height,preview.view.image.pixelsBGRA);need(cg!=nullptr,"bound preview creates native image");if(cg){++native;CGImageRelease(cg);}
        }
    }
    need(saves>0&&wells>0&&bindings==wells&&previews==wells&&native==wells,"all discovered original Well occurrences bind and preview");
    std::cout<<"{\"original_data\":true,\"saved_sources\":"<<saves<<",\"well_occurrences\":"<<wells<<",\"base_bindings\":"<<bindings<<",\"preview_images\":"<<previews<<",\"CGImages\":"<<native<<",\"selector_branches\":2,\"branches_bound_and_decoded\":"<<decodedBases<<",\"semantic_types_bound\":1,\"original_anchor_verified\":false,\"animation_frame_verified\":false}\n";
}
}
int main(){try{authored();const size_t authoredChecks=checks;original();std::cout<<"{\"passed\":true,\"authored_checks\":"<<authoredChecks<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
