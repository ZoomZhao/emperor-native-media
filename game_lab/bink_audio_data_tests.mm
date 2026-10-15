#import <Foundation/Foundation.h>
#include "movie_session.hpp"
#include "../stream_lab/bink_process.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

namespace {
size_t checks=0;
void require(bool value,const char* reason){++checks;if(!value)throw std::runtime_error(reason);}
void shortAudioDoesNotHang(){
    // Authored stream protocol only: one-second video, 10-ms silent PCM.
    // It exercises a failure boundary; it is not original Bink format evidence.
    char folderName[]="/tmp/emperor-short-audio-XXXXXX";char* made=mkdtemp(folderName);
    require(made,"authored short-audio directory");const std::filesystem::path folder(made);
    const char* old=std::getenv("EMPEROR_FFMPEG_PATH");const bool had=old;const std::string prior=old?old:"";
    auto restore=[&]{if(had)setenv("EMPEROR_FFMPEG_PATH",prior.c_str(),1);else unsetenv("EMPEROR_FFMPEG_PATH");};
    try {
        std::vector<uint8_t> h(56);h[0]='B';h[1]='I';h[2]='K';h[3]='i';h[4]=48;h[8]=30;
        h[20]=h[24]=h[32]=h[40]=1;h[28]=30;h[48]=0x44;h[49]=0xac;h[51]=0x20;
        {std::ofstream f(folder/"short.bik",std::ios::binary);f.write(reinterpret_cast<const char*>(h.data()),h.size());}
        auto huge=h;huge[28]=1;huge[32]=huge[33]=huge[34]=huge[35]=255;
        {std::ofstream f(folder/"huge.bik",std::ios::binary);f.write(reinterpret_cast<const char*>(huge.data()),huge.size());}
        bool bounded=false;try{stream_lab::readBinkInfo((folder/"huge.bik").string());}catch(const std::exception&){bounded=true;}
        require(bounded,"huge rational duration rejected before PCM sample arithmetic");
        const auto decoder=folder/"decoder.py";
        {std::ofstream f(decoder);f<<"#!/usr/bin/python3\nimport os,sys\na=sys.argv\nos.write(1,bytes(441*4) if a[a.index('-map')+1]=='0:a:0' else bytes([0,0,0,255])*30)\n";}
        require(chmod(decoder.c_str(),0700)==0&&setenv("EMPEROR_FFMPEG_PATH",decoder.c_str(),1)==0,"authored short-audio child setup");
        game_lab::MovieSession session(game_lab::ResourceRoot::fromAbsolute(folder.string()));
        require(session.openMovie("short.bik"),"authored short-audio open");
        auto s=session.performSync(media_demo::Action::Play).snapshot;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(s.state==media_demo::State::Playing&&std::chrono::steady_clock::now()<deadline){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));s=session.performSync(media_demo::Action::Tick).snapshot;}
        require(s.state==media_demo::State::Failed&&s.message.find("audio ends before video")!=std::string::npos,"short audio fails explicitly instead of freezing video forever");
        session.close();require(session.resourcesZero(),"short-audio rejection closes native sink and both children");
        restore();std::filesystem::remove_all(folder);
    }catch(...){restore();std::filesystem::remove_all(folder);throw;}
}

}
int main(int argc,char** argv){@autoreleasepool{
    const char* supplied=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!supplied||!*supplied){std::cout<<"SKIP: EMPEROR_GAMEDATA_ROOT is unset\n";return 0;}
    if(argc!=2){std::cerr<<"usage: bink-audio-data-tests RELATIVE_BIK_PATH\n";return 2;}
    try {
        auto root=game_lab::ResourceRoot::fromAbsolute(supplied);const auto path=root.resolve(argv[1]);
        require(!path.empty(),"audio remains inside explicit data root");const auto info=stream_lab::readBinkInfo(path);
        require(info.audioTracks==1&&info.audioRate&&info.audioChannels,"one bounded native audio track");
        stream_lab::BinkProcess reader;reader.openAudio(path,0);std::vector<int16_t> pcm;
        uint64_t frames=0,nonzero=0;size_t peak=0;
        for(;;){const auto count=reader.nextAudio(pcm,[]{return false;});if(!count)break;
            require(count<=4096&&pcm.size()==count*info.audioChannels,"bounded whole PCM packet");
            peak=std::max(peak,pcm.size()*2);frames+=count;for(auto sample:pcm)nonzero+=sample!=0;}
        require(frames>uint64_t(info.duration()*info.audioRate)-info.audioRate&&nonzero>0,"complete nonempty audio stream near video extent");
        require(!reader.active(),"audio child reaped at EOF");reader.close();
        reader.openAudio(path,0);bool cancelled=false;
        try{reader.nextAudio(pcm,[]{return true;});}catch(const std::exception&){cancelled=true;}
        reader.close();require(cancelled&&!reader.active(),"audio cancellation reaps owned child");
        game_lab::MovieSession session(root);require(session.openMovie(argv[1]),"native audio session opens");
        auto s=session.performSync(media_demo::Action::Play).snapshot;
        require(s.state==media_demo::State::Playing&&s.nativeAudio&&s.audioGain==0,"default native output is muted");
        require(s.readers==2&&s.nativePCMBytes<=6*4096*info.audioChannels*2,"two independent pipes and bounded native buffers");
        session.setAudioGain(0.03);s=session.performSync(media_demo::Action::Tick).snapshot;
        require(s.audioGain==0.03,"application gain is applied");session.setAudioGain(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));s=session.performSync(media_demo::Action::Pause).snapshot;
        require(s.state==media_demo::State::Paused&&s.audioClockFrames>0,"native audio clock advances before pause");
        const auto frozen=s.audioClockFrames;std::this_thread::sleep_for(std::chrono::milliseconds(120));
        s=session.performSync(media_demo::Action::Tick).snapshot;require(s.audioClockFrames==frozen,"pause freezes audio master");
        s=session.performSync(media_demo::Action::Play).snapshot;require(s.audioClockFrames>=frozen&&s.audioClockFrames-frozen<info.audioRate/10,"resume preserves audio position");
        session.performSync(media_demo::Action::Stop);require(session.resourcesZero(),"active Stop releases audio and both children");
        // Final-frame seek must retain the decoder's partial final packet/tail,
        // and must not report Ended merely because its last buffer was reused.
        const auto last=info.frames-1;
        s=session.performSync(media_demo::Action::Seek,last).snapshot;
        require(s.state==media_demo::State::Paused&&s.frame==last,"paused seek sets matching video/audio origin");
        const uint64_t base=uint64_t(double(last)*info.fpsDenominator*info.audioRate/info.fpsNumerator);
        require(s.audioDecodedFrames==frames-base,"seek decodes exact remaining PCM samples");
        session.performSync(media_demo::Action::Play);
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
        do {std::this_thread::sleep_for(std::chrono::milliseconds(10));s=session.performSync(media_demo::Action::Tick).snapshot;}
        while(s.state==media_demo::State::Playing&&std::chrono::steady_clock::now()<deadline);
        require(s.state==media_demo::State::Ended&&s.resourcesZero(),"natural EOF drains and releases native output");
        require(s.audioEOF&&s.audioDrained&&s.audioClockFrames==frames,"end follows complete native audio extent");
        require(s.audioSubmittedFrames==frames-base&&s.audioReturnedFrames==frames-base,"partial tail submitted and returned exactly once");
        session.close();require(session.resourcesZero(),"session closes with no output resources");
        shortAudioDoesNotHang();
        std::cout<<"{\"checks\":"<<checks<<",\"audio_files\":1,\"files\":1,\"decoded_sample_frames\":"<<frames
                 <<",\"channels\":"<<info.audioChannels<<",\"sample_rate\":"<<info.audioRate<<",\"max_packet_bytes\":"<<peak
                 <<",\"tail_sample_frames\":"<<frames-base<<",\"native_tail_drained\":true,\"default_muted\":true,\"authored_short_audio_rejected\":true,\"sound_quality_measured\":false}\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}}
