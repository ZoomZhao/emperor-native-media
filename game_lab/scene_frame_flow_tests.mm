// SPDX-License-Identifier: MIT
#import <Foundation/Foundation.h>
#include "scene_frame_flow.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using namespace game_lab;
namespace {
size_t checks=0,orders=0,lifetimeOrders=0;
std::string group;
std::map<std::string,size_t> groups;
void need(bool ok,const char* why){++checks;++groups[group];if(!ok)throw std::runtime_error(why);}
struct Metrics {
    unsigned created=0,destroyed=0,ownersDestroyed=0,commits=0,cpuStarts=0,scheduled=0,visible=0;
    std::map<unsigned,unsigned> releases;
};
struct Lifetime {
    std::shared_ptr<Metrics> metrics;unsigned id;
    explicit Lifetime(std::shared_ptr<Metrics> m):metrics(std::move(m)),id(++metrics->created){}
    ~Lifetime(){++metrics->destroyed;++metrics->releases[id];}
};
// Immutable candidate and completed-frame identities are authored stand-ins, not
// game snapshots, pixels or a second renderer. Both carry checked lifetimes.
struct Candidate:Lifetime {int balance;uint64_t revision,digest;
    Candidate(std::shared_ptr<Metrics> m,int b,uint64_t r,uint64_t d):Lifetime(std::move(m)),balance(b),revision(r),digest(d){}
};
struct Frame:Lifetime {using Lifetime::Lifetime;};
struct Request:Lifetime {
    SceneFrameFlow flow;std::shared_ptr<const Candidate> candidate;std::shared_ptr<const Frame> frame;
    Request(std::shared_ptr<Metrics> m,SceneFrameTicket t,std::shared_ptr<const Candidate> c):Lifetime(std::move(m)),flow{t},candidate(std::move(c)){}
};
}
typedef BOOL (^FlowReply)(SceneFrameSignal,const SceneFrameTicket&,uint32_t,uint32_t);
namespace {
struct Job {unsigned id;SceneFrameWork work;std::shared_ptr<Request> request;FlowReply reply;};
// No threads, clocks, GPU work or implicit success: tests explicitly choose which
// queued completion reaches the main-thread owner next. Two-slot limit models
// backend backpressure only; stage47 tests cover the real display bridge's cap.
struct FakePorts {
    std::vector<Job> jobs;unsigned next=0;bool declineGPU=false,declineDisplay=false;
    unsigned submit(SceneFrameWork work,std::shared_ptr<Request> request,FlowReply reply) {
        if((work==SceneFrameWork::GPU&&declineGPU)||(work==SceneFrameWork::Display&&declineDisplay))return 0;
        if(work!=SceneFrameWork::CPU&&std::count_if(jobs.begin(),jobs.end(),[&](const Job&j){return j.work==work;})>=2)return 0;
        unsigned id=++next;jobs.push_back({id,work,std::move(request),[reply copy]});return id;
    }
    unsigned find(SceneFrameWork work,bool newest=false)const {
        unsigned found=0;for(const auto&j:jobs)if(j.work==work){found=j.id;if(!newest)break;}return found;
    }
    bool deliver(unsigned id,SceneFrameSignal signal,uint32_t w=4,uint32_t h=3,bool duplicate=false) {
        auto it=std::find_if(jobs.begin(),jobs.end(),[&](const Job&j){return j.id==id;});need(it!=jobs.end(),"queued callback exists");
        Job job=std::move(*it);jobs.erase(it);bool admitted=job.reply(signal,job.request->flow.ticket,w,h);
        if(duplicate)need(!job.reply(signal,job.request->flow.ticket,w,h),"duplicate callback cannot publish twice");
        if(admitted&&signal==SceneFrameSignal::DisplayAdmit){++job.request->metrics->scheduled;job.reply(SceneFrameSignal::DisplayScheduled,job.request->flow.ticket,w,h);}
        return admitted;
    }
};
}
@interface FlowOwner:NSObject {
@public
    SceneFrameGateState gate;
    std::shared_ptr<FakePorts> ports;
    std::shared_ptr<Metrics> metrics;
    std::shared_ptr<Request> active;
    std::shared_ptr<const Candidate> city;
    std::shared_ptr<const Frame> image;
    int visibleBalance;
}
- (instancetype)initWithPorts:(std::shared_ptr<FakePorts>)p metrics:(std::shared_ptr<Metrics>)m;
- (SceneFrameTicket)requestBalance:(int)balance revision:(uint64_t)revision digest:(uint64_t)digest;
- (void)invalidate:(SceneFrameChange)change;
- (void)queue:(std::shared_ptr<Request>)request work:(SceneFrameWork)work;
- (BOOL)receive:(std::shared_ptr<Request>)request signal:(SceneFrameSignal)signal ticket:(const SceneFrameTicket&)ticket width:(uint32_t)width height:(uint32_t)height;
@end
@implementation FlowOwner
- (instancetype)initWithPorts:(std::shared_ptr<FakePorts>)p metrics:(std::shared_ptr<Metrics>)m {
    if(!(self=[super init]))return nil;ports=std::move(p);metrics=std::move(m);visibleBalance=100;
    gate=changeSceneFrame({},SceneFrameChange::BeginSession).state;return self;
}
- (void)dealloc{++metrics->ownersDestroyed;}
- (SceneFrameTicket)requestBalance:(int)balance revision:(uint64_t)revision digest:(uint64_t)digest {
    need(NSThread.isMainThread,"all owner state transitions on main");
    auto issued=requestSceneFrame(gate,revision,digest,4,3);gate=issued.state;
    need(gate.pending.has_value(),"test owner has current request");
    auto candidate=std::make_shared<const Candidate>(metrics,balance,revision,digest);
    active=std::make_shared<Request>(metrics,*gate.pending,std::move(candidate));
    const auto ticket=active->flow.ticket;[self queue:active work:SceneFrameWork::GPU];return ticket;
}
- (void)invalidate:(SceneFrameChange)change {gate=changeSceneFrame(gate,change).state;active.reset();}
- (void)queue:(std::shared_ptr<Request>)request work:(SceneFrameWork)work {
    if(work==SceneFrameWork::CPU)++metrics->cpuStarts;
    __weak FlowOwner* weakOwner=self;
    FlowReply reply=^BOOL(SceneFrameSignal signal,const SceneFrameTicket& ticket,uint32_t w,uint32_t h){
        // The callback owns its immutable request, NEVER the owner. A real backend
        // adapter must dispatch to main before this weak load (tested separately).
        FlowOwner* owner=weakOwner;if(!owner)return NO;
        return [owner receive:request signal:signal ticket:ticket width:w height:h];
    };
    if(!ports->submit(work,request,reply)) {
        [self receive:request signal:work==SceneFrameWork::GPU?SceneFrameSignal::GPUDeclined:SceneFrameSignal::DisplayDeclined ticket:request->flow.ticket width:0 height:0];
    }
}
- (BOOL)receive:(std::shared_ptr<Request>)request signal:(SceneFrameSignal)signal ticket:(const SceneFrameTicket&)ticket width:(uint32_t)width height:(uint32_t)height {
    need(NSThread.isMainThread,"callback delivered to main before weak owner publication");
    auto r=reduceSceneFrameFlow(gate,request->flow,signal,ticket,width,height);gate=r.gate;request->flow=r.flow;
    switch(r.action) {
        case SceneFrameAction::StartDisplay:
            request->frame=std::make_shared<const Frame>(metrics);[self queue:request work:SceneFrameWork::Display];break;
        case SceneFrameAction::StartCPU:
            request->frame.reset();[self queue:request work:SceneFrameWork::CPU];break;
        case SceneFrameAction::PublishCPU:request->frame=std::make_shared<const Frame>(metrics);[[fallthrough]];
        case SceneFrameAction::PublishGPU:
            // Atomic model publication after consume, with no yield/modal between.
            city=request->candidate;image=request->frame;visibleBalance=city->balance;++metrics->commits;
            if(active==request)active.reset();return YES;
        case SceneFrameAction::None:break;
    }
    if(request->flow.work==SceneFrameWork::Finished&&active==request)active.reset();
    return NO;
}
@end
namespace {
void released(const std::shared_ptr<Metrics>&m) {
    need(m->created==m->destroyed,"all candidate/frame/request owners released");
    need(m->releases.size()==m->created,"all allocated identities released");
    for(auto [id,count]:m->releases){(void)id;need(count==1,"every resource destructor exactly once");}
}
void fallbackCases(){group="fallback_chain";
    auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
    @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];
        auto t=[o requestBalance:90 revision:1 digest:7];p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUFailed,0,0,true);
        need(m->cpuStarts==1&&m->commits==0&&o->gate.pending==t&&o->visibleBalance==100,"GPU failure queues one CPU without early consume or charge");
        auto oldCPU=p->find(SceneFrameWork::CPU);[o invalidate:SceneFrameChange::Camera];auto current=[o requestBalance:80 revision:2 digest:7];
        need(!p->deliver(oldCPU,SceneFrameSignal::CPUReady)&&o->gate.pending==current&&o->visibleBalance==100,"CPU finish checks again after camera supersession");
        p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUFailed);
        need(p->deliver(p->find(SceneFrameWork::CPU),SceneFrameSignal::CPUReady,4,3,true)&&o->visibleBalance==80&&!o->gate.pending,"current CPU publishes exactly once after consume");
        need(m->cpuStarts==2&&m->commits==1,"fallback and commit counters have separate meanings");
        // Complete GPU => display => unavailable/timeout outcome => CPU.
        [o requestBalance:70 revision:3 digest:7];p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUReady);
        need(o->visibleBalance==80&&m->scheduled==0,"GPU Ready itself does not commit or present");
        p->deliver(p->find(SceneFrameWork::Display),SceneFrameSignal::DisplayFailed);
        need(o->visibleBalance==80&&m->cpuStarts==3,"drawable unavailable or timeout leaves prior city intact");
        p->deliver(p->find(SceneFrameWork::CPU),SceneFrameSignal::CPUFailed);
        need(o->visibleBalance==80&&!o->gate.pending&&m->commits==1,"CPU failure terminates without partial commit or retry loop");
        [o invalidate:SceneFrameChange::Close];o=nil;
    }released(m);need(p->jobs.empty(),"fallback queues drained");
}
void lifecycleOrdersCases(){group="reload_geometry_resources";
    for(auto change:{SceneFrameChange::BeginSession,SceneFrameChange::Camera,SceneFrameChange::Resources}) {
        for(auto late:{SceneFrameSignal::GPUReady,SceneFrameSignal::GPUFailed}) {
            auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
            @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];
                auto old=[o requestBalance:90 revision:7 digest:23];auto oldID=p->find(SceneFrameWork::GPU);
                [o invalidate:change];auto now=[o requestBalance:80 revision:7 digest:23];
                p->deliver(oldID,late);need(o->gate.pending==now&&m->cpuStarts==0&&m->commits==0,"old callback cannot fall back after same-source reload/resize/resource change");
                need(!(old==now),"new lifecycle retained with reused revision and scene");
                p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUReady);auto display=p->find(SceneFrameWork::Display);
                [o invalidate:change];need(!p->deliver(display,SceneFrameSignal::DisplayAdmit)&&o->visibleBalance==100&&m->scheduled==0,"last-moment display admission rejects changed lifecycle");
                [o invalidate:SceneFrameChange::Close];o=nil;
            }released(m);
        }
    }
    // A failed replacement keeps the old city, but never restores its epoch.
    auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
    @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];
        auto a=[o requestBalance:90 revision:1 digest:5];auto first=p->find(SceneFrameWork::GPU);
        [o invalidate:SceneFrameChange::BeginSession];[o requestBalance:80 revision:1 digest:5];auto second=p->find(SceneFrameWork::GPU,true);
        p->deliver(second,SceneFrameSignal::GPUFailed);p->deliver(p->find(SceneFrameWork::CPU),SceneFrameSignal::CPUFailed);
        need(o->visibleBalance==100&&o->gate.sessionEpoch>a.key.sessionEpoch,"failed replacement retains city and new epoch");
        p->deliver(first,SceneFrameSignal::GPUReady);need(m->commits==0&&p->jobs.empty(),"failed load cannot resurrect preceding completion");o=nil;
    }released(m);
}
void permutations(){group="completion_orders";
    std::array<unsigned,3> order={0,1,2};
    do {auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
        @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];
            [o requestBalance:90 revision:1 digest:1];unsigned a=p->find(SceneFrameWork::GPU);
            [o invalidate:SceneFrameChange::Camera];[o requestBalance:80 revision:1 digest:1];unsigned b=p->find(SceneFrameWork::GPU,true);
            [o invalidate:SceneFrameChange::Camera];[o requestBalance:70 revision:1 digest:1];unsigned c=p->find(SceneFrameWork::CPU);
            need(p->jobs.size()==3&&c&&m->cpuStarts==1,"two GPU slots full; current third request uses bounded CPU fallback");
            const unsigned ids[]={a,b,c};
            for(auto i:order)p->deliver(ids[i],i==2?SceneFrameSignal::CPUReady:SceneFrameSignal::GPUFailed);
            need(o->visibleBalance==70&&m->commits==1&&m->cpuStarts==1&&!o->gate.pending,"every completion order admits only newest camera");o=nil;
        }released(m);++orders;
    }while(std::next_permutation(order.begin(),order.end()));
}
void displayAndSizeCases(){group="display_and_sizes";
    for(bool declinedGPU:{false,true})for(bool declinedDisplay:{false,true}) {
        auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();p->declineGPU=declinedGPU;p->declineDisplay=declinedDisplay;
        @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];[o requestBalance:90 revision:1 digest:4];
            if(!declinedGPU)p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUReady);
            if(declinedGPU||declinedDisplay)p->deliver(p->find(SceneFrameWork::CPU),SceneFrameSignal::CPUReady);
            else {auto j=p->jobs.front();need(!j.reply(SceneFrameSignal::DisplayScheduled,j.request->flow.ticket,4,3)&&o->visibleBalance==100,"Scheduled alone never publishes or proves visibility");p->deliver(p->find(SceneFrameWork::Display),SceneFrameSignal::DisplayAdmit,4,3,true);}
            need(o->visibleBalance==90&&m->commits==1&&m->visible==0,"single publication is distinct from visible presentation proof");o=nil;
        }released(m);
    }
    for(auto where:{SceneFrameWork::GPU,SceneFrameWork::Display,SceneFrameWork::CPU}) {
        auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
        @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];[o requestBalance:90 revision:1 digest:4];
            if(where==SceneFrameWork::GPU)p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUReady,5,3);
            if(where==SceneFrameWork::Display){p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUReady);p->deliver(p->find(SceneFrameWork::Display),SceneFrameSignal::DisplayAdmit,5,3);}
            if(where==SceneFrameWork::CPU)p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUFailed);
            need(o->visibleBalance==100&&m->cpuStarts==1,"wrong GPU/display output never publishes before CPU");
            p->deliver(p->find(SceneFrameWork::CPU),SceneFrameSignal::CPUReady,5,3);
            need(o->visibleBalance==100&&!o->gate.pending&&m->commits==0,"wrong CPU output refuses entire candidate");o=nil;
        }released(m);
    }
    for(auto sig:{SceneFrameSignal::GPUCancelled,SceneFrameSignal::DisplayCancelled}) {
        auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
        @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];[o requestBalance:90 revision:1 digest:4];
            if(sig==SceneFrameSignal::DisplayCancelled)p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUReady);
            p->deliver(p->jobs.front().id,sig);need(m->cpuStarts==0&&m->commits==0&&!o->gate.pending,"accepted cancellation never silently retries");o=nil;
        }released(m);
    }
}
void weakOwnerCases(){group="weak_owner_lifetime";
    for(bool closeFirst:{false,true})for(bool cpuFirst:{false,true}) {
        auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();__weak FlowOwner* weak=nil;
        @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];weak=o;
            [o requestBalance:90 revision:1 digest:4];p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUFailed);
            [o invalidate:SceneFrameChange::Camera];[o requestBalance:80 revision:2 digest:4];
            need(p->jobs.size()==2,"CPU and GPU completions both outstanding");if(closeFirst)[o invalidate:SceneFrameChange::Close];o=nil;
        }
        need(weak==nil&&m->ownersDestroyed==1,"pending backend callbacks do not retain Objective-C owner");
        auto cpu=p->find(SceneFrameWork::CPU),gpu=p->find(SceneFrameWork::GPU);
        if(cpuFirst){p->deliver(cpu,SceneFrameSignal::CPUReady);p->deliver(gpu,SceneFrameSignal::GPUReady);}
        else {p->deliver(gpu,SceneFrameSignal::GPUReady);p->deliver(cpu,SceneFrameSignal::CPUReady);}
        need(m->commits==0&&m->cpuStarts==1&&p->jobs.empty(),"late callbacks on destroyed owner neither publish nor start new work");released(m);++lifetimeOrders;
    }
    // Closing an owner that remains alive also rejects pending completions.
    auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
    @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];[o requestBalance:90 revision:1 digest:4];p->deliver(p->find(SceneFrameWork::GPU),SceneFrameSignal::GPUReady);
        [o invalidate:SceneFrameChange::Close];p->deliver(p->find(SceneFrameWork::Display),SceneFrameSignal::DisplayFailed);
        need(m->cpuStarts==0&&m->commits==0&&o->visibleBalance==100,"closed live owner rejects display failure without CPU work");o=nil;
    }released(m);
}
void mainQueueHopCase(){group="callback_queue_hop";
    auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
    @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];[o requestBalance:90 revision:1 digest:4];
        Job job=std::move(p->jobs.front());p->jobs.clear();__block bool arrived=false;
        dispatch_queue_t queue=dispatch_queue_create("org.emperor.test.backend-callback",DISPATCH_QUEUE_SERIAL);
        dispatch_async(queue,^{
            bool offMain=!NSThread.isMainThread;
            dispatch_async(dispatch_get_main_queue(),^{need(offMain,"backend-style callback originates off main");job.reply(SceneFrameSignal::GPUFailed,job.request->flow.ticket,4,3);arrived=true;});
        });
        NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:3];while(!arrived&&deadline.timeIntervalSinceNow>0)[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.005]];
        need(arrived&&m->cpuStarts==1,"explicit main queue hop before owner access");p->deliver(p->find(SceneFrameWork::CPU),SceneFrameSignal::CPUReady);o=nil;
    }released(m);
}
void echoAndOrderingCases(){group="echo_and_stage_contract";
    auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();
    @autoreleasepool {FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];
        auto t=[o requestBalance:90 revision:1 digest:7];auto j=p->jobs.front();
        for(unsigned field=0;field<8;++field){auto wrong=t;switch(field){
            case 0:++wrong.key.sessionEpoch;break;case 1:++wrong.key.revision;break;
            case 2:++wrong.key.sceneDigest;break;case 3:++wrong.key.cameraSeq;break;
            case 4:++wrong.key.cacheGeneration;break;case 5:++wrong.requestSerial;break;
            case 6:++wrong.width;break;case 7:++wrong.height;break;}
            need(!j.reply(SceneFrameSignal::GPUReady,wrong,4,3)&&o->gate.pending==t&&j.request->flow.work==SceneFrameWork::GPU,"wrong echoed ticket cannot advance callback chain");
            need(m->cpuStarts==0&&m->commits==0&&p->jobs.size()==1,"wrong echo causes no work or publication");
        }
        need(!j.reply(SceneFrameSignal::CPUReady,t,4,3)&&o->gate.pending==t,"CPU result cannot skip GPU phase");
        p->deliver(j.id,SceneFrameSignal::GPUReady);p->deliver(p->find(SceneFrameWork::Display),SceneFrameSignal::DisplayAdmit);
        need(m->commits==1&&m->scheduled==1&&m->visible==0,"only display admission consumes; scheduling is still not visibility");o=nil;
    }released(m);
}
void destroyedBetweenQueuesCase(){group="destroyed_between_queues";
    auto p=std::make_shared<FakePorts>();auto m=std::make_shared<Metrics>();__weak FlowOwner* weak=nil;
    __block bool arrived=false;
    @autoreleasepool {
        FlowOwner* o=[[FlowOwner alloc]initWithPorts:p metrics:m];weak=o;[o requestBalance:90 revision:1 digest:4];
        Job job=std::move(p->jobs.front());p->jobs.clear();
        dispatch_semaphore_t queued=dispatch_semaphore_create(0);
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT,0),^{
            dispatch_async(dispatch_get_main_queue(),^{job.reply(SceneFrameSignal::GPUFailed,job.request->flow.ticket,4,3);arrived=true;});
            dispatch_semaphore_signal(queued);
        });
        // Test synchronization only: force the main callback to remain queued
        // until after the actual ARC owner is released. No production GPU wait.
        need(dispatch_semaphore_wait(queued,dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC))==0,"backend enqueued main callback");
        o=nil;need(weak==nil&&m->ownersDestroyed==1,"main queue hop did not capture strong owner");
    }
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:3];
    while(!arrived&&deadline.timeIntervalSinceNow>0)[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.005]];
    need(arrived&&m->cpuStarts==0&&m->commits==0&&p->jobs.empty(),"destroyed owner cannot start fallback from queued main callback");released(m);
}

}
int main(){@autoreleasepool {try {
    fallbackCases();lifecycleOrdersCases();permutations();displayAndSizeCases();weakOwnerCases();mainQueueHopCase();echoAndOrderingCases();destroyedBetweenQueuesCase();
    std::cout<<"{\"passed\":true,\"authored_checks\":"<<checks<<",\"completion_orders\":"<<orders<<",\"weak_owner_orders\":"<<lifetimeOrders<<",\"groups\":{";
    bool first=true;for(const auto&[name,count]:groups){if(!first)std::cout<<',';first=false;std::cout<<'"'<<name<<"\":"<<count;}
    std::cout<<"},\"App_executed\":false,\"GPU_executed\":false,\"visible_frames_proved\":0}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}}
