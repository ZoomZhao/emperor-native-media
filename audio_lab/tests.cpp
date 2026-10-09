#include "audio.hpp"
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

using namespace audio_lab;
namespace {
constexpr uint64_t seed=0xa0d105eed2026001ULL;
struct Random {uint64_t state;uint64_t next(){state^=state<<13;state^=state>>7;state^=state<<17;return state;}
    int below(int limit){return int(next()%uint64_t(limit));}};
struct Totals {size_t assertions=0,stateComparisons=0,samplesCompared=0,pullCalls=0,negativeReturns=0,groups=0,randomEvents=0;
    uint64_t finalRng=0;} totals;
std::string directory,context;
void check(bool value,const std::string& message){++totals.assertions;if(!value)throw std::runtime_error(context+": "+message);}
#define CHECK(value) check((value),std::string(__FILE__)+":"+std::to_string(__LINE__)+": " #value)
bool eventEqual(const Event& a,const Event& b){return a.type==b.type&&a.ticket==b.ticket&&a.handle==b.handle&&
    a.deliveredFrames==b.deliveredFrames&&a.device==b.device;}
bool eventsEqual(const std::vector<Event>& a,const std::vector<Event>& b){
    return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),eventEqual);}
bool countersEqual(Counters a,Counters b){return a.outputFrames==b.outputFrames&&a.sourceFrames==b.sourceFrames&&
    a.underrunFrames==b.underrunFrames&&a.blockedFrames==b.blockedFrames;}
bool stateEqual(const State& a,const State& b){
    if(a.format.rate!=b.format.rate||a.format.channels!=b.format.channels||a.playback!=b.playback||
        a.deviceReady!=b.deviceReady||a.device!=b.device||a.nextTicket!=b.nextTicket||
        !countersEqual(a.counters,b.counters)||a.queue.size()!=b.queue.size()||!eventsEqual(a.events,b.events))return false;
    for(size_t i=0;i<a.sources.size();++i){const auto& x=a.sources[i];const auto& y=b.sources[i];
        if(x.generation!=y.generation||x.live!=y.live||x.queueReferences!=y.queueReferences||x.samples!=y.samples)return false;}
    for(size_t i=0;i<a.queue.size();++i){const auto& x=a.queue[i];const auto& y=b.queue[i];
        if(x.handle!=y.handle||x.ticket!=y.ticket||x.loops!=y.loops||x.passes!=y.passes||
            x.cursorFrames!=y.cursorFrames||x.deliveredFrames!=y.deliveredFrames)return false;}
    return true;
}
bool pullEqual(const Pull& a,const Pull& b){return a.samples==b.samples&&a.sourceFrames==b.sourceFrames&&
    a.underrunFrames==b.underrunFrames&&a.blockedFrames==b.blockedFrames&&a.guard==b.guard;}
void writePCM(const std::string& name,const std::vector<int16_t>& values){
    std::ofstream out(directory+"/"+name,std::ios::binary);CHECK(bool(out));
    for(int16_t sample:values){const uint16_t word=uint16_t(sample);const char bytes[2]={char(word%256),char(word/256)};out.write(bytes,2);}
    CHECK(bool(out));
}
std::string stateText(const State& s){std::ostringstream out;
    out<<"{\"rate\":"<<s.format.rate<<",\"channels\":"<<s.format.channels<<",\"mode\":"<<int(s.playback)
        <<",\"ready\":"<<(s.deviceReady?"true":"false")<<",\"device\":"<<s.device<<",\"next_ticket\":"<<s.nextTicket
        <<",\"counters\":["<<s.counters.outputFrames<<","<<s.counters.sourceFrames<<","<<s.counters.underrunFrames<<","<<s.counters.blockedFrames<<"],\"queue\":[";
    for(size_t i=0;i<s.queue.size();++i){if(i)out<<",";const auto& q=s.queue[i];out<<"["<<q.handle<<","<<q.ticket<<","<<q.loops<<","<<q.passes<<","<<q.cursorFrames<<","<<q.deliveredFrames<<"]";}
    out<<"],\"sources\":[";
    for(size_t i=0;i<s.sources.size();++i){if(i)out<<",";const auto& src=s.sources[i];
        out<<"{\"generation\":"<<src.generation<<",\"live\":"<<(src.live?"true":"false")<<",\"refs\":"<<src.queueReferences<<",\"samples\":[";
        for(size_t j=0;j<src.samples.size();++j){if(j)out<<",";out<<src.samples[j];}out<<"]}";}
    out<<"],\"events\":[";
    for(size_t i=0;i<s.events.size();++i){if(i)out<<",";const auto& e=s.events[i];
        out<<"["<<int(e.type)<<","<<e.ticket<<","<<e.handle<<","<<e.deliveredFrames<<","<<e.device<<"]";}out<<"]}";return out.str();}
