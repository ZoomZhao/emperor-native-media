#include "app_media.hpp"
#include "asset_bytes.hpp"

namespace game_lab {
DecodedImage AppMedia::jpeg(const std::string& relative) const {
    const auto path = resourcePath(relative);
    return path.empty() ? DecodedImage{} : loadJPEG(path);
}
Raw555Image AppMedia::raw555(const std::string& relative, uint32_t width, uint32_t height) const {
    const auto path = resourcePath(relative);
    return path.empty() ? Raw555Image{} : loadRaw555(path, width, height);
}
CityMap AppMedia::cityMap(const std::string& relative) const {
    const auto path = resourcePath(relative);
    return path.empty() ? CityMap{} : loadCityMap(path);
}
SG3Archive AppMedia::sg3(const std::string& relative) const {
    const auto path = resourcePath(relative);
    return path.empty() ? SG3Archive{} : loadSG3(path);
}
DecodedImage AppMedia::sprite(const SG3Sprite& spriteRecord, const std::string& companionRelative) const {
    const auto path = resourcePath(companionRelative);
    return path.empty() ? DecodedImage{} : extractSprite(spriteRecord, path);
}
std::vector<std::string> AppMedia::text(const std::string& relative) const {
    const auto path = resourcePath(relative);
    return path.empty() ? std::vector<std::string>{} : loadEmperorText(path);
}
bool AppMedia::readAudio(const std::string& relative, bool wav, std::vector<uint8_t>& bytes) {
    const auto path = resourcePath(relative);
    if (path.empty() || !detail::readFile(path, wav ? kMaxWavBytes : kMaxAssetBytes, bytes) || bytes.empty()) {
        status_ = {PlaybackState::ResourceFailure, "音频资源不可读取或超出大小限制"};
        return false;
    }
    if (wav && !decodeWAV(bytes).valid()) {
        status_ = {PlaybackState::DecodeFailure, "音效不是受支持的完整 PCM16 WAV"};
        return false;
    }
    return true;
}
void AppMedia::selectMusic(const std::string& relative) {
    musicTicket_ = 0;
    retirePlayer(music_);
    selectedMusic_ = relative;
    startMusic();
}
void AppMedia::startMusic() {
    if (!policy_.permitsPlayback()) {
        status_ = {PlaybackState::Muted, "请取消静音并设置音量，然后点击播放"};
        return;
    }
    std::vector<uint8_t> bytes;
    if (!readAudio(selectedMusic_, false, bytes)) return;
    auto player = factory_.create(bytes);
    if (!player) { status_ = {PlaybackState::DecodeFailure, "无法解码音乐资源"}; return; }
    player->setLoops(-1);
    player->setGain(policy_.gain());
    const uint64_t ticket = ++nextTicket_;
    musicTicket_ = ticket;
    player->setListener(this, ticket);
    music_ = std::move(player);
    playCurrent(music_, musicTicket_, "音乐正在播放", "音乐播放未启动");
}
void AppMedia::toggleMusic() {
    if (musicPlaying()) {
        music_->pause();
        status_ = {PlaybackState::Paused, "音乐已暂停"};
        return;
    }
    if (!policy_.permitsPlayback()) {
        status_ = {PlaybackState::Muted, "请取消静音并设置音量，然后点击播放"};
        return;
    }
    if (!music_) { startMusic(); return; }
    music_->setGain(policy_.gain());
    playCurrent(music_, musicTicket_, "音乐正在播放", "音乐播放未启动");
}
void AppMedia::playSfx(const std::string& relative) {
    sfxTicket_ = 0;
    retirePlayer(sfx_); // Failed replacement must not leave the previous sound playing.
    if (!policy_.permitsPlayback()) {
        status_ = {PlaybackState::Muted, "请取消静音并设置音量，再播放音效"};
        return;
    }
    std::vector<uint8_t> bytes;
    if (!readAudio(relative, true, bytes)) return;
    auto player = factory_.create(bytes);
    if (!player) { status_ = {PlaybackState::DecodeFailure, "无法解码音效资源"}; return; }
    player->setLoops(0);
    player->setGain(policy_.gain());
    const uint64_t ticket = ++nextTicket_;
    sfxTicket_ = ticket;
    player->setListener(this, ticket);
    sfx_ = std::move(player);
    playCurrent(sfx_, sfxTicket_, "音效正在播放", "音效播放未启动");
}
void AppMedia::retirePlayer(std::unique_ptr<AudioPlayback>& player) {
    auto retiring = std::move(player);
    if (!retiring) return;
    retiring->stop();
    // A synchronous event or a reentrant control operation may retire the
    // backend while its play() is still on the stack.
    if (inPlayDepth_ > 0) retiredPlayers_.push_back(std::move(retiring));
}
void AppMedia::collectRetiredPlayers() {
    if (inPlayDepth_ == 0) retiredPlayers_.clear();
}
void AppMedia::playCurrent(std::unique_ptr<AudioPlayback>& player, uint64_t& currentTicket,
                           const char* playingMessage, const char* failureMessage) {
    const uint64_t ticket = currentTicket;
    ++inPlayDepth_;
    const bool started = player->play();
    --inPlayDepth_;
    // A terminal callback, stop, or replacement owns the resulting state.
    // The return value can only update the playback that invoked play().
    if (currentTicket == ticket) {
        if (started) {
            status_ = {PlaybackState::Playing, playingMessage};
        } else {
            currentTicket = 0;
            retirePlayer(player);
            status_ = {PlaybackState::StartFailure, failureMessage};
        }
    }
    collectRetiredPlayers();
}
void AppMedia::applyPolicy() {
    if (music_) music_->setGain(policy_.gain());
    if (sfx_) sfx_->setGain(policy_.gain());
    if (!policy_.permitsPlayback()) {
        sfxTicket_ = 0;
        if (music_) music_->pause();
        retirePlayer(sfx_);
        status_ = {PlaybackState::Muted, "音乐和音效已静音；设置音量后点击播放"};
    }
}
void AppMedia::setMuted(bool muted) {
    policy_.setMuted(muted);
    applyPolicy();
    if (policy_.permitsPlayback() && !musicPlaying())
        status_ = {PlaybackState::Paused, "已取消静音；点击播放以启动音频"};
}
void AppMedia::setVolume(double volume) {
    policy_.setVolume(volume);
    applyPolicy();
}
void AppMedia::stopAll() {
    musicTicket_ = 0;
    sfxTicket_ = 0;
    retirePlayer(music_);
    retirePlayer(sfx_);
    collectRetiredPlayers();
    status_ = {PlaybackState::Stopped, "音频已停止"};
}
void AppMedia::onPlaybackEvent(uint64_t ticket, PlaybackEvent event) {
    if (ticket && ticket == sfxTicket_) {
        sfxTicket_ = 0;
        retirePlayer(sfx_);
        if (event == PlaybackEvent::Finished) {
            if (musicPlaying()) {
                status_ = {PlaybackState::Playing, "音乐正在播放"};
            } else {
                status_ = {PlaybackState::Stopped, "音效播放完成"};
            }
        } else {
            status_ = {PlaybackState::DecodeFailure, "音效播放过程中解码失败"};
        }
    } else if (ticket && ticket == musicTicket_) {
        musicTicket_ = 0;
        retirePlayer(music_);
        if (event == PlaybackEvent::Finished) {
            if (sfx_ && sfx_->playing()) {
                status_ = {PlaybackState::Playing, "音效正在播放"};
            } else {
                status_ = {PlaybackState::Stopped, "音乐播放完成"};
            }
        } else {
            status_ = {PlaybackState::DecodeFailure, "音乐播放过程中解码失败"};
        }
    }
}
} // namespace game_lab
