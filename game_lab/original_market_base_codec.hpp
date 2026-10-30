// SPDX-License-Identifier: MIT
#pragma once
#include "original_evidence_fields.hpp"
#include "native_walker_codec.hpp"
#include <sstream>

namespace game_lab {
// Observation/checkpoint payload only. It cannot authorize a24/25 updater.
struct OriginalMarketKnownBase {uint64_t sourceRevision=0;EvidenceFigure figure;};
inline std::string originalMarketKnownBaseIssue(const OriginalMarketKnownBase& b){
    const auto& f=b.figure;
    if(!b.sourceRevision||!f.slot||f.slot>=2000||!f.recordComplete||f.alias||f.runtimeClass!=OriginalFigureClass::Figure)return "market_base_identity";
    if(f.field(18)!=24&&f.field(18)!=25)return "market_base_type";
    if(f.fields.size()!=std::size(emperor::file_io::research::figure7_fields))return "market_base_field_count";
    size_t n=0;for(const auto& d:emperor::file_io::research::figure7_fields){const auto v=f.fields[n++];
        if((d.bytes<4&&d.bytes!=20&&v>=(uint64_t(1)<<(8*d.bytes)))||(d.bytes==20&&v))return "market_base_wire_width";}
    return {};
}
// Compose a known base at the post-draw boundary: typed fields own their
// mapped bits; raw owns unknown fields, enum high bytes and opaque padding.
// Decoder validation compares this composition rather than accepting two writers.
inline OriginalMarketKnownBase originalMarketKnownBaseAt(const OriginalMarketKnownBase& saved,const OriginalFigure& f){
    auto out=saved;out.figure.slot=f.slot;
    const auto put=[&](unsigned offset,uint32_t value,unsigned bytes){size_t n=0;
        for(const auto& d:emperor::file_io::research::figure7_fields){if(d.receiver_offset==offset){
            const uint32_t mask=bytes==4?UINT32_MAX:(uint32_t(1)<<(bytes*8))-1;
            if(n<out.figure.fields.size())out.figure.fields[n]=(out.figure.fields[n]&~mask)|(value&mask);return;}++n;}};
    put(5,uint32_t(f.frame),1);
    put(8,uint32_t(f.image),4);
    put(404,uint32_t(f.secondaryImage),4);
    put(16,uint32_t(f.next),2);
    put(18,uint32_t(f.type),1);
    put(22,uint32_t(f.alive),1);
    put(64,uint32_t(f.action),1);
    put(65,uint32_t(f.progress),1);
    put(25,uint32_t(f.direction),1);
    put(26,uint32_t(f.previousDirection),1);
    put(27,uint32_t(f.attackDirection),1);
    put(28,uint32_t(f.x),2);
    put(30,uint32_t(f.y),2);
    put(40,uint32_t(f.cell),4);
    put(82,uint32_t(f.subX),2);
    put(84,uint32_t(f.subY),2);
    put(118,uint32_t(f.extraX),2);
    put(120,uint32_t(f.extraY),2);
    put(20,uint32_t(f.freeMovement),1);
    put(36,uint32_t(f.raise),1);
    put(111,uint32_t(f.hidden),1);
    put(133,uint32_t(f.state85),1);
    put(134,uint32_t(f.height),1);
    put(19,uint32_t(f.subtype),1);
    put(116,uint32_t(f.attackFrame),1);
    put(136,uint32_t(f.resource),1);
    put(135,uint32_t(f.cartStyle),1);
    put(137,uint32_t(f.stationary),1);
    put(62,uint32_t(f.counter),2);
    put(98,uint32_t(f.owner),2);
    put(304,uint32_t(f.variant),2);
    put(130,uint32_t(f.amount),2);
    put(114,uint32_t(f.leaderLink),2);
    put(144,uint32_t(f.target),2);
    put(122,uint32_t(f.faction),1);
    put(132,uint32_t(f.effectDraw),1);
    put(316,uint32_t(f.updateOwner),4);
    put(172,uint32_t(f.threat),4);
    put(176,uint32_t(f.threatDistance),4);
    put(188,uint32_t(f.herdTarget),4);
    put(192,uint32_t(f.herdDistance),4);
    put(204,uint32_t(f.idleCounter),4);
    put(328,uint32_t(f.linkedBuilding),4);
    put(128,uint32_t(f.movementMode),1);
    put(153,uint32_t(f.attackerCount),1);
    put(66,uint32_t(f.route),2);
    put(68,uint32_t(f.routeCursor),2);
    put(70,uint32_t(f.routeLength),2);
    put(44,uint32_t(f.destinationX),2);
    put(46,uint32_t(f.destinationY),2);
    put(48,uint32_t(f.destinationCell),4);
    put(368,uint32_t(f.routePhase),1);
    put(97,uint32_t(f.speed),1);
    put(276,uint32_t(f.age),4);
    put(400,uint32_t(f.priorProgress),4);
    put(146,uint32_t(f.reciprocalTarget),2);
    put(102,uint32_t(f.follower),2);
    put(32,uint32_t(f.priorX),2);
    put(34,uint32_t(f.priorY),2);
    put(300,uint32_t(f.priorCell),4);
    put(52,uint32_t(f.homeX),2);
    put(54,uint32_t(f.homeY),2);
    put(100,uint32_t(f.home),2);
    put(104,uint32_t(f.delivery),2);
    put(73,uint32_t(f.water),1);
    put(74,uint32_t(f.roamLimit),2);
    put(76,uint32_t(f.roamSteps),2);
    put(78,uint32_t(f.roamState),1);
    put(79,uint32_t(f.roamTurns),1);
    put(80,uint32_t(f.roamTurn),1);
    put(81,uint32_t(f.roamCountdown),1);
    put(117,uint32_t(f.searchCounter),1);
    put(348,uint32_t(f.roamOriginX),2);
    put(350,uint32_t(f.roamOriginY),2);
    put(86,uint32_t(f.freeTargetX),2);
    put(88,uint32_t(f.freeTargetY),2);
    put(90,uint32_t(f.freeRemainingX),2);
    put(92,uint32_t(f.freeRemainingY),2);
    put(94,uint32_t(f.freeError),2);
    put(96,uint32_t(f.freeAxis),1);
    put(72,uint32_t(f.cellMovement),1);
    put(168,uint32_t(f.damage),4);
    put(160,uint32_t(f.projectileHits),1);
    put(106,uint32_t(f.formation),2);
    put(108,uint32_t(f.formationOrdinal),1);
    put(109,uint32_t(f.formationStationary),1);
    put(110,uint32_t(f.employmentAmount),1);
    put(112,uint32_t(f.watchCoverage),1);
    put(288,uint32_t(f.flags120),4);
    put(24,uint32_t(f.previousAction),1);
    put(148,uint32_t(f.allocationSerial),2);
    put(138,uint32_t(f.allocationPhase),1);
    put(140,uint32_t(f.allocationPhaseCopy),1);
    for(unsigned n=0;n<3;++n)put(208+4*n,uint32_t(f.cooldowns[n]),4);
    for(unsigned n=0;n<2;++n)put(154+2*n,uint16_t(f.attackers[n]),2);
    put(37,f.drawClass?std::max(1u,saved.figure.field(37).value_or(0)):0u,1);
    put(297,f.projectile?std::max(1u,saved.figure.field(297).value_or(0)):0u,1);
    auto& o=out.figure.opaque380;o[0]=f.effectKind;o[1]=f.effectVariant;o[2]=f.effectFlags;o[3]=uint8_t(f.effectX);o[4]=uint8_t(f.effectY);
    o[6]=uint8_t(f.effectHeight);o[7]=uint8_t(uint16_t(f.effectHeight)>>8);o[8]=uint8_t(f.effectVelocityX);o[9]=uint8_t(f.effectVelocityY);
    o[10]=uint8_t(f.effectVelocityZ);o[11]=uint8_t(uint16_t(f.effectVelocityZ)>>8);return out;
}
inline bool originalMarketKnownBaseMatches(const OriginalMarketKnownBase& b,const OriginalFigure& f){
    const auto canonical=originalMarketKnownBaseAt(b,f);
    return f.runtimeClass==b.figure.runtimeClass&&b.figure.fields==canonical.figure.fields&&b.figure.opaque380==canonical.figure.opaque380&&
        f.effectFrame==f.progress&&uint16_t(f.projectileDistance)==uint16_t(f.threat)&&uint16_t(f.projectileStartHeight)==uint16_t(f.threat>>16)&&
        uint16_t(f.projectileTargetHeight)==uint16_t(f.threatDistance)&&uint16_t(f.projectileEndHeight)==uint16_t(f.threatDistance>>16);
}
inline std::optional<std::string> encodeOriginalMarketKnownBase(const OriginalMarketKnownBase& b,std::string& error){
    error=originalMarketKnownBaseIssue(b);if(!error.empty())return std::nullopt;
    std::ostringstream o;o<<"ORIGINAL-MARKET-BASE 1\n"<<"source "<<b.sourceRevision<<"\nslot "<<b.figure.slot<<"\nclass 0\nbase "<<b.figure.fields.size()<<'\n';
    size_t n=0;for(const auto& d:emperor::file_io::research::figure7_fields){o<<d.receiver_offset<<' '<<d.bytes<<' '<<b.figure.fields[n++];
        if(d.bytes==20)for(uint8_t v:b.figure.opaque380)o<<' '<<unsigned(v);o<<'\n';}
    o<<"extension 0\nend\n";return o.str();
}
inline std::optional<OriginalMarketKnownBase> decodeOriginalMarketKnownBase(const std::string& bytes,uint64_t expectedSource,std::string& error){
    using native_walker_detail::parseInteger;
    const auto fail=[&](const char* why)->std::optional<OriginalMarketKnownBase>{error=why;return std::nullopt;};
    if(bytes.empty()||bytes.size()>16384||bytes.back()!='\n'||bytes.find('\0')!=std::string::npos||bytes.find('\r')!=std::string::npos)return fail("market_base_framing");
    std::istringstream in(bytes);std::string line;
    if(!std::getline(in,line)||line!="ORIGINAL-MARKET-BASE 1")return fail("market_base_version");
    const auto read=[&](const char* name,auto& v){if(!std::getline(in,line))return false;const std::string prefix=std::string(name)+' ';return line.rfind(prefix,0)==0&&parseInteger(std::string_view(line).substr(prefix.size()),v);};
    OriginalMarketKnownBase b;unsigned cls=1;size_t count=0;
    if(!read("source",b.sourceRevision)||b.sourceRevision!=expectedSource||!read("slot",b.figure.slot)||!read("class",cls)||cls||!read("base",count)||count!=std::size(emperor::file_io::research::figure7_fields))return fail("market_base_header");
    for(const auto& d:emperor::file_io::research::figure7_fields){if(!std::getline(in,line))return fail("market_base_truncated");std::istringstream row(line);std::vector<std::string> t;std::string token;while(row>>token)t.push_back(token);
        if(t.size()!=(d.bytes==20?23u:3u))return fail("market_base_record");
        unsigned offset=0,width=0;uint32_t value=0;
        if(!parseInteger(t[0],offset)||!parseInteger(t[1],width)||!parseInteger(t[2],value)||offset!=d.receiver_offset||width!=d.bytes)return fail("market_base_layout");
        b.figure.fields.push_back(value);
        if(d.bytes==20)for(size_t n=0;n<20;++n){uint8_t v=0;if(!parseInteger(t[3+n],v))return fail("market_base_opaque_width");b.figure.opaque380[n]=v;}}
    if(!std::getline(in,line)||line!="extension 0")return fail("market_base_unknown_fsa");
    if(!std::getline(in,line)||line!="end"||std::getline(in,line))return fail("market_base_trailing_data");
    b.figure.recordComplete=true;error=originalMarketKnownBaseIssue(b);if(!error.empty())return std::nullopt;
    return b;
}
} // namespace game_lab
