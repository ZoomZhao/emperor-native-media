#pragma once
#include "../media_demo/controller.hpp"
#include "bink_audio.hpp"
#include <chrono>
#include <cmath>

namespace game_lab {
// Worker-owned bounded Bink playback. Video follows the native PCM clock when
// a single audio track exists; silent files retain the monotonic video clock.
class BinkPreview {
public:
    explicit BinkPreview(std::string path,std::shared_ptr<MovieAudioGain> gain=std::make_shared<MovieAudioGain>())
        :path_(std::move(path)),info_(stream_lab::readBinkInfo(path_)),gain_(std::move(gain)),audio_(gain_){}
    ~BinkPreview(){close();}
    media_demo::Snapshot snapshot(bool observeClock=true) {
        media_demo::Snapshot out;out.state=state_;out.frame=frameIndex_;out.stats=stats_;
        out.imageWidth=info_.width;out.imageHeight=info_.height;out.imageStride=info_.stride();
        out.fpsNumerator=info_.fpsNumerator;out.fpsDenominator=info_.fpsDenominator;
        out.videoFrames=info_.frames;out.positionSeconds=position_;out.durationSeconds=info_.duration();
        out.readers=reader_.usage().readers;out.videoBytes=held_.bgra.size();
        out.stats.decodedVideoFrames=reader_.stats().videoPackets;
        out.stats.maxVideoBytes=reader_.stats().maxVideoBufferBytes;
        if(info_.audioTracks){
            out.nativeAudio=true;out.audioRate=info_.audioRate;out.audioChannels=info_.audioChannels;
            out.audioDecodedFrames=audio_.decoded();out.audioSubmittedFrames=audio_.submitted();
            out.audioReturnedFrames=audio_.returned();out.audioClockFrames=audio_.base()+audio_.currentFrames(observeClock);
            out.audioClockObservations=audio_.observations();out.audioResumeBackstepFrames=audio_.maxResumeBackstep();out.audioEOF=audio_.eof();out.audioDrained=audio_.finished();
            out.audioGain=gain_->value();out.readers+=audio_.readers();out.pcmQueue=audio_.pendingBuffers();
            out.nativePCMBytes=audio_.nativeBytes();
            out.stats.maxPCMBytes=BinkAudio::bufferCount*BinkAudio::packetFrames*info_.audioChannels*2+BinkAudio::packetFrames*info_.audioChannels*2;
            out.avDifferenceSeconds=double(out.audioClockFrames)/info_.audioRate-double(frameIndex_)*info_.fpsDenominator/info_.fpsNumerator;
            if(observeClock&&state_==media_demo::State::Playing)maxDifference_=std::max(maxDifference_,std::abs(out.avDifferenceSeconds));
            out.maxAVDifferenceSeconds=maxDifference_;
        }
        out.message=info_.audioTracks?(gain_->value()>0?"Bink 音画播放":"Bink 音画播放 · 静音"):"Bink 静默视频（无音轨）";return out;
    }
    void close(){
        audio_.close();reader_.close();held_={};dirty_=false;
        if(active_)++stats_.runtimeReleases;
        active_=false;state_=media_demo::State::Ready;position_=0;frameIndex_=0;
    }
    media_demo::Snapshot perform(media_demo::Action action,uint64_t target,
                                const std::shared_ptr<stream_lab::OpenControl>& control) {
        try {
            switch(action){
            case media_demo::Action::Play:
                if(state_==media_demo::State::Playing)return snapshot();
                if(state_==media_demo::State::Paused){
                    // Bring video to the frozen audio position before restarting
                    // output. A second Pause can interrupt this read safely too.
                    advanceVideo(control);
                    if(info_.audioTracks)audio_.play();anchor_=Clock::now();anchorSeconds_=position_;
                    state_=media_demo::State::Playing;return imageSnapshot();}
                return begin(0,true,control);
            case media_demo::Action::Pause:{
                if(state_==media_demo::State::Playing){
                    if(info_.audioTracks){audio_.pause();position_=std::min(info_.duration(),double(audio_.base()+audio_.currentFrames())/info_.audioRate);}
                    else position_=std::min(info_.duration(),anchorSeconds_+std::chrono::duration<double>(Clock::now()-anchor_).count());
                    state_=media_demo::State::Paused;
                }
                // Pause never asks the decoder for more bytes.
                return imageSnapshot();}
            case media_demo::Action::Seek:
                return begin(target,state_==media_demo::State::Playing,control);
            case media_demo::Action::Stop:{close();auto out=snapshot();out.clearImage=true;return out;}
            case media_demo::Action::Tick:return tick(control);
            }
        }catch(const stream_lab::BinkReadCancelled& error){
            if(active_&&(state_==media_demo::State::Playing||state_==media_demo::State::Paused))
                return snapshot(false); // Superseded delivery; preserve owned byte prefixes/queues.
            close();state_=media_demo::State::Failed;auto out=snapshot();out.clearImage=true;out.message=error.what();return out;
        }catch(const std::exception& error){close();state_=media_demo::State::Failed;
            auto out=snapshot();out.clearImage=true;out.message=error.what();return out;}
        return snapshot();
    }
private:
    using Clock=std::chrono::steady_clock;
    std::string path_;stream_lab::BinkInfo info_;stream_lab::MovieReader reader_;
    std::shared_ptr<MovieAudioGain> gain_;BinkAudio audio_;
    video_lab::Frame held_;media_demo::ControllerStats stats_;
    bool videoEOF_=false,dirty_=false;double maxDifference_=0;
    media_demo::State state_=media_demo::State::Ready;bool active_=false;
    Clock::time_point anchor_;double anchorSeconds_=0,position_=0;uint64_t frameIndex_=0;
    media_demo::Snapshot begin(uint64_t first,bool playing,const std::shared_ptr<stream_lab::OpenControl>& control){
        if(first>=info_.frames)throw std::runtime_error("Bink seek outside video");
        close();videoEOF_=false;maxDifference_=0;reader_.open(path_,first,control);active_=true;++stats_.runtimeOpens;
        if(!reader_.nextFrame(held_,control))throw std::runtime_error("Bink has no requested video frame");
        frameIndex_=first;position_=double(first)*info_.fpsDenominator/info_.fpsNumerator;
        if(info_.audioTracks){
            const auto base=uint64_t(double(first)*info_.fpsDenominator*info_.audioRate/info_.fpsNumerator);
            audio_.open(path_,base,[control]{return control&&control->cancelled.load();});
            if(playing)audio_.play();
        }
        anchorSeconds_=position_;anchor_=Clock::now();
        state_=playing?media_demo::State::Playing:media_demo::State::Paused;
        dirty_=true;return imageSnapshot();
    }
    media_demo::Snapshot imageSnapshot(){
        if(dirty_)++stats_.renders;
        auto out=snapshot();if(dirty_){out.bgra=held_.bgra;dirty_=false;}return out;
    }
    void advanceVideo(const std::shared_ptr<stream_lab::OpenControl>& control){
        auto dueFrame=[this]{return std::min(info_.frames-1,uint64_t(std::floor(position_*info_.fpsNumerator/info_.fpsDenominator)));};
        uint64_t due=dueFrame();size_t caught=0;
        bool changed=false;
        // Re-read the master clock while decoding: a slow read must not leave
        // this delivery scheduled against an old timestamp. Cap worker catch-up
        // so a long owner stall still yields to pending pause/stop/replacement.
        while(frameIndex_<due&&caught<240){video_lab::Frame next;
            if(!reader_.nextFrame(next,control))throw std::runtime_error("Bink timeline reached premature EOF");
            if(changed)++stats_.discardedVideo;
            held_=std::move(next);frameIndex_=held_.pts;changed=true;dirty_=true;++caught;
            if(info_.audioTracks){position_=std::min(info_.duration(),double(audio_.base()+audio_.currentFrames())/info_.audioRate);due=dueFrame();}}
        stats_.maxCatchupFrames=std::max(stats_.maxCatchupFrames,uint64_t(caught));
    }
    media_demo::Snapshot tick(const std::shared_ptr<stream_lab::OpenControl>& control){
        if(state_!=media_demo::State::Playing)return snapshot();
        if(info_.audioTracks){audio_.service([control]{return control&&control->cancelled.load();});
            position_=std::min(info_.duration(),double(audio_.base()+audio_.currentFrames())/info_.audioRate);
            if(audio_.finished()&&position_+1.0/info_.audioRate<info_.duration())
                throw std::runtime_error("Bink audio ends before video; this extent is unsupported");
        }else position_=std::min(info_.duration(),anchorSeconds_+std::chrono::duration<double>(Clock::now()-anchor_).count());
        advanceVideo(control);
        auto out=imageSnapshot();
        if(position_>=info_.duration()&&frameIndex_==info_.frames-1){
            if(!videoEOF_){video_lab::Frame extra;if(reader_.nextFrame(extra,control))throw std::runtime_error("Bink has extra video frames");videoEOF_=true;}
            if(info_.audioTracks&&!audio_.finished())return out;
            const auto endFrame=frameIndex_;close();state_=media_demo::State::Ended;
            position_=info_.duration();frameIndex_=endFrame;
            auto ended=snapshot();ended.bgra=std::move(out.bgra);return ended;
        }
        return out;
    }
};
} // namespace game_lab
