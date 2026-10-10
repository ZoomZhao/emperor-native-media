#import <AVFoundation/AVFoundation.h>
#include "av_audio_backend.hpp"
namespace game_lab {
namespace {
class AVPlayback final : public AudioPlayback {
public:
    explicit AVPlayback(AVAudioPlayer* player) : player_(player) {}
    void setGain(double gain) override { player_.volume = gain; }
    void setLoops(int loops) override { player_.numberOfLoops = loops; }
    bool play() override { return [player_ play]; }
    void pause() override { [player_ pause]; }
    void stop() override { [player_ stop]; }
    bool playing() const override { return player_.isPlaying; }
    double duration() const override { return player_.duration; }
private:
    AVAudioPlayer* __strong player_;
};
}
std::unique_ptr<AudioPlayback> AVAudioFactory::create(const std::vector<uint8_t>& bytes) {
    if (bytes.empty() || bytes.size() > kMaxAssetBytes) return {};
    NSData* data = [NSData dataWithBytes:bytes.data() length:bytes.size()];
    NSError* error = nil;
    AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithData:data error:&error];
    if (!player) return {};
    return std::make_unique<AVPlayback>(player);
}
}
