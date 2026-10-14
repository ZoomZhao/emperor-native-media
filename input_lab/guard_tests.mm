// SPDX-License-Identifier: MIT
#include "scoped_click.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <vector>
using namespace scoped_input;
int main() { @autoreleasepool {
    try {
        Identity owner{123, 501, 42, 1234, "/synthetic/Fixture"};
        Request good{owner, 77, CGRectMake(220, 242, 640, 448), 0.484375,
            247.0 / 448.0, 1100};
        Snapshot base{owner, 501, true, true, false, 123, 123, 123, 77, 77,
            good.capturedFrame, 1000};
        base.hitFrame=good.capturedFrame; base.candidateOwnerPid=owner.pid;
        unsigned sent = 0;
        dispatchChecked(good, base, [&](CGPoint p) {
            require(p.x == 530 && p.y == 489, "point conversion mismatch"); ++sent;
        });
        require(sent == 1, "valid control not submitted");
        require(occludesPoint(CGRectMake(525,484,28,40),1,
            CGWindowLevelForKey(kCGCursorWindowLevelKey),CGPointMake(530,489)),"legal app cursor-level window skipped");
        require(occludesPoint(good.capturedFrame,1,NSFloatingWindowLevel,CGPointMake(530,489)),
            "floating cover ignored");
        require(!occludesPoint(good.capturedFrame,0,NSFloatingWindowLevel,CGPointMake(530,489)),
            "invisible cover blocks target");
        struct Case { const char* reason; std::function<void(Request&, Snapshot&)> change; };
        const double nan = std::numeric_limits<double>::quiet_NaN();
        std::vector<Case> cases = {
            {"invalid normalized point", [=](auto& r, auto&) {r.normalizedX = nan;}},
            {"invalid normalized point", [](auto& r, auto&) {r.normalizedY = -0.01;}},
            {"invalid normalized point", [](auto& r, auto&) {r.normalizedX = 1;}},
            {"invalid captured frame", [](auto& r, auto&) {r.capturedFrame.size.width = 0;}},
            {"invalid captured frame", [=](auto& r, auto&) {r.capturedFrame.origin.y = nan;}},
            {"explicit window required", [](auto& r, auto&) {r.window = 0;}},
            {"invalid owner identity", [](auto& r, auto&) {r.owner.pid = 0;}},
            {"invalid owner identity", [](auto& r, auto&) {r.owner.executable = "relative";}},
            {"existing input permission unavailable", [](auto&, auto& s) {s.inputPermission = false;}},
            {"owner identity changed", [](auto&, auto& s) {s.owner.birthMicros++;}},
            {"owner identity changed", [](auto&, auto& s) {s.owner.birthSeconds++;}},
            {"owner identity changed", [](auto&, auto& s) {s.owner.pid++;}},
            {"owner identity changed", [](auto&, auto& s) {s.owner.executable += "-other";}},
            {"owner identity changed", [](auto&, auto& s) {s.owner.uid++;}},
            {"owner identity changed", [](auto&, auto& s) {s.callerUid++;}},
            {"selected window absent or foreign", [](auto&, auto& s) {s.window = 0;}},
            {"selected window absent or foreign", [](auto&, auto& s) {s.windowPid++;}},
            {"captured window moved or resized", [](auto&, auto& s) {s.frame.origin.x++;}},
            {"captured window moved or resized", [](auto&, auto& s) {s.frame.size.height++;}},
            {"owned app not frontmost", [](auto&, auto& s) {s.active = false;}},
            {"owned app not frontmost", [](auto&, auto& s) {s.frontmostPid++;}},
            {"selected point obscured", [](auto&, auto& s) {s.hitWindow++;}},
            {"selected point obscured", [](auto&, auto& s) {s.hitPid++;}},
            {"selected point obscured", [](auto&, auto& s) {s.hitFrame.size.height++;}},
            {"left button already held", [](auto&, auto& s) {s.leftButtonDown = true;}},
            {"expired or excessive deadline", [](auto& r, auto&) {r.deadlineUnixSeconds = 999;}},
            {"expired or excessive deadline", [](auto& r, auto&) {r.deadlineUnixSeconds = 1000.49;}},
            {"expired or excessive deadline", [](auto& r, auto&) {r.deadlineUnixSeconds = 2201;}},
            {"invalid deadline", [=](auto& r, auto&) {r.deadlineUnixSeconds = nan;}},
        };
        for (const auto& test : cases) {
            Request r = good; Snapshot s = base; test.change(r, s);
            bool rejected = false;
            try { dispatchChecked(r, s, [&](CGPoint) {++sent;}); }
            catch (const std::runtime_error& e) {rejected = std::string(e.what()) == test.reason;}
            require(rejected && sent == 1, "rejection submitted input or returned wrong reason");
        }
        const int cursorLevel=CGWindowLevelForKey(kCGCursorWindowLevelKey);
        require(candidateAtPoint(good.capturedFrame,1,cursorLevel,cursorLevel,CGPointMake(530,489),false),"legal cursor-level app excluded");
        Identity server{163,88,42,1234,"/System/Library/PrivateFrameworks/SkyLight.framework/Resources/WindowServer"};
        require(systemCursorWitness(server,501,cursorLevel,CGRectMake(525,484,28,40),CGPointMake(530,489)),"system cursor not recognized");
        require(!systemCursorWitness(owner,501,cursorLevel,CGRectMake(525,484,28,40),CGPointMake(530,489)),"app at cursor level exempted");
        require(!systemCursorWitness(server,501,cursorLevel,good.capturedFrame,CGPointMake(530,489)),"large system overlay exempted");
        Snapshot foreign=base; foreign.hitWindow=88; foreign.hitLayer=cursorLevel+1; foreign.candidateOwnerPid=999;
        require(candidateAtPoint(good.capturedFrame,1,cursorLevel+1,cursorLevel,CGPointMake(530,489),false),"foreign front cover skipped");
        bool blocked=false; try {dispatchChecked(good,foreign,[&](CGPoint){++sent;});}
        catch(const std::runtime_error& e){blocked=std::string(e.what())=="selected point obscured";}
        require(blocked&&sent==1,"foreign CG cover with own AX hit allowed");
        foreign.candidateOwnerPid=owner.pid; blocked=false;
        try {dispatchChecked(good,foreign,[&](CGPoint){++sent;});}
        catch(const std::runtime_error& e){blocked=std::string(e.what())=="selected point obscured";}
        require(blocked&&sent==1,"same-owner high cover allowed");
        require(!candidateAtPoint(good.capturedFrame,1,cursorLevel-2,cursorLevel,CGPointMake(530,489),false),"lower back window occludes high app");
        // A full-bounds Dock row is skipped only with fresh per-point transparency
        // and an independent target AX witness; no other owner gets this exception.
        Snapshot dockSnapshot=base;dockSnapshot.selectedLayer=0;dockSnapshot.selectedAlpha=1;
        dockSnapshot.display=1;dockSnapshot.displayBounds=CGRectMake(0,0,1440,900);
        dockSnapshot.hitWindow=88;dockSnapshot.candidateOwnerPid=456;
        dockSnapshot.hitLayer=CGWindowLevelForKey(kCGDockWindowLevelKey);
        dockSnapshot.candidateBounds=dockSnapshot.displayBounds;dockSnapshot.candidateAlpha=1;
        Identity dock{456,501,42,1234,"/System/Library/CoreServices/Dock.app/Contents/MacOS/Dock"};
        const auto acquired=DockClock::time_point(std::chrono::seconds(1000));
        const auto checked=acquired+std::chrono::milliseconds(100);
        DockWitness witness{dock,88,dockSnapshot.candidateBounds,target(good),0,acquired};
        require(transparentDockWitness(good,dockSnapshot,dock,witness,[=]{return checked;}),"proved transparent Dock point rejected");
        struct DockCase {std::function<void(Snapshot&,Identity&,DockWitness&)> change;};
        std::vector<DockCase> dockCases={
            {[](auto&,auto& d,auto&){d.executable="/third-party/Dock";}},
            {[](auto&,auto& d,auto&){d.uid++;}},
            {[](auto&,auto& d,auto&){d.birthSeconds++;}},
            {[=](auto&,auto&,auto& w){w.captureRequestedAt=acquired-std::chrono::milliseconds(151);}},
            {[=](auto&,auto&,auto& w){w.captureRequestedAt=checked+std::chrono::milliseconds(1);}},
            {[](auto&,auto&,auto& w){w.captureRequestedAt=std::nullopt;}},
            {[](auto&,auto&,auto& w){w.patchMaxAlpha=1;}},
            {[](auto&,auto&,auto& w){w.patchMaxAlpha=255;}},
            {[](auto&,auto&,auto& w){w.window++;}},
            {[](auto&,auto&,auto& w){w.bounds.size.width++;}},
            {[](auto&,auto&,auto& w){w.point.x++;}},
            {[](auto& s,auto&,auto&){s.hitPid=999;}},
            {[](auto& s,auto&,auto&){s.hitFrame.origin.x++;}},
            {[](auto& s,auto&,auto&){s.hitLayer++;}},
            {[](auto& s,auto&,auto&){s.publicDisplayShield=s.hitWindow;}},
            {[](auto& s,auto&,auto&){s.selectedLayer=CGWindowLevelForKey(kCGCursorWindowLevelKey);}},
            {[](auto& s,auto&,auto&){s.selectedAlpha=std::nullopt;}},
            {[](auto& s,auto&,auto&){s.candidateBounds.size.width--;}},
        };
        for(const auto& test:dockCases) {
            auto s=dockSnapshot;auto d=dock;auto w=witness;test.change(s,d,w);
            require(!transparentDockWitness(good,s,d,w,[=]{return checked;}),"unproved/opaque/foreign Dock candidate exempted");
        }
        // Acquisition starts at0ms; callback/processing resumes at400ms. Never
        // restamp the frame: the410ms observation must refuse it, with no UI.
        unsigned freshnessChecks=0;
        auto freshnessNeed=[&](bool result,const char* message){require(result,message);++freshnessChecks;};
        auto captureClock=acquired;
        const auto delayedCapture=timedDockCapture(dockSnapshot,dock,target(good),[&]() -> std::optional<uint8_t> {
            captureClock+=std::chrono::milliseconds(400);return uint8_t(0);
        },[&]{return captureClock;});
        freshnessNeed(!delayedCapture,"400ms capture processing was restamped and accepted");
        captureClock=acquired;
        const auto timelyCapture=timedDockCapture(dockSnapshot,dock,target(good),[&]() -> std::optional<uint8_t> {
            captureClock+=std::chrono::milliseconds(200);return uint8_t(0);
        },[&]{return captureClock;});
        freshnessNeed(timelyCapture && timelyCapture->captureRequestedAt==acquired,
            "capture producer did not preserve its request timestamp");
        const auto unknownCapture=timedDockCapture(dockSnapshot,dock,target(good),[]() -> std::optional<uint8_t> {
            return std::nullopt;
        },[&]{return captureClock;});
        freshnessNeed(!unknownCapture,"missing capture evidence accepted");
        freshnessNeed(!transparentDockWitness(good,dockSnapshot,dock,witness,[=]{return acquired+std::chrono::milliseconds(410);}),
            "400ms processing delay refreshed an old Dock frame");
        freshnessNeed(freshDockWitness(witness,acquired+std::chrono::milliseconds(250)),"250ms boundary incorrectly refused");
        freshnessNeed(!freshDockWitness(witness,acquired+std::chrono::milliseconds(251)),"expired boundary accepted");
        freshnessNeed(!freshDockWitness(witness,acquired-std::chrono::milliseconds(1)),"future request timestamp accepted");
        auto resolved=base;resolved.transparentDock=witness;
        unsigned finalSends=0;
        dispatchChecked(good,resolved,[&](CGPoint){++finalSends;},[=]{return checked;});
        freshnessNeed(finalSends==1,"fresh final-dispatch control refused");
        auto refusedAtDispatch=[&](Snapshot stale,DockClock::time_point now) {
            bool refused=false;
            try {dispatchChecked(good,stale,[&](CGPoint){++finalSends;},[=]{return now;});}
            catch(const std::runtime_error& e){refused=std::string(e.what())=="Dock transparency witness expired or unavailable";}
            freshnessNeed(refused && finalSends==1,"late/unknown witness dispatched an event");
        };
        // The same resolved snapshot ages after metadata validation but before send.
        refusedAtDispatch(resolved,acquired+std::chrono::milliseconds(410));
        resolved.transparentDock->captureRequestedAt=std::nullopt;refusedAtDispatch(resolved,checked);
        resolved.transparentDock->captureRequestedAt=checked+std::chrono::milliseconds(1);refusedAtDispatch(resolved,checked);
        // Deadline wall time cannot refresh a monotonic capture-age witness.
        resolved.transparentDock=witness;resolved.nowUnixSeconds=1000.40;
        refusedAtDispatch(resolved,acquired+std::chrono::milliseconds(410));
        // Exact public API ID identifies a display-capture shield; level/name do not.
        Snapshot displayShield = base;
        displayShield.hitWindow = 88; displayShield.hitLayer = CGShieldingWindowLevel();
        displayShield.display = 1; displayShield.publicDisplayShield = 88;
        require(std::string(candidateRole(displayShield))=="public_display_capture_shield","public shield not identified");
        bool stillBlocked = false;
        try {dispatchChecked(good,displayShield,[&](CGPoint){++sent;});}
        catch (const std::runtime_error& e) {stillBlocked=std::string(e.what())=="selected point obscured";}
        require(stillBlocked && sent==1,"diagnostic role accidentally permits input");
        displayShield.hitPid = base.hitPid + 1;
        bool foreignBlocked = false;
        try {dispatchChecked(good,displayShield,[&](CGPoint){++sent;});}
        catch (const std::runtime_error& e) {foreignBlocked=std::string(e.what())=="selected point obscured";}
        require(foreignBlocked && sent==1,"public shield identity erased foreign-window protection");
        displayShield.hitPid = base.hitPid;
        displayShield.publicDisplayShield = 89;
        require(std::string(candidateRole(displayShield))=="unclassified_window","same-level spoof called shield");
        displayShield.publicDisplayShield = 0;
        require(std::string(candidateRole(displayShield))=="unclassified_window","missing API shield ID guessed");
        NSDictionary* missing = inspectionJSON(good,base);
        require(missing[@"candidate_alpha"]==NSNull.null && missing[@"selected_layer"]==NSNull.null &&
            ![missing[@"input_sent"] boolValue],"missing metadata fabricated or inspection sent input");
        std::cout << "{\"valid_fake_submission\":1,\"rejections\":" << cases.size()
            << ",\"occlusion_cases\":3,\"layer_regression_cases\":7,\"shield_diagnostic_cases\":5,\"Dock_witness_cases\":" << dockCases.size()+1
            << ",\"Dock_freshness_checks\":" << freshnessChecks
            << ",\"rejected_fake_submissions\":0,\"UI_events_posted\":0}\n";
        return 0;
    } catch (const std::exception& e) {std::cerr << e.what() << '\n'; return 1;}
}}
