#include "city_game_state.hpp"
#include "app/city_goal_feedback_adapter.hpp"
#include <fstream>
#include <iostream>
using namespace game_lab;
namespace {
int checks = 0;
void require(bool yes, const char* message) {
    ++checks; if (!yes) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
CityScenarioUpdate observe(const CitySimulation& city, CityScenarioSession& session, CityScenarioAwardBook& book) {
    const auto feedback=evaluateCityGoals(makeCityFeedbackSnapshot(city,CityHarvestRule::StaffedServicesV1),session.checkpoint().spec.goals);
    return session.observe({city.stats().year,city.stats().month},feedback,book);
}
void bytesFile(const std::filesystem::path& path,const std::vector<uint8_t>& bytes) {
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    require(bool(out),"self-authored fixture write");
}
std::vector<uint8_t> state(const CitySimulation& city,const CityScenarioSession& session,const CityScenarioAwardBook& book) {
    std::string error; auto bytes=encodeCityGameState(city,session,book,error);
    require(!bytes.empty() && error.empty(),"complete game serialization"); return bytes;
}
void build(CitySimulation& city,uint32_t x,uint32_t y,BuildingType type) {
    std::string message;require(city.build(x,y,type,message),"authored route build");
}
void initialRoute(CitySimulation& city,int index) {
    if (index==0) { for(unsigned i=0;i<6;++i) build(city,10+3*i,10,BuildingType::House); }
    else {
        build(city,1,1,BuildingType::House);build(city,2,2,BuildingType::House);
        build(city,1,2,BuildingType::Road);build(city,2,1,BuildingType::Well);
    }
}
void lateFarms(CitySimulation& city) {
    build(city,3,1,BuildingType::Market);build(city,4,1,BuildingType::Temple);
    for(unsigned i=0;i<4;++i) build(city,10+i,10,BuildingType::Farm);
}
}
int main(int argc,char** argv) {
    require(argc==2,"isolated fixture directory required");
    const std::filesystem::path dir(argv[1]);std::filesystem::create_directories(dir);
    std::string error;
    const int expected[] = {4,7,9};
    for(int index=0;index<3;++index) {
        CitySimulation city(CityRuleset::StaffedServices);city.setSpeed(0);
        CityScenarioSession session;CityScenarioAwardBook book;
        const auto& spec=selfAuthoredCityScenarios()[index].spec;
        require(session.begin(spec,100+index,{city.stats().year,city.stats().month},book).ok,"begin profile3 challenge");
        initialRoute(city,index);int first=-1;
        for(int month=0;month<=18;++month) {
            if(index==2 && month==7) { require(city.stats().population==40,"bootstrap population40 before jobs");lateFarms(city); }
            const auto update=observe(city,session,book);require(update.ok,"new model observation");
            if(update.becameWon) first=month;
            const auto bytes=state(city,session,book);const auto events=city.events();
            const auto file=dir/("route-"+std::to_string(index)+"-"+std::to_string(month)+".emp");
            require(saveCityGameState(city,session,book,file,error),"atomic paired challenge save");
            CitySimulation loaded;CityScenarioSession restored;CityScenarioAwardBook restoredBook;
            require(loadCityGameState(loaded,restored,restoredBook,file,error),"paired file restore");
            require(state(loaded,restored,restoredBook)==bytes,"complete city, deadline and receipts roundtrip");
            require(restored.checkpoint().runId==session.checkpoint().runId &&
                restored.checkpoint().startMonth==session.checkpoint().startMonth,"run and original start preserved");
            require(observe(loaded,restored,restoredBook).ok,"restored observation");
            require(!observe(loaded,restored,restoredBook).award,"restored win never emits duplicate receipt");
            require(city.events()==events,"saving and projection preserve source events");
            auto clone=city;auto cloneSession=session;auto cloneBook=book;
            clone.advanceMonth();loaded.advanceMonth();require(observe(clone,cloneSession,cloneBook).ok,"source future observation");
            require(observe(loaded,restored,restoredBook).ok,"restored future observation");
            require(state(clone,cloneSession,cloneBook)==state(loaded,restored,restoredBook),"same future city and challenge");
            city.advanceMonth();
        }
        require(first==expected[index],"actual service rules win4/7/9 months");
        require(book.completedRuns.size()==1,"one internal settlement receipt");
        const auto rejectTerminalInput=[&](CityScenarioSession settled,CityScenarioAwardBook receipts,const CitySimulation& current) {
            const auto previous=encodeCityScenarioCheckpoint(settled.checkpoint(),error);
            const auto savedBook=encodeCityScenarioAwards(receipts,error);
            const auto base=evaluateCityGoals(makeCityFeedbackSnapshot(current,CityHarvestRule::StaffedServicesV1),settled.checkpoint().spec.goals);
            for(int kind=0;kind<3;++kind) {
                auto invalid=base;
                if(kind==0) invalid.valid=false;
                if(kind==1) invalid.goals[0].current=-1;
                if(kind==2) ++invalid.goals[0].target;
                const auto update=settled.observe({current.stats().year,current.stats().month},invalid,receipts);
                require(!update.ok,"terminal invalid feedback rejected");
                require(encodeCityScenarioCheckpoint(settled.checkpoint(),error)==previous &&
                    encodeCityScenarioAwards(receipts,error)==savedBook,"terminal invalid input preserves date and receipts");
            }
        };
        rejectTerminalInput(session,book,city);
        CitySimulation empty(CityRuleset::StaffedServices);empty.setSpeed(0);CityScenarioSession failure;CityScenarioAwardBook noAwards;
        require(failure.begin(spec,200+index,{empty.stats().year,empty.stats().month},noAwards).ok,"failure run begins");
        for(unsigned n=0;n<=spec.deadlineMonths;++n) {
            require(observe(empty,failure,noAwards).ok,"empty route monthly observation");
            if(n<spec.deadlineMonths) empty.advanceMonth();
        }
        require(failure.checkpoint().status==CityScenarioStatus::Lost,"empty city loses at8/12/14");
        require(noAwards.completedRuns.empty(),"failure does not settle award receipt");
        empty.advanceMonth();rejectTerminalInput(failure,noAwards,empty);
    }
    CitySimulation city(CityRuleset::StaffedServices);city.setSpeed(0);CityScenarioSession session;CityScenarioAwardBook book;
    require(session.begin(selfAuthoredCityScenarios()[0].spec,500,{city.stats().year,city.stats().month},book).ok,"corruption target begins");
    initialRoute(city,0);city.advanceMonth();require(observe(city,session,book).ok,"running sync");
    const auto good=state(city,session,book);
    const auto unchanged=[&]() { return state(city,session,book)==good; };
    auto other=city;build(other,70,70,BuildingType::Road);const auto otherCity=other.saveSnapshot();
    CitySimulation validForeign;
    require(validForeign.loadSnapshot(otherCity) && validForeign.stats().year==city.stats().year &&
        validForeign.stats().month==city.stats().month,"foreign city is independently valid at the same date");
    auto mixed=good;std::copy(otherCity.begin(),otherCity.end(),mixed.begin()+kCityGameHeaderBytes);
    require(!decodeCityGameState(mixed,city,session,book,error) && unchanged(),"same-month different-city cut and paste refused atomically");
    auto changedMeta=good;changedMeta[kCityGameHeaderBytes+kCityStateBytes+24]^=1;
    require(!decodeCityGameState(changedMeta,city,session,book,error) && unchanged(),"valid-looking altered scenario targets refused by whole-pair checksum");
    const auto rehash=[](std::vector<uint8_t>& bytes) {
        std::vector<uint8_t> payload(bytes.begin(),bytes.begin()+20);
        payload.insert(payload.end(),bytes.begin()+kCityGameHeaderBytes,bytes.end());
        const auto digest=city_game_io_detail::checksum(payload);std::copy(digest.begin(),digest.end(),bytes.begin()+20);
    };
    for(size_t offset : {kCityGameHeaderBytes+4,kCityGameHeaderBytes+kCityStateBytes+48,
                        kCityGameHeaderBytes+kCityStateBytes+68}) {
        auto invalid=good;invalid[offset]^=0x80;rehash(invalid);
        require(!decodeCityGameState(invalid,city,session,book,error) && unchanged(),"correct checksum cannot legitimize invalid city, date or terminal status");
    }
    for(size_t n : {size_t(0),size_t(4),size_t(19),good.size()-1}) {
        auto bad=good;bad.resize(n);require(!decodeCityGameState(bad,city,session,book,error) && unchanged(),"truncated paired state rejected atomically");
    }
    for(size_t offset : {size_t(0),size_t(4),size_t(8),size_t(12),size_t(16),kCityGameHeaderBytes+4,kCityGameHeaderBytes+kCityStateBytes+48}) {
        auto bad=good;bad[offset]^=0x80;require(!decodeCityGameState(bad,city,session,book,error) && unchanged(),"damaged header city date or metadata rejected atomically");
    }
    auto tail=good;tail.push_back(0);require(!decodeCityGameState(tail,city,session,book,error) && unchanged(),"trailing byte rejected");
    auto missing=good;missing.resize(kCityGameHeaderBytes+kCityStateBytes);
    require(!decodeCityGameState(missing,city,session,book,error) && unchanged(),"missing challenge data never begins anew");
    const auto badFile=dir/"damaged.emp";bytesFile(badFile,tail);
    require(!loadCityGameState(city,session,book,badFile,error) && unchanged(),"bad file preserves city and deadline");
    const auto linked=dir/"link.emp";std::filesystem::create_symlink(badFile,linked);
    require(!loadCityGameState(city,session,book,linked,error) && unchanged(),"symlink refused");
    require(!saveCityGameState(city,session,book,dir/"missing"/"state.emp",error) && unchanged(),"save failure preserves state");
    auto outOfSync=city;outOfSync.advanceMonth();
    require(encodeCityGameState(outOfSync,session,book,error).empty(),"unsynchronized city and challenge refuse save");
    for(auto rules : {CityRuleset::LegacyServices,CityRuleset::StaffedServices}) {
        CitySimulation free(rules);CityScenarioSession none;CityScenarioAwardBook own;
        const auto emp=free.saveSnapshot();require(state(free,none,own)==emp,"free mode remains EMP1 exact");
        require(decodeCityGameState(emp,city,session,book,error) && !session.active() && city.saveSnapshot()==emp,"old EMP1 loads as free mode");
    }
    const auto counter=dir/"RunCounter.ecr";uint64_t run=777;
    require(allocateCityRunId(counter,0,run,error) && run==1,"first persistent run ID");
    require(allocateCityRunId(counter,0,run,error) && run==2,"allocator survives separate call");
    require(allocateCityRunId(counter,900,run,error) && run==901,"loaded run floor advances allocator");
    require(allocateCityRunId(counter,0,run,error) && run==902,"older save cannot reset allocator");
    const int held=open((counter.string()+".lock").c_str(),O_RDWR|O_CLOEXEC);
    require(held>=0 && flock(held,LOCK_EX|LOCK_NB)==0,"isolated counter lock held");
    require(!allocateCityRunId(counter,0,run,error) && run==902,"busy counter refuses without changing output");
    flock(held,LOCK_UN);close(held);
    require(!allocateCityRunId(counter,std::numeric_limits<uint64_t>::max(),run,error) && run==902,"exhaustion preserves output");
    bytesFile(counter,{'x'});
    require(!allocateCityRunId(counter,0,run,error) && run==902,"corrupt counter fails closed");
    const auto counterLink=dir/"linked-counter.ecr";std::filesystem::create_symlink(counter,counterLink);
    require(!allocateCityRunId(counterLink,0,run,error) && run==902,"symlink counter refused");
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"ruleset\":3,\"win_months\":[4,7,9],\"paired_checkpoints_per_scenario\":19,\"rewards_issued\":false}\n";
}