std::vector<int16_t> tagged(size_t frames,size_t channels,int salt){
    std::vector<int16_t> out;
    for(size_t f=0;f<frames;++f)for(size_t c=0;c<channels;++c){const int magnitude=int(f)*100+int(c)*7+salt;
        out.push_back(int16_t(c%2?-magnitude:magnitude));}return out;
}

struct Pair {
    Player actual;scalar_reference::Reference reference;
    Handle lastHandle=0;uint64_t lastTicket=0;
    explicit Pair(Format f={22050,2}):actual(f),reference(f){compare();}
    void compare(){const auto a=actual.state(),b=reference.state();++totals.stateComparisons;
        if(!stateEqual(a,b)){std::ofstream(directory+"/failure_actual_state.json")<<stateText(a);
            std::ofstream(directory+"/failure_expected_state.json")<<stateText(b);}
        CHECK(stateEqual(a,b));
        CHECK(a.counters.outputFrames==a.counters.sourceFrames+a.counters.underrunFrames+a.counters.blockedFrames);
        size_t refs=0;for(const auto& source:a.sources)refs+=source.queueReferences;CHECK(refs==a.queue.size());
        for(const auto& q:a.queue){const size_t i=size_t(q.handle%4294967296ULL-1);CHECK(i<16);
            const auto& source=a.sources[i];CHECK(source.live&&source.generation==q.handle/4294967296ULL);
            const uint64_t length=source.samples.size()/a.format.channels;
            CHECK(q.cursorFrames<length&&q.deliveredFrames==q.passes*length+q.cursorFrames);
            CHECK(q.loops==0||q.passes<q.loops);}
    }
    Status completed(Status a,Status b){check(a==b,std::string("component status ")+statusName(a)+" != model "+statusName(b));
        if(a!=Status::Ok)++totals.negativeReturns;compare();return a;}
    Status source(Format f,const std::vector<int16_t>& samples){Handle expected=0;lastHandle=0;
        const auto a=actual.createSource(f,samples,lastHandle),b=reference.createSource(f,samples,expected);
        CHECK(lastHandle==expected);return completed(a,b);}
    Handle clip(const std::vector<int16_t>& samples){CHECK(source(actual.state().format,samples)==Status::Ok);return lastHandle;}
    Status release(Handle h){return completed(actual.releaseSource(h),reference.releaseSource(h));}
    Status enqueue(Handle h,int64_t loops){uint64_t expected=0;lastTicket=0;
        const auto a=actual.enqueue(h,loops,lastTicket),b=reference.enqueue(h,loops,expected);CHECK(lastTicket==expected);return completed(a,b);}
    Pull consume(size_t frames){Pull a{{123,-123},9,8,7,Guard::Paused},b=a;
        const auto sa=actual.consume(frames,a),sb=reference.consume(frames,b);
        if(!pullEqual(a,b)){writePCM("failure_actual.s16le",a.samples);writePCM("failure_expected.s16le",b.samples);}
        CHECK(pullEqual(a,b));completed(sa,sb);
        if(sa==Status::Ok){++totals.pullCalls;totals.samplesCompared+=b.samples.size();
            CHECK(a.samples.size()==frames*actual.state().format.channels);
            CHECK(a.sourceFrames+a.underrunFrames+a.blockedFrames==frames);}
        return a;
    }
    template<typename A,typename B>Status op(A a,B b){return completed((actual.*a)(),(reference.*b)());}
    Status start(){return op(&Player::start,&scalar_reference::Reference::start);}
    Status pause(){return op(&Player::pause,&scalar_reference::Reference::pause);}
    Status resume(){return op(&Player::resume,&scalar_reference::Reference::resume);}
    Status stop(){return op(&Player::stop,&scalar_reference::Reference::stop);}
    Status flush(){return op(&Player::flush,&scalar_reference::Reference::flush);}
    Status loss(){return op(&Player::simulateLoss,&scalar_reference::Reference::simulateLoss);}
    Status rebuild(uint64_t id,Format f,bool available=true){return completed(actual.simulateRebuild(id,f,available),reference.simulateRebuild(id,f,available));}
    std::vector<Event> poll(){auto a=actual.pollEvents(),b=reference.pollEvents();CHECK(eventsEqual(a,b));compare();return a;}
};

