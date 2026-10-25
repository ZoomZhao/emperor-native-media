// SPDX-License-Identifier: MIT
// Native v6 labor: authored equivalence against the strict original employment
// step, plus real-data checks via EMPEROR_GAMEDATA_ROOT (unset -> SKIP).
#include "native_session_source.hpp"
#include "native_session_feedback.hpp"
#include <iostream>
#include <random>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool ok,const std::string& why){++checks;if(!ok)throw std::runtime_error(why);}
void act(const NativeSessionSeed& seed,NativeSessionState& s,NativeSessionAction a,int64_t x=0,int64_t y=0,int64_t tx=0,int64_t ty=0){
    NativeSessionCommand c{a,s.revision,x,y,tx,ty};const auto r=transactNativeSession(seed,s,c);need(r.committed,"command refused: "+r.error);
}
NativeSessionSeed flatSeed(uint32_t side){
    NativeSessionSeed s;s.source="labor.sav";s.sourceRevision=1;s.modelRevision=2;s.side=side;s.roadCost=1;s.houseCost=1;s.houseCapacity=100;
    s.wellCost=1;s.hunterCost=1;s.initialFunds=1000000;s.buildable.assign(size_t(side)*side,1);s.sourceRoad.assign(size_t(side)*side,0);
    s.laborPercent=42;s.hunterWorkers=15;s.wellWorkers=4;s.laborPriority.fill(3);return s;
}
// Equivalence: the native planner and the strict 4F19A0/51E310 step must
// assign identical workers for the same candidates, labor and priorities.
void equivalence(){
    std::mt19937 rng(58);const auto pick=[&](uint32_t n){return uint32_t(rng()%n);};
    size_t cases=0,buildings=0,shortageCases=0,mixedPriority=0;
    for(unsigned trial=0;trial<400;++trial){
        auto seed=flatSeed(40);seed.hunterWorkers=1+pick(30);seed.wellWorkers=pick(31);seed.laborPercent=1+pick(100);
        for(auto& p:seed.laborPriority)p=int32_t(pick(6));
        if(seed.laborPriority[0]!=seed.laborPriority[3])++mixedPriority;
        auto s=startNativeSession(seed,6);
        act(seed,s,NativeSessionAction::RoadPath,0,10,39,10);
        if(pick(2))act(seed,s,NativeSessionAction::HuntingPause);
        const unsigned count=1+pick(20);
        for(unsigned n=0;n<count;++n){const bool adjacent=pick(4)!=0;const auto kind=pick(2)?NativeSessionAction::Hunter:NativeSessionAction::Well;
            act(seed,s,kind,2*n,adjacent?8:20);}
        const unsigned houses=pick(20);
        for(unsigned n=0;n<houses;++n)act(seed,s,NativeSessionAction::House,2*n,30);
        // Directly install a valid resident distribution (authored fixture).
        uint32_t population=0;for(auto& o:s.objects)if(o.kind==NativeObjectKind::House){o.residents=pick(101);population+=o.residents;}
        s.ticks=200;s.revision=s.revision+200;s.taxCycles=s.ticks/nativeTaxPeriod;s.lastFood.demand=population;
        need(validateNativeSession(seed,s).empty(),"fixture valid: "+validateNativeSession(seed,s));
        const auto plan=planNativeLabor(seed,s);need(plan.error.empty(),"native labor plan: "+plan.error);
        // Strict context over exactly the same buildings, ascending slot = native id.
        OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
        c.buildings.emplace();c.buildingInputs.emplace();c.buildingUnavailable.emplace();c.buildingLinks.emplace();
        c.employmentModel=OriginalBuildingEmployment{{33,int32_t(seed.hunterWorkers)},{72,int32_t(seed.wellWorkers)}};
        c.cityEmployment.emplace();c.cityEmployment->shortageNotice=c.cityEmployment->unemploymentNotice=true;
        c.cityEmployment->available=originalBuildingPercent(int32_t(population),int32_t(seed.laborPercent));
        for(unsigned n=0;n<10;++n)c.cityEmployment->rows[n].priority=seed.laborPriority[n];
        c.unemploymentTrend=0;c.shortageNoticeCountdown=0;c.elapsedYears=0;c.scenarioStartYears=0;
        const auto roads=nativeRoads(seed,s);
        for(const auto& o:s.objects){if(o.kind!=NativeObjectKind::Hunter&&o.kind!=NativeObjectKind::Well)continue;
            OriginalBuildingFootprint f;f.type=uint16_t(o.kind);f.slot=size_t(o.id);c.buildings->emplace(o.id,f);
            OriginalBuildingUpdateInputs i;i.alive=1;i.operational=nativeHouseConnected(seed,o,roads)?100:0;
            i.enabled=o.kind==NativeObjectKind::Hunter?uint8_t(s.huntingEnabled):1;c.buildingInputs->emplace(o.id,i);
            c.buildingUnavailable->emplace(o.id,false);c.buildingLinks->emplace(o.id,OriginalFigureBuildingLink{});}
        need(originalBuildingEmploymentStep(c)==OriginalBuildingUpdateBlocker::None,"strict employment step");
        need(plan.employed==c.cityEmployment->employed&&plan.shortage==c.cityEmployment->shortage&&plan.unemployed==c.cityEmployment->unemployed&&
             plan.available==c.cityEmployment->available,"city totals equal strict step");
        for(const auto& j:plan.jobs){need(int(j.workers)==c.buildingInputs->at(j.id).workers,"per-building workers equal strict step");++buildings;}
        shortageCases+=plan.shortage>0;++cases;
    }
    need(shortageCases>50&&mixedPriority>50,"equivalence exercised shortages and mixed priorities");
    std::cout<<"{\"labor_equivalence_cases\":"<<cases<<",\"buildings_compared\":"<<buildings<<",\"shortage_cases\":"<<shortageCases<<",\"mixed_priority_cases\":"<<mixedPriority<<",\"passed\":true}\n";
}
// Version isolation and the v6 growth/production contract.
void rules(){
    auto seed=flatSeed(24);
    for(unsigned v=1;v<=5;++v){auto s=startNativeSession(seed,v);need(planNativeLabor(seed,s).error=="labor_requires_native_v6","older rules have no labor");
        need(encodeNativeSession(seed,s).find("LABOR")==std::string::npos,"older encodings unchanged");}
    // v5 and v6 diverge only by labor: same construction, unfed city.
    for(unsigned v:{5u,6u}){auto s=startNativeSession(seed,v);act(seed,s,NativeSessionAction::RoadPath,0,4,23,4);
        act(seed,s,NativeSessionAction::House,0,2);act(seed,s,NativeSessionAction::Well,4,2);act(seed,s,NativeSessionAction::Hunter,8,2);act(seed,s,NativeSessionAction::Resume);
        for(int n=0;n<2;++n)act(seed,s,NativeSessionAction::Tick);
        if(v==5)need(nativePopulation(s)==1&&s.foodStock==3,"v5 unchanged: unstaffed hunter yields 4 and arrival costs 1");
        else need(nativePopulation(s)==1&&s.foodStock==0&&s.foodProduced==0,"v6: no workers, no meat; arrival free via understaffed hunter");
        for(int n=0;n<40;++n)act(seed,s,NativeSessionAction::Tick);
        if(v==6){const auto labor=planNativeLabor(seed,s);need(labor.error.empty()&&labor.available==int32_t(nativePopulation(s)*42/100),"labor is 42% of residents");
            const auto hunter=std::find_if(labor.jobs.begin(),labor.jobs.end(),[](const auto& j){return j.kind==NativeObjectKind::Hunter;});
            need(hunter!=labor.jobs.end()&&hunter->required==15,"hunter requires model column5");
            const auto q=queryNativeCityFeedback(seed,s);need(q.error.empty()&&q.find(hunter->id)&&q.find(hunter->id)->workers==hunter->workers,"feedback exposes workers");
            need(s.foodProduced>0,"staffed hunter produces under v6");}
        const auto bytes=encodeNativeSession(seed,s);std::string error;const auto back=decodeNativeSession(seed,bytes,error);need(back&&encodeNativeSession(seed,*back)==bytes,"roundtrip exact");}
    // Fully staffed hunter with a shortage blocks growth (authored v6 gate).
    {auto big=flatSeed(24);big.hunterWorkers=1;big.wellWorkers=0;auto s=startNativeSession(big,6);act(big,s,NativeSessionAction::RoadPath,0,4,23,4);
     for(int x:{0,2,4,6,8,10,12,14,16})act(big,s,NativeSessionAction::House,x,2);act(big,s,NativeSessionAction::Well,18,2);act(big,s,NativeSessionAction::Hunter,20,2);act(big,s,NativeSessionAction::Resume);
     bool blocked=false;for(int n=0;n<200&&!blocked;++n){const auto before=nativePopulation(s);const auto plan=planNativeFoodCycle(big,s);const bool understaffed=planNativeLabor(big,s).hunterUnderstaffed();act(big,s,NativeSessionAction::Tick);
        if(s.ticks%2==0&&s.lastFood.fed<s.lastFood.demand){need(plan.laborOverride==understaffed,"override exactly when an enabled hunter is understaffed");
            if(nativePopulation(s)==before)blocked=true;}}
     need(blocked,"staffed hunter shortage stops growth");}
    std::cout<<"{\"v6_contract_checks\":"<<checks<<",\"passed\":true}\n";
}
// Real data: saved labor pool vs original formula on inhabited SAVs, and a
// v6 city started from an empty original source.
void real(int argc,char** argv){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
    const auto root=ResourceRoot::fromAbsolute(env);size_t compared=0,matched=0,inhabited=0,inhabitedMatched=0,sources=0;
    for(int n=2;n<argc;++n){std::vector<uint8_t> raw;need(detail::readFile(root.resolve(argv[n]),4194304,raw),std::string("readable ")+argv[n]);
        const auto save=decodeOriginalSave(raw);const auto payload=decodeOriginalMapContainer(raw);OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
        if(save.status!=OriginalSaveStatus::LoadedSnapshot||!payload.complete||!originalBuildingSavedCityContext(payload.payload,save.sizeOffset,c))continue;
        int32_t residents=0;for(const auto& f:save.buildings.footprints)if(f.type>=1&&f.type<=10){need(bool(f.imageInputs),"house inputs");residents+=std::max<int>(0,f.imageInputs->word20);}
        const auto percent=originalBuildingLaborPercent(c.labor->difficulty,c.labor->wageBand,c.labor->health);need(bool(percent),"labor bands");
        int32_t predicted=originalBuildingPercent(residents,*percent);for(unsigned k=0;k<4;++k)predicted-=int32_t(k==2?2:k==3?4:1)*c.labor->formationLabor[k];predicted=std::max(0,predicted);
        ++compared;matched+=predicted==c.cityEmployment->available;if(residents){++inhabited;inhabitedMatched+=predicted==c.cityEmployment->available;}
        std::string error;const auto source=loadNativeSessionSource(root,argv[n],error);if(!source)continue;++sources;
        const auto& seed=source->seed;need(seed.laborPercent==uint32_t(*percent)&&seed.hunterWorkers>0,"v6 seed from source");
        // Find a flat 12x5 site, build road+2 houses+well+hunter, grow and save.
        bool found=false;unsigned x=0,y=0;
        for(unsigned yy=1;yy+5<seed.side&&!found;++yy)for(unsigned xx=1;xx+12<seed.side&&!found;++xx){bool ok=true;
            for(unsigned dy=0;dy<5&&ok;++dy)for(unsigned dx=0;dx<12;++dx)ok&=seed.buildable[size_t(yy+dy)*seed.side+xx+dx]!=0;if(ok){x=xx;y=yy;found=true;}}
        need(found,"flat site");auto s=startNativeSession(seed,6);need(validateNativeSession(seed,s).empty(),"v6 start valid");
        act(seed,s,NativeSessionAction::RoadPath,x,y+2,x+11,y+2);act(seed,s,NativeSessionAction::House,x,y);act(seed,s,NativeSessionAction::House,x+2,y);
        act(seed,s,NativeSessionAction::Well,x+4,y);act(seed,s,NativeSessionAction::Hunter,x+6,y);act(seed,s,NativeSessionAction::Resume);
        for(int t=0;t<80;++t)act(seed,s,NativeSessionAction::Tick);act(seed,s,NativeSessionAction::Pause);
        const auto labor=planNativeLabor(seed,s);need(labor.error.empty()&&labor.available==int32_t(nativePopulation(s)*seed.laborPercent/100),"real labor share");
        const auto bytes=encodeNativeSession(seed,s);const auto back=decodeNativeSession(seed,bytes,error);need(back&&encodeNativeSession(seed,*back)==bytes,"real v6 roundtrip");
        const auto projected=nativeSessionScene(*source,s);need(bool(projected),"v6 projection");
        std::cout<<"{\"real_v6_source\":"<<std::quoted(argv[n])<<",\"labor_percent\":"<<seed.laborPercent<<",\"hunter_workers\":"<<seed.hunterWorkers<<",\"well_workers\":"<<seed.wellWorkers
            <<",\"population\":"<<nativePopulation(s)<<",\"labor\":"<<labor.available<<",\"employed\":"<<labor.employed<<",\"shortage\":"<<labor.shortage<<",\"meat_produced\":"<<s.foodProduced<<",\"roundtrip\":true}\n";}
    need(compared>0,"at least one SAV compared");
    std::cout<<"{\"saved_labor_compared\":"<<compared<<",\"saved_labor_matched\":"<<matched<<",\"inhabited\":"<<inhabited<<",\"inhabited_matched\":"<<inhabitedMatched<<",\"v6_sources\":"<<sources<<",\"checks\":"<<checks<<",\"passed\":"<<(matched==compared?"true":"false")<<"}\n";
    need(matched==compared,"saved labor pool equals original formula");
}
}
int main(int argc,char** argv){
    try{if(argc>1&&std::string(argv[1])=="--real")real(argc,argv);else{equivalence();rules();}return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
