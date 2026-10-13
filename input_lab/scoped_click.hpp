// SPDX-License-Identifier: MIT
#pragma once
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <libproc.h>
#include <sys/proc_info.h>
#include <unistd.h>
#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <optional>

namespace scoped_input {
struct Identity {
    pid_t pid = 0;
    uid_t uid = 0;
    uint64_t birthSeconds = 0, birthMicros = 0;
    std::string executable;
};
inline bool sameIdentity(const Identity& a, const Identity& b) {
    return a.pid == b.pid && a.uid == b.uid && a.birthSeconds == b.birthSeconds &&
        a.birthMicros == b.birthMicros && a.executable == b.executable;
}
struct Request {
    Identity owner;
    CGWindowID window = 0;
    CGRect capturedFrame{}; // Quartz global points, including title bar.
    double normalizedX = 0, normalizedY = 0;
    double deadlineUnixSeconds = 0;
};
struct Snapshot {
    Identity owner;
    uid_t callerUid = 0;
    bool inputPermission = false, active = false, leftButtonDown = false;
    pid_t frontmostPid = 0, windowPid = 0, hitPid = 0;
    CGWindowID window = 0, hitWindow = 0;
    CGRect frame{};
    double nowUnixSeconds = 0;
    int hitLayer = 0;
    CGRect hitFrame{};
    // Layer metadata determines candidate ordering; shield role is diagnostic only.
    std::optional<int> selectedLayer = std::nullopt;
    std::optional<double> selectedAlpha = std::nullopt, candidateAlpha = std::nullopt;
    CGRect candidateBounds{}, displayBounds{};
    CGDirectDisplayID display = 0;
    CGWindowID publicDisplayShield = 0;
    pid_t candidateOwnerPid = 0;
};
inline const char* candidateRole(const Snapshot& s) {
    return s.display != 0 && s.publicDisplayShield != 0 && s.hitWindow == s.publicDisplayShield
        ? "public_display_capture_shield" : "unclassified_window";
}
inline NSArray* frameJSON(CGRect r) {
    return @[@(r.origin.x),@(r.origin.y),@(r.size.width),@(r.size.height)];
}
inline NSDictionary* inspectionJSON(const Request& r, const Snapshot& s) {
    return @{@"read_only":@YES, @"input_sent":@NO, @"pid":@(s.owner.pid),
        @"expected_window_id":@(r.window), @"selected_window_id":@(s.window),
        @"selected_owner_pid":@(s.windowPid), @"selected_frame":frameJSON(s.frame),
        @"selected_layer":s.selectedLayer?(id)@(*s.selectedLayer):NSNull.null,
        @"selected_alpha":s.selectedAlpha?(id)@(*s.selectedAlpha):NSNull.null,
        @"AX_owner_pid":@(s.hitPid), @"AX_window_frame":frameJSON(s.hitFrame),
        @"candidate_window_id":@(s.hitWindow), @"candidate_owner_pid":@(s.candidateOwnerPid), @"candidate_layer":@(s.hitLayer),
        @"candidate_bounds":frameJSON(s.candidateBounds),
        @"candidate_alpha":s.candidateAlpha?(id)@(*s.candidateAlpha):NSNull.null,
        @"candidate_role":[NSString stringWithUTF8String:candidateRole(s)],
        @"display_id":@(s.display), @"display_bounds":frameJSON(s.displayBounds),
        @"public_display_shield_window_id":@(s.publicDisplayShield),
        @"active":@(s.active), @"frontmost_pid":@(s.frontmostPid)};
}
inline void require(bool value, const char* why) {
    if (!value) throw std::runtime_error(why);
}
inline bool validFrame(CGRect r) {
    return std::isfinite(r.origin.x) && std::isfinite(r.origin.y) &&
        std::isfinite(r.size.width) && std::isfinite(r.size.height) &&
        r.size.width > 0 && r.size.height > 0 &&
        r.size.width <= 4096 && r.size.height <= 4096;
}
inline bool occludesPoint(CGRect bounds, double alpha, int /*layer*/, CGPoint point) {
    return alpha > 0 && CGRectContainsPoint(bounds,point);
}
inline bool systemCursorWitness(const Identity& owner, uid_t caller, int layer, CGRect bounds, CGPoint cursor) {
    const bool systemPath = owner.executable == "/System/Library/PrivateFrameworks/SkyLight.framework/Resources/WindowServer" ||
        owner.executable == "/System/Library/PrivateFrameworks/SkyLight.framework/Versions/A/Resources/WindowServer";
    return systemPath && owner.uid != caller && layer == CGWindowLevelForKey(kCGCursorWindowLevelKey) &&
        validFrame(bounds) && bounds.size.width <= 128 && bounds.size.height <= 128 && CGRectContainsPoint(bounds,cursor);
}
inline bool candidateAtPoint(CGRect bounds,double alpha,int layer,int selectedLayer,CGPoint point,bool verifiedCursor) {
    return !verifiedCursor && layer >= selectedLayer && occludesPoint(bounds,alpha,layer,point);
}
inline CGPoint target(const Request& r) {
    require(r.owner.pid > 0 && r.owner.birthSeconds > 0 && r.owner.birthMicros < 1000000 &&
        !r.owner.executable.empty() && r.owner.executable.front() == '/', "invalid owner identity");
    require(r.window != 0, "explicit window required");
    require(validFrame(r.capturedFrame), "invalid captured frame");
    require(std::isfinite(r.normalizedX) && std::isfinite(r.normalizedY) &&
        r.normalizedX >= 0 && r.normalizedX < 1 && r.normalizedY >= 0 && r.normalizedY < 1,
        "invalid normalized point");
    require(std::isfinite(r.deadlineUnixSeconds), "invalid deadline");
    CGPoint p = CGPointMake(r.capturedFrame.origin.x + r.normalizedX * r.capturedFrame.size.width,
        r.capturedFrame.origin.y + r.normalizedY * r.capturedFrame.size.height);
    require(std::isfinite(p.x) && std::isfinite(p.y), "invalid target");
    return p;
}
inline CGPoint validate(const Request& r, const Snapshot& s) {
    CGPoint p = target(r);
    require(s.inputPermission, "existing input permission unavailable");
    require(s.callerUid == r.owner.uid && sameIdentity(r.owner, s.owner), "owner identity changed");
    require(s.window == r.window && s.windowPid == r.owner.pid, "selected window absent or foreign");
    require(validFrame(s.frame) && CGRectEqualToRect(r.capturedFrame, s.frame), "captured window moved or resized");
    require(s.active && s.frontmostPid == r.owner.pid, "owned app not frontmost");
    require(s.hitPid == r.owner.pid && s.candidateOwnerPid == r.owner.pid && s.hitWindow == r.window &&
        CGRectEqualToRect(s.hitFrame,r.capturedFrame), "selected point obscured");
    require(!s.leftButtonDown, "left button already held");
    const double remaining = r.deadlineUnixSeconds - s.nowUnixSeconds;
    require(std::isfinite(s.nowUnixSeconds) && remaining >= 0.5 && remaining <= 1200,
        "expired or excessive deadline");
    return p;
}
// Both the real sender and no-UI regression use this exact rejection boundary.
template<class Sender> inline void dispatchChecked(const Request& r, const Snapshot& s, Sender send) {
    send(validate(r, s));
}
inline Identity processIdentity(pid_t pid) {
    proc_bsdinfo info{};
    char path[PROC_PIDPATHINFO_MAXSIZE]{};
    require(proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) == sizeof(info) &&
        proc_pidpath(pid, path, sizeof(path)) > 0, "owned process unavailable");
    return {pid, info.pbi_uid, info.pbi_start_tvsec, info.pbi_start_tvusec, path};
}
inline Snapshot currentSnapshot(const Request& r) {
    const CGPoint p = target(r);
    Snapshot s;
    s.inputPermission = CGPreflightPostEventAccess() && AXIsProcessTrusted(); // No requests.
    require(s.inputPermission, "existing input permission unavailable");
    s.owner = processIdentity(r.owner.pid);
    s.callerUid = getuid();
    auto app = [NSRunningApplication runningApplicationWithProcessIdentifier:r.owner.pid];
    s.active = app && app.active;
    s.frontmostPid = NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier;
    s.leftButtonDown = CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft);
    // AX hit-testing supplies a separate owner/frame witness. Rectangle metadata
    // may include transparent backdrops; unresolved front candidates still block.
    auto system=AXUIElementCreateSystemWide(); AXUIElementRef hit=nullptr;
    AXError hitError=AXUIElementCopyElementAtPosition(system,p.x,p.y,&hit); CFRelease(system);
    if (hitError==kAXErrorSuccess && hit) {
        AXUIElementGetPid(hit,&s.hitPid);
        CFTypeRef value=nullptr; AXUIElementRef axWindow=nullptr;
        if (AXUIElementCopyAttributeValue(hit,kAXWindowAttribute,&value)==kAXErrorSuccess && value &&
            CFGetTypeID(value)==AXUIElementGetTypeID()) axWindow=(AXUIElementRef)value;
        else {
            if (value) CFRelease(value); value=nullptr;
            if (AXUIElementCopyAttributeValue(hit,kAXRoleAttribute,&value)==kAXErrorSuccess && value &&
                CFEqual(value,kAXWindowRole)) {axWindow=hit; CFRetain(axWindow);}
            if (value) CFRelease(value);
        }
        if (axWindow) {
            CFTypeRef pos=nullptr, size=nullptr; CGPoint origin{}; CGSize extent{};
            AXUIElementCopyAttributeValue(axWindow,kAXPositionAttribute,&pos);
            AXUIElementCopyAttributeValue(axWindow,kAXSizeAttribute,&size);
            if (pos && size && CFGetTypeID(pos)==AXValueGetTypeID() && CFGetTypeID(size)==AXValueGetTypeID() &&
                AXValueGetValue((AXValueRef)pos,(AXValueType)kAXValueCGPointType,&origin) &&
                AXValueGetValue((AXValueRef)size,(AXValueType)kAXValueCGSizeType,&extent))
                s.hitFrame=CGRectMake(origin.x,origin.y,extent.width,extent.height);
            if (pos) CFRelease(pos); if (size) CFRelease(size); CFRelease(axWindow);
        }
        CFRelease(hit);
    }
    NSArray* windows = CFBridgingRelease(CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID));
    require(windows != nil, "window metadata unavailable");
    // Resolve the selected layer first; then examine every front-layer owner.
    for (NSDictionary* row in windows) {
        if ([row[(__bridge NSString*)kCGWindowNumber] unsignedIntValue] != r.window) continue;
        require(s.window == 0,"ambiguous selected window");
        require(CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)row[(__bridge NSString*)kCGWindowBounds],&s.frame),"selected bounds unavailable");
        s.window=r.window; s.windowPid=[row[(__bridge NSString*)kCGWindowOwnerPID] intValue];
        NSNumber* level=row[(__bridge NSString*)kCGWindowLayer];
        NSNumber* alpha=row[(__bridge NSString*)kCGWindowAlpha];
        if ([level isKindOfClass:NSNumber.class]) s.selectedLayer=[level intValue];
        if ([alpha isKindOfClass:NSNumber.class] && std::isfinite([alpha doubleValue])) s.selectedAlpha=[alpha doubleValue];
    }
    require(s.selectedLayer.has_value(),"selected layer unavailable");
    CGEventRef cursorEvent=CGEventCreate(nullptr);
    CGPoint cursorPoint=cursorEvent?CGEventGetLocation(cursorEvent):CGPointMake(NAN,NAN);
    if (cursorEvent) CFRelease(cursorEvent);
    for (NSDictionary* row in windows) {
        CGRect bounds{};
        require(CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)row[(__bridge NSString*)kCGWindowBounds],&bounds),"window bounds unavailable");
        pid_t ownerPid=[row[(__bridge NSString*)kCGWindowOwnerPID] intValue];
        NSNumber* level=row[(__bridge NSString*)kCGWindowLayer];
        require([level isKindOfClass:NSNumber.class],"window layer unavailable");
        int layer=[level intValue];
        NSNumber* alpha=row[(__bridge NSString*)kCGWindowAlpha];
        require([alpha isKindOfClass:NSNumber.class] && std::isfinite([alpha doubleValue]),"window alpha unavailable");
        bool cursor=false;
        if (ownerPid!=r.owner.pid && layer==CGWindowLevelForKey(kCGCursorWindowLevelKey)) {
            try {cursor=systemCursorWitness(processIdentity(ownerPid),getuid(),layer,bounds,cursorPoint);}
            catch (const std::exception&) {cursor=false;} // Unknown identity never grants an exception.
        }
        if (!candidateAtPoint(bounds,[alpha doubleValue],layer,*s.selectedLayer,p,cursor)) continue;
        s.hitWindow=[row[(__bridge NSString*)kCGWindowNumber] unsignedIntValue];
        s.candidateOwnerPid=ownerPid; s.hitLayer=layer; s.candidateBounds=bounds; s.candidateAlpha=[alpha doubleValue];
        break;
    }
    uint32_t displayCount = 0;
    CGDirectDisplayID display = 0;
    if (CGGetDisplaysWithPoint(p,1,&display,&displayCount) == kCGErrorSuccess && displayCount == 1) {
        s.display = display;
        s.displayBounds = CGDisplayBounds(display);
        s.publicDisplayShield = CGShieldingWindowID(display); // Does not capture/release a display.
    }
    s.nowUnixSeconds = NSDate.date.timeIntervalSince1970;
    return s;
}
struct EventDeleter { void operator()(CGEventRef event) const { if (event) CFRelease(event); } };
using Event = std::unique_ptr<std::remove_pointer_t<CGEventRef>, EventDeleter>;
inline CGPoint postClick(const Request& r) {
    const CGPoint p = target(r);
    Event down(CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, p, kCGMouseButtonLeft));
    Event up(CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp, p, kCGMouseButtonLeft));
    require(down && up, "mouse allocation failed");
    for (CGEventRef event : {down.get(), up.get()}) {
        CGEventSetFlags(event, 0);
        CGEventSetIntegerValueField(event, kCGMouseEventClickState, 1);
        CGEventSetIntegerValueField(event, kCGMouseEventWindowUnderMousePointer, r.window);
        CGEventSetIntegerValueField(event, kCGMouseEventWindowUnderMousePointerThatCanHandleThisEvent, r.window);
    }
    // Fresh identity/focus/geometry/occlusion snapshot immediately before submitting.
    dispatchChecked(r, currentSnapshot(r), [&](CGPoint) {
        CGEventPost(kCGSessionEventTap, down.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        // Always balance a submitted down, even if focus changes during the hold.
        CGEventPost(kCGSessionEventTap, up.get());
    });
    return p; // Submission is not proof that an application acted on the click.
}
} // namespace scoped_input
