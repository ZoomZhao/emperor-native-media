#import <Foundation/Foundation.h>
#include "app_media.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
size_t checks = 0;
void require(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
struct UnusedAudio final : game_lab::AudioFactory {
    size_t calls = 0;
    std::unique_ptr<game_lab::AudioPlayback> create(const std::vector<uint8_t>&) override {
        ++calls;
        throw std::runtime_error("text loading must not create audio playback");
    }
};
void write(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    self_fixture::bytes("text-resource", bytes);
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!bytes.empty()) output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    output.close();
    require(bool(output), "self-authored fixture written");
}
void failed(const game_lab::TextLoadResult& result, game_lab::TextLoadState expected) {
    require(result.state == expected, "actual failure classification preserved to AppMedia");
    require(result.lines.empty(), "failed result contains no stale or partially decoded lines");
    const auto title = game_lab::formatTextLoadResult(result);
    require(!title.empty() && title.find("文本库 (共") == std::string::npos,
            "failure does not show the loaded text-library title");
    if (expected != game_lab::TextLoadState::NoText)
        require(title.find("失败") != std::string::npos, "failed load has explicit failure feedback");
}
void loaded(const game_lab::TextLoadResult& result) {
    require(result.state == game_lab::TextLoadState::Loaded, "successful retry is loaded");
    require(result.lines == std::vector<std::string>({"中文","two"}),
            "actual GB18030 conversion and trimming preserve exact new lines");
    const auto title = game_lab::formatTextLoadResult(result);
    require(title.find("共 2 条") != std::string::npos && title.find("失败") == std::string::npos,
            "successful retry replaces the failure feedback with the actual new count");
}
}
int main() {
    try {
        @autoreleasepool {
            const auto root = std::filesystem::current_path() / "fixtures" / "text-root";
            std::filesystem::create_directories(root);
            auto resource = game_lab::ResourceRoot::fromAbsolute(root.string());
            require(resource.configured(), "test resource root configured");
            UnusedAudio audio;
            game_lab::AppMedia app(resource,audio);
            const auto file = root / "EmperorText.txt";
            failed(app.textWithStatus("EmperorText.txt"),game_lab::TextLoadState::ResourceFailure);
            const std::vector<uint8_t> valid{' ',' ',0xd6,0xd0,0xce,0xc4,'\r','\n','t','w','o','\n'};
            write(file,valid);
            loaded(app.textWithStatus("EmperorText.txt"));
            require(app.text("EmperorText.txt") == std::vector<std::string>({"中文","two"}),
                    "existing vector API retains valid behavior");
            // Repair the same filename after each failure, without rebuilding AppMedia.
            for (const auto& input : {std::vector<uint8_t>{}, std::vector<uint8_t>{' ','\r','\n','\t'},
                                     std::vector<uint8_t>{0xff}, std::vector<uint8_t>{'o','k','\n',0xff}}) {
                write(file,input);
                const auto state = input.empty() || input.front() == ' ' ?
                    game_lab::TextLoadState::NoText : game_lab::TextLoadState::DecodeFailure;
                failed(app.textWithStatus("EmperorText.txt"),state);
                require(app.text("EmperorText.txt").empty(), "existing vector API remains empty on failure");
                write(file,valid);
                loaded(app.textWithStatus("EmperorText.txt"));
            }
            std::filesystem::remove(file);
            failed(app.textWithStatus("EmperorText.txt"),game_lab::TextLoadState::ResourceFailure);
            write(file,valid);
            loaded(app.textWithStatus("EmperorText.txt"));
            failed(app.textWithStatus("../EmperorText.txt"),game_lab::TextLoadState::ResourceFailure);
            failed(app.textWithStatus(""),game_lab::TextLoadState::ResourceFailure);
            const auto oversized = root / "too-large.txt";
            std::ofstream sparse(oversized,std::ios::binary);
            sparse.seekp(game_lab::kMaxTextBytes); sparse.put('x'); sparse.close();
            require(bool(sparse), "bounded-read fixture created");
            failed(app.textWithStatus("too-large.txt"),game_lab::TextLoadState::ResourceFailure);
            loaded(app.textWithStatus("EmperorText.txt"));
            require(audio.calls == 0 && app.muted(), "load and retry do not touch audio playback");
        }
        std::cout << "{\"passed\":true,\"checks\":" << checks
                  << ",\"same_path_repair_retries\":6,\"self_authored_inputs\":true,"
                     "\"UI_used\":false,\"audio_devices_used\":false}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
