// SPDX-License-Identifier: MIT
#include "city_playback.hpp"
#include "../asset_bytes.hpp"
#include <algorithm>
#include <array>
#include <limits>

namespace game_lab {
namespace {
constexpr size_t preferenceBytes = 0x32C;
uint32_t u32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
// Documented source classes only: fresh-process zeros, file-initial .data
// values, App UI state, Emperor.inf preferences and the host CRT seed.
OriginalCombinedProcessInputs documentedPremises(const AppCityPlaybackPreferences& preferences,
    const AppCitySessionState& session, uint32_t farmerMs) {
    OriginalCombinedProcessInputs p;
    // Initialized once at process creation, then committed across loads.
    p.annualDemandResponseMode = preferences.annualDemandResponseMode;
    p.cityClocks = session.cityClocks;
    p.randomHistory = session.randomHistory;
    p.wildlifeCounter = session.wildlifeCounter;
    p.wildlifePause = 0;
    // File initial .data values.
    p.wildlifePeriod = session.wildlifePeriod;
    p.wildlifeMode = 1;
    p.cityMode = 0;
    // 4FAE50 is the network-active predicate (SAV_FIGURES scheduler
    // persistence). This App has only a local, single-player session.
    constexpr bool networkActive = false;
    p.schedulerGate16 = int32_t(networkActive);
    p.cropDispatchModel = std::array<int32_t, 14>{5, 3, 4, 2, 3, 2, 10, 6, 8, 4, 3, 4, 50, 8};
    p.cropDispatchRadius = std::array<int32_t, 2>{3, 3};
    // App UI state: ordinary play, not editing, viewport culling on.
    p.alternateMode = false;
    p.effectPlacementCulled = false;
    p.buildingEditMode = false;
    // Emperor.inf preference block.
    p.soundEffectsEnabled = preferences.soundEffects;
    // Host clock: Unix seconds sampled once at App start, and milliseconds
    // since App start for the farmer 766DA0 observation.
    p.crtRandom = session.crtRandom;
    p.wallClock = farmerMs;
    return p;
}
// Load/continue hypotheses for ordinary play. Each value is the best-supported
// reading of an input the SAV does not store, marked in place; none is a
// recovered runtime observation. strictPremises withholds every one of them so
// the named refusal stays selectable. They are applied to the live update only:
// the world-checkpoint policy keeps them absent, because a present gate there
// is rejected as policy promotion.
void applyLoadHypotheses(OriginalCombinedProcessInputs& p) {
    // HYPOTHESIS(event-ui-gates-closed-at-load): 5EFF20 reads singleton 1E53538
    // bytes +17 then +16 and returns at 5F00A2 with no stores when a byte is
    // zero (SAV_FIGURES.md). At load/continue no event dialog is open, so both
    // bytes are zero and only that proven early return runs.
    p.eventGate17 = 0;
    p.eventGate16 = 0;
    // HYPOTHESIS(scenario-slots-empty-at-load): the eight dwords at
    // A98370+259920 are process state. The SAV19 read window never names that
    // range, and a fresh process zero-fills it (ORIGINAL_TICK_WIRING_AUDIT.md
    // §1.6; SAV.md Unresolved table). With every slot null, both 466E90 loops
    // make no calls. A nonempty slot is still refused by name.
    p.eventSlots = std::array<uint32_t, 8>{};
    // HYPOTHESIS(manager-gate-closed-at-load): 425250 reads singleton 10AF4E0
    // byte +8, and 492452..460 skips a mode5 record when that byte is zero, so
    // no callback or write happens (SAV_FIGURES.md). The byte is not in the
    // SAV19 window and a fresh process zero-fills it. A nonzero byte keeps the
    // named mode5 refusal.
    p.managerGate8 = 0;
    // HYPOTHESIS(migrant-spacing-zero-at-load): D62418 is the migrant spacing
    // counter. Phase 23 (4AD4A0) subtracts 51 and clamps at zero before any
    // migrant walk, and the SAV19 window does not restore it, so a fresh
    // process starts it at zero (ORIGINAL_TICK_WIRING_AUDIT.md phase 23; SAV.md
    // Unresolved table). Zero only delays the first migrant; it invents none.
    p.migrantSpacing = 0;
    // HYPOTHESIS(notice-history-at-file-initial): 8501FC/FD are the shortage and
    // unemployment notice latches. The SAV19 window does not restore them
    // (ORIGINAL_TICK_WIRING_AUDIT.md phase 1: 25 of 29 written addresses are
    // outside it) and the initialized .data image holds 1 for both (SAV.md
    // input table). 4F1590 reads them before it may emit 548210/548270, so a
    // fresh process starts both set. The experiment's false is a test premise,
    // not this initial value.
    p.shortageNotice = true;
    p.unemploymentNotice = true;
    // HYPOTHESIS(service-coverage-zero-at-load): C82D58 is the 11-column
    // coverage scratch. 4AF2D0 rewrites columns 0..3 and 7..10 and keeps 4..6,
    // and phase 1 refuses before that write when the row is absent. The SAV19
    // window does not restore it and a fresh process zero-fills it (SAV.md
    // Unresolved table; SAV_FIGURES.md coverage scratch). Zeros survive only in
    // the three preserved columns.
    p.serviceCoverage = std::array<int32_t, 11>{};
    // HYPOTHESIS(highest-building-slot-zero-at-load): phase 10 (4AEB30) refuses
    // unless the caller supplies the highest-ever building slot, then sets it to
    // the max of that value and the highest live slot. It is not restored from
    // the SAV (original_scene_timeline.hpp; SAV.md Unresolved table), so a fresh
    // process starts it at zero and the first scan records the live maximum.
    p.highestEverBuildingSlot = 0;
    // HYPOTHESIS(religion-bonus-off-at-load): 10BFB50 is the religion-bonus
    // gate. Phase 18 (5A8170) refuses while it is absent, and its festival
    // branch only runs when the gate is set, so zero disables that branch. A
    // fresh process zero-fills it and the SAV19 window does not restore it
    // (SAV.md Unresolved table; SAV_FIGURES.md religion bonus disabled).
    p.religionBonusEnabled = false;
    // HYPOTHESIS(religion-scratch-at-file-initial): 1312F78 is the previous
    // religion rating and 85D720 the selected worship. Phase 18 reads both and,
    // in ordinary mode, writes neither (5A832A is the only rating writer, and
    // only in alternate mode). 85D720's initialized .data value is -1 and
    // 1312F78 is zero-filled at process creation; neither is in the SAV19
    // window (SAV.md input table and Unresolved table).
    p.religionPreviousRating = 0;
    p.religionSelected = -1;
    // HYPOTHESIS(producer-tails-zero-at-load): phase 33 (4AF230) resets the
    // first 30 producer and staffed-producer entries and preserves entries
    // 30..35, refusing while those rows are absent. They are zero-filled at
    // process creation and not restored by the SAV19 window (SAV.md Unresolved
    // table; SAV_FIGURES.md producer tails), so only the preserved tail is
    // supplied and it starts at zero.
    p.producerTail = std::array<int32_t, 6>{};
    p.staffedProducerTail = std::array<int32_t, 6>{};
    // HYPOTHESIS(effect-scratch-zero-at-load): 101D12C is the effect-decay
    // counter and 8900E8 the visual-effect count. Phase 45 (5F02C0) refuses
    // while the counter is absent, then increments it and decays the effect
    // grid only when it passes 7. The count gates debris emission. Both are
    // zero-filled at process creation and outside the SAV19 window (SAV.md
    // Unresolved table; SAV_FIGURES.md effect premises).
    p.effectDecayCounter = 0;
    p.visualEffectCount = 0;
}
// Explicit, labelled experiment: the documented-but-Unresolved rows are fed
// the same declared zeros the CLI combined corpus uses. These are caller
// premises, not recovered SAV values or original runtime observations.
void applyUnresolvedZeroPremises(OriginalCombinedProcessInputs& p) {
    p.eventGate17 = 0;
    p.eventGate16 = 0;
    p.eventSlots = std::array<uint32_t, 8>{};
    p.eventProgress = 0;
    p.managerGate8 = 0;
    p.shortageNotice = false;
    p.unemploymentNotice = false;
    p.serviceCoverage = std::array<int32_t, 11>{};
    p.religionBonusEnabled = false;
    p.religionPreviousRating = 0;
    p.religionSelected = -1;
    p.highestEverBuildingSlot = 0;
    p.migrantSpacing = 0;
    p.scenarioFigurePlacementCheck = false;
    p.producerTail = std::array<int32_t, 6>{};
    p.staffedProducerTail = std::array<int32_t, 6>{};
    p.forceHouseEvolution = false;
    p.effectDecayCounter = 0;
    p.visualEffectCount = 0;
    p.previousHealthRating = 0;
}
std::string describePremise(const AppCityPlaybackPreferences& preferences, uint32_t crtSeed, uint32_t farmerMs,
    bool experiment, bool strictPremises, unsigned intervalMs) {
    std::string text = "前提（App 自动供给，无需输入）：SAV 存档时序 + Emperor.inf 速度 " +
        std::to_string(preferences.speed) + "/" + std::to_string(preferences.personalSpeed) + "、音效 " +
        (preferences.soundEffects ? "开" : "关") + " + 会话 CRT 状态 " + std::to_string(crtSeed) +
        "、农夫毫秒 " + std::to_string(farmerMs) + " + 单机会话（进程初值仅初始化一次，跨存档保留）；间隔 " + std::to_string(intervalMs) + " ms。";
    if (experiment)
        text += "实验前提已开启：未解析输入按声明零值/文档初值供给（combinedTestProcess），非原版观测。";
    else if (strictPremises)
        text += "严格前提：加载假设不供给，时序一旦需要即显示具名拒绝。";
    else
        text += "加载假设已供给（HYPOTHESIS，非原版观测）：其余未解析输入不供给，时序一旦需要即显示具名拒绝。";
    return text;
}
}

bool readAppCityPlaybackPreferences(const ResourceRoot& root, AppCityPlaybackPreferences& out, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!detail::readFile(root.resolve("Emperor.inf"), preferenceBytes, bytes)) {
        error = "Emperor.inf 不可读"; return false;
    }
    if (bytes.size() != preferenceBytes) { error = "Emperor.inf 不是 0x32C 字节偏好块"; return false; }
    out.speed = int32_t(u32(bytes.data() + 0x14));
    out.personalSpeed = int32_t(u32(bytes.data() + 0x58));
    out.soundEffects = bytes[0x0B] != 0;
    out.annualDemandResponseMode = bytes[0x9C];
    return true;
}

