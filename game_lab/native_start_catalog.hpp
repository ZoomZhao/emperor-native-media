// SPDX-License-Identifier: MIT
#pragma once
#include "native_session_source.hpp"
#include "native_minimap.hpp"
#include <atomic>
#include <functional>
#include <memory>
namespace game_lab {
// Catalog admission means data-supported, not a promise that arbitrary external
// assets or every supplied city have passed GUI acceptance. Rendering still gates publication.
struct NativeStartEntry {
    std::string source,label,reason;
    NativeSourceMetadata metadata;
    uint64_t sourceRevision=0,modelRevision=0;
    bool supported()const{return reason.empty()&&sourceRevision&&modelRevision;}
};
struct NativeStartCatalog {std::vector<NativeStartEntry> entries;size_t omitted=0;};
// A fresh shared token is a generation identity. Workers keep their own token
// alive, so cancelling/reopening cannot reuse it (no raw-pointer ABA).
struct NativeStartWork {std::atomic<bool> cancelled{false};};
using NativeStartWorkPtr=std::shared_ptr<NativeStartWork>;
inline bool nativeStartWorkCurrent(const NativeStartWorkPtr& result,const NativeStartWorkPtr& current){
    return result&&result==current&&!result->cancelled.load();
}
inline std::optional<NativeStartCatalog> loadNativeStartCatalog(const ResourceRoot& root,std::vector<std::string> paths,const NativeStartWorkPtr& work,
        const std::function<void(size_t,size_t)>& progress={}){
    auto cancelled=[&]{return work&&work->cancelled.load();};
    if(cancelled())return {};
    std::sort(paths.begin(),paths.end());paths.erase(std::unique(paths.begin(),paths.end()),paths.end());
    NativeStartCatalog out;constexpr size_t limit=128;
    if(paths.size()>limit){out.omitted=paths.size()-limit;paths.resize(limit);}
    if(progress)progress(0,paths.size());
    for(const auto& path:paths){
        if(cancelled())return {}; // Do not begin another read after cancellation.
        NativeStartEntry e;e.source=path;
        const auto file=std::filesystem::path(path);e.label=file.filename().string()+" · "+file.parent_path().generic_string();
        const auto s=loadNativeSessionSource(root,path,e.reason,&e.metadata);
        if(cancelled())return {}; // An in-progress OS read/decode is not forcibly interrupted.
        if(s){e.reason=validateNativeSession(s->seed,startNativeSession(s->seed,nativeLatestRulesVersion));if(e.reason.empty()){e.sourceRevision=s->seed.sourceRevision;e.modelRevision=s->seed.modelRevision;}}
        out.entries.push_back(std::move(e));
        if(progress)progress(out.entries.size(),paths.size());
    }
    if(cancelled())return {}; // No partial catalog publication, even at the last row.
    return out;
}
inline NativeStartCatalog nativeStartCatalog(const ResourceRoot& root,std::vector<std::string> paths){
    return *loadNativeStartCatalog(root,std::move(paths),{});
}
inline bool nativeStartMatches(const NativeStartEntry& expected,const NativeSessionSource& loaded){
    return expected.supported()&&loaded.seed.source==expected.source&&loaded.seed.sourceRevision==expected.sourceRevision&&loaded.seed.modelRevision==expected.modelRevision;
}
inline std::optional<NativeSessionSource> reloadNativeStartSource(const ResourceRoot& root,const NativeStartEntry& expected,std::string& error){
    if(!expected.supported()){error=expected.reason.empty()?"尚未核验此来源。":expected.reason;return {};}
    auto source=loadNativeSessionSource(root,expected.source,error);
    if(!source)return {};
    if(!nativeStartMatches(expected,*source)){error="原始来源或建筑模型已改变；请重新打开来源列表。当前城市未替换。";return {};}
    return source;
}
inline NativeStartEntry nativeStartIdentity(const NativeSessionSource& source){
    NativeStartEntry e;e.source=source.seed.source;e.sourceRevision=source.seed.sourceRevision;e.modelRevision=source.seed.modelRevision;return e;
}
} // namespace game_lab
