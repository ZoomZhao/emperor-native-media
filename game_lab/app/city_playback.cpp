// SPDX-License-Identifier: MIT
#include "city_playback.hpp"
#include "../asset_bytes.hpp"
#include <algorithm>
#include <array>

namespace game_lab {
namespace {
constexpr size_t preferenceBytes = 0x32C;
uint32_t u32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
// Documented source classes only: fresh-process zeros, file-initial .data
// values, App UI state, Emperor.inf preferences and the host CRT seed.
OriginalCombinedProcessInputs documentedPremises(const AppCityPlaybackPreferences& preferences,
    uint32_t crtSeed, uint32_t farmerMs) {
    OriginalCombinedProcessInputs p;
    // Fresh process: zero-filled at creation, carried across loads (SAV.md).
    p.cityClocks = OriginalCityClocks{};
    p.randomHistory = OriginalFigureRandomHistory{{}, 0, 0};
    p.wildlifeCounter = 0;
    p.wildlifePause = 0;
    // File initial .data values.
    p.wildlifePeriod = 30;
    p.wildlifeMode = 1;
    p.cityMode = 0;
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
    p.crtRandom = crtSeed;
    p.wallClock = farmerMs;
    return p;
}
// Explicit, labelled experiment: the documented-but-Unresolved rows are fed
// the same declared zeros the CLI combined corpus uses. These are caller
// premises, not recovered SAV values or original runtime observations.
void applyUnresolvedZeroPremises(OriginalCombinedProcessInputs& p) {
    p.eventGate17 = 0;
    p.eventGate16 = 0;
    p.schedulerGate16 = 0;
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
    bool experiment, unsigned intervalMs) {
    std::string text = "前提（App 自动供给，无需输入）：SAV 存档时序 + Emperor.inf 速度 " +
        std::to_string(preferences.speed) + "/" + std::to_string(preferences.personalSpeed) + "、音效 " +
        (preferences.soundEffects ? "开" : "关") + " + 宿主时钟 CRT 种子 " + std::to_string(crtSeed) +
        "、农夫毫秒 " + std::to_string(farmerMs) + " + 新进程零值/文件初值；间隔 " + std::to_string(intervalMs) + " ms。";
    if (experiment)
        text += "实验前提已开启：未解析输入按声明零值/文档初值供给（combinedTestProcess），非原版观测。";
    else
        text += "未解析输入不供给：时序一旦需要即显示具名拒绝。";
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
    uint32_t crtSeed, uint32_t farmerMs, bool experiment)
    : surface_(surface), preferences_(preferences) {
    status_.experiment = experiment;
    if (!admissionIntervalMs(preferences.personalSpeed, intervalMs_)) {
        status_.state = AppCityPlaybackState::Refused;
        status_.error = "Emperor.inf 个人速度 " + std::to_string(preferences.personalSpeed) + " 不在调度区间表内";
        return;
    }
    status_.intervalMs = intervalMs_;
    status_.premise = describePremise(preferences, crtSeed, farmerMs, experiment, intervalMs_);
    auto premises = documentedPremises(preferences, crtSeed, farmerMs);
    if (experiment) applyUnresolvedZeroPremises(premises);
    if (!surface_.prepareCityPlayback(save, payload, premises)) {
        status_.state = AppCityPlaybackState::Refused;
        status_.error = surface_.cityPlaybackError();
        return;
    }
    prepared_ = true;
}

void AppCityPlayback::play(unsigned additional) {
    if (!prepared_ || status_.state == AppCityPlaybackState::Refused) return;
    additional = std::max(1u, std::min(additional, 10000u));
    status_.target = std::min(10000u, status_.updates + additional);
    if (status_.updates >= status_.target) { status_.state = AppCityPlaybackState::Paused; return; }
    status_.state = AppCityPlaybackState::Playing;
}

void AppCityPlayback::pause() {
    if (status_.state == AppCityPlaybackState::Playing) status_.state = AppCityPlaybackState::Paused;
}

bool AppCityPlayback::tick(uint64_t nowMs) {
    if (status_.state != AppCityPlaybackState::Playing) return false;
    const uint32_t now = uint32_t(nowMs);
    if (int32_t(now - (lastAdmission_ + intervalMs_)) < 0) return false;
    return advance(now);
}

bool AppCityPlayback::step(uint64_t nowMs) {
    if (!prepared_ || status_.state == AppCityPlaybackState::Refused) return false;
    status_.state = AppCityPlaybackState::Paused;
    uint32_t now = uint32_t(nowMs);
    // A step advances the admission clock by one cadence when the host clock
    // has not reached it yet; it never moves the clock backwards.
    const uint32_t due = lastAdmission_ + intervalMs_;
    if (int32_t(now - due) < 0) now = due;
    const bool stepped = advance(now);
    status_.target = std::max(status_.target, status_.updates);
    return stepped;
}

bool AppCityPlayback::advance(uint32_t now) {
    OriginalSceneTimelineFrame frame;
    frame.admission.now = now;
    frame.admission.speed = preferences_.personalSpeed;
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
    lastAdmission_ = now;
    status_.updates = step.updates;
    status_.frames = step.updates;
    status_.state = status_.updates >= status_.target ? AppCityPlaybackState::Paused : AppCityPlaybackState::Playing;
    return true;
}

OriginalBuildingSceneView AppCityPlayback::render(const OriginalMapCamera& camera, int bit) {
    auto view = surface_.renderSavedBuildings(camera, bit);
    status_.decodeFailures += view.overlayDecodeFailures + view.overlayInvalid + view.bases.decodeFailures;
    return view;
}
}