void fifoBoundaries(){Pair p;const auto aBytes=tagged(3,2,10),bBytes=tagged(5,2,20);
    const Handle a=p.clip(aBytes),b=p.clip(bBytes);CHECK(p.enqueue(a,1)==Status::Ok);const uint64_t ta=p.lastTicket;
    CHECK(p.enqueue(b,1)==Status::Ok);const uint64_t tb=p.lastTicket;CHECK(p.release(a)==Status::Busy);CHECK(p.start()==Status::Ok);
    const auto first=p.consume(2);CHECK(first.samples==std::vector<int16_t>(aBytes.begin(),aBytes.begin()+4));
    const auto middle=p.consume(4);std::vector<int16_t> expected(aBytes.begin()+4,aBytes.end());expected.insert(expected.end(),bBytes.begin(),bBytes.begin()+6);
    CHECK(middle.samples==expected&&middle.sourceFrames==4&&middle.underrunFrames==0);
    const auto e=p.poll();CHECK(e.size()==1&&e[0].type==EventType::Completed&&e[0].ticket==ta&&e[0].deliveredFrames==3);
    CHECK(p.release(a)==Status::Ok);const auto last=p.consume(3);expected.assign(bBytes.begin()+6,bBytes.end());expected.insert(expected.end(),2,0);
    CHECK(last.samples==expected&&last.sourceFrames==2&&last.underrunFrames==1);
    const auto done=p.poll();CHECK(done.size()==1&&done[0].ticket==tb&&done[0].deliveredFrames==5);
    const auto stats=p.actual.state().counters;CHECK(stats.outputFrames==9&&stats.sourceFrames==8&&stats.underrunFrames==1&&stats.blockedFrames==0);
}
void channelAccounting(){for(uint32_t channels:{1u,2u,4u,6u,8u}){Pair p({48000,channels});auto bytes=tagged(3,channels,1);
        bytes[0]=std::numeric_limits<int16_t>::min();bytes.back()=std::numeric_limits<int16_t>::max();
        const Handle h=p.clip(bytes);CHECK(p.enqueue(h,1)==Status::Ok);CHECK(p.start()==Status::Ok);const auto result=p.consume(5);
        auto expected=bytes;expected.insert(expected.end(),2*channels,0);CHECK(result.samples==expected);
        CHECK(result.sourceFrames==3&&result.underrunFrames==2&&result.samples.size()==5*channels);
        const auto event=p.poll();CHECK(event.size()==1&&event[0].deliveredFrames==3);}
}
void pauseStopFlush(){Pair p;const auto bytes=tagged(11,2,3);const Handle h=p.clip(bytes);
    CHECK(p.enqueue(h,1)==Status::Ok);CHECK(p.start()==Status::Ok);p.consume(3);const auto progress=p.actual.state().queue;
    CHECK(p.pause()==Status::Ok);const auto silent=p.consume(4);CHECK(silent.guard==Guard::Paused&&silent.blockedFrames==4&&silent.underrunFrames==0);
    CHECK(p.actual.state().queue[0].deliveredFrames==progress[0].deliveredFrames);
    CHECK(p.resume()==Status::Ok);const auto resumed=p.consume(2);CHECK(resumed.samples==std::vector<int16_t>(bytes.begin()+6,bytes.begin()+10));
    CHECK(p.stop()==Status::Ok);const auto stopped=p.consume(2);CHECK(stopped.guard==Guard::Stopped&&stopped.blockedFrames==2);
    CHECK(p.start()==Status::Ok);p.consume(1);CHECK(p.flush()==Status::Ok);
    CHECK(p.actual.state().queue.empty()&&p.actual.state().playback==Playback::Stopped);
    const auto events=p.poll();CHECK(events.size()==1&&events[0].type==EventType::Cancelled&&events[0].deliveredFrames==6);
    CHECK(p.release(h)==Status::Ok);const Handle next=p.clip(bytes);CHECK(next!=h);CHECK(p.enqueue(h,1)==Status::InvalidHandle);
    CHECK(p.enqueue(next,1)==Status::Ok);CHECK(p.flush()==Status::Ok);CHECK(p.flush()==Status::Ok);
    CHECK(p.poll().size()==1);
}
void looping(){for(int64_t loops:{1,2,3}){Pair p;const auto bytes=tagged(5,2,4);const Handle h=p.clip(bytes);
        CHECK(p.enqueue(h,loops)==Status::Ok);CHECK(p.start()==Status::Ok);std::vector<int16_t> actual;
        for(int n=0;n<4;++n){const auto out=p.consume(4);actual.insert(actual.end(),out.samples.begin(),out.samples.end());}
        std::vector<int16_t> expected;for(int64_t n=0;n<loops;++n)expected.insert(expected.end(),bytes.begin(),bytes.end());
        expected.insert(expected.end(),size_t(16-loops*5)*2,0);CHECK(actual==expected);
        const auto events=p.poll();CHECK(events.size()==1&&events[0].deliveredFrames==uint64_t(loops*5));CHECK(p.actual.state().queue.empty());}
    Pair p;const Handle a=p.clip(tagged(3,2,5)),b=p.clip(tagged(2,2,6));CHECK(p.enqueue(a,0)==Status::Ok);CHECK(p.enqueue(b,1)==Status::Ok);
    CHECK(p.start()==Status::Ok);const auto out=p.consume(11);CHECK(out.sourceFrames==11&&out.underrunFrames==0&&p.poll().empty());
    const auto s=p.actual.state();CHECK(s.queue[0].passes==3&&s.queue[0].cursorFrames==2&&s.queue[1].deliveredFrames==0);
    CHECK(p.flush()==Status::Ok);const auto cancel=p.poll();CHECK(cancel.size()==2&&cancel[0].deliveredFrames==11&&cancel[1].deliveredFrames==0);
}
void ownershipCapacity(){Pair p;auto caller=tagged(4,2,7);const auto original=caller;const Handle h=p.clip(caller);caller.assign(caller.size(),0);
    std::vector<Handle> handles{h};for(int i=1;i<16;++i)handles.push_back(p.clip(tagged(1,2,i)));
    CHECK(p.source({22050,2},{1,2})==Status::Capacity);CHECK(p.lastHandle==0);
    for(int i=0;i<8;++i)CHECK(p.enqueue(h,1)==Status::Ok);CHECK(p.enqueue(h,1)==Status::Capacity);
    CHECK(p.release(h)==Status::Busy);CHECK(p.start()==Status::Ok);CHECK(p.consume(4).samples==original);
    CHECK(p.release(h)==Status::Busy);CHECK(p.flush()==Status::Ok);const auto events=p.poll();CHECK(events.size()==8);
    CHECK(events[0].type==EventType::Completed);for(size_t i=1;i<events.size();++i)CHECK(events[i].type==EventType::Cancelled);
    CHECK(p.release(h)==Status::Ok);const Handle replacement=p.clip(original);CHECK(replacement!=h);
    CHECK(p.release(h)==Status::InvalidHandle);CHECK(p.enqueue(h,1)==Status::InvalidHandle);
    for(size_t i=1;i<handles.size();++i)CHECK(p.release(handles[i])==Status::Ok);CHECK(p.release(replacement)==Status::Ok);
    for(const auto& source:p.actual.state().sources)CHECK(!source.live&&source.samples.empty()&&source.queueReferences==0);
}
void simulatedDevice(){Pair p;const auto aBytes=tagged(7,2,8),bBytes=tagged(3,2,9);const Handle a=p.clip(aBytes),b=p.clip(bBytes);
    CHECK(p.enqueue(a,2)==Status::Ok);CHECK(p.enqueue(b,1)==Status::Ok);CHECK(p.start()==Status::Ok);p.consume(5);
    const auto queueBefore=p.actual.state().queue;CHECK(p.loss()==Status::Ok);const auto blocked=p.consume(4);
    CHECK(blocked.guard==Guard::DeviceLost&&blocked.blockedFrames==4&&blocked.sourceFrames==0&&blocked.underrunFrames==0);
    CHECK(p.actual.state().queue[0].cursorFrames==queueBefore[0].cursorFrames);
    auto before=p.actual.state();CHECK(p.rebuild(7,{22050,4})==Status::UnsupportedFormat);CHECK(stateEqual(before,p.actual.state()));
    CHECK(p.rebuild(0,{22050,2})==Status::InvalidDevice);CHECK(stateEqual(before,p.actual.state()));
    CHECK(p.rebuild(7,{22050,2},false)==Status::DeviceUnavailable);CHECK(stateEqual(before,p.actual.state()));
    CHECK(p.rebuild(7,{22050,2})==Status::Ok);CHECK(p.actual.state().playback==Playback::Running);
    const auto next=p.consume(10);std::vector<int16_t> expected(aBytes.begin()+10,aBytes.end());expected.insert(expected.end(),aBytes.begin(),aBytes.end());
    expected.insert(expected.end(),bBytes.begin(),bBytes.begin()+2);CHECK(next.samples==expected);
    const auto last=p.consume(4);CHECK(last.sourceFrames==2&&last.underrunFrames==2);
    const auto events=p.poll();CHECK(events.size()==4&&events[0].type==EventType::DeviceLost&&events[1].type==EventType::DeviceRebuilt);
    CHECK(events[2].type==EventType::Completed&&events[2].deliveredFrames==14&&events[3].deliveredFrames==3);
    CHECK(p.actual.state().counters.sourceFrames==17);
    Pair paused;const Handle h=paused.clip(tagged(5,2,1));CHECK(paused.enqueue(h,1)==Status::Ok);CHECK(paused.start()==Status::Ok);
    paused.consume(2);CHECK(paused.pause()==Status::Ok);CHECK(paused.loss()==Status::Ok);
    CHECK(paused.resume()==Status::DeviceUnavailable);CHECK(paused.rebuild(2,{22050,2})==Status::Ok);
    CHECK(paused.actual.state().playback==Playback::Paused&&paused.consume(2).blockedFrames==2);
    CHECK(paused.resume()==Status::Ok);CHECK(paused.consume(3).sourceFrames==3);
}
void negativeNoMutation(){Pair p;const Handle h=p.clip({1,-1,2,-2});const auto baseline=p.actual.state();
    CHECK(p.source({0,2},{1,2})==Status::InvalidFormat);CHECK(stateEqual(baseline,p.actual.state()));
    CHECK(p.source({22050,0},{1,2})==Status::InvalidFormat);CHECK(p.source({44100,2},{1,2})==Status::UnsupportedFormat);
    CHECK(p.source({22050,2},{})==Status::InvalidPCM);CHECK(p.source({22050,2},{1,2,3})==Status::InvalidPCM);
    CHECK(p.source({22050,2},std::vector<int16_t>(65537*2,0))==Status::InvalidPCM);
    CHECK(p.enqueue(h,-1)==Status::InvalidLoops);CHECK(p.enqueue(h,2147483648LL)==Status::InvalidLoops);
    CHECK(p.enqueue(0,1)==Status::InvalidHandle);CHECK(p.release(h+(1ULL<<32))==Status::InvalidHandle);
    CHECK(p.pause()==Status::InvalidState);CHECK(p.resume()==Status::InvalidState);CHECK(p.rebuild(1,{22050,2})==Status::InvalidState);
    for(size_t frames:{size_t(0),size_t(4097),std::numeric_limits<size_t>::max()}){const auto result=p.consume(frames);
        CHECK(result.samples==std::vector<int16_t>({123,-123})&&result.sourceFrames==9&&result.underrunFrames==8&&result.blockedFrames==7);}
    CHECK(stateEqual(baseline,p.actual.state()));CHECK(p.loss()==Status::Ok);const auto lost=p.actual.state();
    CHECK(p.loss()==Status::InvalidState);CHECK(p.start()==Status::DeviceUnavailable);CHECK(p.rebuild(1,{0,2})==Status::InvalidFormat);
    CHECK(stateEqual(lost,p.actual.state()));
    bool rejected=false;try{Player invalid({22050,9});}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
}
void boundaryRegressions(){Pair p;const Handle h=p.clip({0,0});CHECK(p.enqueue(h,2)==Status::Ok);CHECK(p.start()==Status::Ok);
    const auto out=p.consume(2);CHECK(out.samples==std::vector<int16_t>({0,0,0,0})&&out.sourceFrames==2&&out.underrunFrames==0);
    auto e=p.poll();CHECK(e.size()==1&&e[0].type==EventType::Completed&&e[0].deliveredFrames==2);
    CHECK(p.consume(1).underrunFrames==1&&p.poll().empty());CHECK(p.enqueue(h,1)==Status::Ok);CHECK(p.consume(1).sourceFrames==1);
    CHECK(p.flush()==Status::Ok);e=p.poll();CHECK(e.size()==1&&e[0].type==EventType::Completed);
    CHECK(p.release(h)==Status::Ok);CHECK(p.flush()==Status::Ok&&p.poll().empty());
}

