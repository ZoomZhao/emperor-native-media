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
    if (music_) music_->stop();
    music_.reset();
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
    if (!player->play()) {
        player->stop();
        status_ = {PlaybackState::StartFailure, "音乐播放未启动"};
        return;
    }
    music_ = std::move(player);
    status_ = {PlaybackState::Playing, "音乐正在播放"};
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
    if (!music_->play()) {
        music_->stop();
        music_.reset();
        status_ = {PlaybackState::StartFailure, "音乐播放未启动"};
        return;
    }
    status_ = {PlaybackState::Playing, "音乐正在播放"};
}
void AppMedia::playSfx(const std::string& relative) {
    if (sfx_) sfx_->stop();
    sfx_.reset(); // Failed replacement must not leave the previous sound playing.
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
    if (!player->play()) {
        player->stop();
        status_ = {PlaybackState::StartFailure, "音效播放未启动"};
        return;
    }
    sfx_ = std::move(player);
    status_ = {PlaybackState::Playing, "音效正在播放"};
}
void AppMedia::applyPolicy() {
    if (music_) music_->setGain(policy_.gain());
    if (sfx_) sfx_->setGain(policy_.gain());
    if (!policy_.permitsPlayback()) {
        if (music_) music_->pause();
        if (sfx_) sfx_->stop();
        sfx_.reset();
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
    if (music_) music_->stop();
    if (sfx_) sfx_->stop();
    music_.reset();
    sfx_.reset();
    status_ = {PlaybackState::Stopped, "音频已停止"};
}
} // namespace game_lab
