// SPDX-License-Identifier: MIT
#pragma once
#include "original_figure.hpp"
#include "original_house_goods_contract.hpp"
#include <atomic>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace game_lab {
// Host metadata only. Never used as original RNG, serial, or business state.
struct OriginalKernelAllocation {size_t slot=0;uint16_t serial=0;};
struct OriginalKernelHandle {uint64_t epoch=0,generation=0;size_t slot=0;uint16_t serial=0;};
class OriginalKernelIdentities {
    struct Entry {uint16_t serial=0;uint8_t type=0;uint64_t generation=0;};
    uint64_t epoch_=0,next_=1;
    std::map<size_t,Entry> live_;
    static std::optional<uint64_t> epoch(){
        static std::atomic<uint64_t> value{1};auto n=value.load();
        while(n!=std::numeric_limits<uint64_t>::max())if(value.compare_exchange_weak(n,n+1))return n;
        return std::nullopt;
    }
public:
    template<class Pool> static std::optional<OriginalKernelIdentities> loaded(const Pool& pool,std::string& error){
        OriginalKernelIdentities out;const auto e=epoch();if(!e){error="kernel_load_epoch_exhausted";return std::nullopt;}out.epoch_=*e;
        for(const auto& [slot,f]:pool){if(f.slot!=slot||slot>=2000){error="kernel_identity_slot";return std::nullopt;}
            if(f.alive){if(!slot){error="kernel_identity_zero_live";return std::nullopt;}out.live_.emplace(slot,Entry{f.allocationSerial,f.type,out.next_++});}}
        error.clear();return out;
    }
    template<class Pool> std::optional<OriginalKernelIdentities> advanced(const Pool& pool,const std::vector<OriginalKernelAllocation>& events,std::string& error)const{
        auto out=*this;std::map<size_t,Entry> born;
        for(const auto& e:events){if(!e.slot||e.slot>=2000||!pool.count(e.slot)){error="kernel_allocation_event_slot";return std::nullopt;}
            if(out.next_==std::numeric_limits<uint64_t>::max()){error="kernel_generation_exhausted";return std::nullopt;}
            born[e.slot]={e.serial,pool.at(e.slot).type,out.next_++};}
        std::map<size_t,Entry> next;
        for(const auto& [slot,f]:pool){if(f.slot!=slot||slot>=2000){error="kernel_identity_slot";return std::nullopt;}if(!f.alive)continue;
            if(!slot){error="kernel_identity_zero_live";return std::nullopt;}
            const auto b=born.find(slot);const auto old=live_.find(slot);
            if(b!=born.end()){if(b->second.serial!=f.allocationSerial){error="kernel_allocation_event_serial";return std::nullopt;}next.emplace(slot,b->second);}
            else {if(old==live_.end()||old->second.serial!=f.allocationSerial){error="kernel_untracked_allocation";return std::nullopt;}
                auto entry=old->second;if(entry.type!=f.type){if(out.next_==std::numeric_limits<uint64_t>::max()){error="kernel_generation_exhausted";return std::nullopt;}entry.type=f.type;entry.generation=out.next_++;}next.emplace(slot,entry);}}
        out.live_=std::move(next);error.clear();return out;
    }
    template<class Pool> bool consistent(const Pool& pool)const{
        if(!epoch_)return false;size_t count=0;
        for(const auto& [slot,f]:pool)if(f.alive){++count;const auto i=live_.find(slot);
            if(i==live_.end()||f.slot!=slot||i->second.serial!=f.allocationSerial||i->second.type!=f.type||!i->second.generation)return false;}
        return count==live_.size();
    }
    std::optional<OriginalKernelIdentities> reloaded(std::string& error)const{
        auto out=*this;const auto e=epoch();if(!e){error="kernel_load_epoch_exhausted";return std::nullopt;}out.epoch_=*e;error.clear();return out;
    }
    std::optional<OriginalKernelHandle> handle(size_t slot)const{const auto i=live_.find(slot);if(i==live_.end())return std::nullopt;return OriginalKernelHandle{epoch_,i->second.generation,slot,i->second.serial};}
    bool matches(const OriginalKernelHandle& h)const{const auto i=live_.find(h.slot);return h.epoch==epoch_&&i!=live_.end()&&h.generation==i->second.generation&&h.serial==i->second.serial;}
};

