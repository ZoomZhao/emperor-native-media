// SPDX-License-Identifier: MIT
#pragma once
// Lossless, versioned text codec for one persisted live Well walker (type28)
// OriginalFigure value. Pure: no IO, no shared services, no context. Every
// OriginalFigure member is written in one fixed explicit order (no memcpy, no
// padding/ABI dependence). Decoding never fills an absent field; any missing,
// renamed, reordered, duplicated, type-width-overflowing or trailing data
// refuses. No semantic bounds are imposed beyond the live type28 Figure payload
// policy; the runtime bridge validates references and path state. Callers own
// shared route/RNG/cell-list state.
#include "original_figure.hpp"
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace game_lab {
inline constexpr unsigned nativeWellWalkerCodecVersion=1;
inline constexpr const char* nativeWellWalkerCodecMagic="NATIVE-WELL-WALKER";
inline constexpr size_t nativeWellWalkerCodecMaxBytes=16384;

namespace native_walker_detail {
// Single field list shared by encoder, decoder and counter: order is the format.
template<class V,class F> void visitWellWalker(V& v,F& f){
    v("slot",f.slot);v("recordOffset",f.recordOffset);
    v("image",f.image);v("secondaryImage",f.secondaryImage);v("cell",f.cell);
    v("x",f.x);v("y",f.y);v("subX",f.subX);v("subY",f.subY);v("extraX",f.extraX);v("extraY",f.extraY);
    v("next",f.next);
    v("type",f.type);v("alive",f.alive);v("frame",f.frame);v("action",f.action);v("progress",f.progress);
    v("direction",f.direction);v("previousDirection",f.previousDirection);
    v("freeMovement",f.freeMovement);v("height",f.height);v("hidden",f.hidden);v("drawClass",f.drawClass);
    v("state85",f.state85);v("raise",f.raise);
    v("subtype",f.subtype);v("attackDirection",f.attackDirection);v("attackFrame",f.attackFrame);
    v("resource",f.resource);v("cartStyle",f.cartStyle);v("stationary",f.stationary);
    v("counter",f.counter);v("owner",f.owner);v("variant",f.variant);v("amount",f.amount);
    v("leaderLink",f.leaderLink);v("target",f.target);
    v("faction",f.faction);v("employmentAmount",f.employmentAmount);v("watchCoverage",f.watchCoverage);
    v("effectKind",f.effectKind);v("effectVariant",f.effectVariant);v("effectFlags",f.effectFlags);
    v("effectX",f.effectX);v("effectY",f.effectY);v("effectVelocityX",f.effectVelocityX);v("effectVelocityY",f.effectVelocityY);
    v("effectHeight",f.effectHeight);v("effectVelocityZ",f.effectVelocityZ);v("effectDraw",f.effectDraw);
    v("effectAnimation",f.effectAnimation);v("effectFrame",f.effectFrame);v("updateOwner",f.updateOwner);
    v("threat",f.threat);v("threatDistance",f.threatDistance);v("herdTarget",f.herdTarget);
    v("herdDistance",f.herdDistance);v("idleCounter",f.idleCounter);
    v("cooldown0",f.cooldowns[0]);v("cooldown1",f.cooldowns[1]);v("cooldown2",f.cooldowns[2]);
    v("linkedBuilding",f.linkedBuilding);v("movementMode",f.movementMode);v("attackerCount",f.attackerCount);
    v("route",f.route);v("routeCursor",f.routeCursor);v("routeLength",f.routeLength);
    v("destinationX",f.destinationX);v("destinationY",f.destinationY);
    v("destinationCell",f.destinationCell);v("age",f.age);v("priorProgress",f.priorProgress);
    v("reciprocalTarget",f.reciprocalTarget);v("follower",f.follower);
    v("routePhase",f.routePhase);v("speed",f.speed);
    v("priorX",f.priorX);v("priorY",f.priorY);v("homeX",f.homeX);v("homeY",f.homeY);v("home",f.home);v("delivery",f.delivery);
    v("priorCell",f.priorCell);v("flags120",f.flags120);v("water",f.water);
    v("roamLimit",f.roamLimit);v("roamSteps",f.roamSteps);
    v("roamState",f.roamState);v("roamTurns",f.roamTurns);v("roamTurn",f.roamTurn);v("roamCountdown",f.roamCountdown);v("searchCounter",f.searchCounter);
    v("roamOriginX",f.roamOriginX);v("roamOriginY",f.roamOriginY);
    v("freeTargetX",f.freeTargetX);v("freeTargetY",f.freeTargetY);v("freeRemainingX",f.freeRemainingX);
    v("freeRemainingY",f.freeRemainingY);v("freeError",f.freeError);
    v("freeAxis",f.freeAxis);v("cellMovement",f.cellMovement);v("previousAction",f.previousAction);
    v("allocationSerial",f.allocationSerial);v("allocationPhase",f.allocationPhase);v("allocationPhaseCopy",f.allocationPhaseCopy);
    v("projectile",f.projectile);
    v("projectileDistance",f.projectileDistance);v("projectileStartHeight",f.projectileStartHeight);
    v("projectileTargetHeight",f.projectileTargetHeight);v("projectileEndHeight",f.projectileEndHeight);
    v("attacker0",f.attackers[0]);v("attacker1",f.attackers[1]);
    v("damage",f.damage);v("projectileHits",f.projectileHits);v("formation",f.formation);
    v("formationOrdinal",f.formationOrdinal);v("formationStationary",f.formationStationary);
    v("runtimeClass",f.runtimeClass);
}
struct Counter{size_t n=0;template<class T> void operator()(const char*,const T&){++n;}};
struct Writer{
    std::string out;
    template<class T> void operator()(const char* name,const T& value){
        out+=name;out+=' ';
        if constexpr(std::is_same_v<T,std::optional<int32_t>>){if(value)out+=std::to_string(*value);else out+='-';}
        else if constexpr(std::is_same_v<T,bool>)out+=value?'1':'0';
        else if constexpr(std::is_same_v<T,OriginalFigureClass>)out+=std::to_string(int(value));
        else if constexpr(std::is_signed_v<T>)out+=std::to_string(int64_t(value));
        else out+=std::to_string(uint64_t(value));
        out+='\n';
    }
};
template<class T> bool parseInteger(std::string_view token,T& out){
    if(token.empty()||token.size()>20)return false;
    // Canonical decimal only: no '+', no leading zeros, no "-0".
    const bool negative=token.front()=='-';const std::string_view digits=negative?token.substr(1):token;
    if(digits.empty()||(digits.size()>1&&digits.front()=='0')||(negative&&digits=="0"))return false;
    if constexpr(std::is_signed_v<T>){int64_t v=0;const auto r=std::from_chars(token.data(),token.data()+token.size(),v);
        if(r.ec!=std::errc{}||r.ptr!=token.data()+token.size()||v<int64_t(std::numeric_limits<T>::min())||v>int64_t(std::numeric_limits<T>::max()))return false;out=T(v);}
    else {if(negative)return false;uint64_t v=0;const auto r=std::from_chars(token.data(),token.data()+token.size(),v);
        if(r.ec!=std::errc{}||r.ptr!=token.data()+token.size()||v>uint64_t(std::numeric_limits<T>::max()))return false;out=T(v);}
    return true;
}
struct Reader{
    std::vector<std::pair<std::string_view,std::string_view>> lines;size_t at=0;std::string error;
    template<class T> void operator()(const char* name,T& value){
        if(!error.empty())return;
        if(at>=lines.size()){error=std::string("missing_field:")+name;return;}
        const auto& [key,token]=lines[at++];
        if(key!=name){error=std::string("field_order:")+name;return;}
        bool ok=false;
        if constexpr(std::is_same_v<T,std::optional<int32_t>>){if(token=="-"){value.reset();ok=true;}else{int32_t v=0;ok=parseInteger(token,v);if(ok)value=v;}}
        else if constexpr(std::is_same_v<T,bool>){ok=token=="0"||token=="1";if(ok)value=token=="1";}
        else if constexpr(std::is_same_v<T,OriginalFigureClass>){ok=token=="0"||token=="1"||token=="2";if(ok)value=OriginalFigureClass(token[0]-'0');}
        else ok=parseInteger(token,value);
        if(!ok)error=std::string("invalid_value:")+name;
    }
};
inline size_t fieldCount(){OriginalFigure f;Counter c;visitWellWalker(c,f);return c.n;}
// Payload policy only: this codec persists live type28 Figure-class records.
// Every other member is lossless over its declared type. Path-specific checks
// (pool slot/next/owner/cell/route references, action/direction handling,
// graph and admission consistency) belong to the runtime bridge, not here:
// e.g. the strict Well update normalizes any action outside1/2/4/5 to1.
inline std::string validate(const OriginalFigure& f){
    if(f.type!=28)return "not_well_walker_type28";
    if(f.alive!=1)return "not_live_walker";
    if(f.runtimeClass!=OriginalFigureClass::Figure)return "runtime_class_not_figure";
    return {};
}
} // namespace native_walker_detail

inline std::optional<std::string> encodeNativeWellWalker(const OriginalFigure& figure,std::string& error){
    using namespace native_walker_detail;
    error=validate(figure);if(!error.empty())return std::nullopt;
    Writer w;w.out=std::string(nativeWellWalkerCodecMagic)+' '+std::to_string(nativeWellWalkerCodecVersion)+' '+std::to_string(fieldCount())+'\n';
    auto copy=figure;visitWellWalker(w,copy);
    if(w.out.size()>nativeWellWalkerCodecMaxBytes){error="encoded_size_limit";return std::nullopt;}
    return w.out;
}
inline std::optional<OriginalFigure> decodeNativeWellWalker(const std::string& bytes,std::string& error){
    using namespace native_walker_detail;
    error.clear();
    if(bytes.size()>nativeWellWalkerCodecMaxBytes){error="input_size_limit";return std::nullopt;}
    if(bytes.empty()||bytes.back()!='\n'){error="missing_final_newline";return std::nullopt;}
    if(bytes.find('\0')!=std::string::npos||bytes.find('\r')!=std::string::npos){error="invalid_character";return std::nullopt;}
    std::vector<std::string_view> rows;
    {const std::string_view all(bytes);size_t start=0;
        while(start<all.size()){const size_t end=all.find('\n',start);rows.push_back(all.substr(start,end-start));start=end+1;}}
    const std::string expectedHeader=std::string(nativeWellWalkerCodecMagic)+' '+std::to_string(nativeWellWalkerCodecVersion)+' '+std::to_string(fieldCount());
    if(rows.empty()||rows.front()!=expectedHeader){
        error=rows.empty()||rows.front().rfind(std::string(nativeWellWalkerCodecMagic)+' ',0)!=0?"bad_magic":"unsupported_version_or_field_count";return std::nullopt;}
    Reader r;
    for(size_t n=1;n<rows.size();++n){const auto row=rows[n];const size_t space=row.find(' ');
        if(space==std::string_view::npos||space==0||row.find(' ',space+1)!=std::string_view::npos){error="malformed_line";return std::nullopt;}
        r.lines.emplace_back(row.substr(0,space),row.substr(space+1));}
    if(r.lines.size()!=fieldCount()){error=r.lines.size()<fieldCount()?"missing_fields":"trailing_data";return std::nullopt;}
    OriginalFigure f;visitWellWalker(r,f);
    if(!r.error.empty()){error=r.error;return std::nullopt;}
    if(r.at!=r.lines.size()){error="trailing_data";return std::nullopt;}
    error=validate(f);if(!error.empty())return std::nullopt;
    return f;
}

// Separate identity: never disguise an Inspector as a Well walker.
inline constexpr unsigned nativeInspectorCodecVersion=1;
inline constexpr const char* nativeInspectorCodecMagic="NATIVE-INSPECTOR";
inline constexpr size_t nativeInspectorCodecMaxBytes=16384;
namespace native_walker_detail {
inline std::string validateInspector(const OriginalFigure& f){
    if(f.type!=39||f.alive!=1||f.runtimeClass!=OriginalFigureClass::Figure)return "inspection_figure_identity";
    if(!f.slot||f.slot>=2000||f.owner<=0||f.owner>=4000||f.action<6||f.action>9)return "inspection_figure_action_or_owner";
    return {};
}
}
inline std::optional<std::string> encodeNativeInspector(const OriginalFigure& figure,std::string& error){
    using namespace native_walker_detail;
    error=validateInspector(figure);if(!error.empty())return std::nullopt;
    Writer w;w.out=std::string(nativeInspectorCodecMagic)+' '+std::to_string(nativeInspectorCodecVersion)+' '+std::to_string(fieldCount())+'\n';
    auto copy=figure;visitWellWalker(w,copy);
    if(w.out.size()>nativeInspectorCodecMaxBytes){error="encoded_size_limit";return std::nullopt;}
    return w.out;
}
inline std::optional<OriginalFigure> decodeNativeInspector(const std::string& bytes,std::string& error){
    using namespace native_walker_detail;
    error.clear();
    if(bytes.size()>nativeInspectorCodecMaxBytes){error="input_size_limit";return std::nullopt;}
    if(bytes.empty()||bytes.back()!='\n'){error="missing_final_newline";return std::nullopt;}
    if(bytes.find('\0')!=std::string::npos||bytes.find('\r')!=std::string::npos){error="invalid_character";return std::nullopt;}
    std::vector<std::string_view> rows;
    {const std::string_view all(bytes);size_t start=0;
        while(start<all.size()){const size_t end=all.find('\n',start);rows.push_back(all.substr(start,end-start));start=end+1;}}
    const std::string expectedHeader=std::string(nativeInspectorCodecMagic)+' '+std::to_string(nativeInspectorCodecVersion)+' '+std::to_string(fieldCount());
    if(rows.empty()||rows.front()!=expectedHeader){
        error=rows.empty()||rows.front().rfind(std::string(nativeInspectorCodecMagic)+' ',0)!=0?"bad_magic":"unsupported_version_or_field_count";return std::nullopt;}
    Reader r;
    for(size_t n=1;n<rows.size();++n){const auto row=rows[n];const size_t space=row.find(' ');
        if(space==std::string_view::npos||space==0||row.find(' ',space+1)!=std::string_view::npos){error="malformed_line";return std::nullopt;}
        r.lines.emplace_back(row.substr(0,space),row.substr(space+1));}
    if(r.lines.size()!=fieldCount()){error=r.lines.size()<fieldCount()?"missing_fields":"trailing_data";return std::nullopt;}
    OriginalFigure f;visitWellWalker(r,f);
    if(!r.error.empty()){error=r.error;return std::nullopt;}
    if(r.at!=r.lines.size()){error="trailing_data";return std::nullopt;}
    error=validateInspector(f);if(!error.empty())return std::nullopt;
    return f;
}

// A separate type13 policy and magic reuse the exact same 118 named members.
// Home/link, pool, route and graph references remain bridge responsibilities.
inline constexpr unsigned nativeMigrantCodecVersion=1;
inline constexpr const char* nativeMigrantCodecMagic="NATIVE-MIGRANT";
inline constexpr size_t nativeMigrantCodecFieldCount=118;
inline constexpr size_t nativeMigrantCodecMaxBytes=16384;
namespace native_walker_detail {
inline std::string validateMigrant(const OriginalFigure& f){
    if(f.type!=13)return "not_migrant_type13";
    if(f.alive!=1)return "not_live_migrant";
    if(f.runtimeClass!=OriginalFigureClass::Figure)return "runtime_class_not_figure";
    if(f.action<6||f.action>9)return "unsupported_migrant_action";
    if(f.owner!=0)return "migrant_owner_not_zero";
    return {};
}
}
inline std::optional<std::string> encodeNativeMigrant(const OriginalFigure& figure,std::string& error){
    using namespace native_walker_detail;
    error=validateMigrant(figure);if(!error.empty())return std::nullopt;
    if(fieldCount()!=nativeMigrantCodecFieldCount){error="field_schema_changed";return std::nullopt;}
    Writer w;w.out=std::string(nativeMigrantCodecMagic)+' '+std::to_string(nativeMigrantCodecVersion)+' '+std::to_string(nativeMigrantCodecFieldCount)+'\n';
    auto copy=figure;visitWellWalker(w,copy);
    if(w.out.size()>nativeMigrantCodecMaxBytes){error="encoded_size_limit";return std::nullopt;}
    return w.out;
}
inline std::optional<OriginalFigure> decodeNativeMigrant(const std::string& bytes,std::string& error){
    using namespace native_walker_detail;error.clear();
    if(bytes.size()>nativeMigrantCodecMaxBytes){error="input_size_limit";return std::nullopt;}
    if(bytes.empty()||bytes.back()!='\n'){error="missing_final_newline";return std::nullopt;}
    if(bytes.find('\0')!=std::string::npos||bytes.find('\r')!=std::string::npos){error="invalid_character";return std::nullopt;}
    if(fieldCount()!=nativeMigrantCodecFieldCount){error="field_schema_changed";return std::nullopt;}
    std::vector<std::string_view> rows;const std::string_view all(bytes);size_t start=0;
    while(start<all.size()){const size_t end=all.find('\n',start);rows.push_back(all.substr(start,end-start));start=end+1;}
    const std::string expected=std::string(nativeMigrantCodecMagic)+' '+std::to_string(nativeMigrantCodecVersion)+' '+std::to_string(nativeMigrantCodecFieldCount);
    if(rows.empty()||rows.front()!=expected){error=rows.empty()||rows.front().rfind(std::string(nativeMigrantCodecMagic)+' ',0)!=0?"bad_magic":"unsupported_version_or_field_count";return std::nullopt;}
    Reader r;
    for(size_t n=1;n<rows.size();++n){const auto row=rows[n];const size_t space=row.find(' ');
        if(space==std::string_view::npos||space==0||row.find(' ',space+1)!=std::string_view::npos){error="malformed_line";return std::nullopt;}
        r.lines.emplace_back(row.substr(0,space),row.substr(space+1));}
    if(r.lines.size()!=nativeMigrantCodecFieldCount){error=r.lines.size()<nativeMigrantCodecFieldCount?"missing_fields":"trailing_data";return std::nullopt;}
    OriginalFigure f;visitWellWalker(r,f);
    if(!r.error.empty()){error=r.error;return std::nullopt;}
    if(r.at!=r.lines.size()){error="trailing_data";return std::nullopt;}
    error=validateMigrant(f);if(!error.empty())return std::nullopt;
    return f;
}
} // namespace game_lab
