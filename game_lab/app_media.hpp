#pragma once
#include "audio_policy.hpp"
#include "emperor_assets.hpp"
#include "resource_root.hpp"
#include <memory>

namespace game_lab {
// AppKit-independent control path shared by the real delegate and integration tests.
class AudioPlayback {
public:
    virtual ~AudioPlayback() = default;
    virtual void setGain(double gain) = 0;
    virtual void setLoops(int loops) = 0;
    virtual bool play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual bool playing() const = 0;
    virtual double duration() const = 0;
};
class AudioFactory {
public:
    virtual ~AudioFactory() = default;
    virtual std::unique_ptr<AudioPlayback> create(const std::vector<uint8_t>& bytes) = 0;
};
enum class PlaybackState { Muted, Paused, Playing, ResourceFailure, DecodeFailure, StartFailure, Stopped };
struct PlaybackStatus {
    PlaybackState state = PlaybackState::Muted;
    std::string message = "默认静音；取消静音后点击播放";
};
class AppMedia {
public:
    AppMedia(ResourceRoot root, AudioFactory& factory) : root_(std::move(root)), factory_(factory) {}
    ~AppMedia() { stopAll(); }
    AppMedia(const AppMedia&) = delete;
    AppMedia& operator=(const AppMedia&) = delete;
    std::string resourcePath(const std::string& relative) const { return root_.resolve(relative); }
    DecodedImage jpeg(const std::string& relative) const;
    Raw555Image raw555(const std::string& relative, uint32_t width, uint32_t height) const;
    void selectMusic(const std::string& relative);
    void toggleMusic();
    void playSfx(const std::string& relative);
    void setMuted(bool muted);
    void setVolume(double volume);
    void stopAll();
    bool muted() const { return policy_.muted(); }
    bool musicPlaying() const { return music_ && music_->playing(); }
    const PlaybackStatus& status() const { return status_; }
private:
    bool readAudio(const std::string& relative, bool wav, std::vector<uint8_t>& bytes);
    void startMusic();
    void applyPolicy();
    ResourceRoot root_;
    AudioFactory& factory_;
    AudioPolicy policy_;
    std::string selectedMusic_ = "Audio/Music/MENU1-San_Xuan.mp3";
    std::unique_ptr<AudioPlayback> music_, sfx_;
    PlaybackStatus status_;
};
} // namespace game_lab