bool AppCityPlayback::admissionIntervalMs(int32_t speed, unsigned& out) {
    // VA0x855214/file0x455214; speedBits=(100-speed), index=bits/10.
    static constexpr unsigned intervals[10] = {0, 20, 35, 55, 80, 110, 160, 240, 350, 500};
    const int64_t index = int64_t(100) - speed;
    if (index < 0 || index >= 100 || index / 10 >= 10) return false;
    out = intervals[size_t(index / 10)] + 2;
    return true;
}

AppCityPlayback::AppCityPlayback(AppMapSurface& surface, const OriginalSaveResult& save,
    const std::vector<uint8_t>& payload, const AppCityPlaybackPreferences& preferences,
    uint32_t crtSeed, uint32_t farmerMs, bool experiment, AppCitySessionState* session, bool recordReplay,const OriginalWorldBoundSource* boundSource, bool strictPremises)
    : surface_(surface), preferences_(preferences), session_(session) {
    AppCitySessionState initial;initial.crtRandom=crtSeed;
    if(session_)initial=*session_;
    lastAdmission_=initial.admissionClock;
    const auto loaded=originalFigureSchedulerAfterLoad({lastAdmission_,preferences.speed,false,false},preferences.personalSpeed,false);
    preferences_.speed=loaded.speed;
    status_.experiment = experiment;
    status_.strictPremises = strictPremises;
    if (!admissionIntervalMs(preferences_.speed, intervalMs_)) {
        status_.state = AppCityPlaybackState::Refused;
        status_.error = "Emperor.inf 个人速度 " + std::to_string(preferences.personalSpeed) + " 不在调度区间表内";
        return;
    }
    status_.intervalMs = intervalMs_;
    status_.premise = describePremise(preferences, initial.crtRandom, farmerMs, experiment, strictPremises, intervalMs_);
    auto premises = documentedPremises(preferences, initial, farmerMs);
    const auto documented=premises;
    if (experiment) applyUnresolvedZeroPremises(premises);
    // Checkpoint policy records documented inputs only. A present event gate,
    // slot table or manager byte in a non-experimental snapshot is refused as
    // policy promotion, so load hypotheses never enter that record.
    OriginalWorldPolicy policy;policy.experimental=experiment;policy.initial=premises;
    policy.preferences={preferences_.speed,preferences_.personalSpeed,preferences_.soundEffects,preferences_.annualDemandResponseMode};
    policy.origins[0]=premises.cityClocks?uint8_t(documented.cityClocks?2:3):0;
    policy.origins[1]=premises.randomHistory?uint8_t(documented.randomHistory?2:3):0;
    policy.origins[2]=premises.eventGate17?uint8_t(documented.eventGate17?2:3):0;
    policy.origins[3]=premises.eventGate16?uint8_t(documented.eventGate16?2:3):0;
    policy.origins[4]=premises.cityMode?uint8_t(documented.cityMode?2:3):0;
    policy.origins[5]=premises.schedulerGate16?uint8_t(documented.schedulerGate16?2:3):0;
    policy.origins[6]=premises.eventSlots?uint8_t(documented.eventSlots?2:3):0;
    policy.origins[7]=premises.eventProgress?uint8_t(documented.eventProgress?2:3):0;
    policy.origins[8]=premises.eventRecordModes?uint8_t(documented.eventRecordModes?2:3):0;
    policy.origins[9]=premises.managerGate8?uint8_t(documented.managerGate8?2:3):0;
    policy.origins[10]=premises.wildlifeCounter?uint8_t(documented.wildlifeCounter?2:3):0;
    policy.origins[11]=premises.wildlifePeriod?uint8_t(documented.wildlifePeriod?2:3):0;
    policy.origins[12]=premises.wildlifePause?uint8_t(documented.wildlifePause?2:3):0;
    policy.origins[13]=premises.wildlifeMode?uint8_t(documented.wildlifeMode?2:3):0;
    policy.origins[14]=premises.serviceCoverage?uint8_t(documented.serviceCoverage?2:3):0;
    policy.origins[15]=premises.shortageNotice?uint8_t(documented.shortageNotice?2:3):0;
    policy.origins[16]=premises.unemploymentNotice?uint8_t(documented.unemploymentNotice?2:3):0;
    policy.origins[17]=premises.religionBonusEnabled?uint8_t(documented.religionBonusEnabled?2:3):0;
    policy.origins[18]=premises.religionPreviousRating?uint8_t(documented.religionPreviousRating?2:3):0;
    policy.origins[19]=premises.religionSelected?uint8_t(documented.religionSelected?2:3):0;
    policy.origins[20]=premises.migrantSpacing?uint8_t(documented.migrantSpacing?2:3):0;
    policy.origins[21]=premises.scenarioFigurePlacementCheck?uint8_t(documented.scenarioFigurePlacementCheck?2:3):0;
    policy.origins[22]=premises.highestEverBuildingSlot?uint8_t(documented.highestEverBuildingSlot?2:3):0;
    policy.origins[23]=premises.producerTail?uint8_t(documented.producerTail?2:3):0;
    policy.origins[24]=premises.staffedProducerTail?uint8_t(documented.staffedProducerTail?2:3):0;
    policy.origins[25]=premises.forceHouseEvolution?uint8_t(documented.forceHouseEvolution?2:3):0;
    policy.origins[26]=premises.alternateMode?uint8_t(documented.alternateMode?2:3):0;
    policy.origins[27]=premises.effectDecayCounter?uint8_t(documented.effectDecayCounter?2:3):0;
    policy.origins[28]=premises.wallClock?uint8_t(documented.wallClock?2:3):0;
    policy.origins[29]=premises.cropDispatchModel?uint8_t(documented.cropDispatchModel?2:3):0;
    policy.origins[30]=premises.cropDispatchRadius?uint8_t(documented.cropDispatchRadius?2:3):0;
    policy.origins[31]=premises.effectPlacementCulled?uint8_t(documented.effectPlacementCulled?2:3):0;
    policy.origins[32]=premises.visualEffectCount?uint8_t(documented.visualEffectCount?2:3):0;
    policy.origins[33]=premises.windDirection?uint8_t(documented.windDirection?2:3):0;
    policy.origins[34]=premises.previousHealthRating?uint8_t(documented.previousHealthRating?2:3):0;
    policy.origins[35]=premises.crtRandom?uint8_t(documented.crtRandom?2:3):0;
    policy.origins[36]=premises.soundEffectsEnabled?uint8_t(documented.soundEffectsEnabled?2:3):0;
    policy.origins[37]=premises.buildingEditMode?uint8_t(documented.buildingEditMode?2:3):0;
    policy.origins[38]=premises.annualDemandResponseMode?uint8_t(documented.annualDemandResponseMode?2:3):0;

    const bool matches=!boundSource||(boundSource->save.snapshotRevision==save.snapshotRevision&&boundSource->binding.payloadDigest==originalWorldDigest(payload.data(),payload.size()));
    if (!experiment && !strictPremises) applyLoadHypotheses(premises);
    // `policy` stays the documented record. The played premises may also carry
    // a load hypothesis, which must not be written back into that record.
    if (!matches||!(boundSource?surface_.prepareBoundCityPlayback(*boundSource,policy,initial,recordReplay,&premises):surface_.prepareCityPlayback(save, payload, premises, &initial, recordReplay))) {
        status_.state = AppCityPlaybackState::Refused;
        status_.error = surface_.cityPlaybackError();
        return;
    }
    prepared_ = true;
}

