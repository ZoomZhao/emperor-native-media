// SPDX-License-Identifier: MIT
#pragma once
#include "emperor_assets.hpp"
#include <array>
#include <atomic>
#include <limits>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <fcntl.h>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
namespace game_lab {
// Process-local immutable resource identity. Cached hits retain the identity;
// eviction/redecode or file-version change obtains a new one. Lists retain the
// pixels themselves, not an archive pointer or a recyclable memory address.
struct SG3DecodedSprite {uint64_t identity=0;std::shared_ptr<const SG3DecodeResult> value;};
inline SG3DecodedSprite identifySG3Sprite(std::shared_ptr<const SG3DecodeResult> value){
    static std::atomic<uint64_t> sequence{0};uint64_t id=0;
    if(value&&value->image.valid()){
        auto previous=sequence.load(std::memory_order_relaxed);
        while(previous!=std::numeric_limits<uint64_t>::max()){
            if(sequence.compare_exchange_weak(previous,previous+1,std::memory_order_relaxed)){id=previous+1;break;}
        }
    }
    return {id,std::move(value)};
}
struct SG3CacheStats {
    uint64_t frames=0,requests=0,hits=0,decodes=0,evictions=0,invalidations=0;
    uint64_t resolveNs=0,validateNs=0,decodeNs=0,pathResolutions=0,pathHits=0;
    size_t bytes=0,peakBytes=0,entries=0;
};
// Synchronous, single-owner cache. Begin once per complete render. Pixels are
// immutable and shared with the bounded frame-local caches until that draw ends.
// Budget covers retained pixel capacity; entry count independently bounds metadata.
class SG3SpriteCache {
    struct Stamp {
        uint64_t device=0,inode=0,size=0;
        int64_t mt=0,mn=0,ct=0,cn=0;
        bool operator==(const Stamp& b)const{return std::tie(device,inode,size,mt,mn,ct,cn)==std::tie(b.device,b.inode,b.size,b.mt,b.mn,b.ct,b.cn);}
    };
    struct Key {
        std::string path;std::array<int64_t,12> fields{};
        bool operator<(const Key& b)const{return std::tie(path,fields)<std::tie(b.path,b.fields);}
    };
    struct Entry {Stamp stamp;SG3DecodedSprite resource;size_t bytes;std::list<Key>::iterator age;};
    using Clock=std::chrono::steady_clock;
    const size_t budget_,entryLimit_;
    std::list<Key> ages_;std::map<Key,Entry> entries_;
    std::map<std::string,std::optional<Stamp>> frameFiles_;
    SG3CacheStats stats_;SG3CompanionMemo companionMemo_;
    static uint64_t ns(Clock::time_point a){return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-a).count();}
    static std::optional<Stamp> stamp(const std::string& path){
        const int fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC);if(fd<0)return std::nullopt;
        struct stat s{};const bool ok=::fstat(fd,&s)==0&&S_ISREG(s.st_mode)&&s.st_size>=0;::close(fd);if(!ok)return std::nullopt;
#if defined(__APPLE__)
        return Stamp{uint64_t(s.st_dev),uint64_t(s.st_ino),uint64_t(s.st_size),s.st_mtimespec.tv_sec,s.st_mtimespec.tv_nsec,s.st_ctimespec.tv_sec,s.st_ctimespec.tv_nsec};
#else
        return Stamp{uint64_t(s.st_dev),uint64_t(s.st_ino),uint64_t(s.st_size),s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};
#endif
    }
    void erase(std::map<Key,Entry>::iterator i){stats_.bytes-=i->second.bytes;ages_.erase(i->second.age);entries_.erase(i);stats_.entries=entries_.size();}
