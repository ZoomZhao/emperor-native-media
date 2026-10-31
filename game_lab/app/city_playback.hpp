// SPDX-License-Identifier: MIT
#pragma once
#include "map_surface.hpp"
#include "../original_save.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace game_lab {
// The App drives the combined figure/building timeline of an original SAV on
// the host clock. Every non-SAV input below is either read from the user's own
// files or sampled from the host clock; the user never types a value.
enum class AppCityPlaybackState { Ready, Playing, Paused, Refused };
struct AppCityPlaybackPreferences {
    int32_t speed = 80, personalSpeed = 80; // Emperor.inf 0x14 / 0x58
    bool soundEffects = true;               // Emperor.inf 0x0B
    // Existing block origin 10DE0C8 and annual caller byte 10DE164 give
    // file offset 0x9C. Absence is unknown; unsupported values stay intact.
    std::optional<uint8_t> annualDemandResponseMode;
};
// Reads the 0x32C-byte preference block loaded into 10DE0C8. An absent, short
// or unreadable file is reported instead of inventing a value.
bool readAppCityPlaybackPreferences(const ResourceRoot&, AppCityPlaybackPreferences&, std::string& error);
struct AppCityPlaybackStatus {
    AppCityPlaybackState state = AppCityPlaybackState::Ready;
    bool continuous = false;     // no automatic update target; Pause/Step/refusal stops admission
    bool experiment = false;      // headless mechanics tests only: Unresolved inputs supplied
    bool strictPremises = false;  // tests only: withhold normal-mode load hypotheses and refuse
    unsigned target = 0;          // requested admitted updates
    unsigned updates = 0;         // N, admitted updates so far
    unsigned long long refusedUpdate = 0;   // 1-based update that refused
    unsigned intervalMs = 0;      // admission cadence at personal speed
    unsigned long long frames = 0; // admitted frames drawn
    size_t decodeFailures = 0;    // accumulated over every rendered frame
    std::string refusal;          // exact named refusal, empty when none
    std::string error;            // preparation failure, not a timeline refusal
    std::string premise;          // on-screen provenance label
};
// One explicitly premised combined timeline driven at the documented cadence.
// The saved snapshot stays the rendered default until play/step admits a frame.
class AppCityPlayback {
public:
    AppCityPlayback(AppMapSurface&, const OriginalSaveResult&, const std::vector<uint8_t>& decodedPayload,
        const AppCityPlaybackPreferences&, uint32_t crtSeed, uint32_t farmerMs, bool experiment, AppCitySessionState* session=nullptr, bool recordReplay=false,const OriginalWorldBoundSource* boundSource=nullptr, bool strictPremises=false);
    // VA0x855214 ten scheduler intervals indexed by (100-speed)/10; admission
    // requires interval+2 ms elapsed. False when the speed has no interval.
    static bool admissionIntervalMs(int32_t speed, unsigned& out);
    bool prepared() const { return prepared_; }
    const AppCityPlaybackStatus& status() const { return status_; }
    const AppCityNotifications* notifications() const {return prepared_?surface_.cityNotifications():nullptr;}
    std::optional<AppCityTributeFinances> tributeFinances() const {return prepared_?surface_.cityTributeFinances():std::nullopt;}
    AppCityTributeResult respondNotification(AppCityTributeId id,AppCityTributeDecision decision) {
        return prepared_?surface_.respondCityNotification(id,decision):AppCityTributeResult::Stale;
    }
    // Normal App playback continues until paused, stepped or refused.
    // Value restore is distinct from an original-SAV load and resumes paused.
    bool restoreCheckpoint(const AppMapSurface::CityCheckpoint&,uint64_t sourceRevision);
    static std::unique_ptr<AppCityPlayback> fromWorldSnapshot(AppMapSurface&,const OriginalWorldSnapshot&,uint64_t hostNow,AppCitySessionState*,std::string&);
    std::optional<OriginalWorldSnapshot> worldSnapshot(std::string& error)const{
        if(!prepared_){error="world_controller_not_prepared";return std::nullopt;}auto value=surface_.worldSnapshot(error);
        if(value)value->continuation.controller={status_.refusedUpdate,status_.decodeFailures,status_.refusal,status_.error,status_.premise};return value;
    }

    // Rebind the host owner only after preparing the complete App replacement.
    void bindSession(AppCitySessionState* session) noexcept {session_=session;}
    void play();
    // Explicit bounded requests remain available for mechanics checks.
    void play(unsigned additional);
    void pause();
    // Exactly one admitted update, never earlier than the cadence allows;
    // `nowMs` is monotonic milliseconds since App start.
    bool step(uint64_t nowMs);
    // At most one admitted update, only when the host cadence elapsed.
    bool tick(uint64_t nowMs);
    OriginalBuildingSceneView render(const OriginalMapCamera&, int bit = -1);
    std::vector<OriginalSceneTimelineFrame> replayFrames() const { return surface_.cityPlaybackReplayFrames(); }
private:
    explicit AppCityPlayback(AppMapSurface& surface):surface_(surface){}
    uint32_t logicalNow(uint64_t now){if(!virtualClock_)return uint32_t(now);if(reanchor_){hostAnchor_=now;logicalAnchor_=lastAdmission_;reanchor_=false;}return logicalAnchor_+uint32_t(now-hostAnchor_);}
    bool virtualClock_=false,reanchor_=true;uint64_t hostAnchor_=0;uint32_t logicalAnchor_=0;
    bool advance(uint32_t nowMs);
    AppMapSurface& surface_;
    AppCityPlaybackPreferences preferences_;
    AppCitySessionState* session_=nullptr;
    AppCityPlaybackStatus status_;
    bool prepared_ = false;
    uint32_t intervalMs_ = 0, lastAdmission_ = 0;
};
}