std::unique_ptr<AppCityPlayback> AppCityPlayback::fromWorldSnapshot(AppMapSurface& surface,const OriginalWorldSnapshot& s,uint64_t hostNow,AppCitySessionState* session,std::string& error){
    auto out=std::unique_ptr<AppCityPlayback>(new AppCityPlayback(surface));
    out->preferences_={s.policy.preferences.speed,s.policy.preferences.personalSpeed,s.policy.preferences.soundEffects,s.policy.preferences.annualDemandResponseMode};
    if(!admissionIntervalMs(out->preferences_.speed,out->intervalMs_)){error="world_restore_speed";return {};}
    out->status_.state=AppCityPlaybackState::Paused;out->status_.experiment=s.policy.experimental;out->status_.intervalMs=out->intervalMs_;out->status_.updates=s.continuation.updates;out->status_.frames=s.continuation.frames;
    out->status_.refusedUpdate=s.continuation.controller.refusedUpdate;out->status_.decodeFailures=s.continuation.controller.decodeFailures;
    out->status_.refusal=s.continuation.controller.refusal;out->status_.error=s.continuation.controller.error;out->status_.premise=s.continuation.controller.premise;
    out->session_=session;out->lastAdmission_=s.continuation.admission;out->virtualClock_=true;out->hostAnchor_=hostNow;out->logicalAnchor_=out->lastAdmission_;out->reanchor_=true;
    if(!surface.restoreWorldSnapshot(s,error))return {};if(session)*session=s.continuation.session;out->prepared_=true;return out;
}
bool AppCityPlayback::restoreCheckpoint(const AppMapSurface::CityCheckpoint& checkpoint,uint64_t sourceRevision){
    if(!prepared_||!surface_.restoreCityCheckpoint(checkpoint,sourceRevision))return false;
    const auto session=surface_.cityPlaybackSession();if(session){lastAdmission_=session->admissionClock;if(session_)*session_=*session;}
    status_.updates=surface_.cityPlaybackUpdates();status_.frames=status_.updates;status_.target=0;status_.continuous=false;status_.state=AppCityPlaybackState::Paused;
    status_.refusal.clear();status_.error.clear();status_.refusedUpdate=0;return true;
}
void AppCityPlayback::play() {
    if (!prepared_ || status_.state == AppCityPlaybackState::Refused) return;
    status_.continuous = true;
    status_.target = 0;
    status_.state = AppCityPlaybackState::Playing;
}

