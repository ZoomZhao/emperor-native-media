#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include "app_media.hpp"
#include "movie_session.hpp"
#include "image_presentation.hpp"
#include "app_launch.hpp"
#include "map_geometry.hpp"
#include "city_simulation.hpp"
#include "../stream_lab/movie_io.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <cstdio>
#include <zlib.h>

namespace {
size_t checks = 0;
void require(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}
void drainMovieDeliveries() {
    const auto done = std::make_shared<std::atomic<bool>>(false);
    dispatch_async(dispatch_get_main_queue(), ^{ done->store(true); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!done->load() && std::chrono::steady_clock::now() < deadline) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);
        usleep(1000);
    }
    require(done->load(), "headless owned main deliveries drained");
}
void put16(std::vector<uint8_t>& v, uint16_t n) { v.push_back(n & 255); v.push_back(n >> 8); }
void put32(std::vector<uint8_t>& v, uint32_t n) { for (int i=0;i<4;++i) v.push_back((n >> (8*i)) & 255); }
void set16(std::vector<uint8_t>& v, size_t off, uint16_t n) { v.at(off) = n & 255; v.at(off+1) = n >> 8; }
void set32(std::vector<uint8_t>& v, size_t off, uint32_t n) { for (int i=0;i<4;++i) v.at(off+i) = (n >> (8*i)) & 255; }
std::vector<uint8_t> wave(bool malformed = false) {
    std::vector<uint8_t> v{'R','I','F','F'};
    put32(v, malformed ? 42 : 40);
    v.insert(v.end(), {'W','A','V','E','f','m','t',' '}); put32(v, malformed ? 17 : 16);
    put16(v,1); put16(v,1); put32(v,22050); put32(v,44100); put16(v,2); put16(v,16);
    if (malformed) { v.push_back(0); v.push_back(0); }
    v.insert(v.end(), {'d','a','t','a'}); put32(v,4); put16(v,0x1234); put16(v,0xffff);
    return v;
}
void write(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    self_fixture::bytes("app-file",bytes);
    const auto name=path.string();
    self_fixture::bytes("app-file-path",std::vector<uint8_t>(name.begin(),name.end()));
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    require(bool(file),"synthetic fixture write");
}
struct Record {
    std::vector<uint8_t> bytes;
    double gain=0;
    int loops=0, plays=0, pauses=0, stops=0;
    bool playing=false, destroyed=false, allowPlay=true;
    bool insidePlay=false, destroyedDuringPlay=false;
    bool emitDuringPlay=false;
    game_lab::PlaybackEvent playEvent = game_lab::PlaybackEvent::Finished;
    game_lab::PlaybackListener* listener = nullptr;
    uint64_t ticket = 0;
};
class FakePlayback final : public game_lab::AudioPlayback {
public:
    explicit FakePlayback(std::shared_ptr<Record> record) : record_(std::move(record)) {}
    ~FakePlayback() override {
        record_->destroyed=true;
        record_->destroyedDuringPlay = record_->insidePlay;
    }
    void setListener(game_lab::PlaybackListener* listener, uint64_t ticket) override {
        record_->listener = listener;
        record_->ticket = ticket;
    }
    void setGain(double n) override { record_->gain=n; }
    void setLoops(int n) override { record_->loops=n; }
    bool play() override {
        ++record_->plays;
        record_->insidePlay = true;
        if (record_->emitDuringPlay && record_->listener && record_->ticket) {
            record_->listener->onPlaybackEvent(record_->ticket, record_->playEvent);
        }
        record_->playing=record_->allowPlay;
        record_->insidePlay = false;
        return record_->playing;
    }
    void pause() override { ++record_->pauses; record_->playing=false; }
    void stop() override { ++record_->stops; record_->playing=false; }
    bool playing() const override { return record_->playing; }
    double duration() const override { return 4.0/22050; }
private:
    std::shared_ptr<Record> record_;
};
class FakeFactory final : public game_lab::AudioFactory {
public:
    std::vector<std::shared_ptr<Record>> records;
    size_t calls=0;
    bool failCreate=false, failPlay=false;
    bool emitDuringPlay=false;
    game_lab::PlaybackEvent playEvent = game_lab::PlaybackEvent::Finished;
    std::unique_ptr<game_lab::AudioPlayback> create(const std::vector<uint8_t>& bytes) override {
        ++calls;
        self_fixture::bytes("factory-actual-data",bytes);
        if (failCreate) { failCreate=false; return {}; }
        auto r=std::make_shared<Record>(); r->bytes=bytes; r->allowPlay=!failPlay; failPlay=false;
        r->emitDuringPlay=emitDuringPlay; emitDuringPlay=false;
        r->playEvent=playEvent;
        records.push_back(r);
        return std::make_unique<FakePlayback>(r);
    }
};
class FakeImagePresenter final : public game_lab::ImagePresenter {
public:
    bool reject = false;
    size_t calls = 0;
    std::vector<uint32_t> image;
    void clear() override { image.clear(); }
    bool present(uint32_t width, uint32_t height, const std::vector<uint32_t>& pixels) override {
        ++calls;
        self_fixture::numbers("presenter-input", {width, height, reject ? 0u : 1u});
        const auto* bytes = reinterpret_cast<const uint8_t*>(pixels.data());
        self_fixture::bytes("presenter-pixels", std::vector<uint8_t>(bytes, bytes + pixels.size() * sizeof(uint32_t)));
        image = pixels; // A failed converter may leave partial output behind.
        return !reject;
    }
};
void presentation(uint32_t width, uint32_t height, const std::vector<uint32_t>& pixels) {
    FakeImagePresenter presenter;
    const auto show = [&] { return game_lab::presentImage(presenter, width, height, pixels); };
    require(show() == game_lab::PresentationResult::Presented && presenter.image == pixels,
            "valid decoded pixels reach the shared presenter");
    presenter.reject = true;
    require(show() == game_lab::PresentationResult::ConversionFailure && presenter.image.empty(),
            "conversion failure clears old and partial images and cannot report success");
    presenter.reject = false;
    require(show() == game_lab::PresentationResult::Presented && presenter.image == pixels,
            "valid replacement recovers after conversion failure");
    const auto calls = presenter.calls;
    require(game_lab::presentImage(presenter, 0, height, {}) == game_lab::PresentationResult::InvalidPixels &&
            presenter.calls == calls && presenter.image.empty(), "invalid input clears the old image before the converter");
    require(game_lab::presentImage(presenter, width, height, {}) == game_lab::PresentationResult::InvalidPixels &&
            presenter.calls == calls, "short pixel storage cannot reach the converter");
}
void notifications(const game_lab::ResourceRoot& root) {
    FakeFactory factory;
    game_lab::AppMedia app(root, factory);
    app.setMuted(false);
    app.selectMusic("Audio/Music/self.wav");
    app.playSfx("Audio/Wavs/self.wav");
    auto music = factory.records.front(), sfx = factory.records.back();
    std::vector<game_lab::PlaybackStatus> notices;
    app.setStatusChangedCallback([&](const game_lab::PlaybackStatus& status) { notices.push_back(status); });
    sfx->listener->onPlaybackEvent(sfx->ticket, game_lab::PlaybackEvent::Finished);
    require(notices.size() == 1 && notices.back().state == game_lab::PlaybackState::Playing && app.musicPlaying(),
            "SFX completion notifies once and preserves the active music status");
    sfx->listener->onPlaybackEvent(sfx->ticket, game_lab::PlaybackEvent::DecodeError);
    require(notices.size() == 1, "obsolete terminal event cannot notify again");
    app.playSfx("Audio/Wavs/self.wav");
    sfx = factory.records.back(); notices.clear();
    sfx->listener->onPlaybackEvent(sfx->ticket, game_lab::PlaybackEvent::DecodeError);
    require(notices.size() == 1 && notices.back().state == game_lab::PlaybackState::DecodeFailure,
            "active decode error notifies exactly once");
    sfx->listener->onPlaybackEvent(sfx->ticket, game_lab::PlaybackEvent::Finished);
    require(notices.size() == 1, "obsolete completion cannot replace the error notification");
    music->listener->onPlaybackEvent(music->ticket, game_lab::PlaybackEvent::Finished);
    require(notices.size() == 2 && notices.back().state == game_lab::PlaybackState::Stopped,
            "music completion notifies the stopped state without a user action");
    app.setStatusChangedCallback({});
    app.selectMusic("Audio/Music/self.wav"); music = factory.records.back();
    music->listener->onPlaybackEvent(music->ticket, game_lab::PlaybackEvent::Finished);
    require(notices.size() == 2, "unsubscribed observer receives no control or event notifications");
    app.selectMusic("Audio/Music/self.wav"); music = factory.records.back();
    bool snapshotStable = false; size_t reentrantCalls = 0;
    app.setStatusChangedCallback([&](const game_lab::PlaybackStatus& status) {
        ++reentrantCalls;
        const auto message = status.message;
        app.setStatusChangedCallback({});
        app.stopAll();
        snapshotStable = status.message == message;
    });
    music->listener->onPlaybackEvent(music->ticket, game_lab::PlaybackEvent::Finished);
    require(reentrantCalls == 1 && snapshotStable, "self-unsubscription and reentry preserve the notification snapshot");
    auto disposable = std::make_unique<game_lab::AppMedia>(root, factory);
    disposable->setMuted(false); disposable->selectMusic("Audio/Music/self.wav");
    auto retiring = factory.records.back();
    disposable->setStatusChangedCallback([&](const game_lab::PlaybackStatus&) { ++reentrantCalls; });
    disposable.reset();
    require(reentrantCalls == 1 && retiring->destroyed && !retiring->playing,
            "destruction unregisters the observer before playback cleanup");
}
void launches(const std::filesystem::path& files) {
    const auto environmentFiles = files.parent_path() / "self-env root";
    write(environmentFiles / "marker.wav", wave());
    size_t choices = 0;
    const auto choose = [&] {
        self_fixture::numbers("launch-choice", {++choices});
        const auto path = files.string();
        self_fixture::bytes("launch-choice-path", std::vector<uint8_t>(path.begin(), path.end()));
        return path;
    };
    const auto selected = game_lab::prepareLaunch({}, "", choose);
    require(selected.action == game_lab::LaunchAction::Start && choices == 1 && selected.root.configured(),
            "unconfigured launch accepts one explicit directory selection");
    FakeFactory factory;
    {
        game_lab::AppMedia media(selected.root, factory);
        require(media.muted() && factory.calls == 0, "selected launch root cannot auto-start audio");
        media.setMuted(false); media.playSfx("Audio/Wavs/self.wav");
        require(factory.calls == 1 && factory.records.back()->bytes == wave(),
                "chosen launch root reaches the actual bounded AppMedia audio path");
    }
    const auto explicitRoot = game_lab::prepareLaunch({"--data-root", files.string()}, environmentFiles.string(), choose);
    require(explicitRoot.action == game_lab::LaunchAction::Start && choices == 1 &&
            !explicitRoot.root.resolve("Audio/Wavs/self.wav").empty() && explicitRoot.root.resolve("marker.wav").empty(),
            "command-line root wins over environment and never opens the chooser");
    const auto environmentRoot = game_lab::prepareLaunch({}, environmentFiles.string(), choose);
    require(environmentRoot.action == game_lab::LaunchAction::Start && choices == 1 &&
            !environmentRoot.root.resolve("marker.wav").empty(), "explicit environment root with spaces avoids the chooser");
    for (const auto& arguments : std::vector<std::vector<std::string>>{
            {"--data-root"}, {"--unknown"}, {"--data-root", files.string(), "--data-root", files.string()}}) {
        const auto plan = game_lab::prepareLaunch(arguments, environmentFiles.string(), choose);
        require(plan.action == game_lab::LaunchAction::ArgumentError && choices == 1,
                "malformed/unknown/duplicate arguments fail before root choice");
    }
    for (const auto& path : std::vector<std::string>{"", "relative", (files.parent_path() / "missing-root").string()}) {
        const auto plan = game_lab::prepareLaunch({"--data-root", path}, environmentFiles.string(), choose);
        require(plan.action == game_lab::LaunchAction::RootError && choices == 1 && !plan.root.configured(),
                "invalid supplied root cannot fall back to environment or picker");
    }
    require(game_lab::prepareLaunch({}, "relative", choose).action == game_lab::LaunchAction::RootError && choices == 1,
            "invalid configured environment fails without a picker fallback");
    require(game_lab::prepareLaunch({"--help"}, "relative", choose).action == game_lab::LaunchAction::Help && choices == 1,
            "help does not inspect resources or open a picker");
    require(game_lab::prepareLaunch({}, "", {}).action == game_lab::LaunchAction::RootError,
            "headless caller without configuration fails rather than searching cwd");
    size_t cancellations = 0;
    const auto cancelled = game_lab::prepareLaunch({}, "", [&] { ++cancellations; return std::string{}; });
    require(cancelled.action == game_lab::LaunchAction::Cancelled && cancellations == 1 && !cancelled.root.configured(),
            "cancelled root choice ends startup without retry or resource access");
    size_t invalidSelections = 0;
    const auto invalid = game_lab::prepareLaunch({}, "", [&] { ++invalidSelections; return (files / "Audio/Wavs/self.wav").string(); });
    require(invalid.action == game_lab::LaunchAction::RootError && invalidSelections == 1,
            "picker output is validated as a directory rather than trusted");
}
std::vector<uint8_t> jpeg() {
    uint32_t pixels[2] = {0xff123456,0xffaabbcc};
    auto space=CGColorSpaceCreateDeviceRGB(); require(space,"JPEG fixture color space");
    auto context=CGBitmapContextCreate(pixels,2,1,8,8,space,kCGBitmapByteOrder32Little|kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(space); require(context,"JPEG fixture context");
    auto image=CGBitmapContextCreateImage(context); CGContextRelease(context); require(image,"JPEG fixture image");
    auto data=CFDataCreateMutable(nullptr,0); require(data,"JPEG fixture data");
    auto dest=CGImageDestinationCreateWithData(data,CFSTR("public.jpeg"),1,nullptr); require(dest,"JPEG fixture writer");
    CGImageDestinationAddImage(dest,image,nullptr);
    bool ok=CGImageDestinationFinalize(dest); CFRelease(dest); CGImageRelease(image);
    require(ok,"JPEG fixture finalize");
    auto bytes=std::vector<uint8_t>(CFDataGetBytePtr(data),CFDataGetBytePtr(data)+CFDataGetLength(data));
    CFRelease(data); return bytes;
}
std::vector<uint8_t> syntheticMap() {
    constexpr size_t tileCount = 128u * 128u;
    std::vector<uint8_t> raw(tileCount * 2, 0x42);
    uLongf bound = compressBound(raw.size());
    std::vector<uint8_t> compressed(bound);
    int res = compress(compressed.data(), &bound, raw.data(), raw.size());
    require(res == Z_OK, "zlib compress synthetic map");
    compressed.resize(bound);
    std::vector<uint8_t> fileBytes(16, 0);
    fileBytes.insert(fileBytes.end(), compressed.begin(), compressed.end());
    return fileBytes;
}
std::vector<uint8_t> syntheticSG3() {
    constexpr size_t tableOffset = 40680, recordSize = 64;
    std::vector<uint8_t> bytes(tableOffset + recordSize, 0);
    set32(bytes, 4, 213);
    set32(bytes, 16, 1);
    set32(bytes, tableOffset + 0, 0);
    set32(bytes, tableOffset + 4, 512);
    set32(bytes, tableOffset + 8, 512);
    set16(bytes, tableOffset + 12, 16);
    set16(bytes, tableOffset + 14, 16);
    set16(bytes, tableOffset + 20, 16);
    return bytes;
}
std::vector<uint8_t> synthetic555() {
    return std::vector<uint8_t>(512, 0x7c);
}
std::vector<uint8_t> syntheticText() {
    std::string text = "Line 1 Chinese Text\nLine 2 Second Line\n\n";
    return std::vector<uint8_t>(text.begin(), text.end());
}
void integration() {
    const auto cwd=std::filesystem::current_path();
    const auto files=cwd/"self-root";
    const auto pcm=wave();
    write(files/"Audio/Music/self.wav",pcm);
    write(files/"Audio/Wavs/self.wav",pcm);
    write(files/"Audio/Wavs/broken.wav",wave(true));
    write(cwd/"self-outside.wav",pcm);
    std::filesystem::create_symlink(cwd/"self-outside.wav",files/"escape.wav");
    const auto root=game_lab::ResourceRoot::fromAbsolute(files.string());
    require(root.configured(),"explicit synthetic resource root");
    notifications(root);
    launches(files);
    FakeFactory factory;
    {
        game_lab::AppMedia app(root,factory);
        require(app.muted() && factory.calls==0,"construction creates no player");
        app.selectMusic("Audio/Music/self.wav");
        require(factory.calls==0 && !app.musicPlaying(),"muted selection creates no player");
        app.playSfx("Audio/Wavs/self.wav");
        require(factory.calls==0,"muted SFX creates no player");
        app.setVolume(0.4); app.setMuted(false);
        require(factory.calls==0,"unmute and gain change do not start playback");
        app.toggleMusic();
        auto music=factory.records.back();
        require(factory.calls==1 && app.musicPlaying(),"explicit playback creates player");
        require(music->bytes==pcm && music->loops==-1 && music->gain==0.4,"same bounded bytes and music loop/gain reach backend");
        app.playSfx("Audio/Wavs/self.wav"); auto sfx=factory.records.back();
        require(factory.calls==2 && sfx->playing && sfx->loops==0 && sfx->gain==0.4,"SFX validates WAV and uses shared gain");
        app.setMuted(true);
        require(!music->playing && music->pauses==1 && music->gain==0,"mute pauses music");
        require(sfx->stops==1 && sfx->destroyed && sfx->gain==0,"mute stops and retires SFX");
        app.setMuted(false);
        require(factory.calls==2 && music->plays==1 && !app.musicPlaying(),"unmute cannot auto-resume");
        app.toggleMusic();
        require(factory.calls==2 && music->plays==2 && app.musicPlaying(),"explicit resume uses retained music player");
        app.toggleMusic(); require(!app.musicPlaying(),"explicit pause");
        app.toggleMusic(); require(app.musicPlaying(),"explicit resume");
        app.setVolume(0); require(!app.musicPlaying() && music->gain==0,"zero gain pauses music");
        app.setVolume(0.7); require(!app.musicPlaying(),"restoring gain does not resume");
        app.toggleMusic(); require(music->gain==0.7 && app.musicPlaying(),"resume propagates restored gain");
        app.selectMusic("missing.wav");
        require(music->stops==1 && music->destroyed && !app.musicPlaying(),"missing replacement retires old music");
        require(app.status().state==game_lab::PlaybackState::ResourceFailure && factory.calls==2,"missing path rejected before factory");
        for (const std::string& path : std::vector<std::string>{"../self-outside.wav",(cwd/"self-outside.wav").string(),"escape.wav"}) {
            app.selectMusic(path);
            require(factory.calls==2 && app.status().state==game_lab::PlaybackState::ResourceFailure,"App music root escape rejected");
        }
        app.playSfx("Audio/Wavs/self.wav"); sfx=factory.records.back();
        const auto calls=factory.calls;
        app.playSfx("Audio/Wavs/broken.wav");
        require(sfx->stops==1 && sfx->destroyed,"invalid WAV replacement retires old SFX");
        require(factory.calls==calls && app.status().state==game_lab::PlaybackState::DecodeFailure,"WAV extension regression rejected by actual App service before native factory");
        app.playSfx("Audio/Wavs/self.wav"); sfx=factory.records.back();
        app.playSfx("missing.wav");
        require(sfx->destroyed && sfx->stops==1 && app.status().state==game_lab::PlaybackState::ResourceFailure,"missing SFX clears previous playback");
        factory.failCreate=true; app.selectMusic("Audio/Music/self.wav");
        require(!app.musicPlaying() && app.status().state==game_lab::PlaybackState::DecodeFailure,"native factory error reaches App status");
        factory.failPlay=true; app.toggleMusic(); auto failed=factory.records.back();
        require(failed->stops==1 && failed->destroyed && app.status().state==game_lab::PlaybackState::StartFailure,"failed music start releases player");
        factory.failPlay=true; app.playSfx("Audio/Wavs/self.wav"); failed=factory.records.back();
        require(failed->stops==1 && failed->destroyed && app.status().state==game_lab::PlaybackState::StartFailure,"failed SFX start releases player");
        app.selectMusic("Audio/Music/self.wav"); music=factory.records.back();
        app.toggleMusic(); music->allowPlay=false; app.toggleMusic();
        require(music->destroyed && music->stops==1 && !app.musicPlaying(),"failed resume retires retained player");
        app.setVolume(std::numeric_limits<double>::quiet_NaN());
        const auto noCreate=factory.calls; app.toggleMusic();
        require(factory.calls==noCreate && app.status().state==game_lab::PlaybackState::Muted,"nonfinite gain prevents backend call");
        app.setVolume(0.6); app.selectMusic("Audio/Music/self.wav"); music=factory.records.back();
        app.playSfx("Audio/Wavs/self.wav"); sfx=factory.records.back();
        app.stopAll(); app.stopAll();
        require(music->stops==1 && sfx->stops==1 && music->destroyed && sfx->destroyed,"termination clears both players idempotently");
        require(app.status().state==game_lab::PlaybackState::Stopped,"termination status");
        const auto image=jpeg(); write(files/"self.jpg",image);
        auto decoded=app.jpeg("self.jpg");
        require(decoded.valid() && decoded.width==2 && decoded.height==1,"App JPEG loading shares production decoder");
        presentation(decoded.width, decoded.height, decoded.pixelsBGRA);
        auto broken=image; broken.pop_back(); write(files/"broken.jpg",broken);
        require(!app.jpeg("broken.jpg").valid(),"App JPEG truncation regression");
        require(!app.jpeg("missing.jpg").valid() && !app.jpeg("../self.jpg").valid(),"App JPEG missing/traversal invalid");
        write(files/"self.555",{0,0,255,127});
        auto raw=app.raw555("self.555",2,1);
        require(raw.valid() && raw.pixelsBGRA==std::vector<uint32_t>({0xff000000,0xffffffff}),"App raw loading uses actual surface conversion");
        presentation(raw.width, raw.height, raw.pixelsBGRA);
        require(!app.raw555("self.555",3,1).valid(),"App raw invalid replacement returns no pixels");
        require(!app.raw555("escape.wav",2,1).valid(),"App raw external symlink rejected");
        std::ofstream oversized(files/"oversized.wav",std::ios::binary);
        oversized.seekp(game_lab::kMaxAssetBytes); oversized.put(0); oversized.close();
        const auto before=factory.calls; app.selectMusic("oversized.wav");
        require(factory.calls==before && app.status().state==game_lab::PlaybackState::ResourceFailure,"App music size limit before native factory");
        app.playSfx("oversized.wav");
        require(factory.calls==before && app.status().state==game_lab::PlaybackState::ResourceFailure,"App SFX size limit before native factory");

        // E4 Native Event Lifecycle: natural completion, ticket validation, async decode error, obsolete events
        app.playSfx("Audio/Wavs/self.wav");
        sfx = factory.records.back();
        const auto activeTicket = sfx->ticket;
        require(activeTicket > 0 && sfx->playing && app.status().state == game_lab::PlaybackState::Playing,
                "SFX starts with non-zero ticket");
        sfx->listener->onPlaybackEvent(activeTicket, game_lab::PlaybackEvent::Finished);
        require(sfx->destroyed && app.status().state == game_lab::PlaybackState::Stopped,
                "natural completion event retires SFX and updates status");

        // Stale ticket invalidation on replacement:
        app.playSfx("Audio/Wavs/self.wav");
        auto sfx1 = factory.records.back();
        const auto ticket1 = sfx1->ticket;
        app.playSfx("Audio/Wavs/self.wav");
        auto sfx2 = factory.records.back();
        const auto ticket2 = sfx2->ticket;
        require(ticket2 > ticket1, "replacement assigns newer ticket");
        sfx1->listener->onPlaybackEvent(ticket1, game_lab::PlaybackEvent::Finished);
        require(!sfx2->destroyed && sfx2->playing && app.status().state == game_lab::PlaybackState::Playing,
                "stale ticket event cannot retire newer active player");

        // Async decode error event retires active player:
        sfx2->listener->onPlaybackEvent(ticket2, game_lab::PlaybackEvent::DecodeError);
        require(sfx2->destroyed && app.status().state == game_lab::PlaybackState::DecodeFailure,
                "async decode error event retires player and sets failure status");

        // Stop/mute invalidates ticket:
        app.playSfx("Audio/Wavs/self.wav");
        auto sfx3 = factory.records.back();
        const auto ticket3 = sfx3->ticket;
        app.stopAll();
        require(app.status().state == game_lab::PlaybackState::Stopped, "stopAll sets Stopped");
        sfx3->listener->onPlaybackEvent(ticket3, game_lab::PlaybackEvent::Finished);
        require(app.status().state == game_lab::PlaybackState::Stopped,
                "obsolete event after stopAll ignored");

        // Music natural finish and async error:
        app.selectMusic("Audio/Music/self.wav");
        music = factory.records.back();
        const auto musicT = music->ticket;
        require(musicT > 0 && music->playing, "music starts with ticket");
        music->listener->onPlaybackEvent(musicT, game_lab::PlaybackEvent::Finished);
        require(music->destroyed && app.status().state == game_lab::PlaybackState::Stopped,
                "music finish event retires player and updates status");
        app.selectMusic("Audio/Music/self.wav");
        music = factory.records.back();
        const auto musicErrT = music->ticket;
        music->listener->onPlaybackEvent(musicErrT, game_lab::PlaybackEvent::DecodeError);
        require(music->destroyed && app.status().state == game_lab::PlaybackState::DecodeFailure,
                "music async decode error retires player and sets failure status");

        // Six synchronous false-return scenarios with terminal state preservation and lifetime:
        // 1. music-start with DecodeError returning false:
        factory.emitDuringPlay = true;
        factory.playEvent = game_lab::PlaybackEvent::DecodeError;
        factory.failPlay = true;
        app.selectMusic("Audio/Music/self.wav");
        auto syncMusicErr = factory.records.back();
        require(syncMusicErr->destroyed && !syncMusicErr->playing && !app.musicPlaying(),
                "music-start sync decode error retired player");
        require(!syncMusicErr->destroyedDuringPlay,
                "music-start player survived own play callback");
        require(app.status().state == game_lab::PlaybackState::DecodeFailure,
                "music-start sync decode error preserved terminal state");

        // 2. music-start with Finished returning false:
        factory.emitDuringPlay = true;
        factory.playEvent = game_lab::PlaybackEvent::Finished;
        factory.failPlay = true;
        app.selectMusic("Audio/Music/self.wav");
        auto syncMusicFin = factory.records.back();
        require(syncMusicFin->destroyed && !syncMusicFin->playing && !app.musicPlaying(),
                "music-start sync finished retired player");
        require(!syncMusicFin->destroyedDuringPlay,
                "music-start player survived own play callback");
        require(app.status().state == game_lab::PlaybackState::Stopped,
                "music-start sync finished preserved terminal state");

        // 3. music-resume with DecodeError returning false:
        app.selectMusic("Audio/Music/self.wav"); // starts normally
        app.toggleMusic(); // pauses
        auto syncResumeErr = factory.records.back();
        syncResumeErr->emitDuringPlay = true;
        syncResumeErr->playEvent = game_lab::PlaybackEvent::DecodeError;
        syncResumeErr->allowPlay = false;
        app.toggleMusic(); // resumes
        require(syncResumeErr->destroyed && !syncResumeErr->playing && !app.musicPlaying(),
                "music-resume sync decode error retired player");
        require(!syncResumeErr->destroyedDuringPlay,
                "music-resume player survived own play callback");
        require(app.status().state == game_lab::PlaybackState::DecodeFailure,
                "music-resume sync decode error preserved terminal state");

        // 4. music-resume with Finished returning false:
        app.selectMusic("Audio/Music/self.wav"); // starts normally
        app.toggleMusic(); // pauses
        auto syncResumeFin = factory.records.back();
        syncResumeFin->emitDuringPlay = true;
        syncResumeFin->playEvent = game_lab::PlaybackEvent::Finished;
        syncResumeFin->allowPlay = false;
        app.toggleMusic(); // resumes
        require(syncResumeFin->destroyed && !syncResumeFin->playing && !app.musicPlaying(),
                "music-resume sync finished retired player");
        require(!syncResumeFin->destroyedDuringPlay,
                "music-resume player survived own play callback");
        require(app.status().state == game_lab::PlaybackState::Stopped,
                "music-resume sync finished preserved terminal state");

        // 5. sfx-start with DecodeError returning false:
        factory.emitDuringPlay = true;
        factory.playEvent = game_lab::PlaybackEvent::DecodeError;
        factory.failPlay = true;
        app.playSfx("Audio/Wavs/self.wav");
        auto syncSfxErr = factory.records.back();
        require(syncSfxErr->destroyed && !syncSfxErr->playing,
                "sfx-start sync decode error retired player");
        require(!syncSfxErr->destroyedDuringPlay,
                "sfx-start player survived own play callback");
        require(app.status().state == game_lab::PlaybackState::DecodeFailure,
                "sfx-start sync decode error preserved terminal state");

        // 6. sfx-start with Finished returning false:
        factory.emitDuringPlay = true;
        factory.playEvent = game_lab::PlaybackEvent::Finished;
        factory.failPlay = true;
        app.playSfx("Audio/Wavs/self.wav");
        auto syncSfxFin = factory.records.back();
        require(syncSfxFin->destroyed && !syncSfxFin->playing,
                "sfx-start sync finished retired player");
        require(!syncSfxFin->destroyedDuringPlay,
                "sfx-start player survived own play callback");
        require(app.status().state == game_lab::PlaybackState::Stopped,
                "sfx-start sync finished preserved terminal state");

        // AppMedia CityMap loading through ResourceRoot:
        write(files/"Cities/test.map", syntheticMap());
        auto map = app.cityMap("Cities/test.map");
        require(map.valid() && map.width == 128 && map.height == 128 && map.tiles.size() == 128 * 128,
                "AppMedia cityMap loads valid synthetic 128x128 map");
        presentation(map.width, map.height, std::vector<uint32_t>(map.tiles.size(), 0xff456789));
        require(!app.cityMap("../test.map").valid() && !app.cityMap("missing.map").valid(),
                "AppMedia cityMap traversal/missing fail-closed");

        // AppMedia SG3 and Sprite extraction through ResourceRoot:
        write(files/"DATA/test.sg3", syntheticSG3());
        write(files/"DATA/test.555", synthetic555());
        auto archive = app.sg3("DATA/test.sg3");
        require(archive.version == 213 && archive.sprites.size() == 1,
                "AppMedia sg3 loads valid archive header and sprite table");
        auto spr = app.sprite(archive.sprites[0], "DATA/test.555");
        require(spr.valid() && spr.width == 16 && spr.height == 16,
                "AppMedia sprite extracts 16x16 raw555 from companion");
        presentation(spr.width, spr.height, spr.pixelsBGRA);
        require(!app.sg3("../test.sg3").sprites.size() && !app.sprite(archive.sprites[0], "../test.555").valid(),
                "AppMedia sg3/sprite traversal fail-closed");

        // AppMedia Game Text loading through ResourceRoot:
        write(files/"EmperorText.txt", syntheticText());
        auto lines = app.text("EmperorText.txt");
        require(lines.size() == 2 && lines[0] == "Line 1 Chinese Text",
                "AppMedia text loads trimmed lines from EmperorText.txt");
        require(app.text("../EmperorText.txt").empty() && app.text("missing.txt").empty(),
                "AppMedia text traversal/missing fail-closed");

        // AppMedia Movie Poster loading through ResourceRoot:
        const auto movieRel = "Movies/self_authored_8s.mov";
        const auto movieAbs = files / "Movies/self_authored_8s.mov";
        std::filesystem::create_directories(movieAbs.parent_path());
        stream_lab::writeSyntheticMovie(movieAbs.string());

        // Capture self-authored MOV fixture bytes into input evidence:
        {
            std::ifstream movStream(movieAbs, std::ios::binary);
            std::vector<uint8_t> movBytes((std::istreambuf_iterator<char>(movStream)),
                                           std::istreambuf_iterator<char>());
            require(!movBytes.empty(), "synthetic movie must have non-empty bytes");
            self_fixture::bytes("app-movie-mov", movBytes);
        }

        auto poster = app.moviePoster(movieRel);
        require(poster.valid() && poster.width == 64 && poster.height == 48,
                "AppMedia moviePoster loads valid 64x48 first frame");
        require(poster.pixelsBGRA.size() == 64 * 48,
                "AppMedia moviePoster returns tight BGRA pixels without stride padding");

        // Verify first frame pixels with color error <= 16 (PTS=0 -> expected RGB: 160, 80, 40):
        for (size_t band = 0; band < 8; ++band) {
            for (size_t y = 2; y < 48 - 2; ++y) {
                for (size_t x = band * 8 + 2; x < band * 8 + 6; ++x) {
                    const uint32_t pixel = poster.pixelsBGRA[y * 64 + x];
                    const int b = int(pixel & 0xff);
                    const int g = int((pixel >> 8) & 0xff);
                    const int r = int((pixel >> 16) & 0xff);
                    const int a = int((pixel >> 24) & 0xff);
                    require(a == 255, "movie poster alpha must be 255");
                    require(std::abs(b - 40) <= 16, "movie poster blue error <= 16");
                    require(std::abs(g - 80) <= 16, "movie poster green error <= 16");
                    require(std::abs(r - 160) <= 16, "movie poster red error <= 16");
                }
            }
        }
        for (const auto pixel : poster.pixelsBGRA) {
            require(((pixel >> 24) & 0xff) == 255, "all poster pixels have alpha 255");
        }

        // Shared presentation contract with FakeImagePresenter:
        FakeImagePresenter moviePresenter;
        require(game_lab::presentImage(moviePresenter, poster.width, poster.height, poster.pixelsBGRA) == game_lab::PresentationResult::Presented &&
                moviePresenter.image == poster.pixelsBGRA,
                "valid movie poster presented successfully");

        // Error boundary: empty relative path clears presenter
        auto emptyPoster = app.moviePoster("");
        require(!emptyPoster.valid() && emptyPoster.pixelsBGRA.empty(),
                "empty movie path rejected fail-closed");
        require(game_lab::presentImage(moviePresenter, emptyPoster.width, emptyPoster.height, emptyPoster.pixelsBGRA) == game_lab::PresentationResult::InvalidPixels &&
                moviePresenter.image.empty(),
                "invalid empty poster clears old image in presenter");

        // Error boundary: missing file clears presenter
        auto missingPoster = app.moviePoster("Movies/missing.mov");
        require(!missingPoster.valid() && missingPoster.pixelsBGRA.empty(),
                "missing movie rejected fail-closed");
        require(game_lab::presentImage(moviePresenter, missingPoster.width, missingPoster.height, missingPoster.pixelsBGRA) == game_lab::PresentationResult::InvalidPixels &&
                moviePresenter.image.empty(),
                "missing movie clears old image in presenter");

        // Error boundary: path traversal rejected
        auto escapePoster = app.moviePoster("../escape.mov");
        require(!escapePoster.valid() && escapePoster.pixelsBGRA.empty(),
                "movie path traversal rejected fail-closed");

        // Error boundary: corrupt media rejected and clears presenter
        write(files / "Movies/corrupt.mov", {0, 0, 0, 8, 'f', 't', 'y', 'p', 0, 0});
        auto corruptPoster = app.moviePoster("Movies/corrupt.mov");
        require(!corruptPoster.valid() && corruptPoster.pixelsBGRA.empty(),
                "corrupt movie rejected fail-closed");
        require(game_lab::presentImage(moviePresenter, corruptPoster.width, corruptPoster.height, corruptPoster.pixelsBGRA) == game_lab::PresentationResult::InvalidPixels &&
                moviePresenter.image.empty(),
                "corrupt movie clears old image in presenter");

        // ----------------------------------------------------
        // Continuous Silent Movie Session (Reusing media_demo::Controller):
        // ----------------------------------------------------
        const size_t audioCallsBeforeMovie = factory.calls;
        FakeImagePresenter sessionPresenter;

        auto session = app.createMovieSession(movieRel);
        require(session != nullptr, "createMovieSession produces non-null session");
        require(session->openMovie(movieRel), "openMovie succeeds for valid synthetic MOV");
        require(session->hasActiveController(), "session owns an active Controller on worker");

        // 1. Play produces first frame with 32x24 tight pixels (unpacked from 133-byte stride)
        auto playDel = session->performSync(media_demo::Action::Play, 0, &sessionPresenter);
        require(playDel.accepted && playDel.presented, "Play delivery accepted and presented");
        require(playDel.snapshot.state == media_demo::State::Playing, "State transitions to Playing");
        require(playDel.snapshot.frame == 0, "Play initial frame is 0");
        require(playDel.pixelsBGRA.size() == 32 * 24, "Tightly packed 32x24 pixels (768 pixels)");
        require(sessionPresenter.image.size() == 32 * 24, "Presenter updated with 32x24 pixels");
        for (const auto p : playDel.pixelsBGRA) {
            require(((p >> 24) & 0xff) == 255, "Alpha must be 255");
        }

        // 2. Advance with HostClock ticks
        // Sleep slightly to let HostClock advance
        usleep(40000); // 40ms
        auto tickDel = session->performSync(media_demo::Action::Tick, 0, &sessionPresenter);
        require(tickDel.accepted, "Tick delivery accepted");
        require(tickDel.snapshot.state == media_demo::State::Playing, "Tick preserves Playing state");

        // 3. Pause, Seek, near EOF, Stop
        auto pauseDel = session->performSync(media_demo::Action::Pause, 0, &sessionPresenter);
        require(pauseDel.accepted && pauseDel.snapshot.state == media_demo::State::Paused, "Pause transitions to Paused");

        auto seekDel = session->performSync(media_demo::Action::Seek, 100, &sessionPresenter);
        require(seekDel.accepted && seekDel.snapshot.frame == 100, "Seek jumps to target frame 100");
        require(seekDel.presented && sessionPresenter.image.size() == 32 * 24, "Seek delivers frame 100 image");

        // Seek near EOF (e.g. frame 238 of 240)
        auto nearEofDel = session->performSync(media_demo::Action::Seek, 238, &sessionPresenter);
        require(nearEofDel.accepted && nearEofDel.snapshot.frame == 238, "Seek jumps near EOF");

        auto stopDel = session->performSync(media_demo::Action::Stop, 0, &sessionPresenter);
        require(stopDel.accepted && stopDel.snapshot.state == media_demo::State::Ready, "Stop transitions to Ready");
        require(stopDel.snapshot.clearImage && sessionPresenter.image.empty(), "Stop clears presenter image");

        // 4. Stale delivery rejection after seek, replace, stop, and close
        // Re-play to have an active state
        session->performSync(media_demo::Action::Play, 0, &sessionPresenter);
        const uint64_t oldRevision = session->revision();
        media_demo::Snapshot lateSnap;
        lateSnap.state = media_demo::State::Playing;
        lateSnap.frame = 50;
        lateSnap.bgra.resize(133 * 24, 0x7f);

        // Advance revision via seek
        session->performSync(media_demo::Action::Seek, 10, &sessionPresenter);
        require(session->revision() > oldRevision, "Seek advances revision");

        game_lab::MovieDelivery lateDelivery;
        require(!game_lab::processMovieDelivery(session->revision(), oldRevision, lateSnap, &sessionPresenter, lateDelivery),
                "Late delivery with old revision is rejected after Seek");
        require(!lateDelivery.accepted, "Late delivery marked not accepted");

        // Reject after stop
        const uint64_t beforeStopRev = session->revision();
        session->performSync(media_demo::Action::Stop, 0, &sessionPresenter);
        require(!game_lab::processMovieDelivery(session->revision(), beforeStopRev, lateSnap, &sessionPresenter, lateDelivery),
                "Late delivery with old revision is rejected after Stop");

        // Reject after close
        const uint64_t beforeCloseRev = session->revision();
        session->close();
        require(session->resourcesZero(), "Resources zero after close");
        require(!game_lab::processMovieDelivery(session->revision(), beforeCloseRev, lateSnap, &sessionPresenter, lateDelivery),
                "Late delivery with old revision is rejected after Close");

        // 5. Double open/close and bad replacement recovery
        require(session->openMovie(movieRel), "First open succeeds");
        require(session->openMovie(movieRel), "Second consecutive open succeeds without leak");
        session->close();
        session->close(); // Double close
        require(session->resourcesZero(), "Double close keeps resources zero");

        // Bad replacement (missing or corrupt)
        require(!session->openMovie("Movies/missing.mov"), "Opening missing file fails");
        require(!session->hasActiveController(), "Bad open leaves no active controller");
        require(session->openMovie(movieRel), "Recovery to valid movie succeeds after bad replacement");
        require(session->hasActiveController(), "Recovered session has active controller");
        session->close();

        // 6. Presenter conversion failure clears old image
        struct FailPresenter final : public game_lab::ImagePresenter {
            std::vector<uint32_t> oldImage{1, 2, 3};
            void clear() override { oldImage.clear(); }
            bool present(uint32_t, uint32_t, const std::vector<uint32_t>&) override {
                return false; // Conversion failure
            }
        } failPresenter;

        game_lab::MovieDelivery failDel;
        media_demo::Snapshot validSnap;
        validSnap.state = media_demo::State::Playing;
        validSnap.bgra.resize(133 * 24, 0xff);
        const bool procOk = game_lab::processMovieDelivery(100, 100, validSnap, &failPresenter, failDel);
        require(procOk && failDel.accepted, "Delivery processed");
        require(!failDel.presented, "Presentation fails on presenter rejection");
        require(failPresenter.oldImage.empty(), "Failed presentation clears old image from presenter");

        // Exercise the real asynchronous request path, not only a fabricated revision.
        require(session->openMovie(movieRel), "queued-delivery session opens");
        session->performSync(media_demo::Action::Play, 0, &sessionPresenter);
        size_t queuedCallbacks = 0;
        session->request(media_demo::Action::Tick, 0, [&](const game_lab::MovieDelivery& d) {
            (void)d; ++queuedCallbacks;
        });
        const auto newFrame = session->performSync(media_demo::Action::Seek, 50, &sessionPresenter);
        const auto newPixels = sessionPresenter.image;
        drainMovieDeliveries();
        require(queuedCallbacks == 0 && sessionPresenter.image == newPixels,
                "queued old request cannot replace post-seek image");
        require(newFrame.snapshot.frame == 50, "new consumer seek uses existing Controller");
        session->close();
        size_t destroyedCallbacks = 0;
        auto disposable = app.createMovieSession(movieRel);
        disposable->request(media_demo::Action::Play, 0, [&](const game_lab::MovieDelivery&) {
            ++destroyedCallbacks;
        });
        disposable.reset();
        drainMovieDeliveries();
        require(destroyedCallbacks == 0, "destroyed owner suppresses queued callback and presentation");

        // 7. Verify zero AudioFactory calls throughout entire movie session execution
        require(factory.calls == audioCallsBeforeMovie,
                "Continuous movie session produces exactly ZERO AudioFactory requests");

        // Shared Map Geometry (mapPointToTile) pure component validation:
        int tx = -77, ty = -88;
        // 1. Normal coordinate mapping
        require(game_lab::mapPointToTile(1.0, 0.5, 2.0, 1.0, 128, tx, ty) && tx == 64 && ty == 64,
                "Normal center point maps to (64, 64)");
        // 2. Extreme wide finite viewWidth (DBL_MAX) with intermediate overflow avoidance
        tx = -77; ty = -88;
        const double maxFinite = std::numeric_limits<double>::max();
        require(game_lab::mapPointToTile(maxFinite / 2.0, 0.5, maxFinite, 1.0, 128, tx, ty) && tx == 64 && ty == 64,
                "Extreme finite viewWidth does not overflow to inf and maps to (64, 64)");
        // 3. NaN rejection preserving outX and outY unmodified
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(std::numeric_limits<double>::quiet_NaN(), 0.5, 2.0, 1.0, 128, tx, ty) && tx == -77 && ty == -88,
                "NaN x rejected and leaves outX/outY unmodified");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(1.0, std::numeric_limits<double>::quiet_NaN(), 2.0, 1.0, 128, tx, ty) && tx == -77 && ty == -88,
                "NaN y rejected and leaves outX/outY unmodified");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(1.0, 0.5, std::numeric_limits<double>::quiet_NaN(), 1.0, 128, tx, ty) && tx == -77 && ty == -88,
                "NaN viewWidth rejected and leaves outX/outY unmodified");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(1.0, 0.5, 2.0, std::numeric_limits<double>::quiet_NaN(), 128, tx, ty) && tx == -77 && ty == -88,
                "NaN viewHeight rejected and leaves outX/outY unmodified");
        // 4. Infinity rejection preserving outX and outY unmodified
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(1.0, 0.5, std::numeric_limits<double>::infinity(), 1.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Infinite width rejected and leaves outX/outY unmodified");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(1.0, 0.5, 2.0, std::numeric_limits<double>::infinity(), 128, tx, ty) && tx == -77 && ty == -88,
                "Infinite height rejected and leaves outX/outY unmodified");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(std::numeric_limits<double>::infinity(), 0.5, 2.0, 1.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Infinite x rejected and leaves outX/outY unmodified");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(1.0, std::numeric_limits<double>::infinity(), 2.0, 1.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Infinite y rejected and leaves outX/outY unmodified");
        // 5. 4 corners & center coordinate mapping
        require(game_lab::mapPointToTile(0.0, 0.0, 560.0, 560.0, 128, tx, ty) && tx == 0 && ty == 127,
                "Bottom-left origin (0,0) maps to tile (0, 127) without dropping y=0");
        require(game_lab::mapPointToTile(0.0, 559.99, 560.0, 560.0, 128, tx, ty) && tx == 0 && ty == 0,
                "Top-left edge maps to tile (0, 0)");
        require(game_lab::mapPointToTile(559.99, 559.99, 560.0, 560.0, 128, tx, ty) && tx == 127 && ty == 0,
                "Top-right edge maps to tile (127, 0)");
        require(game_lab::mapPointToTile(559.99, 0.0, 560.0, 560.0, 128, tx, ty) && tx == 127 && ty == 127,
                "Bottom-right edge maps to tile (127, 127)");
        require(game_lab::mapPointToTile(280.0, 280.0, 560.0, 560.0, 128, tx, ty) && tx == 64 && ty == 64,
                "Center coordinate maps to central tile (64, 64)");
        // 6. 4 side out-of-bounds rejection with unmodified outputs
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(-1.0, 100.0, 560.0, 560.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Negative x rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, -1.0, 560.0, 560.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Negative y rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(560.0, 100.0, 560.0, 560.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Upper bound x rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, 560.0, 560.0, 560.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Upper bound y rejected");
        // 7. Invalid view dimensions rejection with unmodified outputs
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, 100.0, 0.0, 560.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Zero width view rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, 100.0, -10.0, 560.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Negative width view rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, 100.0, 560.0, 0.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Zero height view rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, 100.0, 560.0, -10.0, 128, tx, ty) && tx == -77 && ty == -88,
                "Negative height view rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, 100.0, 560.0, 560.0, 0, tx, ty) && tx == -77 && ty == -88,
                "Zero dim map rejected");
        tx = -77; ty = -88;
        require(!game_lab::mapPointToTile(100.0, 100.0, 560.0, 560.0, -10, tx, ty) && tx == -77 && ty == -88,
                "Negative dim map rejected");

        // Self-authored CitySimulation Engine Verification:
        game_lab::CitySimulation sim;
        require(sim.stats().treasury == 5000 && sim.stats().population == 0 && sim.stats().year == -1046 && sim.stats().month == 1,
                "CitySimulation initializes default governance stats");

        std::string buildMsg;
        // 1. Build road
        require(sim.build(10, 10, game_lab::BuildingType::Road, buildMsg), "Road build succeeded");
        require(sim.stats().treasury == 4995 && sim.stats().totalRoads == 1 && sim.buildingAt(10, 10) == game_lab::BuildingType::Road,
                "Road building cost deducted and placed on grid");
        require(sim.build(9, 10, game_lab::BuildingType::Road, buildMsg), "Auxiliary road build succeeded");
        require(sim.stats().treasury == 4990 && sim.stats().totalRoads == 2, "Auxiliary road accounted");

        // 2. Reject building on occupied tile
        require(!sim.build(10, 10, game_lab::BuildingType::Road, buildMsg), "Cannot build on already occupied tile");

        // 3. Build house adjacent to road
        require(sim.build(10, 11, game_lab::BuildingType::House, buildMsg), "House build succeeded");
        require(sim.stats().treasury == 4975 && sim.stats().housingCapacity == 8,
                "House building adjacent to road provides shelter capacity for 8 citizens");

        // 4. Build farm
        require(sim.build(10, 12, game_lab::BuildingType::Farm, buildMsg), "Farm build succeeded");
        require(sim.stats().treasury == 4935 && sim.stats().totalFarms == 1, "Farm building cost accounted");

        // 5. Build well near house -> upgrades house to level 2!
        require(sim.build(11, 11, game_lab::BuildingType::Well, buildMsg), "Well build succeeded");
        require(sim.stats().treasury == 4925 && sim.stats().housingCapacity == 20,
                "Well provides clean water and upgrades adjacent house from level 1 to level 2 (capacity 20)");

        // 6. Build temple
        require(sim.build(12, 12, game_lab::BuildingType::Temple, buildMsg), "Temple build succeeded");
        require(sim.stats().treasury == 4875 && sim.stats().totalTemples == 1, "Temple cost accounted");

        // 7. Demolish auxiliary road with clearance labor cost
        require(sim.demolish(9, 10, buildMsg), "Demolish road succeeded");
        require(sim.stats().treasury == 4873 && sim.buildingAt(9, 10) == game_lab::BuildingType::None,
                "Demolish cleared building and deducted clearance labor fee");
        require(!sim.demolish(9, 10, buildMsg), "Cannot demolish empty tile");

        // 8. Reject out of bounds
        require(!sim.build(128, 128, game_lab::BuildingType::House, buildMsg), "Out of bounds build rejected");
        require(!sim.demolish(128, 128, buildMsg), "Out of bounds demolish rejected");

        // 9. Monthly tick advance & economic cycle
        const int prevFood = sim.stats().foodSupply;
        const int prevPop = sim.stats().population;
        sim.advanceMonth();
        require(sim.stats().month == 2 && sim.stats().year == -1046, "Month advances on simulation step");
        require(sim.stats().foodSupply > prevFood, "Farm generates grain in harvest cycle");
        require(sim.stats().population >= prevPop, "Population grows towards housing capacity");

        // 10. Annual calendar turnover
        for (int m = 0; m < 11; ++m) sim.advanceMonth();
        require(sim.stats().month == 1 && sim.stats().year == -1045, "Year advances correctly across 12 months");

        // 11. Pixel grid rendering
        const auto gridPixels = sim.renderGridPixels(11, 11);
        require(gridPixels.size() == 128 * 128, "CitySimulation grid renders full 128x128 BGRA buffer");
        presentation(128, 128, gridPixels);

        // 12. Load from CityMap
        sim.loadFromMap(map);
        require(sim.stats().totalRoads == 0 && sim.stats().treasury == 5000, "Loading city map resets civil layout onto new terrain");

        // 13. BuildingType validation & failure contract
        const size_t eventsBeforeNone = sim.events().size();
        const int treasuryBeforeNone = sim.stats().treasury;
        require(!sim.canBuild(15, 15, game_lab::BuildingType::None), "canBuild rejects None building type");
        require(!sim.build(15, 15, game_lab::BuildingType::None, buildMsg), "build rejects None building type");
        require(sim.events().size() == eventsBeforeNone && sim.stats().treasury == treasuryBeforeNone,
                "Failed None build produces no events and modifies no treasury state");
        require(!sim.canBuild(15, 15, static_cast<game_lab::BuildingType>(255)), "canBuild rejects invalid 255 type");
        require(!sim.build(15, 15, static_cast<game_lab::BuildingType>(255), buildMsg), "build rejects invalid 255 type");
        require(sim.buildingAt(15, 15) == game_lab::BuildingType::None, "Grid tile untouched by invalid building type");

        // 14. Tick phase restoration on reset and loadFromMap
        sim.setSpeed(1);
        for (int t = 0; t < 9; ++t) sim.tick();
        require(sim.stats().month == 1 && sim.monthAccumulator() == 9, "9 ticks at 1x advance accumulator to 9 without advancing month");
        sim.reset();
        require(sim.stats().month == 1 && sim.monthAccumulator() == 0, "reset restores tick accumulator phase to 0");
        sim.tick();
        require(sim.stats().month == 1 && sim.monthAccumulator() == 1, "Single tick after reset does not advance month");
        for (int t = 0; t < 9; ++t) sim.tick();
        require(sim.stats().month == 2 && sim.monthAccumulator() == 0, "10th tick after reset completes first month cycle");

        // 15. Speed remainder preservation across multi-speed simulation
        sim.reset();
        sim.setSpeed(1);
        for (int t = 0; t < 30; ++t) sim.tick();
        require(sim.stats().month == 4, "30 ticks at 1x advance exactly 3 months (from month 1 to 4)");

        sim.reset();
        sim.setSpeed(3);
        for (int t = 0; t < 30; ++t) sim.tick();
        require(sim.stats().month == 10, "30 ticks at 3x advance exactly 9 months (from month 1 to 10, exactly 3x speed)");

        // 16. Multi-tier house progression (Level 1..4) with Market and Temple
        game_lab::CitySimulation city;
        std::string outMsg;
        require(city.build(20, 20, game_lab::BuildingType::Road, outMsg), "Road placed");
        require(city.build(20, 21, game_lab::BuildingType::House, outMsg), "House placed on road");
        require(city.stats().housingCapacity == 8, "House starts at Level 1 (8 capacity)");

        require(city.build(21, 21, game_lab::BuildingType::Well, outMsg), "Well placed");
        require(city.stats().housingCapacity == 20, "Well upgrades house to Level 2 (20 capacity)");

        require(city.build(22, 21, game_lab::BuildingType::Market, outMsg), "Market placed");
        require(city.stats().housingCapacity == 32, "Market food supply upgrades house to Level 3 (32 capacity)");

        require(city.build(23, 21, game_lab::BuildingType::Temple, outMsg), "Temple placed");
        require(city.stats().housingCapacity == 48, "Temple blessing upgrades house to Level 4 (48 capacity)");

        // 17. Labor and employment tracking
        require(city.stats().neededWorkers == 10, "Needed workers accurately calculated across industries");
        require(city.stats().availableWorkers == 0, "No citizens yet, available workers = 0");
        require(city.stats().employmentPercent == 0, "Employment is 0% when no workers available");

        // Advance months to immigrate citizens into available 48 capacity
        city.advanceMonth();
        city.advanceMonth();
        require(city.stats().population >= 12, "Immigration flows into higher capacity housing");
        require(city.stats().availableWorkers >= 6, "Working-age labor force scales with population");
        require(city.stats().employmentPercent > 50, "Employment percent reflects available vs needed workers");

        // Build Watchtower
        require(city.build(20, 19, game_lab::BuildingType::Watchtower, outMsg), "Watchtower placed");
        require(city.stats().totalWatchtowers == 1 && city.stats().neededWorkers == 12,
                "Watchtower adds 2 required guards to workforce demand");

        // 18. State Snapshot Serialization Round-Trip
        const auto snapshot = city.saveSnapshot();
        require(snapshot.size() == 65580, "Snapshot serialized to exact expected byte size");
        require(snapshot[0] == 'E' && snapshot[1] == 'M' && snapshot[2] == 'P' && snapshot[3] == '1',
                "Snapshot header contains EMP1 magic");

        game_lab::CitySimulation restoredCity;
        require(restoredCity.stats().totalHouses == 0 && restoredCity.stats().treasury == 5000,
                "Fresh city starts with default stats before loading snapshot");

        require(restoredCity.loadSnapshot(snapshot), "Snapshot loads successfully");
        require(restoredCity.stats().treasury == city.stats().treasury, "Treasury matches original snapshot");
        require(restoredCity.stats().population == city.stats().population, "Population matches original snapshot");
        require(restoredCity.stats().housingCapacity == city.stats().housingCapacity, "Housing capacity matches snapshot");
        require(restoredCity.stats().totalHouses == 1 && restoredCity.stats().totalWells == 1 &&
                restoredCity.stats().totalMarkets == 1 && restoredCity.stats().totalTemples == 1 &&
                restoredCity.stats().totalWatchtowers == 1,
                "All civil buildings restored from snapshot");
        require(restoredCity.stats().employmentPercent == city.stats().employmentPercent,
                "Employment percent matches original simulation");

        // Test truncated / corrupted snapshot rejection
        std::vector<uint8_t> corrupt = snapshot;
        corrupt[0] = 'X';
        require(!restoredCity.loadSnapshot(corrupt), "Corrupted magic rejected");
        corrupt.resize(100);
        require(!restoredCity.loadSnapshot(corrupt), "Truncated payload rejected");
    }
    for (const auto& record : factory.records) require(record->destroyed && !record->playing,"all fake player ownership retired");
}
}
int main() {
    @autoreleasepool {
        try {
            integration();
            std::printf("{\"passed\":true,\"assertions\":%zu,\"ui\":false,\"audio_devices\":false,\"backend\":\"fake\"}\n",checks);
            return 0;
        } catch (const std::exception& error) {
            std::fprintf(stderr,"App integration failure: %s\n",error.what()); return 1;
        }
    }
}
