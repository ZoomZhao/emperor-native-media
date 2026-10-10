#import <Foundation/Foundation.h>
#include "app_media.hpp"
#include "scripts/self_fixture_evidence.hpp"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <functional>

struct Record {
    game_lab::PlaybackListener* listener=nullptr;
    uint64_t ticket=0;
    bool playing=false, destroyed=false, insidePlay=false, destroyedDuringPlay=false;
    bool emitDuringPlay=false;
    bool accepted=true;
    int stops=0;
    std::function<void()> duringPlay;
    game_lab::PlaybackEvent event=game_lab::PlaybackEvent::DecodeError;
};
class Fake final : public game_lab::AudioPlayback {
    std::shared_ptr<Record> r_;
public:
    explicit Fake(std::shared_ptr<Record> r):r_(std::move(r)){}
    ~Fake() override { r_->destroyed=true; r_->destroyedDuringPlay=r_->insidePlay; }
    void setListener(game_lab::PlaybackListener* listener,uint64_t ticket) override {r_->listener=listener;r_->ticket=ticket;}
    void setGain(double) override {}
    void setLoops(int) override {}
    bool play() override {
        auto r=r_; // Retain only the observation record, never the backend object.
        r->playing=true;r->insidePlay=true;
        if(r->emitDuringPlay && r->listener) {
            self_fixture::numbers("event",{r->ticket,static_cast<uint64_t>(r->event)});
            r->listener->onPlaybackEvent(r->ticket,r->event);
        }
        if (r->duringPlay) r->duringPlay();
        r->insidePlay=false;
        return r->accepted;
    }
    void pause() override {r_->playing=false;}
    void stop() override {r_->playing=false;++r_->stops;}
    bool playing() const override {return r_->playing;}
    double duration() const override {return 0.01;}
};
class Factory final : public game_lab::AudioFactory {
public:
    bool immediateError=false;
    bool accepted=true;
    game_lab::PlaybackEvent event=game_lab::PlaybackEvent::DecodeError;
    std::vector<std::shared_ptr<Record>> records;
    std::unique_ptr<game_lab::AudioPlayback> create(const std::vector<uint8_t>& bytes) override {
        self_fixture::bytes("factory-actual-data",bytes);
        auto r=std::make_shared<Record>();r->emitDuringPlay=immediateError;r->accepted=accepted;r->event=event;records.push_back(r);
        return std::make_unique<Fake>(r);
    }
};
void le16(std::vector<uint8_t>&v,uint16_t n){v.push_back(n&255);v.push_back(n>>8);}
void le32(std::vector<uint8_t>&v,uint32_t n){for(int i=0;i<4;++i)v.push_back((n>>(8*i))&255);}
std::vector<uint8_t> wav(){
    std::vector<uint8_t> v{'R','I','F','F'};le32(v,40);v.insert(v.end(),{'W','A','V','E','f','m','t',' '});le32(v,16);
    le16(v,1);le16(v,1);le32(v,22050);le32(v,44100);le16(v,2);le16(v,16);
    v.insert(v.end(),{'d','a','t','a'});le32(v,4);le16(v,0x1234);le16(v,0xffff);return v;
}
int main(int argc,char**argv){@autoreleasepool{
    auto directory=std::filesystem::current_path()/"self-root";std::filesystem::create_directories(directory);
    auto pcm=wav();self_fixture::bytes("wav",pcm);std::ofstream file(directory/"self.wav",std::ios::binary);file.write(reinterpret_cast<const char*>(pcm.data()),pcm.size());file.close();
    if(!file)return 2;
    auto root=game_lab::ResourceRoot::fromAbsolute(directory.string());if(!root.configured())return 2;
    if(argc==2){
        const std::string action=argv[1];
        if(action!="nested-replacement" && action!="stop-during-play")return 2;
        bool lifetime=true, state=true, ownership=true;
        // Exercise reentrant control with and without an earlier terminal event.
        for(bool terminal : {false,true}){
            Factory f;game_lab::AppMedia app(root,f);app.setMuted(false);
            app.selectMusic("self.wav");app.toggleMusic();
            auto old=f.records.back();old->emitDuringPlay=terminal;old->accepted=false;
            old->duringPlay=[&]{
                self_fixture::numbers("control-during-play",{terminal?1u:0u,action=="nested-replacement"?1u:0u});
                if(action=="nested-replacement")app.selectMusic("self.wav");else app.stopAll();
            };
            app.toggleMusic();
            lifetime=lifetime && !old->destroyedDuringPlay;
            ownership=ownership && old->destroyed && !old->playing;
            if(action=="nested-replacement"){
                auto current=f.records.back();
                ownership=ownership && current!=old && !current->destroyed && current->stops==0;
                state=state && app.musicPlaying() && app.status().state==game_lab::PlaybackState::Playing;
            }else{
                state=state && !app.musicPlaying() && app.status().state==game_lab::PlaybackState::Stopped;
            }
        }
        std::printf("{\"action\":\"%s\",\"with_and_without_terminal_callback\":true,\"backend_survives_own_play_callback\":%s,\"ownership_preserved\":%s,\"status_preserved\":%s}\n",action.c_str(),lifetime?"true":"false",ownership?"true":"false",state?"true":"false");
        return lifetime && state && ownership?0:1;
    }
    if(argc==3){
        const std::string path=argv[1], kind=argv[2];
        if((path!="music-start" && path!="music-resume" && path!="sfx-start") || (kind!="error" && kind!="finished"))return 2;
        const auto event=kind=="error"?game_lab::PlaybackEvent::DecodeError:game_lab::PlaybackEvent::Finished;
        Factory f;game_lab::AppMedia app(root,f);app.setMuted(false);
        size_t notificationCount=0;
        auto notifiedState=game_lab::PlaybackState::Muted;
        const auto observer=[&](const game_lab::PlaybackStatus& status){++notificationCount;notifiedState=status.state;};
        if(path=="music-resume"){
            app.selectMusic("self.wav");app.toggleMusic();
            auto r=f.records.back();r->emitDuringPlay=true;r->accepted=false;r->event=event;
            app.setStatusChangedCallback(observer);
            app.toggleMusic();
        }else{
            f.immediateError=true;f.accepted=false;f.event=event;
            app.setStatusChangedCallback(observer);
            if(path=="music-start")app.selectMusic("self.wav");else app.playSfx("self.wav");
        }
        auto r=f.records.back();
        const auto expected=kind=="error"?game_lab::PlaybackState::DecodeFailure:game_lab::PlaybackState::Stopped;
        const bool status=app.status().state==expected;
        const bool retired=r->destroyed && !r->playing && !app.musicPlaying();
        const bool lifetime=!r->destroyedDuringPlay;
        const bool notified=notificationCount==1 && notifiedState==expected;
        std::printf("{\"path\":\"%s\",\"event\":\"%s\",\"return_value\":false,\"status_preserved\":%s,\"retired\":%s,\"backend_survives_own_play_callback\":%s,\"terminal_notification_once\":%s}\n",path.c_str(),kind.c_str(),status?"true":"false",retired?"true":"false",lifetime?"true":"false",notified?"true":"false");
        return status && retired && lifetime && notified?0:1;
    }
    bool overlap=false, synchronousStart=false, synchronousResume=false, callbackLifetime=false;
    {
        Factory f;game_lab::AppMedia app(root,f);app.setMuted(false);app.selectMusic("self.wav");app.playSfx("self.wav");
        auto sfx=f.records.back();self_fixture::numbers("event",{sfx->ticket,static_cast<uint64_t>(game_lab::PlaybackEvent::Finished)});sfx->listener->onPlaybackEvent(sfx->ticket,game_lab::PlaybackEvent::Finished);
        overlap=app.musicPlaying() && app.status().state==game_lab::PlaybackState::Playing && sfx->destroyed;
    }
    {
        Factory f;f.immediateError=true;game_lab::AppMedia app(root,f);app.setMuted(false);app.playSfx("self.wav");
        auto sfx=f.records.back();synchronousStart=sfx->destroyed && app.status().state==game_lab::PlaybackState::DecodeFailure;
    }
    {
        Factory f;game_lab::AppMedia app(root,f);app.setMuted(false);app.selectMusic("self.wav");
        auto music=f.records.back();app.toggleMusic();music->emitDuringPlay=true;app.toggleMusic();
        synchronousResume=music->destroyed && !app.musicPlaying() && app.status().state==game_lab::PlaybackState::DecodeFailure;
        callbackLifetime=!music->destroyedDuringPlay;
    }
    std::printf("{\"overlap_status\":%s,\"synchronous_start\":%s,\"synchronous_resume\":%s,\"backend_survives_own_play_callback\":%s,\"ui\":false,\"audio_devices\":false,\"input\":\"self-authored PCM16 and fake events\"}\n",overlap?"true":"false",synchronousStart?"true":"false",synchronousResume?"true":"false",callbackLifetime?"true":"false");
    return overlap && synchronousStart && synchronousResume && callbackLifetime ? 0 : 1;
}}