void AppCityPlayback::play(unsigned additional) {
    if (!prepared_ || status_.state == AppCityPlaybackState::Refused) return;
    status_.continuous = false;
    additional = std::max(1u, std::min(additional, 10000u));
    status_.target = status_.updates + std::min(additional, std::numeric_limits<unsigned>::max() - status_.updates);
    if (status_.updates >= status_.target) { status_.state = AppCityPlaybackState::Paused; return; }
    status_.state = AppCityPlaybackState::Playing;
}

void AppCityPlayback::pause() {
    if(virtualClock_)reanchor_=true;
    if (status_.state == AppCityPlaybackState::Playing) status_.state = AppCityPlaybackState::Paused;
}

bool AppCityPlayback::tick(uint64_t nowMs) {
    if (status_.state != AppCityPlaybackState::Playing) return false;
    const uint32_t now = logicalNow(nowMs);
    if (int32_t(now - (lastAdmission_ + intervalMs_)) < 0) return false;
    return advance(now);
}

bool AppCityPlayback::step(uint64_t nowMs) {
    if (!prepared_ || status_.state == AppCityPlaybackState::Refused) return false;
    status_.continuous = false;
    status_.state = AppCityPlaybackState::Paused;
    uint32_t now = logicalNow(nowMs);
    // A step advances the admission clock by one cadence when the host clock
    // has not reached it yet; it never moves the clock backwards.
    const uint32_t due = lastAdmission_ + intervalMs_;
    if (int32_t(now - due) < 0) now = due;
    const bool stepped = advance(now);
    // advance() also serves timer playback and may mark an unfinished target
    // Playing. A manual step always finishes paused; retain a real refusal.
    if (status_.state != AppCityPlaybackState::Refused)
        status_.state = AppCityPlaybackState::Paused;
    status_.target = std::max(status_.target, status_.updates);
    if(virtualClock_)reanchor_=true;
    return stepped;
}

