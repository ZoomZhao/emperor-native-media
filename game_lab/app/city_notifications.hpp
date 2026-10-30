// SPDX-License-Identifier: MIT
#pragma once
#include "../original_building_update.hpp"
#include <atomic>
#include <sstream>

namespace game_lab {
// Native runtime transactions only. Saved records never become payable, and
// these local states do not initialize or modify original event/UI predicates.
enum class AppCityTributeState { History, Pending, Accepted, Rejected, Unsupported };
enum class AppCityTributeDecision { Accept, Reject };
enum class AppCityTributeResult { Applied, Stale, AlreadyCompleted, Unsupported, MissingLedger, Overflow, ReplayRecording };
struct AppCityTributeId {uint64_t load=0;uint16_t slot=0;};
struct AppCityTributeRecord {
    uint16_t slot=0;
    int16_t faction=0,resource=0,amount=0;
    AppCityTributeState state=AppCityTributeState::History;
};
struct AppCityTributeFinances {int32_t treasury=0,incomeOther=0,tributeIncomeComponent=0;};
struct OriginalWorldNotifications {unsigned savedCount=0,committedCount=0;std::vector<AppCityTributeRecord> saved,live;};
class AppCityNotifications {
public:
    OriginalWorldNotifications persistent()const{return {savedCount_,committedCount_,saved_,live_};}
    static bool validPersistent(const OriginalWorldNotifications& value,const OriginalBuildingScenarioMessages& messages){
        if(value.savedCount>value.committedCount||value.committedCount!=messages.count||messages.count>150||value.saved.size()+value.live.size()>150)return false;
        std::set<uint16_t> used;
        for(unsigned group=0;group<2;++group)for(const auto& row:group?value.live:value.saved){
            if(row.slot>=messages.count||!used.insert(row.slot).second||messages.records[row.slot][2]!=29||!same(row,read(messages,row.slot)))return false;
            if(group){if(row.slot<value.savedCount||row.state==AppCityTributeState::History||unsigned(row.state)>unsigned(AppCityTributeState::Unsupported)||row.resource!=29||row.faction<0||row.faction>=22||((row.state==AppCityTributeState::Unsupported)!=(row.amount<=0)))return false;}
            else if(row.slot>=value.savedCount||row.state!=AppCityTributeState::History)return false;
        }
        for(unsigned n=0;n<value.committedCount;++n)if(messages.records[n][2]==29&&!used.count(uint16_t(n)))return false;
        return true;
    }
    static std::optional<AppCityNotifications> restore(const OriginalWorldNotifications& value,const OriginalBuildingScenarioMessages& messages){
        if(!validPersistent(value,messages))return std::nullopt;const auto id=nextLoad();if(!id)return std::nullopt;
        AppCityNotifications out;out.load_=*id;out.savedCount_=value.savedCount;out.committedCount_=value.committedCount;out.saved_=value.saved;out.live_=value.live;return out;
    }
    static std::optional<AppCityNotifications> fromSaved(const OriginalBuildingScenarioMessages& messages) {
        if(messages.count>messages.records.size()||!messages.notices.empty())return std::nullopt;
        AppCityNotifications out;
        // Every load has a process-unique identity, including a new surface or
        // another resource root. Exhaustion fails rather than recycling IDs.
        const auto id=nextLoad();if(!id)return std::nullopt;
        out.load_=*id;out.savedCount_=out.committedCount_=messages.count;
        for(unsigned i=0;i<messages.count;++i)if(messages.records[i][2]==29)
            out.saved_.push_back(read(messages,i));
        return out;
    }
    // Checkpoint restores retain dispositions but invalidate pre-restore UI IDs.
    std::optional<AppCityNotifications> reloaded()const{
        const auto id=nextLoad();if(!id)return std::nullopt;auto out=*this;out.load_=*id;return out;
    }
    // Called on a candidate and published only after the whole update installs.
    // Slots are append-only in the current producer. No slot reuse is admitted.
    bool observeCommitted(const OriginalBuildingScenarioMessages& messages) {
        if(messages.count>messages.records.size()||messages.count<committedCount_||messages.notices.size()>150)return false;
        for(const auto& old:live_)if(old.slot>=messages.count||messages.records[old.slot][2]!=29||!same(old,read(messages,old.slot)))return false;
        auto next=live_;
        for(const auto& n:messages.notices){
            if(n.id!=225||n.slot<0||unsigned(n.slot)<savedCount_||unsigned(n.slot)>=messages.count)return false;
            auto row=read(messages,unsigned(n.slot));
            if(messages.records[size_t(n.slot)][2]!=29||row.resource!=29||row.faction<0||row.faction>=22||row.faction!=n.faction||row.resource!=n.resource||row.amount!=n.amount||
                n.title!=(n.amount>=1?558:560)||n.dialogFlags!=(n.amount>=1?5u:3u))return false;
            const auto found=std::find_if(next.begin(),next.end(),[&](const auto& old){return old.slot==row.slot;});
            if(found!=next.end()){
                if(!same(*found,row))return false;
            }else {row.state=row.amount>0?AppCityTributeState::Pending:AppCityTributeState::Unsupported;next.push_back(row);}
        }
        if(saved_.size()+next.size()>150)return false;
        live_=std::move(next);committedCount_=messages.count;return true;
    }
    const std::vector<AppCityTributeRecord>& saved() const {return saved_;}
    const std::vector<AppCityTributeRecord>& generated() const {return live_;}
    const AppCityTributeRecord* find(AppCityTributeId id) const {
        if(!id.load||id.load!=load_)return nullptr;
        const auto it=std::find_if(live_.begin(),live_.end(),[&](const auto& row){return row.slot==id.slot;});
        return it==live_.end()?nullptr:&*it;
    }
    std::optional<AppCityTributeId> nextPending() const {
        for(const auto& row:live_)if(row.state==AppCityTributeState::Pending)return AppCityTributeId{load_,row.slot};
        return std::nullopt;
    }
    // The only effectful API. It never writes the original record/receipt,
    // notificationPending, event slots or manager state. All preconditions and
    // checked arithmetic precede the nonthrowing ledger/status publication.
    AppCityTributeResult respond(AppCityTributeId id,AppCityTributeDecision decision,OriginalBuildingUpdateContext& city) {
        const auto* selected=find(id);if(!selected)return AppCityTributeResult::Stale;
        if(selected->state==AppCityTributeState::Accepted||selected->state==AppCityTributeState::Rejected)return AppCityTributeResult::AlreadyCompleted;
        if(selected->state!=AppCityTributeState::Pending||selected->resource!=29||selected->amount<=0||
            (decision!=AppCityTributeDecision::Accept&&decision!=AppCityTributeDecision::Reject))return AppCityTributeResult::Unsupported;
        auto& row=live_[size_t(selected-live_.data())];
        if(decision==AppCityTributeDecision::Reject){row.state=AppCityTributeState::Rejected;return AppCityTributeResult::Applied;}
        if(!city.treasury||!city.accounting||!city.accounting->tributeIncomeComponent)return AppCityTributeResult::MissingLedger;
        const int64_t money=int64_t(*city.treasury)+row.amount;
        const int64_t income=int64_t(city.accounting->incomeOther)+row.amount;
        const int64_t category=int64_t(*city.accounting->tributeIncomeComponent)+row.amount;
        const auto fits=[](int64_t v){return v>=INT32_MIN&&v<=INT32_MAX;};
        if(!fits(money)||!fits(income)||!fits(category))return AppCityTributeResult::Overflow;
        *city.treasury=int32_t(money);city.accounting->incomeOther=int32_t(income);*city.accounting->tributeIncomeComponent=int32_t(category);
        row.state=AppCityTributeState::Accepted;return AppCityTributeResult::Applied;
    }
    std::string report() const {
        std::ostringstream out;
        out<<"存档记录："<<saved_.size()<<" 条\n处理结果不可从记录判定；仅查看，不重复发放资金。\n";
        for(const auto& row:saved_)describe(out,row);
        out<<"\n本次运行新通知："<<live_.size()<<" 条\n原生处理仅适用于本次更新生成的正数资金通知；不代表完整原版事件兼容。\n";
        for(const auto& row:live_){describe(out,row);out<<"  "<<stateName(row.state)<<"\n";}
        return out.str();
    }
    static const char* stateName(AppCityTributeState state){switch(state){
        case AppCityTributeState::History:return "历史记录";case AppCityTributeState::Pending:return "待处理";
        case AppCityTributeState::Accepted:return "已接受";case AppCityTributeState::Rejected:return "已拒绝";
        case AppCityTributeState::Unsupported:return "金额不在当前支持范围";}return "未知";}
private:
    static std::optional<uint64_t> nextLoad(){static std::atomic<uint64_t> next{1};auto id=next.load();
        do{if(id==UINT64_MAX)return std::nullopt;}while(!next.compare_exchange_weak(id,id+1));return id;}
    static bool same(const AppCityTributeRecord& a,const AppCityTributeRecord& b){return a.slot==b.slot&&a.faction==b.faction&&a.resource==b.resource&&a.amount==b.amount;}
    static AppCityTributeRecord read(const OriginalBuildingScenarioMessages& m,unsigned i) {
        const auto* r=m.records[i].data();return {uint16_t(i),detail::i16(r+28),detail::i16(r+4),detail::i16(r+12),AppCityTributeState::History};
    }
    static void describe(std::ostringstream& out,const AppCityTributeRecord& row) {
        out<<"消息 "<<row.slot<<" · 来源编号 "<<row.faction<<" · ";
        if(row.resource==29)out<<"资金 ";else out<<"资源 "<<row.resource<<" · 数量 ";
        out<<row.amount<<"\n";
    }
    uint64_t load_=0;
    unsigned savedCount_=0,committedCount_=0;
    std::vector<AppCityTributeRecord> saved_,live_;
};
}
