#include <app/city_scenario_checkpoint.hpp>
#include <app/city_goal_feedback_adapter.hpp>
#include <cstdlib>
#include <iostream>

using namespace game_lab;
namespace {
int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::cerr << "FAIL " << name << '\n'; std::exit(1); }
}
CityScenarioClock clock(const CitySimulation& city) { return {city.stats().year, city.stats().month}; }
CityFeedbackSnapshot snap(int population = 0) {
    CityFeedbackSnapshot s;
    s.population = population;
    s.houses = 6;
    s.housingCapacity = 24;
    return s;
}
std::vector<uint8_t> checkpointBytes(const CityScenarioSession& session) {
    std::string error;
    auto bytes = encodeCityScenarioCheckpoint(session.checkpoint(), error);
    require(bytes.size() == 72 && error.empty(), "encode valid fixed-size checkpoint");
    return bytes;
}
CityGoalFeedback report(const CityFeedbackSnapshot& s, const CityScenarioSpec& spec) {
    return evaluateCityGoals(s, spec.goals);
}
void fixedRulesAndIdempotence() {
    CityScenarioSession session;
    CityScenarioAwardBook book;
    auto result = session.observe({-1046, 0}, {}, book);
    require(result.ok && !result.active && !result.award, "optional disabled session has no effects");
    auto spec = selfAuthoredCityScenarios()[0].spec;
    spec.deadlineMonths = 2;
    result = session.begin(spec, 1001, {-1046, 12}, book);
    require(result.ok && result.remainingMonths == 2 && result.elapsedMonths == 0, "begin at December without consuming a month");
    auto partial = snap(6);
    partial.speed = 0;
    partial.month = 12;
    for (int i = 0; i < 100; ++i) {
        result = session.observeSnapshot({-1046, 12}, partial, book);
        require(result.ok && result.elapsedMonths == 0 && result.remainingMonths == 2, "paused refresh does not consume deadline");
    }
    partial.month = 1;
    result = session.observeSnapshot({-1045, 1}, partial, book);
    require(result.ok && result.elapsedMonths == 1 && result.remainingMonths == 1 &&
            result.status == CityScenarioStatus::Running, "cross-year simulated month progress");
    auto met = snap(24); met.month = 2;
    result = session.observeSnapshot({-1045, 2}, met, book);
    require(result.ok && result.becameWon && result.award && result.status == CityScenarioStatus::Won &&
            result.elapsedMonths == 2 && book.completedRuns == std::vector<uint64_t>{1001}, "win on inclusive deadline, one badge");
    const auto settled = session.checkpoint().settledMonth;
    result = session.observeSnapshot({-1045, 2}, met, book);
    require(result.ok && !result.becameWon && !result.award && book.completedRuns.size() == 1, "settlement idempotent on repeated observation");
    auto regressed = snap(); regressed.month = 3;
    result = session.observeSnapshot({-1045, 3}, regressed, book);
    require(result.ok && result.status == CityScenarioStatus::Won && !result.award &&
            session.checkpoint().settledMonth == settled && result.elapsedMonths == 2, "settled victory remains terminal after targets regress");
    require(!session.begin(spec, 1001, {-1045, 3}, book).ok, "already awarded run ID cannot be reused as a new challenge");
    session.disable();
    require(!session.active() && book.completedRuns.size() == 1, "explicit free-build mode retains profile awards");

    CityScenarioSession failed;
    require(failed.begin(spec, 1002, {-1046, 1}, book).ok, "start failure case");
    auto empty = snap(); empty.month = 3;
    result = failed.observeSnapshot({-1046, 3}, empty, book);
    require(result.ok && result.becameLost && !result.award && result.status == CityScenarioStatus::Lost &&
            result.remainingMonths == 0, "fail at deadline when goals are unmet");
    met.month = 4;
    result = failed.observeSnapshot({-1046, 4}, met, book);
    require(result.ok && result.status == CityScenarioStatus::Lost && !result.becameLost && !result.award,
            "terminal failure is not retroactively won");
    CityScenarioSession loadedFailure;
    std::string error;
    require(resumeCityScenarioFromBytes(loadedFailure, checkpointBytes(failed), {-1046, 4}, book, error),
            "failed checkpoint restores terminal outcome");
    result = loadedFailure.observeSnapshot({-1046, 4}, met, book);
    require(result.ok && result.status == CityScenarioStatus::Lost && !result.becameLost && !result.award,
            "loading failure does not restart deadline or repeat settlement");
    CityScenarioSession jumped;
    require(jumped.begin(spec, 1003, {-1046, 1}, book).ok, "start skipped-month case");
    result = jumped.observeSnapshot({-1046, 4}, met, book);
    require(result.ok && result.status == CityScenarioStatus::Lost && !result.award,
            "goals observed only after deadline do not invent earlier attainment");
    CityScenarioSession additionalRows;
    require(additionalRows.begin(spec, 1004, {-1046, 1}, book).ok, "start extensible-report case");
    auto extraReport = report(snap(24), spec);
    extraReport.goals.push_back({"额外只读信息", 0, 1, 1, false});
    result = additionalRows.observe({-1046, 1}, extraReport, book);
    require(result.ok && result.status == CityScenarioStatus::Won, "additional report rows do not become undisclosed challenge targets");
    std::cout << "PASS deadline rules: pause, rollover, inclusive last month, terminal and idempotent outcomes\n";
}

