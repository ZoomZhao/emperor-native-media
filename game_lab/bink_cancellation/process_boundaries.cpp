#include "stream_lab/bink_process.hpp"
#include <iostream>
#include <thread>
#include <fstream>
#include <pthread.h>
using Clock=std::chrono::steady_clock;
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
void mode(const char* value){need(setenv("REVIEW_CHILD_MODE",value,1)==0,"own mode environment");}
template<class F>void rejects(F f,const char* text){bool caught=false;try{f();}catch(const std::exception& e){caught=std::string(e.what()).find(text)!=std::string::npos;}need(caught,text);}
pid_t childPid(const std::string& path){for(unsigned i=0;i<1000;++i){std::ifstream f(path);pid_t p=-1;if(f>>p)return p;std::this_thread::sleep_for(std::chrono::milliseconds(1));}throw std::runtime_error("own child marker missing");}
int main(int argc,char** argv){try{need(argc==3,"fixture and child marker required");std::string path=argv[1],marker=argv[2];
 mode("stderr");{stream_lab::BinkProcess p;p.open(path,0);video_lab::Frame f;unsigned n=0;while(p.nextFrame(f,{}))++n;need(n==30&&!p.active(),"stderr saturation blocked valid output/reap");}
 std::cout<<"stderr_1MiB_then_valid_video=PASS\n";
 mode("partial_video");{stream_lab::BinkProcess p;p.open(path,0);video_lab::Frame f;rejects([&]{while(p.nextFrame(f,{})){};},"premature/partial frame");p.close();need(!p.active(),"partial frame child not reaped");}
 std::cout<<"partial_video_rejected=PASS\n";
 mode("bad_exit");{stream_lab::BinkProcess p;p.openAudio(path,0);std::vector<int16_t> pcm;rejects([&]{p.nextAudio(pcm,{});},"decoder failed");p.close();need(!p.active(),"failed exit child not reaped");}
 std::cout<<"nonzero_audio_exit_rejected=PASS\n";
 mode("partial_pcm");{stream_lab::BinkProcess p;p.openAudio(path,0);std::vector<int16_t> pcm;rejects([&]{p.nextAudio(pcm,{});},"partial PCM sample frame");p.close();need(!p.active(),"partial PCM child not reaped");}
 std::cout<<"partial_PCM_sample_frame_rejected=PASS\n";
 mode("blocked");std::filesystem::remove(marker);need(setenv("REVIEW_CHILD_PID",marker.c_str(),1)==0,"own marker environment");
 sigset_t blocked,old;sigemptyset(&blocked);sigaddset(&blocked,SIGTERM);sigaddset(&blocked,SIGINT);sigaddset(&blocked,SIGPIPE);need(pthread_sigmask(SIG_BLOCK,&blocked,&old)==0,"block reviewer thread signals");
 {stream_lab::BinkProcess p;p.openAudio(path,0);auto pid=childPid(marker);auto start=Clock::now();std::vector<int16_t> pcm;
 rejects([&]{p.nextAudio(pcm,[&]{return Clock::now()-start>=std::chrono::milliseconds(100);});},"decode cancelled");p.close();
 need(!p.active()&&kill(pid,0)<0&&errno==ESRCH,"cancelled child still exists");need(Clock::now()-start<std::chrono::seconds(1),"cancel/reap exceeded one second");}
 need(pthread_sigmask(SIG_SETMASK,&old,nullptr)==0,"restore reviewer thread signals");
 std::cout<<"blocked_read_cancel_with_parent_signal_mask_reaped=PASS\n";
 mode("backpressure");std::filesystem::remove(marker);{stream_lab::BinkProcess p;p.open(path,0);auto pid=childPid(marker);std::this_thread::sleep_for(std::chrono::milliseconds(100));auto start=Clock::now();p.close();need(!p.active()&&kill(pid,0)<0&&errno==ESRCH,"backpressure child still exists");need(Clock::now()-start<std::chrono::seconds(1),"backpressure close exceeded one second");}
 std::cout<<"stdout_backpressure_close_reaped=PASS\n";
 std::cout<<"{\"scenarios\":6,\"passed\":6,\"UI\":false,\"audio_devices\":false,\"original_data\":false}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
