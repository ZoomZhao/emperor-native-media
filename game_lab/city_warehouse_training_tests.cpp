#include <app/city_warehouse_training.hpp>
#include <city_game_state.hpp>
#include <cstdlib>
#include <iostream>

using namespace game_lab;
namespace {
int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::cerr << "FAIL " << name << '\n'; std::exit(1); }
}
bool hasHint(const CityWarehouseTrainingFeedback& feedback, CityWarehouseTrainingHintCode code) {
    return std::any_of(feedback.hints.begin(),feedback.hints.end(),[&](const auto& h){return h.code == code;});
}
struct Run {
    CitySimulation city{CityRuleset::RoadWarehouses};
    CityScenarioSession session;
    CityScenarioAwardBook awards;
};
struct Pair {
    std::vector<uint8_t> city, checkpoint, awards;
    bool active = false;
    bool operator==(const Pair& other) const {
        return city == other.city && checkpoint == other.checkpoint && awards == other.awards && active == other.active;
    }
};
Pair pairedBytes(const Run& run) {
    std::string error;
    Pair out{run.city.saveSnapshot(),encodeCityScenarioCheckpoint(run.session.checkpoint(),error),{},run.session.active()};
    require(error.empty() && out.city.size() == CitySimulation::kWarehouseSnapshotBytes && out.checkpoint.size() == 72,
            "encode paired authored v4 city and unchanged ECS1 metadata");
    out.awards = encodeCityScenarioAwards(run.awards,error);
    require(error.empty(),"encode paired award receipts");
    return out;
}
// Test-only in-memory tuple. No product file container, checksum binding or file I/O is implemented here.
bool resumePair(Run& live, const Pair& pair) {
    CitySimulation candidateCity;
    CityScenarioCheckpoint checkpoint;
    CityScenarioAwardBook savedAwards;
    auto candidateAwards = live.awards;
    CityScenarioSession candidateSession;
    std::string error;
    if (!pair.active || !candidateCity.loadSnapshot(pair.city) ||
        !decodeCityScenarioCheckpoint(pair.checkpoint,checkpoint,error) ||
        !decodeCityScenarioAwards(pair.awards,savedAwards,error) ||
        !mergeCityScenarioAwards(candidateAwards,savedAwards,error) ||
        !restoreCityWarehouseTraining(candidateSession,checkpoint,candidateCity,candidateAwards).ok) return false;
    live.city = std::move(candidateCity);
    live.session = std::move(candidateSession);
    live.awards = std::move(candidateAwards);
    return true;
}
void pairedRoundtrip(Run& run) {
    const auto before = pairedBytes(run);
    Run resumed; resumed.awards = run.awards;
    require(resumePair(resumed,before),"restore original v4 city, run, starting month, deadline and profile union");
    require(pairedBytes(resumed) == before,"paired in-memory restore exact byte roundtrip");
    const auto duplicate = observeCityWarehouseTraining(resumed.session,resumed.city,resumed.awards);
    require(duplicate.ok && !duplicate.award && !duplicate.becameWon && !duplicate.becameLost &&
            pairedBytes(resumed) == before,"observe restored checkpoint without repeated settlement or badge");
    run = std::move(resumed);
}
CityWarehouseTrainingFeedback feedbackChecked(const CitySimulation& city, const CityGoalConfig& goals) {
    const auto bytes = city.saveSnapshot(); const auto events = city.events();
    const auto result = evaluateCityWarehouseTraining(makeCityWarehouseTrainingSnapshot(city),goals);
    require(result.valid,"real copied logistics snapshot produces valid Chinese training hints");
    require(city.saveSnapshot() == bytes && city.events() == events,"capture and pure evaluation leave city bytes/events unchanged");
    return result;
}
CityScenarioUpdate observeChecked(Run& run) {
    const auto bytes = run.city.saveSnapshot(); const auto events = run.city.events();
    const auto update = observeCityWarehouseTraining(run.session,run.city,run.awards);
    require(update.ok,"observe current v4 city using captured training goals");
    require(run.city.saveSnapshot() == bytes && run.city.events() == events,"scenario observation does not mutate simulation or events");
    return update;
}
CityScenarioUpdate advanceChecked(Run& run) {
    const auto bytes = run.city.saveSnapshot(); const auto events = run.city.events();
    const auto preview = run.city.nextMonthLogistics();
    require(run.city.saveSnapshot() == bytes && run.city.events() == events,"real month preview leaves city and events unchanged");
    run.city.advanceMonth(); const auto& actual = run.city.lastLogisticsMonth();
    require(actual.valid && actual.beginStored == preview.beginStored && actual.produced == preview.produced &&
            actual.delivered == preview.delivered && actual.noRouteDiscard == preview.noRouteDiscard &&
            actual.fullDiscard == preview.fullDiscard && actual.consumed == preview.consumed &&
            actual.unmetDemand == preview.unmetDemand && actual.endStored == preview.endStored,
            "published core preview equals all eight actual next-month ledger fields");
    require(int64_t(actual.beginStored) + actual.produced == int64_t(actual.endStored) + actual.consumed +
            actual.noRouteDiscard + actual.fullDiscard && actual.endStored == run.city.stats().foodSupply,
            "actual monthly warehouse stock and discard conservation");
    return observeChecked(run);
}
void add(CitySimulation& city, uint32_t x, uint32_t y, BuildingType type) {
    std::string message; require(city.build(x,y,type,message),"build deterministic authored dry-map recipe");
}
void recipe(CitySimulation& city, bool roads, bool secondWarehouse) {
    for (uint32_t x : {20u,30u,40u,50u,60u,70u,80u,90u}) add(city,x,20,BuildingType::House);
    for (uint32_t x = 7; x <= 10; ++x) add(city,x,4,BuildingType::Farm);
    if (roads) for (uint32_t x = 5; x <= 10; ++x) add(city,x,3,BuildingType::Road);
    if (secondWarehouse) add(city,10,2,BuildingType::Warehouse);
}
void begin(Run& run, uint64_t id) {
    const auto bytes = run.city.saveSnapshot(); const auto events = run.city.events();
    require(beginCityWarehouseTraining(run.session,run.city,id,run.awards).ok,"explicit training start on v4 with unique run id");
    require(run.city.saveSnapshot() == bytes && run.city.events() == events,"begin grants no resources and does not reset the city");
}
void successfulTraining() {
    Run run; begin(run,4001);
    require(run.city.stats().treasury == 5000 && run.city.stats().foodSupply == 400 && run.city.stats().population == 0 &&
            run.city.warehouseStockAt(5,2) == 400 && run.city.stats().grainStorageCapacity == 600,
            "fair core-defined starting resources, one seed warehouse and no bonus grant");
    auto hints = feedbackChecked(run.city,run.session.checkpoint().spec.goals);
    require(hasHint(hints,CityWarehouseTrainingHintCode::Capacity) && hasHint(hints,CityWarehouseTrainingHintCode::NoFarms) &&
            hasHint(hints,CityWarehouseTrainingHintCode::Population),"empty city explains capacity, farms and population objective");
    pairedRoundtrip(run);
    recipe(run.city,true,false);
    require(run.city.stats().treasury == 4690,"actual recipe cost 310 uses the same starting 5000 treasury");
    require(run.city.logisticsOverview().connectedFarms == 4,"four farms share the seed warehouse road component");
    hints = feedbackChecked(run.city,run.session.checkpoint().spec.goals);
    require(hasHint(hints,CityWarehouseTrainingHintCode::NoProduction),"no population means no initial farm production even with correct roads");
    pairedRoundtrip(run);
    int fullMonth = 0, brokenMonth = 0, winMonth = 0, badges = 0;
    Pair preWin;
    for (int month = 1; month <= 24; ++month) {
        const auto prior = pairedBytes(run);
        const auto update = advanceChecked(run);
        if (update.award) ++badges;
        const auto ledger = run.city.lastLogisticsMonth();
        hints = feedbackChecked(run.city,run.session.checkpoint().spec.goals);
        if (!fullMonth && ledger.fullDiscard > 0) {
            fullMonth = month;
            require(ledger.endStored < run.city.logisticsOverview().capacity &&
                    hasHint(hints,CityWarehouseTrainingHintCode::LastCapacityLoss),
                    "before-consumption full loss is explained even when month-end storage has space");
            const auto before = run.city.saveSnapshot(); const auto events = run.city.events(); std::string message;
            require(!run.city.demolish(5,2,message) && run.city.saveSnapshot() == before && run.city.events() == events,
                    "cannot demolish a stocked seed warehouse or lose food/treasury");
            require(run.city.demolish(6,3,message),"deliberate tutorial road break");
            hints = feedbackChecked(run.city,run.session.checkpoint().spec.goals);
            require(run.city.logisticsOverview().disconnectedFarms == 4 &&
                    hasHint(hints,CityWarehouseTrainingHintCode::Disconnected) &&
                    hasHint(hints,CityWarehouseTrainingHintCode::NextNoRouteLoss),"broken road diagnosed before next month");
        } else if (fullMonth && !brokenMonth) {
            brokenMonth = month;
            require(ledger.produced > 0 && ledger.delivered == 0 && ledger.noRouteDiscard == ledger.produced &&
                    hasHint(hints,CityWarehouseTrainingHintCode::LastNoRouteLoss),"all production discarded during actual broken-road month");
            add(run.city,6,3,BuildingType::Road); add(run.city,10,2,BuildingType::Warehouse);
            hints = feedbackChecked(run.city,run.session.checkpoint().spec.goals);
            require(run.city.logisticsOverview().capacity == 1200 && run.city.logisticsOverview().connectedFarms == 4 &&
                    !hasHint(hints,CityWarehouseTrainingHintCode::Capacity) &&
                    !hasHint(hints,CityWarehouseTrainingHintCode::Disconnected) &&
                    !hasHint(hints,CityWarehouseTrainingHintCode::NextNoRouteLoss),"road restoration and second warehouse recover next copied month");
        } else if (brokenMonth && month == brokenMonth + 1) {
            require(ledger.delivered == ledger.produced && ledger.noRouteDiscard == 0 && ledger.fullDiscard == 0,
                    "reconnection delivers only current production; no backlog reward");
        }
        pairedRoundtrip(run);
        if (update.status == CityScenarioStatus::Won) { winMonth = month; preWin = prior; break; }
    }
    require(fullMonth == 10 && brokenMonth == 11 && winMonth == 19 && badges == 1 &&
            run.city.stats().population == 32 && run.city.stats().foodSupply == 677 && run.city.stats().treasury == 5058,
            "deterministic full/break/expand/reconnect route wins within 24 months without resource edits");
    Run replay; replay.awards = run.awards;
    require(resumePair(replay,preWin),"restore old pre-win city with current persistent award history");
    const auto replayWin = advanceChecked(replay);
    require(replayWin.status == CityScenarioStatus::Won && replayWin.becameWon && !replayWin.award &&
            replay.awards.completedRuns == run.awards.completedRuns,"old save replay cannot issue a second badge");
    // Disconnect the first component and remove the second warehouse's access so a real terminal city regresses.
    std::string message;
    require(run.city.demolish(6,3,message) && run.city.demolish(10,3,message),"disconnect both warehouse approaches after victory");
    for (int i = 0; i < 8; ++i) {
        const auto update = advanceChecked(run);
        require(update.status == CityScenarioStatus::Won && !update.award && !update.becameWon,
                "valid terminal regression advances date but never revokes or reissues settlement");
        pairedRoundtrip(run);
    }
    require(run.city.stats().foodSupply < 650,"actual post-win grain regression exercised");
    std::cout << "PASS success: full loss month=" << fullMonth << ", broken month=" << brokenMonth
              << ", won month=" << winMonth << ", population=32 grain=677 treasury=5058 badge=1\n";
}
void failuresAndPause() {
    for (int route = 0; route < 3; ++route) {
        Run run; begin(run,4101+route);
        if (route) recipe(run.city,route == 1,route == 2);
        const auto initial = pairedBytes(run); run.city.setSpeed(0);
        const auto paused = pairedBytes(run);
        for (int i = 0; i < 100; ++i) { run.city.tick(); observeChecked(run); }
        require(pairedBytes(run) == paused,"paused real ticks/refreshes do not spend deadline or settle");
        require(initial.checkpoint == paused.checkpoint,"pause changes only city speed, not challenge start/deadline");
        bool full = false, noRoute = false;
        for (int month = 1; month <= 24; ++month) {
            const auto update = advanceChecked(run); // explicit +1 month remains legal while speed=0
            full |= run.city.lastLogisticsMonth().fullDiscard > 0;
            noRoute |= run.city.lastLogisticsMonth().noRouteDiscard > 0;
            require(update.status == (month < 24 ? CityScenarioStatus::Running : CityScenarioStatus::Lost) &&
                    !update.award && update.becameLost == (month == 24),"unmet route fails exactly on inclusive deadline without badge");
            if (month % 6 == 0 || month == 24) pairedRoundtrip(run);
        }
        const auto hints = feedbackChecked(run.city,run.session.checkpoint().spec.goals);
        if (route == 0) require(hasHint(hints,CityWarehouseTrainingHintCode::NoFarms),"idle failure explains missing production");
        if (route == 1) require(full && run.city.stats().foodSupply < 600 &&
                              hasHint(hints,CityWarehouseTrainingHintCode::Capacity),"single connected warehouse cannot reach 650 despite actual full losses");
        if (route == 2) require(noRoute && run.city.logisticsOverview().capacity == 1200 &&
                              hasHint(hints,CityWarehouseTrainingHintCode::Disconnected),"two warehouses without roads still fail with actual route losses");
        const int deadlineGrain = run.city.stats().foodSupply;
        const auto update = advanceChecked(run);
        require(update.status == CityScenarioStatus::Lost && !update.becameLost && !update.award,
                "failed checkpoint continues without implicit restart or repeated event");
        std::cout << "PASS failure route=" << route << " at month24, full=" << full << " noRoute=" << noRoute
                  << " grain=" << deadlineGrain << " badge=0\n";
    }
    // Training steps are advisory: an expanded connected city can win without deliberately breaking a road.
    Run direct; begin(direct,4201); recipe(direct.city,true,true);
    bool won = false;
    for (int month = 1; month <= 24; ++month) if (advanceChecked(direct).status == CityScenarioStatus::Won) { won=true; break; }
    require(won && direct.awards.completedRuns.size() == 1,"objective-only direct solution accepted without scripted full/break steps");
    std::cout << "PASS direct solution: optional break/full exercises are not hidden victory requirements\n";
}
void rejectionAndCompatibility() {
    Run live; begin(live,4301); const auto before = pairedBytes(live);
    CitySimulation v3(CityRuleset::StaffedServices);
    require(!beginCityWarehouseTraining(live.session,v3,4302,live.awards).ok && pairedBytes(live) == before,
            "v3 cannot masquerade as warehouse training or reset active state");
    require(!observeCityWarehouseTraining(live.session,v3,live.awards).ok && pairedBytes(live) == before,
            "wrong rule observation rejected without state mutation");
    const auto saved = live.session.checkpoint();
    require(!restoreCityWarehouseTraining(live.session,saved,v3,live.awards).ok && pairedBytes(live) == before,
            "wrong rule restoration rejected without state mutation");
    auto wrongSpec = saved; wrongSpec.spec.id = 1;
    require(!restoreCityWarehouseTraining(live.session,wrongSpec,live.city,live.awards).ok && pairedBytes(live) == before,
            "other scenario cannot be restored through training guard");
    wrongSpec = saved; ++wrongSpec.spec.revision;
    require(!restoreCityWarehouseTraining(live.session,wrongSpec,live.city,live.awards).ok && pairedBytes(live) == before,
            "unsupported training revision rejected without changing session or awards");
    for (int fault = 0; fault < 5; ++fault) {
        auto bytes = before;
        if (fault == 0) bytes.city.pop_back();
        if (fault == 1) bytes.checkpoint.pop_back();
        if (fault == 2) bytes.awards.pop_back();
        if (fault == 3) bytes.city = v3.saveSnapshot();
        if (fault == 4) bytes.active = false;
        require(!resumePair(live,bytes) && pairedBytes(live) == before,"test-only candidate tuple rejects corrupt/missing/wrong-rule inputs atomically");
    }
    auto s = makeCityWarehouseTrainingSnapshot(live.city);
    const auto unknown = [&](const CityWarehouseTrainingSnapshot& snapshot) {
        const auto f = evaluateCityWarehouseTraining(snapshot);
        require(!f.valid && f.hints.size() == 1 && f.hints[0].code == CityWarehouseTrainingHintCode::Unknown,
                "malformed or missing snapshot returns unknown instead of false transport diagnosis");
    };
    auto bad=s; bad.warehouseRules=false; unknown(bad);
    bad=s; bad.city.population=-1; unknown(bad);
    bad=s; bad.city.coreDelivered.reset(); unknown(bad);
    bad=s; bad.clock.month=13; unknown(bad);
    bad=s; bad.network.capacity=std::numeric_limits<int>::max(); unknown(bad);
    bad=s; bad.network.disconnectedFarms=std::numeric_limits<int>::max(); unknown(bad);
    bad=s; bad.lastMonth.produced=1; unknown(bad); // valid=false must have all-zero ledger
    bad=s; bad.nextMonth.produced=std::numeric_limits<int>::max(); unknown(bad);
    bad=s; bad.nextMonth.unmetDemand=std::numeric_limits<int>::max(); unknown(bad);
    bad=s; bad.nextMonth.unmetDemand=1; unknown(bad);
    bad=s; bad.nextMonth.endStored=-1; unknown(bad);
    const auto invalidGoal = evaluateCityWarehouseTraining(s,{-1,650,4000,0});
    require(!invalidGoal.valid && hasHint(invalidGoal,CityWarehouseTrainingHintCode::Unknown),"invalid goals do not produce actionable hints");
    auto config=selfAuthoredWarehouseTraining().spec.goals; config.population=0; config.foodReserve=0;
    require(!evaluateCityWarehouseTraining(s,config).valid,"published goal contract rejects zero population target");
    config.population=1;
    require(evaluateCityWarehouseTraining(s,config).valid,"minimum population and zero food target boundary remains valid");
    // Captured configuration remains authoritative when resumed; catalog edits cannot restart the timer.
    Run configured; auto spec=selfAuthoredWarehouseTraining().spec; spec.goals.foodReserve=700; spec.deadlineMonths=30;
    require(configured.session.begin(spec,4303,{-1046,1},configured.awards).ok,"custom replaceable training config starts");
    pairedRoundtrip(configured);
    require(configured.session.checkpoint().spec.goals.foodReserve == 700 &&
            configured.session.checkpoint().spec.deadlineMonths == 30,"custom saved targets and deadline not overwritten by catalog");
    std::string error;
    const auto complete=encodeCityGameState(live.city,live.session,live.awards,error);
    require(!complete.empty() && error.empty() && complete[4]==2,
            "integrated ECG1 version2 now supports the registered active v4 training");
    for (size_t i=0; i<3; ++i) {
        const auto& old=selfAuthoredCityScenarios()[i];
        const int populations[]={24,40,48}, foods[]={200,200,450}, cash[]={4000,4000,3500}, deadlines[]={8,12,14};
        require(old.spec.id==i+1 && old.spec.revision==1 && old.spec.goals.population==populations[i] &&
                old.spec.goals.foodReserve==foods[i] && old.spec.goals.treasuryReserve==cash[i] &&
                old.spec.goals.monthlyBalance==0 && old.spec.deadlineMonths==uint32_t(deadlines[i]),"original three catalog definitions preserved");
        CityScenarioSession session; CityScenarioAwardBook awards;
        require(session.begin(old.spec,4401+i,{-1046,1},awards).ok,"original v3 challenge begins independently");
        const auto bytes=encodeCityGameState(v3,session,awards,error);
        require(!bytes.empty() && error.empty(),"original ECG1 v1 staffed challenge still encodes");
        CitySimulation loaded; CityScenarioSession resumed; CityScenarioAwardBook receipts;
        require(decodeCityGameState(bytes,loaded,resumed,receipts,error) &&
                encodeCityGameState(loaded,resumed,receipts,error)==bytes &&
                loaded.ruleset()==CityRuleset::StaffedServices && resumed.checkpoint().spec.id==old.spec.id,
                "original three v3 challenge saves restore and re-encode exactly");
        const auto cbytes=loaded.saveSnapshot();
        require(!beginCityWarehouseTraining(resumed,loaded,4501+i,receipts).ok && loaded.saveSnapshot()==cbytes &&
                encodeCityGameState(loaded,resumed,receipts,error)==bytes,"training cannot silently convert/reset original challenge");
    }
    for (const auto rules : {CityRuleset::LegacyServices,CityRuleset::StaffedServices}) {
        CitySimulation old(rules), loaded;
        const auto bytes=old.saveSnapshot();
        require(loaded.loadSnapshot(bytes) && loaded.saveSnapshot()==bytes && loaded.ruleset()==rules,
                "legacy EMP1 v1/v3 byte roundtrip remains available");
    }
    std::cout << "PASS rejection/compatibility: unknown hints, complete-state rejection, original three ECG1 saves and v1/v3 retained; integrated active v4 ECG1 version2 encodes\n";
}
void routeSnapshotConsistencyAndRounding() {
    const auto unknown = [](const CityWarehouseTrainingSnapshot& s) {
        const auto result = evaluateCityWarehouseTraining(s);
        require(!result.valid && result.hints.size() == 1 &&
                result.hints[0].code == CityWarehouseTrainingHintCode::Unknown,
                "contradictory current route/next-ledger input returns only Unknown");
    };
    for (bool routed : {false,true}) {
        CitySimulation city(CityRuleset::RoadWarehouses);
        add(city,20,20,BuildingType::House); city.advanceMonth();
        add(city,7,4,BuildingType::Farm);
        if (routed) for (uint32_t x=5; x<=7; ++x) add(city,x,3,BuildingType::Road);
        const auto original = makeCityWarehouseTrainingSnapshot(city);
        require(evaluateCityWarehouseTraining(original).valid && original.nextMonth.produced == 1,
                "authoritative single-farm rounded one-grain snapshot remains valid");
        auto contradictory = original;
        contradictory.network.connectedFarms = routed ? 0 : 1;
        contradictory.network.disconnectedFarms = routed ? 1 : 0;
        unknown(contradictory); // independent reviewer cases; all other factory fields are untouched
        if (routed) {
            // Isolate the same zero-connected implication for fullDiscard without a delivery.
            auto impossibleFull = contradictory;
            const int rejectedDelivery = impossibleFull.nextMonth.delivered;
            impossibleFull.nextMonth.fullDiscard += rejectedDelivery;
            impossibleFull.nextMonth.endStored -= rejectedDelivery;
            impossibleFull.nextMonth.delivered = 0;
            impossibleFull.city.coreDelivered = 0;
            impossibleFull.city.coreNextMonth->foodSupply = impossibleFull.nextMonth.endStored;
            const auto goals = evaluateCityGoals(impossibleFull.city,selfAuthoredWarehouseTraining().spec.goals);
            require(goals.valid && goals.nextMonth.known &&
                    city_warehouse_training_detail::validLedger(impossibleFull.nextMonth),
                    "isolated forged full loss passes existing scalar/ledger checks but has no reachable farm");
            unknown(impossibleFull);
        }
    }
    for (bool noLabor : {false,true}) for (bool connectedFirst : {false,true}) {
        CitySimulation city(CityRuleset::RoadWarehouses);
        add(city,20,20,BuildingType::House);
        if (!noLabor) city.advanceMonth();
        // The disconnected farm is either later or earlier in the core's row-major allocation.
        add(city,7,4,BuildingType::Farm);
        add(city,40,connectedFirst ? 4 : 1,BuildingType::Farm);
        for (uint32_t x=5; x<=7; ++x) add(city,x,3,BuildingType::Road);
        const auto s = makeCityWarehouseTrainingSnapshot(city);
        const auto guide = evaluateCityWarehouseTraining(s);
        require(guide.valid && s.network.connectedFarms == 1 && s.network.disconnectedFarms == 1 &&
                s.nextMonth.produced == (noLabor ? 0 : 1),"real mixed-route zero/one-grain rounding boundary accepted");
        require(s.nextMonth.delivered == (!noLabor && connectedFirst ? 1 : 0) &&
                s.nextMonth.noRouteDiscard == (!noLabor && !connectedFirst ? 1 : 0),
                "real zero share can mean no route loss despite disconnected farm or no delivery despite connected farm");
        require(hasHint(guide,CityWarehouseTrainingHintCode::NextNoRouteLoss) == (s.nextMonth.noRouteDiscard > 0),
                "rounding-zero loss never produces a fabricated next-month loss hint");
        if (!noLabor && !connectedFirst) {
            city.advanceMonth();
            require(city.lastLogisticsMonth().noRouteDiscard == 1,"real historical disconnected-farm loss before repair");
            for (uint32_t x=8; x<=40; ++x) add(city,x,3,BuildingType::Road);
            add(city,40,2,BuildingType::Road);
            const auto repaired = makeCityWarehouseTrainingSnapshot(city);
            const auto repairedGuide = evaluateCityWarehouseTraining(repaired);
            require(repairedGuide.valid && repaired.network.disconnectedFarms == 0 &&
                    repaired.lastMonth.noRouteDiscard == 1 && repaired.nextMonth.noRouteDiscard == 0 &&
                    hasHint(repairedGuide,CityWarehouseTrainingHintCode::LastNoRouteLoss) &&
                    !hasHint(repairedGuide,CityWarehouseTrainingHintCode::NextNoRouteLoss),
                    "current all-connected layout may retain historical loss after repair without being rejected");
        }
    }
    std::cout << "PASS route consistency: contradictory next loss/admission rejected; real mixed-route zero shares and historical loss after repair accepted\n";
}
} // namespace
int main() {
    successfulTraining();
    failuresAndPause();
    rejectionAndCompatibility();
    routeSnapshotConsistencyAndRounding();
    std::cout << "PASS city_warehouse_training " << checks << " checks\n";
}