void persistenceAndOldSaveRewards() {
    const auto spec = selfAuthoredCityScenarios()[0].spec;
    CityScenarioSession session;
    CityScenarioAwardBook book;
    require(session.begin(spec, 2001, {-1046, 1}, book).ok, "start persistence case");
    auto partial = snap(6); partial.month = 2;
    require(session.observeSnapshot({-1046, 2}, partial, book).ok, "advance before save");
    const auto running = checkpointBytes(session);
    const auto start = session.checkpoint().startMonth;
    CityScenarioSession restored;
    std::string error;
    require(resumeCityScenarioFromBytes(restored, running, {-1046, 2}, book, error), "restore running checkpoint");
    auto result = restored.observeSnapshot({-1046, 2}, partial, book);
    require(result.ok && result.elapsedMonths == 1 && result.remainingMonths == 7 &&
            restored.checkpoint().startMonth == start && restored.checkpoint().runId == 2001,
            "load preserves original start, remaining deadline, and run identity");
    const auto before = checkpointBytes(restored);
    require(!resumeCityScenarioFromBytes(restored, {}, {-1046, 2}, book, error) && checkpointBytes(restored) == before,
            "missing metadata never begins a new challenge or erases active state");
    require(!resumeCityScenarioFromBytes(restored, running, {-1046, 3}, book, error) && checkpointBytes(restored) == before,
            "city/checkpoint calendar mismatch is rejected atomically");
    auto met = snap(24); met.month = 5;
    result = restored.observeSnapshot({-1046, 5}, met, book);
    require(result.ok && result.award && result.elapsedMonths == 4, "win restored run under original deadline");
    const auto won = checkpointBytes(restored);
    CityScenarioAwardBook emptyProfile;
    CityScenarioSession wonRestored;
    require(resumeCityScenarioFromBytes(wonRestored, won, {-1046, 5}, emptyProfile, error), "restore terminal win into empty profile");
    require(emptyProfile.completedRuns == std::vector<uint64_t>{2001}, "repair recorded badge receipt without awarding again");
    result = wonRestored.observeSnapshot({-1046, 5}, met, emptyProfile);
    require(result.ok && !result.award && !result.becameWon && result.status == CityScenarioStatus::Won,
            "saved settled victory has no duplicate outcome/reward event");

    // Loading a pre-victory save must retain the durable live/profile reward book.
    CityScenarioAwardBook olderBook;
    require(mergeCityScenarioAwards(book, olderBook, error) && book.completedRuns.size() == 1, "older save cannot erase live rewards");
    CityScenarioSession replay;
    require(resumeCityScenarioFromBytes(replay, running, {-1046, 2}, book, error), "load pre-victory checkpoint with current profile");
    result = replay.observeSnapshot({-1046, 5}, met, book);
    require(result.ok && result.status == CityScenarioStatus::Won && !result.award && book.completedRuns.size() == 1,
            "replaying old save cannot redeem the same run badge twice");
    const auto encodedBook = encodeCityScenarioAwards(book, error);
    CityScenarioAwardBook decodedBook;
    require(decodeCityScenarioAwards(encodedBook, decodedBook, error) && decodedBook.completedRuns == book.completedRuns,
            "profile award book byte roundtrip");
    std::cout << "PASS restore rules: original deadline retained, no implicit restart, current-profile badge deduplication\n";
}

