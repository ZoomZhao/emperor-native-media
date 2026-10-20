#include "original_building_scene.hpp"
#include "original_figure_update.hpp"
#include "original_pak.hpp"
#include "app/map_surface.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <unistd.h>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
void authored(){
    // Explicit independent frame-major truth tables: 0x5CFDF0 uses metadata
    // count at+20 as direction stride, not frames-per-direction.
    const uint32_t truth[3][8]={{201,202,203,204,205,206,207,208},{209,210,211,212,213,214,215,216},{217,218,219,220,221,222,223,224}};
    for(unsigned f=0;f<3;++f)for(unsigned d=0;d<8;++d)
        need(originalFigureFrameIndex(201,3,8,f,d)==truth[f][d],"frame-major direction-interleaved index truth table");
    need(!originalFigureFrameIndex(201,3,8,3,0)&&!originalFigureFrameIndex(201,3,8,0,8)&&
        !originalFigureFrameIndex(UINT32_MAX,3,8,0,0),"index domain/overflow refusals");
    OriginalFigure f;f.type=11;f.progress=0;
    const int expected[8][2]={{2,-1},{4,0},{2,1},{0,2},{-2,1},{-4,0},{-2,-1},{0,-2}};
    for(unsigned d=0;d<8;++d){f.direction=d;
        need(originalFigureSubcell(f)==std::make_pair(expected[d][0],expected[d][1]),"eight direction progress0 position truth table");
        f.progress=9;need(originalFigureSubcell(f)==std::make_pair(10*expected[d][0],10*expected[d][1]),"last positive step");
        f.progress=10;need(originalFigureSubcell(f)==std::make_pair(-9*expected[d][0],-9*expected[d][1]),"cell transition wraps signed subcell");
        f.progress=20;need(originalFigureSubcell(f)==std::make_pair(0,0),"stationary progress20");f.progress=0;
    }
    f.direction=8;f.previousDirection=6;need(originalFigureSubcell(f)==std::make_pair(-2,-1),"stationary direction uses previous direction");
    f.progress=21;need(!originalFigureSubcell(f),"table bounds refusal");
    f.freeMovement=1;f.subX=47;f.subY=62;f.raise=3;
    need(originalFigureSubcell(f)==std::make_pair(10,6),"free movement modulo20 projected from independent (7,2) example");
    f.subX=-1;need(!originalFigureSubcell(f),"negative modulo table address refused");
    // Literal source wire offsets, decoy values in adjacent enum bytes.
    std::vector<uint8_t> bytes{1,0,1,0,0,0,255,255,0,0,6,0,'F','i','g','u','r','e',7,0};
    std::array<uint8_t,408> raw{};raw[18]=11;raw[22]=1;raw[5]=7;raw[25]=3;raw[26]=6;raw[64]=17;raw[65]=9;
    raw[8]=0x34;raw[9]=0x12;raw[28]=5;raw[30]=6;raw[114]=9;raw[144]=12;raw[122]=3;
    raw[380]=13;raw[381]=2;raw[382]=36;raw[316]=9;
    raw[383]=0xf9;raw[384]=19;raw[386]=0xf8;raw[387]=0xff;raw[388]=3;raw[389]=0xfe;raw[390]=9;raw[132]=6;
    raw[172]=11;raw[176]=12;raw[188]=13;raw[192]=14;raw[204]=199;
    raw[208]=2;raw[212]=1;raw[216]=0xff;raw[217]=raw[218]=raw[219]=0xff;raw[328]=17;raw[128]=5;raw[153]=1;
    for(const auto& field:emperor::file_io::research::figure7_fields)
        for(size_t k=0;k<field.bytes;++k)bytes.push_back(std::string(field.helper)=="0x503db0"&&k==1?0xee:raw[field.receiver_offset+k]);
    bytes.push_back(0);
    auto parsed=parseOriginalSavedFigures(bytes,0,bytes.size(),17,0);
    need(parsed.status==OriginalFigureStatus::Complete&&parsed.parsed==1&&parsed.end==bytes.size()&&parsed.live.size()==1,"complete bounded Figure7 wire reuse");
    const auto& p=parsed.live[0];need(p.type==11&&p.frame==7&&p.action==17&&p.progress==9&&p.direction==3&&p.previousDirection==6&&p.image==0x1234&&p.x==5&&p.y==6,"field identities and enum storage width");
    need(p.leaderLink==9&&p.target==12&&p.faction==3,"Figure7 leader, attack target and faction retain distinct cited requests");
    need(p.effectKind==13&&p.effectVariant==2&&p.effectFlags==36&&p.updateOwner==9,"inline effect state and updater gate are retained separately from absent FSA extension");
    need(p.effectX==-7&&p.effectY==19&&p.effectHeight==-8&&p.effectVelocityX==3&&p.effectVelocityY==-2&&
        p.effectVelocityZ==9&&p.effectDraw==6,"inline effect signed fields retain exact Figure7 twenty-byte request offsets");
    need(p.threat==11&&p.threatDistance==12&&p.herdTarget==13&&p.herdDistance==14&&p.idleCounter==199&&
        p.cooldowns==std::array<int32_t,3>{2,1,-1}&&p.linkedBuilding==17&&p.movementMode==5&&p.attackerCount==1,
        "Pheasant dword links, signed cooldowns and idle counter retain separate Figure7 requests");
    auto bad=bytes;bad.back()=1;need(parseOriginalSavedFigures(bad,0,bad.size(),17,0).status==OriginalFigureStatus::Unsupported,"FSA does not guess a variable extension length");
    bad=bytes;bad.pop_back();need(parseOriginalSavedFigures(bad,0,bad.size(),17,0).status==OriginalFigureStatus::Malformed,"last byte truncation publishes no figures");
    bad=bytes;bad[18]=6;need(parseOriginalSavedFigures(bad,0,bad.size(),17,0).status==OriginalFigureStatus::Unsupported,"unknown Figure version fails closed");
    {
        auto multiple=bytes;multiple[2]=3;multiple.insert(multiple.end(),{17,128});multiple.insert(multiple.end(),bytes.begin()+18,bytes.end());multiple.insert(multiple.end(),{18,0});
        const auto read=parseOriginalSavedFigures(multiple,0,multiple.size(),17,0);
        need(read.status==OriginalFigureStatus::Complete&&read.live.size()==3&&read.live[2].slot==2&&read.end==multiple.size(),"carried archive class17, object18 and tag-only reference preserve list slots");
        auto extended=multiple;extended.erase(extended.begin()+bytes.size(),extended.begin()+bytes.size()+2);
        extended.insert(extended.begin()+bytes.size(),{255,127,17,0,0,128});
        need(parseOriginalSavedFigures(extended,0,extended.size(),17,0).status==OriginalFigureStatus::Complete,"extended class-reference tag preserves archive identity");
        auto invalidExtended=bytes;invalidExtended[2]=2;invalidExtended.insert(invalidExtended.end(),{255,127,255,255,0,0});
        need(parseOriginalSavedFigures(invalidExtended,0,invalidExtended.size(),17,0).status==OriginalFigureStatus::Malformed,"extended object65535 cannot become a class declaration");
        multiple[bytes.size()]=16;
        need(parseOriginalSavedFigures(multiple,0,multiple.size(),17,0).status==OriginalFigureStatus::Malformed,"class references cannot resolve to opaque preceding archive entries");
        auto pheasant=bytes;pheasant[10]=12;pheasant.erase(pheasant.begin()+12,pheasant.begin()+18);
        const std::string name="cPheasantFig";pheasant[10]=name.size();pheasant.insert(pheasant.begin()+12,name.begin(),name.end());
        pheasant.insert(pheasant.begin()+12+name.size(),{2,0});
        need(parseOriginalSavedFigures(pheasant,0,pheasant.size(),17,0).status==OriginalFigureStatus::Complete,"Pheasant own2 precedes Figure7");
        auto emissary=pheasant;const std::string other="cEmissaryDude";
        emissary.erase(emissary.begin()+12,emissary.begin()+12+name.size());emissary.insert(emissary.begin()+12,other.begin(),other.end());emissary[10]=other.size();
        emissary.insert(emissary.end(),{3,0,0,0,'a','b',0});
        const auto readEmissary=parseOriginalSavedFigures(emissary,0,emissary.size(),17,0);
        need(readEmissary.status==OriginalFigureStatus::Complete&&readEmissary.end==emissary.size(),"Emissary own2 consumes bounded trailing string bytes");
        emissary.pop_back();need(parseOriginalSavedFigures(emissary,0,emissary.size(),17,0).status==OriginalFigureStatus::Malformed,"Emissary variable tail truncation publishes no prefix");
    }
    OriginalSceneDepth base{5,2,0,0},object1{5,2,1,0},animation{5,2,2,0},figure{5,2,3,0},object2{5,2,4,0},next{5,3,0,0};
    need(base<object1&&object1<animation&&animation<figure&&figure<object2&&object2<next,"city cell phase order: surface, mode1, animation, figure, mode2, next cell");
    // Opaque authored rectangles make selected record, signed offsets, scale and
    // occlusion observable as pixels rather than implementation-mirroring keys.
    const auto work=std::filesystem::temp_directory_path()/("emperor-figure-pixels-"+std::to_string(getpid()));
    std::filesystem::create_directories(work);const auto file=work/"authored.555";
    std::vector<uint8_t> payload;SG3Archive archive;archive.version=214;archive.numImages=2;archive.sprites.resize(2);
    archive.imageIndex[0]=1;archive.bitmaps.push_back({"AuthoredFigures"});
    for(unsigned id=0;id<2;++id){auto& s=archive.sprites[id];s.width=30;s.height=40;s.type=256;s.offset=payload.size();s.animationOffsetX=10;s.animationOffsetY=45;
        for(unsigned y=0;y<40;++y){payload.push_back(30);for(unsigned x=0;x<30;++x){payload.push_back(id?0x1f:0);payload.push_back(id?0:0x7c);}}
        s.length=payload.size()-s.offset;
    }
    {std::ofstream output(file,std::ios::binary);output.write(reinterpret_cast<const char*>(payload.data()),payload.size());need(bool(output),"authored pixels written in temp only");}
    OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);
    for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[y*228+x]=0;
    OriginalBuildingProfile buildings;buildings.status=OriginalBuildingStatus::Complete;
    auto scene=composeOriginalBuildingScene(terrain,buildings,archive);
    OriginalFigure one;one.type=11;one.alive=1;one.image=38u<<14;one.x=0;one.y=0;one.cell=72*229;one.progress=20;one.slot=4;
    OriginalFigureProfile figures;figures.status=OriginalFigureStatus::Complete;figures.live={one};
    figures.cellHeads.assign(51984,0);figures.cellHeads[one.cell]=one.slot;
    const std::map<uint32_t,const SG3Archive*> refs{{38,&archive}};
    {
        // Independent authored group: T(4C15)=physical1-1, 12x8. The
        // selected physical72 is frame8,direction7; saved image is a decoy.
        auto group=archive;group.sprites.resize(100);group.numImages=100;
        for(unsigned i=0;i<21;++i)group.imageIndex[i]=1;
        group.sprites[0].animationSpriteCount=12;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> supplied{{38,&group}};
        auto input=one;input.frame=8;input.direction=7;input.image=123;
        const auto chosen=figureImage(originalFigureImageInputs(input),supplied);
        need(chosen.status==OriginalFigureImageStatus::Selected&&chosen.code==0x4c15&&chosen.image==((38u<<14)+71),"forward type11 subtype0 selects code4C15 from non-image phase");
        input.image=0;input.secondaryImage=UINT32_MAX;
        need(figureImage(originalFigureImageInputs(input),supplied).image==chosen.image,"forward selector cannot consume either saved image word");
        auto forwardFigures=figures;forwardFigures.live={input};forwardFigures.live[0].secondaryImage=0;
        forwardFigures.cellHeads.assign(51984,0);forwardFigures.cellHeads[input.cell]=input.slot;
        input.image=123;forwardFigures.live[0].image=123;
        auto composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,forwardFigures,supplied,0,OriginalFigureRenderSource::Forward);
        // The authored target record needs valid geometry to exercise binding.
        group.sprites[71]=archive.sprites[0];
        installOriginalFigureOverlays(composed,terrain,forwardFigures,supplied,0,OriginalFigureRenderSource::Forward);
        need(composed.figurePlaced==1&&composed.overlays[0].physicalId==72,"explicit forward scene binds reconstructed primary despite saved-image decoy");
        input.type=76;input.runtimeClass=OriginalFigureClass::Pheasant;input.action=31;input.frame=255;
        for(unsigned i=0;i<45;++i)group.imageIndex[i]=1;
        need(figureImage(originalFigureImageInputs(input),supplied).image==((38u<<14)+95),"5CFDF0 clamps high frame after action31 selects Pheasant entry3");
        input.action=33;
        need(figureImage(originalFigureImageInputs(input),supplied,0x100).code==0x4c2c,
            "Pheasant entry0 actions33/34 bypass terrain substitution just like action5");
        input.type=45;input.action=9;input.frame=8;input.runtimeClass=OriginalFigureClass::Emissary;
        const auto emissaryImage=figureImage(originalFigureImageInputs(input),supplied);
        need(emissaryImage.status==OriginalFigureImageStatus::Selected&&emissaryImage.code==0x4c17&&emissaryImage.image==((38u<<14)+71),
            "serialized Emissary virtual updater overrides semantic type table");
        input.type=30;input.runtimeClass=OriginalFigureClass::Figure;
        need(figureImage(originalFigureImageInputs(input),supplied).status==OriginalFigureImageStatus::OwnerState,"building virtual selector is refused without owner trace");
        input.type=11;group.imageIndex[3]=0;
        need(figureImage(originalFigureImageInputs(input),supplied).status==OriginalFigureImageStatus::MissingGroup,"sparse lookup compaction cannot silently choose another group");
    }
    {
        auto group=archive;group.numImages=100;group.sprites.resize(100);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=24;group.sprites[0].animationDirections=1;
        const std::map<uint32_t,const SG3Archive*> supplied{{14,&group},{38,&group}};
        OriginalFigure effect=one;effect.type=1;effect.image=0;effect.effectKind=13;effect.effectFlags=36;effect.frame=14;
        const auto first=originalFigureFirstTickImage(originalFigureImageInputs(effect),supplied);
        need(first.status==OriginalFigureImageStatus::Selected&&first.code==0x1c0f&&first.image==((14u<<14)+5),
            "first-tick kind13 increments saved14 to15 then divides by3, independently of saved image");
        effect.x=2;effect.y=3;effect.subX=999;effect.subY=-999;effect.effectX=19;effect.effectY=22;
        effect.effectVelocityX=-3;effect.effectVelocityY=2;effect.effectHeight=16;effect.effectVelocityZ=-1;effect.hidden=1;
        const auto updated=originalFigureFirstUpdate(effect,supplied);
        need(updated.state&&updated.state->subX==32&&updated.state->subY==48&&updated.state->raise==31&&
            updated.state->effectX==16&&updated.state->effectY==24&&updated.state->effectHeight==15&&
            updated.state->frame==15&&updated.state->image==first.image&&updated.state->secondaryImage==0&&
            updated.state->hidden==0&&updated.state->effectDraw==1&&updated.state->action==effect.action&&
            updated.state->direction==effect.direction&&updated.state->counter==effect.counter&&updated.state->cell==effect.cell,
            "kind13 full bounded update moves inline coordinates before height, frame and image writes");
        effect.image=1234;effect.secondaryImage=5678;
        need(originalFigureFirstUpdate(effect,supplied).state->image==updated.state->image&&effect.subX==999&&effect.hidden==1,
            "explicit first update ignores saved image words and preserves the saved snapshot");
        effect.effectHeight=32767;effect.effectVelocityZ=1;
        need(originalFigureFirstUpdate(effect,supplied).state->effectHeight==0,"effect height wraps as i16 BEFORE negative ground clamp");
        effect.effectHeight=0;effect.effectVelocityZ=0;effect.effectX=-7;effect.effectVelocityX=0;
        need(originalFigureFirstUpdate(effect,supplied).state->subX==30,"signed effect division truncates toward zero");
        effect.effectX=119;effect.effectVelocityX=1;
        need(!originalFigureFirstUpdate(effect,supplied).state&&
            originalFigureFirstUpdate(effect,supplied).status==OriginalFigureUpdateStatus::CellListMutation,
            "crossing a cell refuses unimplemented unlink/relink effects");
        effect.frame=71;
        need(originalFigureFirstTickImage(originalFigureImageInputs(effect),supplied).image==(14u<<14),"kind13 first tick wraps at three times SG frame count");
        effect.frame=255;
        need(originalFigureFirstTickImage(originalFigureImageInputs(effect),supplied).image==(14u<<14),"effect counter first wraps as an unsigned byte");
        effect.effectKind=12;
        need(originalFigureFirstTickImage(originalFigureImageInputs(effect),supplied).status==OriginalFigureImageStatus::ExternalState,"linked owner effects do not inherit kind13 first-tick prediction");
        effect.effectKind=13;effect.frame=14;
        effect.updateOwner=1;
        need(originalFigureFirstTickImage(originalFigureImageInputs(effect),supplied).status==OriginalFigureImageStatus::ExternalState,"a nonzero saved update owner skips virtual update, not a guessed first-tick frame");
        effect.updateOwner=0;
        effect.image=0;effect.secondaryImage=0;effect.hidden=0;
        auto zeroFigures=figures;zeroFigures.live={effect};
        auto zeroScene=scene;installOriginalFigureOverlays(zeroScene,terrain,zeroFigures,supplied,0,OriginalFigureRenderSource::Forward);
        need(zeroScene.figurePlaced==0&&zeroScene.figureZeroResolved==1&&zeroScene.figureFirstTickImages.at(effect.slot).image==first.image,
            "a selected first-tick effect remains absent in forward tick-0 overlays");
        effect.type=76;effect.runtimeClass=OriginalFigureClass::Pheasant;effect.action=19;
        need(originalFigureImageClearedAtLoad(originalFigureImageInputs(effect))&&
            originalFigureFirstTickImage(originalFigureImageInputs(effect),supplied).status==OriginalFigureImageStatus::ExternalState,
            "Pheasant action19 load image is cleared but first tick requires actual state transitions");
    }
    {
        OriginalFigureProfile saved;saved.status=OriginalFigureStatus::Complete;saved.declared=saved.parsed=2000;
        auto nonzero=one;nonzero.type=19;nonzero.secondaryImage=(38u<<14)|1;nonzero.frame=17;nonzero.hidden=1;
        auto zero=one;zero.type=76;zero.runtimeClass=OriginalFigureClass::Pheasant;zero.slot=7;zero.image=0;
        auto unknown=one;unknown.type=11;unknown.slot=8;
        saved.live={unknown,zero,nonzero};const auto step=originalFigurePostLoadStep(saved);
        need(step.accepted&&step.computed==2&&step.blocked==1&&step.figures.begin()->first==4,
            "explicit post-load image step accounts for every live slot in ascending pool order");
        need(step.figures.at(nonzero.slot).images==std::make_pair(0u,0u)&&step.figures.at(nonzero.slot).callbackVA==0x4d6cf0,
            "post-load callback clears BOTH nonzero image words even for hidden figures");
        need(step.figures.at(zero.slot).images==std::make_pair(0u,0u),"post-load zero stays zero before any first update");
        need(!step.figures.at(unknown.slot).images&&step.figures.at(unknown.slot).status==OriginalFigurePostLoadStatus::MissingGroup&&
            step.figures.at(unknown.slot).callbackVA==0x4d6d30,"type11 callback requires supplied SG group rather than retaining a snapshot");
        auto group=archive;group.numImages=200;group.sprites.resize(200);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=12;group.imageIndex[20]=101;
        const std::map<uint32_t,const SG3Archive*> supplied{{38,&group}};
        auto input11=originalFigureImageInputs(unknown);input11.frame=2;input11.direction=7;input11.subtype=0;
        need(originalFigurePostLoadImages(8,input11,supplied).images==std::make_pair((38u<<14)+23,0u),
            "type11 callback uses fixed4C13 even with subtype0, frame times8 plus raw direction");
        input11.direction=8;input11.previousDirection=1;
        need(originalFigurePostLoadImages(8,input11,supplied).images->first==(38u<<14)+24,
            "callback raw direction8 does not use fallback or SG direction clamp");
        input11.direction=0;input11.action=4;input11.counter=100;
        need(originalFigurePostLoadImages(8,input11,supplied).images->first==(38u<<14)+11,
            "callback death counter clamps without increment");
        input11.counter=2;
        need(originalFigurePostLoadImages(8,input11,supplied,2).images->first==(38u<<14)+1,
            "callback death direction adds stride1 once when view subtraction is negative");
        need(saved.live[2].image==one.image&&saved.live[2].secondaryImage==((38u<<14)|1)&&saved.live[2].frame==17&&saved.live[2].hidden==1,
            "post-load image projection preserves source image words and non-image saved state");
        auto input=originalFigureImageInputs(one);input.type=19;
        need(!originalFigurePostLoadImages(0,input).images&&!originalFigurePostLoadImages(2000,input).images,
            "post-load callback pool excludes slot0 and slots beyond1999");
        input.alive=0;need(!originalFigurePostLoadImages(4,input).images,"dead slots do not reach live callback pass");
        input.alive=2;need(originalFigurePostLoadImages(4,input).images.has_value(),"post-load live gate is nonzero, not state1 only");
        input.type=12;need(!originalFigurePostLoadImages(4,input).images&&
            originalFigurePostLoadImages(4,input).status==OriginalFigurePostLoadStatus::TypeUnspecified,
            "uncited type does not inherit clearing from neighbouring callback entries");
        saved.live.push_back(nonzero);const auto duplicate=originalFigurePostLoadStep(saved);
        need(!duplicate.accepted&&duplicate.figures.empty(),"duplicate pool identities publish no partial post-load step");
        saved.live.pop_back();saved.parsed=1999;
        need(!originalFigurePostLoadStep(saved).accepted,"incomplete parsed pool cannot run a post-load image step");
    }
    {
        OriginalFigureAdmissionInputs timing;timing.now=501;timing.speed=10;
        need(!originalFigureAdmission(timing).admitted,"scheduler speed10 interval500 needs extra2ms");
        timing.now=502;const auto due=originalFigureAdmission(timing);
        need(due.admitted&&due.last==502&&due.limit==10000,"scheduler admits at exact wall-clock threshold");
        need(originalFigureUpdatesBeforeDraw(due,{false})==1&&originalFigureUpdatesBeforeDraw(due,{true,true,false})==3,
            "draw follows one ordinary pass or explicit fast-forward observations");
        need(!originalFigureUpdatesBeforeDraw(due,{true})&&originalFigureUpdatesBeforeDraw(due,std::vector<bool>(10000,true))==10000,
            "missing live fast-forward observation stays unknown; maximum loop is10000");
        timing.paused=true;need(originalFigureUpdatesBeforeDraw(originalFigureAdmission(timing),{})==0,"pause admits zero passes before draw");
        timing.paused=false;timing.mode=7;need(!originalFigureAdmission(timing).admitted,"non-city mode gates admission");
        timing.mode=0x12;need(!originalFigureAdmission(timing).admitted,"special mode requires live ready gate");
        timing.specialModeReady=true;need(originalFigureAdmission(timing).admitted,"special mode ready allows clock admission");
        timing.mode=1;timing.speed=100;timing.now=1;timing.last=0;
        need(!originalFigureAdmission(timing).admitted,"fastest speed still needs2ms");
        timing.now=2;need(originalFigureAdmission(timing).admitted,"fastest threshold2ms includes equality");
        timing.now=0;timing.last=1;need(originalFigureAdmission(timing).admitted,"negative signed clock delta substitutes10000ms");
        timing.now=1;timing.last=UINT32_MAX;need(originalFigureAdmission(timing).admitted,"32-bit clock wrap subtracts modulo before signed test");
        std::array<bool,51> flags{};
        need(originalFigureAdvanceFrame(13,12,4,0,flags)->counter==1,"out-of-range counter resets BEFORE unconditional increment");
        need(originalFigureAdvanceFrame(13,12,4,100,flags)->counter==0,"ineligible frame still resets an out-of-range byte");
        need(originalFigureAdvanceFrame(11,12,4,0,flags)->counter==0,"increment reaching group length resets to zero");
        need(originalFigureAdvanceFrame(255,300,4,0,flags)->counter==0,"frame increment stores one byte even for a longer SG group");
        flags[8]=true;const auto advanced=originalFigureAdvanceFrame(3,12,4,50,flags);
        need(advanced&&advanced->counter==4&&advanced->eligibilityIndex==8,"explicit flag index is100 times SG speed divided by argument");
        need(!originalFigureAdvanceFrame(3,12,255,1,flags)&&!originalFigureAdvanceFrame(3,0,4,0,flags),
            "unsafe speed index or empty SG group refuses frame advance");
        auto refused=one;refused.runtimeClass=OriginalFigureClass::Pheasant;refused.type=76;refused.action=31;
        need(originalFigureFirstUpdate(refused,{}).status==OriginalFigureUpdateStatus::PheasantContext&&
            !originalFigureFirstUpdate(refused,{}).state,"saved Pheasant walk frame does not bypass earlier target predicates");
        refused.runtimeClass=OriginalFigureClass::Figure;refused.type=20;
        need(originalFigureFirstUpdate(refused,{}).status==OriginalFigureUpdateStatus::CartLeaderMovement,
            "cart refuses first update until leader order and movement are computed");
        refused.type=11;need(originalFigureFirstUpdate(refused,{}).status==OriginalFigureUpdateStatus::WorkerOwnerMovement,
            "known type11 load image does not imply a complete owner-dependent first update");
    }
    {
        auto group=archive;group.numImages=256;group.sprites.resize(256);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=20;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> supplied{{38,&group}};
        auto bird=one;bird.type=76;bird.runtimeClass=OriginalFigureClass::Pheasant;bird.action=31;
        bird.frame=19;bird.direction=6;bird.idleCounter=198;bird.hidden=1;bird.freeMovement=1;
        bird.herdTarget=4;bird.herdDistance=99;bird.cooldowns={2,1,-1};bird.threat=80;bird.threatDistance=2;
        auto profile=figures;profile.declared=profile.parsed=2000;profile.live={bird};
        profile.slots.resize(2000);for(size_t n=0;n<2000;++n)profile.slots[n].slot=n;profile.slots[bird.slot]=bird;
        auto context=originalFigureUpdateContext(profile);need(bool(context),"complete saved pool supplies explicit Pheasant context");
        const auto result=originalFigureFirstUpdate(bird,supplied,&*context);
        need(result.state&&result.state->action==31&&result.state->frame==0&&result.state->idleCounter==199&&
            result.state->cooldowns==std::array<int32_t,3>{1,0,-1}&&result.state->hidden==0&&result.state->freeMovement==0&&
            result.state->movementMode==5&&result.state->herdTarget==4&&result.state->herdDistance==99&&
            result.state->threat==0&&result.state->threatDistance==100&&result.state->image==((38u<<14)+6),
            "Pheasant action31 preserves valid self target, decrements positive cooldowns and advances after idle increment");
        bird.cooldowns={0,INT32_MIN,INT32_MAX};
        need(originalFigureFirstUpdate(bird,supplied,&*context).state->cooldowns==std::array<int32_t,3>{0,INT32_MIN,INT32_MAX-1},
            "Pheasant cooldown zero and negative values are retained while positive signed maximum decrements");
        need(originalFigureStateAfterUpdates(bird,supplied,0,&*context).state->image==0&&
            originalFigureStateAfterUpdates(bird,supplied,1,&*context).state->idleCounter==199&&
            originalFigureStateAfterUpdates(bird,supplied,2,&*context).status==OriginalFigureUpdateStatus::OrderedPoolPass,
            "explicit k0 applies clearing callback; k1 advances state; later Pheasant pool passes refuse");
        group.imageIndex[42]=2;group.sprites[1].animationSpriteCount=12;group.sprites[1].animationDirections=8;
        const auto terrain=originalFigureFirstUpdate(bird,supplied,&*context,0x100);
        need(terrain.state&&terrain.state->frame==1&&terrain.state->image==((38u<<14)+15),
            "Pheasant terrain replacement group is resolved before out-of-range frame reset then increment");
        bird.idleCounter=199;const auto transitioned=originalFigureFirstUpdate(bird,supplied,&*context);
        need(transitioned.state&&transitioned.state->action==30&&transitioned.state->idleCounter==200&&transitioned.state->frame==1&&
            transitioned.state->image==((38u<<14)+15),
            "Pheasant signed idle threshold200 changes action before selecting entry9");
        bird.idleCounter=INT32_MAX;need(originalFigureFirstUpdate(bird,supplied,&*context).state->action==31,
            "Pheasant idle counter wraps as32 bits before signed comparison");
        bird.idleCounter=0;bird.herdTarget=0;auto other=bird;other.slot=2;other.x=12;other.y=0;other.action=32;
        auto later=other;later.slot=6;later.x=1;auto inactive=OriginalFigure{};inactive.x=25;
        context->figures={{0,inactive},{2,other},{4,bird},{6,later}};
        const auto search=originalFigureFirstUpdate(bird,supplied,&*context);
        need(search.state&&search.state->herdTarget==0&&search.state->herdDistance==25,
            "Pheasant herd search returns vector ordinal0 then rereads inactive pool0; radius12 admits first candidate");
        context->figures.at(2).x=13;
        need(originalFigureFirstUpdate(bird,supplied,&*context).state->herdTarget==2&&
            originalFigureFirstUpdate(bird,supplied,&*context).state->herdDistance==13,
            "Pheasant herd range13 fails and next vector ordinal2 rereads pool2 rather than candidate slot6");
        context->figures.at(6).action=4;
        need(originalFigureFirstUpdate(bird,supplied,&*context).state->herdTarget==0&&
            originalFigureFirstUpdate(bird,supplied,&*context).state->herdDistance==100,"dead-action herd member and self cannot satisfy new-target search");
        auto threat=other;threat.type=11;threat.x=3;context->figures[2]=threat;
        need(originalFigureFirstUpdate(bird,supplied,&*context).status==OriginalFigureUpdateStatus::PheasantThreat&&
            !originalFigureFirstUpdate(bird,supplied,&*context).state,"nearby non-animal refuses unresolved combat qualification");
        context->figures.at(2).x=4;
        need(originalFigureFirstUpdate(bird,supplied,&*context).state.has_value(),"outside conservative threat radius does not require combat predicate");
        bird.linkedBuilding=1;need(originalFigureFirstUpdate(bird,supplied,&*context).status==OriginalFigureUpdateStatus::PheasantBuilding,
            "linked building validity is refused before publishing a Pheasant state");
        bird.linkedBuilding=0;bird.action=32;
        need(originalFigureFirstUpdate(bird,supplied,&*context).status==OriginalFigureUpdateStatus::PheasantRoute,"action32 refuses route and arrival mutation");
        bird.action=19;need(originalFigureFirstUpdate(bird,supplied,&*context).status==OriginalFigureUpdateStatus::PheasantDestination,
            "action19 refuses destination RNG and map selection after its known action30 assignment");
        profile.slots[1]=profile.slots[0];need(!originalFigureUpdateContext(profile),"duplicate saved slots cannot supply complete updater context");
    }
    {
        OriginalFigureSchedulerGlobals live;live.previousClock=1234;live.speed=70;live.fastForward=true;live.paused=true;
        const auto single=originalFigureSchedulerAfterLoad(live,80,false),network=originalFigureSchedulerAfterLoad(live,80,true);
        need(single.previousClock==1234&&single.speed==80&&single.fastForward&&!single.paused,
            "SAV load keeps clock and fast-forward, restores personal speed and clears pause");
        need(network.speed==70&&network.previousClock==1234&&network.fastForward&&!network.paused,
            "network-active load preserves effective speed rather than applying personal preference");
    }
    {
        // Exercise missing dependencies and amount thresholds without an oracle.
        auto group=archive;group.sprites.resize(200);group.numImages=200;
        group.imageIndex.fill(1);group.sprites[0].animationSpriteCount=12;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> supplied{{14,&group},{38,&group},{39,&group}};
        OriginalFigure leader;leader.type=19;leader.alive=1;leader.slot=10;leader.subtype=10;leader.amount=800;
        OriginalFigure cart=one;cart.type=20;cart.action=6;cart.leaderLink=10;cart.frame=3;cart.direction=2;
        OriginalFigureProfile profile;profile.live={leader};auto state=originalFigureImageState(profile);
        need(figureImage(originalFigureImageInputs(cart),supplied,0,0,{},&state).code==0x4e90,
            "bulk resource and amount over400 choose large direct-cart family");
        state.figures.at(10).amount=400;
        need(figureImage(originalFigureImageInputs(cart),supplied,0,0,{},&state).code==0x4e38,
            "amount400 belongs to small direct-cart family");
        state.figures.at(10).type=20;state.figures.at(10).leaderLink=10;
        need(figureImage(originalFigureImageInputs(cart),supplied,0,0,{},&state).status==OriginalFigureImageStatus::OwnerState,
            "cyclic cart leader chain refuses after original eight-link bound");
        cart.type=40;cart.action=7;cart.target=10;cart.x=5;cart.y=8;cart.counter=1;
        state.figures.at(10).type=76;state.figures.at(10).x=7;state.figures.at(10).y=8;state.figures.at(10).action=31;
        auto near=figureImage(originalFigureImageInputs(cart),supplied,0,0,{},&state);
        need(near.code==0x4ec0&&near.frame==0,"attack target within two cells selects static attack pose");
        state.figures.at(10).x=8;
        need(figureImage(originalFigureImageInputs(cart),supplied,0,0,{},&state).code==0x4ebf,
            "attack target three cells away selects moving attack family");
        cart.type=45;state.factionModels=std::array<int16_t,22>{};state.factionModels->at(0)=3;
        need(figureImage(originalFigureImageInputs(cart),supplied,0,0,{},&state).status==OriginalFigureImageStatus::ExternalState,
            "lazy faction model3 remains unresolved instead of choosing a default");
        cart.type=19;cart.action=7;cart.subtype=1;cart.amount=200;cart.direction=2;
        need(figureSecondaryImage(originalFigureImageInputs(cart),supplied).image==((38u<<14)+2),"cargo200 uses first directional band");
        cart.amount=201;
        need(figureSecondaryImage(originalFigureImageInputs(cart),supplied).image==((38u<<14)+10),"cargo201 enters second directional band");
        cart.amount=401;
        need(figureSecondaryImage(originalFigureImageInputs(cart),supplied).image==((38u<<14)+18),"cargo401 enters third directional band");
        cart.subtype=10;
        need(figureSecondaryImage(originalFigureImageInputs(cart),supplied).image==0,"special-resource leader itself does not carry a cargo component");
    }
    installOriginalFigureOverlays(scene,terrain,figures,refs);
    need(scene.figurePlaced==1&&scene.overlays.size()==1&&scene.overlays[0].physicalId==1&&scene.overlays[0].offsetX==31&&scene.overlays[0].offsetY==-15,"v214 no-system-prefix physical1 and selected sprite anchor");
    installOriginalFigureOverlays(scene,terrain,figures,refs);
    need(scene.figurePlaced==1&&scene.overlays.size()==1,"reinstallation replaces figures without duplicate commands or counts");
    OriginalMapCamera camera;camera.width=200;camera.height=200;camera.centerX=0;camera.centerY=0;camera.outlines=false;
    const std::vector<MapImageGridTileset> tilesets{{38,&archive,file.string(),1}};
    for(unsigned scale:{1u,2u}){camera.scale=scale;const auto view=renderOriginalBuildingScene(terrain,scene,archive,file.string(),camera,-1,tilesets);
        const unsigned left=100-8*scale,top=100-35*scale;
        need(view.image.valid()&&view.figureDecoded==1&&view.overlayDrawn==1&&view.overlayInvalid==0&&view.image.pixelsBGRA[top*200+left]==0xffff0000u,"figure blits selected saved image at exact signed scaled position");
        need(view.image.pixelsBGRA[top*200+left-1]!=0xffff0000u,"sprite left edge is exact");
    }
    camera.scale=1;auto ordered=scene;
    ordered.overlays.push_back({"DATA/China_General.sg3",2,2,0,0,31,-15,{0,0,2,0},0,3,1});
    auto rendered=renderOriginalBuildingScene(terrain,ordered,archive,file.string(),camera,-1,tilesets);
    need(rendered.image.pixelsBGRA[65*200+92]==0xffff0000u,"figure pixel follows same-cell building animation");
    ordered.overlays.back().mode=2;ordered.overlays.back().depth.phase=4;
    need(renderOriginalBuildingScene(terrain,ordered,archive,file.string(),camera,-1,tilesets).image.pixelsBGRA[65*200+92]==0xff0000ffu,"same-cell mode2 pixel follows figure");
    {
        auto sorted=figures;auto front=one;front.slot=3;front.image=(38u<<14)|1;front.progress=0;front.direction=3;front.height=2;front.next=one.slot;
        sorted.cellHeads[one.cell]=3;sorted.live.push_back(front);
        auto composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,sorted,refs);
        const auto view=renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,tilesets);
        need(view.figureDecoded==2&&view.image.pixelsBGRA[65*200+92]==0xff0000ffu,"larger subcell score draws last despite smaller slot and height cancelling pixel displacement");
        sorted.live.back().progress=20;sorted.live.back().height=0;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,sorted,refs);
        need(renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,tilesets).image.pixelsBGRA[65*200+92]==0xffff0000u,"equal subcell score breaks ties by source slot");
    }
    {
        auto pair=figures;pair.live[0].type=20;pair.live[0].secondaryImage=(38u<<14)|1;
        auto composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,pair,refs);
        need(composed.figurePlaced==1&&composed.overlays.size()==2&&composed.overlays[0].physicalId==1&&composed.overlays[1].physicalId==2,
            "two-image action keeps primary then secondary");
        const auto view=renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,tilesets);
        need(view.figureDecoded==2&&view.image.pixelsBGRA[65*200+92]==0xff0000ffu,"type20 secondary overlaps primary at independently equal anchor");
        pair.live[0].extraX=7;pair.live[0].extraY=-4;composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,pair,refs);
        need(composed.overlays[1].offsetX==38&&composed.overlays[1].offsetY==-19,"saved secondary extra offset participates");
    }
    {
        auto missing=figures;missing.cellHeads[one.cell]=0;auto composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,missing,refs);need(composed.figurePlaced==0&&composed.figureUnsupported==1,"unlinked live records cannot bypass cell scan");
        auto cycle=figures;cycle.live[0].next=one.slot;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,cycle,refs);need(composed.figurePlaced==0&&composed.figureUnsupported==1,"cycle after requested slot cannot masquerade as a valid cell list");
        auto suppressed=figures;suppressed.live[0].hidden=1;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,suppressed,refs);need(composed.figurePlaced==0&&composed.figureHidden==1,"saved hidden gate suppresses image");
        auto unset=figures;unset.live[0].image=0;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,unset,refs);need(composed.figurePlaced==0&&composed.figureZeroImage==1,"image zero is not an invented initialization frame");
        auto duplicate=figures;duplicate.live.push_back(one);composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,duplicate,refs);need(composed.figurePlaced==0&&composed.figureUnsupported==2,"duplicate source slots fail before placement");
    }
    auto elapsed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(elapsed,terrain,figures,refs,1);
    need(elapsed.figurePlaced==0&&elapsed.figureUnsupported==1,"nonzero caller tick refuses unproved simulation phase");
    for(unsigned type:{11u,13u,19u,20u,21u,22u,23u,26u,28u,29u,30u,35u,36u,39u,40u,45u,47u,76u}){
        for(unsigned d=0;d<8;++d){auto p=figures;p.live[0].type=type;p.live[0].direction=d;p.live[0].progress=0;
            auto composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,p,refs);
            need(composed.figurePlaced==1&&composed.overlays[0].physicalId==1,"all bound types/directions retain saved image at tick0");
        }
    }
    std::filesystem::remove(file);std::filesystem::remove(work);
}
void corpus(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    const auto root=ResourceRoot::fromAbsolute(env);const auto catalog=enumerateOriginalMapSources(root);need(catalog.complete,"complete source catalog");
    AppMapSurface app(root);size_t sources=0,slots=0,live=0,placed=0,hidden=0,zero=0,unsupported=0,decoded=0,failures=0;
    std::set<unsigned> types,boundTypes;std::map<unsigned,size_t> blocked;size_t focused=0,placedDecoded=0;
    struct Score {size_t matched=0,total=0,selected=0;std::map<std::string,size_t> mismatch;};
    Score score,hiddenScore,cargoScore;size_t cargoNonzero=0,cargoNonzeroMatched=0;
    size_t zeroResolved=0,zeroBlocked=0,firstTickSelected=0,firstTickBlocked=0;std::map<unsigned,size_t> zeroTypes;
    size_t postLoadComputed=0,postLoadBlocked=0,postLoadChanged=0,postLoadOracleMatched=0;
    size_t postLoadPrimaryChanged=0,postLoadSecondaryChanged=0;std::map<unsigned,size_t> postLoadBlockedTypes;
    size_t postLoadSelected=0,postLoadDecoded=0,updateComputed=0,updateDecoded=0,updateConsistent=0;
    std::map<unsigned,size_t> updateBlockedTypes,updateComputedTypes;std::map<std::string,size_t> updateBlockers;
    std::map<std::pair<unsigned,unsigned>,Score> byAction,hiddenAction;std::set<unsigned> selectors;
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::SaveFile)continue;
        const auto save=loadOriginalSave(root.resolve(source.relativePath));need(save.readStable&&save.layers.figures.has_value(),"same-source stable figure profile");
        const auto& figures=*save.layers.figures;
        const auto imageState=originalFigureImageState(figures);
        auto decoys=figures;for(auto& f:decoys.live){f.image=UINT32_MAX;f.secondaryImage=UINT32_MAX;}
        for(auto& f:decoys.slots){f.image=UINT32_MAX;f.secondaryImage=UINT32_MAX;}
        const auto decoyState=originalFigureImageState(decoys);
        const auto updateContext=originalFigureUpdateContext(figures),decoyUpdateContext=originalFigureUpdateContext(decoys);
        need(updateContext&&decoyUpdateContext,"complete same-source saved context available for per-type virtual updates");
        need(figures.factionModels.has_value(),"complete following Goods1/Empire3/Grid15/child5 publishes faction models");
        std::vector<uint8_t> savedBytes;need(detail::readFile(root.resolve(source.relativePath),4u*1024u*1024u,savedBytes),"read original container for following-layout refusal checks");
        const auto payload=decodeOriginalMapContainer(savedBytes).payload;
        need(!originalFigureFactionModels(payload,figures.end,figures.end)&&
            !originalFigureFactionModels(payload,figures.end,figures.end+6),"truncated following Goods/Empire cannot publish default faction models");
        need(figures.status==OriginalFigureStatus::Complete&&figures.parsed==figures.declared,"complete serialized figure list");
        ++sources;slots+=figures.declared;live+=figures.live.size();for(const auto& t:figures.liveTypes)types.insert(t.first);
        need(app.selectSavedBuildings(save.terrain,save.buildings,save.layers.ground?&*save.layers.ground:nullptr,0,&figures),"App installs saved figures");
        OriginalMapCamera camera;camera.width=800;camera.height=800;camera.centerX=save.terrain.activeSide/2;camera.centerY=camera.centerX;camera.outlines=false;
        const auto view=app.renderSavedBuildings(camera);need(view.image.valid()&&view.overlayInvalid==0&&view.overlayDecodeFailures==0,"original figure images decode with generic overlay layer");
        placed+=view.figurePlaced;hidden+=view.figureHidden;zero+=view.figureZeroImage;unsupported+=view.figureUnsupported;decoded+=view.figureDecoded;failures+=view.overlayDecodeFailures;
        need(view.figureLive==figures.live.size()&&view.figureLive==view.figurePlaced+view.figureHidden+view.figureZeroImage+view.figureUnsupported,"all live figure outcomes accounted");
        // Recompose explicitly for type and blockage denominators, independent of App cache.
        auto scene=composeOriginalBuildingScene(save.terrain,save.buildings,loadSG3(root.resolve("DATA/China_General.sg3")));
        std::map<uint32_t,SG3Archive> archives;std::map<uint32_t,const SG3Archive*> refs;
        for(unsigned slot:{14u,38u,39u,40u}){archives.emplace(slot,loadSG3(root.resolve(std::string("DATA/")+originalFigureArchiveName(slot))));refs[slot]=&archives.at(slot);}
        const auto postLoad=originalFigurePostLoadStep(figures,refs),postLoadDecoys=originalFigurePostLoadStep(decoys,refs);
        need(postLoad.accepted&&postLoad.figures.size()==figures.live.size()&&
            postLoad.computed+postLoad.blocked==figures.live.size(),"post-load step partitions every original live occurrence");
        postLoadComputed+=postLoad.computed;postLoadBlocked+=postLoad.blocked;
        const auto decodeWord=[&](uint32_t word){auto name=std::filesystem::path(originalFigureArchiveName(word>>14));name.replace_extension(".555");
            return decodeSG3Sprite(archives.at(word>>14),word&0x3fff,root.resolve("DATA/"+name.string())).image.valid();};
        for(const auto& f:figures.live){
            const auto& after=postLoad.figures.at(f.slot);const auto& decoy=postLoadDecoys.figures.at(f.slot);
            need(after.status==decoy.status&&after.callbackVA==decoy.callbackVA&&after.images==decoy.images,
                "post-load image computation ignores both saved image words on every original figure");
            if(!after.images){++postLoadBlockedTypes[f.type];continue;}
            need(after.images->second==0&&((f.type==11&&after.callbackVA==0x4d6d30&&after.images->first)||
                (f.type!=11&&after.callbackVA==0x4d6cf0&&after.images->first==0)),
                "type11 selects its callback image while the nineteen clearing types clear both words");
            if(*after.images!=std::make_pair(f.image,f.secondaryImage))++postLoadChanged;else ++postLoadOracleMatched;
            if(after.images->first!=f.image)++postLoadPrimaryChanged;
            if(after.images->second!=f.secondaryImage)++postLoadSecondaryChanged;
            if(after.images->first){++postLoadSelected;need(decodeWord(after.images->first),"type11 post-load selected image decodes");++postLoadDecoded;}
            const auto update=originalFigureFirstUpdate(f,refs,&*updateContext,save.terrain.flags.at(f.cell)),
                decoyUpdate=originalFigureFirstUpdate(decoys.live.at(&f-figures.live.data()),refs,&*decoyUpdateContext,save.terrain.flags.at(f.cell));
            need(update.status==decoyUpdate.status&&bool(update.state)==bool(decoyUpdate.state),
                "first-update computation status ignores saved primary and secondary words");
            if(!update.state){++updateBlockedTypes[f.type];++updateBlockers[originalFigureUpdateStatusName(update.status)];continue;}
            ++updateComputed;++updateComputedTypes[f.type];const auto& u=*update.state;
            need(u.image==decoyUpdate.state->image&&u.secondaryImage==decoyUpdate.state->secondaryImage&&
                u.action==f.action&&u.x==f.x&&u.y==f.y&&u.counter==f.counter&&
                u.herdTarget==decoyUpdate.state->herdTarget&&u.herdDistance==decoyUpdate.state->herdDistance&&
                u.threat==decoyUpdate.state->threat&&u.threatDistance==decoyUpdate.state->threatDistance&&
                u.idleCounter==decoyUpdate.state->idleCounter&&u.cooldowns==decoyUpdate.state->cooldowns,
                "bounded original virtual update computes state from serialized non-image inputs");
            if(f.type==76)need(u.idleCounter==f.idleCounter+1&&u.threat==0&&u.threatDistance==100&&u.movementMode==5&&
                u.hidden==0&&u.freeMovement==0&&u.linkedBuilding==0&&u.progress==f.progress&&u.cell==f.cell,
                "original Pheasant action31 computes target/cooldown/idle prelude without movement");
            const auto selected=figureImage(originalFigureImageInputs(u),refs,save.terrain.flags.at(u.cell),figures.savedView,{},&imageState);
            need(selected.status==OriginalFigureImageStatus::Selected&&selected.image==u.image,
                "consistency only: figureImage of updated state equals updater image write; no saved post-update oracle");
            ++updateConsistent;need(decodeWord(u.image),"computed first-update image decodes");++updateDecoded;
        }
        const auto scoreOne=[&](Score& item,const OriginalFigure& f,const OriginalFigureImageSelection& prediction){
            ++item.total;if(prediction.status==OriginalFigureImageStatus::Selected){++item.selected;
                if(prediction.image==f.image)++item.matched;else ++item.mismatch["selected_image_differs"];
            }else ++item.mismatch[originalFigureImageStatusName(prediction.status)];
        };
        for(const auto& f:figures.live){
            std::optional<uint16_t> owner;if(f.owner>=0&&size_t(f.owner)<save.buildings.slotTypes.size())owner=save.buildings.slotTypes[f.owner];
            const auto prediction=figureImage(originalFigureImageInputs(f),refs,save.terrain.flags.at(f.cell),figures.savedView,owner,&imageState);
            if(prediction.selectorVA)selectors.insert(f.type);
            if(!f.image){
                ++zeroTypes[f.type];const auto input=originalFigureImageInputs(f);
                need(originalFigureImageClearedAtLoad(input),"every original zero-image population reaches the cited clearing callback before ticks");
                const auto first=originalFigureFirstTickImage(input,refs);
                if(first.status==OriginalFigureImageStatus::Selected){
                    ++firstTickSelected;
                    need(f.type==1&&f.effectKind==13&&f.effectVariant==0&&f.effectFlags==36&&!f.updateOwner&&first.code==0x1c0f,
                        "all supplied effects use cited kind13, not guessed smoke/fire/flag state");
                    const auto& a=archives.at(first.image>>14);const auto id=first.image&0x3fff;
                    need(a.imageIndex[14]==490&&a.sprites.at(489).animationSpriteCount==24&&a.sprites.at(489).animationDirections==1,
                        "kind13 first-tick group has independently supplied24 by1 metadata");
                    need(decodeSG3Sprite(a,id,root.resolve("DATA/SprAmbient.555")).image.valid(),"first-tick diagnostic effect image decodes without placement");
                }else {++firstTickBlocked;need(f.type==76&&f.action==19,"only Pheasant action19 first-tick prediction remains blocked");}
                auto independent=f;independent.image=UINT32_MAX;independent.secondaryImage=UINT32_MAX;
                need(originalFigureFirstTickImage(originalFigureImageInputs(independent),refs).image==first.image,
                    "first-tick diagnostics exclude primary and secondary saved image words");
                continue;
            }
            if(f.hidden){scoreOne(hiddenScore,f,prediction);scoreOne(hiddenAction[{f.type,f.action}],f,prediction);continue;}
            const auto cargo=figureSecondaryImage(originalFigureImageInputs(f),refs,figures.savedView,&imageState);
            auto cargoOracle=f;cargoOracle.image=f.secondaryImage;scoreOne(cargoScore,cargoOracle,cargo);
            if(f.secondaryImage){++cargoNonzero;if(cargo.image==f.secondaryImage)++cargoNonzeroMatched;}
            scoreOne(score,f,prediction);scoreOne(byAction[{f.type,f.action}],f,prediction);
            auto independent=f;independent.image=UINT32_MAX;independent.secondaryImage=UINT32_MAX;
            need(figureImage(originalFigureImageInputs(independent),refs,save.terrain.flags.at(f.cell),figures.savedView,owner,&decoyState).image==prediction.image&&
                figureSecondaryImage(originalFigureImageInputs(independent),refs,figures.savedView,&decoyState).image==cargo.image,
                "primary and cargo predictions ignore both image words throughout the linked figure context");
        }
        installOriginalFigureOverlays(scene,save.terrain,figures,refs,0,OriginalFigureRenderSource::Saved,&save.buildings);
        zeroResolved+=scene.figureZeroResolved;zeroBlocked+=scene.figureZeroBlocked;
        need(scene.figureFirstTickImages.size()==scene.figureZeroImage&&view.figureZeroResolved==scene.figureZeroResolved&&
            view.figureZeroBlocked==scene.figureZeroBlocked,"App exposes first-tick diagnostics separately from tick-0 placement");
        size_t selectedImages=0;
        for(const auto& overlay:scene.overlays)if(overlay.mode==4){
            ++selectedImages;const auto& sourceFigure=*std::find_if(figures.live.begin(),figures.live.end(),[&](const auto& f){return f.slot==overlay.sourceSlot;});
            const uint32_t selected=overlay.physicalId-1+(overlay.tileset<<14);
            need(selected==sourceFigure.image||selected==sourceFigure.secondaryImage,"independent inverse v214 mapping equals saved image word");
        }
        need(selectedImages==view.figureDecoded,"every installed figure component decodes, including offscreen");
        for(const auto& t:scene.figureBoundTypes)boundTypes.insert(t.first);
        for(const auto& t:scene.figureBlockedTypes)blocked[t.first]+=t.second;
        need(scene.figurePlaced==view.figurePlaced,"direct binder and App agree");
        auto forwardScene=scene;installOriginalFigureOverlays(forwardScene,save.terrain,figures,refs,0,OriginalFigureRenderSource::Forward,&save.buildings);
        need(forwardScene.figureForwardCompared==scene.figureForwardCompared&&forwardScene.figureForwardMatched==scene.figureForwardMatched&&
            forwardScene.figurePlaced==scene.figureForwardSelected,"explicit forward scene places precisely the reconstructible original figures");
        size_t forwardComponents=0;
        for(const auto& overlay:forwardScene.overlays)if(overlay.mode==4){
            ++forwardComponents;const auto& original=*std::find_if(figures.live.begin(),figures.live.end(),[&](const auto& f){return f.slot==overlay.sourceSlot;});
            const uint32_t word=overlay.physicalId-1+(overlay.tileset<<14);
            need(word==original.image||word==original.secondaryImage,"every forward primary and cargo component equals its independently scored oracle");
        }
        need(forwardComponents==selectedImages,"forward scene reconstructs the complete saved component count, including cargo");
        need(app.selectSavedBuildings(save.terrain,save.buildings,save.layers.ground?&*save.layers.ground:nullptr,0,&figures,OriginalFigureRenderSource::Forward),"App exposes opt-in forward reconstruction without replacing saved default");
        const auto forwardView=app.renderSavedBuildings(camera);
        need(forwardView.figurePlaced==forwardScene.figurePlaced&&forwardView.figureForwardMatched==scene.figureForwardMatched&&
            forwardView.figureForwardCompared==scene.figureForwardCompared&&forwardView.figureDecoded==selectedImages&&
            forwardView.overlayInvalid==0&&forwardView.overlayDecodeFailures==0,"forward App source exposes score and decodes all selected primary and cargo images");
        std::set<size_t> focusSlots;
        for(const auto& overlay:scene.overlays)if(overlay.mode==4)focusSlots.insert(overlay.sourceSlot);
        for(const auto slot:focusSlots){
            const auto& f=*std::find_if(figures.live.begin(),figures.live.end(),[&](const auto& item){return item.slot==slot;});
            auto isolated=scene;isolated.overlays.erase(std::remove_if(isolated.overlays.begin(),isolated.overlays.end(),[&](const auto& item){return item.mode!=4||item.sourceSlot!=slot;}),isolated.overlays.end());
            camera.centerX=f.x;camera.centerY=f.y;OriginalBuildingSceneView oneView;DecodedImage canvas;
            canvas.width=800;canvas.height=800;canvas.pixelsBGRA.assign(800*800,0);
            std::vector<MapImageGridTileset> resources{{2,nullptr,"",201}};
            // Only the isolated figure draws; omit the background surface so
            // painted pixels establish that this particular occurrence blits.
            isolated.surface.image.assign(51984,0);
            for(const auto& entry:archives){auto path=std::filesystem::path(std::string("DATA/")+originalFigureArchiveName(entry.first));path.replace_extension(".555");resources.push_back({entry.first,&entry.second,root.resolve(path.string()),1});}
            need(renderOriginalSceneOverlays(save.terrain,isolated,resources,camera,oneView,canvas)&&oneView.figureDecoded==isolated.overlays.size()&&
                oneView.overlayDrawn==isolated.overlays.size()&&oneView.overlayInvalid==0&&oneView.overlayDecodeFailures==0&&
                std::any_of(canvas.pixelsBGRA.begin(),canvas.pixelsBGRA.end(),[](auto pixel){return pixel>>24;}),"each original figure itself paints a focused generic compositor view");
            ++focused;++placedDecoded;
        }
        std::cout<<"{\"figure_source\":"<<sources<<",\"slots\":"<<figures.declared<<",\"live\":"<<figures.live.size()<<",\"placed\":"<<view.figurePlaced<<",\"hidden\":"<<view.figureHidden<<",\"zero_image\":"<<view.figureZeroImage<<",\"unsupported\":"<<view.figureUnsupported<<",\"decoded_images\":"<<view.figureDecoded<<"}\n";
    }
    need(sources==6&&slots==12000&&live==192&&types.size()==20&&boundTypes.size()==18&&placed==158&&hidden==6&&zero==28&&unsupported==0&&decoded==174&&failures==0&&focused==158&&placedDecoded==158,"six-source original denominator");
    need(postLoadComputed==192&&postLoadBlocked==0&&postLoadChanged+postLoadOracleMatched==192&&
        postLoadSecondaryChanged==16&&postLoadBlockedTypes.empty()&&postLoadSelected==17&&postLoadDecoded==17,
        "post-load image step computes192/192 and decodes17/17 selected callback images");
    std::cout<<"{\"post_load_step\":\"callback_images_before_simulation\",\"computed\":"<<postLoadComputed<<",\"live_total\":"<<live
        <<",\"blocked\":"<<postLoadBlocked<<",\"blocked_types\":{},\"saved_word_comparison\":{\"equal_pairs\":"<<postLoadOracleMatched
        <<",\"changed_pairs\":"<<postLoadChanged<<",\"primary_changed\":"<<postLoadPrimaryChanged<<",\"secondary_clears\":"<<postLoadSecondaryChanged
        <<"},\"selected_nonzero\":"<<postLoadSelected<<",\"decoded\":"<<postLoadDecoded<<",\"decode_failures\":0,\"post_update_oracle\":\"unmeasured\"}\n";
    need(updateComputed==29&&updateDecoded==29&&updateConsistent==29&&updateBlockedTypes[76]==47&&updateComputedTypes[76]==22,
        "first-update state computes29/192, decodes29/29, selector consistency29/29; Pheasant22/69 computed");
    std::cout<<"{\"first_update_state\":true,\"k\":1,\"computed\":"<<updateComputed<<",\"live_total\":"<<live
        <<",\"decoded\":"<<updateDecoded<<",\"image_consistency\":"<<updateConsistent<<",\"decode_failures\":0,\"computed_types\":{\"1\":"<<updateComputedTypes[1]<<",\"76\":"<<updateComputedTypes[76]<<"},\"blocked_types\":{";
    bool updateComma=false;for(const auto& entry:updateBlockedTypes){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<entry.first<<"\":"<<entry.second;}
    std::cout<<"},\"blockers\":{";updateComma=false;for(const auto& entry:updateBlockers){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<entry.first<<"\":"<<entry.second;}
    std::cout<<"},\"first_draw\":\"after_explicit_k_admitted_updates\",\"ordinary_k\":[0,1],\"fast_forward_max_k\":10000,\"post_update_oracle\":\"unmeasured\"}\n";
    const auto printScore=[](const Score& item){
        std::cout<<"\"matched\":"<<item.matched<<",\"compared\":"<<item.total<<",\"selected\":"<<item.selected<<",\"mismatch\":{";
        bool first=true;for(const auto& entry:item.mismatch){if(!first)std::cout<<',';first=false;std::cout<<'"'<<entry.first<<"\":"<<entry.second;}std::cout<<'}';
    };
    for(const auto& entry:byAction){std::cout<<"{\"forward_type\":"<<entry.first.first<<",\"action\":"<<entry.first.second<<',';printScore(entry.second);std::cout<<"}\n";}
    for(const auto& entry:hiddenAction){std::cout<<"{\"forward_hidden_type\":"<<entry.first.first<<",\"action\":"<<entry.first.second<<',';printScore(entry.second);std::cout<<"}\n";}
    std::cout<<"{\"forward_secondary\":true,\"nonzero_matched\":"<<cargoNonzeroMatched<<",\"nonzero_total\":"<<cargoNonzero<<',';printScore(cargoScore);std::cout<<"}\n";
    size_t mismatched=0;for(const auto& entry:score.mismatch)mismatched+=entry.second;
    need(score.total==158&&score.matched+mismatched==158&&hiddenScore.total==6,"forward mismatch classes partition all158 visible nonzero oracles; hidden6 reported separately");
    need(score.matched==158&&score.selected==158&&mismatched==0&&hiddenScore.matched==6&&
        cargoScore.matched==158&&cargoScore.selected==158&&cargoNonzeroMatched==16&&cargoNonzero==16,
        "cited forward selector158/158 with0 misses; hidden6/6");
    need(selectors.size()==20&&zeroResolved==28&&zeroBlocked==0&&zeroTypes[1]==7&&zeroTypes[76]==21&&firstTickSelected==7&&firstTickBlocked==21,
        "zero-image load outcome resolved28/28; first-tick diagnostics selected7, blocked21");
    std::cout<<"{\"zero_image_load\":true,\"resolved\":"<<zeroResolved<<",\"total\":28,\"drawn_before_tick\":0,\"invisible_until_tick\":28,\"blocked\":"<<zeroBlocked<<",\"first_displayed_frame\":\"scheduler_dependent\",\"types\":{\"1\":7,\"76\":21},\"first_tick_image\":{\"selected\":"<<firstTickSelected<<",\"blocked\":"<<firstTickBlocked<<",\"placed\":0}}\n";
    std::cout<<"{\"forward_selector\":true,\"cited_types\":"<<selectors.size()<<",\"live_types\":"<<types.size()<<",\"zero_resolved\":"<<zeroResolved<<",\"zero_total\":28,";printScore(score);std::cout<<",\"hidden\":{";printScore(hiddenScore);std::cout<<"}}\n";
    std::cout<<"{\"sources\":"<<sources<<",\"slots\":"<<slots<<",\"live_figures\":"<<live<<",\"bound_types\":"<<boundTypes.size()<<",\"live_types\":"<<types.size()<<",\"figures_placed\":"<<placed<<",\"figures_placed_and_decoded\":"<<placedDecoded<<",\"focused_figure_blits\":"<<focused<<",\"hidden\":"<<hidden<<",\"zero_image\":"<<zero<<",\"unsupported\":"<<unsupported<<",\"decoded_images\":"<<decoded<<",\"decode_failures\":"<<failures<<",\"tick\":0,\"blocked_types\":{";
    bool first=true;for(const auto& t:blocked){if(!first)std::cout<<',';first=false;std::cout<<'"'<<t.first<<"\":"<<t.second;}std::cout<<"}}\n";
}
}
int main(){try{authored();const size_t synthetic=checks;corpus();std::cout<<"{\"passed\":true,\"authored_checks\":"<<synthetic<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