void randomizedEvents(){Pair p;Random rng{seed};std::vector<Handle> history;std::ofstream trace(directory+"/random_trace.jsonl");CHECK(bool(trace));
    for(int step=0;step<4096;++step){context="random seed 0xa0d105eed2026001 step "+std::to_string(step);
        const int op=history.empty()?0:rng.below(12);Handle h=0;
        if(!history.empty()){std::vector<Handle> live;const auto state=p.reference.state();
            for(size_t i=0;i<16;++i)if(state.sources[i].live)live.push_back(uint64_t(state.sources[i].generation)*4294967296ULL+i+1);
            if(!live.empty()&&rng.below(2)==0)h=live[size_t(rng.below(int(live.size())))];else h=history[size_t(rng.below(int(history.size())))];}
        trace<<"{\"step\":"<<step<<",\"op\":"<<op<<",\"handle\":"<<h;
        if(op==0){const int count=1+rng.below(17),salt=1+rng.below(200);const auto samples=tagged(size_t(count),2,salt);
            trace<<",\"frames\":"<<count<<",\"salt\":"<<salt<<"}\n";trace.flush();p.source({22050,2},samples);
            if(p.lastHandle)history.push_back(p.lastHandle);}
        else if(op==1){trace<<"}\n";trace.flush();p.release(h);}
        else if(op==2){const int64_t options[]={-1,0,1,2,3,2147483648LL};const int64_t loops=options[rng.below(6)];
            trace<<",\"loops\":"<<loops<<"}\n";trace.flush();p.enqueue(h,loops);}
        else if(op==3){const size_t count=size_t(rng.below(66));trace<<",\"frames\":"<<count<<"}\n";trace.flush();p.consume(count);}
        else if(op==10){const uint64_t device=uint64_t(rng.below(9));const bool good=rng.below(4)!=0,available=rng.below(4)!=0;
            const Format format{good?22050u:44100u,2};trace<<",\"device\":"<<device<<",\"rate\":"<<format.rate
                <<",\"available\":"<<(available?"true":"false")<<"}\n";trace.flush();p.rebuild(device,format,available);}
        else {trace<<"}\n";trace.flush();switch(op){case 4:p.start();break;case 5:p.pause();break;case 6:p.resume();break;
            case 7:p.stop();break;case 8:p.flush();break;case 9:p.loss();break;case 11:p.poll();break;default:CHECK(false);}}
        ++totals.randomEvents;
    }
    totals.finalRng=rng.state;context="random cleanup";CHECK(p.flush()==Status::Ok);
    const auto state=p.reference.state();for(size_t i=0;i<16;++i)if(state.sources[i].live)
        CHECK(p.release(uint64_t(state.sources[i].generation)*4294967296ULL+i+1)==Status::Ok);
    for(const auto& s:p.actual.state().sources)CHECK(!s.live&&s.samples.empty()&&s.queueReferences==0);
}
void goldenFixture(){Pair p;const std::vector<int16_t> a{100,-100,101,-101,102,-102},b{200,-200,201,-201};
    const Handle ah=p.clip(a),bh=p.clip(b);CHECK(p.enqueue(ah,2)==Status::Ok);CHECK(p.enqueue(bh,1)==Status::Ok);CHECK(p.start()==Status::Ok);
    std::vector<int16_t> actual,expected;
    for(size_t count:{size_t(3),size_t(4),size_t(3)}){const auto out=p.consume(count);actual.insert(actual.end(),out.samples.begin(),out.samples.end());}
    expected={100,-100,101,-101,102,-102,100,-100,101,-101,102,-102,200,-200,201,-201,0,0,0,0};
    CHECK(actual==expected);writePCM("golden_source_a.s16le",a);writePCM("golden_source_b.s16le",b);
    writePCM("golden_expected.s16le",expected);writePCM("golden_actual.s16le",actual);
    const auto events=p.poll();CHECK(events.size()==2&&events[0].deliveredFrames==6&&events[1].deliveredFrames==2);
    std::ofstream json(directory+"/golden.json");CHECK(bool(json));
    json<<"{\"format\":\"S16LE interleaved\",\"rate\":22050,\"channels\":2,\"source_frames\":[3,2],"
        <<"\"loops\":[2,1],\"pull_frames\":[3,4,3],\"output_frames\":10,\"output_samples\":20,"
        <<"\"source_frames_delivered\":8,\"underrun_frames\":2,\"different_samples\":0,\"completed_events\":2}\n";
    std::ofstream(directory+"/golden_final_state.json")<<stateText(p.actual.state());CHECK(bool(json));
}
std::string escape(const std::string& text){std::string out;for(char c:text){if(c=='"'||c=='\\')out+='\\';if(c=='\n')out+="\\n";else out+=c;}return out;}
void summary(bool passed,const std::string& error,double ms){std::ofstream out(directory+"/summary.json");if(!out)throw std::runtime_error("cannot save summary");
    out<<"{\n  \"passed\":"<<(passed?"true":"false")<<",\n  \"scope\":\"self-authored offline PCM queue and simulated device events\",\n"
        <<"  \"groups_passed\":"<<totals.groups<<",\n  \"assertions\":"<<totals.assertions<<",\n  \"state_comparisons\":"<<totals.stateComparisons
        <<",\n  \"pcm_samples_compared\":"<<totals.samplesCompared<<",\n  \"successful_pull_calls\":"<<totals.pullCalls
        <<",\n  \"negative_returns_checked\":"<<totals.negativeReturns<<",\n  \"random_events\":"<<totals.randomEvents
        <<",\n  \"random_seed_hex\":\"0xa0d105eed2026001\",\n  \"random_final_hex\":\"0x"<<std::hex<<totals.finalRng<<"\",\n"<<std::dec
        <<"  \"elapsed_ms\":"<<ms<<",\n  \"error\":\""<<escape(error)<<"\",\n"
        <<"  \"real_audio_device_accessed\":false,\n  \"auditory_output_verified\":false,\n  \"actual_device_recovery_verified\":false,\n"
        <<"  \"game_resources_or_code_loaded\":false,\n  \"network_UI_or_input_used\":false,\n  \"ASAN_attempted_this_lab\":false\n}\n";
    if(!out)throw std::runtime_error("summary write failed");}
}
int main(int argc,char** argv){if(argc!=2){std::cerr<<"usage: audio-tests EXISTING_EVIDENCE_DIRECTORY\n";return 2;}directory=argv[1];
    const auto start=std::chrono::steady_clock::now();
    const std::vector<std::pair<std::string,std::function<void()>>> groups{
        {"FIFO buffer boundaries and underrun",fifoBoundaries},{"1/2/4/6/8-channel frame/sample accounting",channelAccounting},
        {"pause resume stop flush",pauseStopFlush},{"finite and infinite total-play loops",looping},
        {"copied ownership capacity and stale handles",ownershipCapacity},{"simulated device loss/rebuild",simulatedDevice},
        {"negative inputs preserve state",negativeNoMutation},{"last-frame zero-PCM and event regressions",boundaryRegressions},
        {"4096 deterministic reference-model events",randomizedEvents},{"persisted hand-written PCM golden",goldenFixture}};
    try{for(const auto& group:groups){context=group.first;const size_t before=totals.assertions;group.second();++totals.groups;
            std::cout<<"PASS "<<group.first<<" ("<<totals.assertions-before<<" assertions)\n";}
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();summary(true,"",ms);
        std::cout<<"PASS all: "<<totals.groups<<" groups; "<<totals.samplesCompared<<" PCM samples compared; offline only\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<"\n";
        std::ofstream(directory+"/failure_context.txt")<<context<<"\n"<<error.what()<<"\n";
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        try{summary(false,error.what(),ms);}catch(const std::exception& writeError){std::cerr<<writeError.what()<<"\n";}return 1;}}