void activeStateInvalidObservations() {
    auto spec = selfAuthoredCityScenarios()[0].spec;
    spec.deadlineMonths = 2;
    for (const auto state : {CityScenarioStatus::Running, CityScenarioStatus::Won, CityScenarioStatus::Lost}) {
        CityScenarioSession session;
        CityScenarioAwardBook book;
        require(session.begin(spec, 6001 + uint8_t(state), {-1046, 1}, book).ok, "start all-state validation fixture");
        const int month = state == CityScenarioStatus::Lost ? 3 : 1;
        auto initial = snap(state == CityScenarioStatus::Won ? 24 : 0);
        initial.month = month;
        const auto settled = session.observeSnapshot({-1046, month}, initial, book);
        require(settled.ok && settled.status == state, "prepare running/won/lost observation fixture");
        const bool activeBefore = session.active();
        const auto before = checkpointBytes(session);
        const auto observedBefore = session.checkpoint().lastObservedMonth;
        const auto settledBefore = session.checkpoint().settledMonth;
        std::string error;
        const auto bookBefore = encodeCityScenarioAwards(book, error);
        require(error.empty(), "capture full award state before invalid observation");
        const CityScenarioClock next{-1046, month + 1};
        const auto rejection = [&](const CityScenarioUpdate& result) {
            require(!result.ok && !result.award && !result.becameWon && !result.becameLost,
                    "invalid input rejected in every active state without events");
            require(session.active() == activeBefore && checkpointBytes(session) == before &&
                    encodeCityScenarioAwards(book, error) == bookBefore,
                    "invalid input preserves full active state, ECS1 and ECA1 byte-for-byte");
        };
        rejection(session.observe(next, {}, book));
        auto negative = snap(-1); negative.month = next.month;
        rejection(session.observeSnapshot(next, negative, book));
        auto otherConfig = spec.goals; ++otherConfig.population;
        auto different = snap(); different.month = next.month;
        rejection(session.observe(next, evaluateCityGoals(different, otherConfig), book));
        // Positive control: correct-schema goal regression/recovery remains a valid terminal observation.
        auto valid = snap(state == CityScenarioStatus::Lost ? 24 : 0);
        valid.month = next.month;
        const auto accepted = session.observeSnapshot(next, valid, book);
        require(accepted.ok && accepted.status == state && !accepted.award && !accepted.becameWon &&
                !accepted.becameLost && session.checkpoint().lastObservedMonth == observedBefore + 1 &&
                session.checkpoint().settledMonth == settledBefore &&
                encodeCityScenarioAwards(book, error) == bookBefore,
                "valid post-state observation advances paired timestamp without changing settlement or awards");
    }
    std::cout << "PASS active-state validation: running/won/lost reject invalid feedback, negative population and different goals with complete state preservation\n";
}

