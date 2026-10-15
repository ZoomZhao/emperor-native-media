#pragma once
// Authored MIT adapter. FFmpeg remains a separately installed executable.
// Tested dependency: Homebrew FFmpeg 8.0.1 arm64, GPLv3-or-later. Do not bundle
// that executable or link its GPL-enabled libraries as part of this adapter.
// Published format: https://ffmpeg.org/doxygen/trunk/libavformat_2bink_8c_source.html
#include "../video_lab/stream.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <poll.h>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

extern char** environ;
namespace stream_lab {
// Cancellation interrupts an owned read; it is not malformed decoder output.
class BinkReadCancelled : public std::runtime_error {
public: BinkReadCancelled():std::runtime_error("external Bink decode cancelled"){}
};
constexpr size_t maxBinkFrameBytes = 8u * 1024u * 1024u;
struct BinkInfo {
    uint32_t width=0,height=0,fpsNumerator=0,fpsDenominator=0;
    uint64_t frames=0;
    uint32_t audioTracks=0,audioRate=0,audioChannels=0;
    size_t stride() const { return size_t(width)*4; }
    size_t frameBytes() const { return stride()*height; }
    double duration() const { return double(frames)*fpsDenominator/fpsNumerator; }
};
inline uint32_t binkU32(const uint8_t* p) {
    return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;
}
inline bool hasBinkSignature(const std::string& path) {
    std::ifstream file(path,std::ios::binary);char h[4]{};file.read(h,4);
    return file.gcount()==4&&((h[0]=='B'&&h[1]=='I'&&h[2]=='K')||
                              (h[0]=='K'&&h[1]=='B'&&h[2]=='2'));
}
inline BinkInfo readBinkInfo(const std::string& path) {
    std::array<uint8_t,44> h{};std::ifstream file(path,std::ios::binary);
    file.read(reinterpret_cast<char*>(h.data()),h.size());
    if(file.gcount()!=std::streamsize(h.size()))throw std::runtime_error("truncated Bink header");
    if(h[0]=='K'&&h[1]=='B'&&h[2]=='2')
        throw std::runtime_error("KB2 is Bink 2; this Bink 1 video adapter does not support it");
    const std::string revisions="bfghik";
    if(h[0]!='B'||h[1]!='I'||h[2]!='K'||revisions.find(char(h[3]))==std::string::npos)
        throw std::runtime_error("unsupported Bink 1 signature/revision");
    BinkInfo m{binkU32(h.data()+20),binkU32(h.data()+24),binkU32(h.data()+28),
               binkU32(h.data()+32),binkU32(h.data()+8)};
    if(!m.width||!m.height||m.width>4096||m.height>4096||
       m.stride()>maxBinkFrameBytes/m.height||!m.frames||m.frames>1000000||
       !m.fpsNumerator||!m.fpsDenominator||
       uint64_t(m.fpsNumerator)>uint64_t(240)*m.fpsDenominator||
       uint64_t(binkU32(h.data()+4))+8!=std::filesystem::file_size(path)||m.duration()>86400)
        throw std::runtime_error("Bink metadata exceeds bounded video layout or file size");
    m.audioTracks=binkU32(h.data()+40);
    if(m.audioTracks>256)throw std::runtime_error("Bink audio track count exceeds bound");
    if(m.audioTracks){
        const auto offset=44u+(h[3]=='k'?4u:0u)+4u*m.audioTracks;
        std::array<uint8_t,4> audio{};file.seekg(offset);file.read(reinterpret_cast<char*>(audio.data()),4);
        if(file.gcount()!=4)throw std::runtime_error("truncated Bink audio metadata");
        m.audioRate=uint32_t(audio[0])|uint32_t(audio[1])<<8;
        m.audioChannels=(audio[3]&0x20)?2:1;
        if(m.audioRate<8000||m.audioRate>192000)throw std::runtime_error("unsupported Bink audio sample rate");
    }
    return m;
}

// One child and two pipes per video/audio stream; no shell, files or clip cache.
// Client frames <=8 MiB. Pipe backpressure bounds decoded bytes in flight; the
// separately running FFmpeg decoder's internal memory is not counted as ours.
class BinkProcess {
public:
    ~BinkProcess(){close();}
    BinkProcess()=default;
    BinkProcess(const BinkProcess&)=delete;BinkProcess& operator=(const BinkProcess&)=delete;
    void open(const std::string& path,uint64_t first) {
        close();info_=readBinkInfo(path);audioMode_=false;
        if(first>=info_.frames)throw std::runtime_error("Bink seek outside video");
        const auto executable=decoderPath();
        std::vector<std::string> args={executable,"-nostdin","-hide_banner","-v","error",
            "-xerror","-err_detect","explode","-protocol_whitelist","file,pipe",
            "-threads","2","-filter_threads","1","-f","bink","-i",path,
            "-map","0:v:0","-an","-sn","-dn","-vf","trim=start_frame="+std::to_string(first),
            "-fps_mode","passthrough","-frames:v",std::to_string(info_.frames-first+1),
            "-c:v","rawvideo","-pix_fmt","bgra","-f","rawvideo","-blocksize","65536","pipe:1"};
        launch(args);next_=first;
    }
    // Audio is a separate backpressured child, so a full video pipe cannot
    // deadlock PCM reads. The first track only; no mixing/track guessing.
    void openAudio(const std::string& path,uint64_t firstSample){
        close();info_=readBinkInfo(path);audioMode_=true;
        if(info_.audioTracks!=1)throw std::runtime_error("native Bink audio requires exactly one track");
        const auto executable=decoderPath();
        std::vector<std::string> args={executable,"-nostdin","-hide_banner","-v","error",
            "-xerror","-err_detect","explode","-protocol_whitelist","file,pipe",
            "-threads","2","-f","bink","-i",path,"-map","0:a:0","-vn","-sn","-dn",
            "-af","atrim=start_sample="+std::to_string(firstSample),
            "-c:a","pcm_s16le","-ar",std::to_string(info_.audioRate),
            "-ac",std::to_string(info_.audioChannels),"-f","s16le","-blocksize","16384","pipe:1"};
        launch(args);
    }
    size_t nextAudio(std::vector<int16_t>& samples,const std::function<bool()>& cancelled){
        if(!audioMode_)throw std::runtime_error("not an audio decoder");
        if(eof_){samples.clear();return 0;}
        if(pendingPCM_.empty())pendingPCM_.resize(4096*info_.audioChannels);
        const size_t bytes=readBytes(reinterpret_cast<uint8_t*>(pendingPCM_.data()),pendingPCM_.size()*2,true,true,cancelled,received_);
        if(bytes%(2*info_.audioChannels))throw std::runtime_error("Bink partial PCM sample frame");
        pendingPCM_.resize(bytes/2);samples=std::move(pendingPCM_);received_=0;
        return bytes/(2*info_.audioChannels);
    }
private:
    static std::string decoderPath(){
        const char* supplied=std::getenv("EMPEROR_FFMPEG_PATH");
        const std::string executable=supplied&&*supplied?supplied:"/opt/homebrew/bin/ffmpeg";
        if(!std::filesystem::path(executable).is_absolute()||access(executable.c_str(),X_OK))
            throw std::runtime_error("Bink requires an existing external FFmpeg executable; set EMPEROR_FFMPEG_PATH");
        return executable;
    }
    void launch(std::vector<std::string>& args){
        int out[2]={-1,-1},err[2]={-1,-1};
        if(pipe(out)||pipe(err)){
            for(int fd:{out[0],out[1],err[0],err[1]})if(fd>=0)::close(fd);
            throw std::runtime_error("Bink decoder pipe creation failed");
        }
        for(int fd:{out[0],out[1],err[0],err[1]})fcntl(fd,F_SETFD,FD_CLOEXEC);
        posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_addopen(&actions,STDIN_FILENO,"/dev/null",O_RDONLY,0);
        posix_spawn_file_actions_adddup2(&actions,out[1],STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions,err[1],STDERR_FILENO);
        for(int fd:{out[0],out[1],err[0],err[1]})posix_spawn_file_actions_addclose(&actions,fd);
        std::vector<char*> argv;for(auto& arg:args)argv.push_back(arg.data());argv.push_back(nullptr);
        // Dispatch workers block several signals. Child cancellation must not
        // inherit that mask or the host's ignored dispositions.
        posix_spawnattr_t attributes;int result=posix_spawnattr_init(&attributes);
        const bool attributesReady=result==0;
        sigset_t mask,defaults;sigemptyset(&mask);sigemptyset(&defaults);
        sigaddset(&defaults,SIGTERM);sigaddset(&defaults,SIGINT);sigaddset(&defaults,SIGPIPE);
        if(!result)result=posix_spawnattr_setsigmask(&attributes,&mask);
        if(!result)result=posix_spawnattr_setsigdefault(&attributes,&defaults);
        if(!result)result=posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETSIGMASK|POSIX_SPAWN_SETSIGDEF);
        pid_t child=-1;if(!result)result=posix_spawn(&child,args[0].c_str(),&actions,&attributes,argv.data(),environ);
        if(attributesReady)posix_spawnattr_destroy(&attributes);
        posix_spawn_file_actions_destroy(&actions);::close(out[1]);::close(err[1]);
        if(result){::close(out[0]);::close(err[0]);throw std::runtime_error("external Bink FFmpeg launch failed");}
        pid_=child;output_=out[0];errors_=err[0];eof_=false;errorText_.clear();
        if(fcntl(output_,F_SETFL,fcntl(output_,F_GETFL)|O_NONBLOCK)<0||
           fcntl(errors_,F_SETFL,fcntl(errors_,F_GETFL)|O_NONBLOCK)<0){
            close();throw std::runtime_error("external Bink pipe cannot be made nonblocking");
        }
    }
public:
    bool nextFrame(video_lab::Frame& frame,const std::function<bool()>& cancelled) {
        if(eof_)return false;
        if(next_==info_.frames){
            uint8_t byte=0;
            if(readExact(&byte,1,true,cancelled))throw std::runtime_error("Bink decoded bytes exceed declared frame count");
            eof_=true;return false;
        }
        if(pendingVideo_.bgra.empty()){pendingVideo_.pts=next_;pendingVideo_.bgra.resize(info_.frameBytes());}
        readExact(pendingVideo_.bgra.data(),pendingVideo_.bgra.size(),false,cancelled);
        ++next_;frame=std::move(pendingVideo_);received_=0;return true;
    }
    void close() noexcept {
        if(output_>=0)::close(output_);if(errors_>=0)::close(errors_);output_=errors_=-1;
        if(pid_>0){
            bool requested=false;
            for(;;){
                int status=0;pid_t result;
                do{result=waitpid(pid_,&status,WNOHANG);}while(result<0&&errno==EINTR);
                if(result==pid_ || (result<0&&errno==ECHILD)){pid_=-1;break;}
                if(result==0){
                    // Preserve ownership until reaped. Unexpected wait failures
                    // retain PID/active state; the owner's deadline refuses exit.
                    if(!requested){kill(pid_,SIGTERM);requested=true;}
                    do{result=waitpid(pid_,&status,0);}while(result<0&&errno==EINTR);
                    if(result==pid_ || (result<0&&errno==ECHILD)){pid_=-1;break;}
                }
                usleep(1000);
            }
        }
        pendingVideo_={};std::vector<int16_t>().swap(pendingPCM_);received_=0;
    }
    bool active() const{return pid_>0;}
    BinkInfo info() const{return info_;}
private:
    using Clock=std::chrono::steady_clock;
    pid_t pid_=-1;int output_=-1,errors_=-1;BinkInfo info_{};uint64_t next_=0;bool eof_=false,audioMode_=false;
    // A cancelled read retains its prefix and cursor in process-owned storage.
    // A caller may supply a different output object after Pause/Resume.
    video_lab::Frame pendingVideo_;std::vector<int16_t> pendingPCM_;size_t received_=0;
    std::string errorText_;
    void drainErrors(){
        char text[1024];
        for(size_t attempts=0;attempts<16;++attempts){const auto n=read(errors_,text,sizeof(text));if(n<=0)break;
            if(errorText_.size()<8192)errorText_.append(text,std::min(size_t(n),8192-errorText_.size()));}
    }
    bool readExact(uint8_t* bytes,size_t size,bool allowEof,const std::function<bool()>& cancelled){
        return readBytes(bytes,size,allowEof,false,cancelled,received_)!=0;
    }
    size_t readBytes(uint8_t* bytes,size_t size,bool allowEof,bool partial,const std::function<bool()>& cancelled,size_t& received){
        if(received>size)throw std::runtime_error("Bink read continuation layout changed");
        const auto deadline=Clock::now()+std::chrono::seconds(5);
        for(;;){
            if(cancelled&&cancelled())throw BinkReadCancelled();
            if(Clock::now()>=deadline)throw std::runtime_error("external Bink frame read timeout (5000 ms)");
            drainErrors();const auto n=read(output_,bytes+received,size-received);
            if(n>0){received+=size_t(n);if(received==size)return received;continue;}
            if(n==0){
                int status=0;pid_t done;
                do{if(cancelled&&cancelled())throw BinkReadCancelled();
                    drainErrors();done=waitpid(pid_,&status,WNOHANG);
                    if(done==0){if(Clock::now()>=deadline)throw std::runtime_error("external Bink decoder exit timeout");usleep(1000);}}
                while(done==0||(done<0&&errno==EINTR));
                if(done==pid_ || (done<0&&errno==ECHILD))pid_=-1;
                drainErrors();
                if(done<0||!WIFEXITED(status)||WEXITSTATUS(status)!=0)
                    throw std::runtime_error("external Bink decoder failed: "+errorText_);
                if(allowEof&&(received==0||partial)){eof_=true;return received;}
                throw std::runtime_error("external Bink decoder returned premature/partial frame");
            }
            if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
                throw std::runtime_error("external Bink decoder pipe read failed");
            pollfd fds[2]={{output_,POLLIN,0},{errors_,POLLIN,0}};
            const int result=poll(fds,2,50);
            if(result<0&&errno!=EINTR)throw std::runtime_error("external Bink decoder poll failed");
        }
    }
};
} // namespace stream_lab
