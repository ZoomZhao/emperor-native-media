#include "video.hpp"
#include "reference.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace video_lab;
namespace {
constexpr uint64_t seed=0x71de05eed2026001ULL;
struct Random{uint64_t state;uint64_t next(){state^=state<<13;state^=state>>7;state^=state<<17;return state;}
    int below(int limit){return int(next()%uint64_t(limit));}};
struct Totals{size_t assertions=0,states=0,decisions=0,bytes=0,negative=0,groups=0,randomEvents=0;
    uint64_t finalRng=0;} totals;
std::string directory,context;
void check(bool value,const std::string& message){++totals.assertions;if(!value)throw std::runtime_error(context+": "+message);}
#define CHECK(value) check((value),std::string(__FILE__)+":"+std::to_string(__LINE__)+": " #value)
Clip colored(int count=12,uint64_t step=3000,uint64_t timebase=90000){Clip c;c.width=3;c.height=2;c.stride=17;
    c.ticksPerSecond=timebase;c.duration=uint64_t(count)*step;c.declaredFrames=uint32_t(count);
    for(int i=0;i<count;++i){Frame f;f.pts=uint64_t(i)*step;f.bgra.assign(c.stride*c.height,uint8_t(0xb0+i%32));
        for(uint32_t y=0;y<c.height;++y)for(uint32_t x=0;x<c.width;++x){const size_t p=size_t(y)*c.stride+x*4;
            f.bgra[p]=uint8_t((17*uint32_t(i)+5*x)%256);f.bgra[p+1]=uint8_t((31*uint32_t(i)+11*y)%256);
            f.bgra[p+2]=uint8_t((uint32_t(i)+13*x+7*y)%256);f.bgra[p+3]=255;}c.frames.push_back(std::move(f));}return c;}
bool clipEqual(const Clip& a,const Clip& b){if(a.width!=b.width||a.height!=b.height||a.stride!=b.stride||
    a.ticksPerSecond!=b.ticksPerSecond||a.duration!=b.duration||a.declaredFrames!=b.declaredFrames||a.frames.size()!=b.frames.size())return false;
    for(size_t i=0;i<a.frames.size();++i)if(a.frames[i].pts!=b.frames[i].pts||a.frames[i].bgra!=b.frames[i].bgra)return false;return true;}
bool countEqual(Counters a,Counters b){return a.advances==b.advances&&a.presented==b.presented&&a.repeated==b.repeated&&
    a.held==b.held&&a.dropped==b.dropped&&a.skippedUnfed==b.skippedUnfed&&a.eofs==b.eofs;}
bool stateEqual(const State& a,const State& b){return a.loaded==b.loaded&&a.clockBound==b.clockBound&&a.eofSent==b.eofSent&&
    clipEqual(a.clip,b.clip)&&a.audioRate==b.audioRate&&a.mode==b.mode&&a.epoch==b.epoch&&a.mediaTicks==b.mediaTicks&&
    a.lastSamples==b.lastSamples&&a.anchorSamples==b.anchorSamples&&a.anchorTicks==b.anchorTicks&&a.nextFeed==b.nextFeed&&
    a.shown==b.shown&&a.queue==b.queue&&countEqual(a.counters,b.counters);}
bool decisionEqual(const Decision& a,const Decision& b){return a.kind==b.kind&&a.epoch==b.epoch&&a.mediaTicks==b.mediaTicks&&
    a.pts==b.pts&&a.index==b.index&&a.bgra==b.bgra;}
void writeBytes(const std::string& name,const std::vector<uint8_t>& bytes){std::ofstream out(directory+"/"+name,std::ios::binary);CHECK(bool(out));
    out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));CHECK(bool(out));}
