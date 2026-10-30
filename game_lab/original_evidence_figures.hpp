// SPDX-License-Identifier: MIT
#pragma once
#include "original_figure.hpp"
#include <map>

namespace game_lab {
// Observation only. Never feed a partial prefix into a simulation or a SAV
// acceptance result. Unknown classes/FSA stop BEFORE any unqualified payload.
struct EvidenceFigure {
    size_t slot=0,begin=0,end=0;
    bool recordComplete=false,alias=false;
    OriginalFigureClass runtimeClass=OriginalFigureClass::Figure;
    std::vector<uint32_t> fields;
    std::array<uint8_t,20> opaque380{};
    std::optional<uint32_t> field(unsigned offset) const {
        size_t n=0;for(const auto& f:emperor::file_io::research::figure7_fields){
            if(f.receiver_offset==offset)return f.bytes<=4&&n<fields.size()?std::optional<uint32_t>(fields[n]):std::nullopt;++n;
        }return std::nullopt;
    }
};
struct EvidenceFigurePrefix {
    bool complete=false;
    size_t declared=0,completed=0,stopOffset=0,stopSlot=0;
    std::string reason="not_started",unqualifiedClass;
    std::vector<EvidenceFigure> records;
};
inline EvidenceFigurePrefix evidenceFigurePrefix(const std::vector<uint8_t>& data,
    size_t start,size_t limit,size_t nextPID){
    EvidenceFigurePrefix out;size_t at=start;
    const auto stop=[&](const char* why){out.reason=why;out.stopOffset=at;out.stopSlot=out.completed;return out;};
    if(data.size()>8u*1024u*1024u||nextPID>32768)return stop("budget");
    if(!nextPID||start>limit||limit>data.size())return stop("bounds");
    const auto take=[&](size_t n){if(n>limit-at)return false;at+=n;return true;};
    if(!take(6))return stop("list_header_truncated");
    if(detail::u16(data.data()+start)!=1)return stop("list_version");
    out.declared=detail::u32(data.data()+start+2);
    if(out.declared>8192)return stop("count_budget");
    enum class Kind {Figure,Pheasant,Emissary};
    std::map<size_t,Kind> classes;std::map<size_t,EvidenceFigure> objects;
    for(size_t slot=0;slot<out.declared;++slot){
        EvidenceFigure f;f.slot=slot;f.begin=at;
        if(!take(2))return stop("tag_truncated");
        uint32_t tag=detail::u16(data.data()+at-2);const bool declaration=tag==0xffff;
        if(tag==0x7fff){if(!take(4))return stop("wide_tag_truncated");tag=detail::u32(data.data()+at-4);}
        else if(!declaration)tag=(tag&0x7fff)|((tag&0x8000)<<16);
        Kind kind=Kind::Figure;
        if(declaration){
            if(!take(4))return stop("class_header_truncated");
            const auto schema=detail::u16(data.data()+at-4),n=detail::u16(data.data()+at-2);
            if(schema||!n||n>=64||!take(n))return stop("class_schema_or_name");
            const std::string name(reinterpret_cast<const char*>(data.data()+at-n),n);
            if(name=="Figure")kind=Kind::Figure;
            else if(name=="cPheasantFig")kind=Kind::Pheasant;
            else if(name=="cEmissaryDude")kind=Kind::Emissary;
            else {out.unqualifiedClass=name;return stop("unqualified_class");}
            if(nextPID>=32768)return stop("cache_budget");classes.emplace(nextPID++,kind);
        }else if(tag&0x80000000u){
            const auto i=classes.find(tag&0x7fffffffu);
            if(i==classes.end())return stop("class_reference");kind=i->second;
        }else{
            const auto i=objects.find(tag);if(i==objects.end())return stop("object_reference");
            const size_t begin=f.begin;f=i->second;f.slot=slot;f.begin=begin;f.end=at;f.alias=true;
            out.records.push_back(f);++out.completed;continue;
        }
        if(nextPID>=32768)return stop("cache_budget");const auto objectPID=nextPID++;
        f.runtimeClass=kind==Kind::Pheasant?OriginalFigureClass::Pheasant:kind==Kind::Emissary?OriginalFigureClass::Emissary:OriginalFigureClass::Figure;
        if(kind!=Kind::Figure){if(!take(2))return stop("own_version_truncated");
            if(detail::u16(data.data()+at-2)!=2)return stop("own_version");}
        if(!take(2))return stop("base_version_truncated");
        if(detail::u16(data.data()+at-2)!=7)return stop("base_version");
        for(const auto& field:emperor::file_io::research::figure7_fields){
            if(field.bytes!=1&&field.bytes!=2&&field.bytes!=4&&!(field.receiver_offset==380&&field.bytes==20))return stop("unqualified_field_width");
            if(!take(field.bytes))return stop("base_truncated");
            if(field.bytes==20){std::copy_n(data.data()+at-20,20,f.opaque380.begin());f.fields.push_back(0);continue;}
            uint32_t value=0;for(size_t n=0;n<field.bytes;++n)value|=uint32_t(data[at-field.bytes+n])<<(8*n);
            f.fields.push_back(value); // Preserve BOTH enum wire bytes.
        }
        if(!take(1))return stop("extension_flag_truncated");
        if(data[at-1]){f.end=at;out.records.push_back(f);return stop("variable_fsa_unqualified");}
        if(kind==Kind::Emissary){
            if(!take(4))return stop("emissary_length_truncated");const auto n=detail::u32(data.data()+at-4);
            if(n>8192)return stop("emissary_budget");if(!take(n))return stop("emissary_truncated");
        }
        f.end=at;f.recordComplete=true;objects.emplace(objectPID,f);out.records.push_back(f);++out.completed;
    }
    out.complete=true;out.reason="known_base_list_complete";out.stopOffset=at;out.stopSlot=out.completed;return out;
}
} // namespace game_lab
