#include "app_media.hpp"
#include <stdexcept>
#include "asset_bytes.hpp"
#include "movie_session.hpp"
#include "../stream_lab/movie_io.hpp"
#include <cstring>
#include <algorithm>

namespace game_lab {
void AppMedia::setStatus(PlaybackState state, const char* message) {
    status_ = {state, message};
    if (statusChanged_) {
        // Both copies survive a callback that changes status or unregisters itself.
        const auto callback = statusChanged_;
        const PlaybackStatus snapshot = status_;
        callback(snapshot);
    }
}
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
std::vector<std::string> AppMedia::originalMapFiles() const {
    std::vector<std::string> files;std::error_code error;
    for(std::filesystem::recursive_directory_iterator it(root_.configuredDirectory(),error),end;!error && it!=end;it.increment(error)) {
        if(!it->is_regular_file(error))continue;
        auto ext=it->path().extension().string();for(char& c:ext)if(c>='A' && c<='Z')c=char(c-'A'+'a');
        if(ext!=".map")continue;
        auto relative=it->path().lexically_relative(root_.configuredDirectory()).generic_string();
        if(!resourcePath(relative).empty())files.push_back(relative);
    }
    std::sort(files.begin(),files.end());return files;
}
OriginalMapResult AppMedia::originalMap(const std::string& relative) const {
    const auto path=resourcePath(relative);return path.empty()?OriginalMapResult{}:loadOriginalMapTerrain(path);
}
SG3Archive AppMedia::sg3(const std::string& relative) const {
    const auto path = resourcePath(relative);
    return path.empty() ? SG3Archive{} : loadSG3(path);
}
std::vector<std::string> AppMedia::sg3Packages() const {
    std::vector<std::string> result;
    std::error_code error;
    auto directory=std::filesystem::path(root_.configuredDirectory())/"DATA";
    for(std::filesystem::directory_iterator it(directory,error),end;!error && it!=end;it.increment(error)) {
        auto ext=it->path().extension().string();
        for(char& c:ext)if(c>='A' && c<='Z')c=char(c-'A'+'a');
        if(ext!=".sg3")continue;
        auto relative=(std::filesystem::path("DATA")/it->path().filename()).generic_string();
        if(!resourcePath(relative).empty())result.push_back(relative);
    }
    std::sort(result.begin(),result.end());return result;
}
SG3DecodeResult AppMedia::sprite(const SG3Archive& archive,size_t index,const std::string& companionRelative) const {
    // The internal file may be absent while this record has a valid external
    // dependency. A validated SG path fixes its parent inside the selected root.
    auto sgPath=std::filesystem::path(companionRelative);sgPath.replace_extension(".sg3");
    auto path=resourcePath(sgPath.generic_string());
    if(path.empty())return {};
    auto internal=std::filesystem::path(path);internal.replace_extension(".555");
    return decodeSG3Sprite(archive,index,internal.string());
}
DecodedImage AppMedia::sprite(const SG3Sprite& spriteRecord, const std::string& companionRelative) const {
    const auto path = resourcePath(companionRelative);
    return path.empty() ? DecodedImage{} : extractSprite(spriteRecord, path);
}
TextLoadResult AppMedia::textWithStatus(const std::string& relative) const {
    const auto path = resourcePath(relative);
    return path.empty() ? TextLoadResult{} : loadEmperorTextResult(path);
}
std::vector<std::string> AppMedia::text(const std::string& relative) const {
    return textWithStatus(relative).lines;
}
DecodedImage readMoviePosterBytes(const std::string& path, std::shared_ptr<stream_lab::OpenControl> control) {
    if (path.empty()) {
        throw std::runtime_error("missing poster or fixed first-frame layout mismatch");
    }
        stream_lab::MovieReader reader;
        reader.open(path, 0, control);
        video_lab::Frame frame;
        if (!reader.nextFrame(frame,control)) {
            throw std::runtime_error("missing poster or fixed first-frame layout mismatch");
        }
        if (frame.pts != 0) {
            throw std::runtime_error("missing poster or fixed first-frame layout mismatch");
        }
        const auto meta = reader.metadata();
        if (!meta.width || !meta.height || meta.stride < size_t(meta.width)*4 || meta.stride > 8u*1024u*1024u/meta.height) {
            throw std::runtime_error("missing poster or fixed first-frame layout mismatch");
        }
        if (frame.bgra.size() != meta.stride * meta.height) {
            throw std::runtime_error("missing poster or fixed first-frame layout mismatch");
        }
        DecodedImage image;
        image.width = meta.width;
        image.height = meta.height;
        image.pixelsBGRA.resize(size_t(image.width) * size_t(image.height));
        for (uint32_t y = 0; y < image.height; ++y) {
            const uint8_t* rowSrc = frame.bgra.data() + size_t(y) * meta.stride;
            uint8_t* rowDst = reinterpret_cast<uint8_t*>(image.pixelsBGRA.data()) + size_t(y) * size_t(image.width) * 4;
            std::memcpy(rowDst, rowSrc, size_t(image.width) * 4);
        }
        for (auto& pixel : image.pixelsBGRA) {
            pixel = (pixel & 0x00ffffffu) | 0xff000000u;
        }
        reader.close();
        return image;
}
DecodedImage AppMedia::moviePoster(const std::string& relative) const {
    try {return readMoviePosterBytes(resourcePath(relative));}
    catch (const std::exception&) {return {};}
}
std::unique_ptr<MovieSession> AppMedia::createMovieSession(const std::string& relative) const {
    auto session = std::make_unique<MovieSession>(root_);
    if (!relative.empty()) {
        session->openMovie(relative);
    }
    return session;
}
bool AppMedia::readAudio(const std::string& relative, bool wav, std::vector<uint8_t>& bytes) {
    const auto path = resourcePath(relative);
    if (path.empty() || !detail::readFile(path, wav ? kMaxWavBytes : kMaxAssetBytes, bytes) || bytes.empty()) {
        setStatus(PlaybackState::ResourceFailure, "音频资源不可读取或超出大小限制");
        return false;
    }
    if (wav && !decodeWAV(bytes).valid()) {
        setStatus(PlaybackState::DecodeFailure, "音效不是受支持的完整 PCM16 WAV");
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
        setStatus(PlaybackState::Muted, "请取消静音并设置音量，然后点击播放");
        return;
    }
    std::vector<uint8_t> bytes;
    if (!readAudio(selectedMusic_, false, bytes)) return;
    auto player = factory_.create(bytes);
    if (!player) { setStatus(PlaybackState::DecodeFailure, "无法解码音乐资源"); return; }
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
        setStatus(PlaybackState::Paused, "音乐已暂停");
        return;
    }
    if (!policy_.permitsPlayback()) {
        setStatus(PlaybackState::Muted, "请取消静音并设置音量，然后点击播放");
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
        setStatus(PlaybackState::Muted, "请取消静音并设置音量，再播放音效");
        return;
    }
    std::vector<uint8_t> bytes;
    if (!readAudio(relative, true, bytes)) return;
    auto player = factory_.create(bytes);
    if (!player) { setStatus(PlaybackState::DecodeFailure, "无法解码音效资源"); return; }
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
            setStatus(PlaybackState::Playing, playingMessage);
        } else {
            currentTicket = 0;
            retirePlayer(player);
            setStatus(PlaybackState::StartFailure, failureMessage);
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
        setStatus(PlaybackState::Muted, "音乐和音效已静音；设置音量后点击播放");
    }
}
void AppMedia::setMuted(bool muted) {
    policy_.setMuted(muted);
    applyPolicy();
    if (policy_.permitsPlayback() && !musicPlaying())
        setStatus(PlaybackState::Paused, "已取消静音；点击播放以启动音频");
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
    setStatus(PlaybackState::Stopped, "音频已停止");
}
void AppMedia::onPlaybackEvent(uint64_t ticket, PlaybackEvent event) {
    if (ticket && ticket == sfxTicket_) {
        sfxTicket_ = 0;
        retirePlayer(sfx_);
        if (event == PlaybackEvent::Finished) {
            if (musicPlaying()) {
                setStatus(PlaybackState::Playing, "音乐正在播放");
            } else {
                setStatus(PlaybackState::Stopped, "音效播放完成");
            }
        } else {
            setStatus(PlaybackState::DecodeFailure, "音效播放过程中解码失败");
        }
    } else if (ticket && ticket == musicTicket_) {
        musicTicket_ = 0;
        retirePlayer(music_);
        if (event == PlaybackEvent::Finished) {
            if (sfx_ && sfx_->playing()) {
                setStatus(PlaybackState::Playing, "音效正在播放");
            } else {
                setStatus(PlaybackState::Stopped, "音乐播放完成");
            }
        } else {
            setStatus(PlaybackState::DecodeFailure, "音乐播放过程中解码失败");
        }
    }
}
} // namespace game_lab
