// SPDX-License-Identifier: MIT
#pragma once
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
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
#include <algorithm>
#include <vector>

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
using DockClock = std::chrono::steady_clock;
struct DockTimeNow { DockClock::time_point operator()() const {return DockClock::now();} };
struct DockWitness {
    Identity owner;
    CGWindowID window = 0;
    CGRect bounds{};
    CGPoint point{};
    uint8_t patchMaxAlpha = 255;
    std::optional<DockClock::time_point> captureRequestedAt = std::nullopt;
};
inline bool freshDockWitness(const DockWitness& w,DockClock::time_point now) {
    return w.captureRequestedAt && now >= *w.captureRequestedAt &&
        now - *w.captureRequestedAt <= std::chrono::milliseconds(250);
}
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
    std::optional<DockWitness> transparentDock = std::nullopt;
};
// Both the actual screenshot pipeline and the delayed-frame regression use this
// producer: a slow callback/processing step cannot replace the request timestamp.
template<class Capture,class ClockReader=DockTimeNow>
inline std::optional<DockWitness> timedDockCapture(const Snapshot& s,const Identity& dock,CGPoint point,
    Capture capture,ClockReader now={}) {
    const auto captureRequestedAt=now();
    const auto maximum=capture();
    if(!maximum)return std::nullopt;
    const DockWitness witness{dock,s.hitWindow,s.candidateBounds,point,*maximum,captureRequestedAt};
    return freshDockWitness(witness,now()) ? std::optional<DockWitness>(witness) : std::nullopt;
}
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
        @"transparent_Dock_window_id":s.transparentDock?(id)@(s.transparentDock->window):NSNull.null,
        @"transparent_Dock_patch_max_alpha":s.transparentDock?(id)@(s.transparentDock->patchMaxAlpha):NSNull.null,
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
template<class Sender,class ClockReader=DockTimeNow>
inline void dispatchChecked(const Request& r, const Snapshot& s, Sender send,ClockReader now={}) {
    const CGPoint point=validate(r,s);
    // Recheck after all validation, immediately before invoking the event sender.
    require(!s.transparentDock || freshDockWitness(*s.transparentDock,now()),
        "Dock transparency witness expired or unavailable");
    send(point);
}
inline Identity processIdentity(pid_t pid) {
    proc_bsdinfo info{};
    char path[PROC_PIDPATHINFO_MAXSIZE]{};
    require(proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) == sizeof(info) &&
        proc_pidpath(pid, path, sizeof(path)) > 0, "owned process unavailable");
    return {pid, info.pbi_uid, info.pbi_start_tvsec, info.pbi_start_tvusec, path};
}
inline bool systemDockBackdrop(const Snapshot& s,const Identity& dock) {
    return dock.pid > 0 && dock.birthSeconds > 0 && dock.birthMicros < 1000000 &&
        dock.uid == s.callerUid && dock.pid == s.candidateOwnerPid &&
        dock.executable == "/System/Library/CoreServices/Dock.app/Contents/MacOS/Dock" &&
        s.display != 0 && s.hitWindow != s.publicDisplayShield &&
        s.hitLayer == CGWindowLevelForKey(kCGDockWindowLevelKey) &&
        validFrame(s.candidateBounds) && CGRectEqualToRect(s.candidateBounds,s.displayBounds) &&
        s.candidateAlpha && *s.candidateAlpha > 0;
}
template<class ClockReader=DockTimeNow>
inline bool transparentDockWitness(const Request& r,const Snapshot& s,const Identity& dock,const DockWitness& w,
    ClockReader now={}) {
    return systemDockBackdrop(s,dock) && sameIdentity(dock,w.owner) &&
        s.inputPermission && s.active && s.frontmostPid == r.owner.pid &&
        sameIdentity(r.owner,s.owner) && s.callerUid == r.owner.uid &&
        s.window == r.window && s.windowPid == r.owner.pid &&
        CGRectEqualToRect(s.frame,r.capturedFrame) && s.hitPid == r.owner.pid &&
        CGRectEqualToRect(s.hitFrame,r.capturedFrame) && s.selectedLayer && *s.selectedLayer == 0 &&
        s.selectedAlpha && *s.selectedAlpha > 0 && w.window == s.hitWindow &&
        CGRectEqualToRect(w.bounds,s.candidateBounds) && CGPointEqualToPoint(w.point,target(r)) &&
        w.patchMaxAlpha == 0 && freshDockWitness(w,now());
}
// A point-alpha witness is acquired only for the protected system Dock window.
// No image leaves memory; unavailable capture/identity keeps the original refusal.
inline std::optional<DockWitness> captureDockWitness(const Snapshot& s,CGPoint point) {
    if(!CGPreflightScreenCaptureAccess())return std::nullopt;
    Identity dock;
    try {dock=processIdentity(s.candidateOwnerPid);}catch(const std::exception&){return std::nullopt;}
    if(!systemDockBackdrop(s,dock))return std::nullopt;
    auto wait=[](dispatch_semaphore_t signal) {
        auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(dispatch_semaphore_wait(signal,DISPATCH_TIME_NOW)) {
            if(std::chrono::steady_clock::now()>=until)return false;
            [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.01]];
        }
        return true;
    };
    __block SCShareableContent* content=nil; __block NSError* error=nil;
    auto signal=dispatch_semaphore_create(0);
    [SCShareableContent getShareableContentExcludingDesktopWindows:NO onScreenWindowsOnly:YES
        completionHandler:^(SCShareableContent* value,NSError* failure){content=value;error=failure;dispatch_semaphore_signal(signal);}];
    if(!wait(signal) || error || !content)return std::nullopt;
    SCWindow* window=nil;
    for(SCWindow* candidate in content.windows)
        if(candidate.windowID==s.hitWindow && candidate.owningApplication.processID==dock.pid &&
            candidate.isOnScreen && candidate.windowLayer==s.hitLayer && CGRectEqualToRect(candidate.frame,s.candidateBounds))window=candidate;
    if(!window)return std::nullopt;
    auto filter=[[SCContentFilter alloc] initWithDesktopIndependentWindow:window];
    const double scale=filter.pointPixelScale, width=window.frame.size.width*scale, height=window.frame.size.height*scale;
    if(!std::isfinite(scale) || scale <= 0 || width < 3 || height < 3 || width > 4096 || height > 4096 ||
        std::floor(width)!=width || std::floor(height)!=height ||
        filter.contentRect.size.width!=window.frame.size.width || filter.contentRect.size.height!=window.frame.size.height)return std::nullopt;
    auto config=[SCStreamConfiguration new];config.width=size_t(width);config.height=size_t(height);
    config.showsCursor=NO;config.showMouseClicks=NO;config.capturesAudio=NO;config.captureMicrophone=NO;
    config.ignoreShadowsSingleWindow=YES;config.includeChildWindows=NO;config.shouldBeOpaque=NO;
    return timedDockCapture(s,dock,point,[&]() -> std::optional<uint8_t> {
        // ARC owns the NSImage even when a callback arrives after timeout.
        __block NSImage* retained=nil;__block NSError* imageError=nil;
        auto screenshotSignal=dispatch_semaphore_create(0);
        [SCScreenshotManager captureImageWithFilter:filter configuration:config
            completionHandler:^(CGImageRef image,NSError* failure){if(image)retained=[[NSImage alloc] initWithCGImage:image size:NSZeroSize];imageError=failure;dispatch_semaphore_signal(screenshotSignal);}];
        if(!wait(screenshotSignal) || imageError || !retained)return std::nullopt;
        CGImageRef image=[retained CGImageForProposedRect:nullptr context:nil hints:nil];
        if(!image || CGImageGetWidth(image)!=config.width || CGImageGetHeight(image)!=config.height)return std::nullopt;
        const double px=(point.x-window.frame.origin.x)*scale, py=(point.y-window.frame.origin.y)*scale;
        if(px < 1 || py < 1 || px >= width-1 || py >= height-1)return std::nullopt;
        std::vector<uint8_t> pixels(size_t(width)*size_t(height)*4);
        auto rgb=CGColorSpaceCreateDeviceRGB();
        auto context=CGBitmapContextCreate(pixels.data(),size_t(width),size_t(height),8,size_t(width)*4,rgb,
            kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);CGColorSpaceRelease(rgb);
        if(!context)return std::nullopt;
        CGContextDrawImage(context,CGRectMake(0,0,width,height),image);CGContextRelease(context);
        uint8_t maximum=0;
        for(int y=int(py)-1;y<=int(py)+1;++y)for(int x=int(px)-1;x<=int(px)+1;++x)
            maximum=std::max(maximum,pixels[4*(size_t(y)*size_t(width)+size_t(x))+3]);
        return maximum;
    });
}
inline Snapshot snapshotMetadata(const Request& r,const std::optional<DockWitness>& witness=std::nullopt) {
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
    // AX hit-testing supplies a separate owner/frame witness; all unresolved covers block.
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
    uint32_t displayCount=0;CGDirectDisplayID display=0;
    if(CGGetDisplaysWithPoint(p,1,&display,&displayCount)==kCGErrorSuccess && displayCount==1) {
        s.display=display;s.displayBounds=CGDisplayBounds(display);s.publicDisplayShield=CGShieldingWindowID(display);
    }
    s.nowUnixSeconds=NSDate.date.timeIntervalSince1970;
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
        if(witness && s.hitWindow==witness->window) {
            try {
                if(transparentDockWitness(r,s,processIdentity(ownerPid),*witness)) {s.transparentDock=witness;continue;}
            }catch(const std::exception&) {} // No new witness from stale or unavailable identity.
        }
        break;
    }
    return s;
}
inline Snapshot currentSnapshot(const Request& r) {
    auto s=snapshotMetadata(r);
    // Avoid collecting any pixel witness unless all selected-target AX checks agree.
    if(s.active && s.frontmostPid==r.owner.pid && sameIdentity(s.owner,r.owner) &&
        s.hitPid==r.owner.pid && CGRectEqualToRect(s.hitFrame,r.capturedFrame) &&
        s.selectedLayer && *s.selectedLayer==0) {
        if(auto witness=captureDockWitness(s,target(r)))return snapshotMetadata(r,witness);
    }
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