std::string stateText(const State& s){std::ostringstream out;out<<"{\"loaded\":"<<(s.loaded?"true":"false")<<",\"mode\":"<<int(s.mode)
    <<",\"epoch\":"<<s.epoch<<",\"media_ticks\":"<<s.mediaTicks<<",\"clock_bound\":"<<(s.clockBound?"true":"false")
    <<",\"last_samples\":"<<s.lastSamples<<",\"anchor_samples\":"<<s.anchorSamples<<",\"anchor_ticks\":"<<s.anchorTicks
    <<",\"next_feed\":"<<s.nextFeed<<",\"shown\":"<<s.shown<<",\"queue\":[";
    for(size_t i=0;i<s.queue.size();++i){if(i)out<<",";out<<s.queue[i];}out<<"],\"counters\":["<<s.counters.advances<<","<<s.counters.presented
        <<","<<s.counters.repeated<<","<<s.counters.held<<","<<s.counters.dropped<<","<<s.counters.skippedUnfed<<","<<s.counters.eofs<<"]}";return out.str();}
struct Pair{
    Player actual;scalar_reference::Reference model;Status last=Status::Ok;
    explicit Pair(uint32_t rate=22050):actual(rate),model(rate){compare();}
    void compare(){const auto a=actual.state(),b=model.state();++totals.states;
        if(!stateEqual(a,b)){std::ofstream(directory+"/failure_actual_state.json")<<stateText(a);std::ofstream(directory+"/failure_expected_state.json")<<stateText(b);}
        CHECK(stateEqual(a,b));CHECK(a.queue.size()<=4);CHECK(std::is_sorted(a.queue.begin(),a.queue.end()));
        if(a.loaded){CHECK(a.mediaTicks<=a.clip.duration);CHECK(a.nextFeed>=0&&size_t(a.nextFeed)<=a.clip.frames.size());
            CHECK(a.shown<0||size_t(a.shown)<a.clip.frames.size());}
    }
    Status done(Status a,Status b,const State& before){check(a==b,std::string(statusName(a))+" != reference "+statusName(b));last=a;
        if(a!=Status::Ok){++totals.negative;CHECK(stateEqual(before,actual.state()));}compare();return a;}
    Status load(const Clip& c){const auto before=actual.state();const auto a=actual.load(c),b=model.load(c);return done(a,b,before);}
    uint64_t epoch() const{return model.state().epoch;}
    Status enqueue(int64_t index,uint64_t e){const auto before=actual.state();return done(actual.enqueue(index,e),model.enqueue(index,e),before);}
    Status feed(int64_t index){return enqueue(index,epoch());}
    Status start(uint64_t samples=0){const auto before=actual.state();Clock c{epoch(),samples,model.state().audioRate};return done(actual.start(c),model.start(c),before);}
    Status rawStart(Clock c){const auto before=actual.state();return done(actual.start(c),model.start(c),before);}
    Decision tick(Clock c){const auto before=actual.state();const Decision sentinel{Kind::Hold,999,77,33,7,{1,2,3}};
        Decision a=sentinel,b=sentinel;const auto sa=actual.advance(c,a),sb=model.advance(c,b);
        if(!decisionEqual(a,b)){writeBytes("failure_actual.bgra",a.bgra);writeBytes("failure_expected.bgra",b.bgra);}
        CHECK(decisionEqual(a,b));if(sa!=Status::Ok)CHECK(decisionEqual(a,sentinel));done(sa,sb,before);
        if(sa==Status::Ok){++totals.decisions;totals.bytes+=b.bgra.size();}return a;}
    Decision tick(uint64_t samples){return tick({epoch(),samples,model.state().audioRate});}
    template<typename A,typename B>Status op(A a,B b){const auto before=actual.state();return done((actual.*a)(),(model.*b)(),before);}
    Status pause(){return op(&Player::pause,&scalar_reference::Reference::pause);}Status resume(){return op(&Player::resume,&scalar_reference::Reference::resume);}
    Status stop(){return op(&Player::stop,&scalar_reference::Reference::stop);}
    Status seek(int64_t ticks){const auto before=actual.state();return done(actual.seek(ticks),model.seek(ticks),before);}
};