// Structural checks only. Unknown gameplay predicates are not normalized.
// Called only at complete original-world publication boundaries, never by v6.
template<class Context> inline std::string originalKernelPoolIssue(const Context& f){
    if(f.figures.size()!=2000||f.cellHeads.size()!=51984)return "kernel_pool_dimensions";
    size_t highest=0;std::set<size_t> free;
    for(size_t slot=0;slot<2000;++slot){const auto i=f.figures.find(slot);if(i==f.figures.end()||i->second.slot!=slot)return "kernel_pool_identity";
        const auto& w=i->second;if(!w.alive)continue;if(!slot||w.cell>=51984||w.next>=2000)return "kernel_live_figure_bounds";highest=slot;}
    if(highest!=f.highestLiveSlot)return "kernel_highest_live_mismatch";
    for(size_t slot:f.freeSlots)if(!slot||slot>=2000||!free.insert(slot).second||f.figures.at(slot).alive)return "kernel_free_fifo_mismatch";
    std::set<size_t> linked;
    for(size_t cell=0;cell<f.cellHeads.size();++cell){auto slot=f.cellHeads[cell];if(slot>=2000)return "kernel_cell_head_bounds";
        while(slot){const auto& actor=f.figures.at(slot);
            if(!actor.alive||!linked.insert(slot).second||originalFigureDrawCell(actor)!=cell)return "kernel_cell_chain_mismatch";
            slot=actor.next;}}
    return {};
}
template<class City> inline std::string originalKernelAuthorityIssue(const City& city){
    if(city.phase>50)return "kernel_phase_bounds";
    if(auto e=originalKernelPoolIssue(city.figures);!e.empty())return e;
    if(!city.buildings.empty()&&!city.figures.buildings)return "kernel_building_links_missing";
    for(const auto& [slot,b]:city.buildings){if(!slot||slot>=4000||b.state.building.slot!=slot)return "kernel_building_identity";
        const auto i=city.figures.buildings->find(slot);if(i==city.figures.buildings->end())return "kernel_building_link_missing";
        const auto& l=i->second;const auto& s=b.state;
        if(s.building.type!=l.type||s.inputs.alive!=l.alive||s.inputs.primaryFigure!=l.primaryFigure||s.inputs.migrant!=l.worker||s.inputs.workers!=l.workers)
            return "kernel_building_authority_mismatch";
        if(s.inputs.distance!=l.roadDistance||s.inputs.roadX!=l.roadX||s.inputs.roadY!=l.roadY||s.inputs.upgrade!=l.upgrade||
           s.inputs.fireRisk!=l.fireRisk||s.inputs.damageRisk!=l.damageRisk||s.inputs.watchRisk!=l.watchRisk||s.inputs.stock!=l.industryInput||
           s.inputs.direction!=l.roamDirection||s.inputs.operational!=l.serviceCount||s.inputs.enabled!=l.destinationEnabled||
           s.inputs.output!=l.outputResource||s.inputs.available!=l.roadAccess||s.inputs.network!=l.category)return "kernel_building_shared_fields_mismatch";
        if(l.type>=2&&l.type<18){if(s.inputs.model!=l.houseModel||s.inputs.residents!=l.residents||s.inputs.free!=l.freeSlots)return "kernel_house_authority_mismatch";
            if(s.inputs.house&&l.houseGoods&&originalHouseGoodsFromPayload(*s.inputs.house)!=l.houseGoods)return "kernel_house_goods_authority_mismatch";}}
    return {};
}
} // namespace game_lab
