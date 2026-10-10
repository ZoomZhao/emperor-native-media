#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include "app_media.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <filesystem>
#include <fstream>
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
    game_lab::PlaybackListener* listener = nullptr;
    uint64_t ticket = 0;
};
class FakePlayback final : public game_lab::AudioPlayback {
public:
    explicit FakePlayback(std::shared_ptr<Record> record) : record_(std::move(record)) {}
    ~FakePlayback() override { record_->destroyed=true; }
    void setListener(game_lab::PlaybackListener* listener, uint64_t ticket) override {
        record_->listener = listener;
        record_->ticket = ticket;
    }
    void setGain(double n) override { record_->gain=n; }
    void setLoops(int n) override { record_->loops=n; }
    bool play() override { ++record_->plays; record_->playing=record_->allowPlay; return record_->playing; }
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
    std::unique_ptr<game_lab::AudioPlayback> create(const std::vector<uint8_t>& bytes) override {
        ++calls;
        self_fixture::bytes("factory-actual-data",bytes);
        if (failCreate) { failCreate=false; return {}; }
        auto r=std::make_shared<Record>(); r->bytes=bytes; r->allowPlay=!failPlay; failPlay=false;
        records.push_back(r);
        return std::make_unique<FakePlayback>(r);
    }
};
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
        auto broken=image; broken.pop_back(); write(files/"broken.jpg",broken);
        require(!app.jpeg("broken.jpg").valid(),"App JPEG truncation regression");
        require(!app.jpeg("missing.jpg").valid() && !app.jpeg("../self.jpg").valid(),"App JPEG missing/traversal invalid");
        write(files/"self.555",{0,0,255,127});
        auto raw=app.raw555("self.555",2,1);
        require(raw.valid() && raw.pixelsBGRA==std::vector<uint32_t>({0xff000000,0xffffffff}),"App raw loading uses actual surface conversion");
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

        // AppMedia CityMap loading through ResourceRoot:
        write(files/"Cities/test.map", syntheticMap());
        auto map = app.cityMap("Cities/test.map");
        require(map.valid() && map.width == 128 && map.height == 128 && map.tiles.size() == 128 * 128,
                "AppMedia cityMap loads valid synthetic 128x128 map");
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
        require(!app.sg3("../test.sg3").sprites.size() && !app.sprite(archive.sprites[0], "../test.555").valid(),
                "AppMedia sg3/sprite traversal fail-closed");

        // AppMedia Game Text loading through ResourceRoot:
        write(files/"EmperorText.txt", syntheticText());
        auto lines = app.text("EmperorText.txt");
        require(lines.size() == 2 && lines[0] == "Line 1 Chinese Text",
                "AppMedia text loads trimmed lines from EmperorText.txt");
        require(app.text("../EmperorText.txt").empty() && app.text("missing.txt").empty(),
                "AppMedia text traversal/missing fail-closed");
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
