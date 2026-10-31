// SPDX-License-Identifier: MIT
// Real offscreen GPU verification of restored worlds; no App/window or simulation.
#import <AppKit/AppKit.h>
#include "city_playback.hpp"
#include "../original_world_checkpoint_io.hpp"
#include "../metal_scene_renderer/metal_scene_renderer.hpp"
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace {
using namespace game_lab;
using namespace game_lab::metal_scene;
size_t checks = 0;
void need(bool value, const std::string& reason) {
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
std::string hex(const OriginalWorldDigest& digest) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (auto b : digest) { result += digits[b >> 4]; result += digits[b & 15]; }
    return result;
}
std::string encoded(const OriginalWorldSnapshot& value) {
    std::string error;
    const auto bytes = encodeOriginalWorldCheckpoint(value, error);
    need(bool(bytes), error);
    return *bytes;
}
std::string imageHash(const DecodedImage& image) {
    need(image.valid(), "valid CPU/readback pixels");
    return hex(originalWorldDigest(image.pixelsBGRA.data(), image.pixelsBGRA.size() * 4));
}
std::string worldBytes(AppMapSurface& surface) {
    std::string error;
    auto value = surface.worldSnapshot(error);
    need(bool(value), error);
    return encoded(*value);
}
struct Waiter {
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<Completion> results;
    bool onMain = false;
    CompletionHandler callback() {
        return [this](Completion result) {
            std::lock_guard<std::mutex> lock(mutex);
            onMain = onMain || bool(NSThread.isMainThread);
            results.push_back(std::move(result)); changed.notify_all();
        };
    }
    Completion wait() {
        std::unique_lock<std::mutex> lock(mutex);
        need(changed.wait_for(lock, std::chrono::seconds(20), [&]{ return !results.empty(); }), "bounded GPU completion");
        need(results.size() == 1 && !onMain, "one callback on backend queue");
        return results.front();
    }
};
Completion submit(Renderer& renderer, std::shared_ptr<const SceneDrawList> list,
                  SceneFrameTicket ticket, std::vector<std::shared_ptr<Waiter>>& retained) {
    // Callback storage outlives accepted work even if a bounded wait throws.
    auto waiter = std::make_shared<Waiter>();
    retained.push_back(waiter);
    auto result = renderer.submit(std::move(list), ticket, [waiter](Completion value) {
        auto callback = waiter->callback(); callback(std::move(value));
    });
    need(result.status == SubmitStatus::Accepted, "GPU submit: " + result.reason);
    auto completion = waiter->wait();
    need(completion.ticket == ticket, "unchanged completion identity");
    return completion;
}
void pixelsEqual(const Frame& frame, const DecodedImage& cpu) {
    DecodedImage gpu; std::string error;
    need(readFrameForTesting(frame, gpu, error), error);
    need(gpu.width == cpu.width && gpu.height == cpu.height, "GPU readback geometry");
    size_t different = 0;
    for (size_t i = 0; i < cpu.pixelsBGRA.size(); ++i)
        different += cpu.pixelsBGRA[i] != gpu.pixelsBGRA[i];
    need(different == 0, "GPU pixels differ from restored CPU oracle: " + std::to_string(different));
}
void prepare(const ResourceRoot& root, const std::filesystem::path& output,
             const std::vector<std::string>& sources) {
    need(output.is_absolute(), "absolute checkpoint output required");
    const auto target = std::filesystem::weakly_canonical(output);
    const auto dataRoot = std::filesystem::canonical(root.configuredDirectory());
    const auto relative = target.lexically_relative(dataRoot);
    need(target != dataRoot && !relative.empty() && *relative.begin() == "..", "checkpoint output must be outside original data");
    need(!std::filesystem::exists(output), "new checkpoint output directory");
    need(sources.size() == 2, "exactly two original source files");
    const auto first = root.resolve(sources[0]), second = root.resolve(sources[1]);
    need(!first.empty() && !second.empty() && !std::filesystem::equivalent(first, second), "two distinct source file identities required");
    std::filesystem::create_directories(output);
    std::string error;
    for (size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
        auto source = bindOriginalWorldSource(root, sources[sourceIndex], error);
        need(bool(source), error);
        AppCityPlaybackPreferences preferences;
        need(readAppCityPlaybackPreferences(root, preferences, error), error);
        AppMapSurface surface(root); AppCitySessionState session;
        session.crtRandom = uint32_t(17 + sourceIndex);
        AppCityPlayback player(surface, source->save, source->payload, preferences,
            session.crtRandom, 0, false, &session, false, &*source);
        need(player.prepared(), "ordinary N0 checkpoint prepared");
        for (unsigned viewIndex = 0; viewIndex < 3; ++viewIndex) {
            auto camera = originalBuildingFocusCamera(source->save.buildings.footprints, OriginalMapCamera{});
            camera.outlines = false;
            camera.width = viewIndex == 0 ? 320 : viewIndex == 1 ? 257 : 127;
            camera.height = viewIndex == 0 ? 240 : viewIndex == 1 ? 193 : 95;
            camera.scale = viewIndex == 0 ? 1 : viewIndex == 1 ? 2 : 4;
            camera.centerX += .25; camera.centerY += .75;
            const auto cpu = player.render(camera);
            need(cpu.image.valid() && !cpu.bases.decodeFailures && !cpu.overlayDecodeFailures &&
                 !cpu.overlayInvalid && !cpu.groundUnresolved, "source CPU oracle complete");
            const auto snapshot = player.worldSnapshot(error); need(bool(snapshot), error);
            need(snapshot->continuation.updates == 0, "no ordinary runtime admission enabled");
            const auto bytes = encoded(*snapshot);
            const auto file = output / ("source" + std::to_string(sourceIndex) + "-view" + std::to_string(viewIndex) + ".emworld");
            auto result = writeOriginalWorldCheckpointNew(file.string(), root.configuredDirectory(), *snapshot);
            need(result.state == OriginalWorldPublication::PublishedDurable, result.error);
            std::cout << "{\"case\":\"" << file.filename().string() << "\",\"source_index\":" << sourceIndex
                      << ",\"view_index\":" << viewIndex << ",\"cpu_sha256\":\"" << imageHash(cpu.image)
                      << "\",\"world_sha256\":\"" << hex(originalWorldDigest(bytes.data(), bytes.size()))
                      << "\",\"width\":" << camera.width << ",\"height\":" << camera.height << "}\n";
        }
    }
}
void verify(const ResourceRoot& root, const std::string& path, const std::string& cpuHash,
            const std::string& checkpointHash) {
    std::string error;
    auto bytes = readOriginalWorldCheckpointFile(path, root.configuredDirectory(), error);
    need(bool(bytes), error);
    need(hex(originalWorldDigest(bytes->data(), bytes->size())) == checkpointHash, "fresh-process checkpoint hash");
    auto snapshot = decodeOriginalWorldCheckpoint(*bytes, error); need(bool(snapshot), error);
    auto surface = std::make_unique<AppMapSurface>(root); AppCitySessionState session;
    auto player = AppCityPlayback::fromWorldSnapshot(*surface, *snapshot, 1000, &session, error);
    need(bool(player) && player->status().state == AppCityPlaybackState::Paused, "disk restored paused controller");
    need(snapshot->camera.has_value(), "checkpoint carries exact camera");
    const auto camera = *snapshot->camera;
    const auto cpu = player->render(camera);
    need(imageHash(cpu.image) == cpuHash, "fresh-process CPU matches pre-save oracle");
    const auto stable = worldBytes(*surface);
    need(stable == *bytes, "initial full world bytes preserved by restore and same-camera render");
    SceneDrawCapture capture;
    auto deferred = surface->renderSavedBuildings(camera, -1, &capture, true);
    need(deferred.pixelsDeferred && capture.complete(),
         "restored deferred sprite list: deferred=" + std::to_string(deferred.pixelsDeferred) +
         " error=" + capture.error + " items=" + std::to_string(capture.list.items.size()) +
         " unavailable=" + std::to_string(deferred.bases.archiveUnavailable) +
         " unresolved=" + std::to_string(deferred.bases.unresolvedIndices) +
         " geometry=" + std::to_string(deferred.bases.geometryUnsupported));
    auto replay = rasterizeSceneDrawListCPU(capture.list);
    need(imageHash(replay) == cpuHash, "captured list replays exact CPU oracle");
    need(worldBytes(*surface) == stable, "draw capture preserves entire world");
    auto list = std::make_shared<const SceneDrawList>(std::move(capture.list));
    auto renderer = std::make_unique<Renderer>();
    need(renderer->waitForSelfTestForTesting(20) && renderer->available(), "real Metal unavailable or exact-blend selftest failed");
    auto caps = renderer->capabilities();
    need(caps.available && caps.registryId != 0 && caps.blend != BlendPath::None, "actual Metal device and exact blend path");
    SceneFrameTicket ticket{{1, snapshot->commitRevision, 1, 0, 0}, 1, camera.width, camera.height};
    std::vector<std::shared_ptr<Waiter>> retained;
    auto first = submit(*renderer, list, ticket, retained);
    need(first.status == CompletionStatus::Ready && first.frame, "restored GPU frame completed");
    need(first.frame->diagnostics().backgroundBytes == uint64_t(camera.width) * camera.height * 4,
         "actual restored background transferred by completed Metal command");
    need(first.frame->diagnostics().items == list->items.size(), "every submitted scene item accounted for");
    if (!list->items.empty()) need(first.frame->diagnostics().drawCalls > 0, "nonempty restored scene uses GPU draw commands");
    pixelsEqual(*first.frame, cpu.image);
    renderer->invalidateResources(); ++ticket.requestSerial; ++ticket.key.cacheGeneration;
    auto second = submit(*renderer, list, ticket, retained);
    need(second.status == CompletionStatus::Ready && second.frame, "GPU renders after resource invalidation");
    pixelsEqual(*first.frame, cpu.image); pixelsEqual(*second.frame, cpu.image);
    auto rejected = *snapshot; rejected.source.source.digest[0] ^= 1;
    need(!surface->restoreWorldSnapshot(rejected, error) && !error.empty(), "changed source refused");
    need(worldBytes(*surface) == stable, "failed replacement leaves full installed world unchanged");
    renderer->injectFaultForTesting(TestFault::FailAtCompletion); ++ticket.requestSerial;
    auto failed = submit(*renderer, list, ticket, retained);
    need(failed.status == CompletionStatus::Failed && !failed.frame, "GPU completion fault is explicit, no fake ready frame");
    need(worldBytes(*surface) == stable, "GPU failure does not change simulation or camera");
    pixelsEqual(*first.frame, cpu.image);
    // Texture ownership must survive both the source world and renderer/cache.
    player.reset(); surface.reset(); list.reset();
    renderer->shutdown(); need(renderer->waitIdleForTesting(20), "bounded renderer drain"); renderer.reset();
    for (const auto& waiter : retained) {
        std::lock_guard<std::mutex> lock(waiter->mutex);
        need(waiter->results.size() == 1 && !waiter->onMain, "one off-main callback after complete drain/destruction");
    }
    pixelsEqual(*first.frame, cpu.image); pixelsEqual(*second.frame, cpu.image);
    need(NSApp == nil, "no App or window was created");
    std::cout << "{\"passed\":true,\"checks\":" << checks
              << ",\"GPU_executed\":true,\"GUI\":false,\"visible_window\":false,\"ready_frames\":2,\"injected_failed_frames\":1"
              << ",\"scene_items\":" << first.frame->diagnostics().items
              << ",\"background_bytes\":" << first.frame->diagnostics().backgroundBytes
              << ",\"draw_calls\":" << first.frame->diagnostics().drawCalls
              << ",\"drawn_items\":" << first.frame->diagnostics().drawnItems
              << ",\"registry_id\":" << caps.registryId << ",\"blend\":\"" << blendPathName(caps.blend)
              << "\",\"pixels_per_frame\":" << cpu.image.pixelsBGRA.size() << ",\"different_pixels\":0}\n";
}
}
int main(int argc, char** argv) { @autoreleasepool { try {
    const char* configured = std::getenv("EMPEROR_GAMEDATA_ROOT");
    if (!configured || !*configured) { std::cout << "{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n"; return 0; }
    need(argc == 5, "--prepare output source0 source1 OR --verify checkpoint cpuHash worldHash");
    need(NSApp == nil, "no NSApplication before test");
    auto root = game_lab::ResourceRoot::fromAbsolute(configured);
    if (std::string(argv[1]) == "--prepare") prepare(root, argv[2], {argv[3], argv[4]});
    else if (std::string(argv[1]) == "--verify") verify(root, argv[2], argv[3], argv[4]);
    else throw std::runtime_error("unknown operation");
    return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } } }
