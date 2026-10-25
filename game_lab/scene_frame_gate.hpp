// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <limits>
#include <optional>

namespace game_lab {
// Process-local request identity, NOT save-game state or a security digest.
struct SceneFrameKey {
    uint64_t sessionEpoch=0, revision=0, sceneDigest=0, cameraSeq=0, cacheGeneration=0;
};
struct SceneFrameTicket {
    SceneFrameKey key;
    uint64_t requestSerial=0;
    uint32_t width=0, height=0;
};
inline bool operator==(const SceneFrameKey& a,const SceneFrameKey& b) {
    return a.sessionEpoch==b.sessionEpoch && a.revision==b.revision &&
        a.sceneDigest==b.sceneDigest && a.cameraSeq==b.cameraSeq &&
        a.cacheGeneration==b.cacheGeneration;
}
inline bool operator==(const SceneFrameTicket& a,const SceneFrameTicket& b) {
    return a.key==b.key && a.requestSerial==b.requestSerial &&
        a.width==b.width && a.height==b.height;
}
enum class SceneFramePhase { Open, Closed, Exhausted };
enum class SceneFrameChange { BeginSession, Camera, Resources, Cancel, Close };
enum class SceneFrameStatus {
    Current, Closed, Exhausted, NoSession, InvalidTarget, NoPending,
    OldSession, OldRevision, OldScene, OldCamera, OldResources, OldRequest, TargetMismatch
};
// Owned by ONE App lifecycle on the main thread. Pure reducers below return new
// values: assign those results; never restore an earlier state on load failure,
// deserialize it, or reset it while old callbacks can still reach its owner.
// A new owner uses weak lifetime-bound callbacks, not this state as a weak token.
struct SceneFrameGateState {
    SceneFramePhase phase=SceneFramePhase::Open;
    uint64_t sessionEpoch=0, cameraSeq=0, cacheGeneration=0, requestSerial=0;
    std::optional<SceneFrameTicket> pending;
};
struct SceneFrameTransition {
    SceneFrameGateState state;
    SceneFrameStatus status=SceneFrameStatus::Current;
};
inline SceneFrameStatus sceneFramePhaseStatus(const SceneFrameGateState& s) {
    return s.phase==SceneFramePhase::Closed ? SceneFrameStatus::Closed :
        s.phase==SceneFramePhase::Exhausted ? SceneFrameStatus::Exhausted : SceneFrameStatus::Current;
}
inline bool advanceSceneFrameCounter(SceneFrameGateState& s,uint64_t& counter) {
    if(counter==std::numeric_limits<uint64_t>::max()) {
        s.phase=SceneFramePhase::Exhausted; s.pending.reset(); return false;
    }
    ++counter; return true;
}
// Invalidation does not delete the last visible frame or modify city state.
// BeginSession also applies to a reload of the SAME source/revision. A failed
// replacement may retain its old city, but must retain the NEW lifecycle epoch.
inline SceneFrameTransition changeSceneFrame(SceneFrameGateState s,SceneFrameChange change) {
    if(s.phase!=SceneFramePhase::Open) return {s,sceneFramePhaseStatus(s)};
    s.pending.reset();
    switch(change) {
        case SceneFrameChange::BeginSession: advanceSceneFrameCounter(s,s.sessionEpoch); break;
        case SceneFrameChange::Camera: advanceSceneFrameCounter(s,s.cameraSeq); break;
        case SceneFrameChange::Resources: advanceSceneFrameCounter(s,s.cacheGeneration); break;
        case SceneFrameChange::Cancel: break;
        case SceneFrameChange::Close: s.phase=SceneFramePhase::Closed; break;
    }
    return {s,sceneFramePhaseStatus(s)};
}
// A rejected NEW request still cancels the previous pending intent. Revision 0
// and digest 0 are valid; they are compared, not interpreted as unknown flags.
inline SceneFrameTransition requestSceneFrame(SceneFrameGateState s,uint64_t revision,
                                               uint64_t digest,uint32_t width,uint32_t height) {
    if(s.phase!=SceneFramePhase::Open) return {s,sceneFramePhaseStatus(s)};
    s.pending.reset();
    if(!s.sessionEpoch) return {s,SceneFrameStatus::NoSession};
    if(!width || !height || width>2048 || height>2048) return {s,SceneFrameStatus::InvalidTarget};
    if(!advanceSceneFrameCounter(s,s.requestSerial)) return {s,SceneFrameStatus::Exhausted};
    s.pending=SceneFrameTicket{{s.sessionEpoch,revision,digest,s.cameraSeq,s.cacheGeneration},
                              s.requestSerial,width,height};
    return {s,SceneFrameStatus::Current};
}
inline SceneFrameStatus checkSceneFrameTicket(const SceneFrameGateState& s,const SceneFrameTicket& t) {
    if(s.phase!=SceneFramePhase::Open) return sceneFramePhaseStatus(s);
    if(!s.pending) return SceneFrameStatus::NoPending;
    const auto& p=*s.pending;
    if(t.key.sessionEpoch!=s.sessionEpoch || t.key.sessionEpoch!=p.key.sessionEpoch) return SceneFrameStatus::OldSession;
    if(t.key.revision!=p.key.revision) return SceneFrameStatus::OldRevision;
    if(t.key.sceneDigest!=p.key.sceneDigest) return SceneFrameStatus::OldScene;
    if(t.key.cameraSeq!=s.cameraSeq || t.key.cameraSeq!=p.key.cameraSeq) return SceneFrameStatus::OldCamera;
    if(t.key.cacheGeneration!=s.cacheGeneration || t.key.cacheGeneration!=p.key.cacheGeneration) return SceneFrameStatus::OldResources;
    if(t.requestSerial!=p.requestSerial) return SceneFrameStatus::OldRequest;
    if(t.width!=p.width || t.height!=p.height) return SceneFrameStatus::TargetMismatch;
    return SceneFrameStatus::Current;
}
// Only a current failed GPU request may trigger CPU fallback. Failure does NOT
// consume the ticket: that CPU result must pass consumeSceneFrame itself later.
inline bool mayFallbackSceneFrame(const SceneFrameGateState& s,const SceneFrameTicket& t) {
    return checkSceneFrameTicket(s,t)==SceneFrameStatus::Current;
}
// Call at the last MAIN-THREAD publication boundary, after rendering succeeds.
// This checks actual output dimensions as well as the echoed ticket. A refused
// completion leaves the current pending request intact; a success consumes once.
inline SceneFrameTransition consumeSceneFrame(SceneFrameGateState s,const SceneFrameTicket& t,
                                               uint32_t actualWidth,uint32_t actualHeight) {
    auto status=checkSceneFrameTicket(s,t);
    if(status==SceneFrameStatus::Current && (actualWidth!=t.width || actualHeight!=t.height))
        status=SceneFrameStatus::TargetMismatch;
    if(status==SceneFrameStatus::Current) s.pending.reset();
    return {s,status};
}
inline const char* sceneFrameStatusName(SceneFrameStatus s) {
    switch(s) {
        case SceneFrameStatus::Current:return "current";
        case SceneFrameStatus::Closed:return "closed";
        case SceneFrameStatus::Exhausted:return "counter_exhausted";
        case SceneFrameStatus::NoSession:return "no_session";
        case SceneFrameStatus::InvalidTarget:return "invalid_target";
        case SceneFrameStatus::NoPending:return "no_pending_request";
        case SceneFrameStatus::OldSession:return "stale_session";
        case SceneFrameStatus::OldRevision:return "stale_revision";
        case SceneFrameStatus::OldScene:return "stale_scene";
        case SceneFrameStatus::OldCamera:return "stale_camera";
        case SceneFrameStatus::OldResources:return "stale_resources";
        case SceneFrameStatus::OldRequest:return "stale_request";
        case SceneFrameStatus::TargetMismatch:return "target_mismatch";
    }
    return "invalid_frame_status";
}
} // namespace game_lab