void invalidInputAndCodec() {
    const auto spec = selfAuthoredCityScenarios()[0].spec;
    CityScenarioSession session;
    CityScenarioAwardBook book;
    require(session.begin(spec, 3001, {-1046, 1}, book).ok, "start rejection case");
    const auto before = checkpointBytes(session);
    auto bad = spec; bad.id = 0;
    require(!session.begin(bad, 3002, {-1046, 1}, book).ok, "zero scenario ID rejected");
    bad = spec; bad.revision = 0;
    require(!session.begin(bad, 3002, {-1046, 1}, book).ok, "zero definition revision rejected");
    bad = spec; bad.deadlineMonths = 0;
    require(!session.begin(bad, 3002, {-1046, 1}, book).ok, "zero deadline rejected");
    bad.deadlineMonths = 1201;
    require(!session.begin(bad, 3002, {-1046, 1}, book).ok, "deadline beyond supported bound rejected");
    bad = spec; bad.goals.foodReserve = -1;
    require(!session.begin(bad, 3002, {-1046, 1}, book).ok, "existing target validator reused");
    require(!session.begin(spec, 0, {-1046, 1}, book).ok, "zero run ID rejected");
    require(!session.begin(spec, 3002, {-1046, 13}, book).ok, "invalid calendar rejected");
    require(!session.begin(spec, 3002, {std::numeric_limits<int>::max(), 12}, book).ok, "unreachable deadline past core calendar range rejected");
    require(checkpointBytes(session) == before && book.completedRuns.empty(), "invalid begin calls preserve session/profile");
    auto s = snap();
    auto feedback = report(s, spec);
    auto wrong = feedback; wrong.goals[0].target++;
    require(!session.observe({-1046, 1}, wrong, book).ok, "HUD report for different goals rejected");
    wrong = feedback; wrong.goals[0].current = std::numeric_limits<int64_t>::min();
    require(!session.observe({-1046, 1}, wrong, book).ok, "extreme malformed goal current rejected without subtraction overflow");
    wrong = feedback; wrong.goals[0].met = true;
    require(!session.observe({-1046, 1}, wrong, book).ok, "forged met flag rejected");
    require(!session.observe({-1046, 0}, feedback, book).ok, "invalid observation month rejected");
    require(!session.observeSnapshot({-1046, 2}, s, book).ok, "snapshot month mismatch rejected");
    require(!session.observe({-1047, 12}, feedback, book).ok, "backward date requires explicit restore");
    require(checkpointBytes(session) == before, "invalid observations do not consume deadline or settle");
    std::string error;
    CityScenarioCheckpoint decoded = session.checkpoint();
    const auto rejectBytes = [&](std::vector<uint8_t> bytes, const char* name) {
        const auto prior = encodeCityScenarioCheckpoint(decoded, error);
        require(!decodeCityScenarioCheckpoint(bytes, decoded, error) &&
                encodeCityScenarioCheckpoint(decoded, error) == prior, name);
    };
    auto bytes = before; bytes.pop_back(); rejectBytes(bytes, "truncated checkpoint rejected without mutation");
    bytes = before; bytes.push_back(0); rejectBytes(bytes, "trailing checkpoint byte rejected");
    bytes = before; bytes[0] = 'X'; rejectBytes(bytes, "bad metadata magic rejected");
    bytes = before; bytes[4] = 2; rejectBytes(bytes, "unknown metadata version rejected");
    bytes = before; bytes[24] = 0; bytes[25] = 0; rejectBytes(bytes, "invalid persisted target rejected");
    bytes = before; bytes[40] = 0; rejectBytes(bytes, "invalid persisted deadline rejected");
    bytes = before; bytes[68] = 9; rejectBytes(bytes, "unknown persisted outcome enum rejected");
    bytes = before; bytes[69] = 2; rejectBytes(bytes, "invalid award boolean rejected");
    bytes = before; bytes[70] = 1; rejectBytes(bytes, "reserved byte rejected");
    auto c = session.checkpoint(); c.awardRecorded = true;
    require(encodeCityScenarioCheckpoint(c, error).empty(), "running checkpoint cannot already claim an award");
    c = session.checkpoint(); c.startMonth = std::numeric_limits<int64_t>::max();
    require(encodeCityScenarioCheckpoint(c, error).empty(), "extreme checkpoint calendar rejected without overflow");
    c = session.checkpoint(); c.status = CityScenarioStatus::Lost;
    require(encodeCityScenarioCheckpoint(c, error).empty(), "failure before deadline rejected");
    CityScenarioAwardBook invalidBook{{1, 1}};
    require(encodeCityScenarioAwards(invalidBook, error).empty(), "duplicate awards rejected");
    invalidBook.completedRuns = {0};
    require(encodeCityScenarioAwards(invalidBook, error).empty(), "zero award ID rejected");
    require(!mergeCityScenarioAwards(book, invalidBook, error) && book.completedRuns.empty(), "invalid profile merge preserves live book");
    const auto goodBook = encodeCityScenarioAwards({{1}}, error);
    CityScenarioAwardBook priorBook{{99}};
    const auto rejectBook = [&](std::vector<uint8_t> b, const char* name) {
        require(!decodeCityScenarioAwards(b, priorBook, error) && priorBook.completedRuns == std::vector<uint64_t>{99}, name);
    };
    bytes = goodBook; bytes.pop_back(); rejectBook(bytes, "truncated award book rejected");
    bytes = goodBook; bytes.push_back(0); rejectBook(bytes, "trailing award byte rejected");
    bytes = goodBook; bytes[4] = 2; rejectBook(bytes, "unknown award format rejected");
    bytes = goodBook; bytes[12] = 0; rejectBook(bytes, "zero encoded award ID rejected");
    bytes = goodBook; bytes[8] = 0xff; bytes[9] = 0xff; rejectBook(bytes, "oversized award count rejected before allocation");
    CityScenarioAwardBook full;
    for (uint64_t i = 1; i <= CityScenarioAwardBook::kMaxEntries; ++i) full.completedRuns.push_back(i);
    const auto fullBefore = encodeCityScenarioAwards(full, error);
    require(!mergeCityScenarioAwards(full, {{999999}}, error) && encodeCityScenarioAwards(full, error) == fullBefore,
            "full award book rejects merge atomically");
    CityScenarioSession fullBookSession;
    require(fullBookSession.begin(spec, 999999, {-1046, 1}, full).ok, "start challenge with full but valid profile");
    const auto beforeFullSettlement = checkpointBytes(fullBookSession);
    require(!fullBookSession.observeSnapshot({-1046, 1}, snap(24), full).ok &&
            checkpointBytes(fullBookSession) == beforeFullSettlement && encodeCityScenarioAwards(full, error) == fullBefore,
            "full-profile settlement fails without partial outcome, award, or deadline mutation");
    CityScenarioSession edge;
    require(edge.begin(spec, std::numeric_limits<uint64_t>::max(), {std::numeric_limits<int>::min(), 1}, book).ok,
            "minimum year and maximum nonzero run ID supported");
    const auto edgeBytes = checkpointBytes(edge);
    require(decodeCityScenarioCheckpoint(edgeBytes, decoded, error) && decoded.runId == std::numeric_limits<uint64_t>::max(),
            "signed calendar and uint64 run ID lossless codec");
    std::cout << "PASS validation: invalid input, old/missing/unknown formats, corrupt metadata and numeric bounds\n";
}