void exactTimeline(){Pair p;const auto clip=colored(120);CHECK(p.load(clip)==Status::Ok);
    for(int i=0;i<4;++i)CHECK(p.feed(i)==Status::Ok);CHECK(p.start()==Status::Ok);
    for(int i=0;i<120;++i){const auto out=p.tick(uint64_t(i)*735);CHECK(out.kind==Kind::Presented&&out.index==i&&out.mediaTicks==uint64_t(i)*3000);
        CHECK(out.bgra==clip.frames[size_t(i)].bgra);const auto s=p.model.state();if(s.nextFeed<120)CHECK(p.feed(s.nextFeed)==Status::Ok);}
    CHECK(p.tick(88200).kind==Kind::Eof);const auto s=p.actual.state();CHECK(s.mode==Mode::Ended&&s.counters.presented==120&&s.counters.dropped==0&&s.counters.eofs==1);
    p.tick(88200);CHECK(p.last==Status::InvalidState&&p.actual.state().counters.eofs==1);
}
void integerFractions(){Pair p(8000);CHECK(p.load(colored(3,1,3))==Status::Ok);for(int i=0;i<3;++i)CHECK(p.feed(i)==Status::Ok);
    CHECK(p.start()==Status::Ok);CHECK(p.tick(0).index==0);CHECK(p.tick(2666).index==0);CHECK(p.actual.state().mediaTicks==0);
    CHECK(p.tick(2667).index==1&&p.actual.state().mediaTicks==1);CHECK(p.tick(5333).index==1);CHECK(p.tick(5334).index==2);
    CHECK(p.tick(8000).kind==Kind::Eof);
}
void pressureDropRepeat(){Pair p;CHECK(p.load(colored())==Status::Ok);for(int i=0;i<4;++i)CHECK(p.feed(i)==Status::Ok);
    CHECK(p.feed(4)==Status::Backpressure);CHECK(p.feed(2)==Status::OutOfOrder);CHECK(p.start()==Status::Ok);
    CHECK(p.tick(0).kind==Kind::Presented);CHECK(p.feed(4)==Status::Ok);
    const auto held=p.tick(5*735);CHECK(held.kind==Kind::Hold&&held.index==0);CHECK(p.actual.state().counters.dropped==4);
    CHECK(p.feed(5)==Status::Ok);CHECK(p.tick(5*735).kind==Kind::Presented);CHECK(p.tick(5*735).kind==Kind::Repeat);
    for(int i=6;i<10;++i)CHECK(p.feed(i)==Status::Ok);const auto late=p.tick(9*735);CHECK(late.kind==Kind::Presented&&late.index==9);
    CHECK(p.actual.state().counters.dropped==7&&p.actual.state().counters.repeated==1);
    Pair missing;CHECK(missing.load(colored())==Status::Ok);CHECK(missing.start()==Status::Ok);CHECK(missing.tick(6*735).kind==Kind::None);
    CHECK(missing.actual.state().nextFeed==6&&missing.actual.state().counters.skippedUnfed==6);
    CHECK(missing.feed(6)==Status::Ok);CHECK(missing.tick(6*735).index==6);
}
void pauseResume(){Pair p;CHECK(p.load(colored())==Status::Ok);for(int i=0;i<4;++i)CHECK(p.feed(i)==Status::Ok);
    CHECK(p.start(100)==Status::Ok);CHECK(p.tick(100).index==0);CHECK(p.tick(835).index==1);CHECK(p.pause()==Status::Ok);
    const auto before=p.actual.state();const auto out=p.tick(5000);CHECK(out.kind==Kind::Paused&&out.index==1&&out.mediaTicks==3000);
    CHECK(p.actual.state().queue==before.queue&&p.actual.state().counters.presented==before.counters.presented);
    CHECK(p.resume()==Status::Ok);CHECK(p.tick(5735).index==2&&p.actual.state().mediaTicks==6000);CHECK(p.tick(5735).kind==Kind::Repeat);
}
void seekEpochStop(){Pair p;CHECK(p.load(colored(120))==Status::Ok);for(int i=0;i<4;++i)CHECK(p.feed(i)==Status::Ok);
    CHECK(p.start()==Status::Ok);CHECK(p.tick(735).index==1);const auto old=p.epoch();CHECK(p.seek(180000)==Status::Ok);
    CHECK(p.epoch()==old+1&&p.actual.state().queue.empty()&&p.actual.state().shown==-1&&p.actual.state().nextFeed==60);
    CHECK(p.enqueue(2,old)==Status::StaleEpoch);p.tick({old,735,22050});CHECK(p.last==Status::StaleEpoch);
    CHECK(p.feed(60)==Status::Ok);const auto out=p.tick(0);CHECK(out.index==60&&out.mediaTicks==180000&&out.kind==Kind::Presented);
    CHECK(out.bgra[0]==252&&out.bgra[1]==68&&out.bgra[2]==60&&out.bgra[3]==255);
    CHECK(p.tick(735).kind==Kind::Hold);CHECK(p.feed(61)==Status::Ok);CHECK(p.tick(735).index==61);
    CHECK(p.seek(-1)==Status::InvalidSeek);CHECK(p.seek(360000)==Status::InvalidSeek);
    const auto prior=p.epoch();CHECK(p.stop()==Status::Ok);CHECK(p.actual.state().mediaTicks==0&&p.actual.state().mode==Mode::Stopped&&p.actual.state().shown==-1);
    CHECK(p.rawStart({prior,0,22050})==Status::StaleEpoch);CHECK(p.start()==Status::Ok);CHECK(p.enqueue(0,prior)==Status::StaleEpoch);
    CHECK(p.feed(0)==Status::Ok);CHECK(p.tick(0).index==0);
}
void metadataNegatives(){Pair p;const auto good=colored(3);CHECK(p.load(good)==Status::Ok);
    std::vector<Clip> bad;bad.push_back(Clip{});
    auto c=good;c.frames.pop_back();bad.push_back(c);c=good;c.frames.clear();bad.push_back(c);
    c=good;c.declaredFrames=0;bad.push_back(c);c=good;c.declaredFrames=257;bad.push_back(c);
    c=good;c.frames[1].bgra.pop_back();bad.push_back(c);c=good;c.frames[2].bgra.push_back(0);bad.push_back(c);
    c=good;c.width=0;bad.push_back(c);c=good;c.height=65;bad.push_back(c);c=good;c.stride=11;bad.push_back(c);
    c=good;c.stride=std::numeric_limits<size_t>::max();bad.push_back(c);c=good;c.ticksPerSecond=0;bad.push_back(c);
    c=good;c.ticksPerSecond=1000001;bad.push_back(c);c=good;c.duration=0;bad.push_back(c);
    c=good;c.frames[0].pts=1;bad.push_back(c);c=good;c.frames[1].pts=0;bad.push_back(c);
    c=good;c.frames[2].pts=2999;bad.push_back(c);c=good;c.frames[2].pts=c.duration;bad.push_back(c);
    c=good;c.frames[2].pts=std::numeric_limits<uint64_t>::max();bad.push_back(c);
    for(const auto& input:bad)CHECK(p.load(input)==Status::InvalidMetadata);
    auto mutableInput=good;Pair own;CHECK(own.load(mutableInput)==Status::Ok);mutableInput.frames[0].bgra.assign(34,255);
    CHECK(own.feed(0)==Status::Ok);CHECK(own.start()==Status::Ok);const auto out=own.tick(0);CHECK(out.bgra==good.frames[0].bgra);
}
void clocksAndShortEof(){Pair absent;CHECK(absent.stop()==Status::NoClip);CHECK(absent.pause()==Status::NoClip);absent.tick(0);CHECK(absent.last==Status::NoClip);
    Pair p;CHECK(p.load(colored(1))==Status::Ok);CHECK(p.feed(-1)==Status::InvalidFrame);CHECK(p.feed(1)==Status::InvalidFrame);
    CHECK(p.rawStart({p.epoch(),0,44100})==Status::InvalidClockRate);CHECK(p.feed(0)==Status::Ok);CHECK(p.start()==Status::Ok);
    CHECK(p.tick(0).kind==Kind::Presented);p.tick({p.epoch(),1,0});CHECK(p.last==Status::InvalidClockRate);
    CHECK(p.tick(734).kind==Kind::Repeat);p.tick(733);CHECK(p.last==Status::ClockRegression);
    p.tick(std::numeric_limits<uint64_t>::max());CHECK(p.last==Status::ClockOverflow);
    CHECK(p.tick(735).kind==Kind::Eof);p.tick(735);CHECK(p.last==Status::InvalidState&&p.actual.state().counters.eofs==1);
    CHECK(p.seek(0)==Status::Ok);CHECK(p.feed(0)==Status::Ok);CHECK(p.start()==Status::Ok);CHECK(p.tick(735).kind==Kind::Eof);
    CHECK(p.actual.state().counters.eofs==2);bool rejected=false;try{Player invalid(0);}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
}
void randomEvents(){Pair p;Random rng{seed};std::ofstream trace(directory+"/random_trace.jsonl");CHECK(bool(trace));
    for(int step=0;step<4096;++step){context="seed 0x71de05eed2026001 step "+std::to_string(step);const int op=rng.below(12);const auto s=p.model.state();
        trace<<"{\"step\":"<<step<<",\"op\":"<<op;
        if(op==0){trace<<"}\n";trace.flush();p.load(colored());}
        else if(op==1){const uint64_t n=uint64_t(rng.below(201));trace<<",\"samples\":"<<n<<"}\n";trace.flush();p.start(n);}
        else if(op==2){uint64_t n=s.clockBound?s.lastSamples+uint64_t(rng.below(2206)):uint64_t(rng.below(301));
            if(s.lastSamples&&rng.below(8)==0)n=s.lastSamples-1;
            const auto e=rng.below(6)==0?s.epoch-1:s.epoch;const uint32_t rate=rng.below(5)==0?44100u:22050u;
            trace<<",\"samples\":"<<n<<",\"epoch\":"<<e<<",\"rate\":"<<rate<<"}\n";trace.flush();p.tick({e,n,rate});}
        else if(op==3){int index=s.nextFeed;if(rng.below(4)==0)index+=rng.below(3)-1;const auto e=rng.below(5)==0?s.epoch-1:s.epoch;
            trace<<",\"index\":"<<index<<",\"epoch\":"<<e<<"}\n";trace.flush();p.enqueue(index,e);}
        else if(op==6){const int64_t target=int64_t(rng.below(40001))-2000;trace<<",\"seek_ticks\":"<<target<<"}\n";trace.flush();p.seek(target);}
        else{trace<<"}\n";trace.flush();switch(op){case 4:p.pause();break;case 5:p.resume();break;case 7:p.stop();break;
            case 8:{auto bad=colored(3);bad.frames.pop_back();p.load(bad);break;}
            case 9:p.tick(std::numeric_limits<uint64_t>::max());break;
            case 10:for(int i=0;i<5;++i)p.feed(p.model.state().nextFeed);break;
            case 11:p.tick(s.lastSamples);break;default:CHECK(false);}}
        ++totals.randomEvents;
    }
    totals.finalRng=rng.state;
}
void golden(){Pair p;const auto clip=colored(6);CHECK(p.load(clip)==Status::Ok);for(int i=0;i<4;++i)CHECK(p.feed(i)==Status::Ok);
    CHECK(p.start()==Status::Ok);std::vector<int> actual,expected{0,1,1,1,4,4,4};std::vector<uint8_t> actualBytes,expectedBytes;
    std::vector<int> kinds;const std::vector<uint64_t> samples{0,735,735,2940,2940,3675,4410};
    for(size_t i=0;i<samples.size();++i){if(i==4)CHECK(p.feed(4)==Status::Ok);const auto out=p.tick(samples[i]);actual.push_back(out.index);kinds.push_back(int(out.kind));
        actualBytes.insert(actualBytes.end(),out.bgra.begin(),out.bgra.end());const auto& bytes=clip.frames[size_t(expected[i])].bgra;expectedBytes.insert(expectedBytes.end(),bytes.begin(),bytes.end());}
    CHECK(actual==expected);CHECK(kinds==std::vector<int>({1,1,2,3,1,3,5}));CHECK(actualBytes==expectedBytes);
    writeBytes("golden_expected.bgra",expectedBytes);writeBytes("golden_actual.bgra",actualBytes);writeBytes("golden_frame4_source.bgra",clip.frames[4].bgra);
    std::ofstream meta(directory+"/golden.json");CHECK(bool(meta));meta<<"{\"timebase\":90000,\"audio_rate\":22050,\"clock_audio_frames\":[0,735,735,2940,2940,3675,4410],"
        <<"\"indices\":[0,1,1,1,4,4,4],\"kinds\":[1,1,2,3,1,3,5],\"kind_names\":[\"present\",\"present\",\"repeat\",\"hold\",\"present\",\"hold\",\"EOF\"],"
        <<"\"width\":3,\"height\":2,\"stride\":17,\"concatenated_bytes\":238,\"byte_mismatches\":0}\n";
    std::ofstream(directory+"/golden_state.json")<<stateText(p.actual.state());CHECK(bool(meta));
}
std::string escape(const std::string& text){std::string out;for(char c:text){if(c=='"'||c=='\\')out+='\\';if(c=='\n')out+="\\n";else out+=c;}return out;}
void summary(bool ok,const std::string& error,double ms){std::ofstream out(directory+"/summary.json");if(!out)throw std::runtime_error("summary open");
    out<<"{\n  \"passed\":"<<(ok?"true":"false")<<",\n  \"scope\":\"self-authored offline raw-frame queue / integer PTS scheduler\",\n"
        <<"  \"groups_passed\":"<<totals.groups<<",\n  \"assertions\":"<<totals.assertions<<",\n  \"state_comparisons\":"<<totals.states
        <<",\n  \"decisions_compared\":"<<totals.decisions<<",\n  \"frame_bytes_compared\":"<<totals.bytes
        <<",\n  \"negative_returns_checked\":"<<totals.negative<<",\n  \"random_events\":"<<totals.randomEvents
        <<",\n  \"random_seed_hex\":\"0x71de05eed2026001\",\n  \"random_final_hex\":\"0x"<<std::hex<<totals.finalRng<<"\",\n"<<std::dec
        <<"  \"elapsed_ms\":"<<ms<<",\n  \"error\":\""<<escape(error)<<"\",\n  \"decoder_or_Bink_used\":false,\n"
        <<"  \"real_AV_sync_or_device_verified\":false,\n  \"GPU_UI_network_used\":false,\n  \"game_assets_or_code_loaded\":false,\n  \"ASAN_attempted\":false\n}\n";
    if(!out)throw std::runtime_error("summary write");}
}
int main(int argc,char** argv){if(argc!=2){std::cerr<<"usage: video-tests EXISTING_EVIDENCE_DIRECTORY\n";return 2;}directory=argv[1];const auto start=std::chrono::steady_clock::now();
    const std::vector<std::pair<std::string,std::function<void()>>> groups{{"120-frame integer timeline and EOF",exactTimeline},
        {"non-divisible integer clock fractions",integerFractions},{"backpressure drop repeat and starvation hold",pressureDropRepeat},
        {"pause resume clock rebasing",pauseResume},{"seek epoch isolation and stop",seekEpochStop},{"short empty corrupt metadata and ownership",metadataNegatives},
        {"clock errors and one-frame EOF regression",clocksAndShortEof},{"4096 deterministic oracle events",randomEvents},{"persisted hand-written decision golden",golden}};
    try{for(const auto& group:groups){context=group.first;const size_t before=totals.assertions;group.second();++totals.groups;
        std::cout<<"PASS "<<group.first<<" ("<<totals.assertions-before<<" assertions)\n";}
        summary(true,"",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
        std::cout<<"PASS all: "<<totals.groups<<" groups; "<<totals.bytes<<" frame bytes compared; offline scheduling only\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<"\n";std::ofstream(directory+"/failure_context.txt")<<context<<"\n"<<error.what()<<"\n";
        try{summary(false,error.what(),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}catch(const std::exception& e){std::cerr<<e.what()<<"\n";}return 1;}}
