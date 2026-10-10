#pragma once
#include "../app_media.hpp"
namespace game_lab {
class AVAudioFactory final : public AudioFactory {
public:
    std::unique_ptr<AudioPlayback> create(const std::vector<uint8_t>& bytes) override;
};
}
