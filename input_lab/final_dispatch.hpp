// SPDX-License-Identifier: MIT
#pragma once
#include "scoped_click.hpp"

namespace scoped_input {
struct FinalReceipt {
    CGPoint point{};
    DockClock::time_point previewRequestedAt{};
};
// Synchronous, same-process gate. Never accepts a saved JSON "passed" verdict.
// Snapshot collection after the gate may take time; age is checked at the final
// shared sender boundary, after ordinary identity/focus/occlusion validation.
template<class SnapshotReader,class Gate,class Sender,class ClockReader=DockTimeNow>
inline void dispatchFinalChecked(const Request& r,SnapshotReader snapshot,Gate gate,Sender send,ClockReader now={}) {
    const CGPoint point=target(r);
    const FinalReceipt receipt=gate(point);
    require(CGPointEqualToPoint(receipt.point,point),"final proof point differs from request");
    const auto observed=snapshot();
    dispatchChecked(r,observed,[&](CGPoint current) {
        const FinalReceipt confirmed=gate(current);
        require(CGPointEqualToPoint(confirmed.point,receipt.point) &&
            confirmed.previewRequestedAt==receipt.previewRequestedAt,"final proof was replaced at boundary");
        // The validator may yield or take time. Never reuse its earlier snapshot.
        const auto latest=snapshot();
        dispatchChecked(r,latest,[&](CGPoint finalPoint) {
            const auto checked=now();
            require(checked>=receipt.previewRequestedAt &&
                checked-receipt.previewRequestedAt<=std::chrono::milliseconds(250),
                "final preview expired at sender boundary");
            require(CGPointEqualToPoint(finalPoint,receipt.point),"final request point changed");
            send(finalPoint); // One invocation, no retries, retargeting or queues.
        },now);
    },now);
}
template<class Gate>
inline CGPoint postFinalClick(const Request& r,Gate gate) {
    return postClickUsing(r,[&](auto send) {
        dispatchFinalChecked(r,[&]{return currentSnapshot(r);},gate,send);
    });
}
[[noreturn]] inline void refuseUnsupportedFinalRoad() {
    throw std::runtime_error("live final-road dispatch unavailable: no continuous same-process preview/camera/pointer producer");
}
} // namespace scoped_input
