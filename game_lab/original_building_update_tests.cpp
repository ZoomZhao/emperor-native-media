#include "original_building_update.hpp"
#include "original_save.hpp"
#include "original_pak.hpp"
#include "original_building_scene.hpp"
#include "native_city_context.hpp"
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const char* message){++checks;if(!v)throw std::runtime_error(message);}
OriginalBuildingUpdateState field(uint16_t type=194,int16_t progress=0,uint16_t flags=0){
    OriginalBuildingUpdateState s;s.building.type=type;s.building.slot=7;s.building.size=1;
    s.building.imageInputs=OriginalBuildingImageInputs{};s.building.imageInputs->randomByte=0;s.building.imageInputs->cropWord4=progress;
    s.inputs.identity=7;s.inputs.alive=1;s.inputs.output=type==194?19:1;s.inputs.industry.emplace();
    s.inputs.industry->at(0)=uint8_t(progress);s.inputs.industry->at(1)=uint8_t(uint16_t(progress)>>8);
    originalBuildingSetCropFlags(s.inputs,flags);return s;
}
OriginalBuildingUpdateContext context(OriginalBuildingUpdateScope scope){
    OriginalBuildingUpdateContext c(scope,true);c.phase=0;c.monthPart=1;c.cycle=4;c.month=7;c.terrainFlags=8;c.typeDisabled=false;return c;
}
OriginalBuildingUpdateContext tailFixture(){
    auto c=context(OriginalBuildingUpdateScope::AdmittedTicks);c.phase=50;c.monthPart=0;c.cycle=4;
    c.cycleTail.emplace();c.cycleTail->alternateMode=false;c.cycleTail->smoothedHealth=59;c.cycleTail->healthTarget=80;
    c.cycleTail->productionHistory={3,901,2,603};c.cycleTail->currentHealthRating=147;
    c.labor.emplace();c.labor->difficulty=2;c.labor->health=59;c.populationTotal=400;
    c.buildingTotals.emplace();c.buildingTotals->staffedProducers[4]=2;c.buildingTotals->staffedProducers[2]=1;
    c.goodsRecords.emplace();c.goods.emplace();c.trade.emplace();c.statistics.emplace();c.statistics->currentFaction=0;
    c.scenarioMonths=0;c.elapsedYears=-2050;c.scenarioStartYears=-2050;c.monthlyServices=true;
    c.terrain=std::vector<uint32_t>(51984);c.terrainOwners=std::vector<uint16_t>(51984);c.activeSide=1;c.cropBase=229;
    OriginalBuildingFootprint b;b.slot=7;b.type=72;b.size=2;b.wellImageInputs.emplace();b.wellImageInputs->byte6f=2;
    OriginalBuildingUpdateInputs i;i.identity=7;i.alive=1;i.word92=123;i.modelUpgrade=2;
    auto crop=field();crop.inputs.modelUpgrade=0;crop.inputs.word92=45;
    crop.building.slot=8;crop.inputs.identity=8;originalBuildingSetCropFlags(crop.inputs,33);
    c.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,b},{8,crop.building}};
    c.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,i},{8,crop.inputs}};
    c.notificationEffects={1234};c.buildingRedraws={6};return c;
}
// The existing C1 field visitor covers all city-owned tail/trade/forecast data.
// Schedule, Labor, per-building expiry/flags and output buffers have separate
// owners, so include those explicitly rather than claiming a new calendar codec.
std::string tailSnapshot(OriginalBuildingUpdateContext c){
    auto city=captureNativeCityContext(c);native_city_context_detail::Writer w;native_city_context_detail::fields(w,city);
    w.value(c.phase);w.value(c.cycle);w.value(c.monthPart);w.value(c.month);
    w.out<<c.labor.has_value()<<' ';if(c.labor){w.value(c.labor->health);w.value(c.labor->difficulty);}
    if(c.buildingInputs)for(auto& e:*c.buildingInputs){w.value(e.first);w.value(e.second.alive);w.value(e.second.word92);w.value(e.second.modelUpgrade);w.value(e.second.industry);}
    if(c.buildings)for(auto& e:*c.buildings){w.value(e.first);w.out<<e.second.wellImageInputs.has_value()<<' ';if(e.second.wellImageInputs)w.value(e.second.wellImageInputs->byte6f);}
    w.value(c.notificationEffects);for(auto value:c.buildingRedraws)w.value(value);return w.out.str();
}
std::string messageSnapshot(OriginalBuildingScenarioMessages m){
    native_city_context_detail::Writer w;w.value(m.records);w.value(m.count);w.value(m.notificationPending);
    for(auto& n:m.notices){w.value(n.id);w.value(n.slot);w.value(n.faction);w.value(n.resource);w.value(n.amount);w.value(n.title);w.value(n.dialogFlags);}return w.out.str();
}
void refusedTail(OriginalBuildingUpdateContext c,OriginalBuildingUpdateBlocker expected,const char* reason){
    const auto before=tailSnapshot(c);need(originalBuildingCycleTailStep(c)==expected,reason);
    need(tailSnapshot(c)==before,"failed tail preserves all owned state, schedule, Labor, expiry and output buffers");
    need(originalBuildingCycleTailStep(c)==expected&&tailSnapshot(c)==before,"retry of refused tail has no cumulative health/date/expiry effects");
}
void cycleTailTransactions(){
    for(uint32_t part=0;part<=15;++part){auto c=tailFixture();c.monthPart=part;
        const bool missing=part==5||part==6||part==7||part==11||part==15;
        if(missing){refusedTail(c,OriginalBuildingUpdateBlocker::MonthlyCity,"unknown postincrement monthly service remains refused");continue;}
        need(originalBuildingCycleTailStep(c)==OriginalBuildingUpdateBlocker::None,"already supported ordinary parts remain successful");
        need(c.monthPart==part+1&&c.cycle==5&&c.phase==0&&c.cycleTail->calls==1&&c.cycleTail->smoothedHealth==60&&c.labor->health==60,"successful tail commits one health/date/call update");
        need(c.buildingInputs->at(7).word92==0&&c.buildingInputs->at(7).modelUpgrade==1&&c.buildings->at(7).wellImageInputs->byte6f==1&&originalBuildingCropFlags(c.buildingInputs->at(8))==32,"successful expiry/image and crop decay still use the existing rules");
        need(c.cycleTail->foodForecast==std::array<int32_t,2>{3604,901},"successful tail preserves independent production forecast");
    }
    for(uint32_t month:{0u,1u,11u}){auto c=tailFixture();c.month=month;c.monthPart=15;c.cycle=UINT32_MAX;
        refusedTail(c,OriginalBuildingUpdateBlocker::MonthlyCity,"unimplemented month/year crossing rejects without changing saved dates");}
    auto c=tailFixture();c.cycle=UINT32_MAX;
    need(originalBuildingCycleTailStep(c)==OriginalBuildingUpdateBlocker::None&&c.cycle==0,"existing unsigned cycle wrap is preserved only on successful tail");
    c=tailFixture();c.goods.reset();refusedTail(c,OriginalBuildingUpdateBlocker::MonthlyCity,"missing forecast input rolls back prior health and expiry");
    c=tailFixture();c.buildingInputs->at(7).modelUpgrade.reset();refusedTail(c,OriginalBuildingUpdateBlocker::MonthlyCity,"missing expiry state rolls back earlier date effects");
    c=tailFixture();c.monthPart=2;c.cycleTail->currentHealthRating=151;refusedTail(c,OriginalBuildingUpdateBlocker::MonthlyCity,"missing previous rating rolls back forecast and health effects");
    c=tailFixture();c.monthPart=3;c.monthlyServices.reset();refusedTail(c,OriginalBuildingUpdateBlocker::MonthlyCity,"missing fortification service rolls back health/date/expiry");
    c=tailFixture();c.cycleTail->alternateMode=true;refusedTail(c,OriginalBuildingUpdateBlocker::MonthlyCity,"alternate calendar mode is not silently admitted");
    c=tailFixture();c.monthPart=1;c.scenarioMonths=-1;auto& partner=c.trade->partners[0];partner.alive=1;partner.enabled=1;partner.relationTimers.resize(22);partner.relationTimers[0][0][0]=2;
    refusedTail(c,OriginalBuildingUpdateBlocker::InvalidState,"late invalid scenario age rolls back earlier partner timer decrement");
    c=tailFixture();c.buildings->erase(7);const auto beforeException=tailSnapshot(c);bool threw=false;
    try{(void)originalBuildingCycleTailStep(c);}catch(const std::out_of_range&){threw=true;}
    need(threw&&tailSnapshot(c)==beforeException,"mid-tail exception preserves caller state and retains the original error");
    // One known positive annual message can be computed before a later missing
    // timer service refuses. Both the context AND external records must roll back.
    auto annual=tailFixture();annual.month=1;annual.monthPart=1;auto& own=annual.trade->partners[0];own.alive=1;own.faction=0;own.blocked=1;
    auto& ally=annual.trade->partners[1];ally.alive=1;ally.faction=1;ally.kind=2;ally.enabled=1;ally.relations[0].state=2;ally.relations[0].war=1;own.relations[1].state=2;
    std::array<OriginalBuildingAnnualOffer,22> offers{};offers[1]={100,29};OriginalBuildingScenarioMessages messages;messages.records[0][113]=0xa5;
    const auto prior=tailSnapshot(annual),priorMessages=messageSnapshot(messages);
    for(unsigned retry=0;retry<2;++retry){need(originalBuildingCycleTailWithAnnualStep(annual,offers,&messages,uint8_t(2))==OriginalBuildingUpdateBlocker::MonthlyCity,"annual message followed by missing timers refuses");
        need(tailSnapshot(annual)==prior&&messageSnapshot(messages)==priorMessages,"late annual failure publishes neither notice nor health/date/diplomacy prefix");}
    auto complete=annual;complete.trade->partners[1].relationTimers.resize(22);
    auto noMode=complete;need(originalBuildingCycleTailWithAnnualStep(noMode,offers,&messages)==OriginalBuildingUpdateBlocker::MonthlyCity&&tailSnapshot(noMode)==tailSnapshot(complete)&&messageSnapshot(messages)==priorMessages,"unknown annual response mode remains atomic refusal");
    auto restored=complete;auto replayMessages=messages;
    need(originalBuildingCycleTailWithAnnualStep(complete,offers,&messages,uint8_t(2))==OriginalBuildingUpdateBlocker::None&&messages.count==1&&messages.notices.size()==1&&messages.notificationPending&&complete.cycle==5&&complete.monthPart==2,"supported annual message and date commit together");
    need(messages.records[0][113]==0xa5&&messages.notices[0].resource==29&&messages.notices[0].amount==100,"successful annual record preserves padding and original demand units");
    need(originalBuildingCycleTailWithAnnualStep(restored,offers,&replayMessages,uint8_t(2))==OriginalBuildingUpdateBlocker::None&&tailSnapshot(restored)==tailSnapshot(complete)&&messageSnapshot(replayMessages)==messageSnapshot(messages),"same restored annual input produces identical next state and one notice");
    need(originalBuildingCycleTailWithAnnualStep(complete,offers,&messages,uint8_t(2))==OriginalBuildingUpdateBlocker::None&&messages.count==1&&messages.notices.size()==1,"next ordinary part does not repeat the committed annual demand");
    auto full=annual;full.trade->partners[1].relationTimers.resize(22);OriginalBuildingScenarioMessages capacity;capacity.count=140;
    need(originalBuildingCycleTailWithAnnualStep(full,offers,&capacity,uint8_t(2))==OriginalBuildingUpdateBlocker::None&&capacity.count==140&&capacity.notices.empty(),"existing full-message-pool success semantics remain unchanged");
    // C1 retains its own fields only; the fixture's other owners are copied.
    // No new clock/annual-message schema, source, trigger or scheduler is added.
    auto original=tailFixture();auto checkpoint=captureNativeCityContext(original);std::string error;
    const auto bytes=encodeNativeCityContext(checkpoint,error);need(bool(bytes),"existing C1 owned-state checkpoint encodes");
    const auto decoded=decodeNativeCityContext(*bytes,error);need(bool(decoded),"existing C1 checkpoint decodes");
    auto reloaded=original;reloaded.cycleTail->smoothedHealth=0;applyNativeCityContext(*decoded,reloaded);
    need(tailSnapshot(reloaded)==tailSnapshot(original),"restoring C1 state leaves schedule and Labor with their existing owners");
    need(originalBuildingCycleTailStep(original)==OriginalBuildingUpdateBlocker::None&&originalBuildingCycleTailStep(reloaded)==OriginalBuildingUpdateBlocker::None&&tailSnapshot(original)==tailSnapshot(reloaded),"restored C1 fields yield identical supported next-tail state");
    auto blocked=tailFixture();blocked.monthPart=5;const auto blockedBytes=encodeNativeCityContext(captureNativeCityContext(blocked),error);
    need(originalBuildingCycleTailStep(blocked)==OriginalBuildingUpdateBlocker::MonthlyCity&&encodeNativeCityContext(captureNativeCityContext(blocked),error)==blockedBytes,"failed tail does not change persisted C1 bytes");
    need(!decodeNativeCityContext(bytes->substr(0,bytes->size()/2),error),"truncated existing C1 data stays refused; no implicit repair");
}
void authored(){
    {
        OriginalBuildingUpdateContext annual(OriginalBuildingUpdateScope::AdmittedTicks,true);
        annual.trade.emplace();annual.statistics.emplace();annual.statistics->currentFaction=3;annual.monthlyServices=true;
        auto& own=annual.trade->partners[3];own.alive=1;own.faction=3;
        auto& ally=annual.trade->partners[5];ally.alive=1;ally.faction=5;ally.kind=2;ally.enabled=1;
        ally.relations[3].state=2;own.relations[5].state=2;
        need(originalBuildingAnnualDiplomacyStep(annual)==OriginalBuildingUpdateBlocker::None&&ally.relations[3].annualFlag==1,
            "annual alliance state70 with war78 zero sets the first offer flag5D");
        need(originalBuildingAnnualDiplomacyStep(annual)==OriginalBuildingUpdateBlocker::MonthlyCity&&ally.relations[3].annualFlag==1,
            "annual repeated positive offer refuses its scenario service without changing the flag");
        ally.blocked=4;need(originalBuildingAnnualDiplomacyStep(annual)==OriginalBuildingUpdateBlocker::None&&ally.relations[3].annualFlag==0,
            "annual blocked partner clears offer flag before any war branch");
        ally.blocked=0;ally.relations[3].state=0;own.relations[5].state=0;ally.relations[3].annualFlag=1;
        need(originalBuildingAnnualDiplomacyStep(annual)==OriginalBuildingUpdateBlocker::None&&ally.relations[3].annualFlag==0,
            "annual no alliance clears stale flag with complete forward and reverse relations");
        own.relations[5].state=2;need(originalBuildingAnnualDiplomacyStep(annual)==OriginalBuildingUpdateBlocker::None&&ally.relations[3].annualFlag==1,
            "annual reverse alliance also qualifies through444160");
        annual.monthlyServices.reset();need(originalBuildingAnnualDiplomacyStep(annual)==OriginalBuildingUpdateBlocker::MonthlyCity,
            "annual service requires explicit opt in");
        annual.monthlyServices=true;annual.month=1;annual.elapsedYears= -2050;annual.scenarioStartYears= -2050;
        OriginalBuildingScenarioMessages messages;messages.records[0][113]=0xa5;messages.records[0][260]=0x6c;
        const OriginalBuildingAnnualOffer demand{100,29};
        need(originalBuildingAnnualMessage(annual,messages,demand,5,std::nullopt)==OriginalBuildingUpdateBlocker::MonthlyCity&&messages.count==0,
            "annual positive demand requires the caller response mode before allocating or publishing notification");
        need(originalBuildingAnnualMessage(annual,messages,demand,5,2)==OriginalBuildingUpdateBlocker::None&&messages.count==1&&
            messages.records[0][2]==29&&messages.records[0][3]==1&&detail::i16(messages.records[0].data()+4)==29&&
            detail::i16(messages.records[0].data()+12)==100&&detail::i16(messages.records[0].data()+28)==5&&
            detail::u32(messages.records[0].data()+40)==0x100&&messages.records[0][50]==1&&messages.notificationPending&&
            detail::i16(messages.records[0].data()+48)==14&&messages.notices.size()==1&&messages.notices[0].title==558&&
            messages.notices[0].dialogFlags==5&&annual.notificationEffects==std::vector<unsigned>{225},
            "annual demand constructs resource amount faction and pending notification with response14 and no automatic callback");
        need(messages.records[0][113]==0xa5&&messages.records[0][260]==0x6c,
            "annual message reset retains unrequested padding and untouched record extension");
        messages.count=140;need(originalBuildingAnnualMessage(annual,messages,demand,5,2)==OriginalBuildingUpdateBlocker::None&&messages.count==140,
            "annual allocator capacity140 exits without manufacturing another message");
        messages.count=1;messages.records[0][50]=0;messages.records[0][40]=0x10;
        need(originalBuildingAnnualMessage(annual,messages,demand,5,2)==OriginalBuildingUpdateBlocker::MonthlyCity&&messages.count==1,
            "annual recyclable record refuses at the unclosed age and UI predicates");
        std::vector<uint8_t> wire(2000);const size_t side=100;
        const auto word=[&](size_t at,unsigned value){wire[at]=uint8_t(value);wire[at+1]=uint8_t(value>>8);};
        word(side-28,6);word(side+1433,1);word(side+1456,3);word(side+1458,2);
        size_t at=side+1460;
        for(unsigned n=0;n<150;++n){wire.resize(at+263+4);word(at,9);at+=2;
            wire[at]=uint8_t(n);wire[at+113]=0x55;wire[at+113+8+10+56]=0x66;at+=261;}
        wire[at]=2;
        const auto parsed=originalBuildingSavedScenarioMessages(wire,side);
        need(parsed&&parsed->count==2&&parsed->records[149][0]==149&&parsed->records[0][113]==0&&
            parsed->records[0][114]==0x55&&parsed->records[0][214]==0x66,
            "scenario messages parse all150 record9 requests excluding padding and preserving faction array order");
        wire.pop_back();need(!originalBuildingSavedScenarioMessages(wire,side),"scenario messages reject truncated trailing count");
        auto orderedAnnual=annual;orderedAnnual.monthPart=1;orderedAnnual.cycle=7;orderedAnnual.populationTotal=0;orderedAnnual.labor.emplace();
        orderedAnnual.cycleTail.emplace();orderedAnnual.cycleTail->alternateMode=false;orderedAnnual.cycleTail->healthTarget=61;orderedAnnual.cycleTail->smoothedHealth=60;
        orderedAnnual.buildingInputs.emplace();orderedAnnual.buildings.emplace();orderedAnnual.trade->partners[5].relations[3].annualFlag=0;
        need(originalBuildingCycleTailWithAnnualStep(orderedAnnual)==OriginalBuildingUpdateBlocker::MonthlyCity&&
            orderedAnnual.cycleTail->smoothedHealth==60&&orderedAnnual.monthPart==1&&orderedAnnual.cycle==7&&
            orderedAnnual.trade->partners[5].relations[3].annualFlag==0,
            "annual tail with missing forecast publishes neither health nor clock prefix before diplomatic flags");
    }
    auto c=context(OriginalBuildingUpdateScope::ProductionPasses);
    for(uint16_t type:{uint16_t(194),uint16_t(196)})for(uint16_t flags:{uint16_t(0),uint16_t(1),uint16_t(32),uint16_t(33),uint16_t(0x4000),uint16_t(0x8021)}){
        const int delta=(flags&255)>32?(type==194?125:157):(flags&255)?(type==194?63:79):0;
        auto s=field(type,1990,flags);auto r=originalBuildingStateAfterUpdates(s,c,1);
        need(r.state&&originalBuildingCropProgress(r.state->inputs)==1990+delta,"literal per-type growth uses LOW byte32 threshold");
        need(originalBuildingCropProgress(s.inputs)==1990&&originalBuildingCropFlags(s.inputs)==flags,"input state is immutable");
        need(r.changedSelectorInputs==unsigned(delta!=0)&&r.selectorChanged==(delta!=0),"existing crop selector reports threshold image change");
    }
    for(uint16_t type:{uint16_t(194),uint16_t(196)}){
        auto s=field(type,9990,33);auto r=originalBuildingStateAfterUpdates(s,c,100);
        need(r.state&&originalBuildingCropProgress(r.state->inputs)==10000,"production clamps at10000 for repeated passes");
        c.month=10;r=originalBuildingStateAfterUpdates(field(type,0,33),c,1);
        need(r.state&&originalBuildingCropProgress(r.state->inputs)==(type==194?125:0),"millet final harvest month10 differs from hemp");c.month=7;
        auto delay=field(type,100,33);delay.inputs.industry->at(95)=2;
        auto once=originalBuildingStateAfterUpdates(delay,c,1);need(once.state&&originalBuildingCropProgress(once.state->inputs)==100&&once.state->inputs.industry->at(95)==1,"delay decrements before all growth and disabled writes");
        c.typeDisabled=true;once=originalBuildingStateAfterUpdates(delay,c,2);need(once.state&&originalBuildingCropProgress(once.state->inputs)==100,"last delay pass still skips disabled reset");
        once=originalBuildingStateAfterUpdates(delay,c,3);need(once.state&&originalBuildingCropProgress(once.state->inputs)==0,"disabled crop reset after delay expires");c.typeDisabled=false;
        c.terrainFlags=0x108;r=originalBuildingStateAfterUpdates(field(type,500,33),c,10);need(r.state&&originalBuildingCropProgress(r.state->inputs)==500,"terrain100 skips crop production");c.terrainFlags=8;
        r=originalBuildingStateAfterUpdates(field(type,32760,33),c,1);need(r.state&&originalBuildingCropProgress(r.state->inputs)==(type==194?-32651:-32619),"x86 word addition wraps BEFORE signed clamp");
        auto full=originalBuildingStateAfterUpdates(field(type,0,33),c,10),split=originalBuildingStateAfterUpdates(field(type,0,33),c,4);
        auto rest=originalBuildingStateAfterUpdates(*split.state,*split.context,6);need(full.state&&rest.state&&full.state->inputs.industry==rest.state->inputs.industry,"split horizon equals repeated production");
    }
    for(uint16_t flags:{uint16_t(0),uint16_t(1),uint16_t(32),uint16_t(33),uint16_t(0x7fff),uint16_t(0xffff)}){
        auto s=field(194,0,flags);originalBuildingDecayCropFlags(s.inputs);
        const unsigned low=flags&255;need(originalBuildingCropFlags(s.inputs)==((flags&0xc000)|(low?low-1:0)),"crop month-part decay preserves C000 and drops middle bits");
    }
    auto house=field();house.building.type=5;house.inputs.industry.reset();house.inputs.house.emplace();house.inputs.house->fill(3);
    auto hc=context(OriginalBuildingUpdateScope::HouseServiceDecayPasses);auto h=originalBuildingStateAfterUpdates(house,hc,10);
    need(h.state&&h.serviceDecayCalls==10,"house service operation horizon closes");
    for(unsigned offset=4;offset<=64;++offset){const bool dec=offset==41||offset==42||offset==43||offset==44||offset==45||offset==49||offset==50||offset==51||(offset>=13&&offset<=16);
        need(h.state->inputs.house->at(offset-4)==(dec?0:3),"house literal serializer bytes decay and saturate; adjacent fields stay intact");}
    c=context(OriginalBuildingUpdateScope::AdmittedTicks);auto s=field();c.phase=20;
    auto r=originalBuildingStateAfterUpdates(s,c,1);need(r.state&&r.productionCalls==1&&*r.context->phase==21,"phase20 production precedes next admitted figure pass");
    r=originalBuildingStateAfterUpdates(s,c,2);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::RoadRefresh,"phase21 refuses atomically, no production prefix");
    c.phase=31;c.monthPart=0;c.month=8;r=originalBuildingStateAfterUpdates(s,c,1);need(r.state&&(originalBuildingCropFlags(r.state->inputs)&0x8000)&&r.virtual20Calls==1,"phase31 hemp harvest flag is not production growth");
    c.scope=OriginalBuildingUpdateScope::ProductionPasses;s.inputs.output=0;r=originalBuildingStateAfterUpdates(s,c,1);
    need(r.state&&(originalBuildingCropFlags(r.state->inputs)&0x8000)&&originalBuildingCropProgress(r.state->inputs)==0,"production season prepass precedes even zero-output crop gate");
    c=context(OriginalBuildingUpdateScope::AdmittedTicks);c.phase=31;auto wall=field();wall.building.type=231;wall.inputs.industry.reset();
    r=originalBuildingStateAfterUpdates(wall,c,1);need(r.state&&r.virtual20Calls==1&&r.changedSelectorInputs==0,"wall virtual20 is cited false no-op, no crop growth substituted");
    c.phase=50;c.monthPart=1;s=field(194,500,0xc021);r=originalBuildingStateAfterUpdates(s,c,1);
    need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::MonthlyCity,"ordinary month-part wrap refuses unported4AC650 city work");
    c.monthPart=15;r=originalBuildingStateAfterUpdates(s,c,1);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::MonthlyCity,"monthly work cannot masquerade as a closed wrap");
    c=context(OriginalBuildingUpdateScope::ProductionPasses);c.typeDisabled.reset();r=originalBuildingStateAfterUpdates(s,c,1);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::MissingContext,"missing explicit disabled input refuses");
    c=context(OriginalBuildingUpdateScope::ProductionPasses);c.isolateBuildingWrites=false;r=originalBuildingStateAfterUpdates(s,c,1);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::ExternalWrites,"intervening city/figure mutation premise required");
    c.isolateBuildingWrites=true;r=originalBuildingStateAfterUpdates(s,c,10001);need(!r.state,"bounded operation budget");
    auto inconsistent=s;inconsistent.building.imageInputs->cropWord4=9999;r=originalBuildingStateAfterUpdates(inconsistent,c,1);
    need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::InvalidState,"contradictory raw and selector progress refuses");
    // Literal read-order offsets with deliberately contradictory decoys.
    std::vector<uint8_t> globals(1986237+128,0);const size_t terrain=128,side=200;globals[0]=19;
    auto put32=[&](size_t at,uint32_t value){for(unsigned j=0;j<4;++j)globals.at(at+j)=uint8_t(value>>(8*j));};
    put32(terrain+1299328,19);put32(terrain+1299332,1);put32(terrain+1299336,113);put32(side-176,8);
    put32(terrain+1291328,45);put32(terrain+1291332,15);put32(terrain+1291336,999);
    put32(terrain,8);globals[terrain+1985968+194]=1;
    auto gc=originalBuildingSavedUpdateContext(globals,terrain,side,field().building,OriginalBuildingUpdateScope::ProductionPasses,true);
    need(gc&&gc->phase==19&&gc->monthPart==1&&gc->cycle==113&&gc->month==8&&gc->typeDisabled==true&&gc->terrainFlags==8,"literal corrected SAV19 read-order offsets drive context");
    globals.resize(terrain+1985968+268);need(!originalBuildingSavedUpdateContext(globals,terrain,side,field().building,OriginalBuildingUpdateScope::ProductionPasses,true),"missing final disabled-table byte refuses context");
    // Explicit parser fixture: common5 fields must not use neighbouring bytes.
    OriginalBuildingProfile p;p.status=OriginalBuildingStatus::Complete;p.parsedRecords=4000;p.liveTypeCounts[194]=1;p.slotTypes[7]=194;
    auto f=field().building;f.recordOffset=0;p.footprints.push_back(f);
    std::vector<uint8_t> bytes(2+182+140);bytes[0]=1;bytes[1]=128;auto w=[&](size_t at,uint16_t v){bytes.at(at)=uint8_t(v);bytes.at(at+1)=uint8_t(v>>8);};
    w(2,5);w(18,194);bytes[4]=1;bytes[66]=253;bytes[67]=19;bytes[131]=9;bytes[132]=201;w(2+162,7);
    w(184,1);w(186,1);w(188,4321);w(314,1);w(316,1);w(322,1);bytes[184+99]=17;w(184+106,0x8021);
    auto parsed=originalBuildingUpdateInputs(bytes,p);need(parsed&&parsed->size()==1&&parsed->at(7).output==19&&parsed->at(7).identity==7,"common5 exact wire65/162 parsed independently");
    bytes[2+49]=17;bytes[2+50]=18;bytes[2+89]=99;bytes[2+91]=19;w(2+41,123);w(2+43,124);w(2+47,125);
    parsed=originalBuildingUpdateInputs(bytes,p);need(parsed&&parsed->at(7).spawnCounter==17&&parsed->at(7).spawnAux==18&&parsed->at(7).roadFailure==19&&parsed->at(7).primaryFigure==123&&parsed->at(7).word30==124&&parsed->at(7).word34==125,"hook common5 literal bytes avoid adjacent policy and road fields");
    w(2+58,0xfffd);bytes[2+3]=4;bytes[2+51]=6;bytes[2+7]=1;bytes[2+84]=0x80;bytes[2+86]=9;bytes[2+85]=99;
    parsed=originalBuildingUpdateInputs(bytes,p);need(parsed&&parsed->at(7).stock==-3&&parsed->at(7).faction==4&&parsed->at(7).direction==6&&parsed->at(7).houseInUse==1&&parsed->at(7).desirability==-128&&parsed->at(7).desirabilityBonus==9,"additive common5 stock faction direction accepting and signed desirability avoid neighboring decoys");
    need(originalBuildingCropProgress(parsed->at(7))==4321&&originalBuildingCropFlags(parsed->at(7))==0x8021&&parsed->at(7).industry->at(95)==17,"industrial1 byte mapping exact tail4/99/106");
    bytes.pop_back();need(!originalBuildingUpdateInputs(bytes,p),"late crop tail truncation refuses all update inputs");
    for(uint16_t type:{uint16_t(33),uint16_t(35),uint16_t(43)}){
        auto industrial=field();industrial.building.type=type;industrial.inputs.industry.reset();industrial.production.emplace();
        auto& ip=*industrial.production;ip.operational=1;ip.workers=8;ip.enabled=1;
        auto pc=context(OriginalBuildingUpdateScope::ProductionPasses);pc.unavailable=0;pc.requiredWorkers=8;pc.festivalProductionBoost=false;
        auto pr=originalBuildingStateAfterUpdates(industrial,pc,1);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==10,"industry workforce100 percent produces10 with ordinary divisor10");
        pc.festivalProductionBoost=true;pr=originalBuildingStateAfterUpdates(industrial,pc,1);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==20,"qualified festival doubles production via divisor5");
        pc.festivalProductionBoost=false;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==(type==43?320:160),"industry caps differ kiln320 versus raw160");
        ip.industry[0x63-4]=2;ip.industry[0x5f-4]=3;pc.requiredWorkers.reset();pc.festivalProductionBoost.reset();
        pr=originalBuildingStateAfterUpdates(industrial,pc,2);
        need(pr.state&&pr.state->production->industry[0x63-4]==0&&pr.state->production->industry[0x5f-4]==3,"industry delay precedes model festival and auxiliary timer");
        pr=originalBuildingStateAfterUpdates(industrial,pc,3);need(!pr.state&&pr.blocker==OriginalBuildingUpdateBlocker::MissingContext,"industry positive growth requires model festival provenance");
        ip.primaryResource=18;ip.primaryStock=99;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"input stock99 gate precedes delay");
        ip.primaryStock=100;ip.secondaryResource=1;ip.secondaryStock=99;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"secondary stock99 gate is independent");
        ip.secondaryStock=100;pc.unavailable=1;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"unavailable output precedes all input and delay writes");
        pc.unavailable.reset();ip.workers=0;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"zero workers closes without unavailable model or festival guesses");
        pc.unavailable=0;pc.requiredWorkers=8;pc.festivalProductionBoost=false;ip.workers=8;ip.industry.fill(0);
        ip.industry[0]=0xf8;ip.industry[1]=0x7f;pr=originalBuildingStateAfterUpdates(industrial,pc,1);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==-32766,"industry word wraps before signed cap");
    }
    //Independent literal perimeter order and ranked-network precedence.
    OriginalBuildingRoadInputs road;road.terrain.resize(51984);road.distance.resize(51984);road.owners.resize(51984);road.networks.resize(51984);
    auto rf=field().building;rf.x=20;rf.y=20;rf.size=2;rf.imageInputs->gridIndex=4580;
    road.terrain[4352]=road.terrain[4582]=0x40;road.distance[4352]=2;road.distance[4582]=50;
    road.networks[4352]=1;road.networks[4582]=2;road.rankedNetworks[0]=2;road.rankedNetworks[1]=1;
    auto access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==50&&access->x==22&&access->y==20&&access->network==2,"ranked road network beats nearest distance");
    road.rankedNetworks[0]=1;road.rankedNetworks[1]=2;access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==2&&access->x==20&&access->y==19,"rank ordering is caller state not fitted nearest road");
    road.networks[4582]=1;road.distance[4582]=1;access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==2,"equal network rank preserves clockwise first candidate");
    road.terrain.assign(51984,0);access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==1&&access->x==22,"fallback chooses nearest positive distance without terrain or owner gates");
    road.owners[4352]=7;access=originalBuildingRoadAccess(rf,road);need(!access,"unknown owner virtual D0 refuses even before fallback");
    road.ownerTypes.emplace();road.ownerTypes->at(7)=231;access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==1,"cited wall owner rejects ranked candidate while fallback remains independent");
    road.networks.assign(51984,0);road.networks[0]=2;road.networks[1]=1;
    auto ranks=originalBuildingRankedRoadNetworks(road.networks);need(ranks&&ranks->at(0)==1&&ranks->at(1)==2,"equal network sizes retain ascending label order");
    std::vector<uint16_t> movement(51984);movement[10000]=movement[10001]=4;movement[10002]=0x40;movement[10229]=0x1000;
    auto distances=originalBuildingRoadDistances(movement,10000);
    need(distances&&distances->at(10000)==1&&distances->at(10001)==2&&distances->at(10002)==3&&distances->at(10229)==0,
        "road flood uses B7C including40 but excludes1000");
    need(!originalBuildingRoadDistances(movement,0),"road flood refuses original out-of-grid access premise");
    auto capacityContext=context(OriginalBuildingUpdateScope::AdmittedTicks);capacityContext.phase=22;capacityContext.houses.emplace();capacityContext.houseModelCapacity=std::map<int,int>{{0,3},{1,8},{2,15}};
    capacityContext.houses->emplace(7,OriginalBuildingHouseCapacityInputs{3,1,6,99,1,2,0});
    capacityContext.houses->emplace(8,OriginalBuildingHouseCapacityInputs{11,1,12,99,1,10,0});
    capacityContext.houses->emplace(9,OriginalBuildingHouseCapacityInputs{5,1,4,99,1,8,1});
    capacityContext.houses->emplace(10,OriginalBuildingHouseCapacityInputs{5,1,5,99,0,8,0});
    auto capacity=originalBuildingStateAfterUpdates(field(),capacityContext,1);
    need(capacity.state&&capacity.context->cityHouseCapacity==27&&capacity.context->cityHouseFree==5,"capacity totals include model11 next row and labor-disabled resident override");
    need(capacity.context->houses->at(7).free==2&&capacity.context->houses->at(8).free==3&&capacity.context->houses->at(9).free==0&&capacity.context->houses->at(10).free==0,"capacity resets disconnected free space and preserves signed field arithmetic");
    need(capacity.context->houses->at(7).peak==6&&capacity.context->houses->at(9).peak==8,"capacity peak raises only below residents");
    auto rc=context(OriginalBuildingUpdateScope::AdmittedTicks);rc.phase=21;rc.roadAccess=OriginalBuildingRoadAccess{12,3,4,uint8_t(2)};rc.roadRefreshOrdinaryExit=true;
    auto rr=originalBuildingStateAfterUpdates(field(),rc,1);need(rr.state&&rr.state->roadDistance==12&&rr.state->roadX==3&&rr.state->roadY==4&&rr.state->roadNetwork==2,"phase21 publishes cited road fields on verified ordinary city exit");
    rc.roadRefreshOrdinaryExit=false;rr=originalBuildingStateAfterUpdates(field(),rc,1);need(!rr.state&&rr.blocker==OriginalBuildingUpdateBlocker::RoadRefresh,"road repair branch refuses atomically");
    rc.roadRefreshOrdinaryExit=true;rc.roadAccess.reset();auto inactive=field();inactive.inputs.alive=2;
    rr=originalBuildingStateAfterUpdates(inactive,rc,1);need(rr.state&&!rr.state->roadDistance,"road phase skips nonlive record before per-record road input");
    OriginalBuildingCityEmployment city;city.available=7;city.shortageNotice=false;city.unemploymentNotice=false;
    city.rows[0]={10,99,4,99,5};city.rows[1]={10,99,4,99,5};city.rows[9]={99,98,97,96,95};
    auto account=originalBuildingCityEmploymentStep(city);need(account&&account->state.rows[0].allocated==4&&account->state.rows[1].allocated==3&&account->state.rows[0].preferredAllocated==4&&account->state.rows[1].preferredAllocated==3,"accounting preferred shortage distributes ordered integer remainder");
    need(account->state.rows[9].allocated==98&&account->state.rows[9].priority==95,"accounting touches nine of ten saved category rows");
    city.available=12;account=originalBuildingCityEmploymentStep(city);need(account&&account->state.rows[0].allocated==6&&account->state.rows[1].allocated==6&&account->state.shortage==8,"accounting ordinary remainder follows preferred allocation");
    city.available=30;city.shortageNotice=true;city.unemploymentNotice=true;account=originalBuildingCityEmploymentStep(city);
    need(account&&account->state.employed==20&&account->state.unemployed==10&&account->state.unemploymentPercent==33&&account->unemploymentNotification&&!account->shortageNotification,"accounting excess labor preserves exact notification branch order");
    city.shortageNotice.reset();need(!originalBuildingCityEmploymentStep(city),"unserialized notification flags are required inputs");
    city.shortageNotice=false;city.rows[0].priority=9;account=originalBuildingCityEmploymentStep(city);need(account&&account->state.rows[0].priority==3,"out-of-range positive priority normalizes to3 before demand test");
    //Independent serializer positions, with adjacent contradictory values.
    std::vector<uint8_t> population(13000);const size_t anchor=12287;population[0]=7;population[anchor-28]=6;
    const auto put=[&](size_t at,uint32_t value){for(unsigned j=0;j<4;++j)population.at(at+j)=uint8_t(value>>(8*j));};
    put(10190,77);put(10194,11);put(10198,12);put(10202,13);put(10206,14);put(10210,15);
    put(10394,21);put(10398,22);put(10402,23);put(10410,24);
    auto restored=originalBuildingSavedCityEmployment(population,anchor);
    need(restored&&restored->available==77&&restored->rows[0].demand==11&&restored->rows[0].allocated==12&&
        restored->rows[0].preferred==13&&restored->rows[0].preferredAllocated==14&&restored->rows[0].priority==15&&
        restored->employed==21&&restored->unemployed==22&&restored->unemploymentPercent==23&&restored->shortage==24,"accounting nested Population7 literal offsets preserve five distinct row fields");
    put(9904+6,0xfffffff9);need(originalBuildingSavedUnavailable(population,anchor,3)==-7,"unavailable uses signed word at Population7 wire9904");
    std::vector<uint8_t> scenario(900);scenario[0]=6;scenario[794]=11;scenario[796]=12;scenario[798]=13;scenario[800]=14;
    need(originalBuildingSavedRoadEndpoints(scenario,28)==std::array<int16_t,4>{11,12,13,14},"road scenario wire794 through800 is not runtime offset31C");
    std::vector<uint8_t> festival(3355+10*6+2+17+51984+83);const size_t festivalAt=3355+60+2+17+51984;
    for(unsigned n=0;n<10;++n)festival[3355+6*n]=1;festival[3355+60]=1;festival[festivalAt]=4;festival[festivalAt+2]=1;
    festival[festivalAt+66]=1;festival[festivalAt+67]=3;festival[festivalAt+71]=2;
    need(originalBuildingSavedProductionBoost(festival,35)==true&&originalBuildingSavedProductionBoost(festival,43)==true&&originalBuildingSavedProductionBoost(festival,33)==false,"festival production deity2 accepts clay and kiln but never Hunter");
    festival[festivalAt+71]=4;need(originalBuildingSavedProductionBoost(festival,35)==false,"Well water override deity4 cannot be substituted for production deity2");
    need(originalBuildingSavedCropFestival(festival)==true,"crop festival restores saved enabled mode3 deity4 rather than production deity2");
    festival[festivalAt+66]=0;need(originalBuildingSavedCropFestival(festival)==false,"disabled saved Festival4 gates crop ranking bonus");festival[festivalAt+66]=1;
    festival[festivalAt+67]=2;need(originalBuildingSavedCropFestival(festival)==false,"saved Festival4 mode2 does not enable crop ranking bonus");festival[festivalAt+67]=3;
    festival.resize(festivalAt+70);need(!originalBuildingSavedProductionBoost(festival,35),"truncated Festival4 does not invent false boost");
    need(!originalBuildingSavedCropFestival(festival),"truncated Festival4 does not invent false crop ranking bonus");
    //Source6 version6 omits8A..8B and96..97; poison wire148/149.
    std::vector<uint8_t> formationBytes(original_save19::recordsEnd+8);for(unsigned n=0;n<100;++n)formationBytes[original_save19::records+175*n]=6;
    const size_t group=original_save19::records+175*35;formationBytes[group+148]=0xa5;formationBytes[group+149]=0x5a;
    formationBytes[group+150]=0x78;formationBytes[group+151]=0x56;formationBytes[group+152]=0x34;formationBytes[group+153]=0x12;
    const auto groups=originalBuildingSavedFormations(formationBytes,0);
    need(groups&&groups->at(35).lifecycleOwner==0x12345678,"formation lifecycle owner uses wire150 after both omitted padding pairs");
    OriginalBuildingUpdateContext ic=context(OriginalBuildingUpdateScope::AdmittedTicks);ic.phase=27;ic.cropBase=2290;ic.activeSide=3;
    ic.terrain=std::vector<uint32_t>(51984);ic.irrigation.emplace();auto& ir=*ic.irrigation;ir.image.assign(51984,100);ir.subTile.resize(51984);ir.owners.resize(51984);ir.baseImage=100;
    for(size_t at:{2290u,2291u,2518u,2519u,2746u,2747u})ic.terrain->at(at)=0x800;
    ir.subTile[2291]=0x80;ir.owners[2518]=7;ir.ownerTypes[7]=126;ic.terrain->at(2519)|=4;ir.image[2746]=118;ir.image[2747]=117;
    auto irrigation=originalBuildingStateAfterUpdates(field(),ic,1);
    need(irrigation.state&&irrigation.context->irrigationCalls==1&&irrigation.context->irrigation->image[2290]==118&&irrigation.context->irrigation->image[2747]==135,"irrigation dry pass adds18 below base18 threshold");
    need(irrigation.context->irrigation->image[2291]==100&&irrigation.context->irrigation->image[2518]==100&&irrigation.context->irrigation->image[2519]==100&&irrigation.context->irrigation->image[2746]==118,"irrigation dry pass honors subtile80 owner and blocked gates");
    ic.terrain->assign(51984,0);ir.image.assign(51984,118);ir.owners.assign(51984,0);ir.subTile.assign(51984,0);
    ic.terrain->at(2746)=ic.terrain->at(2747)=ic.terrain->at(2748)=0x800;ir.owners[2747]=7;
    ir.sources.push_back({2290,1,0});irrigation=originalBuildingStateAfterUpdates(field(),ic,1);
    need(irrigation.state&&irrigation.context->irrigation->image[2746]==100&&irrigation.context->irrigation->image[2747]==118&&irrigation.context->irrigation->image[2748]==100,"staffed irrigation flood traverses Roadblock without rewriting its image");
    ir.ownerTypes[7]=231;irrigation=originalBuildingStateAfterUpdates(field(),ic,1);
    need(irrigation.state&&irrigation.context->irrigation->image[2748]==118,"irrigation rejects nonRoadblock owner before continuing flood");
    auto wc=context(OriginalBuildingUpdateScope::AdmittedTicks);wc.phase=28;wc.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,house.inputs}};
    wc.buildingInputs->at(7).house->fill(0);wc.buildingInputs->at(7).house->at(45)=1;wc.buildingInputs->at(7).house->at(47)=1;
    house.inputs=wc.buildingInputs->at(7);auto water=originalBuildingStateAfterUpdates(house,wc,1);
    need(water.state&&water.state->inputs.houseWater==2&&water.state->inputs.houseService==0,"water phase carries common field writes into returned record");
    auto formContext=context(OriginalBuildingUpdateScope::AdmittedTicks);formContext.figures=std::vector<OriginalFigure>(2000);for(unsigned n=0;n<2000;++n)formContext.figures->at(n).slot=n;
    formContext.formations.emplace();formContext.formations->at(35).alive=1;formContext.formations->at(35).skipLifecycle=3;formContext.formations->at(35).kind=76;
    auto& animal=formContext.figures->at(9);animal.alive=1;animal.type=76;animal.formation=35;animal.damage=3;
    formContext.figureModels=std::map<unsigned,std::array<int32_t,18>>{};formContext.figureModels->operator[](76)[1]=17;formContext.figureStationary=std::map<size_t,uint8_t>{{9,1}};
    const auto gathered=originalBuildingFormationStep(*formContext.formations,*formContext.figures,*formContext.figureModels,*formContext.figureStationary);
    need(gathered&&gathered->formations[35].count==1&&gathered->formations[35].members[0]==9&&gathered->formations[35].damage==3&&gathered->formations[35].strength==17&&gathered->figureOrdinals.at(9)==0,"formation gather uses actual figure stationary byte and model column1");
    formContext.formations->at(35).skipLifecycle=0;need(!originalBuildingFormationStep(*formContext.formations,*formContext.figures,*formContext.figureModels,*formContext.figureStationary),"positive formation lifecycle refuses despite nonempty supplied pool");
    auto military=formContext;military.phase=5;military.formations->at(35).skipLifecycle=3;military.figures->at(8).alive=1;military.figures->at(8).type=64;military.figures->at(8).damage=2;
    auto militaryResult=originalBuildingStateAfterUpdates(field(),military,1);
    need(!militaryResult.state&&militaryResult.refusedPhase==5,"formation phase refuses independent military damage writes outside animal domain");
    auto religionContext=context(OriginalBuildingUpdateScope::AdmittedTicks);religionContext.phase=18;religionContext.populationTotal=500;
    religionContext.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,field().inputs}};
    religionContext.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,field().building}};religionContext.labor.emplace();religionContext.labor->difficulty=2;
    religionContext.statistics.emplace();religionContext.statistics->religionBonusEnabled=false;religionContext.religion.emplace();
    auto& religion=*religionContext.religion;religion.previousRating=73;religion.selected=3;religion.worship[1].enabled=true;religion.scenarioEnabled[3]=religion.scenarioEnabled[4]=1;
    religion.worship[1].entries={{{0,150,2,3,50},{0,150,1,4,20},{0,150,2,5,3}}};
    auto worship=originalBuildingStateAfterUpdates(field(),religionContext,1);
    need(worship.state&&worship.context->religion->worship[1].entries[0].value==44&&worship.context->religion->worship[1].entries[1].value==19&&worship.context->religion->worship[1].entries[2].value==0,"religion difficulty decrement precedes aggregate and disabled status reset");
    need(worship.context->religion->worship[1].minimum==0&&worship.context->religion->worship[1].maximum==44&&worship.context->religion->worship[1].average==31,"religion min max includes disabled entry while average excludes it");
    need(worship.context->religion->previousRating==73&&worship.context->religion->selected==3,"ordinary religion mode preserves both process scratch inputs");
    religionContext.labor->difficulty=-1;worship=originalBuildingStateAfterUpdates(field(),religionContext,1);
    need(!worship.state&&!worship.context&&worship.blocker==OriginalBuildingUpdateBlocker::MissingContext,"active deity needs explicit difficulty and refuses without prefix state");
    std::vector<uint8_t> religionBytes(56000);std::copy(festival.begin(),festival.end(),religionBytes.begin());
    //Rebuild the complete Festival4 followed by Religion1/child3/entry2.
    religionBytes[festivalAt+71]=2;size_t religionAt=festivalAt+83;religionBytes[religionAt++]=1;++religionAt;
    const auto write32=[&](size_t at,uint32_t value){for(unsigned k=0;k<4;++k)religionBytes.at(at+k)=uint8_t(value>>(8*k));};
    for(unsigned n=0;n<4;++n){religionBytes[religionAt]=3;religionAt+=2;for(unsigned k=0;k<3;++k){religionBytes[religionAt]=2;religionAt+=2;
        write32(religionAt,0);write32(religionAt+4,150);write32(religionAt+8,2);write32(religionAt+12,3*n+k);write32(religionAt+16,99);religionAt+=20;}}
    const size_t religionSide=55000;religionBytes[religionSide-28]=6;religionBytes[religionSide-28+6]=1;religionBytes[religionSide-28+10]=1;
    const auto savedReligion=originalBuildingSavedReligion(religionBytes,religionSide);
    need(savedReligion&&savedReligion->worship[0].entries[0].status==2&&savedReligion->worship[0].entries[0].value==99&&savedReligion->worship[0].entries[1].status==0&&savedReligion->worship[0].entries[1].value==0&&savedReligion->worship[1].entries[0].status==0&&savedReligion->worship[1].entries[0].value==0,"religion load preserves status2 only for enabled worship and figure type");
    auto ac=context(OriginalBuildingUpdateScope::AdmittedTicks);ac.phase=1;ac.month=7;ac.formations.emplace();ac.figures=std::vector<OriginalFigure>(2000);
    for(unsigned n=0;n<2000;++n)ac.figures->at(n).slot=n;
    ac.figureModels.emplace();ac.figureStationary.emplace();ac.cityEmployment.emplace();ac.cityEmployment->available=10;
    ac.cityEmployment->rows[0]={6,0,0,0,3};ac.cityEmployment->shortageNotice=false;ac.cityEmployment->unemploymentNotice=false;
    auto taxHouse=house;taxHouse.inputs.house->fill(0);taxHouse.inputs.residents=10;taxHouse.inputs.byte52=1;taxHouse.inputs.model=2;
    taxHouse.inputs.house->at(38)=1;taxHouse.inputs.house->at(14)=50;taxHouse.building.imageInputs->word20=10;
    ac.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,taxHouse.inputs}};ac.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,taxHouse.building}};
    ac.houseTaxModel=std::map<int,int>{{2,4}};ac.scenarioFoodOverride=0;ac.populationTotal=10;ac.serviceCoverage=std::array<int32_t,11>{};
    ac.serviceCoverage->at(4)=73;ac.goods.emplace();ac.goodsRecords=std::vector<OriginalBuildingGoodsRecord>(1);ac.buildingLinks=std::map<size_t,OriginalFigureBuildingLink>{{7,{}}};
    ac.storageRoadStatus.emplace();ac.poolRoadSelected.emplace();ac.accounting.emplace();auto& ledger=*ac.accounting;
    ledger.taxRate=25;ledger.taxReceived=5;ledger.taxOther=7;ledger.wages=120;ledger.wagesPaid=20;ledger.incomeOther=30;ledger.expensesOther=9;ledger.treasuryStart=100;ledger.festivalCount=13;
    ledger.incomeReceived=99;ledger.expensesPaid=55;
    auto accounted=originalBuildingStateAfterUpdates(taxHouse,ac,1);
    need(accounted.state&&accounted.context->accountingCalls==1&&accounted.context->formationCalls==1,"accounting phase executes complete supplied service chain once");
    const auto& ledgerAfter=*accounted.context->accounting;
    need(ledgerAfter.taxableOrdinary==40&&ledgerAfter.taxableElite==0&&ledgerAfter.taxForecast==72&&ledgerAfter.wageForecast==56,"accounting independent tax and wage forecasts use month7 remaining6");
    need(ledgerAfter.income==42&&ledgerAfter.expenses==29&&ledgerAfter.profit==13&&ledgerAfter.actualProfit==44&&ledgerAfter.treasuryForecast==113&&ledgerAfter.festivalCapacity==9,"accounting income expense profit and festival cap preserve separate saved inputs");
    need(accounted.context->cityDerived->food==50&&accounted.context->cityDerived->foodConsumption==2&&accounted.context->cityDerived->foodMonths==25&&accounted.context->serviceCoverage->at(1)==100&&accounted.context->serviceCoverage->at(4)==73,"accounting food uses house tail18 and preserves untouched coverage columns");
    auto load=ac;load.phase=0;load.loadAccountingTransition=true;load.applyLoadRoadRefresh=false;
    auto loaded=originalBuildingStateAfterUpdates(taxHouse,load,1);auto continued=originalBuildingStateAfterUpdates(*loaded.state,*loaded.context,1);
    need(loaded.state&&loaded.context->loadAccountingApplied&&loaded.context->accountingCalls==1&&continued.state&&continued.context->accountingCalls==2,"explicit UI load accounting transition is applied once before scheduled phase1");
    auto noLoad=load;noLoad.loadAccountingTransition=false;auto unloaded=originalBuildingStateAfterUpdates(taxHouse,noLoad,1);
    need(unloaded.state&&unloaded.context->accountingCalls==0&&!unloaded.context->loadAccountingApplied,"false UI transition does not invent load accounting reachability");
    auto incomplete=load;incomplete.figureStationary.reset();auto rejected=originalBuildingStateAfterUpdates(taxHouse,incomplete,1);
    need(!rejected.state&&!rejected.context&&rejected.operations==0,"load accounting missing dependency refuses atomically");
    auto ec=ac;ec.phase=25;ec.employmentModel=OriginalBuildingEmployment{{194,3},{196,4}};ec.buildingUnavailable=std::map<size_t,bool>{{7,false},{8,false},{9,false}};
    ec.unemploymentTrend=0;ec.shortageNoticeCountdown=0;ec.elapsedYears=0;ec.scenarioStartYears=0;ec.cityEmployment->available=5;
    auto first=field(194);first.inputs.operational=100;first.inputs.enabled=1;
    auto second=field(196);second.building.slot=8;second.inputs.identity=8;second.inputs.operational=100;second.inputs.enabled=1;
    auto suppressed=second;suppressed.building.slot=9;suppressed.inputs.identity=9;suppressed.inputs.suppression=1;suppressed.inputs.workers=99;
    ec.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,first.inputs},{8,second.inputs},{9,suppressed.inputs}};
    ec.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,first.building},{8,second.building},{9,suppressed.building}};
    ec.buildingLinks=std::map<size_t,OriginalFigureBuildingLink>{{7,{}},{8,{}},{9,{}}};
    auto employed=originalBuildingStateAfterUpdates(first,ec,1);
    need(employed.state&&employed.state->inputs.workers==3&&employed.context->buildingInputs->at(8).workers==2&&employed.context->buildingInputs->at(9).workers==0,"employment distributes ordered integer remainder after proportional first pass");
    need(employed.context->cityEmployment->rows[0].demand==7&&employed.context->cityEmployment->shortage==2&&employed.context->buildingLinks->at(8).workers==2,"employment eligibility excludes suppression and synchronizes whole pool workers");
    auto lc=ac;lc.phase=23;lc.labor.emplace();lc.labor->sentiment=100;lc.labor->health=50;lc.labor->difficulty=0;lc.labor->wageBand=3;lc.labor->populationNotices=std::array<bool,10>{};
    lc.buildingInputs->at(7).distance=1;lc.buildingInputs->at(7).free=12;lc.buildingInputs->at(7).random=129;lc.populationTotal=10;lc.populationPeak=10;
    lc.roadEndpoints=std::array<int16_t,4>{3,4,5,6};lc.cropBase=229;lc.terrain=std::vector<uint32_t>(51984);lc.terrain->at(1144)=0x40;
    lc.figureCellHeads=std::vector<uint16_t>(51984);lc.figureServices.emplace();lc.figureServices->allocationSerial=37;lc.figureServices->randomFirst=1;lc.figureServices->randomSecond=2;
    lc.figureSpeechCounters=std::array<int32_t,359>{};lc.figureSpeechCounters->at(265)=7;lc.figureSpeechCounts=std::array<uint32_t,359>{};lc.figureSpeechCounts->at(265)=3;
    lc.freeFigureSlots={19,3};lc.crtRandom=123;lc.scenarioFigurePlacementCheck=false;lc.houseModelCapacity=std::map<int,int>{{2,22}};
    auto migrantHouse=taxHouse;migrantHouse.inputs=lc.buildingInputs->at(7);auto migrated=originalBuildingStateAfterUpdates(migrantHouse,lc,1);
    need(migrated.state&&migrated.context->allocatedFigures==std::vector<size_t>{19}&&migrated.context->freeFigureSlots==std::vector<size_t>{3}&&migrated.state->inputs.migrant==19,"migration uses supplied free queue and publishes reciprocal house link");
    const auto& immigrant=migrated.context->figures->at(19);
    need(immigrant.type==11&&immigrant.action==6&&immigrant.home==7&&immigrant.employmentAmount==6&&immigrant.counter==1&&immigrant.subtype==1&&immigrant.water==1&&immigrant.speed==0&&immigrant.allocationSerial==37,"immigrant constructor and caller writes preserve independent literal fields");
    need(migrated.context->labor->arrivals==6&&migrated.context->labor->unplaced==0&&migrated.context->labor->spacing==50&&migrated.state->inputs.residents==10&&migrated.context->figureCellHeads->at(1144)==19,"migration accounts attempted6 without changing residents before arrival");
    need(migrated.context->figureSpeechChoices.at(19)==1&&migrated.context->figureSpeechCounters->at(265)==8&&migrated.context->crtRandom==123&&migrated.context->crtRandomCalls==0,"migrant speech rotation advances saved counter without separate CRT sound call");
    auto chain=lc;chain.figures->at(5).alive=1;chain.figures->at(5).type=11;chain.figures->at(5).cell=1144;chain.figureCellHeads->at(1144)=5;
    auto chained=originalBuildingStateAfterUpdates(migrantHouse,chain,1);
    need(chained.state&&chained.context->figureCellHeads->at(1144)==5&&chained.context->figures->at(5).next==19&&chained.context->figures->at(19).next==0,"migrant registration appends to existing same-cell list");
    auto leaving=lc;leaving.labor->sentiment=25;leaving.labor->outRemainder=3;leaving.populationTotal=200;leaving.populationPeak=200;
    auto leftHouse=originalBuildingStateAfterUpdates(migrantHouse,leaving,1);
    need(leftHouse.state&&leftHouse.state->inputs.residents==4&&leftHouse.context->populationTotal==194&&leftHouse.context->labor->lastPopulationDelta==-6&&leftHouse.context->labor->departures==6&&
        leftHouse.context->figures->at(19).type==12&&leftHouse.context->figures->at(19).employmentAmount==6,"emigration stores signed last arrival and population before allocating type12");
    auto excess=lc;excess.phase=24;excess.buildingInputs->at(7).free=-3;excess.labor->excessDepartureTotal=7;excess.figureSpeechCounts->at(254)=4;excess.figureSpeechCounters->at(254)=6;
    auto excessHouse=migrantHouse;excessHouse.inputs=excess.buildingInputs->at(7);auto overflow=originalBuildingStateAfterUpdates(excessHouse,excess,1);
    need(overflow.state&&overflow.state->inputs.residents==7&&overflow.context->populationTotal==7&&overflow.context->labor->excessDepartureTotal==10&&
        overflow.context->figures->at(19).type==13&&overflow.context->figures->at(19).employmentAmount==3&&overflow.context->figureSpeechChoices.at(19)==2,"overflow departure increments separate accounting total only after type13 allocation");
    excess.freeFigureSlots.clear();overflow=originalBuildingStateAfterUpdates(excessHouse,excess,1);
    need(overflow.state&&overflow.state->inputs.residents==7&&overflow.context->populationTotal==10&&overflow.context->labor->excessDepartureTotal==7,"overflow allocation exhaustion still trims house but preserves global population and departure total");
    auto exhausted=lc;exhausted.freeFigureSlots.clear();exhausted.figureSpeechCounters.reset();auto attempted=originalBuildingStateAfterUpdates(migrantHouse,exhausted,1);
    need(attempted.state&&attempted.context->allocatedFigures.empty()&&attempted.context->labor->arrivals==12&&attempted.state->inputs.migrant==0&&attempted.context->figureServices->allocationSerial==37,"allocation exhaustion counts attempted migrants without consuming constructor inputs");
    auto splitLabor=originalBuildingStateAfterUpdates(*migrated.state,*migrated.context,1);auto wholeLabor=originalBuildingStateAfterUpdates(migrantHouse,lc,2);
    need(splitLabor.state&&wholeLabor.state&&splitLabor.state->inputs.workers==wholeLabor.state->inputs.workers&&splitLabor.context->freeFigureSlots==wholeLabor.context->freeFigureSlots&&splitLabor.context->labor->arrivals==wholeLabor.context->labor->arrivals,"migration split horizon retains global allocation and employment state");
    auto growing=lc;growing.labor->inDelay=1;growing.labor->statisticsPending=1;growing.labor->health=62;growing.labor->populationSum=15;growing.labor->sampleCount=1;growing.populationTotal=100;
    auto grown=originalBuildingStateAfterUpdates(migrantHouse,growing,1);
    need(grown.state&&grown.state->inputs.residents==12&&grown.state->inputs.free==10&&grown.context->populationTotal==102&&grown.context->labor->statisticsPending==0&&grown.context->labor->averagePopulation==58,"pending population statistics grows ordered ordinary house before sample average");
    auto broken=lc;broken.figureSpeechCounters.reset();auto absent=originalBuildingStateAfterUpdates(migrantHouse,broken,1);
    need(!absent.state&&!absent.context&&lc.freeFigureSlots==std::vector<size_t>({19,3})&&lc.figureServices->allocationSerial==37,"missing speech provenance refuses migration without consuming caller queue or RNG");
    auto storage=ac;storage.goodsRecords=std::vector<OriginalBuildingGoodsRecord>(2);storage.goodsRecords->at(1).alive=1;storage.goodsRecords->at(1).owner=8;
    OriginalFigureBuildingLink mill;mill.type=53;mill.alive=1;mill.storage=std::vector<OriginalStorageCompartment>{{1,300,1000},{2,200,1000},{30,90,1000}};mill.storagePolicy=0;
    storage.buildingLinks->emplace(8,mill);storage.buildingInputs->emplace(8,OriginalBuildingUpdateInputs{});storage.goods->policies.resize(1);storage.storageRoadStatus->emplace(8,2);storage.poolRoadSelected->emplace(8,true);
    storage.goods->warehouse[1]=999;storage.goods->warehouse[35]=77;
    need(originalBuildingStorageStep(storage)==OriginalBuildingUpdateBlocker::None&&storage.goods->warehouse[1]==300&&storage.goods->warehouse[2]==200&&storage.goods->warehouse[30]==90&&storage.goods->warehouse[35]==77&&storage.buildingInputs->at(8).available==2,"storage resets29 counters and preserves high resources while using availability status2");
    need(originalBuildingFoodStep(storage)==OriginalBuildingUpdateBlocker::None&&storage.cityDerived->food==550&&storage.buildingInputs->at(8).available==1&&storage.foodCalls==1,"food rescan overwrites Mill availability with adjacent-road bool and adds house plus nine foods");
    auto savedInputs=population;savedInputs.resize(14000);const auto saved32=[&](size_t at,uint32_t v){for(unsigned k=0;k<4;++k)savedInputs.at(at+k)=uint8_t(v>>(8*k));};
    saved32(anchor-180,1234);saved32(9770,71);saved32(9774,0xfffffffd);savedInputs[anchor-28+2]=0x34;savedInputs[anchor-28+3]=0x12;savedInputs[anchor-28+632]=2;savedInputs[anchor+1433]=1;saved32(anchor+1452,2);
    auto restoredCity=context(OriginalBuildingUpdateScope::AdmittedTicks);
    need(originalBuildingSavedCityContext(savedInputs,anchor,restoredCity)&&restoredCity.elapsedYears==1234&&restoredCity.scenarioStartYears==0x1234&&restoredCity.engineRegion==2,"saved city years and animal region use literal outer and Scenario6 wire offsets");
    need(restoredCity.labor->excessDepartureTotal==71&&restoredCity.labor->lastPopulationDelta==-3,"saved departure total and signed last arrival retain distinct adjacent Population7 words");
    auto availability=ec;availability.empireProduction=std::array<bool,36>{};availability.unavailableResources=std::array<int16_t,36>{};
    availability.buildings->at(7).type=192;availability.buildings->at(8).type=193;availability.buildings->at(9).type=194;
    availability.empireProduction->at(3)=true;
    need(originalBuildingUnavailableStep(availability)==OriginalBuildingUpdateBlocker::None&&availability.buildingUnavailable->at(7)&&!availability.buildingUnavailable->at(8)&&!availability.buildingUnavailable->at(9),"CropHold employment checks Empire production while CropField bypasses unavailable resource table");
    availability.unavailableResources->at(3)=1;availability.empireProduction->at(19)=true;
    need(originalBuildingUnavailableStep(availability)==OriginalBuildingUpdateBlocker::None&&!availability.buildingUnavailable->at(7)&&availability.buildingUnavailable->at(8),"single and multiple CropHold production predicates use independent19 and food lists");
    auto market=field().building;market.type=59;market.x=20;market.y=20;market.imageInputs->gridIndex=4580;market.imageInputs->marketOrientation=0;
    OriginalBuildingRoadInputs marketRoad;marketRoad.distance.resize(51984);marketRoad.networks.resize(51984);marketRoad.distance[5264]=17;marketRoad.networks[5264]=4;
    auto entrance=originalBuildingMarketRoadAccess(market,marketRoad);
    need(entrance&&entrance->distance==17&&entrance->x==20&&entrance->y==23&&entrance->network==4,"Market orientation0 uses descriptor entrance at origin plus684");
    market.imageInputs->marketOrientation=1;marketRoad.distance[4583]=23;marketRoad.networks[4583]=6;entrance=originalBuildingMarketRoadAccess(market,marketRoad);
    need(entrance&&entrance->distance==23&&entrance->x==23&&entrance->y==20,"Market orientation1 uses descriptor entrance at origin plus3");
    OriginalBuildingProfile multipart;market.imageInputs->marketStalls[0]=8;multipart.footprints.push_back(market);auto stall=field().building;stall.type=62;stall.slot=8;
    auto parentRoad=originalBuildingMultipartRoadAccess(stall,multipart,marketRoad,7);
    need(parentRoad&&parentRoad->distance==23&&!parentRoad->network,"Stall copies registered Market road fields and does not dispatch network setter");
    need(!originalBuildingMultipartRoadAccess(stall,multipart,marketRoad,6)&&!originalBuildingMultipartRoadAccess(stall,multipart,marketRoad),"Stall requires saved reciprocal parent to match framed Market child table");
    multipart.footprints[0].slot=9;need(!originalBuildingMultipartRoadAccess(stall,multipart,marketRoad,9),"Stall refuses parent after its own registered slot");
    auto resource=context(OriginalBuildingUpdateScope::AdmittedTicks);resource.activeSide=4;resource.cropBase=112*229;
    resource.terrain=std::vector<uint32_t>(51984);resource.figureServices.emplace();resource.buildings.emplace();resource.buildingInputs.emplace();
    resource.resourceGrowth.emplace();auto& growth=*resource.resourceGrowth;growth.surface.image.assign(51984,999);
    growth.surface.feature.resize(51984);growth.surface.randomByte.resize(51984);growth.surface.roadDistance.resize(51984);growth.owners.resize(51984);
    growth.imageBases={{0x620,1000},{0x624,2000},{0x625,3000},{0x627,4000}};
    const auto rows=originalBuildingResourceRows(4,112*229);
    need(rows&&rows->at(0)==std::pair<uint32_t,uint32_t>{112*229+1,2}&&rows->at(1).second==4&&rows->at(2).second==4&&rows->at(3).second==2,"resource diamond preserves two full middle rows and top offset");
    const size_t tree=rows->at(1).first,reed=tree+1,farm=tree+2,owned=tree+3;
    resource.terrain->at(tree)=1;resource.terrain->at(reed)=0x40000;resource.terrain->at(farm)=0x80;resource.terrain->at(owned)=1;
    growth.surface.roadDistance[tree]=253;growth.surface.roadDistance[reed]=230;growth.surface.roadDistance[farm]=250;
    growth.surface.feature[tree]=2;growth.surface.randomByte[tree]=10;growth.owners[owned]=7;
    auto seededResource=resource;seededResource.figureServices->randomFirst=1;seededResource.figureServices->randomSecond=2;
    need(originalBuildingResourceGrowthStep(seededResource)==OriginalBuildingUpdateBlocker::None&&seededResource.figureServices->randomFirst==32769&&seededResource.figureServices->randomSecond==65538&&seededResource.resourceGrowth->surface.roadDistance[owned]==5,"resource positive engine RNG advances all four draws in scan order");
    need(originalBuildingResourceGrowthStep(resource)==OriginalBuildingUpdateBlocker::None,"resource positive growth closes all three masks without process clocks");
    need(growth.surface.roadDistance[tree]==255&&growth.surface.roadDistance[reed]==255&&growth.surface.roadDistance[farm]==255&&growth.surface.roadDistance[owned]==4,"resource unsigned byte overflow saturates at255 and grows owned cells");
    auto sharedImage=resource;sharedImage.resourceGrowth->surface.roadDistance[tree]=253;sharedImage.irrigation.emplace();sharedImage.irrigation->image.assign(51984,777);
    need(originalBuildingResourceGrowthStep(sharedImage)==OriginalBuildingUpdateBlocker::None&&sharedImage.resourceGrowth->surface.image[farm]==777&&sharedImage.irrigation->image[tree]==3001,"resource image writes preserve prior irrigation surface and publish shared grid");
    need(growth.randomCalls==4&&growth.changed==4&&growth.imageWrites==2&&growth.surface.image[tree]==3001&&growth.surface.image[reed]==1000&&growth.surface.image[farm]==999&&growth.surface.image[owned]==999,"resource RNG precedes owner and farm image gates with explicit table offsets");
    need(growth.flags==std::array<uint8_t,6>{0,1,0,1,0,1},"resource resets six presence and routing flags before ordered scan");
    auto orderedResource=resource;orderedResource.phase=20;auto resourceState=field();resourceState.inputs.output=0;
    orderedResource.buildingInputs->emplace(7,resourceState.inputs);orderedResource.buildings->emplace(7,resourceState.building);
    const auto resourceTick=originalBuildingStateAfterUpdates(resourceState,orderedResource,1);
    need(resourceTick.state&&resourceTick.context->resourceGrowth->calls==2&&resourceTick.context->phase==21,"phase20 invokes terrain growth after production through additive public API");
    resource.terrain->at(owned)=0;need(originalBuildingResourceGrowthStep(resource)==OriginalBuildingUpdateBlocker::None&&growth.randomCalls==4&&growth.changed==0&&growth.matureTrees==std::vector<uint32_t>{uint32_t(tree)}&&growth.matureReeds==std::vector<uint32_t>{uint32_t(reed)},"mature resources collect ordered candidates without RNG or image writes");
    auto hook=context(OriginalBuildingUpdateScope::AdmittedTicks);hook.phase=31;auto ordinary=house;ordinary.building.type=5;
    auto houseHook=originalBuildingStateAfterUpdates(ordinary,hook,1);need(houseHook.state&&houseHook.virtual20Calls==1,"ordinary house hook exits518D90 before road or wall-clock inputs");
    auto well=field();well.building.type=72;well.inputs.industry.reset();well.inputs.operational=100;well.inputs.workers=8;well.inputs.spawnCounter=1;
    hook.figures=std::vector<OriginalFigure>(2000);hook.walkersDisabled=false;hook.poolRoadSelected=std::map<size_t,bool>{{7,true}};hook.employmentModel=OriginalBuildingEmployment{{72,8}};
    auto wellHook=originalBuildingStateAfterUpdates(well,hook,1);need(!wellHook.state&&wellHook.blocker==OriginalBuildingUpdateBlocker::BuildingVirtual20,"Well strict counter greater than1 refuses positive allocation atomically");
    well.inputs.spawnCounter=0;wellHook=originalBuildingStateAfterUpdates(well,hook,1);need(wellHook.state&&wellHook.state->inputs.spawnCounter==1,"Well exact counter boundary advances without premature allocation");
    well.inputs.primaryFigure=13;hook.figures->at(13).alive=1;hook.figures->at(13).type=28;hook.figures->at(13).owner=7;hook.walkersDisabled.reset();hook.employmentModel.reset();
    wellHook=originalBuildingStateAfterUpdates(well,hook,1);need(wellHook.state&&wellHook.state->inputs.spawnCounter==0&&wellHook.state->inputs.primaryFigure==13,"valid Well reciprocal worker bypasses disabled staffing and counter inputs");
    hook.figures->at(13).owner=8;hook.walkersDisabled=true;wellHook=originalBuildingStateAfterUpdates(well,hook,1);need(wellHook.state&&wellHook.state->inputs.primaryFigure==0,"stale Well figure link clears before walkers-disabled early return");
    std::vector<uint8_t> hookBytes(original_save19::buildingDisabled+269);const size_t hookAt=original_save19::criminalCount+20;
    hookBytes[hookAt]=7;hookBytes[hookAt-4]=99;hookBytes[original_save19::buildingDisabled+172]=1;
    auto restoredHook=context(OriginalBuildingUpdateScope::AdmittedTicks);
    need(originalBuildingSavedHookContext(hookBytes,0,restoredHook)&&restoredHook.buildingHookCountdown==7&&restoredHook.walkersDisabled==true,"hook countdown SAV position follows four independent criminal-adjacent dwords");
    auto preamble=hook;preamble.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,well.building}};preamble.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,well.inputs}};
    preamble.buildingInputs->at(7).hookIndustry.emplace();preamble.buildingInputs->at(7).hookIndustry->at(0x5c-4)=255;
    preamble.buildingHookCountdown=2;preamble.highestBuildingSlot=7;preamble.figures->at(13).type=57;preamble.figures->at(13).owner=7;
    need(originalBuildingHookPreamble(preamble)==OriginalBuildingUpdateBlocker::None&&preamble.buildingHookCountdown==1&&preamble.buildingInputs->at(7).hookIndustry->at(0x5c-4)==0,"hook preamble decrements saved countdown and wraps serialized delivery count byte");
    auto hunter=field();hunter.building.type=33;hunter.inputs.industry.reset();hunter.inputs.workers=8;hunter.inputs.operational=100;
    auto hunting=context(OriginalBuildingUpdateScope::AdmittedTicks);hunting.phase=31;hunting.poolRoadSelected=std::map<size_t,bool>{{7,true}};hunting.employmentModel=OriginalBuildingEmployment{{33,8}};
    const auto hunterCounter=originalBuildingStateAfterUpdates(hunter,hunting,1);
    need(hunterCounter.state&&hunterCounter.state->inputs.spawnCounter==1,"Hunter hook counter1 closes before its strict greater-than1 allocation branch");
    hunter.inputs.spawnCounter=1;const auto hunterAllocate=originalBuildingStateAfterUpdates(hunter,hunting,1);
    need(!hunterAllocate.state&&hunterAllocate.blocker==OriginalBuildingUpdateBlocker::BuildingVirtual20,"Hunter positive allocation retains exact hook refusal");
    auto trading=context(OriginalBuildingUpdateScope::AdmittedTicks);trading.phase=32;trading.trade.emplace();trading.statistics.emplace();trading.statistics->currentFaction=0;
    trading.statistics->religionBonusEnabled=false;trading.figures=std::vector<OriginalFigure>(2000);trading.buildings.emplace();trading.buildingInputs.emplace();
    auto& partner=trading.trade->partners[1];partner.alive=1;partner.kind=2;partner.faction=1;partner.enabled=1;partner.buys[0]=3;partner.quota[3]=12;partner.timer=4;
    trading.trade->counters={7,8,2,-3};
    need(originalBuildingTradeStep(trading)==OriginalBuildingUpdateBlocker::None&&partner.timer==3&&trading.trade->counters==std::array<int16_t,4>{0,1,1,0},"active saved trade quota decrements dispatch timer after resetting and saturating four counters");
    partner.blocked=4;need(originalBuildingTradeStep(trading)==OriginalBuildingUpdateBlocker::None&&partner.timer==3&&trading.trade->eligible==0,"saved trade bit4 blocks before quota and timer writes");
    partner.blocked=0;partner.traders[0]=12;trading.figures->at(12).alive=1;
    need(originalBuildingTradeStep(trading)==OriginalBuildingUpdateBlocker::None&&partner.timer==3,"occupied trader link prevents dispatch timer decrement");
    partner.traders[0]=0;partner.timer=0;trading.trade->dispatchDelay=std::array<int32_t,2>{11,19};
    need(originalBuildingTradeStep(trading)==OriginalBuildingUpdateBlocker::None&&partner.timer==19,"land trade expired timer reads explicit process service delay before searching building registry");
    trading.trade->dispatchDelay.reset();partner.timer=0;
    need(originalBuildingTradeStep(trading)==OriginalBuildingUpdateBlocker::MissingContext,"trade timer expiry requires process delay rather than a fitted saved timer");
    std::vector<uint8_t> tradeBytes(13000);tradeBytes[0]=7;tradeBytes[11091]=9;tradeBytes[11093]=8;tradeBytes[11095]=7;tradeBytes[11097]=6;tradeBytes[11090]=99;
    need(originalBuildingSavedTradeContext(tradeBytes,12287,trading)&&trading.trade->counters==std::array<int16_t,4>{9,8,7,6},"nested Population7 trade shorts use four literal offsets and skip neighboring byte");
    tradeBytes[0]=6;need(!originalBuildingSavedTradeContext(tradeBytes,12287,trading),"trade globals reject wrong nested Population version");
    partner.timer=0;trading.trade->dispatchDelay=std::array<int32_t,2>{11,19};auto station=field();station.building.type=58;station.inputs.model=1;
    trading.buildings->emplace(7,station.building);trading.buildingInputs->emplace(7,station.inputs);trading.trade->counters[3]=0;
    const auto tradeRefusal=originalBuildingStateAfterUpdates(station,trading,1);
    need(!tradeRefusal.state&&!tradeRefusal.context&&tradeRefusal.blocker==OriginalBuildingUpdateBlocker::Trade&&partner.timer==0,"positive trader creation refuses atomically after tentative timer service");
    auto totals=context(OriginalBuildingUpdateScope::AdmittedTicks);totals.phase=33;totals.figures=std::vector<OriginalFigure>(2000);totals.employmentModel=OriginalBuildingEmployment{{194,0},{71,0}};
    auto totalCrop=field();totalCrop.inputs.workers=2;totalCrop.inputs.migrant=12;totals.figures->at(12).alive=1;totals.figures->at(12).delivery=7;totals.figures->at(12).home=99;
    auto totalHouse=house;totalHouse.building.slot=8;totalHouse.inputs.identity=8;totalHouse.inputs.houseInUse=1;totalHouse.inputs.residents=4;
    auto music=field();music.building.type=71;music.building.slot=9;music.inputs.identity=9;music.inputs.hookIndustry.emplace();music.inputs.hookIndustry->at(0x5d-4)=2;music.inputs.hookIndustry->at(0x5f-4)=1;
    totals.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,totalCrop.building},{8,totalHouse.building},{9,music.building}};
    totals.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,totalCrop.inputs},{8,totalHouse.inputs},{9,music.inputs}};
    totals.buildingTotals.emplace();totals.buildingTotals->producers.fill(9);totals.buildingTotals->staffedProducers.fill(8);totals.serviceCoverage=std::array<int32_t,11>{};totals.serviceCoverage->fill(73);
    need(originalBuildingTotalsStep(totals)==OriginalBuildingUpdateBlocker::None&&totals.buildingTotals->total[194]==1&&totals.buildingTotals->staffed[194]==0&&totals.buildingTotals->producers[19]==1&&totals.buildingTotals->staffedProducers[19]==1&&totals.buildingTotals->total[5]==1&&totals.buildingTotals->staffed[5]==1,"phase33 counts type and crop resource independently of model workforce");
    need(totals.buildingInputs->at(7).migrant==12&&totals.buildingTotals->producers[30]==9&&totals.serviceCoverage->at(0)==73&&totals.buildingInputs->at(9).hookIndustry->at(0x5c-4)==2&&totals.buildingInputs->at(9).hookIndustry->at(0x5d-4)==1,"totals preserves high resource rows and coverage and validates figure delivery link rather than home");
    totals.figures->at(12).delivery=8;need(originalBuildingTotalsStep(totals)==OriginalBuildingUpdateBlocker::None&&totals.buildingInputs->at(7).migrant==0,"phase33 clears mismatched reciprocal delivery link");
    auto aggregate=house.inputs;aggregate.house->fill(0);for(unsigned wire:{41u,42u,43u,44u,45u,49u,51u})aggregate.house->at(wire-4)=1;aggregate.house->at(53-4)=90;
    originalBuildingAggregateHouseServices(aggregate);
    need(aggregate.house->at(55-4)==100&&aggregate.house->at(54-4)==99&&aggregate.house->at(53-4)==90,"House1 aggregation writes literal service55 religion54 and preserves entertainment53");
    aggregate.house->at(51-4)=0;aggregate.house->at(53-4)=29;originalBuildingAggregateHouseServices(aggregate);
    need(aggregate.house->at(55-4)==60,"house service aggregate uses lower water band and strict entertainment30 boundary");
    std::string modelFixture="BUILDING MODS\n";
    for(unsigned row=0;row<5;++row){modelFixture+="{,";for(unsigned col=0;col<13;++col)modelFixture+=std::to_string(row==2?150:100)+",";modelFixture+="}\n";}
    modelFixture+="ALL BUILDINGS\n";
    for(unsigned row=0;row<269;++row){modelFixture+="{,";for(unsigned col=0;col<13;++col)modelFixture+=std::to_string(col==1?-3:int(col))+",";modelFixture+="}\n";}modelFixture+="HOUSE MODS\n";
    const auto adjusted=originalBuildingModels(modelFixture,2);
    need(adjusted&&adjusted->size()==269&&adjusted->at(268)[1]==-4&&adjusted->at(0)[5]==7,"building model difficulty multiplies thirteen independent columns with signed truncation");
    need(!originalBuildingModels(modelFixture,5)&&!originalBuildingModels(modelFixture.substr(0,modelFixture.find("HOUSE MODS")),2),"building model requires selected difficulty and complete section framing");
    need(originalBuildingInfluenceRing(1,1)==std::vector<std::pair<int,int>>{{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}}&&originalBuildingInfluenceRing(2,1).front()==std::pair<int,int>{0,-1}&&originalBuildingInfluenceRing(2,2).size()==20,"desirability size1 row order and larger clockwise ring match independent generators");
    auto desire=context(OriginalBuildingUpdateScope::AdmittedTicks);desire.activeSide=20;desire.cropBase=104*229;desire.terrain=std::vector<uint32_t>(51984);desire.terrainOwners=std::vector<uint16_t>(51984);desire.resourceGrowth.emplace();desire.resourceGrowth->surface.feature.resize(51984);desire.resourceGrowth->surface.subTile.resize(51984);
    auto emitter=field();emitter.building.x=10;emitter.building.y=10;emitter.building.imageInputs->gridIndex=*desire.cropBase+2290;emitter.building.type=72;
    desire.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,emitter.building}};desire.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,emitter.inputs}};desire.buildingModels.emplace();desire.buildingModels->operator[](72)={0,6,2,-2,3};
    const size_t origin=emitter.building.imageInputs->gridIndex;
    need(originalBuildingDesirabilityStep(desire)==OriginalBuildingUpdateBlocker::None&&desire.desirability->at(origin-228)==6&&desire.desirability->at(origin-456)==6&&desire.desirability->at(origin-684)==4&&desire.desirability->at(origin)==0,"square influence applies step count before delta and excludes source footprint");
    desire.buildings->at(7).size=2;desire.buildingInputs->at(7).desirabilityBonus=1;desire.desirability->at(origin)=-20;desire.desirability->at(origin+229)=95;
    need(originalBuildingApplyDesirability(desire)==OriginalBuildingUpdateBlocker::None&&desire.buildingInputs->at(7).desirability==105,"phase38 takes signed maximum over footprint then applies uncapped bonus10");
    desire.buildings->at(7)=emitter.building;desire.buildings->at(7).x=0;desire.buildings->at(7).y=0;desire.buildingModels->at(72)={0,1,1,0,2};
    need(originalBuildingDesirabilityStep(desire)==OriginalBuildingUpdateBlocker::None&&desire.desirability->at(*desire.cropBase)==7&&desire.desirability->at(*desire.cropBase-228)==1,"clipped influence tests perimeter bounds but repeats original source-origin write");
    desire.buildings->at(7)=emitter.building;desire.buildings->at(7).type=59;desire.buildings->at(7).imageInputs->marketOrientation=0;desire.buildingModels->operator[](59)={0,-4,3,1,3};
    need(originalBuildingDesirabilityStep(desire)==OriginalBuildingUpdateBlocker::None&&desire.desirability->at(origin-228)==-4&&desire.desirability->at(origin+228*7+3)==-4&&desire.desirability->at(origin-456)==-4&&desire.desirability->at(origin-684)==-3&&desire.desirability->at(origin)==0,"Market rectangle uses inclusive descriptor bounds and initial counter1 delta schedule");
    desire.buildingModels->at(59)[1]=4;need(originalBuildingDesirabilityStep(desire)==OriginalBuildingUpdateBlocker::MissingContext,"positive rectangle requires named process8254E4 shadow premise");
    desire.buildings->clear();desire.buildingInputs->clear();const auto desireRows=originalBuildingResourceRows(20,*desire.cropBase);const size_t stale=desireRows->at(0).first;desire.resourceGrowth->surface.subTile[stale]=0x85;
    need(originalBuildingDesirabilityStep(desire)==OriginalBuildingUpdateBlocker::None&&desire.resourceGrowth->surface.subTile[stale]==5,"terrain desirability clears stale bit80 while preserving other subtile bits");
    auto service=lc;service.phase=31;service.activeSide=20;service.terrainOwners=std::vector<uint16_t>(51984);service.terrainRandom=std::vector<uint8_t>(51984);service.walkersDisabled=false;service.terrain->at(1145)=0x40;
    service.figureSpeechCounts->at(271)=3;service.figureSpeechCounters->at(271)=7;service.employmentModel=OriginalBuildingEmployment{{72,8}};
    service.poolRoadSelected=std::map<size_t,bool>{{7,true}};service.poolHookRoadAccess=std::map<size_t,OriginalBuildingRoadAccess>{{7,{}}};service.poolHookRoadAccess->at(7).x=3;service.poolHookRoadAccess->at(7).y=4;
    service.poolRoadAccess=std::map<size_t,OriginalBuildingRoadAccess>{{7,{}}};service.poolRoadAccess->at(7).x=9;service.poolRoadAccess->at(7).y=9;
    service.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,well.inputs}};service.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,well.building}};
    service.buildingLinks=std::map<size_t,OriginalFigureBuildingLink>{{7,{}}};service.buildingLinks->at(7).type=72;service.buildingLinks->at(7).roadX=3;service.buildingLinks->at(7).roadY=4;
    auto allocatingWell=well;allocatingWell.inputs.primaryFigure=0;allocatingWell.inputs.spawnCounter=1;service.buildingInputs->at(7)=allocatingWell.inputs;
    auto created=originalBuildingStateAfterUpdates(allocatingWell,service,1);
    need(created.state&&created.state->inputs.primaryFigure==19&&created.state->inputs.spawnCounter==0&&created.context->allocatedFigures==std::vector<size_t>{19}&&created.context->figures->at(19).type==28&&created.context->figures->at(19).action==6&&created.context->figures->at(19).owner==7&&created.context->figures->at(19).counter==30,"Well positive allocation publishes reciprocal identity action and delay after strict counter gate");
    need(created.context->figures->at(19).cell==1144&&created.context->figureCellHeads->at(1144)==19&&created.context->figures->at(19).roamState==1&&created.context->figureSpeechChoices.at(19)==1&&created.context->figureSpeechCounters->at(271)==8&&created.context->crtRandomCalls==0,"service allocation registers cell and speech and starts cited roam without CRT or clock input");
    auto harvesting=service;auto allocatingHunter=hunter;allocatingHunter.inputs.hookIndustry.emplace();allocatingHunter.production.emplace();allocatingHunter.production->secondaryStock=0;
    harvesting.employmentModel=OriginalBuildingEmployment{{33,8}};harvesting.figureSpeechCounts->at(254)=3;harvesting.figureSpeechCounters->at(254)=0;harvesting.freeFigureSlots={19,18,17};
    harvesting.buildings->at(7)=allocatingHunter.building;harvesting.buildingInputs->at(7)=allocatingHunter.inputs;
    created=originalBuildingStateAfterUpdates(allocatingHunter,harvesting,1);
    need(created.state&&created.state->inputs.hookIndustry->at(0x5c-4)==3&&created.context->allocatedFigures==std::vector<size_t>({19,18,17})&&created.context->figures->at(17).type==40&&created.context->figures->at(17).action==12&&created.context->figures->at(17).counter>=20&&created.context->figures->at(17).counter<=51,"Hunter staffing100 creates three hunters below executable400 capacity with independent engine delays");
    auto dispatch=harvesting;dispatch.industryCarts=true;auto stockedHunter=allocatingHunter;stockedHunter.production->secondaryStock=100;
    stockedHunter.inputs.output=4;stockedHunter.inputs.upgrade=1;stockedHunter.inputs.hookIndustry->at(0x5c-4)=3;
    dispatch.freeFigureSlots={19};
    need(originalBuildingHunterDispatchStep(stockedHunter,dispatch)==OriginalBuildingUpdateBlocker::None&&
        stockedHunter.production->secondaryStock==0&&stockedHunter.inputs.primaryFigure==19&&stockedHunter.inputs.upgrade==0&&
        dispatch.figures->at(19).type==19&&dispatch.figures->at(19).amount==400&&dispatch.figures->at(19).subtype==4&&
        dispatch.figures->at(19).action==6&&dispatch.dispatchedProduction[4]==100,
        "Hunter positive cart preserves upgraded400 cargo while debiting and accounting literal100 only");
    dispatch=harvesting;dispatch.industryCarts=true;dispatch.freeFigureSlots.clear();stockedHunter=allocatingHunter;stockedHunter.production->secondaryStock=100;
    stockedHunter.inputs.output=4;stockedHunter.inputs.hookIndustry->at(0x5c-4)=3;
    need(originalBuildingHunterDispatchStep(stockedHunter,dispatch)==OriginalBuildingUpdateBlocker::None&&
        stockedHunter.production->secondaryStock==100&&stockedHunter.inputs.primaryFigure==0,
        "Hunter allocator exhaustion retains stock and leaves reciprocal cart link zero");
    saved32(22,69);saved32(26,68);saved32(11408,3);saved32(11412,901);saved32(11420,2);saved32(11424,603);
    saved32(12099,151);saved32(11392,999);saved32(anchor-28+1449,48);
    need(originalBuildingSavedCityContext(savedInputs,anchor,restoredCity)&&restoredCity.cycleTail->healthTarget==69&&restoredCity.cycleTail->smoothedHealth==68&&restoredCity.cycleTail->currentHealthRating==151&&restoredCity.scenarioMonths==48,"cycle tail saved health rating uses final59501B wire12099 rather than11392 decoy");
    need(restoredCity.cycleTail->productionHistory==std::array<int32_t,4>{3,901,2,603},"production history retains four independently serialized counters and amounts");
    //Exact Common5 additions preserve legacy accessors,including misleading names.
    std::vector<uint8_t> exact(184+140);exact[0]=1;exact[1]=128;
    const auto exact16=[&](size_t at,uint16_t value){exact.at(at)=uint8_t(value);exact.at(at+1)=uint8_t(value>>8);};
    exact16(2,5);exact16(18,194);exact[4]=1;exact[164]=7;
    for(size_t at:{184u,186u,314u,316u,322u})exact16(at,1);
    exact16(70,123);exact16(72,456);exact16(74,0xffff);exact16(132,0x1234);
    exact[86]=11;exact[87]=0xf4;exact[88]=13;exact[89]=14;exact[101]=15;exact[102]=16;
    const auto exactParsed=originalBuildingUpdateInputs(exact,p);
    need(exactParsed&&exactParsed->at(7).fireRisk==123&&exactParsed->at(7).damageRisk==456&&exactParsed->at(7).burning==-1&&exactParsed->at(7).word92==0x1234,"Common5 risk and serialized92 use68 70 72 130 with adjacent poison");
    need(exactParsed->at(7).desirability==11&&exactParsed->at(7).desirabilityBonus==13&&exactParsed->at(7).upgrade==15&&exactParsed->at(7).modelDesirability==-12&&exactParsed->at(7).modelDesirabilityBonus==14&&exactParsed->at(7).modelUpgrade==16,"legacy accessor contracts survive additive exact5E 60 and6F fields");
    auto bayProfile=p;bayProfile.slotTypes[7]=193;bayProfile.liveTypeCounts={{193,1}};bayProfile.footprints[0].type=193;
    exact.resize(184+222);exact16(18,193);exact16(314,1);exact16(324,1);
    const auto bay32=[&](size_t at,uint32_t value){for(unsigned n=0;n<4;++n)exact.at(at+n)=uint8_t(value>>(8*n));};
    bay32(316,3);bay32(320,2);
    for(unsigned n=0;n<5;++n){const size_t at=184+142+16*n;exact16(at,1);bay32(at+2,n+1);bay32(at+6,99+n);bay32(at+10,400);exact16(at+14,1);}
    const auto bays=originalBuildingUpdateInputs(exact,bayProfile);
    need(bays&&bays->at(7).cropWorkers==std::array<int32_t,2>{3,2}&&bays->at(7).cropBays->at(4).resource==5&&bays->at(7).cropBays->at(4).amount==103&&bays->at(7).cropBays->at(4).capacity==400,"MultipleCropHold five framed bays follow both saved farmer counts and own1");
    exact16(184+142+16*4+14,2);need(!originalBuildingUpdateInputs(exact,bayProfile),"last crop bay trailing wrapper version is mandatory");
    //Farmer grouping:one harvest worker reserves only the first of four fields.
    auto cropContext=harvesting;cropContext.buildingLinks.reset();cropContext.productionInputs.reset();cropContext.buildings->clear();cropContext.buildingInputs->clear();
    auto hold=field();hold.building.type=192;hold.building.size=2;hold.building.x=3;hold.building.y=4;hold.building.imageInputs->gridIndex=1144;
    hold.inputs.industry.reset();hold.inputs.output=0;hold.inputs.enabled=1;hold.inputs.workers=8;hold.inputs.operational=100;hold.inputs.roadX=3;hold.inputs.roadY=4;hold.inputs.spawnCounter=1;hold.inputs.cropWorkers=std::array<int32_t,2>{3,2};
    cropContext.employmentModel=OriginalBuildingEmployment{{192,8}};cropContext.unavailableResources=std::array<int16_t,36>{};cropContext.typeDisabled=false;
    cropContext.cropDispatchModel=std::array<int32_t,14>{5,3,4,2,3,2,10,6,8,4,3,4,50,8};cropContext.cropDispatchRadius=std::array<int32_t,2>{3,3};cropContext.cropServices.emplace();
    cropContext.cropFestival=false;cropContext.movement.emplace();cropContext.movement->cells.resize(51984);cropContext.cycleTail.emplace();cropContext.cycleTail->cropQuality.resize(51984,100);
    for(unsigned n=0;n<4;++n){auto target=field(194,5000,0x8000);target.building.slot=8+n;target.inputs.identity=8+n;target.inputs.enabled=1;target.building.x=3+n;target.building.y=4;target.building.imageInputs->gridIndex=1144+n;
        cropContext.buildings->emplace(8+n,target.building);cropContext.buildingInputs->emplace(8+n,target.inputs);cropContext.movement->cells[1144+n]=1;}
    cropContext.buildingInputs->at(9).industry->at(0)=0x50;cropContext.buildingInputs->at(9).industry->at(1)=0x14; //5200:score24+distance2 ties field8's25+1.
    cropContext.buildings->emplace(7,hold.building);cropContext.buildingInputs->emplace(7,hold.inputs);cropContext.wallClock=1000;cropContext.freeFigureSlots={19,18};
    auto farmers=originalBuildingStateAfterUpdates(hold,cropContext,1);
    need(farmers.state&&farmers.state->inputs.cropWorkers->at(0)==4&&farmers.context->allocatedFigures==std::vector<size_t>{19}&&farmers.context->figures->at(19).action==10&&farmers.context->farmerTasks.at(19)==2,"harvest staffing allocates one missing farmer after the old counter1 gate");
    need(farmers.context->buildingInputs->at(8).primaryFigure==19&&farmers.context->buildingInputs->at(9).primaryFigure==0&&farmers.context->buildingInputs->at(10).primaryFigure==0&&farmers.context->buildingInputs->at(11).primaryFigure==0,"farmer grouping clears skipped candidates rather than assigning the new farmer");
    need(farmers.context->figures->at(19).counter==0&&farmers.context->figures->at(19).owner==7&&farmers.context->figures->at(19).variant==1&&farmers.context->crtRandomCalls==0,"farmer local LCG consumes explicit wall time after constructor and no CRT draws");
    auto missingClock=cropContext;missingClock.wallClock.reset();auto farmerRefusal=originalBuildingStateAfterUpdates(hold,missingClock,1);
    need(!farmerRefusal.state&&!farmerRefusal.context&&farmerRefusal.refusedPhase==31&&farmerRefusal.operations==0&&farmerRefusal.blocker==OriginalBuildingUpdateBlocker::FarmerWallClock&&missingClock.freeFigureSlots==std::vector<size_t>({19,18})&&missingClock.figures->at(19).alive==0,"farmer clock refusal names766DA0 and publishes no allocator or field prefix");
    missingClock.freeFigureSlots.clear();farmers=originalBuildingStateAfterUpdates(hold,missingClock,1);
    need(farmers.state&&farmers.context->allocatedFigures.empty()&&farmers.state->inputs.cropWorkers->at(0)==3,"exhausted farmer FIFO does not require an unconsumed wall clock");
    auto legacyCrop=cropContext;legacyCrop.cropDispatchModel.reset();const auto legacyBoundary=originalBuildingStateAfterUpdates(hold,legacyCrop,1);
    need(!legacyBoundary.state&&legacyBoundary.blocker==OriginalBuildingUpdateBlocker::BuildingVirtual20,"legacy crop hook context keeps virtual20 refusal until additive farmer model is supplied");
    auto cropBayContext=cropContext;cropBayContext.buildings->clear();cropBayContext.buildingInputs->clear();auto multi=hold;multi.building.type=193;multi.inputs.cropWorkers=std::array<int32_t,2>{5,2};multi.inputs.cropBays.emplace();
    for(auto& bay:*multi.inputs.cropBays)bay={1,99,400};cropBayContext.buildings->emplace(7,multi.building);cropBayContext.buildingInputs->emplace(7,multi.inputs);cropBayContext.employmentModel=OriginalBuildingEmployment{{193,8}};
    need(originalBuildingCropDispatch(multi,cropBayContext,100)==OriginalBuildingUpdateBlocker::None,"all five crop bays below100 close without cart allocation");
    multi.inputs.cropBays->at(4).amount=100;need(originalBuildingCropDispatch(multi,cropBayContext,100)==OriginalBuildingUpdateBlocker::BuildingVirtual20,"fifth crop bay at100 reaches named positive cart boundary");
    //Ordinary house failure delays,goods consumption,and supported conversion.
    auto evolution=context(OriginalBuildingUpdateScope::AdmittedTicks);evolution.cropBase=229;evolution.activeSide=20;evolution.terrainOwners=std::vector<uint16_t>(51984);evolution.terrainOwners->at(1144)=7;
    evolution.evolutionDisabled=false;evolution.forceHouseEvolution=false;evolution.houseModels.emplace();for(unsigned n=0;n<15;++n)evolution.houseModels->operator[](n)={};
    auto evolving=house;evolving.building.size=2;evolving.building.x=3;evolving.building.y=4;evolving.building.imageInputs->gridIndex=1144;evolving.inputs.model=2;evolving.inputs.modelDesirability=0;evolving.inputs.house->fill(0);evolving.inputs.residents=10;
    evolution.houseModels->at(2)[0]=0;evolving.inputs.house->at(56-4)=8;
    need(originalBuildingHouseEvolutionStep(evolving,evolution)==OriginalBuildingUpdateBlocker::None&&evolving.building.type==5&&evolving.inputs.house->at(56-4)==9,"house failed requirement waits through old delay8 before conversion");
    need(originalBuildingHouseEvolutionStep(evolving,evolution)==OriginalBuildingUpdateBlocker::HousingEvolution&&evolving.building.type==5,"uncited target4 selector refuses rather than publishing incomplete house geometry");
    evolving.building.type=2;evolving.inputs.model=0;evolving.inputs.modelDesirability=10;evolution.houseModels->at(0)[1]=1;
    need(originalBuildingHouseEvolutionStep(evolving,evolution)==OriginalBuildingUpdateBlocker::None&&evolving.building.type==3&&evolving.inputs.model==0&&evolution.houseConversions==1,"supported type2 to3 evolution retains existing selector and model0");
    evolution.monthPart=7;evolution.evolutionDisabled=true;evolution.houseModels->at(0)[10]=3;evolving.inputs.house->at(0x16-1-4)=5;evolving.inputs.house->at(0x18-1-4)=77;
    need(originalBuildingHouseEvolutionStep(evolving,evolution)==OriginalBuildingUpdateBlocker::None&&evolving.inputs.house->at(0x16-1-4)==2&&evolving.inputs.house->at(0x18-1-4)==77,"house part7 consumes model goods at exact omitted-padding offset and preserves decoy");
    auto upgraded=well;upgraded.building.wellImageInputs.emplace();upgraded.inputs.model=0;upgraded.inputs.modelUpgrade=0;upgraded.inputs.modelDesirability=10;auto lifecycle=context(OriginalBuildingUpdateScope::AdmittedTicks);lifecycle.buildingModels.emplace();lifecycle.buildingModels->operator[](72)[10]=10;
    need(originalBuildingLifecycleStep(upgraded,lifecycle)==OriginalBuildingUpdateBlocker::None&&upgraded.inputs.model==1&&upgraded.building.wellImageInputs->word16==1&&lifecycle.buildingRedraws.count(7)==1,"phase40 inclusive desirability threshold upgrades model and existing Well selector");
    //Risk draw order:the eligible slot's damage draw precedes phase mismatch.
    auto risks=context(OriginalBuildingUpdateScope::AdmittedTicks);risks.evolutionDisabled=false;risks.figureServices.emplace();risks.engineRegion=0;risks.terrain=std::vector<uint32_t>(51984);risks.terrainRandom=std::vector<uint8_t>(51984);risks.buildingModels.emplace();risks.buildingModels->operator[](33)[6]=2;risks.buildingModels->at(33)[7]=3;
    auto riskBuilding=field();riskBuilding.building.type=33;riskBuilding.building.imageInputs->gridIndex=1144;riskBuilding.inputs.industry.reset();riskBuilding.inputs.output=0;riskBuilding.inputs.fireRisk=5;riskBuilding.inputs.damageRisk=7;
    risks.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,riskBuilding.building}};risks.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,riskBuilding.inputs}};
    risks.terrainRandom->at(1144)=1;
    need(originalBuildingRiskStep(risks)==OriginalBuildingUpdateBlocker::None&&risks.buildingInputs->at(7).fireRisk==8&&risks.buildingInputs->at(7).damageRisk==12,"risk phase0 region0 adds collapse model7=3 and ignition model6=2 plus fixed3");
    risks.phase=44;risks.buildingInputs->at(7).damageRisk=999;riskBuilding.inputs=risks.buildingInputs->at(7);const auto riskRefusal=originalBuildingStateAfterUpdates(riskBuilding,risks,1);
    need(!riskRefusal.state&&!riskRefusal.context&&riskRefusal.refusedPhase==44&&risks.buildingInputs->at(7).damageRisk==999,"positive risk disaster remains atomic at44 instead of suppressing threshold1000");
    risks.buildingInputs->at(7).burning=-2;need(originalBuildingFireStep(risks)==OriginalBuildingUpdateBlocker::None&&risks.buildingInputs->at(7).burning==0,"fire pass normalizes negative burning before zero-fire exit");
    risks.effectDecayCounter=7;risks.effectGrid=std::vector<uint32_t>(51984,0);risks.activeSide=4;risks.cropBase=112*229;const size_t effectCell=112*229+1;
    risks.effectGrid->at(effectCell)=0xf8000039;
    need(originalBuildingMonumentStep(risks)==OriginalBuildingUpdateBlocker::None&&risks.effectGrid->at(effectCell)==0xf8000030&&risks.effectDecayCounter==0,"eighth monument pass decays packed3-bit lanes and preserves upper five bits");
    auto resetCounters=context(OriginalBuildingUpdateScope::AdmittedTicks);resetCounters.phase=47;resetCounters.phase47Counters={1,2,3,4};
    const auto resetResult=originalBuildingStateAfterUpdates(riskBuilding,resetCounters,1);
    need(resetResult.state&&resetResult.context->phase47Counters==std::array<int32_t,4>{}&&resetResult.context->counterCalls==1&&resetResult.context->phase==48,"phase47 clears four literal counters and advances to48");
    //An ordinary cycle tail with two independent production averages.
    auto tailContext=context(OriginalBuildingUpdateScope::AdmittedTicks);tailContext.phase=50;tailContext.monthPart=0;tailContext.cycleTail.emplace();tailContext.cycleTail->alternateMode=false;tailContext.cycleTail->smoothedHealth=59;tailContext.cycleTail->healthTarget=80;
    tailContext.labor.emplace();tailContext.labor->difficulty=2;tailContext.populationTotal=400;tailContext.buildingTotals.emplace();tailContext.buildingTotals->staffedProducers[4]=2;tailContext.buildingTotals->staffedProducers[2]=1;tailContext.cycleTail->productionHistory={3,901,2,603};
    tailContext.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,upgraded.building}};tailContext.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,upgraded.inputs}};tailContext.goodsRecords.emplace();tailContext.goods.emplace();tailContext.buildingInputs->at(7).word92=123;tailContext.buildingInputs->at(7).modelUpgrade=1;
    auto expiring=upgraded;expiring.inputs=tailContext.buildingInputs->at(7);auto wrapped=originalBuildingStateAfterUpdates(expiring,tailContext,1);
    need(wrapped.state&&wrapped.context->phase==0&&wrapped.context->monthPart==1&&wrapped.context->cycle==5&&wrapped.context->cycleTail->smoothedHealth==60&&wrapped.context->notificationEffects==std::vector<unsigned>{0xcf},"ordinary phase50 increments both clocks smooths health59 to60 and resets phase");
    need(wrapped.context->cycleTail->foodForecast==std::array<int32_t,2>{3604,901}&&wrapped.context->buildingInputs->at(7).word92==0&&wrapped.context->buildingInputs->at(7).modelUpgrade==0&&wrapped.context->buildingRedraws.count(7)==1,"cycle tail uses production averages and25 denominators then expires exact6F and saved92");
    auto part2=tailContext;part2.monthPart=1;part2.trade.emplace();part2.scenarioMonths=0;part2.statistics.emplace();part2.statistics->currentFaction=21;
    auto& timed=part2.trade->partners[0];timed.alive=1;timed.enabled=1;timed.reserveCountdown=1;timed.reserveAmount=2;timed.reserves={0,0,3,0,10};timed.relationTimers.resize(22);timed.relationTimers[21][0][32]=2;timed.relationTimers[21][1][32]=50;timed.relationTimers[21][0][33]=77;timed.relationTimers[21][1][33]=88;
    timed.relationTimers[0][0][32]=13;timed.relationTimers[0][1][32]=17;timed.relationTimers[20][0][0]=23;timed.relationTimers[20][1][0]=29;
    const auto savedPart2=part2;
    need(originalBuildingCycleTailStep(part2)==OriginalBuildingUpdateBlocker::None&&part2.trade->partners[0].relationTimers[21][0][32]==1&&part2.trade->partners[0].relationTimers[21][1][32]==48&&part2.trade->partners[0].reserves[0]==3&&part2.trade->partners[0].reserves[2]==1,"part2 current faction21 decrements last timer and releases reserves before capacity increment");
    need(part2.trade->partners[0].relationTimers[21][0][33]==77&&part2.trade->partners[0].relationTimers[21][1][33]==88,"part2 preserves serialized34th timer outside33-entry decrement loop");
    need(part2.trade->partners[0].relationTimers[0]==savedPart2.trade->partners[0].relationTimers[0]&&part2.trade->partners[0].relationTimers[20]==savedPart2.trade->partners[0].relationTimers[20],"part2 preserves other faction child timers selected neither by592360 nor442040");
    auto firstFaction=savedPart2;firstFaction.statistics->currentFaction=0;
    need(originalBuildingCycleTailStep(firstFaction)==OriginalBuildingUpdateBlocker::None&&firstFaction.trade->partners[0].relationTimers[0][0][32]==12&&firstFaction.trade->partners[0].relationTimers[0][1][32]==16&&firstFaction.trade->partners[0].relationTimers[21]==savedPart2.trade->partners[0].relationTimers[21],"part2 current faction0 selects first child and preserves last child");
    auto missingFaction=savedPart2;missingFaction.statistics.reset();const auto missingFactionResult=originalBuildingStateAfterUpdates(expiring,missingFaction,1);
    need(!missingFactionResult.state&&!missingFactionResult.context&&missingFactionResult.blocker==OriginalBuildingUpdateBlocker::MonthlyCity&&missingFaction.monthPart==1,"part2 missing consumed faction refuses atomically through public API");
    auto invalidFaction=savedPart2;invalidFaction.statistics->currentFaction=22;const auto invalidFactionResult=originalBuildingStateAfterUpdates(expiring,invalidFaction,1);
    need(!invalidFactionResult.state&&!invalidFactionResult.context&&invalidFactionResult.blocker==OriginalBuildingUpdateBlocker::InvalidState,"part2 out of range current faction refuses before child indexing");
    auto part3=tailContext;part3.monthPart=2;part3.cycleTail->currentHealthRating=147;
    need(originalBuildingCycleTailStep(part3)==OriginalBuildingUpdateBlocker::None&&part3.notificationEffects.back()==0xd6,"part3 rating147 needs no unused previous-rating premise");
    part3=tailContext;part3.monthPart=2;part3.cycleTail->currentHealthRating=151;
    need(originalBuildingCycleTailStep(part3)==OriginalBuildingUpdateBlocker::MonthlyCity,"part3 rating151 requires previous notification rating when consumed");
    auto forecast=tailContext;forecast.engineRegion=0;forecast.activeSide=20;forecast.cropBase=229;forecast.terrain=std::vector<uint32_t>(51984);forecast.cycleTail->cropQuality=std::vector<uint8_t>(51984,100);
    auto cropForecast=field(196).building;cropForecast.x=3;cropForecast.y=4;cropForecast.imageInputs->gridIndex=1144;
    need(originalBuildingCropForecast(cropForecast,forecast)==69,"forecast preserves single precision .01 rounding before extended rate multiplication");
    //Stall road query walks parent-owned pavement;bay ordinal is a saved
    //index,not the requested resource number or the parent's total stock.
    auto stallContext=context(OriginalBuildingUpdateScope::AdmittedTicks);stallContext.cropBase=229;stallContext.terrain=std::vector<uint32_t>(51984);stallContext.terrainOwners=std::vector<uint16_t>(51984);stallContext.roadNetworks.resize(51984);stallContext.rankedNetworks[0]=2;
    auto stallState=field();stallState.building.type=66;stallState.building.size=2;stallState.building.x=3;stallState.building.y=4;stallState.building.imageInputs->gridIndex=1144;
    stallState.inputs.industry.reset();stallState.inputs.parentSlot=6;stallState.inputs.stallResourceIndex=2;stallState.inputs.workers=8;stallState.inputs.spawnCounter=2;
    auto stallParent=stallState.building;stallParent.slot=6;stallParent.type=59;stallParent.imageInputs->marketOrientation=0;
    stallContext.buildings=std::map<size_t,OriginalBuildingFootprint>{{6,stallParent},{7,stallState.building}};stallContext.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{6,{}},{7,stallState.inputs}};stallContext.buildingInputs->at(6).operational=100;
    stallContext.terrainOwners->at(916)=6;stallContext.roadNetworks[1828]=2;stallContext.employmentModel=OriginalBuildingEmployment{{66,8}};stallContext.goods.emplace();stallContext.goods->policies.resize(1);stallContext.buildingLinks=std::map<size_t,OriginalFigureBuildingLink>{{6,{}},{7,{}}};
    stallContext.buildingLinks->at(6).storagePolicy=0;stallContext.buildingLinks->at(6).marketStorage=std::vector<OriginalStorageCompartment>(6);stallContext.buildingLinks->at(6).marketStorage->at(2)={28,400,400};
    need(originalBuildingStallHook(stallState,stallContext)==OriginalBuildingUpdateBlocker::None&&stallState.inputs.network==2&&stallState.inputs.spawnCounter==3&&stallContext.stallHookRoadAccess->at(7).x==3&&stallContext.stallHookRoadAccess->at(7).y==7,"Stall parent pavement maps to actual Market entrance and resource28 capacity400");
    stallContext.stallHookOrdinal=0;stallContext.buildingLinks->at(6).marketStorage->at(2).amount=399;
    need(originalBuildingStallHook(stallState,stallContext)==OriginalBuildingUpdateBlocker::BuildingVirtual20,"Stall saved bay399 reaches supplier boundary below resource28 capacity400");
    OriginalFigureBuildingLink exportBay;exportBay.storagePolicy=0;exportBay.storage=std::vector<OriginalStorageCompartment>{{19,20,400}};OriginalFigureGoodsServices exportGoods;exportGoods.policies.resize(1);exportGoods.policies[0].accept[19]=3;
    need(originalBuildingStorageExport(exportBay,exportGoods,50,true)==19&&originalBuildingStorageExport(exportBay,exportGoods,49,true)==-1,"storage export accepts earliest mode3 bay only from staffing50");
    exportGoods.policies[0].accept[1]=2;exportGoods.policies[0].limit[1]=1;
    need(originalBuildingStorageExport(exportBay,exportGoods,50,true)==0,"storage mode2 unmet limit precedes a stocked mode3 resource");
    auto hookStation=well;hookStation.building.type=58;hookStation.inputs.hookIndustry.emplace();hookStation.inputs.hookIndustry->fill(0);hookStation.inputs.model=0;hookStation.inputs.workers=8;hookStation.inputs.available=0;hookStation.inputs.suppression=99;
    auto stationContext=service;stationContext.employmentModel=OriginalBuildingEmployment{{58,8}};stationContext.buildingLinks->at(7).storagePolicy=0;stationContext.buildingLinks->at(7).storage.emplace();stationContext.goods.emplace();stationContext.goods->policies.resize(1);stationContext.trade.emplace();
    need(originalBuildingStorageHook(hookStation,stationContext)==OriginalBuildingUpdateBlocker::None&&hookStation.inputs.available==1&&hookStation.inputs.suppression==99,"TradingStation hook writes runtime48 availability while preserving legacy suppression contract");
    std::string houseFixture="HOUSE MODS\n";
    for(unsigned row=0;row<5;++row){houseFixture+="{,";for(unsigned col=0;col<24;++col)houseFixture+=std::to_string(row==2?5:0)+",";houseFixture+="}\n";}
    houseFixture+="ALL HOUSES\n";for(unsigned row=0;row<15;++row){houseFixture+="{,";for(unsigned col=0;col<24;++col)houseFixture+=std::to_string(col==0?-110:col==1?110:10)+",";houseFixture+="}\n";}
    const auto houseAdjusted=originalBuildingHouseModels(houseFixture,2);
    need(houseAdjusted&&houseAdjusted->at(14)[0]==-99&&houseAdjusted->at(14)[1]==100&&houseAdjusted->at(14)[23]==15&&!originalBuildingHouseModels(houseFixture,5),"HOUSE MODS adds selected row then clamps all24 independent model columns");
    auto missedPhase=risks;missedPhase.figureServices->randomFirst=1;missedPhase.figureServices->randomSecond=2;missedPhase.figureServices->randomHistory.emplace();missedPhase.figureServices->randomHistory->cursor=0;
    missedPhase.buildingInputs->at(7).damageRisk=10;missedPhase.terrainRandom->at(1144)=0;missedPhase.buildingInputs->at(7).burning=0;
    need(originalBuildingRiskStep(missedPhase)==OriginalBuildingUpdateBlocker::None&&missedPhase.figureServices->randomHistory->cursor==2&&missedPhase.buildingInputs->at(7).damageRisk==10,"risk consumes damage RNG even when slot phase comparison fails");
    auto modelContinuation=tailContext;modelContinuation.phase=40;modelContinuation.buildingModels=lifecycle.buildingModels;
    auto unupgraded=upgraded;unupgraded.inputs.model=0;unupgraded.inputs.modelUpgrade=0;unupgraded.building.wellImageInputs->word16=0;
    modelContinuation.buildingInputs->at(7)=unupgraded.inputs;modelContinuation.buildings->at(7)=unupgraded.building;
    const auto modelStep=originalBuildingStateAfterUpdates(unupgraded,modelContinuation,1);
    need(modelStep.state&&modelStep.context->buildings->at(7).wellImageInputs->word16==1,"phase40 publishes changed Well selector into continued building pool");
    auto modelTail=*modelStep.context;modelTail.phase=50;const auto keptModel=originalBuildingStateAfterUpdates(*modelStep.state,modelTail,1);
    need(keptModel.state&&keptModel.state->building.wellImageInputs->word16==1&&keptModel.state->inputs.model==1,"phase50 preserves phase40 model selection through copied continuation");
    auto positiveRisk=service;positiveRisk.phase=44;positiveRisk.disasters.emplace();positiveRisk.evolutionDisabled=false;positiveRisk.engineRegion=0;
    positiveRisk.figureServices->randomFirst=positiveRisk.figureServices->randomSecond=0;
    positiveRisk.buildingModels.emplace();positiveRisk.buildingModels->operator[](33)[6]=2;
    positiveRisk.terrainRandom->at(1144)=1;positiveRisk.buildings->at(7)=riskBuilding.building;
    riskBuilding.inputs.damageRisk=999;riskBuilding.inputs.fireRisk=0;riskBuilding.inputs.fireDirection=5;riskBuilding.inputs.workers=8;
    positiveRisk.buildingInputs->at(7)=riskBuilding.inputs;positiveRisk.freeFigureSlots={19};
    const auto ignited=originalBuildingStateAfterUpdates(riskBuilding,positiveRisk,1);
    need(ignited.state&&ignited.state->inputs.damageRisk==0&&ignited.state->inputs.fireRisk==0&&ignited.state->inputs.distance==0&&ignited.state->inputs.burning==6&&ignited.state->inputs.workers==0&&ignited.state->inputs.fireproof==1,"ignition counter threshold calls42A700 and initializes burning from saved direction low bits");
    need(ignited.context->createdFigureRecords.size()==1&&ignited.context->createdFigureRecords[0].slot==19&&ignited.context->createdFigureRecords[0].type==2&&ignited.context->createdFigureRecords[0].delivery==7&&ignited.context->createdFigureRecords[0].subtype==33&&ignited.context->createdFigureRecords[0].speed==1&&ignited.context->disasters->figureEffects[0].animation==1,"ignition publishes finalized type2 figure and separate16C animation without speech or CRT draws");
    auto exhaustedFire=positiveRisk;exhaustedFire.freeFigureSlots.clear();exhaustedFire.figureSpeechCounters.reset();exhaustedFire.figureSpeechCounts.reset();
    const auto noFlame=originalBuildingStateAfterUpdates(riskBuilding,exhaustedFire,1);
    need(noFlame.state&&noFlame.state->inputs.burning==6&&noFlame.context->createdFigureRecords.empty(),"fire allocation exhaustion preserves ignition and requires no unused speech inputs");
    auto ignitionContinuation=*ignited.context;ignitionContinuation.phase=46;
    const auto afterIgnition=originalBuildingStateAfterUpdates(*ignited.state,ignitionContinuation,1);
    need(afterIgnition.state&&afterIgnition.context->createdFigureRecords.empty()&&afterIgnition.context->removedFigureRecords.empty()&&afterIgnition.context->allocatedFigures==std::vector<size_t>{19},"created records describe only this call while cumulative allocator bookkeeping survives continuation");
    auto cart=allocatingHunter;cart.building.type=43;cart.inputs.output=11;cart.inputs.primaryFigure=0;cart.inputs.upgrade=1;cart.production->industry[0]=0x40;cart.production->industry[1]=1;
    cart.production->primaryResource=18;cart.production->primaryStock=150;cart.production->secondaryResource=20;cart.production->secondaryStock=200;
    auto cartContext=harvesting;cartContext.industryCarts=true;cartContext.freeFigureSlots={19};cartContext.productionInputs=std::map<size_t,OriginalBuildingProductionInputs>{{7,*cart.production}};
    cartContext.buildings->at(7)=cart.building;cartContext.buildingInputs->at(7)=cart.inputs;
    const auto shipped=originalBuildingStateAfterUpdates(cart,cartContext,1);
    need(shipped.state&&shipped.state->production->industry[0]==0&&shipped.state->production->industry[1]==0&&shipped.state->production->primaryStock==50&&shipped.state->production->secondaryStock==100&&shipped.state->inputs.upgrade==0,"Kiln cart consumes progress and both100 input stocks before allocation using literal runtime70 flag");
    need(shipped.context->createdFigureRecords.size()==1&&shipped.context->createdFigureRecords[0].type==19&&shipped.context->createdFigureRecords[0].amount==400&&shipped.context->createdFigureRecords[0].subtype==11&&shipped.context->createdFigureRecords[0].action==21&&shipped.context->createdFigureRecords[0].counter==30&&shipped.state->inputs.primaryFigure==19&&shipped.context->dispatchedProduction[11]==400,"Kiln publishes finalized cart19 amount400 pottery action21 reciprocal link and production total");
    auto exhaustedCart=cartContext;exhaustedCart.freeFigureSlots.clear();exhaustedCart.figureSpeechCounts.reset();const auto lostShipment=originalBuildingStateAfterUpdates(cart,exhaustedCart,1);
    need(lostShipment.state&&lostShipment.state->production->primaryStock==50&&lostShipment.context->createdFigureRecords.empty()&&lostShipment.context->dispatchedProduction[11]==0,"exhausted cart allocator still consumes industry inputs without publishing shipment totals or unused speech");
    auto clay=cart;clay.building.type=35;clay.inputs.upgrade=0;clay.inputs.output=18;clay.production->industry[0]=160;clay.production->industry[1]=0;
    cartContext.buildings->at(7)=clay.building;cartContext.buildingInputs->at(7)=clay.inputs;
    const auto rawShipment=originalBuildingStateAfterUpdates(clay,cartContext,1);
    need(rawShipment.state&&rawShipment.context->createdFigureRecords[0].amount==100&&rawShipment.context->createdFigureRecords[0].action==17,"Clay160 threshold dispatches100 raw resource18 with action17 rather than generic delivery6");
    auto legacyCart=cartContext;legacyCart.industryCarts.reset();const auto refusedCart=originalBuildingStateAfterUpdates(clay,legacyCart,1);
    need(!refusedCart.state&&refusedCart.blocker==OriginalBuildingUpdateBlocker::BuildingVirtual20,"positive industry cart remains opt in for legacy callers");
    auto supplier=service;supplier.stallSuppliers=true;supplier.goodsRecords=std::vector<OriginalBuildingGoodsRecord>(2);supplier.goods.emplace();supplier.goods->policies.resize(2);supplier.movement.emplace();supplier.movement->cells.resize(51984);
    auto hempStall=stallState;hempStall.building.type=67;hempStall.inputs.parentSlot=6;hempStall.inputs.spawnCounter=9;
    supplier.stallHookRoadAccess=std::map<size_t,OriginalBuildingRoadAccess>{{7,{1,3,4,std::nullopt,true}}};
    supplier.buildings->emplace(6,stallParent);supplier.buildingInputs->emplace(6,OriginalBuildingUpdateInputs{});supplier.buildingLinks->emplace(6,OriginalFigureBuildingLink{});supplier.buildingLinks->at(6).marketStorage=std::vector<OriginalStorageCompartment>(1);
    need(originalBuildingStallSupplier(hempStall,supplier,19)==OriginalBuildingUpdateBlocker::None&&hempStall.inputs.supplierStatus==1&&hempStall.inputs.spawnCounter==0&&supplier.allocatedFigures.empty(),"empty qualified supplier search resets counter and reports status1 without fabricated stock");
    supplier.goods->unavailable[19]=1;
    need(originalBuildingStallSupplier(hempStall,supplier,19)==OriginalBuildingUpdateBlocker::None&&hempStall.inputs.supplierStatus==3,"unavailable resource19 reports supplier status3 before allocation");
    supplier.goods->unavailable[19]=0;auto sourceBay=hempStall.building;sourceBay.type=54;sourceBay.slot=8;sourceBay.x=4;
    supplier.buildings->emplace(8,sourceBay);supplier.buildingInputs->emplace(8,OriginalBuildingUpdateInputs{});auto& sourceLink=supplier.buildingLinks->operator[](8);sourceLink.type=54;sourceLink.alive=1;sourceLink.roadAccess=1;sourceLink.roadDistance=1;sourceLink.roadX=4;sourceLink.roadY=4;sourceLink.storage=std::vector<OriginalStorageCompartment>{{19,100,400}};
    supplier.goodsRecords->at(1).alive=1;supplier.goodsRecords->at(1).owner=8;supplier.movement->cells[1145]=4;supplier.poolHookRoadAccess->operator[](8)={1,4,4,std::nullopt,true};supplier.figureSpeechCounts->at(254)=3;supplier.figureSpeechCounts->at(266)=5;supplier.figureSpeechCounters->at(266)=7;supplier.freeFigureSlots={19};
    need(originalBuildingStallSupplier(hempStall,supplier,19)==OriginalBuildingUpdateBlocker::None&&hempStall.inputs.word30==19&&supplier.figures->at(19).delivery==8&&supplier.figures->at(19).destinationX==4&&supplier.figures->at(19).action==6&&supplier.supplierRequests.at(19)==std::array<int32_t,7>{1,-1,-1,-1,-1,-1,-1},"positive supplier flood chooses stocked Warehouse and publishes type24 delivery request category1");
    need(supplier.figureSpeechCounters->at(254)==1&&supplier.figureSpeechCounters->at(266)==8&&supplier.figureSpeechChoices.at(19)==2,"Stall supplier applies constructor254 then descriptor266 speech rotation");
    auto burning=positiveRisk;burning.phase=43;burning.buildingInputs->at(7)=ignited.state->inputs;burning.buildingInputs->at(7).burning=3;burning.buildingInputs->at(7).fireDirection=0;
    auto spreading=*ignited.state;spreading.inputs=burning.buildingInputs->at(7);
    const auto missingWind=originalBuildingStateAfterUpdates(spreading,burning,1);
    need(!missingWind.state&&!missingWind.context&&missingWind.blocker==OriginalBuildingUpdateBlocker::FireWind&&missingWind.refusedPhase==43&&burning.buildingInputs->at(7).burning==3,"positive spread requires actual process wind only after periodic and random gates and refuses atomically");
    auto collapse=positiveRisk;collapse.phase=44;collapse.buildingModels->at(33)[7]=1;collapse.resourceGrowth.emplace();auto& ruinSurface=collapse.resourceGrowth->surface;
    ruinSurface.image.resize(51984);ruinSurface.subTile.resize(51984,0x9f);ruinSurface.draw.resize(51984);ruinSurface.feature.resize(51984,4);
    collapse.irrigation.emplace();collapse.irrigation->image.resize(51984);collapse.irrigation->owners.resize(51984);collapse.disasters->freeBuildingSlots=std::vector<size_t>{8};collapse.disasters->buildingSerial=37;collapse.disasters->ruinImageBase=100;collapse.disasters->effectPlacementCulled=true;collapse.freeFigureSlots={19};
    auto destroyed=riskBuilding;destroyed.building.size=1;destroyed.inputs.damageRisk=0;destroyed.inputs.fireRisk=999;collapse.buildings->at(7)=destroyed.building;collapse.buildingInputs->at(7)=destroyed.inputs;collapse.terrainOwners->at(1144)=7;
    const auto ruins=originalBuildingStateAfterUpdates(destroyed,collapse,1);
    need(ruins.state&&ruins.state->inputs.alive==4&&ruins.context->disasters->createdBuildings.size()==1&&ruins.context->disasters->destroyedBuildings==std::vector<size_t>{7}&&ruins.context->buildingInputs->at(8).alive==3&&ruins.context->buildingInputs->at(8).burning==0&&ruins.context->disasters->buildingSerial==38,"collapse counter threshold replaces original with ascending free-slot ruin161 and retains original tombstone identity");
    need(ruins.context->terrainOwners->at(1144)==8&&ruins.context->terrain->at(1144)==0x1008&&ruins.context->resourceGrowth->surface.image[1144]==101&&ruins.context->createdFigureRecords.empty(),"ruin terrain612 image and owner use threshold argument0 without flames while viewport culling skips debris");
    auto burningRuin=collapse;need(originalBuildingDestroy(burningRuin,7,true)==OriginalBuildingUpdateBlocker::None&&burningRuin.buildingInputs->at(8).burning==2&&burningRuin.figures->at(19).delivery==8&&burningRuin.figures->at(19).subtype==161,"active fire destruction argument1 independently creates burning ruins and flame2");
    auto musicService=service;musicService.additionalWalkers=true;musicService.employmentModel=OriginalBuildingEmployment{{211,8}};musicService.figureSpeechCounts->at(276)=3;
    auto musician=allocatingWell;musician.building.type=211;musician.inputs.spawnCounter=3;musicService.buildings->at(7)=musician.building;musicService.buildingInputs->at(7)=musician.inputs;
    const auto musicWalker=originalBuildingStateAfterUpdates(musician,musicService,1);
    need(musicWalker.state&&musicWalker.context->createdFigureRecords.size()==1&&musicWalker.context->createdFigureRecords[0].type==34&&musicWalker.context->createdFigureRecords[0].counter==30,"Music strict greater than3 allocates shared service34 through opt in without wall time");
    auto annual=savedPart2;annual.month=1;annual.monthlyServices=true;annual.trade->partners[0].faction=1;annual.trade->partners[0].relations[21].annualFlag=9;annual.trade->partners[0].relations[20].annualFlag=7;
    need(originalBuildingCycleTailStep(annual)==OriginalBuildingUpdateBlocker::None&&annual.trade->partners[0].relations[21].annualFlag==0&&annual.trade->partners[0].relations[20].annualFlag==7,"annual false444160 branch clears only current faction annual flag and preserves other children");
    auto annualWar=savedPart2;annualWar.month=1;annualWar.monthlyServices=true;annualWar.trade->partners[0].faction=1;annualWar.trade->partners[0].relations[21].state=2;annualWar.trade->partners[0].relations[21].war=0;
    need(originalBuildingCycleTailStep(annualWar)==OriginalBuildingUpdateBlocker::MonthlyCity,"positive annual offer or war service remains refused rather than inferred from absent event state");
    hookBytes[hookAt+4]=0x34;hookBytes[hookAt+5]=0x12;hookBytes[hookAt+8]=99;
    need(originalBuildingSavedDisasterContext(hookBytes,0,restoredHook)&&restoredHook.disasters->buildingSerial==0x1234,"saved building allocation serial follows hook countdown at53167D and excludes following decoy");
    exact.resize(184+140);exact16(18,194);exact16(184+130,1);exact16(184+132,1);exact16(184+138,1);exact16(2+56,0x1234);bay32(2+105,0xfffffffd);
    const auto extraCommon=originalBuildingUpdateInputs(exact,p);
    need(extraCommon&&extraCommon->at(7).nextPart==0x1234&&extraCommon->at(7).supplierStatus==-3,"additive Common5 next3E and supplier74 retain signed56 and105 wire values without legacy changes");
    saved32(10722,0x1234);saved32(10722+4*35,0xfffffffd);saved32(10578,99);
    need(originalBuildingSavedCityContext(savedInputs,anchor,restoredCity)&&restoredCity.dispatchedProduction[0]==0x1234&&restoredCity.dispatchedProduction[35]==-3,"nested Population7 production array reads59487B wire10722 independently of prior trade7 array");
    auto fortification=tailContext;fortification.monthPart=3;fortification.monthlyServices=true;fortification.trade.emplace();fortification.statistics.emplace();fortification.statistics->currentFaction=0;fortification.trade->partners[0].faction=0;fortification.trade->partners[0].kind=0;fortification.trade->partners[0].fortified=1;fortification.activeSide=1;fortification.cropBase=229;fortification.terrain=std::vector<uint32_t>(51984);fortification.terrainOwners=std::vector<uint16_t>(51984);
    for(unsigned n=0;n<99;++n)fortification.terrain->at(229+n)=0x4000;const auto belowFort=fortification;
    need(originalBuildingCycleTailStep(fortification)==OriginalBuildingUpdateBlocker::None&&fortification.trade->partners[0].fortified==0,"part4 current city fortification rescans99 wall cells and clears prior flag");
    fortification=belowFort;fortification.terrain->at(328)=0x8000;
    need(originalBuildingCycleTailStep(fortification)==OriginalBuildingUpdateBlocker::None&&fortification.trade->partners[0].fortified==1,"part4 inclusive100 terrain wall cells sets fortified using full228 row scan");
    auto debrisContext=positiveRisk;debrisContext.disasters->effectPlacementCulled=false;debrisContext.disasters->visualEffectCount=2999;debrisContext.freeFigureSlots={19,18};
    need(originalBuildingDebris(debrisContext,3,4,1)==OriginalBuildingUpdateBlocker::None&&debrisContext.disasters->visualEffectCount==3000&&debrisContext.allocatedFigures==std::vector<size_t>{19}&&debrisContext.figures->at(19).effectFlags==2&&debrisContext.figures->at(19).freeMovement==1,"debris respects actual3000 process effect limit between individual allocations");
    auto missingDebrisCount=positiveRisk;missingDebrisCount.disasters->effectPlacementCulled=false;
    need(originalBuildingDebris(missingDebrisCount,3,4,1)==OriginalBuildingUpdateBlocker::MissingContext,"visible debris requires consumed process effect count rather than defaulting it to zero");
}
void original(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    auto root=ResourceRoot::fromAbsolute(env);need(root.configured(),"configured original root");
    auto catalog=enumerateOriginalMapSources(root);need(catalog.complete,"complete read-only source catalog");
    auto archive=loadSG3(root.resolve("DATA/China_General.sg3"));
    auto ground=loadSG3(root.resolve("DATA/China_Terrain.sg3")),elevation=loadSG3(root.resolve("DATA/China_Elevation.sg3"));
    const std::map<uint16_t,size_t> expected{{2,10},{3,8},{5,26},{7,26},{33,12},{35,12},{43,8},{53,4},{54,6},{58,2},{59,2},{62,4},{66,2},{67,2},{71,2},{72,2},{124,6},{126,6},{127,2},{192,6},{193,8},{194,287},{196,490},{207,2},{211,2},{214,2},{231,126}};
    std::vector<uint8_t> modelBytes;need(detail::readFile(root.resolve("Model/EmperorBuildingModels.txt"),1048576,modelBytes),"bounded production employment model");
    const auto employment=parseOriginalBuildingEmployment(std::string(modelBytes.begin(),modelBytes.end()));need(bool(employment),"difficulty-invariant production employment parsed");
    const auto houseModels=originalFigureHouseCapacities(std::string(modelBytes.begin(),modelBytes.end()));need(bool(houseModels),"existing cited house capacity model column");
    const auto houseTaxes=originalFigureHouseModelColumn(std::string(modelBytes.begin(),modelBytes.end()),18);need(bool(houseTaxes),"house tax model column18 is difficulty invariant");
    std::vector<uint8_t> figureModelBytes;need(detail::readFile(root.resolve("Model/EmperorFigureModels.txt"),100000,figureModelBytes),"bounded figure model input");
    std::vector<uint8_t> speechBytes;need(detail::readFile(root.resolve("EmperorText.eng"),8u*1024u*1024u,speechBytes),"bounded original speech group metadata");
    const auto speechCounts=originalBuildingSpeechCounts(speechBytes);need(bool(speechCounts),"framed EmperorText group counts");
    for(unsigned count:{1u,10u,20u,48u,51u,100u,153u,200u}){
        std::map<uint16_t,size_t> live,tickComputed,productionComputed,selectorInputs,changedImages,decodedImages;
        std::map<std::string,size_t> tickRefused,productionRefused;size_t saves=0,consistent=0,growthRecords=0,serviceRecords=0,serviceChanged=0,serviceBytes=0;
        size_t roadComputed=0,roadRefused=0,industryChanged=0,industryGated=0,phase21Computed=0,phase22Computed=0,accountingComputed=0;
         std::map<uint16_t,size_t> tickSelectorInputs,tickImages,tickDecoded,industrialChanges;
         std::map<uint16_t,size_t> createdFigures,removedFigures;size_t figureRecordOutcomes=0;
         std::map<std::pair<uint16_t,uint32_t>,size_t> typeRefused;
        std::map<uint32_t,size_t> refusedPhases;std::map<size_t,std::pair<uint32_t,uint32_t>> capacityTotals;
        for(const auto& source:catalog.sources)if(source.kind==OriginalMapSourceKind::SaveFile){++saves;
            auto snapshot=loadOriginalSave(root.resolve(source.relativePath));need(snapshot.status==OriginalSaveStatus::LoadedSnapshot&&snapshot.readStable,"stable original save");
            std::vector<uint8_t> raw;need(detail::readFile(root.resolve(source.relativePath),4194304,raw),"bounded original read");auto container=decodeOriginalMapContainer(raw);need(container.complete,"closed original container");
            auto parsed=originalBuildingUpdateInputs(container.payload,snapshot.buildings);need(bool(parsed),"all original building update records framed from public parser");
            auto industry=originalBuildingProductionInputs(container.payload,snapshot.buildings);need(bool(industry),"all industrial production inputs framed independently");
            auto houses=originalBuildingHouseCapacityInputs(container.payload,snapshot.buildings);need(bool(houses),"all house capacity inputs independently framed");
            auto accounting=originalBuildingSavedCityEmployment(container.payload,snapshot.sizeOffset);need(bool(accounting),"nested Population7 restores city employment block");
            accounting->shortageNotice=false;accounting->unemploymentNotice=false; //explicit notification-history premises, never SAV fields.
            const auto account=originalBuildingCityEmploymentStep(*accounting);need(bool(account),"first accounting service closes on all six original snapshots");++accountingComputed;
            constexpr int32_t available[]={0,234,397,11,0,11},shortage[]={0,154,2,35,0,35};
            need(saves<=6&&accounting->available==available[saves-1]&&account->state.employed==available[saves-1]&&
                account->state.shortage==shortage[saves-1]&&account->state.unemployed==0,"original employment totals derived from nested serialized table");
            if(count==1)std::cout<<"{\"accounting_source\":"<<saves<<",\"available\":"<<accounting->available<<",\"employed_after\":"<<account->state.employed<<",\"unemployed_after\":"<<account->state.unemployed<<",\"shortage_after\":"<<account->state.shortage<<"}\n";
            std::optional<OriginalBuildingRoadInputs> roads;std::optional<OriginalFigurePassability> movement;bool ordinaryRoadExit=false;
            {
                const auto endpoints=originalBuildingSavedRoadEndpoints(container.payload,snapshot.sizeOffset);
                const auto spawn=originalFigureSpawnServices(container.payload,snapshot.sizeOffset);
                need(endpoints&&spawn&&snapshot.layers.ground,"SAV19 scenario road endpoints and existing ground input contract");
                const auto scene=composeOriginalSavedGroundScene(snapshot.terrain,snapshot.buildings,*snapshot.layers.ground,archive,ground,elevation);
                auto surface=scene.surface;surface.terrain=scene.rebuiltMovementTerrain;
                auto masks=originalFigureMovementPassability(surface,snapshot.buildings,snapshot.layers.ground->owner,*spawn);
                if(masks){movement=*masks;const int entry=int(spawn->cropBase)+endpoints->at(0)+228*endpoints->at(1);
                    need(entry>=0&&entry<51984,"saved road entry has valid backing coordinate");
                    auto distances=originalBuildingRoadDistances(masks->cells,uint32_t(entry));need(bool(distances),"original movement grid satisfies road flood memory bounds");
                    OriginalFigureUpdateContext networkContext;networkContext.terrain=surface.terrain;networkContext.passability=masks;networkContext.cropBase=spawn->cropBase;networkContext.activeSide=spawn->side;
                    const auto networks=originalFigureRoadNetworks(networkContext);need(bool(networks),"existing cited network producer");
                    const auto ranked=originalBuildingRankedRoadNetworks(*networks);need(bool(ranked),"ranked road network producer");
                    roads.emplace();roads->cropBase=spawn->cropBase;roads->terrain=surface.terrain;roads->distance=std::move(*distances);
                    roads->owners=snapshot.layers.ground->owner;roads->networks=*networks;roads->rankedNetworks=*ranked;roads->ownerTypes=snapshot.buildings.slotTypes;
                    const int exit=int(spawn->cropBase)+endpoints->at(2)+228*endpoints->at(3);
                    ordinaryRoadExit=exit>=0&&exit<51984&&roads->distance[size_t(exit)]!=0;
                    for(const auto& h:snapshot.buildings.footprints)if(originalBuildingIsHouse(h.type)&&!originalBuildingHouseRoadAccess(h,*roads))ordinaryRoadExit=false;
                    if(ordinaryRoadExit)for(const auto& h:snapshot.buildings.footprints)if(originalBuildingIsHouse(h.type))houses->at(h.slot).distance=int16_t(originalBuildingHouseRoadAccess(h,*roads)->distance);
                    need(ordinaryRoadExit,"five rebuilt original grids reach scenario exit and every house ordinary branch");
                }
            }
            auto buildings=snapshot.buildings.footprints;
            for(const auto& object:snapshot.buildings.objectBuildings){OriginalBuildingFootprint f;f.type=object.type;f.slot=object.slot;f.size=object.size;f.x=object.x;f.y=object.y;f.recordOffset=object.recordOffset;f.imageInputs=OriginalBuildingImageInputs{};f.imageInputs->gridIndex=object.gridIndex;buildings.push_back(f);}
            OriginalBuildingUpdateContext cityContext(OriginalBuildingUpdateScope::AdmittedTicks,true);
            need(originalBuildingSavedCityContext(container.payload,snapshot.sizeOffset,cityContext),"complete nested city context");
            cityContext.cycleTail->alternateMode=false; //10DE044 explicit ordinary live-mode premise.
            cityContext.disasters.emplace(); //opt-in42A700 service;no clock or CRT premise.
            cityContext.stallSuppliers=true;
            cityContext.monthlyServices=true;
            cityContext.additionalWalkers=true;
            cityContext.industryCarts=true;
            cityContext.cycleTail->previousHealthRating=0; //explicitD6241C notification history
            cityContext.disasters->freeBuildingSlots=originalBuildingSavedInactiveSlots(container.payload,snapshot.buildings);
            need(bool(cityContext.disasters->freeBuildingSlots)&&originalBuildingSavedDisasterContext(container.payload,snapshot.terrainOffset,cityContext),"complete framed inactive building slots and nested saved serial for disaster allocations");
            cityContext.disasters->effectPlacementCulled=false; //explicit417460 viewport premise:effect cells visible.
            cityContext.disasters->visualEffectCount=0; //explicit8900E8 process effect count
            if(snapshot.layers.ground){cityContext.cycleTail->cropQuality=snapshot.layers.ground->state.gEAC3F0;cityContext.cycleTail->waterMode=snapshot.layers.ground->waterMode;}
            need(bool(snapshot.layers.figures),"complete original figure pool");cityContext.figures=snapshot.layers.figures->slots;
            cityContext.formations=originalBuildingSavedFormations(container.payload,snapshot.terrainOffset);
            cityContext.figureStationary=originalBuildingSavedFigureStationary(container.payload,*snapshot.layers.figures);
            cityContext.figureModels=originalFigureModels(std::string(figureModelBytes.begin(),figureModelBytes.end()),originalFigureSavedDifficulty(container.payload,snapshot.sizeOffset));
             cityContext.figureServices=originalFigureSavedServices(container.payload,snapshot.terrainOffset);
             cityContext.cropServices=originalFigureSavedCropServices(container.payload,snapshot.terrainOffset,snapshot.sizeOffset);
             need(bool(cityContext.cropServices),"nested SAV crop service restores month unavailable and per-type disabled inputs");
             cityContext.figureSpeechCounters=originalBuildingSavedSpeechCounters(container.payload,snapshot.terrainOffset);cityContext.figureSpeechCounts=speechCounts;
             need(bool(cityContext.figureSpeechCounters),"saved speech rotation counters restored without process defaults");
            cityContext.buildingLinks=originalFigureBuildingLinks(container.payload,snapshot.buildings);
            cityContext.goods=originalFigureSavedGoodsServices(container.payload,snapshot.layers.figures->end,snapshot.sizeOffset,snapshot.terrainOffset);
            cityContext.goodsRecords=originalBuildingSavedGoodsRecords(container.payload,snapshot.layers.figures->end);
            cityContext.religion=originalBuildingSavedReligion(container.payload,snapshot.sizeOffset);need(bool(cityContext.religion),"framed saved religion entries and scenario enabled bytes");
            cityContext.religion->previousRating=0;cityContext.religion->selected=-1; //explicit scratch inputs1312F78/85D720.
            const auto empireStates=originalBuildingSavedEmpireStates(container.payload,snapshot.layers.figures->end,&cityContext);
            need(bool(empireStates),"complete framed Empire3 states for phase13");cityContext.statistics->empireState=*empireStates;
            need(originalBuildingSavedTradeContext(container.payload,snapshot.sizeOffset,cityContext),"nested Population7 restores trade counters independently of Empire grids");
            cityContext.statistics->religionBonusEnabled=false; //explicit10BFB50 process premise.
            need(cityContext.formations&&cityContext.figureStationary&&cityContext.figureModels&&cityContext.figureServices&&cityContext.buildingLinks&&cityContext.goods,"all independent formation model storage services supplied");
            cityContext.buildingInputs=*parsed;cityContext.buildings.emplace();for(const auto& b:buildings)cityContext.buildings->emplace(b.slot,b);
            cityContext.houseTaxModel=*houseTaxes;cityContext.poolRoadAccess.emplace();cityContext.poolRoadReachable.emplace();cityContext.storageRoadStatus.emplace();cityContext.poolRoadSelected.emplace();cityContext.poolHookRoadAccess.emplace();cityContext.productionInputs=*industry;
            cityContext.employmentModel=*employment;cityContext.buildingUnavailable.emplace();
            //Explicit initial process scratch:30..35 survive phase33 reset.
            cityContext.buildingTotals.emplace();
            cityContext.buildingModels=originalBuildingModels(std::string(modelBytes.begin(),modelBytes.end()),unsigned(cityContext.labor->difficulty));
            cityContext.houseModels=originalBuildingHouseModels(std::string(modelBytes.begin(),modelBytes.end()),unsigned(cityContext.labor->difficulty));
            cityContext.evolutionDisabled=false;cityContext.forceHouseEvolution=false;
            cityContext.cropDispatchModel=std::array<int32_t,14>{5,3,4,2,3,2,10,6,8,4,3,4,50,8};
            cityContext.cropDispatchRadius=std::array<int32_t,2>{3,3};cityContext.cropFestival=originalBuildingSavedCropFestival(container.payload);
            need(bool(cityContext.cropFestival),"nested Festival4 restores crop ranking predicate without process premise");
            cityContext.effectDecayCounter=0;cityContext.effectGrid.emplace(51984);
            for(size_t n=0;n<51984;++n)cityContext.effectGrid->at(n)=detail::u32(container.payload.data()+snapshot.terrainOffset+original_save19::serviceGrid+4*n);
            need(bool(cityContext.buildingModels),"building thirteen-column model applies explicit saved difficulty");
            for(const auto& b:buildings){const auto& i=parsed->at(b.slot);const auto resource=originalBuildingSavedUnavailable(container.payload,snapshot.sizeOffset,i.output);
                need(bool(resource),"per-building output availability");cityContext.buildingUnavailable->emplace(b.slot,!i.enabled||*resource!=0);}
            need(originalBuildingSavedNoticeCountdown(container.payload,snapshot.terrainOffset,cityContext),"saved shortage notification countdown");
             cityContext.highestEverBuildingSlot=0;cityContext.buildingRegistrySize=4000;cityContext.highestBuildingSlot=0;
             for(const auto& b:buildings)*cityContext.highestBuildingSlot=std::max(*cityContext.highestBuildingSlot,uint32_t(b.slot));
            cityContext.cityEmployment->shortageNotice=false;cityContext.cityEmployment->unemploymentNotice=false;
            cityContext.labor->populationNotices=std::array<bool,10>{};
            for(unsigned n=0;n<10;++n)cityContext.labor->populationNotices->at(n)=container.payload.at(snapshot.terrainOffset+1363680+n)!=0;
             //Explicit isolated process inputs: notification flags false,coverage
             //scratch0,migrant spacing0,placement check false. CRT/wall clock remain absent;
             //migrant allocation does not consume either input.
             //Animal region and shortage countdown are restored SAV values.
             cityContext.serviceCoverage=std::array<int32_t,11>{};cityContext.labor->spacing=0;cityContext.scenarioFigurePlacementCheck=false;
            cityContext.figureCellHeads=snapshot.layers.figures->cellHeads;
            need(originalBuildingSavedHookContext(container.payload,snapshot.terrainOffset,cityContext),"hook countdown and walker-disabled byte restored by SAV readers");
             if(count==1)std::cout<<"{\"city_source\":"<<saves<<",\"buildings\":"<<buildings.size()<<",\"population\":"<<*cityContext.populationTotal<<",\"pending_statistics\":"<<cityContext.labor->statisticsPending<<",\"health\":"<<cityContext.labor->health<<",\"sentiment\":"<<cityContext.labor->sentiment<<",\"tax_rate\":"<<cityContext.accounting->taxRate<<",\"phase\":"<<detail::u32(container.payload.data()+snapshot.terrainOffset+original_save19::c82ef8)<<",\"month_part\":"<<detail::u32(container.payload.data()+snapshot.terrainOffset+original_save19::c82ef0)<<",\"month\":"<<detail::u32(container.payload.data()+snapshot.sizeOffset-176)<<"}\n";
            for(size_t n=1999;n>0;--n)if(!cityContext.figures->at(n).alive)cityContext.freeFigureSlots.push_back(n);
            if(roads){cityContext.cropBase=roads->cropBase;cityContext.terrain=roads->terrain;cityContext.movement=movement;cityContext.activeSide=originalFigureSpawnServices(container.payload,snapshot.sizeOffset)->side;
                cityContext.terrainOwners=snapshot.layers.ground->owner;cityContext.terrainRandom=snapshot.layers.ground->state.randomByte;
                for(const auto& b:buildings){const auto access=b.type==59||b.type==62||b.type==66||b.type==67?
                     originalBuildingMultipartRoadAccess(b,snapshot.buildings,*roads,parsed->at(b.slot).parentSlot):originalBuildingIsHouse(b.type)?originalBuildingHouseRoadAccess(b,*roads):originalBuildingRoadAccess(b,*roads);
                     need(bool(access),"complete pool road output");cityContext.poolRoadAccess->emplace(b.slot,*access);cityContext.poolRoadReachable->emplace(b.slot,access->distance!=0);
                     if(b.type==33||b.type==35||b.type==43||b.type==53||b.type==72||b.type==124||b.type==127||b.type==192||b.type==193||b.type==207||b.type==211||b.type==214||b.type==71){const auto adjacent=originalBuildingHouseRoadAccess(b,*roads,false);if(adjacent){cityContext.poolRoadSelected->emplace(b.slot,adjacent->selected);cityContext.poolHookRoadAccess->emplace(b.slot,*adjacent);}else if(b.type==33||b.type==35||b.type==43||b.type==53)need(false,"complete pool virtual58 adjacent-road selection");}
                     if(b.type==53||b.type==54||b.type==58){const auto ranked=originalBuildingHouseRoadAccess(b,*roads,false);need(bool(ranked),"storage ranked road query closes");
                     cityContext.poolRoadSelected->operator[](b.slot)=ranked->selected;cityContext.poolHookRoadAccess->operator[](b.slot)=*ranked;
                    uint8_t status=ranked->selected?1:0;
                    if(!status&&b.type==53){auto fallback=b;fallback.size=5;const auto wider=originalBuildingHouseRoadAccess(fallback,*roads,false);need(bool(wider),"Mill model width5 storage fallback");if(wider->selected)status=2;}
                    cityContext.storageRoadStatus->emplace(b.slot,status);}
                    }
                cityContext.applyLoadRoadRefresh=true;
                cityContext.irrigation.emplace();auto& irrigation=*cityContext.irrigation;
                const auto scene=composeOriginalSavedGroundScene(snapshot.terrain,snapshot.buildings,*snapshot.layers.ground,archive,ground,elevation);
                irrigation.image=scene.surface.image;irrigation.subTile=scene.surface.subTile;irrigation.owners=roads->owners;irrigation.ownerTypes=snapshot.buildings.slotTypes;
                need(archive.imageIndex[0x35]>=201,"irrigation435 archive table handle");irrigation.baseImage=(2u<<14)|uint32_t(archive.imageIndex[0x35]-201);
                //524360 registration is the cIrrigation class only (vtable7B6844).
                for(const auto& b:buildings)need(b.type!=57,"original irrigation registry is empty by complete type pool");}
             if(roads){cityContext.resourceGrowth.emplace();auto& gr=*cityContext.resourceGrowth;
                 gr.surface=snapshot.layers.ground->state;gr.surface.image=cityContext.irrigation->image;gr.owners=snapshot.layers.ground->owner;
                  for(uint16_t code:{uint16_t(0x620),uint16_t(0x624),uint16_t(0x625),uint16_t(0x627)}){size_t index=0;need(resolveGroundTileCode(ground,code,index),"resource image uses existing cited terrain table lookup");gr.imageBases.emplace(code,(3u<<14)|uint32_t(index));}
                  size_t ruinIndex=0;need(resolveGroundTileCode(ground,0x612,ruinIndex),"disaster replacement uses existing cited terrain612 selector");cityContext.disasters->ruinImageBase=(3u<<14)|uint32_t(ruinIndex);}
             for(const auto& f:buildings){++live[f.type];OriginalBuildingUpdateState state{f,parsed->at(f.slot),std::nullopt};
                const auto ip=industry->find(f.slot);if(ip!=industry->end())state.production=ip->second;
                const auto access=roads?(f.type==59||f.type==62||f.type==66||f.type==67?
                     originalBuildingMultipartRoadAccess(f,snapshot.buildings,*roads,parsed->at(f.slot).parentSlot):originalBuildingIsHouse(f.type)?originalBuildingHouseRoadAccess(f,*roads):originalBuildingRoadAccess(f,*roads)):std::nullopt;
                if(count==1){if(access)++roadComputed;else ++roadRefused;}
                need(state.inputs.identity==f.slot,"saved building identity equals table slot");
                auto ctx=originalBuildingSavedUpdateContext(container.payload,snapshot.terrainOffset,snapshot.sizeOffset,f,OriginalBuildingUpdateScope::AdmittedTicks,true);need(bool(ctx),"explicit saved city phase and terrain context");
                const auto local=*ctx;*ctx=cityContext;ctx->phase=local.phase;ctx->monthPart=local.monthPart;ctx->cycle=local.cycle;ctx->month=local.month;ctx->terrainFlags=local.terrainFlags;ctx->typeDisabled=local.typeDisabled;
                ctx->unavailable=originalBuildingSavedUnavailable(container.payload,snapshot.sizeOffset,state.inputs.output);
                const auto required=employment->find(f.type);if(required!=employment->end())ctx->requiredWorkers=required->second;
                if(state.production){ctx->festivalProductionBoost=originalBuildingSavedProductionBoost(container.payload,f.type);need(bool(ctx->festivalProductionBoost),"Festival4 production predicate supplied");}
                if(roads){ctx->roadAccess=access;ctx->roadRefreshOrdinaryExit=ordinaryRoadExit;}
                ctx->houses=*houses;ctx->houseModelCapacity=*houseModels;
                if(count==1&&roads&&ordinaryRoadExit){auto passContext=*ctx;passContext.phase=21;
                    auto pass=originalBuildingStateAfterUpdates(state,passContext,1);if(pass.state){++phase21Computed;
                        need(pass.state->roadDistance&&pass.state->roadX&&pass.state->roadY,"road pass publishes every retained road field");}
                    passContext.phase=22;pass=originalBuildingStateAfterUpdates(state,passContext,1);need(bool(pass.state),"complete house capacity pass returns");++phase22Computed;
                    capacityTotals[saves]={*pass.context->cityHouseCapacity,*pass.context->cityHouseFree};
                    const std::array<std::pair<uint32_t,uint32_t>,6> totals{{{0,0},{572,0},{1066,69},{98,70},{0,0},{28,0}}};
                    need(capacityTotals[saves]==totals[saves-1],"phase22 original city capacity and free totals lock all supported sources");}
                if(state.inputs.industry){need(f.imageInputs&&f.imageInputs->cropWord4==originalBuildingCropProgress(state.inputs),"raw industrial progress equals independent existing selector input");++consistent;}
                 auto result=originalBuildingStateAfterUpdates(state,*ctx,count);
                  if(count==1&&f.slot==buildings.front().slot){auto growth=*ctx;
                      const auto resource=originalBuildingResourceGrowthStep(growth);
                      need(resource==OriginalBuildingUpdateBlocker::None,"complete original terrain-resource phase closes with saved engine random");
                      std::cout<<"{\"resource_source\":"<<saves<<",\"changed\":"<<growth.resourceGrowth->changed<<",\"image_writes\":"<<growth.resourceGrowth->imageWrites<<",\"random_draws\":"<<growth.resourceGrowth->randomCalls<<",\"mature_trees\":"<<growth.resourceGrowth->matureTrees.size()<<",\"mature_reeds\":"<<growth.resourceGrowth->matureReeds.size()<<"}\n";}
                   if(result.state){++tickComputed[f.type];tickSelectorInputs[f.type]+=result.changedSelectorInputs;
                       createdFigures[f.type]+=result.context->createdFigureRecords.size();removedFigures[f.type]+=result.context->removedFigureRecords.size();++figureRecordOutcomes;
                       need(result.context->createdFigureRecords.size()==result.context->allocatedFigures.size()&&result.context->removedFigureRecords.empty(),"complete original call publishes every new figure record and no unexecuted removals");
                       if(f.slot==buildings.front().slot)std::cout<<"{\"N\":"<<count<<",\"source\":"<<saves<<",\"created_figures\":"<<result.context->createdFigureRecords.size()<<",\"removed_figures\":"<<result.context->removedFigureRecords.size()<<",\"new_ruins\":"<<result.context->disasters->createdBuildings.size()<<",\"supplier_calls\":"<<result.context->stallSupplierCalls<<"}\n";
                       if(count==100&&saves==4){std::map<unsigned,size_t> effectTypes;for(const auto& effect:result.context->createdFigureRecords)++effectTypes[effect.type];
                           need(effectTypes==std::map<unsigned,size_t>{{1,72}}&&result.context->disasters->createdBuildings.size()==25&&result.context->disasters->destroyedBuildings==std::vector<size_t>{5},"original Mill threshold produces25 nonburning ruins72 debris and original slot5 tombstone");}
                       if(f.type==67&&count>=100)need(result.state->inputs.supplierStatus==1&&result.context->stallSupplierCalls>0&&result.context->supplierRequests.empty(),"original Stall67 qualified resource19 search finds no supplier and preserves explicit status1");
                      if(count==1&&f.slot==buildings.front().slot){auto later=*result.context;
                          need(originalBuildingTradeStep(later)==OriginalBuildingUpdateBlocker::None&&later.trade->calls==1&&later.trade->eligible==(saves==2||saves==3?1u:0u),"original nonempty trade context closes four cities with exact eligible partner count");
                          size_t quotas=0;for(const auto& partner:later.trade->partners)for(auto quota:partner.quota)quotas+=quota!=0;
                          need(quotas>0&&later.trade->timersChanged==0&&!later.trade->dispatchDelay,"original trade preserves nonempty quotas and saved timers without assumed dispatch delays");
                          need(originalBuildingTotalsStep(later)==OriginalBuildingUpdateBlocker::None&&later.buildingTotals->calls==1,"original phase33 complete retained type and resource totals close all four cities");
                          need(originalBuildingDesirabilityStep(later)==OriginalBuildingUpdateBlocker::None&&originalBuildingApplyDesirability(later)==OriginalBuildingUpdateBlocker::None,"original square rectangular and terrain influences apply to all four complete pools");
                          size_t affected=0,changedDesirability=0;for(auto value:*later.desirability)affected+=value!=0;
                          for(const auto& entry:*later.buildingInputs)changedDesirability+=entry.second.desirability!=parsed->at(entry.first).desirability;
                          std::cout<<"{\"later_services_source\":"<<saves<<",\"trade_eligible\":"<<later.trade->eligible<<",\"nonzero_quotas\":"<<quotas<<",\"timers_changed\":"<<later.trade->timersChanged<<",\"desirability_cells\":"<<affected<<",\"building_desirability_changed\":"<<changedDesirability<<"}\n";}
                       if((count==10||count>=51)&&f.slot==buildings.front().slot){const unsigned prefixCount=count==10?4:count==51?32:count==100?51:102;const auto prefix=originalBuildingStateAfterUpdates(state,*ctx,prefixCount);
                          need(bool(prefix.state),"original split prefix closes");const auto suffix=originalBuildingStateAfterUpdates(*prefix.state,*prefix.context,count-prefixCount);
                          need(suffix.state&&suffix.context->phase==result.context->phase&&suffix.context->accountingCalls==result.context->accountingCalls&&
                              suffix.context->networkCalls==result.context->networkCalls&&suffix.context->allocatedFigures==result.context->allocatedFigures&&
                              suffix.context->freeFigureSlots==result.context->freeFigureSlots&&suffix.context->populationTotal==result.context->populationTotal,
                               "original split through N200 preserves load order city services and allocator state");
                           need(prefix.context->createdFigureRecords.size()+suffix.context->createdFigureRecords.size()==result.context->createdFigureRecords.size()&&suffix.context->removedFigureRecords.empty(),"split continuation publishes disjoint per-call figure deltas without duplicate allocations");
                          need(suffix.state->inputs.model==result.state->inputs.model&&suffix.state->inputs.modelUpgrade==result.state->inputs.modelUpgrade&&suffix.state->inputs.fireRisk==result.state->inputs.fireRisk&&suffix.state->inputs.damageRisk==result.state->inputs.damageRisk&&suffix.context->cycle==result.context->cycle&&suffix.context->monthPart==result.context->monthPart&&suffix.context->notificationEffects==result.context->notificationEffects,"split continuation retains late-phase model risk cycle and notification writes");}
                      if(count==1&&f.slot==buildings.front().slot){auto serviceContext=*result.context;serviceContext.phase=1;serviceContext.applyLoadRoadRefresh=false;
                          const auto accountState=originalBuildingStateAfterUpdates(*result.state,serviceContext,1);need(bool(accountState.state),"full accounting phase closes original source after road initialization");
                          serviceContext.phase=23;const auto laborState=originalBuildingStateAfterUpdates(*result.state,serviceContext,1);need(bool(laborState.state),"labor and pending population statistics close original source");
                           serviceContext=*laborState.context;serviceContext.phase=25;const auto jobs=originalBuildingStateAfterUpdates(*laborState.state,serviceContext,1);need(bool(jobs.state),"employment distribution closes original source");
                           constexpr int32_t populations[]={0,572,1012,28,0,28},laborAvailable[]={0,234,406,11,0,11},employedTotals[]={0,234,399,11,0,11};
                           need(laborState.context->populationTotal==populations[saves-1]&&jobs.context->cityEmployment->available==laborAvailable[saves-1]&&
                               jobs.context->cityEmployment->employed==employedTotals[saves-1]&&laborState.context->allocatedFigures.empty(),"original labor population and employment totals use complete saved city inputs");
                           need(!laborState.context->crtRandom&&!laborState.context->wallClock&&laborState.context->crtRandomCalls==0,"original reached labor needs no fabricated CRT seed or wall clock");
                          const auto& a=*accountState.context->accounting;const auto& d=*accountState.context->cityDerived;
                          std::cout<<"{\"city_services_source\":"<<saves<<",\"taxable_ordinary\":"<<a.taxableOrdinary<<",\"taxable_elite\":"<<a.taxableElite<<",\"food\":"<<d.food<<",\"labor_available\":"<<jobs.context->cityEmployment->available<<",\"employed\":"<<jobs.context->cityEmployment->employed<<",\"allocated_figures\":"<<laborState.context->allocatedFigures.size()<<",\"population_after\":"<<*laborState.context->populationTotal<<"}\n";}
                     if(result.selectorChanged){++tickImages[f.type];const auto bound=bindOriginalBuildingBaseImage(result.state->building,"DATA/China_General.sg3",archive);
                         need(bool(bound),"admitted changed selector binds existing archive");const auto image=decodeSG3Sprite(archive,bound->archiveIndex,root.resolve("DATA/China_General.555"));
                         need(image.status==SG3DecodeStatus::Decoded&&image.image.valid(),"admitted changed image decodes");++tickDecoded[f.type];}
                     if(count==1)need(result.changedSelectorInputs==0&&!result.selectorChanged,"N1 admitted tick keeps all existing image selectors");
                  }else {++tickRefused[originalBuildingUpdateBlockerName(result.blocker)];need(bool(result.refusedPhase),"admitted refusal names its exact phase");++refusedPhases[*result.refusedPhase];
                       ++typeRefused[{f.type,*result.refusedPhase}];
                      need(result.operations<count&&result.operations%51==(*result.refusedPhase+51-*ctx->phase)%51&&!result.context,"atomic refusal reports exact executed prefix without publishing state");
                      if(count>=100&&result.operations>=51){const auto prefix=originalBuildingStateAfterUpdates(state,*ctx,51);need(bool(prefix.state),"later-cycle refusal has a complete first51 prefix");
                          const auto suffix=originalBuildingStateAfterUpdates(*prefix.state,*prefix.context,count-51);
                          need(!suffix.state&&!suffix.context&&suffix.blocker==result.blocker&&suffix.refusedPhase==result.refusedPhase&&suffix.operations+51==result.operations,"second-cycle split refusal preserves exact total completed operations and phase");}}
                ctx->scope=OriginalBuildingUpdateScope::ProductionPasses;result=originalBuildingStateAfterUpdates(state,*ctx,count);
                if(!result.state){++productionRefused[originalBuildingUpdateBlockerName(result.blocker)];continue;}
                ++productionComputed[f.type];selectorInputs[f.type]+=result.changedSelectorInputs;
                if(state.production){if(state.production->industry!=result.state->production->industry){++industryChanged;++industrialChanges[f.type];}else ++industryGated;}
                if(state.inputs.industry&&originalBuildingCropProgress(state.inputs)!=originalBuildingCropProgress(result.state->inputs))++growthRecords;
                if(result.selectorChanged){++changedImages[f.type];auto bound=bindOriginalBuildingBaseImage(result.state->building,"DATA/China_General.sg3",archive);
                    need(bool(bound),"changed crop selector binds existing archive API");auto image=decodeSG3Sprite(archive,bound->archiveIndex,root.resolve("DATA/China_General.555"));need(image.status==SG3DecodeStatus::Decoded&&image.image.valid(),"changed production image word decodes");++decodedImages[f.type];}
                if(originalBuildingIsHouse(f.type)){ctx->scope=OriginalBuildingUpdateScope::HouseServiceDecayPasses;auto service=originalBuildingStateAfterUpdates(state,*ctx,count);need(bool(service.state),"original house service decay closes");++serviceRecords;
                    bool changedService=false;for(unsigned n=0;n<61;++n)if(state.inputs.house->at(n)!=service.state->inputs.house->at(n)){++serviceBytes;changedService=true;}
                    serviceChanged+=changedService;}
            }
        }
        need(saves==6&&live==expected&&consistent==777,"six-source1065/type and777 crop consistency denominators");
        size_t ticks=0,production=0,changed=0,decoded=0,inputs=0;for(const auto& e:tickComputed)ticks+=e.second;for(const auto& e:productionComputed)production+=e.second;
        for(const auto& e:changedImages)changed+=e.second;for(const auto& e:decodedImages)decoded+=e.second;for(const auto& e:selectorInputs)inputs+=e.second;
         need(production==1065&&productionComputed==expected&&productionComputed[194]==287&&productionComputed[196]==490&&productionComputed[59]==2,"production closes all27 types including32 industrial records");
        need(ticks==(count<=20?1065u:1063u),"exact retained admissions through N200 with genuine farmer wall time refusals");
        need(growthRecords==137&&inputs==137&&selectorInputs[194]==114&&selectorInputs[196]==23,"real growth and selector-input changes from saved flags, never fitted deltas");
        if(count==1||count==10||count==100)need(changed==(count==1?9u:count==10?82u:137u)&&changedImages[194]==(count==1?6u:count==10?63u:114u)&&
            changedImages[196]==(count==1?3u:count==10?19u:23u),"per-type image-stage transition denominator locks");
        need(serviceRecords==70,"all70 house service records close");
        need(serviceChanged==52&&serviceBytes==208,"52 of70 saved houses change208 service bytes without selector changes");
         std::map<std::string,size_t> refused;if(count>=48)refused["farmer_wall_clock_766da0_required"]=2;
         need(tickRefused==refused&&productionRefused.empty(),"exact admitted and isolated refusal partitions");
          need(ticks+std::accumulate(refusedPhases.begin(),refusedPhases.end(),size_t(0),[](size_t total,const auto& e){return total+e.second;})==1065,"exact reached scheduler phase partitions");
         std::map<std::pair<uint16_t,uint32_t>,size_t> expectedTypeRefused;
         if(count>=48)expectedTypeRefused[{192,31}]=2;
         need(typeRefused==expectedTypeRefused,"all27 semantic types retain exact refused-phase partitions");
         std::map<uint32_t,size_t> phaseExpected;if(count>=48)phaseExpected[31]=2;
         need(refusedPhases==phaseExpected,"exact first-refused scheduler phases through second cycle");
         for(const auto& e:expected){size_t typeMissing=0;for(const auto& r:expectedTypeRefused)if(r.first.first==e.first)typeMissing+=r.second;
             need(tickComputed[e.first]==e.second-typeMissing,"per-type admitted denominator includes every success and precise refusal");
             const size_t expectedInputs=count<48?0:e.first==194?114:e.first==196?23:e.first==7?3:0;
             const size_t expectedImages=count<48?0:e.first==194?(count==200?25:count==153?19:count==100?11:6):e.first==196?(count>=153?4:3):0;
             need(tickSelectorInputs[e.first]==expectedInputs&&tickImages[e.first]==expectedImages&&tickDecoded[e.first]==expectedImages,"per-type admitted selector inputs image changes and decodes stay independently exact");}
         std::map<uint16_t,size_t> expectedCreated;
         if(count>=48)expectedCreated={{33,4},{72,1},{124,1}};
         if(count>=100){expectedCreated[2]=720;expectedCreated[3]=288;expectedCreated[33]=148;expectedCreated[53]=72;}
         if(count>=153){expectedCreated[33]=149;expectedCreated[211]=1;}
         if(count==200){expectedCreated[35]=1;expectedCreated[43]=1;}
         for(const auto& e:expected)need(createdFigures[e.first]==expectedCreated[e.first]&&removedFigures[e.first]==0,"all27 types assert exact exported figure records across successful independent outcomes");
        need(industryChanged==20&&industryGated==12,"20 of32 industrial records mutate and12 remain gated or saturated");
        need(industrialChanges==std::map<uint16_t,size_t>{{33,4},{35,8},{43,8}},"industrial mutation counts partition exactly by Hunter Clay Kiln");
        need(accountingComputed==6,"accounting allocation service closes6 of6 original cities");
        if(count==1)need(roadComputed==1065&&roadRefused==0&&phase21Computed==1065&&phase22Computed==1065,"road helper and ordinary phase21 close all10 multipart overrides;phase22 covers1065");
        need(changed==decoded,"all changed image words decoded denominator");
        std::cout<<"{\"N\":"<<count<<",\"buildings\":1065,\"admitted_ticks_computed\":"<<ticks<<",\"production_passes_computed\":"<<production<<",\"crop_progress_records_changed\":"<<growthRecords<<",\"image_selector_inputs_changed\":"<<inputs<<",\"image_words_changed\":"<<changed<<",\"image_words_decoded\":"<<decoded<<",\"crop_raw_selector_consistency\":"<<consistent<<",\"house_service_records\":"<<serviceRecords<<",\"house_service_records_changed\":"<<serviceChanged<<",\"house_service_bytes_changed\":"<<serviceBytes<<"}\n";
        need(figureRecordOutcomes==ticks,"figure output denominator equals successful retained outcomes");
        for(const auto& e:live)std::cout<<"{\"N\":"<<count<<",\"type\":"<<e.first<<",\"live\":"<<e.second<<",\"ticks_computed\":"<<tickComputed[e.first]<<",\"tick_selector_inputs_changed\":"<<tickSelectorInputs[e.first]<<",\"tick_images_changed\":"<<tickImages[e.first]<<",\"tick_images_decoded\":"<<tickDecoded[e.first]<<",\"created_figures\":"<<createdFigures[e.first]<<",\"removed_figures\":"<<removedFigures[e.first]<<",\"production_computed\":"<<productionComputed[e.first]<<",\"industry_records_changed\":"<<industrialChanges[e.first]<<",\"selector_inputs_changed\":"<<selectorInputs[e.first]<<",\"images_changed\":"<<changedImages[e.first]<<",\"images_decoded\":"<<decodedImages[e.first]<<"}\n";
         for(const auto& e:tickRefused)std::cout<<"{\"N\":"<<count<<",\"scope\":\"admitted_ticks\",\"blocker\":\""<<e.first<<"\",\"refused\":"<<e.second<<"}\n";
         for(const auto& e:typeRefused)std::cout<<"{\"N\":"<<count<<",\"type\":"<<e.first.first<<",\"refused_phase\":"<<e.first.second<<",\"refused\":"<<e.second<<"}\n";
        for(const auto& e:productionRefused)std::cout<<"{\"N\":"<<count<<",\"scope\":\"production_passes\",\"blocker\":\""<<e.first<<"\",\"refused\":"<<e.second<<"}\n";
        std::cout<<"{\"N\":"<<count<<",\"industry_changed\":"<<industryChanged<<",\"industry_unchanged\":"<<industryGated<<",\"road_service_computed\":"<<roadComputed<<",\"road_service_refused\":"<<roadRefused<<"}\n";
        for(const auto& total:capacityTotals)std::cout<<"{\"phase22_source\":"<<total.first<<",\"city_130F994_capacity\":"<<total.second.first<<",\"city_130F998_free\":"<<total.second.second<<"}\n";
    }
}
}
int main(){try{cycleTailTransactions();authored();const size_t authoredChecks=checks;original();std::cout<<"{\"passed\":true,\"authored_checks\":"<<authoredChecks<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
