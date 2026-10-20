#include "original_building_candidate.hpp"
#include "original_save.hpp"
#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <set>
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
struct BaseFact {uint16_t type,code;uint8_t side;uint32_t id;const char* bitmap;};
// Independent expected contract from cited exe selectors and original SG headers.
const std::vector<BaseFact> facts{
    {2,0x471,2,1509,"China_Housing.bmp"},{3,0x471,2,1509,"China_Housing.bmp"},
    {5,0x473,2,1513,"China_Housing.bmp"},{7,0x475,2,1517,"China_Housing.bmp"},
    {33,0x437,2,708,"China_Husbandry.bmp"},{35,0x448,2,2789,"China_Industry_2.bmp"},
    {43,0x449,2,2810,"China_Industry_2.bmp"},{53,0x423,5,647,"China_StorNDist.bmp"},
    {62,0x42a,2,629,"China_StorNDist.bmp"},{66,0x414,2,611,"China_StorNDist.bmp"},
    {67,0x416,2,617,"China_StorNDist.bmp"},{72,0x482,2,1559,"China_Safety.bmp"},
    {124,0x486,2,1618,"China_Safety.bmp"},{126,0x492,1,2046,"China_Government2.bmp"},
    {127,0x488,2,1680,"China_Safety.bmp"},{192,0x4ad,2,825,"China_Husbandry.bmp"},
    {193,0x43a,3,793,"China_Husbandry.bmp"},{194,0x431,1,2458,"China_Fields.bmp"},
    {196,0x42c,1,2410,"China_Fields.bmp"},{207,0x484,2,1580,"China_Safety.bmp"},
     {211,0x412,2,589,"China_Entertainment.bmp"},{214,0x481,2,2232,"China_Religion.bmp"},
     {231,0x4b5,1,370,"China_Aesthetic.bmp"}};
