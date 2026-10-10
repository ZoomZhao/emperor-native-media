#import <AVFoundation/AVFoundation.h>
#include "av_audio_backend.hpp"

@interface AVPlayerDelegateHelper : NSObject <AVAudioPlayerDelegate>
@property (nonatomic, assign) game_lab::PlaybackListener* listener;
@property (nonatomic, assign) uint64_t ticket;
@end

@implementation AVPlayerDelegateHelper
- (void)deliverEvent:(game_lab::PlaybackEvent)event {
    // App controls and listener mutations share the main queue. Recheck the
    // weak helper at delivery so retired players cannot deliver queued events.
    __weak AVPlayerDelegateHelper* weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        AVPlayerDelegateHelper* helper = weakSelf;
        if (helper && helper.listener && helper.ticket)
            helper.listener->onPlaybackEvent(helper.ticket, event);
    });
}
- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag {
    (void)player;
    [self deliverEvent:flag ? game_lab::PlaybackEvent::Finished : game_lab::PlaybackEvent::DecodeError];
}
- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error {
    (void)player; (void)error;
    [self deliverEvent:game_lab::PlaybackEvent::DecodeError];
}
@end

namespace game_lab {
namespace {
class AVPlayback final : public AudioPlayback {
public:
    explicit AVPlayback(AVAudioPlayer* player) : player_(player) {
        helper_ = [[AVPlayerDelegateHelper alloc] init];
        player_.delegate = helper_;
    }
    ~AVPlayback() override {
        player_.delegate = nil;
        helper_.listener = nullptr;
        helper_.ticket = 0;
    }
    void setListener(PlaybackListener* listener, uint64_t ticket) override {
        helper_.listener = listener;
        helper_.ticket = ticket;
    }
    void setGain(double gain) override { player_.volume = gain; }
    void setLoops(int loops) override { player_.numberOfLoops = loops; }
    bool play() override { return [player_ play]; }
    void pause() override { [player_ pause]; }
    void stop() override { [player_ stop]; }
    bool playing() const override { return player_.isPlaying; }
    double duration() const override { return player_.duration; }
private:
    AVAudioPlayer* __strong player_;
    AVPlayerDelegateHelper* __strong helper_;
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
