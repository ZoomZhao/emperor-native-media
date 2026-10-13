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
            << ",\"occlusion_cases\":3,\"layer_regression_cases\":7,\"shield_diagnostic_cases\":5,\"rejected_fake_submissions\":0,\"UI_events_posted\":0}\n";
        return 0;
    } catch (const std::exception& e) {std::cerr << e.what() << '\n'; return 1;}
}}