OriginalBuildingFootprint footprint(const BaseFact& fact){
    OriginalBuildingFootprint f;f.type=fact.type;f.size=fact.side;f.wellImageInputs=OriginalWellImageInputs{};
    OriginalBuildingImageInputs i;i.word20=1;i.randomByte=0;i.religiousCode=0x481;i.wallMask=0;i.savedView=0;f.imageInputs=i;
    if(fact.type==231)f.x=2;return f;
}
void generalized(){
    const uint8_t wall[16][4]={{0,2,0,2},{2,0,2,0},{0,2,0,2},{7,6,5,4},{2,0,2,0},{2,0,2,0},{4,7,6,5},{9,8,11,10},
        {0,2,0,2},{6,5,4,7},{0,2,0,2},{8,11,10,9},{5,4,7,6},{11,10,9,8},{10,9,8,11},{12,12,12,12}};
    // Literal first-draw parity vectors include LCG wrap and coordinate division.
    for(const auto& xy:std::vector<std::array<unsigned,3>>{{0,0,1},{2,0,0},{4,0,1},{227,0,1},{227,227,1}})
        for(unsigned mask=0;mask<16;++mask)for(unsigned v=0;v<4;++v){const auto b=wall[mask][v];
            need(originalResidentialWallVariant(uint8_t(mask),v*2,uint16_t(xy[0]),uint16_t(xy[1]))==b+((b==0||b==2)?xy[2]:0),"wall literal table/view and coordinate-local LCG contract");
        }
    need(!originalResidentialWallVariant(16,0,0,0)&&!originalResidentialWallVariant(0,1,0,0)&&!originalResidentialWallVariant(0,8,0,0),"wall refuses invalid mask and view");
    for(unsigned kind=0;kind<4;++kind){auto f=footprint(facts.back());
        if(kind==0)f.imageInputs->wallMask.reset();if(kind==1)f.imageInputs->savedView.reset();if(kind==2)f.imageInputs->savedView=2;if(kind==3)f.imageInputs->wallMask=16;
        need(!originalBuildingBaseSelection(f),"wall refuses absent owner context or unsupported rendered view");
    }
    // Authored storage index has deliberately non-adjacent groups. Amount
    // boundaries and the final compartment must select AFTER T(454).
    SG3Archive bays;bays.version=213;bays.numImages=160;bays.sprites.resize(160);
    bays.bitmaps={{"China_StorNDist.bmp"},{"China_Stor-bays.bmp"}};
    bays.imageIndex[0x24]=1;bays.imageIndex[0x25]=2;bays.imageIndex[0x54]=10;bays.imageIndex[0x56]=150;
    for(auto& s:bays.sprites){s.width=78;s.height=40;s.type=30;s.tileSize=1;s.uncompressedLength=3200;s.bitmapId=1;}bays.sprites[0].bitmapId=0;bays.sprites[1].bitmapId=0;
    for(uint16_t type:{uint16_t(54),uint16_t(58)}){
        OriginalBuildingFootprint f;f.type=type;f.size=type==54?3:4;f.imageInputs=OriginalBuildingImageInputs{};
        f.imageInputs->compartments=std::vector<OriginalStorageCompartment>(f.size*f.size-1,{0,0,400});
        for(const auto& entry:std::vector<std::pair<uint32_t,unsigned>>{{0,0},{199,0},{200,1},{299,1},{300,2},{399,2},{400,3}}){
            f.imageInputs->compartments->back()={18,entry.first,400};const auto c=bindOriginalBuildingComponents(f,"DATA/China_General.sg3",bays);
            need(c&&c->size()==size_t(f.size)*f.size&&c->front().binding.physicalId==(type==54?1u:2u)&&c->at(1).binding.physicalId==150&&c->back().binding.physicalId==78+entry.second&&c->back().x==f.size-1&&c->back().y==f.size-1,"storage independent amount thresholds and row-major compartment registration");
        }
        for(unsigned kind=0;kind<7;++kind){auto bad=f;auto broken=bays;
            if(kind==0)bad.imageInputs->compartments.reset();if(kind==1)bad.imageInputs->compartments->pop_back();
            if(kind==2)bad.imageInputs->compartments->back().resource=29;if(kind==3)bad.imageInputs->compartments->back().amount=401;
            if(kind==4)bad.imageInputs->compartments->back().capacity=399;if(kind==5)broken.sprites[80].width=158;
            if(kind==6)broken.bitmaps[1].filename="Unrelated.bmp";
            need(!bindOriginalBuildingComponents(bad,"DATA/China_General.sg3",broken),"storage refuses unavailable compartments, bad resource domain, or wrong SG geometry/family");
        }
    }
    for(const auto& fact:facts){
        auto f=footprint(fact);auto selected=originalBuildingBaseSelection(f);
        need(selected&&selected->code==fact.code&&selected->variant==0&&selected->side==fact.side,"per-type literal selector contract");
        SG3Archive a;a.version=214;a.numImages=16;a.sprites.resize(16);a.imageIndex[fact.code&511]=3;a.bitmaps.push_back({fact.bitmap});
        for(auto& s:a.sprites){s.type=30;s.width=80*fact.side-2;s.height=40*fact.side;s.tileSize=fact.side;s.uncompressedLength=3200*fact.side*fact.side;}
        auto image=bindOriginalBuildingBaseImage(f,"DATA/China_General.sg3",a);
        need(image&&image->physicalId==3&&image->imageCode==fact.code,"generic binder follows caller header instead of corpus IDs");
        f.width=fact.side+1;need(!originalBuildingBaseSelection(f),"every type refuses contradictory footprint");
        if(fact.type!=72){f.width=0;a.bitmaps[0].filename="Unrelated.bmp";need(!bindOriginalBuildingBaseImage(f,"DATA/China_General.sg3",a),"generic binder independently checks bitmap family");}
    }
    {
        auto f=footprint(facts[2]);f.imageInputs->randomByte=1;
        SG3Archive a;a.version=213;a.numImages=8;a.sprites.resize(8);a.imageIndex[115]=3;a.imageIndex[116]=7;a.bitmaps.push_back({"China_Housing.bmp"});
        for(auto& s:a.sprites){s.type=30;s.width=158;s.height=80;s.tileSize=2;s.uncompressedLength=12800;}
        auto bound=bindOriginalBuildingBaseImage(f,"DATA/China_General.sg3",a);
        need(bound&&bound->imageCode==0x473&&bound->variant==1&&bound->physicalId==4,"variant adds to resolved ID, never packed group");
        for(unsigned kind=0;kind<14;++kind){auto bad=a;
            switch(kind){case 0:bad.imageIndex[115]=0;break;case 1:bad.imageIndex[115]=8;break;case 2:bad.numImages=7;break;case 3:bad.version=212;break;case 4:bad.sprites[3].type=256;break;case 5:bad.sprites[3].width=78;break;case 6:bad.sprites[3].height=79;break;case 7:bad.sprites[3].uncompressedLength=3200;break;case 8:bad.sprites[3].tileSize=3;break;case 9:bad.sprites[3].mirrorOffset=-1;break;case 10:bad.sprites[3].bitmapId=1;break;case 11:bad.bitmaps.clear();break;case 12:bad.sprites[2].bitmapId=1;break;case 13:bad.sprites.clear();break;}
            need(!bindOriginalBuildingBaseImage(f,"DATA/China_General.sg3",bad),"generic variant/header/geometry/family negative control");
        }
        need(!bindOriginalBuildingBaseImage(f,"DATA/China_Terrain.sg3",a),"generic variant refuses wrong archive");
    }
    for(uint16_t type:{uint16_t(72),uint16_t(124),uint16_t(127)}){
        auto f=footprint(*std::find_if(facts.begin(),facts.end(),[&](const auto& x){return x.type==type;}));
        for(int16_t w:{int16_t(-32768),int16_t(-1),int16_t(0),int16_t(1),int16_t(32767)})for(uint8_t b:{uint8_t(0),uint8_t(1),uint8_t(128),uint8_t(255)}){
            f.wellImageInputs=OriginalWellImageInputs{w,b};auto selected=originalBuildingBaseSelection(f);
            const uint16_t expected=uint16_t((type==72?0x482:type==124?0x486:0x488)+(w>0||b!=0));
            need(selected&&selected->code==expected,"each predicate selector has an independent signed truth table");
        }
        f.wellImageInputs.reset();need(!originalBuildingBaseSelection(f),"predicate types require serialized inputs");
    }
    for(uint16_t type:{uint16_t(2),uint16_t(3),uint16_t(5),uint16_t(7)})for(int16_t people:{int16_t(-1),int16_t(0),int16_t(1)})for(uint8_t gate:{uint8_t(0),uint8_t(1),uint8_t(255)})for(uint8_t random:{uint8_t(0),uint8_t(1),uint8_t(2),uint8_t(255)}){
        OriginalBuildingFootprint f;f.type=type;f.size=2;OriginalBuildingImageInputs i;i.word20=people;i.houseByte3c=gate;i.randomByte=random;f.imageInputs=i;
        const uint16_t expected=type==2||(!people&&!gate)?0x471:type==3?0x471:type==5?0x473:0x475;
        auto s=originalBuildingBaseSelection(f);need(s&&s->code==expected&&s->variant==(random&1u),"house empty/occupied and random-bit truth table");
    }
    // Literal threshold expectations differ between solo hemp and ordinary millet.
    struct Stage {int16_t word;uint16_t solo,field;};
    const Stage stages[]={{0,0,0},{99,0,0},{100,0,1},{1999,0,1},{2000,1,2},{3999,1,2},{4000,2,3},{5999,2,3},{6000,3,4},{7999,3,4},{8000,4,5},{9999,4,5},{10000,5,5},{32767,5,5}};
    for(const auto& stage:stages)for(uint16_t type:{uint16_t(194),uint16_t(196)})for(uint8_t random:{uint8_t(0),uint8_t(1),uint8_t(2),uint8_t(255)}){
        OriginalBuildingFootprint f;f.type=type;f.size=1;OriginalBuildingImageInputs i;i.cropWord4=stage.word;i.randomByte=random;f.imageInputs=i;
        auto s=originalBuildingBaseSelection(f);need(s&&s->code==(type==194?0x431:0x42c)&&s->variant==(type==194?stage.solo:stage.field)+((random&1u)?6:0),"crop stage boundaries and random halves");
    }
    for(unsigned random=0;random<256;++random){OriginalBuildingFootprint f;f.type=62;f.size=2;OriginalBuildingImageInputs i;i.randomByte=uint8_t(random);f.imageInputs=i;auto s=originalBuildingBaseSelection(f);need(s&&s->code==0x42a&&s->variant==random%3,"empty-stall modulo truth table");}
    for(const auto& fact:facts){
        auto f=footprint(fact);
        if(fact.type==2||fact.type==3||fact.type==5||fact.type==7||fact.type==62||fact.type==194||fact.type==196){
            f.imageInputs->randomByte.reset();need(!originalBuildingBaseSelection(f),"random selectors refuse an unavailable saved byte");
            f.imageInputs.reset();need(!originalBuildingBaseSelection(f),"random selectors refuse unavailable serialized fields");
        }
        if(fact.type==194||fact.type==196){f=footprint(fact);f.imageInputs->cropWord4=-1;need(!originalBuildingBaseSelection(f),"negative crop stage is outside supported domain");}
        if(fact.type==214){f.imageInputs->religiousCode=0;need(!originalBuildingBaseSelection(f),"shrine requires its saved code rather than constructor assumption");f.imageInputs.reset();need(!originalBuildingBaseSelection(f),"shrine refuses missing serialized code");}
    }
    for(uint16_t type:{uint16_t(54),uint16_t(58),uint16_t(59),uint16_t(71),uint16_t(231),uint16_t(0x1234)}){OriginalBuildingFootprint f;f.type=type;f.size=2;need(!originalBuildingBaseSelection(f),"multipart and unknown types do not fall back to default row");}
    OriginalMapTerrain t;t.activeSide=112;t.flags.assign(51984,8);
    {
        std::vector<uint8_t> b{0x13,0,1,0,0xa0,0x0f,0,0,0xff,0xff,0,0,8,0,'B','u','i','l','d','i','n','g'};
        const size_t root=b.size();b.resize(root+182);word(b,root,5);
        for(unsigned slot=1;slot<4000;++slot){b.insert(b.end(),{1,0x80});const size_t body=b.size();b.resize(body+182+(slot<=2?132:0));word(b,body,5);
            if(slot<=2){b[body+2]=1;b[body+5]=1;word(b,body+8,uint16_t(6+slot));word(b,body+10,9);word(b,body+16,231);
                const uint32_t cell=67*228+64+slot;for(unsigned j=0;j<4;++j)b[body+12+j]=uint8_t(cell>>(8*j));}
        }
        const size_t terrainOffset=b.size()+155952;b.resize(terrainOffset+1299328);
        word(b,terrainOffset+1291324,6); // old view offset is unrelated record data
        const size_t first=67*228+65;word(b,terrainOffset-103968+2*first,1);word(b,terrainOffset-103968+2*(first+1),2);
        auto p=parseOriginalSavedBuildings(b,terrainOffset,t);
        need(p.status==OriginalBuildingStatus::Complete&&p.footprints.size()==2&&p.slotTypes[2]==231,"complete authored wall slot context");
        need(p.footprints[0].imageInputs->wallMask==2&&p.footprints[1].imageInputs->wallMask==8&&p.footprints[0].imageInputs->savedView==0,"wall reads literal SAV owner and view spans with east/west bit order");
        need(originalBuildingBaseSelection(p.footprints[0])->variant==0,"wall uses canonical coordinates for local seed");
        word(b,terrainOffset-103968+2*(first+1),4000);p=parseOriginalSavedBuildings(b,terrainOffset,t);
        need(p.footprints.size()==2&&!originalBuildingBaseSelection(p.footprints[0]),"out-of-domain adjacent owner withholds wall instead of assuming no neighbour");
        word(b,terrainOffset-103968+2*(first+1),2);word(b,terrainOffset+1299324,2);p=parseOriginalSavedBuildings(b,terrainOffset,t);
        need(p.savedView==2&&p.footprints.size()==2&&p.footprints[0].imageInputs->savedView==2&&!originalBuildingBaseSelection(p.footprints[0]),"valid saved alternate view remains explicit but view0 renderer refuses");
    }
    for(uint16_t version:{uint16_t(3),uint16_t(4),uint16_t(5)})for(uint16_t type:{uint16_t(5),uint16_t(54),uint16_t(58),uint16_t(124),uint16_t(127),uint16_t(194),uint16_t(196),uint16_t(214)}){
        const size_t common=version==3?179:version==4?181:182;
        std::vector<uint8_t> b{0x13,0,1,0,0xa0,0x0f,0,0,0xff,0xff,0,0,8,0,'B','u','i','l','d','i','n','g'};
        const size_t body0=b.size();b.resize(body0+common);word(b,body0,version);
        b.insert(b.end(),{1,0x80});const size_t body=b.size();b.resize(body+common+*originalBuildingTailBytes(type));word(b,body,version);b[body+2]=1;b[body+5]=type==54?3:type==58?4:(type==194||type==196)?1:2;word(b,body+8,7);word(b,body+10,9);word(b,body+16,type);
        const uint32_t index=uint32_t(67*228+65);for(unsigned j=0;j<4;++j)b[body+12+j]=uint8_t(index>>(8*j));
        word(b,body+18,1);word(b,body+27,3);b[body+(version==5?100:99)]=128;
        const size_t tail=body+common;word(b,tail,1);word(b,tail+2,1);word(b,tail+4,2000);
        if(type==5){b[tail+57]=0;b[tail+59]=20;}
        if(type==214){word(b,tail+130,1);word(b,tail+132,1);word(b,tail+134,1);word(b,tail+156,0x481);}
        if(type==54||type==58){word(b,tail+130,2);
            for(unsigned j=0;j<(type==54?8u:15u);++j){const size_t pos=tail+132+14*j;word(b,pos,1);word(b,pos+2,18);word(b,pos+6,300);word(b,pos+10,400);}
        }
        b.insert(b.end(),{1,0x80});const size_t unknown=b.size();b.resize(unknown+common);word(b,unknown,version);word(b,unknown+16,0x1234);
        const size_t terrainOffset=b.size();b.resize(terrainOffset+467856+51984);b[terrainOffset+415872+index]=255;b[terrainOffset+467856+index]=0;
        auto p=parseOriginalSavedBuildings(b,terrainOffset,t);need(p.footprints.size()==1&&p.liveTypeCounts[type]==1,"generic input fixtures preserve table framing and live counts");
        const auto& f=p.footprints.at(0);need(f.imageInputs&&f.imageInputs->randomByte==255&&f.imageInputs->word20==3,"correct exe random grid and common word offset retained");
        if(type==5)need(f.imageInputs->houseByte3c==20,"house serializer tail+59 after both version headers");
        if(type==194||type==196)need(f.imageInputs->cropWord4==2000,"crop serializer nested word tail+4 after both version headers");
        if(type==214)need(f.imageInputs->religiousCode==0x481,"religious nested serializer tail+156");
        if(type==54||type==58){need(f.imageInputs->compartments&&f.imageInputs->compartments->size()==(type==54?8u:15u)&&f.imageInputs->compartments->back().resource==18&&f.imageInputs->compartments->back().amount==300&&f.imageInputs->compartments->back().capacity==400,"storage serializer offsets retain final component after nested version headers");
            const auto c=bindOriginalBuildingComponents(f,"DATA/China_General.sg3",bays);need(c&&c->back().binding.physicalId==80,"parsed resource/amount closes final storage compartment");
        }else need(bool(originalBuildingBaseSelection(f)),"parsed serialized inputs close each generic selector");
        b[body+12]^=1;p=parseOriginalSavedBuildings(b,terrainOffset,t);need(p.footprints.size()==1&&p.footprints[0].imageInputs&&!p.footprints[0].imageInputs->randomByte,"mismatched serialized index never silently substitutes coordinate random byte");
        if(type==5||type==194||type==196||type==214){b[tail+(type==214?134:2)]=2;p=parseOriginalSavedBuildings(b,terrainOffset,t);need(p.footprints.size()==1&&!p.footprints[0].imageInputs,"unknown nested image layout withholds inputs without losing published footprint");}
        if(type==54||type==58){b[tail+132]=2;p=parseOriginalSavedBuildings(b,terrainOffset,t);need(p.footprints.size()==1&&!p.footprints[0].imageInputs,"unknown compartment serializer version refuses storage binding");}
    }
    // Preview reselects from the current profile, even when caller geometry matches.
    auto current=footprint(facts[4]);current.x=7;current.y=9;auto stale=current;stale.type=72;
    OriginalMapCamera camera;camera.width=320;camera.height=240;
    need(!renderOriginalBuildingBasePreview(t,{current},camera,stale,"DATA/China_General.sg3",authoredArchive(),"").binding,"generic preview refuses stale record identity");
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
    std::map<uint16_t,size_t> live,bound;std::map<uint16_t,std::map<uint32_t,size_t>> physicalCounts;std::set<uint16_t> previewed;size_t records=0,genericPreviews=0,genericNative=0;
    const std::map<uint16_t,size_t> expected{{2,10},{3,8},{5,26},{7,26},{33,12},{35,12},{43,8},{53,4},{54,6},{58,2},{59,2},{62,4},{66,2},{67,2},{71,2},{72,2},{124,6},{126,6},{127,2},{192,6},{193,8},{194,287},{196,490},{207,2},{211,2},{214,2},{231,126}};
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::SaveFile)continue;auto snapshot=loadOriginalSave(root.resolve(source.relativePath));
        need(snapshot.status==OriginalSaveStatus::LoadedSnapshot&&snapshot.readStable&&snapshot.buildings.status==OriginalBuildingStatus::Complete,"corpus count uses complete stable saved tables");
        for(const auto& entry:snapshot.buildings.liveTypeCounts){live[entry.first]+=entry.second;records+=entry.second;}
        for(const auto& f:snapshot.buildings.footprints){auto binding=bindOriginalBuildingBaseImage(f,archivePath,archive);if(!binding)continue;++bound[f.type];++physicalCounts[f.type][binding->physicalId];
            auto fact=std::find_if(facts.begin(),facts.end(),[&](const auto& x){return x.type==f.type;});need(fact!=facts.end(),"only independently traced types bind");
            const auto& s=archive.sprites[binding->archiveIndex];need(s.bitmapId<archive.bitmaps.size()&&archive.bitmaps[s.bitmapId].filename==fact->bitmap&&s.type==30&&s.tileSize==fact->side&&s.width==80*fact->side-2&&s.height>=40*fact->side&&s.uncompressedLength==3200u*fact->side*fact->side,"every live binding passes independent SG format/bitmap contract");
            if(!previewed.insert(f.type).second)continue;
            OriginalMapCamera camera;camera.width=800;camera.height=800;camera.centerX=f.x+.5;camera.centerY=f.y+.5;
            auto preview=renderOriginalBuildingBasePreview(snapshot.terrain,snapshot.buildings.footprints,camera,f,archivePath,archive,root.resolve("DATA/China_General.555"));need(preview.binding&&preview.view.accepted&&preview.view.image.valid()&&preview.view.paintedPixels>0,"each bound semantic type decodes through existing scene preview");++genericPreviews;
            auto cg=createCGImageFromPremultipliedBGRA(preview.view.image.width,preview.view.image.height,preview.view.image.pixelsBGRA);need(cg!=nullptr,"each bound type creates native preview image");if(cg){++genericNative;CGImageRelease(cg);}
        }
    }
    size_t boundRecords=0;for(const auto& entry:bound)boundRecords+=entry.second;
    need(saves==6&&live==expected&&live.size()==27&&records==1065,"six-source distinct-type and live-record denominator lock");
    need(bound.size()==23&&boundRecords==1053&&genericPreviews==23&&genericNative==23,"corpus semantic type/live base-binding count lock");
    for(const auto& fact:facts)need(bound[fact.type]==expected.at(fact.type),"every occurrence of each supported type binds");
    for(const auto& fact:facts){need(archive.imageIndex[fact.code&511]==fact.id,"independent physical header ID contract for every closed type");}
    const std::map<uint16_t,std::map<uint32_t,size_t>> expectedPhysical{
        {2,{{1509,6},{1510,4}}},{3,{{1509,4},{1510,4}}},{5,{{1513,17},{1514,9}}},{7,{{1517,17},{1518,9}}},
        {33,{{708,12}}},{35,{{2789,12}}},{43,{{2810,8}}},{53,{{647,4}}},{62,{{630,2},{631,2}}},
        {66,{{611,2}}},{67,{{617,2}}},{72,{{1559,2}}},{124,{{1618,6}}},{126,{{2046,6}}},{127,{{1680,2}}},
        {192,{{825,6}}},{193,{{793,8}}},{194,{{2458,88},{2459,5},{2460,33},{2461,9},{2464,99},{2465,5},{2466,25},{2467,23}}},
        {196,{{2410,257},{2411,9},{2416,213},{2417,11}}},{207,{{1580,2}}},{211,{{589,2}}},{214,{{2232,2}}}};
    auto withoutWall=physicalCounts;withoutWall.erase(231);
    need(withoutWall==expectedPhysical,"independent scalar audit locks per-type physical record distributions");
    need(physicalCounts[231]==std::map<uint32_t,size_t>{{370,28},{371,18},{372,38},{373,32},{375,4},{376,2},{377,4}},"independent wall physical distribution");
    size_t storageRecords=0,storageCells=0,storageDecoded=0,marketRecords=0,marketCells=0;
    std::set<uint16_t> storageTypes;std::map<uint16_t,std::map<uint32_t,size_t>> storagePhysical;
    for(const auto& source:catalog.sources)if(source.kind==OriginalMapSourceKind::SaveFile){const auto snapshot=loadOriginalSave(root.resolve(source.relativePath));
        for(const auto& f:snapshot.buildings.footprints)if(f.type==54||f.type==58){
            const auto components=bindOriginalBuildingComponents(f,archivePath,archive);
            need(components&&components->size()==size_t(f.size)*f.size,"every original storage compartment closes from saved inputs");++storageRecords;storageTypes.insert(f.type);
            for(const auto& c:*components){++storageCells;++storagePhysical[f.type][c.binding.physicalId];const auto& sprite=archive.sprites[c.binding.archiveIndex];
                need(c.side==1&&sprite.type==30&&sprite.width==78&&sprite.height>=40&&sprite.uncompressedLength==3200&&(!sprite.tileSize||sprite.tileSize==1)&&archive.bitmaps[sprite.bitmapId].filename==(c.x||c.y?"China_Stor-bays.bmp":"China_StorNDist.bmp"),"storage independent one-cell geometry and family");
                const auto decoded=decodeSG3Sprite(archive,c.binding.archiveIndex,root.resolve("DATA/China_General.555"));need(decoded.status==SG3DecodeStatus::Decoded&&decoded.image.valid(),"every original storage component decodes");++storageDecoded;
            }
        }
    }
    need(storageTypes.size()==2&&storageRecords==8&&storageCells==86&&storageDecoded==86,"storage component and occurrence denominators");
    need(storagePhysical==std::map<uint16_t,std::map<uint32_t,size_t>>{{54,{{644,6},{1170,1},{1171,1},{1172,15},{1197,1},{1198,1},{1317,29}}},{58,{{645,2},{1197,1},{1317,29}}}},"independent scalar audit locks saved storage resource/amount image distributions");
    for(const auto& source:catalog.sources)if(source.kind==OriginalMapSourceKind::SaveFile){const auto snapshot=loadOriginalSave(root.resolve(source.relativePath));
        for(const auto& f:snapshot.buildings.footprints)if(f.type==59){const auto c=bindOriginalBuildingComponents(f,archivePath,archive,&snapshot.buildings);
            need(c&&c->size()==16,"common market binds four linked stalls and twelve descriptor cells");++marketRecords;
            for(const auto& component:*c){const auto image=decodeSG3Sprite(archive,component.binding.archiveIndex,root.resolve("DATA/China_General.555"));
                need(image.status==SG3DecodeStatus::Decoded&&image.image.valid(),"every market component decodes original data");marketCells+=size_t(component.side)*component.side;}
            for(unsigned kind=0;kind<5;++kind){auto wrong=f;
                if(kind==0)wrong.imageInputs->marketStalls[0]=3999;if(kind==1)wrong.imageInputs->marketOrientation=2;
                if(kind==2)wrong.imageInputs->marketRandom.clear();if(kind==3)wrong.imageInputs->marketDraw.clear();if(kind==4)wrong.imageInputs->savedView=2;
                need(!bindOriginalBuildingComponents(wrong,archivePath,archive,&snapshot.buildings),"market refuses invalid child, layout, missing saved plane or unsupported view");
            }
        }
    }
    need(marketRecords==2&&marketCells==56,"market footprint denominator includes shared stall cells");
    size_t branchBases=0;
    for(const auto& fact:facts){
        const unsigned variants=fact.type==231?13:(fact.type==194||fact.type==196)?12:(fact.type==5||fact.type==7)?4:fact.type==62?3:(fact.type==2||fact.type==3||fact.type==72||fact.type==124||fact.type==127)?2:1;
         for(unsigned v=0;v<variants;++v){auto f=footprint(fact);uint32_t id=fact.id+v;
            if(fact.type==72||fact.type==124||fact.type==127){f.wellImageInputs->byte6f=uint8_t(v);id=v?(fact.type==72?1538:fact.type==124?1649:1704):fact.id;}
            else if(fact.type==194||fact.type==196){f.imageInputs->randomByte=v>=6?1:0;f.imageInputs->cropWord4=int16_t((v%6)*2000);}
            else if(fact.type==5||fact.type==7){f.imageInputs->randomByte=uint8_t(v&1u);if(v>=2){f.imageInputs->word20=0;id=1509+(v&1u);}}
            else if(fact.type==231){static const uint8_t masks[]={0,0,1,1,6,12,9,3,11,7,14,13,15};f.imageInputs->wallMask=masks[v];f.x=(v==1||v==3)?0:2;}
            else f.imageInputs->randomByte=uint8_t(v);
            auto binding=bindOriginalBuildingBaseImage(f,archivePath,archive);
            if(fact.type==196&&v%6>0){f.imageInputs->cropWord4=int16_t((v%6-1)*2000+100);binding=bindOriginalBuildingBaseImage(f,archivePath,archive);}
            need(binding&&binding->physicalId==id,"every supported selector output binds its independent physical variant");
            auto decoded=decodeSG3Sprite(archive,binding->archiveIndex,root.resolve("DATA/China_General.555"));need(decoded.status==SG3DecodeStatus::Decoded&&decoded.image.valid(),"every selector output decodes original payload");++branchBases;
        }
    }
     std::cout<<"{\"distinct_live_types\":"<<live.size()<<",\"live_records\":"<<records<<",\"bound_types\":"<<bound.size()+storageTypes.size()+1<<",\"bound_records\":"<<boundRecords+storageRecords+marketRecords<<",\"generic_preview_types\":"<<genericPreviews<<",\"generic_CGImages\":"<<genericNative<<",\"selector_outputs_bound_and_decoded\":"<<branchBases<<",\"storage_component_cells\":"<<storageCells<<",\"storage_components_decoded\":"<<storageDecoded<<",\"market_component_cells_decoded\":"<<marketCells<<"}\n";
    std::cout<<"{\"original_data\":true,\"saved_sources\":"<<saves<<",\"well_occurrences\":"<<wells<<",\"well_base_bindings\":"<<bindings<<",\"well_preview_images\":"<<previews<<",\"well_CGImages\":"<<native<<",\"well_branches_bound_and_decoded\":"<<decodedBases<<",\"original_anchor_verified\":false,\"animation_frame_verified\":false}\n";
}
}
int main(){try{authored();generalized();const size_t authoredChecks=checks;original();std::cout<<"{\"passed\":true,\"authored_checks\":"<<authoredChecks<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
