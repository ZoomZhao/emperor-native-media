// SPDX-License-Identifier: MIT
#include "scene_frame_gate.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace game_lab;
namespace {
size_t checks=0, permutations=0, histories=0;
std::map<std::string,size_t> groups;
std::string group;
void need(bool ok,const char* why) {
    ++checks; ++groups[group]; if(!ok) throw std::runtime_error(why);
}
bool same(const SceneFrameGateState& a,const SceneFrameGateState& b) {
    return a.phase==b.phase && a.sessionEpoch==b.sessionEpoch && a.cameraSeq==b.cameraSeq &&
        a.cacheGeneration==b.cacheGeneration && a.requestSerial==b.requestSerial && a.pending==b.pending;
}
SceneFrameTicket request(SceneFrameGateState& s,uint64_t revision=7,uint64_t digest=23,
                         uint32_t width=816,uint32_t height=620) {
    auto r=requestSceneFrame(s,revision,digest,width,height);
    need(r.status==SceneFrameStatus::Current && r.state.pending.has_value(),"valid request");
    s=r.state; return *s.pending;
}
void directCases() {
    group="lifecycle";
    SceneFrameGateState s;
    need(requestSceneFrame(s,0,0,816,620).status==SceneFrameStatus::NoSession,"explicit session required");
    s=changeSceneFrame(s,SceneFrameChange::BeginSession).state;
    auto a=request(s,0,0);
    need(a.key.sessionEpoch==1 && a.requestSerial==1,"first session/request identity");
    auto before=s;
    need(checkSceneFrameTicket(s,a)==SceneFrameStatus::Current && same(s,before),"decision is pure");
    auto done=consumeSceneFrame(s,a,816,620);
    need(done.status==SceneFrameStatus::Current && same(s,before),"consume returns separate value");
    need(!done.state.pending && checkSceneFrameTicket(done.state,a)==SceneFrameStatus::NoPending,"successful consume once");
    need(!mayFallbackSceneFrame(done.state,a),"duplicate failure cannot fallback");
    s=done.state; a=request(s);
    s=changeSceneFrame(s,SceneFrameChange::Cancel).state;
    need(!mayFallbackSceneFrame(s,a),"cancel invalidates fallback");
    auto retry=request(s);
    need(a.key==retry.key && retry.requestSerial>a.requestSerial,"identical key retry has fresh serial");
    need(checkSceneFrameTicket(s,a)==SceneFrameStatus::OldRequest,"identical key old callback refused");
    need(mayFallbackSceneFrame(s,retry),"current failure permits fallback");
    auto wrong=consumeSceneFrame(s,retry,817,620);
    need(wrong.status==SceneFrameStatus::TargetMismatch && same(wrong.state,s),"wrong GPU dimensions retain current intent");
    auto fallback=consumeSceneFrame(wrong.state,retry,816,620);
    need(fallback.status==SceneFrameStatus::Current && !fallback.state.pending,"CPU fallback must consume correct output");
    s=changeSceneFrame(s,SceneFrameChange::Camera).state;
    auto camera=request(s);
    need(checkSceneFrameTicket(s,retry)==SceneFrameStatus::OldCamera,"pan/fit/Retina sequence invalidates old view");
    s=changeSceneFrame(s,SceneFrameChange::Resources).state;
    auto resources=request(s);
    need(checkSceneFrameTicket(s,camera)==SceneFrameStatus::OldResources,"source/device generation invalidates old view");
    s=changeSceneFrame(s,SceneFrameChange::BeginSession).state;
    auto reload=request(s);
    need(reload.key.sessionEpoch==2 && reload.key.cameraSeq==camera.key.cameraSeq &&
        reload.key.cacheGeneration==resources.key.cacheGeneration,"reload advances epoch without resetting other counters");
    need(checkSceneFrameTicket(s,resources)==SceneFrameStatus::OldSession,"same SAV/revision reload rejects earlier frame");
    // A failed/cancelled replacement retains prior CITY state, but not prior gate state.
    auto previousEpoch=s.sessionEpoch;
    s=changeSceneFrame(s,SceneFrameChange::BeginSession).state;
    auto retainedCity=request(s,7,23);
    need(retainedCity.key.sessionEpoch>previousEpoch && !mayFallbackSceneFrame(s,reload),"failed replacement cannot restore stale identity");
    auto closed=changeSceneFrame(s,SceneFrameChange::Close).state;
    need(checkSceneFrameTicket(closed,retainedCity)==SceneFrameStatus::Closed && !closed.pending,"close invalidates pending callbacks");
    for(auto change:{SceneFrameChange::BeginSession,SceneFrameChange::Camera,SceneFrameChange::Resources,SceneFrameChange::Cancel,SceneFrameChange::Close}) {
        auto r=changeSceneFrame(closed,change);
        need(r.status==SceneFrameStatus::Closed && same(r.state,closed),"closed owner cannot reopen/reset");
    }
    need(requestSceneFrame(closed,7,23,816,620).status==SceneFrameStatus::Closed,"closed submit refused");
    need(!mayFallbackSceneFrame(closed,retainedCity),"shutdown failure cannot trigger CPU work");

    group="identity_and_target";
    s=changeSceneFrame({},SceneFrameChange::BeginSession).state;
    a=request(s);
    const std::array<SceneFrameStatus,8> reasons={SceneFrameStatus::OldSession,SceneFrameStatus::OldRevision,
        SceneFrameStatus::OldScene,SceneFrameStatus::OldCamera,SceneFrameStatus::OldResources,
        SceneFrameStatus::OldRequest,SceneFrameStatus::TargetMismatch,SceneFrameStatus::TargetMismatch};
    for(unsigned field=0;field<reasons.size();++field) {
        auto other=a;
        switch(field) {
            case 0:++other.key.sessionEpoch;break; case 1:++other.key.revision;break;
            case 2:++other.key.sceneDigest;break; case 3:++other.key.cameraSeq;break;
            case 4:++other.key.cacheGeneration;break; case 5:++other.requestSerial;break;
            case 6:++other.width;break; case 7:++other.height;break;
        }
        need(checkSceneFrameTicket(s,other)==reasons[field],"each identity/output field independently checked");
        auto rejected=consumeSceneFrame(s,other,other.width,other.height);
        need(rejected.status==reasons[field] && same(rejected.state,s),"mismatched result cannot consume current intent");
        need(!mayFallbackSceneFrame(s,other),"mismatched error never CPU fallbacks");
    }
    for(auto dim:std::array<std::pair<uint32_t,uint32_t>,6>{{{0,620},{816,0},{2049,1},{1,2049},{UINT32_MAX,1},{1,UINT32_MAX}}}) {
        auto bad=requestSceneFrame(s,7,23,dim.first,dim.second);
        need(bad.status==SceneFrameStatus::InvalidTarget && !bad.state.pending,"invalid new intent cancels previous pending frame");
        need(!mayFallbackSceneFrame(bad.state,a),"invalid new request cannot revive old request");
        need(s.pending==a,"pure rejection leaves caller's input untouched");
    }
    for(auto dim:std::array<std::pair<uint32_t,uint32_t>,3>{{{1,1},{2048,2048},{127,95}}}) {
        auto next=requestSceneFrame(s,UINT64_MAX,UINT64_MAX,dim.first,dim.second);
        need(next.status==SceneFrameStatus::Current,"valid target limits and opaque revision/digest");
        auto t=*next.state.pending;
        need(consumeSceneFrame(next.state,t,dim.first,dim.second).status==SceneFrameStatus::Current,"boundary dimensions accepted");
    }
    // Cache LRU eviction is not an invalidation event: identities/pixels remain owned.
    need(checkSceneFrameTicket(s,a)==SceneFrameStatus::Current,"no lifecycle change means owned frame remains eligible");
}
void completionPermutations() {
    group="completion_permutations";
    std::array<unsigned,4> order={0,1,2,3};
    do {
        SceneFrameGateState s=changeSceneFrame({},SceneFrameChange::BeginSession).state;
        std::array<SceneFrameTicket,4> tickets;
        tickets[0]=request(s,1,100);
        tickets[1]=request(s,2,101);
        s=changeSceneFrame(s,SceneFrameChange::Camera).state;
        tickets[2]=request(s,2,101);
        s=changeSceneFrame(s,SceneFrameChange::BeginSession).state;
        tickets[3]=request(s,1,100); // earlier revision/digest deliberately reused in another city.
        unsigned installs=0; uint64_t visible=999;
        for(auto index:order) {
            const auto before=s; const auto t=tickets[index];
            need(mayFallbackSceneFrame(s,t)==(index==3 && installs==0),"only final intent failure may fallback");
            auto r=consumeSceneFrame(s,t,t.width,t.height);
            const bool admitted=r.status==SceneFrameStatus::Current;
            need(admitted==(index==3 && installs==0),"out-of-order callback cannot install wrong city");
            if(admitted) { ++installs; visible=index; }
            else need(same(before,r.state),"stale completion does not cancel newest intent");
            s=r.state;
        }
        need(installs==1 && visible==3,"last intent installed exactly once for any completion order");
        ++permutations;
    } while(std::next_permutation(order.begin(),order.end()));
}
void exhaustiveHistories() {
    group="short_histories";
    // Independent oracle holds only the newest issued token's VECTOR INDEX.
    // No FrameKey comparisons or counter logic is duplicated in this model.
    // All length-six histories over request, cancel, camera, resource, reload.
    constexpr unsigned alphabet=5, length=6, count=15625;
    for(unsigned encoded=0;encoded<count;++encoded) {
        auto s=changeSceneFrame({},SceneFrameChange::BeginSession).state;
        std::vector<SceneFrameTicket> issued;
        std::optional<size_t> newest;
        unsigned code=encoded;
        for(unsigned step=0;step<length;++step) {
            const auto action=code%alphabet; code/=alphabet;
            if(action==0) {
                auto r=requestSceneFrame(s,5,8,127,95); s=r.state;
                need(r.status==SceneFrameStatus::Current,"history request succeeds");
                issued.push_back(*s.pending); newest=issued.size()-1;
            } else {
                constexpr SceneFrameChange changes[]={SceneFrameChange::Cancel,SceneFrameChange::Cancel,
                    SceneFrameChange::Camera,SceneFrameChange::Resources,SceneFrameChange::BeginSession};
                s=changeSceneFrame(s,changes[action]).state; newest.reset();
            }
            const auto snapshot=s;
            for(size_t i=0;i<issued.size();++i) {
                const bool expected=newest && *newest==i;
                const auto& t=issued[i];
                need(mayFallbackSceneFrame(s,t)==expected,"history fallback matches newest-only oracle");
                auto r=consumeSceneFrame(s,t,127,95);
                need((r.status==SceneFrameStatus::Current)==expected,"history admission matches newest-only oracle");
                if(expected) need(!mayFallbackSceneFrame(r.state,t),"consumed result cannot replay");
                else need(same(r.state,s),"stale outcome leaves newest pending unchanged");
            }
            need(same(snapshot,s),"history queries and reducers never mutate input");
        }
        ++histories;
    }
}
void exhaustionCases() {
    group="counter_exhaustion";
    // Value states allow direct arithmetic-boundary fixtures without issuing 2^64 requests.
    for(unsigned which=0;which<4;++which) {
        auto s=changeSceneFrame({},SceneFrameChange::BeginSession).state;
        const uint64_t nearMax=UINT64_MAX-1;
        if(which==0)s.sessionEpoch=nearMax;
        if(which==1)s.cameraSeq=nearMax;
        if(which==2)s.cacheGeneration=nearMax;
        if(which==3)s.requestSerial=nearMax;
        constexpr SceneFrameChange events[]={SceneFrameChange::BeginSession,SceneFrameChange::Camera,SceneFrameChange::Resources};
        if(which<3) {
            auto r=changeSceneFrame(s,events[which]);
            need(r.status==SceneFrameStatus::Current,"largest lifecycle counter is valid"); s=r.state;
        }
        auto last=request(s);
        need((which==0?last.key.sessionEpoch:which==1?last.key.cameraSeq:which==2?last.key.cacheGeneration:last.requestSerial)==UINT64_MAX,"max counter issued without wrap");
        need(checkSceneFrameTicket(s,last)==SceneFrameStatus::Current,"max counter ticket remains valid before exhaustion");
        auto exhausted=which<3?changeSceneFrame(s,events[which]):requestSceneFrame(s,7,23,816,620);
        need(exhausted.status==SceneFrameStatus::Exhausted && !exhausted.state.pending,"overflow fails closed and clears pending");
        s=exhausted.state;
        need(!mayFallbackSceneFrame(s,last),"overflow cannot revive the last ticket");
        need(consumeSceneFrame(s,last,816,620).status==SceneFrameStatus::Exhausted,"completion after overflow refused");
        for(auto event:{SceneFrameChange::BeginSession,SceneFrameChange::Camera,SceneFrameChange::Resources,SceneFrameChange::Cancel,SceneFrameChange::Close})
            need(same(changeSceneFrame(s,event).state,s),"exhaustion terminal for owner lifetime");
        need(same(requestSceneFrame(s,7,23,816,620).state,s),"request cannot reset exhausted counters");
    }
}
}
int main() {
    try {
        directCases(); completionPermutations(); exhaustiveHistories(); exhaustionCases();
        std::cout<<"{\"authored_checks\":"<<checks<<",\"passed\":true,\"completion_permutations\":"<<permutations
            <<",\"short_histories\":"<<histories<<",\"original_data_progress\":false,\"App_executed\":false,\"groups\":{";
        bool comma=false; for(const auto& pair:groups) { if(comma)std::cout<<','; comma=true; std::cout<<'"'<<pair.first<<"\":"<<pair.second; }
        std::cout<<"}}\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<group<<": "<<e.what()<<'\n'; return 1; }
}
