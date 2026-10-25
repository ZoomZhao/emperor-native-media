// SPDX-License-Identifier: MIT
#pragma once
#include "scene_frame_gate.hpp"

namespace game_lab {
// Callback routing only. No renderer, queue, App owner, city state or resource IO.
// Assign BOTH returned values on the same main-thread turn. Backend adapters must
// marshal callbacks to main and weakly capture their actual App owner.
enum class SceneFrameWork { GPU, Display, CPU, Finished };
enum class SceneFrameSignal {
    GPUReady, GPUFailed, GPUDeclined, GPUCancelled,
    DisplayAdmit, DisplayFailed, DisplayDeclined, DisplayCancelled, DisplayScheduled,
    CPUReady, CPUFailed
};
enum class SceneFrameAction { None, StartDisplay, StartCPU, PublishGPU, PublishCPU };
enum class SceneFrameFlowStatus { Applied, Stale, UnexpectedSignal, Finished, ObservationOnly, OutputMismatch, UnexpectedTicket };
struct SceneFrameFlow {
    SceneFrameTicket ticket;
    SceneFrameWork work=SceneFrameWork::GPU;
};
struct SceneFrameFlowTransition {
    SceneFrameGateState gate;
    SceneFrameFlow flow;
    SceneFrameAction action=SceneFrameAction::None;
    SceneFrameFlowStatus status=SceneFrameFlowStatus::Applied;
    SceneFrameStatus ticketStatus=SceneFrameStatus::Current;
};
inline SceneFrameWork sceneFrameSignalWork(SceneFrameSignal signal) {
    switch(signal) {
        case SceneFrameSignal::GPUReady:case SceneFrameSignal::GPUFailed:
        case SceneFrameSignal::GPUDeclined:case SceneFrameSignal::GPUCancelled:return SceneFrameWork::GPU;
        case SceneFrameSignal::DisplayAdmit:case SceneFrameSignal::DisplayFailed:
        case SceneFrameSignal::DisplayDeclined:case SceneFrameSignal::DisplayCancelled:
        case SceneFrameSignal::DisplayScheduled:return SceneFrameWork::Display;
        case SceneFrameSignal::CPUReady:case SceneFrameSignal::CPUFailed:return SceneFrameWork::CPU;
    }
    return SceneFrameWork::Finished;
}
// GPU/CPU Ready means completed output, not submission acceptance. DisplayAdmit
// is called immediately before present, AFTER the display copy succeeded. A later
// Scheduled notification never commits, consumes, retries, or proves visibility.
// Busy/Unavailable/Unsupported submit returns map to Declined (no callback owed).
// Accepted cancellation terminates current work; it does not silently restart CPU.
inline SceneFrameFlowTransition reduceSceneFrameFlow(SceneFrameGateState gate,SceneFrameFlow flow,
        SceneFrameSignal signal,const SceneFrameTicket& echoed,uint32_t actualWidth=0,uint32_t actualHeight=0) {
    SceneFrameFlowTransition r{gate,flow};
    if(!(echoed==flow.ticket)) {
        r.status=SceneFrameFlowStatus::UnexpectedTicket;r.ticketStatus=checkSceneFrameTicket(gate,echoed);return r;
    }
    if(signal==SceneFrameSignal::DisplayScheduled) {
        r.status=SceneFrameFlowStatus::ObservationOnly;return r;
    }
    if(flow.work==SceneFrameWork::Finished) {r.status=SceneFrameFlowStatus::Finished;return r;}
    if(sceneFrameSignalWork(signal)!=flow.work) {r.status=SceneFrameFlowStatus::UnexpectedSignal;return r;}
    r.ticketStatus=checkSceneFrameTicket(gate,flow.ticket);
    if(r.ticketStatus!=SceneFrameStatus::Current) {
        r.flow.work=SceneFrameWork::Finished;r.status=SceneFrameFlowStatus::Stale;return r;
    }
    const bool ready=signal==SceneFrameSignal::GPUReady||signal==SceneFrameSignal::DisplayAdmit||signal==SceneFrameSignal::CPUReady;
    const bool sizeOK=actualWidth==flow.ticket.width&&actualHeight==flow.ticket.height;
    if(ready&&!sizeOK) {r.status=SceneFrameFlowStatus::OutputMismatch;r.ticketStatus=SceneFrameStatus::TargetMismatch;}
    if(signal==SceneFrameSignal::GPUReady&&sizeOK) {
        r.flow.work=SceneFrameWork::Display;r.action=SceneFrameAction::StartDisplay;return r;
    }
    if((signal==SceneFrameSignal::DisplayAdmit||signal==SceneFrameSignal::CPUReady)&&sizeOK) {
        auto consumed=consumeSceneFrame(gate,flow.ticket,actualWidth,actualHeight);
        r.gate=consumed.state;r.ticketStatus=consumed.status;
        r.flow.work=SceneFrameWork::Finished;
        if(consumed.status==SceneFrameStatus::Current)
            r.action=signal==SceneFrameSignal::DisplayAdmit?SceneFrameAction::PublishGPU:SceneFrameAction::PublishCPU;
        else r.status=SceneFrameFlowStatus::Stale;
        return r;
    }
    const bool cancelled=signal==SceneFrameSignal::GPUCancelled||signal==SceneFrameSignal::DisplayCancelled;
    if(!cancelled&&flow.work!=SceneFrameWork::CPU) {
        // The current ticket has NOT been consumed. CPU completion must validate
        // it again: a reload/resize/resource change may happen while CPU is queued.
        r.flow.work=SceneFrameWork::CPU;r.action=SceneFrameAction::StartCPU;return r;
    }
    // Current CPU failure or cancellation: keep the last city/frame externally,
    // discard this pending intent only. Stale callbacks returned before this path.
    r.gate=changeSceneFrame(gate,SceneFrameChange::Cancel).state;
    r.flow.work=SceneFrameWork::Finished;return r;
}
} // namespace game_lab