bool AppCityPlayback::advance(uint32_t now) {
    if (status_.updates == std::numeric_limits<unsigned>::max()) {
        status_.state = AppCityPlaybackState::Refused;
        status_.refusal = "city_playback_update_counter_exhausted";
        status_.refusedUpdate = uint64_t(status_.updates) + 1;
        return false;
    }
    OriginalSceneTimelineFrame frame;
    frame.admission.now = now;
    frame.admission.speed = preferences_.speed;
    frame.admission.mode = 1;
    // Fresh-process fast-forward is false: draw after exactly one admitted pass.
    frame.continueAfterPass = {false};
    frame.animationClock = now;
    frame.farmerWallClock = now;
    const auto step = surface_.advanceCityPlayback(frame);
    if (!step.complete) {
        status_.state = AppCityPlaybackState::Refused;
        status_.refusal = step.refusal;
        status_.refusedUpdate = step.refusedUpdate;
        status_.updates = step.updates;
        return false;
    }
    if (step.updates == status_.updates) return false; // cadence not admitted; keep the clock
    if(session_)*session_=*surface_.cityPlaybackSession();
    lastAdmission_ = now;
    status_.updates = step.updates;
    status_.frames = step.updates;
    status_.state = !status_.continuous && status_.updates >= status_.target ? AppCityPlaybackState::Paused : AppCityPlaybackState::Playing;
    return true;
}

OriginalBuildingSceneView AppCityPlayback::render(const OriginalMapCamera& camera, int bit) {
    auto view = surface_.renderSavedBuildings(camera, bit);
    status_.decodeFailures += view.overlayDecodeFailures + view.overlayInvalid + view.bases.decodeFailures;
    return view;
}
}
