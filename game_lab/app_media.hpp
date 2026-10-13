#pragma once
#include "audio_policy.hpp"
#include "emperor_assets.hpp"
#include "resource_root.hpp"
#include <memory>
#include <functional>

namespace stream_lab { struct OpenControl; }

namespace game_lab {
// Worker-only helper preserves the Reader reason; legacy facade returns invalid.
DecodedImage readMoviePosterBytes(const std::string&, std::shared_ptr<stream_lab::OpenControl> = {});
class MovieSession;
enum class PlaybackEvent { Finished, DecodeError };

class PlaybackListener {
public:
    virtual ~PlaybackListener() = default;
    virtual void onPlaybackEvent(uint64_t ticket, PlaybackEvent event) = 0;
};

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
    virtual void setListener(PlaybackListener* listener, uint64_t ticket) {
        (void)listener; (void)ticket;
    }
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
class AppMedia final : public PlaybackListener {
public:
    AppMedia(ResourceRoot root, AudioFactory& factory) : root_(std::move(root)), factory_(factory) {}
    ~AppMedia() override { setStatusChangedCallback({}); stopAll(); }
    AppMedia(const AppMedia&) = delete;
    AppMedia& operator=(const AppMedia&) = delete;
    std::string resourcePath(const std::string& relative) const { return root_.resolve(relative); }
    DecodedImage jpeg(const std::string& relative) const;
    Raw555Image raw555(const std::string& relative, uint32_t width, uint32_t height) const;
    CityMap cityMap(const std::string& relative) const;
    SG3Archive sg3(const std::string& relative) const;
    std::vector<std::string> sg3Packages() const;
    SG3DecodeResult sprite(const SG3Archive&, size_t index, const std::string& companionRelative) const;
    DecodedImage sprite(const SG3Sprite& sprite, const std::string& companionRelative) const;
    TextLoadResult textWithStatus(const std::string& relative) const;
    std::vector<std::string> text(const std::string& relative) const;
    DecodedImage moviePoster(const std::string& relative) const;
    std::unique_ptr<MovieSession> createMovieSession(const std::string& relative = "") const;
    void selectMusic(const std::string& relative);
    void toggleMusic();
    void playSfx(const std::string& relative);
    void setMuted(bool muted);
    void setVolume(double volume);
    void stopAll();
    bool muted() const { return policy_.muted(); }
    bool musicPlaying() const { return music_ && music_->playing(); }
    const PlaybackStatus& status() const { return status_; }
    // Controls and events use one serialized context. The callback runs inline;
    // replacing/clearing it takes effect for subsequent status notifications.
    void setStatusChangedCallback(std::function<void(const PlaybackStatus&)> callback) {
        statusChanged_ = std::move(callback);
    }
    void onPlaybackEvent(uint64_t ticket, PlaybackEvent event) override;
private:
    bool readAudio(const std::string& relative, bool wav, std::vector<uint8_t>& bytes);
    void startMusic();
    void applyPolicy();
    void setStatus(PlaybackState state, const char* message);
    void retirePlayer(std::unique_ptr<AudioPlayback>& player);
    void collectRetiredPlayers();
    void playCurrent(std::unique_ptr<AudioPlayback>& player, uint64_t& currentTicket,
                     const char* playingMessage, const char* failureMessage);
    ResourceRoot root_;
    AudioFactory& factory_;
    AudioPolicy policy_;
    std::string selectedMusic_ = "Audio/Music/MENU1-San_Xuan.mp3";
    std::unique_ptr<AudioPlayback> music_, sfx_;
    uint64_t nextTicket_ = 0;
    uint64_t musicTicket_ = 0;
    uint64_t sfxTicket_ = 0;
    int inPlayDepth_ = 0;
    std::vector<std::unique_ptr<AudioPlayback>> retiredPlayers_;
    PlaybackStatus status_;
    std::function<void(const PlaybackStatus&)> statusChanged_;
};
} // namespace game_lab
