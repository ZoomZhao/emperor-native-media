#pragma once
#include <algorithm>
#include <cmath>

namespace game_lab {
// Shared by music and SFX. Changing this policy never starts playback.
class AudioPolicy {
public:
    bool muted() const { return muted_; }
    void setMuted(bool value) { muted_ = value; }
    void setVolume(double value) { volume_ = std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0; }
    double gain() const { return muted_ ? 0.0 : volume_; }
    bool permitsPlayback() const { return gain() > 0.0; }
private:
    bool muted_ = true;
    double volume_ = 0.8;
};
} // namespace game_lab
