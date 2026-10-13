#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
volatile sig_atomic_t stopRequested=0;
void stop(int){stopRequested=1;}
unsigned word(const unsigned char* p){return unsigned(p[0])|unsigned(p[1])<<8|unsigned(p[2])<<16|unsigned(p[3])<<24;}
int main(int argc,char** argv){
    signal(SIGTERM,stop);std::string path;
    sigset_t inherited{};sigprocmask(SIG_BLOCK,nullptr,&inherited);
    if(const char* file=getenv("PROBE_SIGNAL_LOG")){
        std::ofstream log(file);log<<"{\"pid\":"<<getpid()<<",\"parent_pid\":"<<getppid()<<",\"term_blocked\":"<<sigismember(&inherited,SIGTERM)
            <<",\"pipe_blocked\":"<<sigismember(&inherited,SIGPIPE)<<",\"int_blocked\":"<<sigismember(&inherited,SIGINT)<<"}\n";
    }
    for(int i=1;i+1<argc;++i)if(std::string(argv[i])=="-i")path=argv[i+1];
    std::array<unsigned char,44> h{};std::ifstream in(path,std::ios::binary);in.read(reinterpret_cast<char*>(h.data()),h.size());
    if(in.gcount()!=44)return 71;
    const size_t count=size_t(word(h.data()+20))*word(h.data()+24)*4;
    if(!count||count>8u*1024u*1024u)return 72;
    std::vector<unsigned char> frame(count,0x50);for(size_t i=3;i<count;i+=4)frame[i]=255;
    size_t sent=0;while(sent<count){const auto n=write(STDOUT_FILENO,frame.data()+sent,count-sent);if(n<=0)return 73;sent+=size_t(n);}
    // Normal cooperative TERM exit, with an independent natural deadline. This
    // authored child neither ignores TERM nor relies on forced termination.
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
    while(!stopRequested&&std::chrono::steady_clock::now()<deadline)usleep(10000);
    return 0;
}
