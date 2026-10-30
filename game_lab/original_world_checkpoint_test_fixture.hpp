// SPDX-License-Identifier: MIT
#pragma once
#include "original_world_checkpoint_io.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
namespace game_lab { namespace world_test {
inline size_t checks=0;
inline void need(bool b,const char* message){++checks;if(!b)throw std::runtime_error(message);}
inline OriginalWorldSnapshot fixture(){
 OriginalWorldSnapshot s;s.commitRevision=7;s.source.source.relative="Save/authored.sav";s.source.source.bytes=100;s.source.source.digest[0]=1;s.source.legacyRevision=17;
 for(const auto& name:original_world_binding_detail::resources()){OriginalWorldFileBinding b;b.relative=name;b.bytes=1;b.digest[0]=3;s.source.resources.push_back(b);}
 s.city.figures.terrain.resize(51984);s.city.figures.cellHeads.resize(51984);
 for(size_t slot=0;slot<2000;++slot){OriginalFigure f;f.slot=slot;s.city.figures.figures.emplace(slot,f);if(slot)s.city.figures.freeSlots.push_back(slot);}
 s.city.scenarioMessages.emplace();s.terrain.map.activeSide=1;s.terrain.map.flags=s.city.figures.terrain;s.scene.shown.surface.terrain=s.terrain.map.flags;s.scene.shown.accepted=true;
 s.continuation.initialFigures=s.city.figures;s.ledger.next=1;return s;
}
inline void rehash(std::string& bytes){auto patch=[&](size_t at,uint64_t n,size_t size){for(size_t i=0;i<size;++i)bytes.at(at+i)=char(n>>(8*i));};
 auto u64=[&](size_t at){uint64_t n=0;for(size_t i=0;i<8;++i)n|=uint64_t(uint8_t(bytes.at(at+i)))<<(8*i);return n;};
 patch(24,bytes.size(),8);for(unsigned i=0;i<14;++i){const auto at=64+56*i;const auto start=u64(at+8),len=u64(at+16);if(start<=bytes.size()&&len<=bytes.size()-start){const auto hash=originalWorldDigest(bytes.data()+start,size_t(len));std::memcpy(bytes.data()+at+24,hash.data(),32);}}
 const auto hash=originalWorldDigest(bytes.data(),bytes.size()-40);std::memcpy(bytes.data()+bytes.size()-32,hash.data(),32);
}
struct Fill {
 size_t fields=0;unsigned mode=0;
 void record(uint16_t){}
 template<class T>void field(uint16_t,T& x){++fields;value(x);}
 template<class T>void value(T& x){using namespace original_world_codec_detail;
  if constexpr(std::is_same_v<T,bool>)x=true;
  else if constexpr(std::is_integral_v<T>)x=mode==2?std::numeric_limits<T>::max():(mode==3?std::numeric_limits<T>::min():(std::is_signed_v<T>?T(-3):T(3)));
  else if constexpr(std::is_enum_v<T>)x=static_cast<T>(1);
  else if constexpr(std::is_same_v<T,double>)x=1.25;
  else if constexpr(std::is_same_v<T,std::string>)x="authored-field";
  else if constexpr(std::is_same_v<T,const char*>)x="plain-land";
  else if constexpr(Optional<T>::value){if(mode==1)x.reset();else {auto v=fresh<typename Optional<T>::element>();if(mode!=4)value(v);x=std::move(v);}}
  else if constexpr(Vector<T>::value){auto v=fresh<typename Vector<T>::element>();value(v);x.clear();x.push_back(std::move(v));}
  else if constexpr(Map<T>::value){auto k=fresh<typename Map<T>::key>();auto v=fresh<typename Map<T>::element>();value(k);value(v);x.clear();x.emplace(std::move(k),std::move(v));}
  else if constexpr(Set<T>::value){auto v=fresh<typename Set<T>::element>();value(v);x.clear();x.insert(std::move(v));}
  else if constexpr(Array<T>::value){for(auto& v:x)value(v);}
  else if constexpr(Pair<T>::value){value(x.first);value(x.second);}
  else worldFields(*this,x);
 }
};
template<class T>inline size_t typedRoundtrip(){using namespace original_world_codec_detail;size_t fields=0;
 for(unsigned mode=0;mode<5;++mode){auto original=fresh<T>();Fill fill;fill.mode=mode;fill.value(original);Writer writer;writer.value(original);auto decoded=fresh<T>();Reader reader(writer.bytes);reader.value(decoded);need(reader.pos==reader.limit,"typed record consumes exact bytes");Writer again;again.value(decoded);need(writer.bytes==again.bytes,"all typed fields preserve values/widths/knownness");fields+=fill.fields;}return fields;
}
}}
