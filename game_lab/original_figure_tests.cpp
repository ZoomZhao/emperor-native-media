#include "original_building_scene.hpp"
#include "original_figure_update.hpp"
#include "original_scene_timeline.hpp"
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
    {auto effect=f;effect.type=1;effect.effectKind=13;effect.effectFlags=0x24;effect.alive=1;
        effect.subX=47;effect.subY=62;effect.raise=31;effect.height=4;effect.extraX=-6;effect.extraY=9;
        need(originalFigureSubcell(effect)==std::make_pair(4,-17)&&originalFigureDepthScore(effect)==9,
            "kind13 free placement includes signed extras and both heights while depth ignores all visual offsets");
        effect.action=4;need(originalFigureDepthScore(effect)==-100,"kind13 death depth uses the same minus100 gate");}
    // Literal source wire offsets, decoy values in adjacent enum bytes.
    std::vector<uint8_t> bytes{1,0,1,0,0,0,255,255,0,0,6,0,'F','i','g','u','r','e',7,0};
    std::array<uint8_t,408> raw{};raw[18]=11;raw[22]=1;raw[5]=7;raw[25]=3;raw[26]=6;raw[64]=17;raw[65]=9;
    raw[8]=0x34;raw[9]=0x12;raw[28]=5;raw[30]=6;raw[114]=9;raw[144]=12;raw[122]=3;
    raw[380]=13;raw[381]=2;raw[382]=36;raw[316]=9;
    raw[383]=0xf9;raw[384]=19;raw[386]=0xf8;raw[387]=0xff;raw[388]=3;raw[389]=0xfe;raw[390]=9;raw[132]=6;
    raw[172]=11;raw[176]=12;raw[188]=13;raw[192]=14;raw[204]=199;
    raw[208]=2;raw[212]=1;raw[216]=0xff;raw[217]=raw[218]=raw[219]=0xff;raw[328]=17;raw[128]=5;raw[153]=1;
    raw[66]=3;raw[68]=4;raw[70]=5;raw[368]=2;raw[97]=11;raw[276]=77;raw[400]=19;raw[146]=8;raw[102]=9;
    raw[32]=3;raw[34]=4;raw[52]=7;raw[54]=8;raw[100]=9;raw[104]=10;raw[300]=11;raw[73]=1;
    raw[74]=17;raw[76]=18;raw[78]=1;raw[79]=2;raw[80]=254;raw[81]=5;raw[117]=19;
    raw[86]=20;raw[88]=21;raw[90]=22;raw[92]=23;raw[94]=255;raw[95]=255;raw[96]=2;
    raw[348]=24;raw[350]=25;raw[72]=1;raw[154]=26;raw[156]=27;raw[112]=37;
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
    need(p.route==3&&p.routeCursor==4&&p.routeLength==5&&p.routePhase==2&&p.speed==11&&p.age==77&&
        p.priorProgress==19&&p.reciprocalTarget==8&&p.follower==9,"route and ordered-pool inputs retain separate Figure7 wire requests");
    need(p.priorX==3&&p.priorY==4&&p.homeX==7&&p.homeY==8&&p.home==9&&p.delivery==10&&p.priorCell==11&&p.water==1,
        "crossing prior coordinates, home/delivery and water byte use separate cited Figure7 reads");
    need(p.roamLimit==17&&p.roamSteps==18&&p.roamState==1&&p.roamTurns==2&&p.roamTurn==254&&p.roamCountdown==5&&p.searchCounter==19&&
        p.roamOriginX==24&&p.roamOriginY==25,"roam counters, origin and search byte retain individual Figure7 receiver requests");
    need(p.freeTargetX==20&&p.freeTargetY==21&&p.freeRemainingX==22&&p.freeRemainingY==23&&p.freeError==-1&&p.freeAxis==2&&
        p.cellMovement==1&&p.attackers==std::array<int16_t,2>{26,27},"free movement error/axis and both attacker slots have distinct signed wire inputs");
    need(p.watchCoverage==37,"watchtower coverage is Figure7 byte112, distinct from building service fields");
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
        bird.idleCounter=0;bird.herdTarget=0;bird.threat=0;auto other=bird;other.slot=2;other.x=12;other.y=0;other.action=32;
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
        bird.linkedBuilding=0;bird.action=32;bird.herdTarget=bird.slot;context->figures[bird.slot]=bird;context->modelSpeeds[76]=11;
        need(originalFigureFirstUpdate(bird,supplied,&*context).status==OriginalFigureUpdateStatus::RouteGeneration,"action32 refuses route generation without saved route");
        bird.route=2;bird.routeCursor=3;bird.routeLength=4;bird.progress=7;bird.direction=2;bird.routePhase=2;bird.subX=90;bird.subY=40;
        bird.frame=4;bird.idleCounter=37;const auto walking=originalFigureFirstUpdate(bird,supplied,&*context);
        need(walking.state&&walking.state->action==32&&walking.state->progress==8&&walking.state->subX==91&&walking.state->subY==40&&
            walking.state->routePhase==3&&walking.state->route==2&&walking.state->routeCursor==3&&walking.state->idleCounter==37&&
            walking.state->frame==5,"Pheasant action32 halves model11 to schedule5 before route step then frame advance;idle is retained");
        bird.routePhase=3;const auto pausedWalk=originalFigureFirstUpdate(bird,supplied,&*context);
        need(pausedWalk.state&&pausedWalk.state->progress==7&&pausedWalk.state->subX==90&&pausedWalk.state->routePhase==0&&pausedWalk.state->frame==5,
            "fractional Pheasant pause still advances animation exactly once");
        bird.action=19;need(originalFigureFirstUpdate(bird,supplied,&*context).status==OriginalFigureUpdateStatus::PheasantDestination,
            "action19 refuses destination RNG and map selection after its known action30 assignment");
        profile.slots[1]=profile.slots[0];need(!originalFigureUpdateContext(profile),"duplicate saved slots cannot supply complete updater context");
    }
    {
        std::string model;
        const char* sections[]={"ALL MODS","ALL UNITTYPES","ALL FIGURES","ALL ENEMIES","ALL HEROES"};
        const unsigned counts[]={5,12,83,42,12};
        for(unsigned section=0;section<5;++section){model+=std::string(sections[section])+"\n";
            for(unsigned row=0;row<counts[section];++row){model+="row,{,";
                for(unsigned field=0;field<18;++field)model+=std::to_string(section==2?(field==0?6:field==8?11:0):0)+",";
                model+="}\n";}}
        const auto speeds=originalFigureModelSpeeds(model);
        need(speeds&&speeds->size()==83&&speeds->at(76)==11,"complete authored model reads speed column8 from sequential figure rows");
        need(!originalFigureModelSpeeds(model.substr(0,model.size()-3))&&!originalFigureModelSpeeds(model+"{,}"),
            "truncated or extra model rows withhold all speeds");
        auto badModel=model;badModel.replace(badModel.find("11,"),3,"2147483648,");
        need(!originalFigureModelSpeeds(badModel),"overflowing signed model value refuses instead of narrowing");
    }
    {
        // Independent speed-table examples:3/4,1+3/4,2+3/4 cycles.
        const unsigned schedule5[]={1,1,1,0},schedule11[]={2,2,2,1},schedule17[]={3,3,3,2};
        for(unsigned phase=0;phase<4;++phase){
            need(originalFigureMovementSteps(5,phase)==std::make_pair(schedule5[phase],uint8_t(phase==3?0:phase+1)),"speed5 three moves then one pause");
            need(originalFigureMovementSteps(11,phase).first==schedule11[phase]&&originalFigureMovementSteps(17,phase).first==schedule17[phase],"fractional speed11 and17 retain their phase cycles");
        }
        need(originalFigureMovementSteps(15,0)==std::make_pair(3u,uint8_t(1))&&
            originalFigureMovementSteps(15,1)==std::make_pair(2u,uint8_t(0)),"speed15 performs three then two moves, preserving original phase ordering");
        auto walker=one;walker.route=1;walker.direction=7;walker.progress=8;walker.subX=INT16_MIN;walker.subY=50;
        walker.state85=2;walker.height=5;walker.cartStyle=8;walker.drawClass=1;
        const auto moved=originalFigureFollowRoute(walker,5,0);
        need(moved.state&&moved.state->progress==9&&moved.state->subX==INT16_MAX&&moved.state->subY==49&&
            moved.state->state85==1&&moved.state->height==7&&moved.state->routePhase==1,"route step wraps signed subcoordinates and approaches height twice on flat terrain");
        need(originalFigureFollowRoute(walker,5,0x800000).state->height==6,"elevated terrain permits one height approach");
        walker.progress=9;need(originalFigureFollowRoute(walker,5,0).status==OriginalFigureUpdateStatus::RouteBoundary,"cell crossing refuses without publishing partial progress");
        walker.progress=19;need(originalFigureFollowRoute(walker,5,0).status==OriginalFigureUpdateStatus::RouteBoundary,"segment end refuses owner and next-direction mutations");
        walker.routePhase=3;walker.progress=9;need(originalFigureFollowRoute(walker,5,0).state->progress==9,"zero-step fractional phase avoids a crossing");
        walker.routePhase=0;walker.progress=2;walker.state85=100;
        need(originalFigureFollowRoute(walker,5,0).status==OriginalFigureUpdateStatus::MovementHeight,"bridge lookup refuses missing table context");
        walker.state85=1;const auto clear=originalFigureFollowRoute(walker,5,0);
        need(clear.state&&clear.state->state85==0&&clear.state->drawClass==0&&clear.state->hidden==0&&clear.state->height==5,"last height transition clears draw overrides without changing height");
        walker.progress=8;walker.state85=0;walker.subX=50;
        const auto crossing=originalFigureFollowRoute(walker,12,0);
        need(!crossing.state&&crossing.status==OriginalFigureUpdateStatus::RouteBoundary&&walker.progress==8&&walker.subX==50,
            "two-step call reaching a crossing withholds the earlier tentative step atomically");
    }
    {
        auto group=archive;group.numImages=512;group.sprites.resize(512);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=12;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> refs{{38,&group},{39,&group}};
        OriginalFigureUpdateContext context;context.terrain.assign(51984,0xc0);context.cellHeads.assign(51984,0);context.cropBase=8627;
        auto walker=one;walker.type=11;walker.slot=2;walker.x=5;walker.y=6;walker.cell=10000;walker.progress=9;walker.direction=2;
        walker.route=3;walker.routeLength=2;walker.routeCursor=0;walker.subX=90;walker.subY=120;walker.height=19;walker.state85=100;walker.drawClass=1;
        auto old=walker;old.slot=1;old.next=2;auto other=walker;other.slot=3;other.type=13;other.cell=10001;other.x=6;other.next=0;
        walker.next=0;context.figures={{1,old},{2,walker},{3,other}};context.cellHeads[10000]=1;context.cellHeads[10001]=3;
        context.models[11]={};context.models[11][0]=1;
        const auto blocked=originalFigureFollowRoute(walker,6,0xc0,&context);
        need(!blocked.state&&blocked.status==OriginalFigureUpdateStatus::CellCombat&&context.figures.at(1).next==2&&
            context.cellHeads[10000]==1&&context.figures.at(2).cell==10000,"neighbour combat refusal rolls back unlink, position and bridge writes");
        context.models[11][0]=0;
        {auto peaceful=context;peaceful.models[11][0]=1;peaceful.models[13]={};peaceful.models[13][0]=2;
            const auto share=originalFigureFollowRoute(walker,6,0xc0,&peaceful);
            need(share.state&&share.state->cell==10001&&peaceful.figures.at(3).next==2,
                "ordinary categories1 and2 share destination after zero combat relation without target writes");}
        // A model0 walker has the original combat early exit.
        const auto crossed=originalFigureFollowRoute(walker,6,0xc0,&context);
        need(crossed.state&&crossed.state->x==6&&crossed.state->y==6&&crossed.state->cell==10001&&crossed.state->progress==10&&
            crossed.state->priorX==5&&crossed.state->priorY==6&&crossed.state->priorCell==10000&&crossed.state->subX==91&&
            crossed.state->state85==0&&crossed.state->height==0&&crossed.state->drawClass==0&&crossed.state->water==1&&
            context.figures.at(1).next==0&&context.cellHeads[10001]==3&&context.figures.at(3).next==2,
            "crossing unlinks predecessor, appends tail, retains prior position then clears flat bridge state before substep");
        context.services.emplace();context.services->directions.assign(500000,0);context.services->directions[1500]=4;
        context.services->routeOwners[3]=2;context.movementMasks.assign(51984,4);
        walker=*crossed.state;walker.progress=19;walker.movementMode=3;walker.routeCursor=0;
        const auto advanced=originalFigureFollowRoute(walker,6,0xc0,&context);
        need(advanced.state&&advanced.state->progress==0&&advanced.state->direction==4&&advanced.state->previousDirection==4&&
            advanced.state->routeCursor==1&&advanced.state->subY==121,"segment selects before cursor increment, resets progress and moves in the selected direction");
        auto slope=walker;slope.state85=0;slope.height=9;slope.cartStyle=7;slope.priorX=5;slope.priorY=6;
        context.elevation.assign(51984,0);context.terrain[slope.cell]=0x800640;context.elevation[10000]=255;
        need(originalFigureCellHeight(slope,context)&&slope.state85==40&&slope.height==40&&slope.cartStyle==0&&slope.water==1,
            "elevated entry compares signed minus1 prior height and initializes forty-step downward transition");
        slope.state85=0;context.elevation[10000]=0;context.elevation[10457]=1;
        need(originalFigureCellHeight(slope,context)&&slope.state85==40&&slope.height==0&&slope.cartStyle==40,
            "elevated lookahead reads current route direction two cells ahead rather than movement direction");
        slope.state85=12;slope.height=17;slope.cartStyle=40;context.elevation[10457]=0;
        need(originalFigureCellHeight(slope,context)&&slope.state85==12&&slope.height==17&&slope.cartStyle==40,
            "elevated entry preserves an existing transition even when neighboring heights change");
        context.terrain[slope.cell]=0x400;context.elevation[10229]=1;
        need(originalFigureCellHeight(slope,context)&&slope.state85==20&&slope.height==0&&slope.cartStyle==40,
            "ordinary slope replaces transition and uses one-cell lookahead with twenty-step duration");
        context.elevation.clear();need(!originalFigureCellHeight(slope,context),"elevated height refuses missing same-source grid");
        context.terrain[slope.cell]=0xc0;
        context.movementMasks.clear();context.passability.emplace();context.passability->cells.assign(51984,4);
        need(originalFigureFollowRoute(walker,6,0xc0,&context).status==OriginalFigureUpdateStatus::NextDirection,
            "reduced reachability mask cannot establish bridge and transport gates for next direction");
        context.movementMasks.assign(51984,4);
        walker.routeCursor=2;const auto exhausted=originalFigureFollowRoute(walker,6,0xc0,&context);
        need(exhausted.state&&exhausted.state->direction==8&&exhausted.state->progress==20&&exhausted.state->route==0&&
            context.services->routeOwners[3]==0&&exhausted.state->subY==120,"segment exhaustion releases owner and returns before subcoordinate movement");
        walker.owner=7;need(originalFigureFollowRoute(walker,6,0xc0,&context).status==OriginalFigureUpdateStatus::SegmentService,
            "segment refuses owner virtual service even when route exhaustion is otherwise known");
        auto goods=one;goods.type=19;goods.slot=4;goods.action=7;goods.subtype=10;goods.amount=401;goods.frame=9;goods.progress=5;
        auto cart=one;cart.slot=5;cart.type=20;cart.action=6;cart.leaderLink=4;cart.progress=6;cart.priorProgress=6;cart.homeX=-1;
        context.figures[4]=goods;context.figures[5]=cart;context.figures[4].secondaryImage=123;
        const auto followed=originalFigureFirstUpdate(cart,refs,&context,0xc0);
        need(followed.state&&followed.state->progress==7&&followed.state->frame==9&&followed.linkedWrites.at(4).secondaryImage==0&&
            context.figures.at(4).secondaryImage==123,"cart uses leader minus18 modulo20, copies leader frame and publishes cargo clear atomically");
        cart.homeX=cart.x;cart.homeY=cart.y;cart.progress=20;cart.priorProgress=20;cart.frame=4;
        const auto home=originalFigureFirstUpdate(cart,refs,&context,0xc0);
        need(home.state&&home.state->progress==20&&home.state->frame==4,"cart waiting at home preserves progress20 and its frame");
        OriginalFigure innocent;innocent.type=11;context.models[11][2]=context.models[11][5]=0;
        need(originalFigureHarmlessToPheasant(innocent,context),"zero attack and missile attack prove prey combat qualification false");
        context.models[11][5]=1;need(!originalFigureHarmlessToPheasant(innocent,context),"nonzero missile attack cannot use harmless prey branch");
        auto worker=one;worker.type=11;worker.action=7;worker.home=8;worker.route=3;worker.progress=3;worker.direction=2;
        worker.frame=11;worker.subtype=1;context.figures[worker.slot]=worker;context.modelSpeeds[11]=8;
        OriginalFigureBuildingLink house;house.type=3;house.alive=3;house.worker=int16_t(worker.slot);house.accepting=1;
        context.buildings=std::map<size_t,OriginalFigureBuildingLink>{{8,house}};
        const auto immigrated=originalFigureFirstUpdate(worker,refs,&context,0xc0);
        need(immigrated.state&&immigrated.state->frame==0&&immigrated.state->progress==4&&immigrated.state->movementMode==3&&
            immigrated.state->secondaryImage==0,"worker validates house live3, reciprocal home link and accepting byte before movement and frame wrap");
        context.buildings->at(8).worker=77;
        need(!originalFigureFirstUpdate(worker,refs,&context,0xc0).state,"worker refuses a home whose worker link belongs to another slot");
        context.buildings->at(8).worker=int16_t(worker.slot);context.buildings->at(8).accepting=0;
        need(!originalFigureFirstUpdate(worker,refs,&context,0xc0).state,"house virtual+B8 reads common accepting byte rather than assuming every live house accepts");
        std::vector<uint8_t> data(184);data[0]=1;data[1]=128;data[2]=5;data[4]=3;data[9]=1;data[18]=3;data[47]=9;
        OriginalBuildingProfile buildings;buildings.status=OriginalBuildingStatus::Complete;buildings.parsedRecords=4000;buildings.slotTypes[8]=3;
        OriginalBuildingFootprint b;b.slot=8;b.recordOffset=0;buildings.footprints.push_back(b);
        const auto links=originalFigureBuildingLinks(data,buildings);
        need(links&&links->at(8).alive==3&&links->at(8).type==3&&links->at(8).worker==9&&links->at(8).accepting==1,
            "framed common5 worker and accepting fields use literal wire45 and7 including version");
        data[2]=4;need(!originalFigureBuildingLinks(data,buildings),"uncited common version cannot publish worker/home validity");
        auto carrier=one;carrier.type=22;carrier.action=9;carrier.route=3;carrier.direction=2;carrier.progress=2;
        carrier.subtype=1;carrier.amount=201;carrier.state85=2;carrier.height=7;carrier.cartStyle=8;
        context.figures[carrier.slot]=carrier;context.modelSpeeds[22]=6;
        const auto carried=originalFigureFirstUpdate(carrier,refs,&context,0xc0);
        need(carried.state&&carried.state->height==8&&carried.state->state85==1&&carried.state->secondaryImage==(39u<<14)+10,
            "goods select cargo style before movement reaches equal height, then add resulting direction");
        auto crew=one;crew.slot=2;crew.type=47;crew.action=6;crew.leaderLink=3;crew.x=5;crew.y=6;crew.cell=10000;crew.next=0;
        crew.homeX=crew.homeY=-1;crew.progress=7;crew.state85=0;
        auto trader=crew;trader.slot=3;trader.type=45;trader.action=9;trader.x=6;trader.cell=10001;trader.progress=8;trader.frame=9;trader.faction=0;
        auto predecessor=crew;predecessor.slot=1;predecessor.type=11;predecessor.next=2;
        context.figures={{1,predecessor},{2,crew},{3,trader}};context.cellHeads.assign(51984,0);
        context.cellHeads[10000]=1;context.cellHeads[10001]=3;context.factionModels.emplace();context.factionModels->fill(0);
        auto crewRefs=refs;crewRefs.emplace(14,&group);
        const auto copied=originalFigureFirstUpdate(crew,crewRefs,&context,0xc0);
        need(copied.state&&copied.state->progress==10&&copied.state->x==6&&copied.state->cell==10001&&copied.state->priorX==5&&
            copied.state->priorCell==10000&&copied.state->frame==9&&copied.linkedWrites.at(1).next==0&&copied.linkedWrites.at(3).next==2&&
            context.figures.at(1).next==2&&context.cellHeads[10000]==1,
            "direct trader crew copies leader cell at progress10 and publishes both list edits with prior coordinates atomically");
        context.terrain[10001]=4;const auto crewBridge=originalFigureFirstUpdate(crew,crewRefs,&context,0xc0);
        need(!crewBridge.state&&crewBridge.status==OriginalFigureUpdateStatus::MovementHeight&&crewBridge.linkedWrites.empty()&&
            context.figures.at(1).next==2,"unsupported crew bridge height withholds tentative cell and predecessor writes");
    }
    {
        auto group=archive;group.numImages=512;group.sprites.resize(512);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=12;group.sprites[0].animationDirections=8;
        group.imageIndex[101]=101;group.sprites[100].animationSpriteCount=12;group.sprites[100].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> refs{{38,&group},{39,&group}};
        OriginalFigureUpdateContext context;context.cropBase=8627;context.terrain.assign(51984,0xc0);context.cellHeads.assign(51984,0);
        auto supplier=one;supplier.type=26;supplier.action=8;supplier.owner=8;supplier.counter=2;supplier.frame=11;supplier.variant=0;
        context.figures[supplier.slot]=supplier;context.buildings.emplace();OriginalFigureBuildingLink owner;owner.type=0xc0;owner.alive=1;
        context.buildings->emplace(8,owner);
        const auto supplied=originalFigureFirstUpdate(supplier,refs,&context,0x88);
        need(supplied.state&&supplied.state->counter==1&&supplied.state->action==8&&supplied.state->frame==0&&
            supplied.state->movementMode==17&&supplied.state->hidden==0&&supplied.state->freeMovement==0&&
            supplied.state->image==(39u<<14)+100,"supplier countdown2 becomes1 before action8 family and SG wrap");
        supplier.counter=1;need(!originalFigureFirstUpdate(supplier,refs,&context,0x88).state,
            "supplier expired countdown refuses destination virtual before any state or service output");
        supplier.delivery=9;auto& crop=(*context.buildings)[9];crop.type=0xc4;crop.alive=1;crop.primaryFigure=17;
        crop.cropDelivery=OriginalFigureBuildingLink::CropDelivery{0x103,0x12345678};
        const auto delivered=originalFigureFirstUpdate(supplier,refs,&context,0x88);
        need(delivered.state&&delivered.state->action==6&&delivered.state->counter==0&&delivered.state->hidden==0&&
            delivered.buildings->at(9).primaryFigure==0&&delivered.buildings->at(9).cropDelivery->flags==0x143&&
            delivered.buildings->at(9).cropDelivery->pending==0&&context.buildings->at(9).primaryFigure==17,
            "Millet callback completes delivery preserving unrelated flags and publishes all three building writes atomically");
        std::vector<uint8_t> cropBytes(324);cropBytes[1]=128;cropBytes[2]=5;cropBytes[4]=1;cropBytes[18]=0xc4;
        for(size_t at:{184u,186u,314u,316u,322u})cropBytes[at]=1;
        cropBytes[290]=3;cropBytes[291]=1;cropBytes[318]=0x78;cropBytes[319]=0x56;cropBytes[320]=0x34;cropBytes[321]=0x12;
        cropBytes[127]=0xf9;cropBytes[128]=cropBytes[129]=cropBytes[130]=255;
        OriginalBuildingProfile cropProfile;cropProfile.status=OriginalBuildingStatus::Complete;cropProfile.parsedRecords=4000;
        cropProfile.slotTypes[9]=0xc4;OriginalBuildingFootprint cf;cf.slot=9;cropProfile.footprints.push_back(cf);
        const auto cropLink=originalFigureBuildingLinks(cropBytes,cropProfile);
        need(cropLink&&cropLink->at(9).cropDelivery&&cropLink->at(9).cropDelivery->pending==0x12345678&&
            cropLink->at(9).cropDelivery->flags==0x103&&cropLink->at(9).watchRisk==-7,
            "crop serializer frames embedded flags and full-width pending separately from signed common risk");
        cropBytes[70]=0xf9;cropBytes[71]=255;cropBytes[72]=0x34;cropBytes[73]=0x12;cropBytes[56]=7;
        const auto riskLink=originalFigureBuildingLinks(cropBytes,cropProfile);
        need(riskLink&&riskLink->at(9).fireRisk==-7&&riskLink->at(9).damageRisk==0x1234&&riskLink->at(9).parent==7,
            "common5 Inspector risk and parent fields use literal68/70/54 wire offsets");
        std::vector<uint8_t> marketBytes(450);marketBytes[1]=128;marketBytes[2]=5;marketBytes[18]=0x3b;
        marketBytes[184]=marketBytes[186]=1;marketBytes[314]=2;marketBytes[400]=6;
        for(unsigned n=0;n<6;++n){marketBytes[316+14*n]=1;marketBytes[326+14*n]=0x90;marketBytes[327+14*n]=1;}
        marketBytes[318]=28;marketBytes[322]=0x23;marketBytes[323]=1;marketBytes[426]=40;marketBytes[264]=0x34;marketBytes[265]=0x12;
        auto marketProfile=cropProfile;marketProfile.slotTypes[9]=0x3b;
        const auto marketLink=originalFigureBuildingLinks(marketBytes,marketProfile);
        need(marketLink&&marketLink->at(9).marketStorage&&marketLink->at(9).marketStorage->at(0).amount==291&&
            marketLink->at(9).marketStorage->at(0).resource==28&&marketLink->at(9).marketDemand->at(1)==0x1234&&*marketLink->at(9).marketQuality==40,
            "Market frames each14-byte compartment and reads demand tail78/quality tail242 after storage2 and child6");
        auto returning=supplier;returning.action=6;returning.counter=0;context.figures[returning.slot]=returning;
        auto& hold=context.buildings->at(8);hold.type=0xc1;hold.cropSearchComplete=true;hold.suppliers=2;hold.roadX=7;hold.roadY=8;
        crop.primaryFigure=0;context.services.emplace();context.services->directions.assign(500000,0);
        const auto noCrop=originalFigureFirstUpdate(returning,refs,&context,0x88);
        need(noCrop.state&&noCrop.state->action==9&&noCrop.state->hidden==1&&noCrop.state->counter==0&&
            noCrop.state->destinationX==7&&noCrop.state->destinationY==8&&*noCrop.buildings->at(8).suppliers==1,
            "expired supplier with no reciprocally linked crop returns to owner road and decrements carried supplier count");
        crop.primaryFigure=int16_t(returning.slot);
        need(!originalFigureFirstUpdate(returning,refs,&context,0x88).state&&*context.buildings->at(8).suppliers==2,
            "linked crop search refuses missing calendar and availability rather than treating delivery completion as no crop");
        context.crops.emplace();context.crops->month=6;crop.destinationEnabled=1;crop.cell=100;crop.x=9;crop.y=10;
        const auto selectedCrop=originalFigureFirstUpdate(returning,refs,&context,0x88);
        need(selectedCrop.state&&selectedCrop.state->action==7&&selectedCrop.state->delivery==9&&selectedCrop.state->destinationX==9&&
            selectedCrop.state->destinationY==10&&selectedCrop.state->hidden==1&&*selectedCrop.buildings->at(8).suppliers==2&&
            selectedCrop.buildings->at(9).primaryFigure==int(returning.slot),"first available reciprocal Millet selects destination without clearing links or supplier count");
        context.crops->month=10;auto& laterCrop=(*context.buildings)[10];laterCrop=crop;laterCrop.type=0xc6;
        need(originalFigureSupplierCrop(returning,context)==0,"first Millet out of season returns home even when later reciprocal crop is in season");
        context.crops->disabled[0xc4]=1;
        need(originalFigureSupplierCrop(returning,context)==10,"disabled first crop is skipped before selecting later in-season candidate");
        context.crops->unavailable[3]=1;need(originalFigureSupplierCrop(returning,context)==0,"resource availability independently excludes reciprocal candidates");
        context.crops->unavailable[3]=0;context.terrain[100]|=0x100;
        need(originalFigureSupplierCrop(returning,context)==0,"crop terrain100 excludes otherwise qualified reciprocal candidate");
        need(originalFigureCropInSeason(0xc4,6)==true&&originalFigureCropInSeason(0xc4,9)==true&&originalFigureCropInSeason(0xc4,10)==false&&
            originalFigureCropInSeason(0xc4,5)==false&&originalFigureCropInSeason(0xc2,6)==true&&!originalFigureCropInSeason(0xc1,6),"crop month intervals are half-open and unknown rows refuse");
        OriginalFigureBuildingLink market,house;market.type=0x3b;market.marketQuality=60;market.marketDemand=std::array<int16_t,7>{};
        market.marketStorage=std::vector<OriginalStorageCompartment>{{28,3,400},{28,20,400},{19,7,400},{19,30,400},{13,40,400},{0,0,400}};
        house.type=7;house.residents=10;house.accepting=1;house.size=1;house.houseModel=4;house.foodQuality=20;house.houseGoods=std::array<int16_t,12>{};
        for(unsigned col=9;col<=13;++col)context.houseMarketModels[col][5]=col==9?12:0;
        int served=0;need(originalFigureMarketHouse(market,house,context,served)&&served==1&&house.houseGoods->at(0)==5&&house.houseGoods->at(1)==12&&
            *house.foodQuality==60&&market.marketStorage->at(0).amount==0&&market.marketStorage->at(0).resource==28&&market.marketStorage->at(1).amount==18&&
            market.marketStorage->at(2).amount==0&&market.marketStorage->at(3).amount==25&&market.marketDemand->at(1)==2000&&*market.marketGoods==83,
            "Market distributes food and hemp, consumes smallest compartments first and carries demand/quality atomically without clearing exhausted resource");
        market.marketDemand->at(1)=INT16_MIN;house.residents=0;house.type=7;
        need(originalFigureMarketHouse(market,house,context,served)&&served==1&&market.marketDemand->at(1)==INT16_MIN,
            "Market non-elite empty house returns0 without touching demand");
        house.type=7;house.residents=2;house.houseGoods->fill(0);house.foodQuality=0;market.marketStorage->assign(6,{0,0,400});
        market.marketStorage->at(0)={28,1,800};market.marketQuality=60;
        need(originalFigureMarketHouse(market,house,context,served)&&*market.marketQuality==0&&*house.foodQuality==60,
            "last food allocation clears Market quality but uses entry quality for that house callback");
        auto delegated=one;delegated.type=34;delegated.action=6;delegated.owner=8;delegated.counter=10;delegated.searchCounter=99;
        context.figures[delegated.slot]=delegated;context.buildings->at(8).type=0xd3;context.models[34]={};context.models[34][15]=36;
        const auto waiting=originalFigureFirstUpdate(delegated,refs,&context,0xc0);
        need(waiting.state&&waiting.state->counter==9&&waiting.state->hidden==1&&waiting.state->frame==0&&
            waiting.state->searchCounter==0&&waiting.state->movementMode==18&&waiting.state->roamLimit==3456,
            "D3 countdown clears search byte and writes model15 times96 before hidden image");
        auto roamer=one;roamer.type=30;roamer.progress=7;roamer.direction=2;roamer.roamState=1;roamer.roamSteps=2688;
        roamer.subX=600;roamer.subY=1040;const auto roaming=originalFigureRoam(roamer,6,0xc0,context);
        need(roaming.state&&roaming.state->roamSteps==2694&&roaming.state->progress==8&&roaming.state->subX==601&&
            roaming.state->route==roamer.route,"roam increments distance by speed6 while moving exactly one substep without allocating a route");
        roamer.progress=19;const auto segment=originalFigureRoam(roamer,6,0xc0,context);
        need(!segment.state&&segment.status==OriginalFigureUpdateStatus::SegmentService,
            "roam segment refuses owner/grid service and road choice atomically");
        auto free=one;free.slot=2;free.x=5;free.y=6;free.cell=10000;free.subX=100;free.subY=120;free.next=0;
        free.freeRemainingX=1;free.freeRemainingY=0;free.freeError=-1;free.freeAxis=1;free.freeTargetX=101;free.freeTargetY=120;free.raise=1;
        auto companion=free;companion.slot=3;companion.next=2;context.figures={{2,free},{3,companion}};context.cellHeads[10000]=3;
        const auto walked=originalFigureFreeStep(free,context);
        need(walked&&!walked->second&&walked->first.subX==101&&walked->first.freeRemainingX==0&&walked->first.raise==0&&
            walked->first.x==5&&walked->first.y==6&&walked->first.cell==10000&&context.figures.at(3).next==2,
            "last free step moves and relinks but does not report arrival until next call");
        const auto arrived=originalFigureFreeStep(walked->first,context);
        need(arrived&&arrived->second&&arrived->first.subX==101,"free arrival tests zero remaining before attempting another step");
        need(originalFigureFollowerSteps(13,2)==2&&originalFigureFollowerSteps(13,3)==1&&
            originalFigureMovementSteps(13,3).first==3,"follower speed13 observes two then one rather than mutating leader's route schedule");
        context.models[76]={};context.models[76][0]=6;context.models[40]={};context.models[40][0]=2;context.models[40][2]=3;context.models[40][5]=6;
        auto hunter=one;hunter.type=40;
        need(originalFigureHarmlessToPheasant(hunter,context),"category6 versus category2 relation byte is false even with nonzero Hunter attacks");
        context.models[76][0]=5;need(!originalFigureHarmlessToPheasant(hunter,context),"uncited predator relation does not inherit the prey table fact");
        auto inspector=one;inspector.type=39;inspector.action=8;inspector.owner=8;context.figures[inspector.slot]=inspector;
        context.modelSpeeds[39]=8;
        context.models[39]={};context.buildings->at(8).type=0x7c;context.buildings->at(8).primaryFigure=int16_t(inspector.slot);
        const auto withheld=originalFigureFirstUpdate(inspector,refs,&context,0xc0);
        need(!withheld.state&&withheld.status==OriginalFigureUpdateStatus::InspectorFireContext,
            "inspector action8 requires the serialized count before publishing counter or movement");
        inspector.x=5;inspector.y=6;inspector.cell=10000;inspector.subX=100;inspector.subY=120;
        inspector.next=0;inspector.direction=2;inspector.progress=7;inspector.roamState=1;
        inspector.roamSteps=10;inspector.searchCounter=19;context.models[39][15]=40;
        context.figures={{inspector.slot,inspector}};context.cellHeads[10000]=uint16_t(inspector.slot);context.services.emplace();
        context.services->fireCount=0;
        const auto noFire=originalFigureFirstUpdate(inspector,refs,&context,0xc0);
        need(noFire.state&&noFire.state->searchCounter==19&&noFire.state->roamSteps==18&&noFire.state->progress==8&&
            noFire.state->subX==101&&noFire.state->frame==uint8_t(inspector.frame+1)&&noFire.state->hidden==0&&
            noFire.services->fireCount==0,"serialized zero fire count preserves search byte and performs Inspector action8 roam before SG frame advance");
        inspector.searchCounter=18;context.services->fireCount=1;
        const auto positiveFire=originalFigureFirstUpdate(inspector,refs,&context,0xc0);
        need(positiveFire.state&&positiveFire.state->searchCounter==19&&positiveFire.state->roamSteps==18,
            "positive saved fire count increments search byte before its new-value20 search threshold");
        inspector.searchCounter=19;const auto searchFire=originalFigureFirstUpdate(inspector,refs,&context,0xc0);
        need(!searchFire.state&&searchFire.status==OriginalFigureUpdateStatus::InspectorFireSearch&&context.figures.at(inspector.slot).searchCounter==19,
            "Inspector fire target search at20 remains explicitly unported and publishes no partial counter or movement");
        inspector.searchCounter=255;const auto wrappedFire=originalFigureFirstUpdate(inspector,refs,&context,0xc0);
        need(wrappedFire.state&&wrappedFire.state->searchCounter==0,"Inspector search counter wraps as a byte before the unsigned20 comparison");
        inspector.searchCounter=19;context.services->fireCount=-1;
        need(originalFigureFirstUpdate(inspector,refs,&context,0xc0).state->searchCounter==19,
            "negative serialized fire count uses the signed less-than-one early return");
        OriginalBuildingProfile buildings;buildings.status=OriginalBuildingStatus::Complete;buildings.parsedRecords=4000;
        OriginalMapTerrain map;map.activeSide=84;map.flags.assign(51984,0);map.flags[10000]=0xc0;map.flags[10001]=0x81;map.flags[10002]=0x100;
        const auto masks=originalFigureRoadMovementMasks(map,buildings);
        need(masks&&masks->at(10000)==4&&masks->at(10001)==0&&masks->at(10002)==0xffff,
            "route projection proves road4 and clear plain A00 bits while unresolved masks remainFFFF");
        buildings.slotTypes[1]=0xd2;need(!originalFigureRoadMovementMasks(map,buildings),
            "typeD2 final virtual270 can rewrite derived masks and withholds the projection");
    }
    {
        OriginalFigureGoodsServices goods;goods.policies.resize(1);goods.warehouse[25]=1000;
        OriginalFigureBuildingLink warehouse;warehouse.type=0x36;warehouse.storagePolicy=0;
        warehouse.storage=std::vector<OriginalStorageCompartment>{{25,200,400},{25,100,400},{25,100,400},{24,70,400}};
        const auto rest=originalFigureStorageWithdraw(warehouse,25,150,true,goods);
        need(rest==0&&warehouse.storage->at(0).amount==200&&warehouse.storage->at(1).resource==0&&
            warehouse.storage->at(2).amount==50&&goods.warehouse[25]==850,
            "withdrawal consumes smallest positive first tie before larger compartments and debits warehouse accounting");
        const auto shortfall=originalFigureStorageWithdraw(warehouse,25,400,true,goods);
        need(shortfall==150&&warehouse.storage->at(0).resource==0&&warehouse.storage->at(2).resource==0&&
            warehouse.storage->at(3).amount==70&&goods.warehouse[25]==600,
            "withdrawal returns unremoved remainder and clears only exhausted matching identities");
        warehouse.type=0x3a;warehouse.storage=std::vector<OriginalStorageCompartment>{{25,80,400}};
        goods.policies[0].trade[25]=7;goods.trade7[25]=40;
        need(originalFigureStorageWithdraw(warehouse,25,50,false,goods)==0&&goods.trade7[25]==UINT32_MAX-9&&
            warehouse.storage->front().amount==30,"station mode7 accounting retains unsigned x86 subtraction");
        goods.policies[0].trade[25]=5;goods.trade5[25]=30;
        need(originalFigureStorageWithdraw(warehouse,25,30,false,goods)==0&&goods.trade5[25]==0&&warehouse.storage->front().resource==25,
            "station mode5 uses its own accounting and clear false preserves identity");
        warehouse.storage->front().capacity=800;
        need(!originalFigureStorageWithdraw(warehouse,25,1,true,goods),"uncited storage capacity refuses before mutation");
        OriginalFigureUpdateContext context;context.cropBase=0;context.goods=goods;context.goods->storageSearch=std::array<uint8_t,36>{};
        context.goods->industrySearch=std::array<uint8_t,36>{};context.goods->godGate=0;context.buildings.emplace();
        context.services.emplace();context.services->directions.assign(500000,0);
        auto carrier=one;carrier.type=19;carrier.subtype=25;carrier.movementMode=1;carrier.routeCursor=8;carrier.routeLength=20;
        context.figures[carrier.slot]=carrier;context.services->routeOwners[1]=99;
        const auto missing=originalFigureGoodsSupplier(carrier,context);
        need(missing==0&&context.goods->storageSearch->at(25)==24&&context.goods->industrySearch->at(25)==1&&
            carrier.delivery==0&&carrier.watchCoverage==1&&carrier.routeCursor==0&&carrier.routeLength==0&&context.services->routeOwners[1]==0,
            "empty supplier search carries cache bits and allocator stale-owner cleanup despite no destination");
        auto absent=context;absent.goods->storageSearch.reset();
        need(!originalFigureGoodsSupplier(carrier,absent),"unknown process cache is unavailable rather than a zeroed premise");
        auto granary=context;OriginalFigureBuildingLink unported;unported.type=0x38;unported.alive=1;unported.roadAccess=1;unported.roadDistance=1;
        granary.buildings->emplace(10,unported);
        need(!originalFigureGoodsSupplier(carrier,granary),"type38 trade-storage candidate refuses before reporting a no-candidate station search");
        OriginalFigureBuildingLink candidate;candidate.type=0x36;candidate.alive=1;candidate.roadAccess=1;candidate.roadDistance=1;
        candidate.workers=1;candidate.storagePolicy=0;candidate.storage=std::vector<OriginalStorageCompartment>(8,{0,0,400});
        context.buildings->emplace(8,candidate);context.buildingEmployment[0x36]=1;context.goods->policies[0].limit[25]=400;
        auto group=archive;group.numImages=512;group.sprites.resize(512);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=12;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> refs{{38,&group},{39,&group}};
        carrier.action=6;carrier.counter=30;carrier.owner=9;carrier.route=0;carrier.resource=25;
        context.buildings->emplace(9,candidate);context.figures[carrier.slot]=carrier;
        const auto refused=originalFigureFirstUpdate(carrier,refs,&context,0xc0);
        need(!refused.state&&!refused.goods&&!refused.services&&!refused.buildings&&carrier.counter==30&&
            context.goods->storageSearch->at(25)==24,"positive warehouse candidate refuses all tentative cache and figure writes");
        context.buildings->erase(8);context.buildings->erase(9);OriginalFigureBuildingLink station;
        station.type=0x3a;station.alive=1;station.cargoFigures[0]=int16_t(carrier.slot);context.buildings->emplace(9,station);
        candidate.type=0x36;candidate.storage->at(0)={25,100,400};context.buildings->emplace(8,candidate);
        // Authored SG metadata exercises the EXISTING storage selector.
        auto general=group;general.version=213;general.numImages=600;general.sprites.resize(600);general.imageIndex.fill(201);
        general.bitmaps.resize(2);general.bitmaps[0].filename="China_StorNDist.bmp";general.bitmaps[1].filename="China_Stor-bays.bmp";
        general.imageIndex[0x454&511]=300;general.imageIndex[0x456&511]=500;
        for(auto& sprite:general.sprites){sprite.type=30;sprite.width=78;sprite.height=40;sprite.uncompressedLength=3200;
            sprite.tileSize=1;sprite.mirrorOffset=0;sprite.animationSpriteCount=0;}
        for(size_t k=299;k<500;++k)general.sprites[k].bitmapId=1;
        OriginalBuildingScene scene;scene.accepted=true;scene.surface.terrain.assign(51984,0xc0);scene.surface.image.assign(51984,0);
        OriginalBuildingPlacement placement;placement.footprint.slot=8;placement.footprint.type=0x36;placement.footprint.size=3;
        placement.footprint.imageInputs.emplace();placement.footprint.imageInputs->compartments=candidate.storage;
        placement.components=*bindOriginalBuildingComponents(placement.footprint,"DATA/China_General.sg3",general);
        placement.binding=placement.components.front().binding;scene.placed.push_back(placement);
        carrier.type=21;carrier.action=12;carrier.counter=3;carrier.delivery=8;carrier.homeX=4;carrier.homeY=7;
        context.figures[carrier.slot]=carrier;context.goods->warehouse[25]=100;
        const auto delay=originalFigureFirstUpdate(carrier,refs,&context,0xc0);
        need(delay.state&&delay.state->counter==4&&delay.state->action==12&&delay.buildings->at(8).storage->front().amount==100,
            "type21 action12 waits through counter4 without withdrawing");
        carrier.counter=4;const auto loaded=originalFigureFirstUpdate(carrier,refs,&context,0xc0);
        need(loaded.state&&loaded.state->action==10&&loaded.state->counter==0&&loaded.state->amount==100&&loaded.state->subtype==25&&
            loaded.state->destinationX==4&&loaded.state->destinationY==7&&loaded.buildings->at(8).storage->front().resource==0&&
            loaded.goods->warehouse[25]==0&&context.buildings->at(8).storage->front().amount==100,
            "type21 withdrawal publishes storage accounting and home route transition together after counter4");
        context.terrain=scene.surface.terrain;context.cellHeads.assign(51984,0);context.cellHeads[carrier.cell]=uint16_t(carrier.slot);
        for(size_t slot=0;slot<2000;++slot)if(!context.figures.count(slot)){OriginalFigure inactive;inactive.slot=slot;context.figures.emplace(slot,inactive);}
        context.figures[carrier.slot]=carrier;
        OriginalSceneTimelineFrame frame;frame.animationClock=37;frame.admission.now=37;frame.admission.speed=80;frame.continueAfterPass={false};
        const auto timeline=originalSceneTimeline(context,refs,scene,general,nullptr,0,{}, {frame});
        need(timeline.complete&&timeline.buildingImageReselections==1&&timeline.buildingStorage.at(8).front().resource==0,
            "shared timeline reselects a warehouse once when resource and amount change in a successful pool pass");
        OriginalFigureProfile saved;saved.status=OriginalFigureStatus::Complete;saved.declared=saved.parsed=2000;saved.live={carrier};
        saved.slots.resize(2000);for(size_t slot=0;slot<2000;++slot)saved.slots[slot]=context.figures.at(slot);
        OriginalMapTerrain terrain;terrain.activeSide=228;terrain.flags=scene.surface.terrain;
        OriginalBuildingProfile buildings;buildings.slotTypes[8]=0x36;buildings.slotTypes[9]=0x3a;
        auto installed=scene;
        need(installOriginalSceneTimeline(installed,terrain,saved,refs,buildings,general,timeline)&&
            installed.placed.front().components.at(1).binding.imageCode==0x456&&installed.surface.image[1]==((2u<<14)|299u)&&
            scene.placed.front().components.at(1).binding.imageCode==0x454,
            "timeline atomically installs existing empty-bay selector output in component and surface grids");
        auto invalid=timeline;invalid.buildingStorage.at(8).front().capacity=800;auto unchanged=scene;
        need(!installOriginalSceneTimeline(unchanged,terrain,saved,refs,buildings,general,invalid)&&
            unchanged.surface.image==scene.surface.image&&unchanged.placed.front().components.at(1).binding.imageCode==0x454,
            "failed storage reselection leaves the entire original scene unchanged");
        {OriginalFigureUpdateContext shot;shot.services.emplace();shot.services->directions.assign(500000,0);
            shot.services->allocationSerial=17;shot.services->randomFirst=1;shot.services->randomSecond=16;shot.crtRandom=1;
            shot.animationFlags=std::array<bool,51>{};shot.animationFlags->at(2)=true;shot.cropBase=0;
            shot.terrain.assign(51984,0x80);shot.buildingOwners.assign(51984,0);shot.cellHeads.assign(51984,0);
            for(size_t slot=0;slot<2000;++slot){OriginalFigure idle;idle.slot=slot;shot.figures.emplace(slot,idle);}
            auto hunter=one;hunter.slot=2;hunter.type=40;hunter.action=7;hunter.counter=0;hunter.target=1;hunter.owner=9;
            hunter.x=10;hunter.y=10;hunter.cell=2290;hunter.subX=hunter.subY=200;hunter.next=0;
            auto prey=one;prey.slot=1;prey.type=76;prey.action=32;prey.x=5;prey.y=15;prey.cell=3425;prey.updateOwner=1;
            shot.figures.at(1)=prey;shot.figures.at(2)=hunter;shot.cellHeads[2290]=2;shot.cellHeads[3425]=1;
            shot.freeSlots={1999,1998};shot.highestLiveSlot=2;shot.buildings.emplace();shot.buildings->emplace(9,station);
            shot.models[40].fill(0);shot.models[40][6]=5;shot.models[40][7]=30;
            auto projectiles=group;projectiles.numImages=512;projectiles.sprites.resize(512);projectiles.imageIndex.fill(1);
            projectiles.imageIndex[0x6d-1]=101;projectiles.imageIndex[0x6c-1]=201;
            projectiles.sprites[100].animationSpriteCount=projectiles.sprites[200].animationSpriteCount=2;
            projectiles.sprites[100].animationDirections=projectiles.sprites[200].animationDirections=32;
            projectiles.sprites[100].animationSpeedId=projectiles.sprites[200].animationSpeedId=2;
            const std::map<uint32_t,const SG3Archive*> shotRefs{{14,&projectiles},{39,&group}};
            const auto pass=originalFigureOrderedNextPass(shot,shotRefs);
            need(pass.complete&&pass.computed==3&&pass.context->allocatedSlots==std::set<size_t>{1999}&&
                pass.context->freeSlots==std::vector<size_t>{1998}&&pass.context->highestLiveSlot==1999,
                "Hunter uses FIFO1999 allocation and the same ascending ordered pass visits the new projectile");
            const auto& p=pass.context->figures.at(1999);const auto& h=pass.context->figures.at(2);const auto& t=pass.context->figures.at(1);
            need(p.age==1&&p.faction==0&&p.home==1&&p.subX==193&&p.subY==207&&p.raise==3&&p.direction==20&&p.frame==1&&p.attackFrame==0&&
                p.allocationSerial==17&&pass.context->services->allocationSerial==18&&pass.context->crtRandom==2745024&&pass.context->crtRandomCalls==1&&
                h.action==13&&h.counter==30&&h.frame==1&&t.action==18&&t.previousAction==32&&t.counter==100&&t.direction==1,
                "shot carries allocation serial, CRT state, seven free steps, both frame flags and earlier-prey writes atomically");
            const auto projectileImages=originalFigureUpdateImageState(*pass.context);
            need(p.image==(14u<<14)+156&&p.secondaryImage==(14u<<14)+224&&
                figureImage(originalFigureImageInputs(p),shotRefs,0,0,std::nullopt,&projectileImages).image==p.image&&originalFigureSubcell(p)==std::make_pair(12,-1),
                "projectile forward primary and secondary use32 directions with the cited four-sector bias and raised free position");
            need(pass.context->primaryImageInputs.at(1).action==32&&pass.context->primaryImageInputs.at(1).direction==prey.direction&&
                pass.context->figures.at(1).action==18&&pass.context->figures.at(1).direction==1,
                "later Hunter turn preserves earlier prey image-write inputs without selecting a replacement image");
            auto arc=p;arc.projectileDistance=6;arc.projectileStartHeight=-8;arc.projectileTargetHeight=-2;arc.projectileEndHeight=18;
            arc.x=10;arc.y=10;arc.destinationX=10;arc.destinationY=16;
            need(originalFigureProjectileArc(arc)==-15,"projectile initial arc truncates separate weighted height terms");
            arc.y=13;need(originalFigureProjectileArc(arc)==-23,"projectile half-distance arc includes both endpoint heights");
            arc.y=16;need(originalFigureProjectileArc(arc)==-2,"projectile endpoint arc retains target height");
            auto draw=p;draw.slot=4;draw.x=draw.y=0;draw.cell=72*229;draw.subX=13;draw.subY=7;
            draw.image=14u<<14;draw.secondaryImage=(14u<<14)+1;draw.projectileDistance=6;draw.projectileEndHeight=18;
            draw.destinationX=0;draw.destinationY=6;
            auto drawProfile=figures;drawProfile.live={draw};drawProfile.cellHeads.assign(51984,0);drawProfile.cellHeads[draw.cell]=4;
            OriginalMapTerrain drawTerrain;drawTerrain.activeSide=84;drawTerrain.flags.assign(51984,0x80000);
            for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)drawTerrain.flags[y*228+x]=0;
            OriginalBuildingProfile drawBuildings;drawBuildings.status=OriginalBuildingStatus::Complete;
            auto drawScene=composeOriginalBuildingScene(drawTerrain,drawBuildings,archive);
            const std::map<uint32_t,const SG3Archive*> drawRefs{{14,&archive}};
            installOriginalFigureOverlays(drawScene,drawTerrain,drawProfile,drawRefs);
            need(drawScene.figurePlaced==1&&drawScene.overlays.size()==2&&drawScene.overlays[0].offsetX==43&&
                drawScene.overlays[0].offsetY==-23&&drawScene.overlays[1].offsetX==43&&drawScene.overlays[1].offsetY==-6&&
                drawScene.overlays[1].depth<drawScene.overlays[0].depth,
                "projectile primary arc and secondary ground anchor retain selected SG offsets and shadow-before-primary order");
            auto missingShot=shot;missingShot.crtRandom.reset();const auto refusedShot=originalFigureOrderedNextPass(missingShot,shotRefs);
            need(!refusedShot.complete&&!refusedShot.context&&refusedShot.firstBlockedSlot==2&&
                refusedShot.figures.at(2).status==OriginalFigureUpdateStatus::CrtRandomRequired&&
                missingShot.figures.at(1).action==32&&missingShot.freeSlots.size()==2&&missingShot.crtRandomCalls==0,
                "unknown CRT process state withholds projectile allocation and all linked prey writes");
            auto zeroShot=shot;zeroShot.crtRandom=0;const auto zeroPass=originalFigureOrderedNextPass(zeroShot,shotRefs);
            need(zeroPass.complete&&zeroPass.context->crtRandom==2531011&&zeroPass.context->crtRandomCalls==1&&
                zeroPass.context->figures.at(1999).image==p.image,"explicit CRT state zero is valid and advances independently of image selection");
            auto exhausted=missingShot;exhausted.freeSlots.clear();const auto noAllocation=originalFigureOrderedNextPass(exhausted,shotRefs);
            need(noAllocation.complete&&!noAllocation.context->crtRandom&&noAllocation.context->crtRandomCalls==0&&
                noAllocation.context->figures.at(2).action==13,"exhausted FIFO does not reach allocation sound rand and needs no CRT state");
            need(originalFigureProjectileDirection(10,10,5,15)==20&&originalFigureProjectileDirection(10,10,11,15)==16&&
                originalFigureAttackDirection(10,10,11,15)==4,"projectile32 and Hunter8 direction thresholds remain distinct");}
        carrier.destinationX=6;carrier.destinationY=7;carrier.subX=100;carrier.subY=120;originalFigureFreeTarget(carrier);
        need(carrier.freeRemainingX==20&&carrier.freeRemainingY==20&&carrier.freeError==0&&carrier.freeAxis==1,
            "free target equal axes use zero Bresenham error rather than the unequal-axis expression");
        context.figures[carrier.slot].alive=0;context.goods->storageSearch->at(25)=1;context.goods->industrySearch->at(25)=1;
        context.goods->savedPhase=2;const auto reset=originalFigureOrderedNextPass(context,refs);
        need(reset.complete&&reset.context->goods->storageSearch->at(25)==0&&reset.context->goods->industrySearch->at(25)==0&&
            context.goods->storageSearch->at(25)==1,"saved phase2 clears both persistent search caches on the private ordered pass");
        context.goods->savedPhase=1;const auto retained=originalFigureOrderedNextPass(context,refs);
        need(retained.complete&&retained.context->goods->storageSearch->at(25)==1&&retained.context->goods->industrySearch->at(25)==1,
            "other saved phases retain both process search caches across passes");
        auto watch=one;watch.type=29;watch.action=8;watch.owner=9;watch.frame=11;watch.direction=2;watch.progress=2;
        watch.roamState=1;watch.roamSteps=0;watch.x=5;watch.y=6;watch.cell=10000;watch.subX=100;watch.subY=120;
        context.buildings->at(9).type=0x7f;context.buildings->at(9).primaryFigure=int16_t(watch.slot);
        context.models[29]={};context.models[29][15]=30;context.modelSpeeds[29]=6;context.figures[watch.slot]=watch;
        context.services->criminalCount=0;
        const auto wrapped=originalFigureFirstUpdate(watch,refs,&context,0xc0);
        need(wrapped.state&&wrapped.state->frame==0&&wrapped.state->progress==3,
            "Watchtower frame wrap uses saved zero criminal count before continuing normal roam");
        context.services->criminalCount.reset();need(!originalFigureFirstUpdate(watch,refs,&context,0xc0).state,
            "Watchtower frame wrap refuses unknown criminal count");
        context.services->criminalCount=1;need(!originalFigureFirstUpdate(watch,refs,&context,0xc0).state,
            "positive criminal count requires target search rather than taking the zero-count branch");
        auto school=watch;school.type=34;school.action=6;school.counter=1;school.owner=9;school.subX=100;school.subY=120;
        context.models[34]={};context.models[34][15]=30;context.buildings->at(9).type=0xd3;
        context.buildings->at(9).x=5;context.buildings->at(9).y=6;context.buildings->at(9).size=1;
        context.buildings->at(9).roadX=5;context.buildings->at(9).roadY=6;context.buildings->at(9).delegatedCount=1;
        context.activeSide=84;context.cropBase=8627;context.buildingOwners.assign(51984,0);context.figures[school.slot]=school;
        const auto departed=originalFigureFirstUpdate(school,refs,&context,0xc0);
        need(departed.state&&departed.state->action==7&&departed.state->counter==0&&departed.state->hidden==1&&
            departed.state->freeRemainingX==0&&departed.state->freeRemainingY==0&&departed.state->destinationX==5,
            "MusicSchool counter1 initializes owner-road free target without moving in the countdown call");
        school=*departed.state;context.figures[school.slot]=school;context.cellHeads.assign(51984,0);context.cellHeads[10000]=uint16_t(school.slot);
        OriginalFigureBuildingLink venue;venue.type=0x47;venue.alive=1;venue.entertainmentRoad=1;venue.entertainment=std::array<uint8_t,3>{0,20,0};
        context.buildings->emplace(10,venue);
        const auto finished=originalFigureFirstUpdate(school,refs,&context,0xc0);
        need(finished.state&&finished.state->alive==2&&finished.state->movementMode==1&&finished.state->hidden==1,
            "MusicSchool excludes venue music20 above both search thresholds before producing removal");
        context.buildings->at(10).entertainment->at(1)=16;
        need(!originalFigureFirstUpdate(school,refs,&context,0xc0).state,
            "qualifying venue music16 refuses positive weighted routing without publishing free-step writes");
        auto removed=*finished.state;context.figures[school.slot]=removed;
        need(originalFigureRemoveArrivedWorker(removed,context)&&!removed.alive&&*context.buildings->at(9).delegatedCount==0,
            "MusicSchool removal decrements its owner count and unlinks before resetting the figure");
        auto migrant=one;migrant.type=13;migrant.action=9;migrant.counter=30;migrant.route=3;migrant.direction=2;migrant.progress=2;
        context.modelSpeeds[13]=0;context.figures[migrant.slot]=migrant;
        const auto noVacancy=originalFigureFirstUpdate(migrant,refs,&context,0xc0);
        need(noVacancy.state&&noVacancy.state->counter==0&&noVacancy.state->action==9,
            "type13 post-movement counter31 resets after proving no eligible house vacancy");
        OriginalFigureBuildingLink house;house.type=3;house.alive=3;house.accepting=1;house.roadDistance=1;house.freeSlots=1;
        context.buildings->emplace(11,house);
        need(!originalFigureFirstUpdate(migrant,refs,&context,0xc0).state,
            "positive house vacancy refuses nearest-house and road selection rather than taking no-target branch");
        auto hunter=one;hunter.type=40;hunter.action=8;hunter.owner=9;hunter.counter=12;hunter.route=3;
        context.figures[hunter.slot]=hunter;context.services->routeOwners[3]=int16_t(hunter.slot);
        const auto returnHome=originalFigureFirstUpdate(hunter,refs,&context,0xc0);
        need(returnHome.state&&returnHome.state->action==10&&returnHome.state->counter==13&&returnHome.state->route==0&&
            returnHome.state->destinationX==5&&returnHome.state->destinationY==6&&returnHome.services->routeOwners[3]==0,
            "Hunter action8 counter13 targets owner road and releases route before action10 image without movement");
    }
    {
        // Literal Goods1/Population7 inputs, independent of runtime offsets.
        std::vector<uint8_t> data(20000);const size_t population=100,anchor=12387,figuresEnd=10;
        const auto put16=[&](size_t at,uint16_t n){data[at]=uint8_t(n);data[at+1]=uint8_t(n>>8);};
        const auto put32=[&](size_t at,uint32_t n){for(unsigned k=0;k<4;++k)data[at+k]=uint8_t(n>>(8*k));};
        put16(population,7);put16(figuresEnd,1);put32(figuresEnd+2,1);put16(figuresEnd+6,1);
        data[figuresEnd+6+12+25]=2;put16(figuresEnd+6+54+2*25,0xfffe);data[figuresEnd+6+198+25]=7;
        put32(population+10034+4*25,11);put32(population+10578+4*25,22);put32(population+10722+4*25,33);
        put32(population+10866+4*25,44);data.resize(1400000);put32(1299328,2);
        const auto parsed=originalFigureSavedGoodsServices(data,figuresEnd,anchor,0);
        need(parsed&&parsed->policies.size()==1&&parsed->policies[0].accept[25]==2&&parsed->policies[0].limit[25]==-2&&
            parsed->policies[0].trade[25]==7&&parsed->unavailable[25]==11&&parsed->trade7[25]==22&&parsed->trade5[25]==33&&
            parsed->warehouse[25]==44&&parsed->savedPhase==2&&!parsed->storageSearch&&!parsed->industrySearch&&!parsed->godGate,
            "Goods1 policy and all four Population7 arrays read independent literal wire offsets without process defaults");
        put16(figuresEnd+6,2);need(!originalFigureSavedGoodsServices(data,figuresEnd,anchor,0),"uncited Goods1 record version refuses all policies");
        put16(figuresEnd+6,1);put16(population,6);need(!originalFigureSavedGoodsServices(data,figuresEnd,anchor,0),"goods accounting requires Population7 framing");
    }
    {
        // Literal SAV19 terrain-relative addresses, distinct decoys either side.
        std::vector<uint8_t> services(17+1299348,0x5a);const size_t t=17;
        for(size_t n=0;n<100;++n){services[t+1281760+175*n]=6;services[t+1281761+175*n]=0;}
        services[t+779760+6]=0x34;services[t+779760+7]=0x12;
        services[t+781760+1507]=6;
        const auto put=[&](size_t at,uint32_t value){for(unsigned n=0;n<4;++n)services[t+at+n]=uint8_t(value>>(8*n));};
        put(1299340,1);put(1299344,16);
        auto saved=originalFigureSavedServices(services,t);
        need(saved&&saved->routeOwners[3]==0x1234&&saved->directions.size()==500000&&saved->directions[1507]==6&&
            saved->randomFirst==1&&saved->randomSecond==16,"SAV19 route rows500 and two RNG registers use literal independent offsets");
        //31 shifts:single bit0 returns bit0+27;single bit4 adds bit4.
        need(originalFigureRandomNext(*saved)==1&&saved->randomFirst==0x08000001&&saved->randomSecond==0x08000011,
            "RNG performs31 bit4-xor-bit0 shifts on both saved registers");
        need(!originalFigureSavedServices(services,t+1),"truncated second RNG register withholds all saved services");
        saved->randomFirst=saved->randomSecond=0;need(originalFigureRandomNext(*saved)==0&&saved->randomFirst==0&&saved->randomSecond==0,"zero saved RNG is retained rather than reseeded");
        auto walker=one;walker.route=3;walker.routeCursor=7;walker.routeLength=8;
        auto direction=originalFigureSavedRouteDirection(walker,*saved);
        need(direction&&direction->state.direction==6&&direction->state.routeCursor==7,"saved route direction uses row500 plus cursor without advancing cursor");
        walker.routeCursor=8;saved->routeOwners[3]=int16_t(one.slot);
        const auto ended=originalFigureSavedRouteDirection(walker,*saved);
        need(ended&&ended->state.direction==8&&ended->state.route==0&&ended->services.routeOwners[3]==0&&saved->routeOwners[3]==int(one.slot),
            "route exhaustion releases only reciprocal owner and publishes atomic service copy");
        saved->routeOwners[3]=99;need(originalFigureSavedRouteDirection(walker,*saved)->services.routeOwners[3]==99,"route release retains another figure's owner");
        services[t+1281760+175*99]=5;need(!originalFigureSavedServices(services,t),"unknown intervening record version refuses RNG framing");
        services[t+1281760+175*99]=6;
        need(!originalFigureSavedServices(services,t)->fireCount,"absent later fire extent remains unknown rather than defaulting to zero");
        services.resize(t+1363854,0xa5);put(1363846,0x12345678);put(1363850,0xfffffffe);
        need(original_save19::fireCount==1363850&&original_save19::fireBlocks80==1363690&&
            originalFigureSavedServices(services,t)->fireCount==-2,
            "SAV19 read order after shared terrain scalars reaches signed fire count at literal1363850 after two80-byte blocks");
        services.pop_back();need(!originalFigureSavedServices(services,t)->fireCount,"partial saved fire dword cannot publish zero or a truncated count");
        services.resize(t+1391646,0);put(1391638,123);put(1391642,0xfffffffd);
        need(original_save19::criminalCount==1391642&&originalFigureSavedServices(services,t)->criminalCount==-3,
            "Watchtower criminal count follows independent SAV19 request cursor to literal1391642");
        services.pop_back();need(!originalFigureSavedServices(services,t)->criminalCount,
            "truncated criminal count remains unknown rather than enabling Watchtower search early return");
        services.resize(t+1770973,0);put(1563037,0x12345678);put(1770969,0x87654321);
        const auto grid=originalFigureSavedServices(services,t);
        need(grid&&grid->serviceGrid.size()==51984&&grid->serviceGrid.front()==0x12345678&&grid->serviceGrid.back()==0x87654321,
            "service grid reads literal T+1563037 through1770973 with unaligned u32 endpoints");
        services.pop_back();need(originalFigureSavedServices(services,t)->serviceGrid.empty(),"truncated service grid publishes no partial cells");
    }
    {
        std::vector<uint8_t> data(1600);const size_t scenario=11,anchor=scenario+28;
        const auto put16=[&](size_t at,uint16_t value){data[at]=uint8_t(value);data[at+1]=uint8_t(value>>8);};
        const auto put32=[&](size_t at,uint32_t value){for(unsigned n=0;n<4;++n)data[at+n]=uint8_t(value>>(8*n));};
        put16(scenario,6);put32(anchor,20);put32(anchor+4,20);put32(anchor+8,104*229);
        for(unsigned n=0;n<4;++n){put16(scenario+853+4*n,uint16_t(2+n));put16(scenario+869+4*n,uint16_t(7+n));}
        const auto spawn=originalFigureSpawnServices(data,anchor);
        need(spawn&&spawn->spawns[2]==std::make_pair(int16_t(4),int16_t(9))&&spawn->cropBase==104*229,
            "scenario6 four dword-spaced signed spawn coordinates use individual wire offsets");
        put16(scenario,5);need(!originalFigureSpawnServices(data,anchor),"uncited scenario version cannot supply spawn defaults");put16(scenario,6);
        OriginalMapTerrain terrain;terrain.activeSide=20;terrain.flags.resize(51984);
        OriginalBuildingProfile buildings;buildings.status=OriginalBuildingStatus::Complete;buildings.parsedRecords=4000;
        std::vector<uint8_t> roads(51984,255);const auto base=spawn->cropBase;
        terrain.flags[base]=0x40;terrain.flags[base+1]=0x100;terrain.flags[base+2]=0x800200;terrain.flags[base+3]=0x400200;
        terrain.flags[base+4]=0x440;terrain.flags[base+5]=0x20000;roads[base+5]=0;
        const auto grid=originalFigurePheasantPassability(terrain,roads,buildings,*spawn);
        need(grid&&grid->cells[base]==4&&grid->cells[base+1]==0&&grid->cells[base+2]==0x400&&grid->cells[base+3]==0&&
            grid->cells[base+4]==0x100&&grid->cells[base+5]==0,"passability respects terrain branch precedence before mask51C");
        terrain.flags[base]=0x10000;need(!originalFigurePheasantPassability(terrain,roads,buildings,*spawn),"river object rebuild cannot be replaced with terrain-only passability");
        terrain.flags[base]=0;buildings.slotTypes[1]=0x21;need(!originalFigurePheasantPassability(terrain,roads,buildings,*spawn),"building virtual passability is withheld for any occupied pool");
        OriginalFigurePassability map;map.spawn=*spawn;map.cells.resize(51984);map.cells[base+1]=0x10;map.cells[base+229]=0x10;map.cells[base+230]=1;
        const auto seen=originalFigureReachableCells(map,0,0);
        need(seen&&(*seen)[base]&&(*seen)[base+1]&&(*seen)[base+229]&&!(*seen)[base+230],"cardinal flood seeds blocked start, reaches via east then south, rejects mask1");
        map.cells[base+1]=0;need(!originalFigureReachableCells(map,0,0)->at(base+229),"cardinal flood cannot cross a diagonal-only connection");
        OriginalTerrainState rebuilt;rebuilt.terrain=terrain.flags;rebuilt.image.resize(51984);rebuilt.draw.resize(51984);rebuilt.subTile.resize(51984);
        rebuilt.randomByte.resize(51984);rebuilt.feature.resize(51984);rebuilt.fertility.resize(51984);rebuilt.roadDistance=roads;
        rebuilt.gF05160.resize(51984);rebuilt.gEAC3F0.resize(51984);rebuilt.gBEBF3A.resize(51984);rebuilt.gBF8A4A.resize(51984);
        std::vector<uint16_t> owners(51984);buildings.slotTypes[1]=0xc1;owners[base+6]=1;rebuilt.terrain[base+6]=8;
        const auto full=originalFigureMovementPassability(rebuilt,buildings,owners,*spawn);
        need(full&&full->fullMovementMasks&&full->cells[base]==0x10&&full->cells[base+1]==2&&full->cells[base+2]==0x400&&
            full->cells[base+3]==2&&full->cells[base+4]==0x100&&full->cells[base+5]==0x80&&full->cells[base+6]==2,
            "full movement words preserve blocked, elevation, transport and forest bits from rebuilt public layers");
        rebuilt.terrain[base]=0x10000;need(!originalFigureMovementPassability(rebuilt,buildings,owners,*spawn),
            "full masks refuse missing river object state instead of using saved image words");rebuilt.terrain[base]=0;
        buildings.slotTypes[1]=0x3b;need(!originalFigureMovementPassability(rebuilt,buildings,owners,*spawn),
            "uncited market pass-through virtual cannot become generic building passability");
        map.fullMovementMasks=true;map.cells.assign(51984,0);for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x)map.cells[base+y*228+x]=0x10;
        const auto route=originalFigureEngineRoute(map,0,0,2,2);
        need(route&&*route==std::vector<uint8_t>({3,3}),"engine backtrack compresses cardinal distances to original eight-direction route");
        map.cells[base+229]=0;const auto detour=originalFigureEngineRoute(map,0,0,2,2);
        need(detour&&*detour==std::vector<uint8_t>({2,3,4}),"engine route follows distance tie ordering around blocked diagonal");
        auto tied=map;tied.cells.assign(51984,0);
        const char* openRows[]={"11101111","11111011","00001111","01111111","11110101","01111010","11111111","11101111"};
        for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)if(openRows[y][x]=='1')tied.cells[base+y*228+x]=0x10;
        const auto tiedRoute=originalFigureEngineRoute(tied,0,0,7,7);
        need(tiedRoute&&*tiedRoute==std::vector<uint8_t>({2,2,3,3,4,5,3,3,2,3}),
            "engine equal-distance backtrack prefers direction toward source before reversing route bytes");
        map.fullMovementMasks=false;need(!originalFigureEngineRoute(map,0,0,2,2),"reduced51C projection cannot authorize route generation");
        OriginalFigureUpdateContext routeContext;routeContext.passability=map;routeContext.passability->fullMovementMasks=true;
        routeContext.services.emplace();routeContext.services->directions.assign(500000,255);routeContext.terrain.assign(51984,0);
        routeContext.movementMasks=routeContext.passability->cells;
        auto routed=one;routed.type=76;routed.owner=0;routed.movementMode=5;routed.effectDraw=0;routed.x=routed.y=0;
        routed.destinationX=routed.destinationY=2;routed.cell=base;routed.route=0;routeContext.figures.emplace(routed.slot,routed);
        need(originalFigureGenerateRoute(routed,routeContext)&&routed.route==1&&routed.routeLength==3&&routed.routeCursor==0&&routed.progress==20&&
            routed.direction==2&&routeContext.services->routeOwners[1]==int(routed.slot)&&routeContext.services->directions[500]==2,
            "route allocation owns first free buffer and writes exact path without saved route fitting");
        need(originalFigureValidateNextDirection(routed,routeContext)&&routed.direction==2,"mode5 accepts flat land through full movement validation");
        routeContext.movementMasks[base+1]=0x800;need(originalFigureValidateNextDirection(routed,routeContext)&&routed.direction==2,
            "transport mask800 reaches original final terrain100 gate without changing direction");
        routeContext.terrain[base+1]=0x100;need(originalFigureValidateNextDirection(routed,routeContext)&&routed.direction==9,
            "final terrain100 rejection survives the transport800 early branch");
        routeContext.terrain[base+1]=8;routeContext.movementMasks[base+1]=0x10;routeContext.buildingOwners=owners;
        routeContext.buildingOwners[base+1]=7;routeContext.buildings.emplace();routeContext.buildings->emplace(7,OriginalFigureBuildingLink{});
        routeContext.buildings->at(7).type=0xc4;routed.direction=2;
        need(originalFigureValidateNextDirection(routed,routeContext)&&routed.direction==2,
            "mode3 hunter can follow engine route into a crop field through4C11B0");
        routeContext.buildings->at(7).type=0x3b;
        need(!originalFigureValidateNextDirection(routed,routeContext),"market virtual pass-through needs its cited specialized predicate");
        routeContext.buildings->at(7).type=0xc1;
        need(originalFigureValidateNextDirection(routed,routeContext)&&routed.direction==9,
            "ordinary building with false pass-through rejects movement after its owner lookup");
        routed.direction=2;routeContext.terrain[base+1]=0xc8;routeContext.movementMasks[base+1]=4;
        routeContext.buildings->at(7).type=0x7e;routeContext.roadblockGate.reset();
        need(!originalFigureValidateNextDirection(routed,routeContext),"roadblock cannot infer unknown1032664 process gate");
        routeContext.roadblockGate=0;
        need(originalFigureValidateNextDirection(routed,routeContext)&&routed.direction==2,"known zero1032664 bypasses roadblock owner predicate");
        routeContext.roadblockGate=1;
        need(!originalFigureValidateNextDirection(routed,routeContext),"positive roadblock process gate needs its separate owner branch");
        auto cropRouteContext=routeContext;cropRouteContext.passability->cells.assign(51984,0);
        for(unsigned x=0;x<3;++x)cropRouteContext.passability->cells[base+x]=0x10;
        cropRouteContext.terrain.assign(51984,0);cropRouteContext.terrain[base+1]=0x10000000;
        cropRouteContext.buildingOwners[base+1]=7;cropRouteContext.services->routeOwners.fill(0);
        auto cropWalker=routed;cropWalker.type=26;cropWalker.movementMode=17;cropWalker.destinationX=2;cropWalker.destinationY=0;cropWalker.route=0;
        need(originalFigureGenerateRoute(cropWalker,cropRouteContext)&&cropWalker.route==0&&cropWalker.direction==10,
            "mode17 flood rejects a non-crop owner on terrain10000000 even when movement mask13D permits it");
        cropRouteContext.buildings->at(7).type=0xc4;
        need(originalFigureGenerateRoute(cropWalker,cropRouteContext)&&cropWalker.route==1&&cropWalker.routeLength==2&&
            cropRouteContext.services->directions[500]==2&&cropRouteContext.services->directions[501]==2,
            "mode17 flood crosses cited Millet owner and allocates the exact two east steps");
        auto group=archive;group.numImages=256;group.sprites.resize(256);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=12;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> refs{{38,&group}};
        auto bird=one;bird.type=76;bird.runtimeClass=OriginalFigureClass::Pheasant;bird.action=19;bird.herdTarget=bird.slot;
        bird.route=3;bird.frame=0;bird.direction=2;bird.counter=0;
        OriginalFigureUpdateContext context;context.figures[bird.slot]=bird;context.services.emplace();context.services->randomFirst=1;
        context.services->routeOwners[3]=int16_t(bird.slot);context.passability=*grid;
        context.passability->cells.assign(51984,0);for(unsigned y=0;y<20;++y)for(unsigned x=0;x<20;++x)context.passability->cells[base+y*228+x]=0x10;
        context.passability->spawn.spawns={{{2,2},{2,0},{19,19},{-1,-1}}};
        const auto destination=originalFigureFirstUpdate(bird,refs,&context);
        need(destination.state&&destination.services&&destination.state->action==32&&destination.state->destinationX==11&&destination.state->destinationY==11&&
            destination.state->destinationCell==base+11*229&&destination.state->route==0&&destination.state->progress==bird.progress&&
            destination.state->frame==1&&destination.services->randomFirst==0x800001&&destination.services->routeOwners[3]==0&&context.services->randomFirst==1,
            "Pheasant destination uses first equidistant spawn, two RNG draws, releases route then advances frame without movement");
        auto choosing=bird;choosing.action=30;
        const auto repeatedDestination=originalFigureFirstUpdate(choosing,refs,&context);
        need(repeatedDestination.state&&repeatedDestination.state->action==32&&repeatedDestination.state->destinationX==11&&
             repeatedDestination.services->randomFirst==0x800001,"action30 runs the same ordered destination predicate as initial19");
        choosing.action=18;choosing.counter=100;choosing.idleCounter=47;
        const auto startled=originalFigureFirstUpdate(choosing,refs,&context);
        need(startled.state&&startled.state->action==31&&startled.state->counter==100&&startled.state->idleCounter==47,
            "Pheasant action18 returns to31 without incrementing its idle counter or moving in that call");
        auto distant=bird;distant.slot=9;distant.action=30;distant.x=distant.y=0;distant.herdTarget=0;
        auto remote=distant;remote.slot=10;remote.x=20;remote.route=0;
        auto herdContext=context;herdContext.cropBase=base;herdContext.figures={{1,OriginalFigure{}},{9,distant},{10,remote}};
        herdContext.passability->spawn.spawns.fill({19,19});herdContext.services->routeOwners[3]=9;
        const auto herdDestination=originalFigureFirstUpdate(distant,refs,&herdContext);
        need(herdDestination.state&&herdDestination.state->herdTarget==1&&herdDestination.state->herdDistance==0&&
            herdDestination.state->destinationX==10&&herdDestination.state->destinationY==10&&herdDestination.state->action==32&&
            herdDestination.services->randomFirst==0x800001&&herdDestination.services->routeOwners[3]==0,
            "extended herd search rereads ordinal1 yet retains minus1 destination origin and consumes exactly two persisted random draws");
        choosing.action=32;choosing.routeCursor=choosing.routeLength=1;choosing.progress=19;choosing.route=3;
        auto arrivalContext=context;arrivalContext.services->directions.assign(500000,0);arrivalContext.modelSpeeds[76]=11;
        const auto arrival=originalFigureFirstUpdate(choosing,refs,&arrivalContext);
        need(arrival.state&&arrival.state->action==31&&arrival.state->idleCounter==0&&arrival.state->direction==8&&arrival.state->route==0,
            "Pheasant route exhaustion runs59F1C0, clears idle counter and selects action31 frame");
        context.passability->cells[base+11*229]=0;
        const auto retry=originalFigureFirstUpdate(bird,refs,&context);
        need(retry.state&&retry.services->randomFirst==0x44000001&&retry.state->destinationX==5&&retry.state->destinationY==11,
            "destination retries from fixed spawn and consumes RNG for every rejected candidate");
        const auto missingGroup=originalFigureFirstUpdate(bird,{},&context);
        need(!missingGroup.state&&!missingGroup.services,"failed final image resolution withholds tentative RNG and route mutations");
        OriginalFigureProfile pool;pool.status=OriginalFigureStatus::Complete;pool.declared=pool.parsed=2000;
        pool.slots.resize(2000);for(size_t n=0;n<2000;++n)pool.slots[n].slot=n;
        auto first=bird;first.slot=1;first.herdTarget=1;first.route=0;auto second=first;second.slot=2;second.herdTarget=2;
        pool.slots[1]=first;pool.slots[2]=second;pool.live={first,second};context.services->randomFirst=0x12345678;
        context.figures={{1,first},{2,second}};
        context.passability->cells.assign(51984,0);for(unsigned y=0;y<20;++y)for(unsigned x=0;x<20;++x)context.passability->cells[base+y*228+x]=0x10;
        const auto firstExpected=originalFigureFirstUpdate(first,refs,&context);auto nextContext=context;
        nextContext.services=firstExpected.services;nextContext.figures[1]=*firstExpected.state;
        const auto secondExpected=originalFigureFirstUpdate(second,refs,&nextContext);
        const auto ordered=originalFigureOrderedFirstPass(pool,refs,std::vector<uint32_t>(51984),{},&*context.services,&*context.passability);
        need(ordered.complete&&ordered.context&&ordered.context->services->randomFirst==secondExpected.services->randomFirst&&
            ordered.figures.at(1).state->destinationX==1&&ordered.figures.at(1).state->destinationY==2&&
            ordered.figures.at(2).state->destinationX==secondExpected.state->destinationX&&
            ordered.figures.at(2).state->destinationY==secondExpected.state->destinationY&&
            secondExpected.services->randomFirst!=firstExpected.services->randomFirst,
            "ordered destination figures consume prior slot RNG state and publish final service state");
        context.figures[bird.slot]=bird;
        context.passability->cells.assign(51984,0);context.passability->spawn.spawns.fill({0,0});
        context.services->randomFirst=0;context.services->randomSecond=16;
        const auto fallback=originalFigureFirstUpdate(bird,refs,&context);
        auto expected=*context.services;for(unsigned n=0;n<2000;++n)originalFigureRandomNext(expected);
        need(fallback.state&&fallback.state->destinationX==0&&fallback.state->destinationY==0&&fallback.services->randomFirst==expected.randomFirst&&
            fallback.services->randomSecond==expected.randomSecond,
            "attempt1000 uses reachable spawn after exactly2000 RNG calls");
        context.hunterLinks=std::map<size_t,OriginalFigureHunterLink>{{8,{3,1,2}}};bird.linkedBuilding=8;bird.action=31;
        need(originalFigureFirstUpdate(bird,refs,&context).state->linkedBuilding==8,"Hunter live3 enabled positive workers retains linked building");
        context.hunterLinks->at(8).workers=0;need(originalFigureFirstUpdate(bird,refs,&context).state->linkedBuilding==0,"Hunter zero worker link clears before action update");
    }
    {
        auto group=archive;group.numImages=256;group.sprites.resize(256);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=20;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> supplied{{14,&group},{38,&group}};
        OriginalFigureProfile profile;profile.status=OriginalFigureStatus::Complete;profile.declared=profile.parsed=2000;
        profile.slots.resize(2000);for(size_t n=0;n<2000;++n)profile.slots[n].slot=n;
        auto bird=one;bird.slot=1;bird.type=76;bird.runtimeClass=OriginalFigureClass::Pheasant;bird.action=31;bird.herdTarget=1;
        bird.age=UINT32_MAX;bird.progress=20;bird.reciprocalTarget=2;bird.leaderLink=2;
        auto other=bird;other.slot=2;other.herdTarget=2;other.x=40;other.age=5;other.target=0;other.reciprocalTarget=0;other.leaderLink=1;
        profile.slots[1]=bird;profile.slots[2]=other;profile.live={bird,other};
        const std::vector<uint32_t> flags(51984);
        const auto ordered=originalFigureOrderedFirstPass(profile,supplied,flags);
        need(ordered.accepted&&ordered.complete&&ordered.computed==2&&ordered.blocked==0&&ordered.figures.at(1).state->age==0&&
            ordered.figures.at(1).state->reciprocalTarget==0&&ordered.figures.at(1).state->priorProgress==20&&
            ordered.figures.at(1).state->follower==2&&ordered.figures.at(2).state->follower==1&&ordered.figures.at(2).state->age==6,
             "ordered pass wraps age, validates reciprocal link and retains later follower writes to earlier slots");
        const auto repeated=originalFigureOrderedNextPass(*ordered.context,supplied);
        need(repeated.complete&&repeated.context->figures.at(1).age==1&&repeated.context->figures.at(2).age==7&&
            repeated.context->figures.at(1).frame==uint8_t(ordered.context->figures.at(1).frame+1),
            "next ordered pass retains prior state and advances age/frame without replaying load callbacks");
        auto drawContext=*ordered.context;drawContext.cellHeads.assign(51984,0);
        drawContext.figures.at(1).cell=drawContext.figures.at(2).cell=one.cell;
        drawContext.figures.at(1).progress=20;drawContext.figures.at(2).progress=0;drawContext.figures.at(2).direction=0;
        drawContext.cellHeads[one.cell]=1;drawContext.figures.at(1).next=2;
        need(originalFigureSortDrawCell(drawContext,one.cell)&&drawContext.cellHeads[one.cell]==2&&drawContext.figures.at(2).next==1&&
            drawContext.figures.at(1).next==0,"draw sorts and persists cell heads and next links before a later ordered update");
        OriginalBuildingScene empty;empty.accepted=true;empty.surface.terrain=flags;OriginalSceneTimelineFrame frame;
        frame.animationClock=37;frame.admission.now=37;frame.admission.speed=80;frame.continueAfterPass={false};
        const auto timeline=originalSceneTimeline(*ordered.context,supplied,empty,group,nullptr,0,{}, {frame});
        need(timeline.complete&&timeline.updates==1&&timeline.context->figures.at(1).age==1,
            "explicit timeline uses admitted37ms update and the current pool rather than saved inputs");
        frame.admission.now=36;const auto denied=originalSceneTimeline(*ordered.context,supplied,empty,group,nullptr,0,{}, {frame});
        need(denied.complete&&!denied.updates&&denied.context->figures.at(1).age==0,
            "timeline draw at36ms does not admit speed80 update");
        frame.admission.now=37;frame.continueAfterPass.clear();
        need(!originalSceneTimeline(*ordered.context,supplied,empty,group,nullptr,0,{}, {frame}).complete,
            "timeline withholds outputs when the admitted loop observation is absent");
        // Two updates in one fast-forward frame still make only one building
        // draw; two separate frames can make two draws with the same speed flag.
        auto animated=empty;auto footprint=OriginalBuildingFootprint{};footprint.slot=8;footprint.type=0x7c;footprint.size=1;
        footprint.animationInputs=OriginalBuildingAnimationInputs{1,0,0,uint8_t(1),uint8_t(0),uint16_t(8)};
        OriginalBuildingPlacement placement;placement.footprint=footprint;placement.binding.physicalId=1;placement.components.resize(1);
        animated.placed.push_back(placement);group.sprites[0].animationSpeedId=0;
        OriginalBuildingEmployment employment{{0x7c,0}};frame.continueAfterPass={true,false};frame.buildingDraws={8};
        const auto fast=originalSceneTimeline(*ordered.context,supplied,animated,group,&employment,0,{}, {frame});
        need(fast.complete&&fast.updates==2&&fast.buildingPhases.at(8)==2&&fast.buildingDraws==1&&
            fast.context->figures.at(1).age==2,"two admitted fast-forward passes advance figures twice but speed0 building once per observed draw");
        frame.continueAfterPass={false};auto later=frame;later.animationClock=later.admission.now=74;
        const auto regular=originalSceneTimeline(*ordered.context,supplied,animated,group,&employment,0,{}, {frame,later});
        need(regular.complete&&regular.updates==2&&regular.buildingPhases.at(8)==3&&regular.buildingDraws==2,
            "same two updates with two observed draws produce a distinct building phase on one explicit timeline");
        profile.slots[1].leaderLink=1;profile.slots[2].leaderLink=0;
        need(originalFigureOrderedFirstPass(profile,supplied,flags).figures.at(1).state->follower==1,
            "self leader link writes follower on the same record before virtual dispatch");
        profile.slots[1]=bird;profile.slots[2]=other;
        profile.slots[1].type=11;profile.slots[1].runtimeClass=OriginalFigureClass::Figure;profile.live[0]=profile.slots[1];
        const auto blockedPass=originalFigureOrderedFirstPass(profile,supplied,flags);
        need(blockedPass.accepted&&!blockedPass.complete&&blockedPass.computed==0&&blockedPass.blocked==2&&blockedPass.firstBlockedSlot==1&&
            blockedPass.figures.at(1).status==OriginalFigureUpdateStatus::WorkerOwnerMovement&&
            blockedPass.figures.at(2).status==OriginalFigureUpdateStatus::EarlierSlot&&!blockedPass.figures.at(2).state,
            "unsupported earlier slot prevents later Pheasant saved-context output from masquerading as ordered coverage");
        profile.slots[1]=bird;profile.slots[2].type=11;profile.slots[2].runtimeClass=OriginalFigureClass::Figure;
        profile.live={bird,profile.slots[2]};const auto laterBlocked=originalFigureOrderedFirstPass(profile,supplied,flags);
        need(laterBlocked.prefixComputed==1&&laterBlocked.computed==0&&laterBlocked.blocked==2&&!laterBlocked.context&&
            laterBlocked.figures.at(1).status==OriginalFigureUpdateStatus::LaterSlotEffects&&!laterBlocked.figures.at(1).state,
            "unknown later updater withholds earlier prefix as final pass state because linked writes may return to it");
    }
    {
        OriginalAnimationClock clock;clock=originalAnimationClockFrame(clock,79);
        need(clock.flags[0]&&clock.flags[3]&&!clock.flags[4]&&clock.last[4]==0,"shared clock79 fires speed3 but not speed4");
        clock=originalAnimationClockFrame(clock,80);
        need(clock.flags[4]&&!clock.flags[3]&&clock.last[4]==80,"shared clock equality fires without catching up other slots");
        clock.last.fill(0x7ffffff0);clock=originalAnimationClockFrame(clock,0x7fffffff);
        need(clock.flags[1]&&clock.last[1]==0x7fffffff,"animation signed comparison follows wrapped last plus20");
        SG3Sprite sprite;sprite.animationSpriteCount=3;sprite.animationSpeedId=0;sprite.animationReversible=1;
        std::array<bool,51> flags{};flags[0]=true;uint8_t phase=0;
        for(uint8_t expected:{1,2,3,131,130,129,1}){phase=*originalBuildingAnimationDraw(sprite,phase,flags);
            need(phase==expected,"explicit building draw repeats each reversible endpoint including speed0");}
        flags[0]=false;need(originalBuildingAnimationDraw(sprite,phase,flags)==phase,"ineligible draw preserves building phase");
        sprite.animationReversible=0;sprite.animationSpeedId=5;phase=0;OriginalAnimationClock irregular;
        for(uint32_t now:{99u,100u,999u}){irregular=originalAnimationClockFrame(irregular,now);
            phase=*originalBuildingAnimationDraw(sprite,phase,irregular.flags);}
        need(phase==2&&irregular.last[5]==999,"long gap advances once per draw without elapsed-time catch-up");
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
    {
        auto effects=figures;auto& effect=effects.live[0];effect.type=1;effect.image=14u<<14;effect.effectKind=13;effect.effectFlags=0x24;
        effect.freeMovement=1;effect.subX=7;effect.subY=2;effect.raise=31;effect.height=4;effect.extraX=-6;effect.extraY=9;
        effects.cellHeads[one.cell]=0;effects.cellHeads[one.cell+229]=effect.slot;
        auto composed=composeOriginalBuildingScene(terrain,buildings,archive);
        const std::map<uint32_t,const SG3Archive*> ambient{{14,&archive}};
        installOriginalFigureOverlays(composed,terrain,effects,ambient);
        need(composed.figurePlaced==1&&composed.overlays[0].anchorX==1&&composed.overlays[0].anchorY==1&&
            composed.overlays[0].offsetX==35&&composed.overlays[0].offsetY==-32,
            "kind13 follows the view0 diagonal draw-cell list and exact selected sprite offsets");
        const std::vector<MapImageGridTileset> resources{{14,&archive,file.string(),1}};
        auto view=renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,resources);
        need(view.figureDecoded==1&&view.overlayDrawn==1&&view.image.pixelsBGRA[88*200+96]==0xffff0000u&&
            view.image.pixelsBGRA[88*200+95]!=0xffff0000u,"kind13 paints exact pixel anchor after diagonal scan displacement");
        camera.width=100;camera.height=100;camera.centerX=1.6;camera.centerY=0;
        view=renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,resources);
        need(view.overlayDrawn==1&&view.figureDecoded==1&&view.image.pixelsBGRA[20*100+11]==0xffff0000u&&
            view.image.pixelsBGRA[20*100+12]!=0xffff0000u,"kind13 partially clipped sprite keeps twelve visible source columns");
        camera.width=200;camera.height=200;camera.centerX=camera.centerY=0;
        effects.live[0].image=0;installOriginalFigureOverlays(composed,terrain,effects,ambient);
        need(composed.figurePlaced==0&&composed.figureZeroImage==1,"kind13 saved image zero never selects a future frame for playback");
    }
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
    {
        auto group=archive;group.numImages=512;group.sprites.resize(512);group.imageIndex.fill(1);
        group.sprites[0].animationSpriteCount=12;group.sprites[0].animationDirections=8;
        const std::map<uint32_t,const SG3Archive*> refs{{38,&group}};
        auto worker=one;worker.slot=2;worker.home=8;worker.action=8;worker.frame=11;worker.employmentAmount=4;
        worker.x=5;worker.y=6;worker.cell=10000;worker.subX=100;worker.subY=120;
        worker.freeRemainingX=worker.freeRemainingY=0;worker.raise=1;
        OriginalFigureUpdateContext context;context.figures[2]=worker;context.cropBase=8627;
        context.cellHeads.assign(51984,0);context.cellHeads[10000]=2;context.modelSpeeds[11]=8;
        context.buildings.emplace();auto& house=(*context.buildings)[8];house.type=7;house.alive=1;house.accepting=1;house.worker=2;
        house.residents=31;house.freeSlots=10;house.houseModel=4;house.laborDisabled=0;context.houseCapacities[4]=41;
        context.services.emplace();context.services->population=OriginalFigurePopulation{993,997,0};
        const auto arrived=originalFigureFirstUpdate(worker,refs,&context);
        need(arrived.state&&arrived.state->alive==2&&arrived.state->frame==0&&arrived.state->raise==0&&
            arrived.buildings->at(8).residents==35&&arrived.buildings->at(8).freeSlots==6&&arrived.buildings->at(8).worker==0&&
            arrived.services->population->total==997&&arrived.services->population->peak==997&&arrived.services->population->lastArrival==4,
            "occupied worker arrival adds four residents, subtracts capacity after the store, clears reciprocal link and accounts once");
        need(context.buildings->at(8).residents==31&&context.services->population->total==993,
            "worker arrival publishes building and accounting writes atomically without changing the source");
        house.laborDisabled=1;const auto disabled=originalFigureFirstUpdate(worker,refs,&context);
        need(disabled.state&&disabled.buildings->at(8).residents==31&&disabled.buildings->at(8).worker==0&&disabled.services->population->total==993,
            "labor-disabled house clears arriving worker link but bypasses residents and accounting writes");
        house.laborDisabled=0;context.houseCapacities.clear();const auto absent=originalFigureFirstUpdate(worker,refs,&context);
        need(!absent.state&&!absent.services&&!absent.buildings,"unknown capacity withholds tentative free-step and employment writes");
        context.houseCapacities[4]=41;house.residents=0;
        need(!originalFigureFirstUpdate(worker,refs,&context).state,"empty house notification virtual remains an explicit refusal");
        house.residents=31;house.entertainment=std::array<uint8_t,3>{};
        auto loaded=*context.services;loaded.population->total=7;
        need(originalFigurePopulationAfterLoad(loaded,*context.buildings)&&loaded.population->total==31&&loaded.population->peak==997,
            "load recount replaces serialized total from accepting houses while preserving historical peak");
        auto removed=*arrived.state;removed.priorProgress=20;context.figures[2]=removed;context.highestLiveSlot=3;
        auto observer=one;observer.slot=3;observer.cell=10001;observer.target=2;context.figures[3]=observer;removed.flags120=2;
        need(originalFigureRemoveArrivedWorker(removed,context)&&removed.alive==0&&removed.type==0&&removed.image==0&&removed.age==0&&
            removed.priorProgress==20&&context.cellHeads[10000]==0&&context.figures.at(3).target==0&&context.freeSlots==std::vector<size_t>{2},
            "dispatcher removal unlinks arrived worker, clears target references, resets fields and appends free slot");
        auto entertainer=one;entertainer.type=36;entertainer.owner=9;entertainer.x=5;entertainer.y=6;entertainer.cell=10000;
        entertainer.direction=2;entertainer.previousDirection=2;entertainer.progress=19;entertainer.roamState=1;entertainer.roamTurn=2;entertainer.roamCountdown=5;
        entertainer.subX=100;entertainer.subY=120;entertainer.roamSteps=12;entertainer.routeCursor=8;
        context.terrain.assign(51984,0x80);context.terrain[9999]=context.terrain[10228]=0xc0;
        context.feature.assign(51984,0);context.buildingOwners.assign(51984,0);context.activeSide=84;context.festivalVenue=0;
        context.services->serviceGrid.assign(51984,0x80000001);context.figures[2]=entertainer;
        auto& owner=(*context.buildings)[9];owner.type=0x47;owner.alive=1;owner.serviceCount=298;owner.entertainment=std::array<uint8_t,3>{0,7,0};
        context.buildingOwners[9999]=8;context.buildingOwners[9998]=8;
        const auto serviced=originalFigureRoam(entertainer,6,0x80,context);
        need(serviced.state&&serviced.state->direction==4&&serviced.state->progress==0&&serviced.state->subY==121&&
            serviced.state->routeCursor==9&&serviced.state->roamTurns==1&&serviced.state->roamSteps==18&&
            context.buildings->at(8).entertainment==std::array<uint8_t,3>{0,96,0}&&context.buildings->at(9).serviceCount==300&&
            context.services->serviceGrid[10000]==0x80000e01,
            "entertainment segment traverses repeated house cells, caps count, marks category3 preserving other bits, then chooses south road");
        context.feature[9999]=4;context.buildings->at(8).type=0xe7;context.buildings->at(9).serviceCount=17;
        const auto refused=originalFigureRoam(entertainer,6,0x80,context);
        need(refused.state&&context.buildings->at(9).serviceCount==17,"interior wall occlusion skips callbacks without inventing service writes");
        need(originalFigureServiceSector(0,0)==-1&&originalFigureServiceSector(-2,-2)==0&&originalFigureServiceSector(0,-2)==14&&
            originalFigureServiceSector(2,0)==10&&originalFigureServiceSector(0,2)==6&&originalFigureServiceSector(-2,0)==2,
            "service sectors retain executable first-match angular order and the center's minus1 callback path");
        {auto shadow=context;shadow.buildingOwners.assign(51984,0);shadow.feature.assign(51984,0);
            shadow.buildingOwners[10001]=10;shadow.buildingOwners[10002]=8;shadow.feature[10001]=4;
            (*shadow.buildings)[10].type=0xe7;shadow.buildings->at(8).watchRisk=83;shadow.watchMaximum=0;
            shadow.buildings->at(8).type=3;shadow.buildings->at(8).houseModel=2;
            shadow.watchtowerReduction=11;shadow.houseWatchFloors={{2,5}};
            auto covered=entertainer;covered.type=29;shadow.buildings->at(9).type=0x7f;shadow.buildings->at(9).serviceCount=17;
            const auto screened=originalFigureRoam(covered,6,0x80,shadow);
            need(screened.state&&shadow.buildings->at(8).watchRisk==83&&shadow.buildings->at(9).serviceCount==17,
                "radius1 residential wall shadows a distinct house two cells behind it across the repeated radius2 box");}
        context.feature[9999]=0;context.buildings->at(8).type=3;context.buildings->at(8).houseModel=2;context.buildings->at(8).watchRisk=83;
        context.houseWatchFloors={{2,5}};context.watchtowerReduction=11;context.watchMaximum=0;context.buildings->at(9).type=0x7f;
        auto watch=entertainer;watch.type=29;watch.watchCoverage=7;
        const auto guarded=originalFigureRoam(watch,6,0x80,context);
        need(guarded.state&&guarded.state->watchCoverage==83&&context.watchMaximum==83&&context.buildings->at(8).watchRisk==50&&
            context.services->serviceGrid[10000]==0x80000fc1,
            "watchtower repeated house callbacks reduce risk three times, retain prior maximum and mark category2 separately");
        watch.watchCoverage=90;const auto guardedAgain=originalFigureRoam(watch,6,0x80,context);
        need(guardedAgain.state&&guardedAgain.state->watchCoverage==80&&context.buildings->at(8).watchRisk==17&&context.watchMaximum==83,
            "watchtower maximum persists across calls while coverage decays by ten and risk repeats per cell");
        context.buildings->at(9).type=0xcf;context.buildings->at(8).herbalist=0;
        auto herbalist=entertainer;herbalist.type=30;
        const auto treated=originalFigureRoam(herbalist,6,0x80,context);
        need(treated.state&&context.buildings->at(8).herbalist==96&&context.services->serviceGrid[10000]==0x80000fc7,
            "herbalist writes house service96 and grid category0 preserving entertainment and watchtower categories");
        context.buildings->at(9).type=0x7c;context.inspectorReduction=11;context.inspectorMaximum=std::array<int32_t,2>{0,0};
        context.buildings->at(8).fireRisk=25;context.buildings->at(8).damageRisk=40;
        auto inspector=entertainer;inspector.type=39;inspector.watchCoverage=3;
        const auto inspected=originalFigureRoam(inspector,6,0x80,context);
        need(inspected.state&&inspected.state->watchCoverage==40&&context.inspectorMaximum==std::array<int32_t,2>{25,40}&&
            context.buildings->at(8).fireRisk==0&&context.buildings->at(8).damageRisk==7&&
            (context.services->serviceGrid[10000]&(7u<<3))==(7u<<3),
            "Inspector repeats signed word risk reductions per occupied cell and retains both pre-subtraction process maxima");
        inspector.watchCoverage=60;const auto inspectedAgain=originalFigureRoam(inspector,6,0x80,context);
        need(inspectedAgain.state&&inspectedAgain.state->watchCoverage==50&&context.inspectorMaximum==std::array<int32_t,2>{25,40}&&
            context.buildings->at(8).damageRisk==0,"Inspector maxima persist while figure coverage decays by10");
        context.buildings->at(9).type=0xd6;context.buildings->at(9).shrineReligion=2;context.buildings->at(9).shrineElite=0;
        context.buildings->at(8).religion=std::array<uint8_t,4>{1,2,3,4};auto priest=entertainer;priest.type=35;
        const auto blessed=originalFigureRoam(priest,6,0x80,context);
        need(blessed.state&&context.buildings->at(8).religion==std::array<uint8_t,4>{1,2,40,4}&&
            (context.services->serviceGrid[10000]&(7u<<15))==(7u<<15),"Shrine writes only its serialized religion service and category5");
        context.buildings->at(9).shrineElite=1;context.buildings->at(8).religion->at(2)=7;
        need(originalFigureRoam(priest,6,0x80,context).state&&context.buildings->at(8).religion->at(2)==7,
            "elite-only Shrine gate skips ordinary occupied houses");
        {auto water=context;auto carrier=entertainer;carrier.type=28;
            water.buildings->at(9).type=0x48;water.buildings->at(9).houseModel=0;water.buildings->at(9).upgrade=0;
            water.buildings->at(8).waterService=std::array<uint8_t,2>{3,4};water.waterOverride=false;
            const auto delivered=originalFigureRoam(carrier,6,0x80,water);
            need(delivered.state&&water.buildings->at(8).waterService==std::array<uint8_t,2>{96,4}&&
                (water.services->serviceGrid[10000]&7)==7,"Well basic callback writes embedded32 and category0 through repeated house visits");
            water.waterOverride=true;need(originalFigureRoam(carrier,6,0x80,water).state&&water.buildings->at(8).waterService->at(1)==96,
                "Well global override redirects water to embedded34");
            water.waterOverride.reset();water.buildings->at(9).upgrade=1;
            need(originalFigureRoam(carrier,6,0x80,water).state.has_value(),"upgraded Well short circuits absent global predicate");
            water.buildings->at(9).upgrade=0;const auto before=water.services->serviceGrid;
            need(!originalFigureRoam(carrier,6,0x80,water).state&&water.services->serviceGrid==before,
                "basic Well without an explicit global predicate withholds the tentative callback atomically");}
        {auto roam=context;roam.randomByte.assign(51984,0);auto arrival=priest;arrival.direction=2;arrival.previousDirection=2;
            roam.buildings->at(9).roadX=-1;roam.buildings->at(9).roadY=100;roam.buildings->at(9).roamDirection=7;
            need(originalFigureBeginRoam(arrival,roam)&&arrival.progress==20&&arrival.roamState==1&&arrival.roamCountdown==255&&arrival.roamTurn==2&&
                arrival.roamOriginX==5&&arrival.roamOriginY==6&&arrival.direction==4&&arrival.destinationX==0&&arrival.destinationY==83&&
                roam.buildings->at(9).roamDirection==4,"ordinary roam arrival chooses a nonreverse road and publishes owner turn and clipped home-road target");
            roam.terrain[9999]=0x80;arrival=priest;arrival.previousDirection=0;
            need(originalFigureBeginRoam(arrival,roam)&&arrival.direction==4&&roam.buildings->at(9).roamDirection==4,
                "single initial roam road accepts the reverse direction without an RNG draw");
            roam.terrain[9772]=roam.terrain[10001]=roam.terrain[9999]=0xc0;
            roam.services->serviceGrid.assign(51984,7u<<15);roam.services->serviceGrid[10001]=0;
            arrival=priest;arrival.destinationX=5;arrival.destinationY=6;auto random=*roam.services;originalFigureRandomNext(random);
            need(originalFigureRoamTargetTurn(arrival,roam)&&arrival.direction==2&&arrival.roamCountdown==5&&
                roam.services->randomFirst==random.randomFirst&&roam.services->randomSecond==random.randomSecond,
                "roam turn target uses minimum category5 score and one persisted RNG draw when its minimum is unique");}
        context.terrain[9772]=0xc0;context.services->serviceGrid[9772]=7u<<6;
        context.services->serviceGrid[9999]=7u;context.services->serviceGrid[10228]=0;
        const auto watchRoads=originalFigureRoamRoads(watch,context),herbRoads=originalFigureRoamRoads(herbalist,context);
        need(watchRoads&&herbRoads&&!watchRoads->at(0)&&watchRoads->at(6)&&!herbRoads->at(6)&&herbRoads->at(0),
            "three-exit road filtering reads the walker's cited service category rather than universal entertainment bits");
        context.terrain[9772]=0xc8;context.buildingOwners[9772]=8;
        const auto builtRoad=originalFigureRoamRoads(herbalist,context);
        need(builtRoad&&builtRoad->at(0),"ordinary building road bit survives4B9590 before service-category filtering");
        std::vector<uint8_t> payload(28+152+12107+1461);const size_t size=28+152+12107;
        payload[0]=7;payload[12107+152]=6;payload[38]=0xfe;payload[39]=payload[40]=payload[41]=255;payload[11271]=73;
        const auto population=originalFigureSavedPopulation(payload,size);
        need(population&&population->total==-2&&population->peak==73,"population7 uses complete12107-byte extent and literal38/11271 wire inputs");
        payload[0]=6;need(!originalFigureSavedPopulation(payload,size),"population version6 cannot inherit the version7 extent");
        payload[0]=7;payload.resize(2100000);payload[size-176]=9;payload[9904+10]=0xfe;payload[9904+11]=255;
        payload[1985968+196]=1;
        const auto crops=originalFigureSavedCropServices(payload,0,size);
        need(original_save19::buildingDisabled==1985968&&crops&&crops->month==9&&crops->unavailable[5]==-2&&crops->disabled[196]==1,
            "SAV19 crop inputs follow independent literal wire9904,monthS-176 and disableT+1985968");
        auto truncated=payload;truncated.resize(1985968+268);need(!originalFigureSavedCropServices(truncated,0,size),
            "truncated final disable-table byte withholds every crop input");
        std::string model="HOUSE MODS\n";
        const auto row=[](int capacity){std::string s="row,{,";for(unsigned n=0;n<24;++n)s+=std::to_string(n==17?capacity:0)+",";return s+"\n";};
        for(unsigned n=0;n<5;++n)model+=row(0);model+="ALL HOUSES\n";for(unsigned n=0;n<15;++n)model+=row(int(n)+7);
        need(originalFigureHouseCapacities(model)->at(4)==11,"house capacity parser reads eighteenth column and sequential model row");
        auto changed=model;changed.replace(changed.find("row,{,"),row(0).size(),row(1));
        need(!originalFigureHouseCapacities(changed),"difficulty-dependent house capacity refuses without fitting a difficulty");
        need(originalFigureHouseModelColumn(model,15)->at(4)==0&&!originalFigureHouseModelColumn(model,24),
            "watch risk floor reads sixteenth model column with bounded indices");
        // Independently authored ten empty goal lists, Flood1 and Festival4.
        // The active festival names9, starts one phase ago and lasts one.
        std::vector<uint8_t> festivalBytes(1400000);
        const auto put16=[&](size_t p,unsigned v){festivalBytes[p]=uint8_t(v);festivalBytes[p+1]=uint8_t(v>>8);};
        const auto put32=[&](size_t p,uint32_t v){for(unsigned n=0;n<4;++n)festivalBytes[p+n]=uint8_t(v>>(8*n));};
        for(unsigned n=0;n<10;++n)put16(3355+6*n,1);
        put16(3415,1);const size_t festival=3415+2+17+51984;
        put16(festival,4);put16(festival+2,1);put32(festival+12,1);put32(festival+16,100);
        put32(festival+52,1);festivalBytes[festival+60]=1;put32(festival+62,9);
        const size_t t=100000,dateSize=200000;put32(t+1299324+4,0);put32(t+1299332,1);put32(dateSize-180,100);put32(dateSize-176,1);
        need(originalFigureSavedFestivalVenue(festivalBytes,t,dateSize)==9,"festival date equality remains active at the duration boundary");
        std::optional<bool> water;festivalBytes[festival+66]=1;put32(festival+67,3);put32(festival+71,4);
        need(originalFigureSavedFestivalVenue(festivalBytes,t,dateSize,&water)==9&&water==true,
            "Festival4 wire66/67/71 restores the enabled3/4 water override independently of venue qualification");
        festivalBytes[festival+60]=0;put32(festival+71,3);
        need(originalFigureSavedFestivalVenue(festivalBytes,t,dateSize,&water)==0&&water==false,
            "inactive Festival4 still restores water predicate and rejects the wrong subtype");festivalBytes[festival+60]=1;
        put32(t+1299332,2);need(originalFigureSavedFestivalVenue(festivalBytes,t,dateSize)==0,"expired active flag cannot select festival entertainment branch");
        put16(festival,3);need(!originalFigureSavedFestivalVenue(festivalBytes,t,dateSize),"unknown festival layout withholds venue and service traversal");
    }
    auto elapsed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(elapsed,terrain,figures,refs,1);
    need(elapsed.figurePlaced==0&&elapsed.figureUnsupported==1,"nonzero caller tick refuses unproved simulation phase");
    auto withheld=scene;OriginalFigureOrderedPass incomplete;
    installOriginalFigureOverlays(withheld,terrain,figures,refs,0,OriginalFigureRenderSource::PostLoadK1,&buildings,&incomplete);
    need(!withheld.figurePostLoadK1,"incomplete ordered context cannot label an existing snapshot as post-load k1");
    AppMapSurface absentRoot(ResourceRoot{});
    need(!absentRoot.selectSavedBuildings(terrain,buildings,nullptr,0,&figures,OriginalFigureRenderSource::PostLoadK1),
        "App refuses explicit k1 without a complete ordered pass before reading archives");
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
    std::vector<uint8_t> modelBytes;need(detail::readFile(root.resolve("Model/EmperorFigureModels.txt"),100000,modelBytes),"read supplied figure model without library environment access");
    const auto modelSpeeds=originalFigureModelSpeeds(std::string(modelBytes.begin(),modelBytes.end()));
    const auto models=originalFigureModels(std::string(modelBytes.begin(),modelBytes.end()));
    std::vector<uint8_t> buildingModel;need(detail::readFile(root.resolve("Model/EmperorBuildingModels.txt"),1048576,buildingModel),"read supplied house capacity model");
    const auto capacities=originalFigureHouseCapacities(std::string(buildingModel.begin(),buildingModel.end()));
    need(capacities&&capacities->at(4)==41,"same supplied model capacity is derived from all five difficulty rows");
    need(models&&models->at(11)[2]==0&&models->at(11)[5]==0&&models->at(40)[6]==5,"same complete model gives worker zero attacks and Hunter range");
    need(modelSpeeds&&modelSpeeds->at(76)==11,"complete supplied model publishes Pheasant speed11");
    AppMapSurface app(root);size_t sources=0,slots=0,live=0,placed=0,hidden=0,zero=0,unsupported=0,decoded=0,failures=0;
    std::set<unsigned> types,boundTypes;std::map<unsigned,size_t> blocked;size_t focused=0,placedDecoded=0;
    struct Score {size_t matched=0,total=0,selected=0;std::map<std::string,size_t> mismatch;};
    Score score,hiddenScore,cargoScore;size_t cargoNonzero=0,cargoNonzeroMatched=0;
    size_t zeroResolved=0,zeroBlocked=0,firstTickSelected=0,firstTickBlocked=0;std::map<unsigned,size_t> zeroTypes;
    size_t postLoadComputed=0,postLoadBlocked=0,postLoadChanged=0,postLoadOracleMatched=0;
    size_t postLoadPrimaryChanged=0,postLoadSecondaryChanged=0;std::map<unsigned,size_t> postLoadBlockedTypes;
    size_t postLoadSelected=0,postLoadDecoded=0,updateComputed=0,updateDecoded=0,updateConsistent=0,updateCargoDecoded=0;
    std::map<unsigned,size_t> updateCrossedTypes;
    std::map<unsigned,size_t> updateBlockedTypes,updateComputedTypes;std::map<std::string,size_t> updateBlockers;
    std::set<std::pair<size_t,size_t>> refusedSlots;
    size_t orderedComputed=0,orderedPrefix=0,orderedDecoded=0,orderedConsistent=0,orderedComplete=0,servicesParsed=0,routesValidated=0,orderedCargoDecoded=0;
    std::map<unsigned,size_t> orderedComputedTypes,orderedBlockedTypes;std::map<std::string,size_t> orderedBlockers;
    std::array<size_t,6> individualBySource{},orderedBySource{},prefixBySource{};
    size_t k1Placed=0,k1Decoded=0,k1Hidden=0,k1Unsupported=0,k1Removed=0,k1Overlays=0,k1Animation=0,k1Objects=0;
    struct TimelineScore {size_t pools=0,computed=0,outcomes=0,allocated=0,placed=0,decoded=0,consistent=0,primary=0,animation=0,animationCandidates=0,animationAdvanced=0,animationChanged=0,reselected=0,prefixReselected=0;
        std::map<std::string,size_t> refusals;};
    std::map<unsigned,TimelineScore> timelineScores,timelineUnseededScores;
    std::map<std::pair<unsigned,unsigned>,Score> byAction,hiddenAction;std::set<unsigned> selectors;
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::SaveFile)continue;
        const auto save=loadOriginalSave(root.resolve(source.relativePath));need(save.readStable&&save.layers.figures.has_value(),"same-source stable figure profile");
        const auto& figures=*save.layers.figures;
        const auto imageState=originalFigureImageState(figures);
        auto decoys=figures;for(auto& f:decoys.live){f.image=UINT32_MAX;f.secondaryImage=UINT32_MAX;}
        for(auto& f:decoys.slots){f.image=UINT32_MAX;f.secondaryImage=UINT32_MAX;}
        const auto decoyState=originalFigureImageState(decoys);
        auto updateContext=originalFigureUpdateContext(figures),decoyUpdateContext=originalFigureUpdateContext(decoys);
        need(updateContext&&decoyUpdateContext,"complete same-source saved context available for per-type virtual updates");
        updateContext->modelSpeeds=decoyUpdateContext->modelSpeeds=*modelSpeeds;
        updateContext->models=decoyUpdateContext->models=*models;
        updateContext->terrain=decoyUpdateContext->terrain=save.terrain.flags;
        updateContext->cropBase=decoyUpdateContext->cropBase=((228-save.terrain.activeSide)/2)*229;
        need(figures.factionModels.has_value(),"complete following Goods1/Empire3/Grid15/child5 publishes faction models");
        std::vector<uint8_t> savedBytes;need(detail::readFile(root.resolve(source.relativePath),4u*1024u*1024u,savedBytes),"read original container for following-layout refusal checks");
        const auto payload=decodeOriginalMapContainer(savedBytes).payload;
        auto services=originalFigureSavedServices(payload,save.terrainOffset);
        const auto population=originalFigureSavedPopulation(payload,save.sizeOffset);
        std::optional<bool> waterOverride;const auto festival=originalFigureSavedFestivalVenue(payload,save.terrainOffset,save.sizeOffset,&waterOverride);
        need(population&&festival&&services&&services->serviceGrid.size()==51984,"six sources frame population7, Festival4 venue and the complete service grid");
        constexpr std::array<int32_t,6> totals{0,572,993,28,0,28},peaks{0,1070,997,28,0,28};
        need(population->total==totals[sources]&&population->peak==peaks[sources]&&
            services->serviceGrid.front()==detail::u32(payload.data()+save.terrainOffset+1563037)&&
            services->serviceGrid.back()==detail::u32(payload.data()+save.terrainOffset+1770969),
            "all6 source population totals/peaks and service-grid endpoints agree with independent literal reads");
        services->population=population;
        updateContext->houseCapacities=decoyUpdateContext->houseCapacities=*capacities;
        const auto buildingText=std::string(buildingModel.begin(),buildingModel.end());
        const auto watchFloors=originalFigureHouseModelColumn(buildingText,15);const auto watchReduction=originalFigureWatchtowerReduction(buildingText);
        need(watchFloors&&watchReduction,"same model frames watchtower reduction and house risk floors");
        updateContext->houseWatchFloors=decoyUpdateContext->houseWatchFloors=*watchFloors;
        for(unsigned col=9;col<=13;++col){const auto values=originalFigureHouseModelColumn(buildingText,col);need(bool(values),"same complete model supplies Market demand columns");
            updateContext->houseMarketModels[col]=decoyUpdateContext->houseMarketModels[col]=*values;}
        updateContext->watchtowerReduction=decoyUpdateContext->watchtowerReduction=watchReduction;
        updateContext->elevation=decoyUpdateContext->elevation=save.layers.ground->state.gBEBF3A;
        updateContext->randomByte=decoyUpdateContext->randomByte=save.layers.ground->state.randomByte;
        updateContext->crtRandom=decoyUpdateContext->crtRandom=1; //Explicit reproducible test input;wall-clock seeded,not in SAV.
        need(waterOverride.has_value(),"complete Festival4 carries the saved Well override predicate");
        updateContext->waterOverride=decoyUpdateContext->waterOverride=waterOverride;
        updateContext->watchMaximum=decoyUpdateContext->watchMaximum=0; //declared fresh-process premise, not SAV state.
        updateContext->inspectorReduction=decoyUpdateContext->inspectorReduction=originalFigureWatchtowerReduction(buildingText,124);
        updateContext->inspectorMaximum=decoyUpdateContext->inspectorMaximum=std::array<int32_t,2>{0,0};
        updateContext->crops=decoyUpdateContext->crops=originalFigureSavedCropServices(payload,save.terrainOffset,save.sizeOffset);
        auto goods=originalFigureSavedGoodsServices(payload,figures.end,save.sizeOffset,save.terrainOffset);
        need(bool(goods),"same SAV19 frames Goods1 storage policy and Population7 accounting inputs");
        goods->storageSearch=std::array<uint8_t,36>{};goods->industrySearch=std::array<uint8_t,36>{}; //explicit fresh-process premises.
        updateContext->goods=decoyUpdateContext->goods=goods;
        updateContext->goods->godGate=decoyUpdateContext->goods->godGate=0; //5350A5 load reset.
        const auto requiredEmployment=parseOriginalBuildingEmployment(buildingText);need(bool(requiredEmployment),"complete model supplies goods candidate employment");
        updateContext->buildingEmployment=decoyUpdateContext->buildingEmployment=*requiredEmployment;
        need(updateContext->crops.has_value(),"same SAV19 frames month, population resource availability and269 disable bytes");
        updateContext->activeSide=decoyUpdateContext->activeSide=save.terrain.activeSide;
        updateContext->festivalVenue=decoyUpdateContext->festivalVenue=festival;
        updateContext->buildingOwners=decoyUpdateContext->buildingOwners=save.layers.ground->owner;
        for(auto value:save.layers.mapperObjects){updateContext->feature.push_back(uint8_t(value));decoyUpdateContext->feature.push_back(uint8_t(value));}
        need(services.has_value(),"same SAV19 contains complete route storage and persisted RNG state");++servicesParsed;
        need(figures.savedView==detail::u32(payload.data()+save.terrainOffset+1299324)&&figures.savedView==0&&
             services->randomFirst==detail::u32(payload.data()+save.terrainOffset+1299340)&&
             services->randomSecond==detail::u32(payload.data()+save.terrainOffset+1299344),
              "figure view and both RNG words independently equal corrected post-record read sites");
        need(services->fireCount&&*services->fireCount==0&&detail::u32(payload.data()+save.terrainOffset+1363850)==0,
            "same-source framed SAV19 restores C82F28 at the independently cited literal T+1363850");
        need(services->criminalCount&&uint32_t(*services->criminalCount)==detail::u32(payload.data()+save.terrainOffset+1391642),
            "all six saved criminal counts equal independent literal T+1391642 reads");
        need(services->allocationSerial&&*services->allocationSerial==detail::u32(payload.data()+save.terrainOffset+1363858),
            "all six saved allocation serials equal independent literal531580 T+1363858 reads");
        const size_t populationAt=save.sizeOffset-12287;
        need(goods->policies.size()==detail::u32(payload.data()+figures.end+2)&&
            goods->policies.at(0).accept[25]==payload.at(figures.end+6+12+25)&&
            goods->policies.at(0).limit[25]==detail::i16(payload.data()+figures.end+6+54+50)&&
            goods->warehouse[25]==detail::u32(payload.data()+populationAt+10866+100)&&
            goods->trade7[25]==detail::u32(payload.data()+populationAt+10578+100)&&
            goods->trade5[25]==detail::u32(payload.data()+populationAt+10722+100)&&
            goods->unavailable[25]==detail::u32(payload.data()+populationAt+10034+100),
            "six original Goods1 policies and Population7 resource25 arrays match independently framed literal reads");
        std::cout<<"{\"saved_fire_source\":"<<sources+1<<",\"terrain_relative_offset\":1363850,\"fire_count\":"<<*services->fireCount<<"}\n";
        const auto spawn=originalFigureSpawnServices(payload,save.sizeOffset);
        need(spawn.has_value(),"same scenario6 supplies four type76 spawn coordinates and validated crop");
        updateContext->entertainmentAvailable=decoyUpdateContext->entertainmentAvailable=spawn->entertainmentAvailable;
        const auto passability=originalFigurePheasantPassability(save.terrain,save.layers.mapperStone,save.buildings,*spawn);
        updateContext->services=decoyUpdateContext->services=*services;
        updateContext->passability=decoyUpdateContext->passability=passability;
        const auto hunterLinks=originalFigureHunterLinks(payload,save.buildings);
        need(hunterLinks.has_value(),"same-source Hunter common5 and embedded data1 frame linked-building validation");
        updateContext->hunterLinks=decoyUpdateContext->hunterLinks=hunterLinks;
        const auto buildingLinks=originalFigureBuildingLinks(payload,save.buildings);
        need(buildingLinks.has_value(),"same-source framed building common5 provides worker/home validity");
        int64_t loadPopulation=0;for(const auto& entry:*buildingLinks){const auto& h=entry.second;
            if(h.type>=2&&h.type<18&&h.alive&&h.alive!=2&&h.alive!=5&&h.alive!=6&&h.accepting)loadPopulation+=h.residents;}
        constexpr std::array<int32_t,6> loadedTotals{0,572,997,28,0,28};
        need(loadPopulation==loadedTotals[sources],"post-read533E71->517CC0/517DE0 house recount validates all6 sources including993->997");
        updateContext->buildings=decoyUpdateContext->buildings=buildingLinks;
        need(originalFigurePopulationAfterLoad(*updateContext->services,*buildingLinks)&&
            originalFigurePopulationAfterLoad(*decoyUpdateContext->services,*buildingLinks),"individual context uses post-load population recount before any worker update");
        const auto roadMasks=originalFigureRoadMovementMasks(save.terrain,save.buildings);
        if(roadMasks)updateContext->movementMasks=decoyUpdateContext->movementMasks=*roadMasks;
        for(const auto& f:figures.live)if(f.route>0){need(size_t(f.route)<services->routeOwners.size()&&services->routeOwners[f.route]==int(f.slot)&&
            f.routeLength>=0&&f.routeLength<=500&&f.routeCursor>=0&&f.routeCursor<=f.routeLength,
            "serialized positive route has reciprocal live owner and bounded saved cursor/length");++routesValidated;}
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
        const auto ordered=originalFigureOrderedFirstPass(figures,refs,save.terrain.flags,*modelSpeeds,&*services,passability?&*passability:nullptr,&*hunterLinks,&*buildingLinks,*models,spawn->cropBase,roadMasks?*roadMasks:std::vector<uint16_t>{},&*updateContext),
            orderedDecoys=originalFigureOrderedFirstPass(decoys,refs,save.terrain.flags,*modelSpeeds,&*services,passability?&*passability:nullptr,&*hunterLinks,&*buildingLinks,*models,spawn->cropBase,roadMasks?*roadMasks:std::vector<uint16_t>{},&*decoyUpdateContext);
        need(ordered.accepted&&ordered.figures.size()==figures.live.size()&&ordered.computed+ordered.blocked==figures.live.size()&&
            ordered.computed==orderedDecoys.computed,"ordered pass partitions complete same-source live pool independent of saved images");
        orderedComputed+=ordered.computed;orderedPrefix+=ordered.prefixComputed;orderedComplete+=ordered.complete;
        orderedBySource.at(sources-1)=ordered.computed;prefixBySource.at(sources-1)=ordered.prefixComputed;
        std::cout<<"{\"ordered_source\":"<<sources<<",\"complete\":"<<(ordered.complete?"true":"false")<<",\"computed\":"<<ordered.computed<<",\"prefix\":"<<ordered.prefixComputed<<",\"live\":"<<figures.live.size()<<",\"first_blocked_slot\":"<<ordered.firstBlockedSlot;
        if(ordered.firstBlockedSlot)std::cout<<",\"first_blocker\":\""<<originalFigureUpdateStatusName(ordered.figures.at(ordered.firstBlockedSlot).status)<<'"';
        std::cout<<"}\n";
        const auto general=loadSG3(root.resolve("DATA/China_General.sg3"));
        const auto employment=parseOriginalBuildingEmployment(std::string(buildingModel.begin(),buildingModel.end()));
        need(bool(employment),"complete employment model supplies timeline building gates");
        const auto groundArchive=loadSG3(root.resolve("DATA/China_Terrain.sg3")),elevationArchive=loadSG3(root.resolve("DATA/China_Elevation.sg3"));
        auto timelineScene=composeOriginalSavedGroundScene(save.terrain,save.buildings,*save.layers.ground,general,groundArchive,elevationArchive,0,&*employment);
        const auto fullPassability=originalSceneMovementPassability(timelineScene,save.buildings,*save.layers.ground,*spawn);
        const auto* timelinePassability=fullPassability?&*fullPassability:passability?&*passability:nullptr;
        const auto timelineMasks=fullPassability?fullPassability->cells:roadMasks?*roadMasks:std::vector<uint16_t>{};
        const auto initial=originalFigurePostLoadContext(figures,refs,save.terrain.flags,*modelSpeeds,&*services,timelinePassability,
            &*hunterLinks,&*buildingLinks,*models,spawn->cropBase,timelineMasks,&*updateContext);
        need(initial.complete&&initial.context.has_value(),"timeline initializes callbacks/population exactly once");
        for(bool suppliedCrt:{false,true})for(unsigned n:{1u,2u,3u,4u,5u,6u,10u,11u,12u,13u,19u,20u,26u,50u,84u,85u,98u,100u,200u}){
            std::vector<OriginalSceneTimelineFrame> frames;
            for(unsigned k=1;k<=n;++k){OriginalSceneTimelineFrame frame;frame.animationClock=37*k;
                frame.admission.now=37*k;frame.admission.speed=80;frame.continueAfterPass={false};
                for(const auto& p:timelineScene.placed)if(p.components.size()==1&&general.sprites[p.binding.physicalId-1].animationSpriteCount&&
                    originalBuildingAnimationEnabled(p.footprint,&*employment)==true)frame.buildingDraws.push_back(p.footprint.slot);
                frames.push_back(frame);}
            // Full active-crop draw premise, including empty cells which an
            // admitted movement may populate. Original view0 scan order.
            const unsigned origin=(228-save.terrain.activeSide)/2;
            for(auto& frame:frames)for(unsigned diagonal=0;diagonal<2*save.terrain.activeSide-1;++diagonal)
                for(unsigned x=0;x<save.terrain.activeSide;++x)if(diagonal>=x&&diagonal-x<save.terrain.activeSide)
                    frame.figureDrawCells.push_back((origin+diagonal-x)*228+origin+x);
            auto timelineInitial=*initial.context;if(suppliedCrt)timelineInitial.crtRandom=1;else timelineInitial.crtRandom.reset();
            const auto timeline=originalSceneTimeline(timelineInitial,refs,timelineScene,general,&*employment,0,{},frames);
            const unsigned firstFailure[]={0,27,20,suppliedCrt?99u:85u,0,0};const size_t firstSlot[]={0,1641,309,suppliedCrt?1999u:1981u,0,0};
            const unsigned expected=firstFailure[sources-1];
            auto& total=(suppliedCrt?timelineScores:timelineUnseededScores)[n];total.animationCandidates+=timelineScene.animationEnabled;
            need(timeline.buildingImageReselections==(sources==2&&n>=19?1u:0u),"warehouse withdrawal reselects once in its successful prefix; requested-N installation still requires every pass");
            std::set<size_t> outcomeSlots;for(const auto& f:figures.live)outcomeSlots.insert(f.slot);
            for(size_t slot:timeline.lastPass.allocatedSlots)outcomeSlots.insert(slot);
            total.outcomes+=outcomeSlots.size();total.allocated+=timeline.lastPass.allocatedSlots.size();
            std::cout<<"{\"timeline_source\":"<<sources<<",\"crt_seed\":\""<<(suppliedCrt?"explicit_1":"none")<<"\",\"requested_updates\":"<<n<<",\"complete\":"<<(timeline.complete?"true":"false")
                <<",\"completed_updates\":"<<timeline.updates<<",\"first_refused_update\":"<<timeline.refusedUpdate
                <<",\"first_refused_slot\":"<<timeline.lastPass.firstBlockedSlot<<",\"refusal\":\""<<timeline.refusal
                <<"\",\"building_image_reselections_in_completed_prefix\":"<<timeline.buildingImageReselections
                <<",\"outcome_total\":"<<outcomeSlots.size()<<",\"allocated_slots\":"<<timeline.lastPass.allocatedSlots.size()
                <<",\"crt_rand_calls_in_prefix\":"<<timeline.lastPass.crtRandomCalls<<"}\n";
            need(timeline.complete==(!expected||n<expected)&&timeline.updates==(expected&&n>=expected?expected-1:n)&&
                timeline.refusedUpdate==(expected&&n>=expected?expected:0)&&timeline.lastPass.firstBlockedSlot==(expected&&n>=expected?firstSlot[sources-1]:0),
                "six original timelines retain independently recorded first refusal boundaries and stop there");
            need(timeline.lastPass.crtRandomCalls==size_t(suppliedCrt&&sources==4&&n>=85),
                "only source4 allocation at N85 consumes CRT rand in the ported reachable prefix");
            if(!timeline.complete){need(!timeline.context,"failed timeline publishes no context from an earlier completed update");
                total.prefixReselected+=timeline.buildingImageReselections;
                for(size_t slot:outcomeSlots){const auto found=timeline.lastPass.figures.find(slot);
                    ++total.refusals[found==timeline.lastPass.figures.end()?"prior_removed_outcome_withheld":originalFigureUpdateStatusName(found->second.status)];}
                auto unchanged=timelineScene;need(!installOriginalSceneTimeline(unchanged,save.terrain,figures,refs,save.buildings,general,timeline),
                    "refused timeline cannot install earlier frame as requested N");continue;}
            ++total.pools;total.computed+=outcomeSlots.size();
            total.reselected+=timeline.buildingImageReselections;
            total.animationAdvanced+=timeline.buildingAdvances;
            for(const auto& p:timelineScene.placed){const auto found=timeline.buildingPhases.find(p.footprint.slot);
                if(found!=timeline.buildingPhases.end())total.animationChanged+=found->second!=*p.footprint.animationInputs->phase;}
            auto installed=timelineScene;need(installOriginalSceneTimeline(installed,save.terrain,figures,refs,save.buildings,general,timeline),"complete timeline installs figure and building phases atomically");
            total.placed+=installed.figurePlaced;
            const auto finalImages=originalFigureUpdateImageState(*timeline.context);
            for(const auto& e:timeline.context->figures){const auto& u=e.second;if(!u.alive)continue;
                const auto selected=figureImage(timeline.context->primaryImageInputs.at(u.slot),refs,save.terrain.flags.at(u.cell),figures.savedView,
                    save.buildings.slotTypes.at(size_t(u.owner)),&finalImages);
                need(selected.status==OriginalFigureImageStatus::Selected&&selected.image==u.image,"timeline surviving primary agrees with independent selector");
                ++total.consistent;++total.primary;}
            for(const auto& overlay:installed.overlays){if(overlay.mode==4){const auto word=(overlay.tileset<<14)+overlay.physicalId-1;
                    need(decodeWord(word),"every placed timeline primary/cargo component decodes");++total.decoded;}
                if(overlay.mode==3){need(decodeSG3Sprite(general,overlay.physicalId-1,root.resolve("DATA/China_General.555")).image.valid(),"every timeline building frame decodes");++total.animation;}}
        }
        auto k1=scene;installOriginalFigureOverlays(k1,save.terrain,figures,refs,0,OriginalFigureRenderSource::PostLoadK1,&save.buildings,&ordered);
        need(k1.figurePostLoadK1&&k1.figureLive==figures.live.size()&&k1.animationTickMs==0,"explicit k1 scene accepts complete pool and retains building animation tick0");
        need(app.selectPostLoadK1(save,payload),"App computes and installs explicit same-source post-load k1");
        const auto k1View=app.renderSavedBuildings(camera);
        need(k1View.image.valid()&&k1View.figurePostLoadK1&&k1View.timelineUpdates==1&&k1View.timelineFrames==1&&
            k1View.figurePlaced==k1.figurePlaced&&k1View.overlayDecodeFailures==0&&k1View.overlayInvalid==0,
            "k1 App decodes same ordered scene with no invalid overlay or decode failures");
        k1Placed+=k1View.figurePlaced;k1Decoded+=k1View.figureDecoded;k1Hidden+=k1View.figureHidden;k1Unsupported+=k1View.figureUnsupported;
        k1Removed+=k1View.figureRemoved;
        k1Overlays+=k1View.overlayObjects;k1Animation+=k1View.animationDecoded;k1Objects+=k1View.overlayObjects-k1View.figureDecoded-k1View.animationDecoded;
        std::cout<<"{\"k1_scene_source\":"<<sources<<",\"placed\":"<<k1View.figurePlaced<<",\"decoded_components\":"<<k1View.figureDecoded
            <<",\"hidden\":"<<k1View.figureHidden<<",\"unsupported\":"<<k1View.figureUnsupported<<",\"overlays\":"<<k1View.overlayObjects<<"}\n";
        for(const auto& f:figures.live){const auto& result=ordered.figures.at(f.slot);const auto& decoy=orderedDecoys.figures.at(f.slot);
            need(result.status==decoy.status&&bool(result.state)==bool(decoy.state),"ordered statuses ignore both saved image words");
            if(!result.state){++orderedBlockedTypes[f.type];++orderedBlockers[originalFigureUpdateStatusName(result.status)];continue;}
            ++orderedComputedTypes[f.type];const auto& u=*result.state;
            if(!u.alive){need(f.type==11&&u.type==0&&u.image==0&&u.age==0&&ordered.context->freeSlots.back()==f.slot&&
                u.priorProgress==20,"ordered worker removal resets fields and appends the freed slot after employment writes");continue;}
            need(u.image==decoy.state->image&&u.age==f.age+1&&u.priorProgress==u.progress,"ordered computed output contains dispatch age and post-return progress");
            auto finalImages=imageState;if(ordered.context)finalImages=originalFigureUpdateImageState(*ordered.context);
            const auto selected=figureImage(originalFigureImageInputs(u),refs,save.terrain.flags.at(u.cell),figures.savedView,
                save.buildings.slotTypes.at(size_t(u.owner)),&finalImages);
            need(selected.status==OriginalFigureImageStatus::Selected&&selected.image==u.image,"ordered updated image matches independent phase selector");++orderedConsistent;
            need(decodeWord(u.image),"ordered computed primary image decodes");++orderedDecoded;
            if(u.secondaryImage){need(decodeWord(u.secondaryImage),"ordered final cargo image decodes");++orderedCargoDecoded;}
        }
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
            if(!update.state){++updateBlockedTypes[f.type];++updateBlockers[originalFigureUpdateStatusName(update.status)];
                refusedSlots.emplace(sources,f.slot);
                std::cout<<"{\"update_refusal_source\":"<<sources<<",\"slot\":"<<f.slot<<",\"type\":"<<unsigned(f.type)<<",\"action\":"<<unsigned(f.action)<<",\"blocker\":\""<<originalFigureUpdateStatusName(update.status)<<"\"}\n";continue;}
            ++updateComputed;++updateComputedTypes[f.type];++individualBySource.at(sources-1);const auto& u=*update.state;
            if(sources==2&&f.slot==1750){need(u.direction==4&&u.progress==0&&u.subY==f.subY+1&&u.routeCursor==f.routeCursor+1&&
                update.buildings->at(470).entertainment->at(1)==96&&update.services->serviceGrid[f.cell]==(services->serviceGrid[f.cell]|0xe00),
                "original entertainer publishes house music96, category3 grid bits and south road before image");}
            if(sources==3&&f.slot==320){need(f.employmentAmount==10&&u.alive==2&&update.buildings->at(61).residents==41&&
                update.buildings->at(61).freeSlots==0&&update.buildings->at(61).worker==0&&update.services->population->total==1007&&
                update.services->population->peak==1007,"original worker arrival publishes exact41 residents,0 free slots and1007 population before dispatcher removal");}
            if(u.cell!=f.cell){++updateCrossedTypes[f.type];
                need(u.priorX==f.x&&u.priorY==f.y&&u.priorCell==f.cell&&(update.cellHeads||!update.linkedWrites.empty())&&
                    u.cell==uint32_t(int64_t(spawn->cropBase)+u.x+228LL*u.y),
                    "original crossing output publishes cell heads, prior coordinates and validated crop-derived cell");}
            need(u.image==decoyUpdate.state->image&&u.secondaryImage==decoyUpdate.state->secondaryImage&&
                u.action==decoyUpdate.state->action&&u.x==decoyUpdate.state->x&&u.y==decoyUpdate.state->y&&u.counter==decoyUpdate.state->counter&&
                u.herdTarget==decoyUpdate.state->herdTarget&&u.herdDistance==decoyUpdate.state->herdDistance&&
                u.threat==decoyUpdate.state->threat&&u.threatDistance==decoyUpdate.state->threatDistance&&
                u.idleCounter==decoyUpdate.state->idleCounter&&u.cooldowns==decoyUpdate.state->cooldowns,
                "bounded original virtual update computes state from serialized non-image inputs");
            if(f.action==19&&f.type==76)need(u.action==32&&update.services&&decoyUpdate.services&&
                update.services->randomFirst==decoyUpdate.services->randomFirst&&update.services->randomSecond==decoyUpdate.services->randomSecond&&
                u.destinationX==decoyUpdate.state->destinationX&&u.destinationY==decoyUpdate.state->destinationY,
                "original action19 destination and mutated RNG depend only on same-source non-image services");
            if(f.type==76)need(u.idleCounter==f.idleCounter+(f.action==31)&&u.threat==0&&u.threatDistance==100&&u.movementMode==5&&
                u.hidden==0&&u.freeMovement==0&&u.linkedBuilding==decoyUpdate.state->linkedBuilding&&u.cell==f.cell,
                "original Pheasant computes predicates before its idle or bounded route branch");
            const auto selected=figureImage(originalFigureImageInputs(u),refs,save.terrain.flags.at(u.cell),figures.savedView,
                save.buildings.slotTypes.at(size_t(u.owner)),&imageState);
            need(selected.status==OriginalFigureImageStatus::Selected&&selected.image==u.image,
                "consistency only: figureImage of updated state equals updater image write; no saved post-update oracle");
            ++updateConsistent;need(decodeWord(u.image),"computed first-update image decodes");++updateDecoded;
            if(f.type==39&&f.action==8)need(u.searchCounter==f.searchCounter&&u.roamSteps==f.roamSteps+modelSpeeds->at(39)&&
                u.action==8&&u.hidden==0&&update.services&&update.services->fireCount==0,
                "original Inspector action8 consumes same-source saved zero fire count and publishes its full bounded roam update");
            if(u.secondaryImage){need(decodeWord(u.secondaryImage),"computed first-update cargo image decodes");++updateCargoDecoded;}
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
        std::cout<<"{\"figure_source\":"<<sources<<",\"saved_view\":"<<figures.savedView<<",\"slots\":"<<figures.declared<<",\"live\":"<<figures.live.size()<<",\"placed\":"<<view.figurePlaced<<",\"hidden\":"<<view.figureHidden<<",\"zero_image\":"<<view.figureZeroImage<<",\"unsupported\":"<<view.figureUnsupported<<",\"decoded_images\":"<<view.figureDecoded
            <<",\"forward_matched\":"<<view.figureForwardMatched<<",\"forward_compared\":"<<view.figureForwardCompared<<"}\n";
    }
    need(sources==6&&slots==12000&&live==192&&types.size()==20&&boundTypes.size()==18&&placed==158&&hidden==6&&zero==28&&unsupported==0&&decoded==174&&failures==0&&focused==158&&placedDecoded==158,"six-source original denominator");
    need(postLoadComputed==192&&postLoadBlocked==0&&postLoadChanged+postLoadOracleMatched==192&&
        postLoadSecondaryChanged==16&&postLoadBlockedTypes.empty()&&postLoadSelected==17&&postLoadDecoded==17,
        "post-load image step computes192/192 and decodes17/17 selected callback images");
    std::cout<<"{\"post_load_step\":\"callback_images_before_simulation\",\"computed\":"<<postLoadComputed<<",\"live_total\":"<<live
        <<",\"blocked\":"<<postLoadBlocked<<",\"blocked_types\":{},\"saved_word_comparison\":{\"equal_pairs\":"<<postLoadOracleMatched
        <<",\"changed_pairs\":"<<postLoadChanged<<",\"primary_changed\":"<<postLoadPrimaryChanged<<",\"secondary_clears\":"<<postLoadSecondaryChanged
        <<"},\"selected_nonzero\":"<<postLoadSelected<<",\"decoded\":"<<postLoadDecoded<<",\"decode_failures\":0,\"post_update_oracle\":\"unmeasured\"}\n";
    std::cout<<"{\"update_measurement\":"<<updateComputed<<",\"cargo\":"<<updateCargoDecoded<<",\"ordered_prefix\":"<<orderedPrefix<<",\"types\":{";
    bool measuredComma=false;for(const auto& e:updateComputedTypes){if(measuredComma)std::cout<<',';measuredComma=true;std::cout<<'"'<<e.first<<"\":"<<e.second;}std::cout<<"}}\n";
    need(updateComputed==192&&updateDecoded==192&&updateConsistent==192&&updateComputedTypes[76]==69&&
        updateComputedTypes[11]==17&&updateComputedTypes[19]==18&&updateComputedTypes[20]==20&&updateComputedTypes[21]==2&&
        updateComputedTypes[22]==1&&updateComputedTypes[40]==14&&updateCargoDecoded==16,
        "first-update state computes192/192, decodes208/208 including cargo16, selector consistency192/192; Pheasant69/69");
    need(updateComputedTypes==std::map<unsigned,size_t>{{1,7},{11,17},{13,3},{19,18},{20,20},{21,2},{22,1},{23,4},{26,8},{28,1},
        {29,2},{30,2},{34,1},{35,2},{36,4},{39,5},{40,14},{45,4},{47,8},{76,69}},"individual per-type counts close the complete192-occurrence denominator");
    need(individualBySource==std::array<size_t,6>{21,46,65,32,7,21}&&orderedBySource==individualBySource&&
        prefixBySource==individualBySource,"all six ordered pools publish their complete final context");
    std::cout<<"{\"crossed_types\":{";measuredComma=false;for(const auto& e:updateCrossedTypes){if(measuredComma)std::cout<<',';measuredComma=true;std::cout<<'"'<<e.first<<"\":"<<e.second;}std::cout<<"}}\n";
    need(updateCrossedTypes==std::map<unsigned,size_t>{{20,1},{40,1},{45,1}},
        "original computed first updates publish all crossing prior-cell and list writes");
    need(updateBlockers.empty()&&refusedSlots.empty(),"individual refusals partition exactly0/192 occurrences");
    std::cout<<"{\"first_update_state\":true,\"k\":1,\"computed\":"<<updateComputed<<",\"live_total\":"<<live
        <<",\"decoded\":"<<updateDecoded<<",\"cargo_decoded\":"<<updateCargoDecoded<<",\"image_consistency\":"<<updateConsistent<<",\"decode_failures\":0,\"computed_types\":{";
    bool updateComma=false;for(const auto& e:updateComputedTypes){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<e.first<<"\":"<<e.second;}
    std::cout<<"},\"blocked_types\":{";updateComma=false;for(const auto& entry:updateBlockedTypes){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<entry.first<<"\":"<<entry.second;}
    std::cout<<"},\"blockers\":{";updateComma=false;for(const auto& entry:updateBlockers){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<entry.first<<"\":"<<entry.second;}
    std::cout<<"},\"first_draw\":\"after_explicit_k_admitted_updates\",\"ordinary_k\":[0,1],\"fast_forward_max_k\":10000,\"post_update_oracle\":\"unmeasured\"}\n";
    need(orderedComputed==192&&orderedPrefix==192&&orderedDecoded==191&&orderedConsistent==191&&orderedComplete==6&&servicesParsed==6&&routesValidated==78&&
        orderedComputedTypes==updateComputedTypes,"complete ordered pass computes192/192 from6/6 pools,78 reciprocal saved routes");
    need(orderedBlockers.empty(),"ordered refusal names and counts partition exactly0/192 final states");
    need(k1Placed==185&&k1Decoded==201&&k1Hidden==6&&k1Removed==1&&k1Unsupported==0&&k1Overlays==274&&k1Animation==53&&k1Objects==20&&
        k1Placed+k1Hidden+k1Removed+k1Unsupported==192,"k1 scene partitions192 originals:185 placed,6 hidden,1 removed,0 special-placement refusals;274 overlays");
    for(bool suppliedCrt:{false,true})for(const auto& row:(suppliedCrt?timelineScores:timelineUnseededScores)){
        const auto& t=row.second;size_t refused=0;for(const auto& r:t.refusals)refused+=r.second;
        const unsigned n=row.first,projectileLimit=suppliedCrt?99:85;
        const size_t expectedPools=n<20?6:n<27?5:n<projectileLimit?4:3;
        const size_t expectedComputed=n<20?192:n<27?127:n<85?81:n<projectileLimit?82:49;
        need(t.pools==expectedPools&&t.computed==expectedComputed&&t.placed==(n<20?(n==4||n==11?184:185):n<27?125:expectedComputed)&&
            t.decoded==(n<20?(n==4||n==11?200:n==19?202:201):n<27?130:n>=85&&n<projectileLimit?83:expectedComputed)&&
            t.animation==(n<20?53:n<27?28:n<projectileLimit?6:3),
          "perN timeline coverage locks complete-pool placements, decoded components and building frames");
        need(t.outcomes==192+(suppliedCrt&&n>=85)&&t.allocated==size_t(suppliedCrt&&n>=85)&&t.computed+refused==t.outcomes&&t.consistent==t.primary,
            "perN computed/refused outcomes partition originals plus allocations and every surviving primary agrees at its write");
        need(t.primary==(n<=10?191u:n==11?190u:n<20?189u:n<27?126u:n<85?81u:n<projectileLimit?82u:49u)&&
            t.reselected==size_t(n>=19&&n<27)&&t.prefixReselected==size_t(n>=27),
             "perN surviving primary denominator and original-data building reselection count are locked");
        std::map<std::string,size_t> expectedRefusals;
        if(n>=20&&n<27)expectedRefusals={{"type_action_route_target_combat_update",1},{"earlier_ordered_slot_uncomputed",20},
            {"later_ordered_slot_may_write_earlier_figure",42},{"prior_removed_outcome_withheld",2}};
        else if(n>=27){expectedRefusals={{"type_action_route_target_combat_update",1},{"earlier_ordered_slot_uncomputed",60},
            {"later_ordered_slot_may_write_earlier_figure",n<projectileLimit?46u:suppliedCrt?78u:67u},{"prior_removed_outcome_withheld",3},{"goods_delivery_arrival_virtual_4cd8f0",1}};
            if(n>=projectileLimit){if(suppliedCrt)expectedRefusals["projectile_allocation_or_collision_5ea510_5e6200"]=1;
                else {expectedRefusals["crt_rand_state_required_wall_clock_seeded_not_in_sav"]=1;expectedRefusals["earlier_ordered_slot_uncomputed"]+=10;}}}
        std::cout<<"{\"timeline_N\":"<<row.first<<",\"crt_seed\":\""<<(suppliedCrt?"explicit_1":"none")<<"\",\"pools_complete\":"<<t.pools<<",\"pool_total\":6,\"computed\":"<<t.computed
            <<",\"original_total\":192,\"outcome_total\":"<<t.outcomes<<",\"allocated_slots\":"<<t.allocated
            <<",\"placed\":"<<t.placed<<",\"decoded_components\":"<<t.decoded<<",\"primary_consistent\":"<<t.consistent
            <<",\"primary_total\":"<<t.primary<<",\"building_frames_decoded\":"<<t.animation<<",\"building_candidates\":"<<t.animationCandidates
            <<",\"building_admitted_advances\":"<<t.animationAdvanced<<",\"building_changed_phases\":"<<t.animationChanged
            <<",\"building_image_reselections\":"<<t.reselected<<",\"withheld_prefix_reselections\":"<<t.prefixReselected<<",\"refusals\":{";
        bool comma=false;for(const auto& r:t.refusals){if(comma)std::cout<<',';comma=true;std::cout<<'"'<<r.first<<"\":"<<r.second;}std::cout<<"}}\n";
        need(t.refusals==expectedRefusals,"perN refusal classes and multiplicities match exact disjoint original-outcome partition");
    }
    std::cout<<"{\"k1_scene\":true,\"placed\":"<<k1Placed<<",\"live_total\":192,\"decoded_components\":"<<k1Decoded<<",\"hidden\":"<<k1Hidden
        <<",\"removed\":"<<k1Removed<<",\"unsupported\":"<<k1Unsupported<<",\"overlays\":"<<k1Overlays<<",\"animation_overlays\":"<<k1Animation<<",\"object_overlays\":"<<k1Objects<<"}\n";
    std::cout<<"{\"ordered_first_pass\":true,\"k\":1,\"computed\":"<<orderedComputed<<",\"live_total\":"<<live
        <<",\"prefix_computed\":"<<orderedPrefix<<",\"decoded\":"<<orderedDecoded<<",\"cargo_decoded\":"<<orderedCargoDecoded<<",\"image_consistency\":"<<orderedConsistent<<",\"decode_failures\":0,\"complete_sources\":"<<orderedComplete
        <<",\"source_total\":6,\"services_parsed\":"<<servicesParsed<<",\"positive_routes_validated\":"<<routesValidated<<",\"computed_types\":{";
    updateComma=false;for(const auto& e:orderedComputedTypes){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<e.first<<"\":"<<e.second;}
    std::cout<<"},\"blocked_types\":{";updateComma=false;for(const auto& e:orderedBlockedTypes){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<e.first<<"\":"<<e.second;}
    std::cout<<"},\"blockers\":{";updateComma=false;for(const auto& e:orderedBlockers){if(updateComma)std::cout<<',';updateComma=true;std::cout<<'"'<<e.first<<"\":"<<e.second;}
    std::cout<<"},\"post_update_oracle\":\"unmeasured\"}\n";
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
