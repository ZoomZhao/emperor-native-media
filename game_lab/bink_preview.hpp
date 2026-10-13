#pragma once
#include "../media_demo/controller.hpp"
#include "../stream_lab/bink_process.hpp"
#include <chrono>
#include <cmath>

namespace game_lab {
// Worker-owned video-only branch. It reuses MovieReader and MovieSession's
// bounded asynchronous delivery, but does not imply PCM playback or A/V sync.
class BinkPreview {
public:
    explicit BinkPreview(std::string path):path_(std::move(path)),info_(stream_lab::readBinkInfo(path_)){}
    ~BinkPreview(){close();}
    media_demo::Snapshot snapshot() const {
        media_demo::Snapshot out;out.state=state_;out.frame=frameIndex_;out.stats=stats_;
        out.imageWidth=info_.width;out.imageHeight=info_.height;out.imageStride=info_.stride();
        out.fpsNumerator=info_.fpsNumerator;out.fpsDenominator=info_.fpsDenominator;
        out.videoFrames=info_.frames;out.positionSeconds=position_;out.durationSeconds=info_.duration();
        out.readers=reader_.usage().readers;out.videoBytes=held_.bgra.size();
        out.stats.decodedVideoFrames=reader_.stats().videoPackets;
        out.stats.maxVideoBytes=reader_.stats().maxVideoBufferBytes;
        out.message="Bink 静默视频 · 外部 FFmpeg 解码";return out;
    }
    void close(){
        reader_.close();held_={};
        if(active_)++stats_.runtimeReleases;
        active_=false;state_=media_demo::State::Ready;position_=0;frameIndex_=0;
    }
    media_demo::Snapshot perform(media_demo::Action action,uint64_t target,
                                const std::shared_ptr<stream_lab::OpenControl>& control) {
        try {
            switch(action){
            case media_demo::Action::Play:
                if(state_==media_demo::State::Playing)return snapshot();
                if(state_==media_demo::State::Paused){anchor_=Clock::now();anchorSeconds_=position_;state_=media_demo::State::Playing;return snapshot();}
                return begin(0,true,control);
            case media_demo::Action::Pause:{
                auto out=tick(control);
                if(state_==media_demo::State::Playing)state_=media_demo::State::Paused;
                out.state=state_;return out;}
            case media_demo::Action::Seek:
                return begin(target,state_==media_demo::State::Playing,control);
            case media_demo::Action::Stop:{close();auto out=snapshot();out.clearImage=true;return out;}
            case media_demo::Action::Tick:return tick(control);
            }
        }catch(const std::exception& error){close();state_=media_demo::State::Failed;
            auto out=snapshot();out.clearImage=true;out.message=error.what();return out;}
        return snapshot();
    }
private:
    using Clock=std::chrono::steady_clock;
    std::string path_;stream_lab::BinkInfo info_;stream_lab::MovieReader reader_;
    video_lab::Frame held_;media_demo::ControllerStats stats_;
    media_demo::State state_=media_demo::State::Ready;bool active_=false;
    Clock::time_point anchor_;double anchorSeconds_=0,position_=0;uint64_t frameIndex_=0;
    media_demo::Snapshot begin(uint64_t first,bool playing,const std::shared_ptr<stream_lab::OpenControl>& control){
        if(first>=info_.frames)throw std::runtime_error("Bink seek outside video");
        close();reader_.open(path_,first,control);active_=true;++stats_.runtimeOpens;
        if(!reader_.nextFrame(held_,control))throw std::runtime_error("Bink has no requested video frame");
        frameIndex_=first;position_=double(first)*info_.fpsDenominator/info_.fpsNumerator;
        anchorSeconds_=position_;anchor_=Clock::now();
        state_=playing?media_demo::State::Playing:media_demo::State::Paused;
        ++stats_.renders;auto out=snapshot();out.bgra=held_.bgra;return out;
    }
    media_demo::Snapshot tick(const std::shared_ptr<stream_lab::OpenControl>& control){
        if(state_!=media_demo::State::Playing)return snapshot();
        position_=std::min(info_.duration(),anchorSeconds_+std::chrono::duration<double>(Clock::now()-anchor_).count());
        const uint64_t due=std::min(info_.frames-1,uint64_t(std::floor(position_*info_.fpsNumerator/info_.fpsDenominator)));
        bool changed=false;
        while(frameIndex_<due){video_lab::Frame next;
            if(!reader_.nextFrame(next,control))throw std::runtime_error("Bink timeline reached premature EOF");
            if(changed)++stats_.discardedVideo;
            held_=std::move(next);frameIndex_=held_.pts;changed=true;}
        if(changed)++stats_.renders;
        auto out=snapshot();if(changed)out.bgra=held_.bgra;
        if(position_>=info_.duration()){
            video_lab::Frame extra;if(reader_.nextFrame(extra,control))throw std::runtime_error("Bink has extra video frames");
            const auto endFrame=frameIndex_;close();state_=media_demo::State::Ended;
            position_=info_.duration();frameIndex_=endFrame;
            auto ended=snapshot();ended.bgra=std::move(out.bgra);return ended;
        }
        return out;
    }
};
} // namespace game_lab
