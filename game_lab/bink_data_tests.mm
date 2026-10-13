#import <Foundation/Foundation.h>
#include "movie_session.hpp"
#include "cg_image_conversion.hpp"
#include "../stream_lab/bink_process.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>
#include <fstream>
#include <sys/stat.h>

namespace {
size_t checks=0;
void require(bool value,const char* reason){++checks;if(!value)throw std::runtime_error(reason);}
struct Presenter:game_lab::ImagePresenter {
    uint32_t width=0,height=0;size_t calls=0;
    void clear() override {}
    bool present(uint32_t w,uint32_t h,const std::vector<uint32_t>& p) override {
        width=w;height=h;++calls;return p.size()==size_t(w)*h;
    }
};
void controlledChildCleanup(){
    // Entirely authored header/child protocol. This checks reaping, not Bink decoding.
    char name[]="/tmp/emperor-owned-child-XXXXXX";char* made=mkdtemp(name);
    require(made,"owned process fixture directory");const std::filesystem::path folder(made);
    const auto file=folder/"header-only.bik",helper=folder/"child.py",marker=folder/"finished";
    const char* prior=std::getenv("EMPEROR_FFMPEG_PATH");const bool hadPrior=prior;
    const std::string previous=prior?prior:"";
    auto restore=[&]{if(hadPrior)setenv("EMPEROR_FFMPEG_PATH",previous.c_str(),1);else unsetenv("EMPEROR_FFMPEG_PATH");};
    try{
        std::vector<uint8_t> header(44,0);header[0]='B';header[1]='I';header[2]='K';header[3]='i';
        header[4]=36;header[8]=1;header[20]=1;header[24]=1;header[28]=30;header[32]=1;
        {std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(header.data()),header.size());}
        {std::ofstream out(helper);out<<R"PY(#!/usr/bin/python3
import os,signal,time
from pathlib import Path
signal.signal(signal.SIGTERM,signal.SIG_IGN)
os.write(1,bytes([1,2,3,255]))
time.sleep(.7)
Path(__file__).with_name('finished').write_text('natural child completion')
)PY";}
        require(chmod(helper.c_str(),0700)==0,"owned child executable");
        require(setenv("EMPEROR_FFMPEG_PATH",helper.c_str(),1)==0,"process-local child dependency injection");
        stream_lab::BinkProcess process;process.open(file.string(),0);video_lab::Frame frame;
        require(process.nextFrame(frame,[]{return false;}),"owned child readiness after TERM handler installed");
        const auto start=std::chrono::steady_clock::now();process.close();
        const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        require(seconds>.55&&std::filesystem::exists(marker)&&!process.active(),"child cleanup waits past 250ms for natural completion without force kill");
        restore();std::filesystem::remove_all(folder);
    }catch(...){restore();std::filesystem::remove_all(folder);throw;}
}
void legacyMovCompatibility(stream_lab::MovieReader& reused){
    char name[]="/tmp/emperor-owned-mov-XXXXXX";
    char* made=mkdtemp(name);require(made,"own fixture directory");
    const std::filesystem::path folder(made),movie=folder/"synthetic.mov";
    try{
        stream_lab::writeSyntheticMovie(movie.string());
        reused.open(movie.string());video_lab::Frame ownFrame;
        require(reused.metadata().width==64&&reused.nextFrame(ownFrame),"Reader reopens MOV after Bink");
        reused.close();
        game_lab::MovieSession session(game_lab::ResourceRoot::fromAbsolute(folder.string()));
        Presenter p;require(session.openMovie("synthetic.mov"),"existing MOV session open");
        auto play=session.performSync(media_demo::Action::Play,0,&p);
        require(play.presented&&p.width==32&&p.height==24,"legacy MOV layout preserved");
        require(play.snapshot.durationSeconds==8,"legacy duration preserved");
        auto pause=session.performSync(media_demo::Action::Pause,0,&p);
        require(pause.snapshot.state==media_demo::State::Paused,"legacy pause");
        auto seek=session.performSync(media_demo::Action::Seek,10,&p);
        require(seek.snapshot.frame==10&&seek.presented,"legacy seek");
        session.close();require(session.resourcesZero(),"legacy close");
    }catch(...){std::filesystem::remove_all(folder);throw;}
    std::filesystem::remove_all(folder);
}
}
int main(int argc,char** argv){@autoreleasepool{
    const char* supplied=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!supplied||!*supplied){std::cout<<"SKIP: EMPEROR_GAMEDATA_ROOT is unset\n";return 0;}
    if(argc!=2&&(argc!=3||std::string(argv[2])!="--legacy-mov")){std::cerr<<"usage: bink-data-tests RELATIVE_BIK_PATH [--legacy-mov]\n";return 2;}
    try{
        auto root=game_lab::ResourceRoot::fromAbsolute(supplied);
        const auto path=root.resolve(argv[1]);require(!path.empty(),"video resolves inside explicit data root");
        const auto info=stream_lab::readBinkInfo(path);
        stream_lab::MovieReader reader;reader.open(path);
        const auto m=reader.metadata();
        require(m.width==info.width&&m.height==info.height&&m.frames==info.frames,
                "Reader uses original dimensions/frame count");
        require(m.fps==info.fpsNumerator&&reader.frameRateDenominator()==info.fpsDenominator,
                "Reader uses original rational frame rate");
        require(info.frameBytes()>1048576&&info.frameBytes()<=stream_lab::maxBinkFrameBytes,
                "real frame exceeds old cap and fits dynamic cap");
        const auto start=std::chrono::steady_clock::now();uint64_t count=0;
        video_lab::Frame frame;
        Presenter presenter;
        while(reader.nextFrame(frame)){
            require(frame.pts==count,"real frame sequence");
            require(frame.bgra.size()==info.frameBytes(),"real BGRA layout");
            if(count==0){
                media_demo::Snapshot snapshot;snapshot.imageWidth=m.width;snapshot.imageHeight=m.height;
                snapshot.imageStride=m.stride;snapshot.bgra=frame.bgra;
                game_lab::MovieDelivery delivered;
                require(game_lab::processMovieDelivery(1,1,snapshot,&presenter,delivered),"dynamic delivery accepted");
                require(delivered.presented&&presenter.width==m.width&&presenter.height==m.height,
                        "real frame reaches project's presenter without 32x24 resize");
                CGImageRef image=game_lab::createCGImageFromPremultipliedBGRA(m.width,m.height,delivered.pixelsBGRA);
                require(image!=nullptr,"original decoded BGRA converts to native CGImage");CGImageRelease(image);
                const auto calls=presenter.calls;
                require(!game_lab::processMovieDelivery(2,1,snapshot,&presenter,delivered)&&presenter.calls==calls,
                        "stale real frame does not reach presenter");
                snapshot.imageStride=stream_lab::maxBinkFrameBytes;
                require(game_lab::processMovieDelivery(1,1,snapshot,&presenter,delivered)&&!delivered.presented,
                        "oversized dynamic layout rejected");
            }
            ++count;
        }
        const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        require(count==info.frames,"full real-data frame denominator");
        require(reader.stats().videoPackets==count,"actual decoded-frame accounting");
        require(reader.usage().readers==0,"FFmpeg process reaped at EOF");reader.close();
        reader.open(path,info.frames/2);require(reader.nextFrame(frame)&&frame.pts==info.frames/2,"seek starts at requested original frame");
        auto cancel=std::make_shared<stream_lab::OpenControl>();cancel->cancelled=true;
        bool cancelled=false;try{reader.nextFrame(frame,cancel);}catch(const std::exception&){cancelled=true;}
        require(cancelled,"Bink read observes cancellation");reader.close();
        require(reader.usage().readers==0,"cancelled FFmpeg process reaped");
        game_lab::MovieSession session(root);
        bool posterReady=false;game_lab::MovieDelivery poster;
        session.requestPoster(argv[1],[&](const game_lab::MovieDelivery& value){poster=value;posterReady=true;},&presenter);
        const auto posterDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(!posterReady&&std::chrono::steady_clock::now()<posterDeadline)CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.01,false);
        require(posterReady&&poster.presented&&poster.width==info.width&&poster.height==info.height,"original poster presents full dimensions");
        require(poster.snapshot.durationSeconds==info.duration()&&poster.snapshot.videoFrames==info.frames,"poster reports original timeline metadata");
        require(session.resourcesZero(),"poster releases external decoder");
        require(session.openMovie(argv[1]),"project session recognizes original Bink");
        auto play=session.performSync(media_demo::Action::Play,0,&presenter);
        require(play.presented&&play.width==m.width&&play.height==m.height,"project session displays original dimensions");
        require(play.snapshot.state==media_demo::State::Playing&&play.snapshot.durationSeconds==info.duration(),"dynamic silent session timeline");
        auto pause=session.performSync(media_demo::Action::Pause,0,&presenter);
        require(pause.snapshot.state==media_demo::State::Paused,"Bink pause");
        const auto position=pause.snapshot.positionSeconds;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        auto frozen=session.performSync(media_demo::Action::Tick,0,&presenter);
        require(frozen.snapshot.positionSeconds==position,"pause freezes monotonic video timeline");
        auto seek=session.performSync(media_demo::Action::Seek,info.frames-1,&presenter);
        require(seek.snapshot.frame==info.frames-1&&seek.presented,"Bink seek displays last original frame");
        session.performSync(media_demo::Action::Play,0,&presenter);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        auto ended=session.performSync(media_demo::Action::Tick,0,&presenter);
        require(ended.snapshot.state==media_demo::State::Ended&&ended.snapshot.resourcesZero(),"Bink EOF releases child and frame cache");
        session.close();require(session.resourcesZero(),"project Bink session closes cleanly");
        controlledChildCleanup();
        if(argc==3)legacyMovCompatibility(reader);
        std::cout<<"{\"checks\":"<<checks<<",\"opened_files\":1,\"files\":1,\"decoded_frames\":"<<count
                 <<",\"header_frames\":"<<info.frames<<",\"decode_seconds\":"<<elapsed
                 <<",\"width\":"<<m.width<<",\"height\":"<<m.height<<",\"max_client_frame_bytes\":"<<info.frameBytes()
                 <<",\"frame_budget_bytes\":"<<stream_lab::maxBinkFrameBytes
                 <<",\"fps_numerator\":"<<m.fps<<",\"fps_denominator\":"<<info.fpsDenominator
                 <<",\"controlled_child_cleanup\":true,\"legacy_MOV_regression\":"<<(argc==3?"true":"null")<<",\"playback_accepted\":null}\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}}