public:
    explicit SG3SpriteCache(size_t bytes=32u*1024u*1024u,size_t entries=2048):budget_(bytes),entryLimit_(entries){companionMemo_.limit=std::min(size_t(2048),entries); }
    SG3SpriteCache(const SG3SpriteCache&)=delete;SG3SpriteCache& operator=(const SG3SpriteCache&)=delete;
    void beginFrame(){frameFiles_.clear();companionMemo_.paths.clear();++stats_.frames;}
    void clear(){entries_.clear();ages_.clear();frameFiles_.clear();companionMemo_.paths.clear();stats_.bytes=stats_.entries=0;++stats_.invalidations;}
    SG3CacheStats statistics()const{auto result=stats_;result.pathResolutions=companionMemo_.resolutions;result.pathHits=companionMemo_.hits;return result;}
    size_t budget()const{return budget_;}
    SG3DecodedSprite decodeResource(const SG3Archive& archive,size_t index,const std::string& companion){
        ++stats_.requests;auto a=Clock::now();const auto source=resolveSG3Sprite(archive,index,companion,&companionMemo_);stats_.resolveNs+=ns(a);
        const auto decodeFresh=[&](){auto t=Clock::now();auto d=std::make_shared<const SG3DecodeResult>(decodeResolvedSG3Sprite(source));stats_.decodeNs+=ns(t);++stats_.decodes;return identifySG3Sprite(std::move(d));};
        if(source.status!=SG3DecodeStatus::Decoded||!budget_||!entryLimit_)return decodeFresh();
        a=Clock::now();auto file=frameFiles_.find(source.path);
        // At most entryLimit distinct dependency checks are memoized in a frame.
        const auto before=file==frameFiles_.end()?stamp(source.path):file->second;
        if(file==frameFiles_.end()){
            if(frameFiles_.size()<entryLimit_)frameFiles_.emplace(source.path,before);
            for(auto i=entries_.begin();i!=entries_.end();){auto current=i++;
                if(current->first.path==source.path&&(!before||!(current->second.stamp==*before))){erase(current);++stats_.invalidations;}}
        }
        stats_.validateNs+=ns(a);
        if(!before)return decodeFresh();
        const auto& s=source.sprite;
        // Every field consumed by pixels(), plus the resolved mirror parity.
        Key key{source.path,{{s.offset,s.length,s.uncompressedLength,s.width,s.height,s.type,s.alphaOffset,s.alphaLength,s.external,s.tileSize,source.mirrored,0}}};
        auto found=entries_.find(key);
        if(found!=entries_.end()&&found->second.stamp==*before){++stats_.hits;ages_.splice(ages_.end(),ages_,found->second.age);return found->second.resource;}
        auto decoded=decodeFresh();a=Clock::now();const auto after=stamp(source.path);stats_.validateNs+=ns(a);
        if(!after||!(*before==*after)){clear();auto failed=std::make_shared<SG3DecodeResult>();failed->status=SG3DecodeStatus::InvalidPayload;return identifySG3Sprite(std::move(failed));}
        if(!decoded.value->image.valid())return decoded; // Failures never survive a render.
        const size_t bytes=decoded.value->image.pixelsBGRA.capacity()*sizeof(uint32_t);if(bytes>budget_)return decoded;
        while(!entries_.empty()&&(stats_.bytes>budget_-bytes||entries_.size()>=entryLimit_)){erase(entries_.find(ages_.front()));++stats_.evictions;}
        ages_.push_back(key);entries_.emplace(std::move(key),Entry{*after,decoded,bytes,std::prev(ages_.end())});stats_.bytes+=bytes;stats_.entries=entries_.size();stats_.peakBytes=std::max(stats_.peakBytes,stats_.bytes);return decoded;
    }
    std::shared_ptr<const SG3DecodeResult> decode(const SG3Archive& a,size_t i,const std::string& p){return decodeResource(a,i,p).value;}
};
inline SG3DecodedSprite decodeSG3Resource(const SG3Archive& a,size_t i,const std::string& p,SG3SpriteCache* cache){
    return cache?cache->decodeResource(a,i,p):identifySG3Sprite(std::make_shared<const SG3DecodeResult>(decodeSG3Sprite(a,i,p)));
}
inline std::shared_ptr<const SG3DecodeResult> decodeSG3Shared(const SG3Archive& a,size_t i,const std::string& p,SG3SpriteCache* cache){
    return cache?cache->decode(a,i,p):std::make_shared<const SG3DecodeResult>(decodeSG3Sprite(a,i,p));
}
} // namespace game_lab