void modelProofForThreeScenarios() {
    const int expectedWinMonths[] = {4, 7, 8};
    for (size_t index = 0; index < selfAuthoredCityScenarios().size(); ++index) {
        const auto& definition = selfAuthoredCityScenarios()[index];
        CitySimulation city;
        CityScenarioSession session;
        CityScenarioAwardBook book;
        require(session.begin(definition.spec, 4001 + index, clock(city), book).ok, "start authored empty-city scenario");
        city.setSpeed(0);
        const auto cityBeforePause = city.saveSnapshot();
        for (int i = 0; i < 100; ++i) {
            city.tick();
            const auto s = makeCityFeedbackSnapshot(city, CityHarvestRule::LaborLimitedIntegerV1);
            const auto result = session.observeSnapshot(clock(city), s, book);
            require(result.ok && result.elapsedMonths == 0, "real paused ticks do not advance challenge");
        }
        require(city.saveSnapshot() == cityBeforePause, "paused city has no hidden month progress");
        std::string message;
        const auto build = [&](uint32_t x, uint32_t y, BuildingType type) {
            require(city.build(x, y, type, message), "self-authored deterministic build recipe");
        };
        if (index == 0) {
            for (uint32_t i = 0; i < 6; ++i) build(i * 5, 0, BuildingType::House);
        } else {
            build(1, 1, BuildingType::House); build(2, 2, BuildingType::House);
            build(1, 2, BuildingType::Road); build(2, 1, BuildingType::Well);
            if (index == 2) {
                build(3, 1, BuildingType::Market); build(4, 1, BuildingType::Temple);
                for (uint32_t i = 0; i < 4; ++i) build(10 + i, 10, BuildingType::Farm);
            }
        }
        int firstWin = -1, awards = 0;
        for (int month = 0; month <= 18; ++month) {
            const auto cityBytesBefore = city.saveSnapshot();
            const auto events = city.events();
            const auto s = makeCityFeedbackSnapshot(city, CityHarvestRule::LaborLimitedIntegerV1);
            // Primary API uses the same evaluated snapshot/report the HUD can render.
            const auto goals = evaluateCityGoals(s, definition.spec.goals);
            const auto result = session.observe(clock(city), goals, book);
            require(result.ok && city.saveSnapshot() == cityBytesBefore && city.events() == events,
                    "scenario consumes published goal report without changing model/events");
            if (result.becameWon) firstWin = month;
            if (result.award) ++awards;
            // Roundtrip city + independent metadata every month, including after settlement.
            const auto cityBytes = city.saveSnapshot();
            const auto scenarioBytes = checkpointBytes(session);
            std::string error;
            const auto awardBytes = encodeCityScenarioAwards(book, error);
            CitySimulation loadedCity;
            CityScenarioSession loadedSession;
            CityScenarioAwardBook savedBook;
            require(loadedCity.loadSnapshot(cityBytes) && decodeCityScenarioAwards(awardBytes, savedBook, error), "independent paired model/metadata decode");
            require(mergeCityScenarioAwards(book, savedBook, error) &&
                    resumeCityScenarioFromBytes(loadedSession, scenarioBytes, clock(loadedCity), book, error),
                    "paired restore retains deadline and global award history");
            require(loadedCity.saveSnapshot() == cityBytes && checkpointBytes(loadedSession) == scenarioBytes,
                    "exact city and scenario checkpoint roundtrip");
            city = std::move(loadedCity);
            session = std::move(loadedSession);
            if (month < 18) city.advanceMonth(); // Explicit single-month action still counts while paused.
        }
        require(firstWin == expectedWinMonths[index] && awards == 1 && book.completedRuns.size() == 1 &&
                session.checkpoint().status == CityScenarioStatus::Won, "three reachable scenarios win on expected months and award once across loads");
        std::cout << "PASS playable " << definition.title << ": won after " << firstWin
                  << " months, 19 paired save/restore checkpoints, 1 badge\n";
        CitySimulation empty;
        CityScenarioSession failure;
        CityScenarioAwardBook failureBook;
        require(failure.begin(definition.spec, 5001 + index, clock(empty), failureBook).ok, "start no-building failure proof");
        for (uint32_t month = 0; month <= definition.spec.deadlineMonths; ++month) {
            const auto result = failure.observeSnapshot(clock(empty), makeCityFeedbackSnapshot(empty, CityHarvestRule::LaborLimitedIntegerV1), failureBook);
            require(result.ok && result.status == (month == definition.spec.deadlineMonths ? CityScenarioStatus::Lost : CityScenarioStatus::Running),
                    "empty city fails exactly on configured deadline");
            if (month < definition.spec.deadlineMonths) empty.advanceMonth();
        }
        require(failureBook.completedRuns.empty(), "failed challenge gives no badge");
    }
}
} // namespace

int main() {
    fixedRulesAndIdempotence();
    persistenceAndOldSaveRewards();
    activeStateInvalidObservations();
    invalidInputAndCodec();
    modelProofForThreeScenarios();
    std::cout << "PASS city_scenarios " << checks << " checks\n";
}
