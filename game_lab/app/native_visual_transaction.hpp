// SPDX-License-Identifier: MIT
#pragma once
#include "../scene_frame_flow.hpp"
#include "../metal_scene_renderer/metal_scene_renderer.hpp"
#include "../native_session_source.hpp"
#include "map_surface.hpp"
#import <AppKit/AppKit.h>
// Only the explicit process-local EMPEROR_NATIVE_METAL=1 switch uses this path.
// Owned candidate survives backend/display callbacks. It is NOT live city state.
enum class NativeInstallResult { Failed, Installed, Pending };
struct NativeVisualTransaction {
    std::shared_ptr<const game_lab::NativeSessionSource> source;
    game_lab::NativeSessionState candidate;
    game_lab::OriginalMapCamera camera;
    std::shared_ptr<game_lab::AppMapSurface> surface;
    std::shared_ptr<const game_lab::DecodedImage> cpuPixels;
    std::shared_ptr<const game_lab::SceneDrawList> drawList;
    std::shared_ptr<const game_lab::metal_scene::Frame> frame;
    game_lab::SceneFrameFlow flow;
    NSImage* cpuImage=nil;
    void (^completion)(BOOL)=nil; // always weak App owner; not a retained delegate
    double startedAt=0,prepareMs=0,fallbackRasterMs=0;size_t preparedCPUSpriteDraws=0;
    bool settled=false,installed=false;
};
