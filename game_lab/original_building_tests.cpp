#include "original_building_view.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <limits>
using namespace game_lab;using Bytes=std::vector<uint8_t>;
size_t checks=0;
void need(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void word(Bytes& b,size_t at,uint16_t value){b.at(at)=value&255;b.at(at+1)=value>>8;}
Bytes record(uint16_t type,const std::string& name,uint16_t version,uint16_t x=0,uint16_t y=0,uint8_t size=0,uint8_t alive=0){
 size_t header=name.empty()?2:6+name.size();const size_t common=version==3?179:version==4?181:182;const size_t tail=(type==72||type==124||type==127)?130:0;
 Bytes b(header+common+tail);word(b,0,name.empty()?0x8001:0xffff);if(!name.empty()){word(b,4,uint16_t(name.size()));std::copy(name.begin(),name.end(),b.begin()+6);}word(b,header,version);b[header+2]=alive;b[header+5]=size;word(b,header+8,x);word(b,header+10,y);word(b,header+16,type);return b;
}
void append(Bytes& b,const Bytes& r){b.insert(b.end(),r.begin(),r.end());}
Bytes table(unsigned version=5,bool complete=false){Bytes b(31);b[0]=0x13;append(b,{1,0,0xa0,0x0f,0,0});append(b,record(0,"Building",version));append(b,record(72,"cWellBldg",version,7,9,2,1));if(complete)for(size_t i=2;i<4000;++i)append(b,record(0,"",version));else append(b,record(0x1234,"UnsupportedAuthoredClass",version));return b;}
OriginalMapTerrain terrain(){OriginalMapTerrain t;t.activeSide=112;t.flags.assign(51984,0x80000u);for(size_t y=58;y<170;++y)for(size_t x=58;x<170;++x)t.flags[y*228+x]=0;for(size_t y=67;y<69;++y)for(size_t x=65;x<67;++x)t.flags[y*228+x]=8;return t;}
int main(){try{
 auto grid=terrain();OriginalBuildingProfile profile;
 for(unsigned v:{3u,4u,5u}){auto data=table(v);profile=parseOriginalSavedBuildings(data,data.size(),grid);need(profile.status==OriginalBuildingStatus::UnsupportedTail&&profile.parsedRecords==2&&profile.namedWellRecords==1&&profile.footprints.size()==1,"independent public-span fixtures close Well before unknown tail");const auto& f=profile.footprints[0];need(f.type==72&&f.x==7&&f.y==9&&f.size==2&&f.slot==1,"physical source fields preserved;no sprite ID inference");}
 auto complete=table(5,true);auto all=parseOriginalSavedBuildings(complete,complete.size(),grid);need(all.status==OriginalBuildingStatus::Complete&&all.parsedRecords==4000&&all.footprints.size()==1,"4000 closed authored slots distinguished from prefix");
 auto missing=Bytes{0x13,0};need(parseOriginalSavedBuildings(missing,missing.size(),grid).status==OriginalBuildingStatus::MissingAnchor,"no fabricated table");auto duplicate=table();append(duplicate,table());auto bad=parseOriginalSavedBuildings(duplicate,duplicate.size(),grid);need(bad.status==OriginalBuildingStatus::AmbiguousAnchor&&bad.footprints.empty(),"ambiguous table refuses all footprints");
 auto many=table();for(size_t i=1;i<17;++i)append(many,table());need(parseOriginalSavedBuildings(many,many.size(),grid).status==OriginalBuildingStatus::BudgetExceeded,"anchor cardinality bound");
 auto truncated=table();truncated.pop_back();bad=parseOriginalSavedBuildings(truncated,truncated.size(),grid);need(bad.status==OriginalBuildingStatus::Malformed&&bad.footprints.empty()&&bad.parsedRecords==0,"late truncated common body clears earlier Well");
 auto wrongRole=table();wrongRole[0]=5;need(parseOriginalSavedBuildings(wrongRole,wrongRole.size(),grid).footprints.empty(),"MAP role not borrowed as SAV instances");need(parseOriginalSavedBuildings(table(),99999999,grid).footprints.empty(),"terrain extent bound");
 auto rejected=grid;rejected.flags[67*228+65]=0;auto data=table();bad=parseOriginalSavedBuildings(data,data.size(),rejected);need(bad.namedWellRecords==1&&bad.rejectedTerrainFootprints==1&&bad.footprints.empty(),"named Well still requires complete saved terrain footprint");
 for(unsigned v:{3u,4u,5u}){
  auto safety=table(v);auto prefix=parseOriginalSavedBuildings(safety,safety.size(),grid);safety.resize(prefix.footprints[0].recordOffset); // Keep only the closed root record.
  append(safety,record(124,"cInspectorBldg",v,7,9,2,1));append(safety,record(127,"cWatchtower",v,7,9,2,1));append(safety,record(0x1234,"UnsupportedAuthoredClass",v));
  auto found=parseOriginalSavedBuildings(safety,safety.size(),grid);need(found.status==OriginalBuildingStatus::UnsupportedTail&&found.parsedRecords==3&&found.namedInspectorRecords==1&&found.namedWatchtowerRecords==1&&found.footprints.size()==2,"public130-byte safety tails close sequentially before unknown type");
  need(found.footprints[0].type==124&&found.footprints[1].type==127,"new semantic types retain source order without image IDs");
  for(size_t cut=0;cut<safety.size();++cut){auto shortData=Bytes(safety.begin(),safety.begin()+cut);auto q=parseOriginalSavedBuildings(shortData,shortData.size(),grid);if(q.status!=OriginalBuildingStatus::UnsupportedTail)need(q.footprints.empty(),"every late truncation clears all safety footprints");}
 }
 need(!namedOriginalBuilding(124,"cWellBldg")&&!namedOriginalBuilding(127,"cInspectorBldg")&&!namedOriginalBuilding(0x1234,"cWatchtower"),"class/type pair must be explicit and supported");
 // Independent literal span oracle, grouped from the cited public schema.
 const std::vector<std::pair<uint16_t,size_t>> publishedSpans={{0x0,0},{0x2,65},{0x3,65},{0x4,65},{0x5,65},{0x6,65},{0x7,65},{0x8,65},{0x9,65},{0xa,65},{0xb,65},{0xc,65},{0xd,65},{0xe,65},{0xf,65},{0x10,65},{0x11,65},{0x5e,2},{0x48,130},{0x6f,130},{0x71,130},{0x7c,130},{0x7e,130},{0x7f,130},{0x8a,130},{0x97,130},{0xb1,130},{0xb2,130},{0xb3,130},{0xb4,130},{0xb5,130},{0xb7,130},{0xb9,130},{0xba,130},{0xbb,130},{0xbc,130},{0xbd,130},{0xa1,130},{0xa2,130},{0xa3,130},{0xa4,130},{0xa5,130},{0xa7,130},{0xad,130},{0xae,130},{0xaf,130},{0xcf,130},{0xd0,130},{0x59,132},{0x5a,132},{0x5b,132},{0xe7,132},{0x68,132},{0x69,132},{0x6a,132},{0xe8,132},{0x74,132},{0x75,132},{0x76,132},{0x77,132},{0x78,132},{0x79,132},{0x7a,132},{0x7d,132},{0x83,132},{0xcb,132},{0xd1,132},{0xf3,132},{0xf4,132},{0xf5,132},{0xf6,132},{0xf7,132},{0xf8,132},{0xf9,132},{0xfa,132},{0xfb,132},{0xfc,132},{0x1f,134},{0x21,134},{0x26,134},{0x2a,134},{0x2b,134},{0x2c,134},{0x2d,134},{0x2e,134},{0x2f,134},{0x31,134},{0x34,134},{0xd3,134},{0xd4,134},{0xd5,134},{0xe2,134},{0xe9,134},{0xeb,134},{0xec,134},{0x23,136},{0x24,136},{0x25,136},{0x27,136},{0x28,136},{0x29,136},{0x30,136},{0xdc,136},{0xdd,136},{0xdf,136},{0xe0,136},{0xe1,136},{0x1a,140},{0x1b,140},{0x1c,140},{0x3e,140},{0x40,140},{0x41,140},{0x42,140},{0x43,140},{0x44,140},{0x45,140},{0x46,140},{0x82,140},{0xc2,140},{0xc3,140},{0xc4,140},{0xc5,140},{0xc6,140},{0xc7,140},{0xc0,142},{0xed,142},{0xee,142},{0xef,142},{0xd6,165},{0xd7,165},{0xd9,165},{0xd8,167},{0xda,167},{0xdb,169},{0x6e,179},{0xc1,222},{0x4b,236},{0x36,248},{0x3b,266},{0x3c,266},{0x35,288},{0x3a,348},{0x47,352},{0x38,356},{0xd2,2140}};
 auto authoredSpan=[&](uint16_t kind,unsigned v,size_t tail){auto b=record(kind,"AuthoredSpan",v);b.resize(6+std::string("AuthoredSpan").size()+(v==3?179:v==4?181:182)+tail);return b;};
 for(const auto& entry:publishedSpans)for(unsigned v:{3u,4u,5u}){
  auto bytes=table(v);auto root=parseOriginalSavedBuildings(bytes,bytes.size(),grid);bytes.resize(root.footprints[0].recordOffset);
  auto b=authoredSpan(entry.first,v,entry.second);append(bytes,b);const size_t next=bytes.size();append(bytes,record(72,"cWellBldg",v,7,9,2,1));append(bytes,record(0x1234,"UnknownTail",v));
  auto result=parseOriginalSavedBuildings(bytes,bytes.size(),grid);need(result.status==OriginalBuildingStatus::UnsupportedTail&&result.parsedRecords==3&&result.footprints.size()==1&&result.footprints[0].recordOffset==next,"every supported type closes its literal span before the following Well");
  bytes.resize(next-1);result=parseOriginalSavedBuildings(bytes,bytes.size(),grid);need(result.status==OriginalBuildingStatus::Malformed&&result.footprints.empty(),"every supported type refuses a one-byte-short tail/common span");
 }
 const std::vector<uint16_t> monumentTypes={0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x5c,0x5d,0xfd,0xfe,0xff,0x100,0x101,0x102,0x103,0x104};
 for(auto kind:monumentTypes)for(unsigned v:{3u,4u,5u})for(unsigned sub:{9u,10u,11u}){
  auto bytes=table(v);auto root=parseOriginalSavedBuildings(bytes,bytes.size(),grid);bytes.resize(root.footprints[0].recordOffset);auto m=authoredSpan(kind,v,sub==9?140:141);const size_t common=v==3?179:v==4?181:182;word(m,6+std::string("AuthoredSpan").size()+common,1);word(m,6+std::string("AuthoredSpan").size()+common+2,sub);append(bytes,m);append(bytes,record(72,"cWellBldg",v,7,9,2,1));append(bytes,record(0x1234,"UnknownTail",v));auto q=parseOriginalSavedBuildings(bytes,bytes.size(),grid);
  need(q.status==OriginalBuildingStatus::UnsupportedTail&&q.parsedRecords==(sub==11?1:3)&&q.footprints.size()==(sub==11?0:1),"monument9/10 spans close;unknown subversion stops without guessing");
 }
 need(!originalBuildingSquareType(0x82)&&!originalBuildingSquareType(0x3b)&&!originalBuildingSquareType(0xd2)&&!originalBuildingSquareType(0xdc),"nonsquare and dock layouts never become size-square annotations");
 need(originalBuildingSquareType(0xcf)&&originalBuildingSquareType(0x21)&&originalBuildingSquareType(5),"published ordinary footprint families are eligible by semantic type");
 // Opaque class references are NOT resolved:the separately stored semantic type
 // supplies the published footprint profile,not a guessed serialization class.
 auto referenced=table();auto rr=parseOriginalSavedBuildings(referenced,referenced.size(),grid);referenced.resize(rr.footprints[0].recordOffset);auto house=record(5,"",5,7,9,2,1);house.resize(house.size()+65);append(referenced,house);append(referenced,record(0x1234,"UnknownTail",5));auto ref=parseOriginalSavedBuildings(referenced,referenced.size(),grid);need(ref.footprints.size()==1&&ref.footprints[0].type==5,"semantic type remains independently usable with an opaque class-reference tag");
 auto marketGrid=grid;for(size_t y=67;y<74;++y)for(size_t x=65;x<72;++x)marketGrid.flags[y*228+x]=8;
 for(uint16_t type:{uint16_t(0x3b),uint16_t(0x3c)})for(unsigned rotation:{0u,1u}){
  auto bytes=table();auto closed=parseOriginalSavedBuildings(bytes,bytes.size(),grid);bytes.resize(closed.footprints[0].recordOffset);auto m=record(type,"cMarket",5,7,9,1,1);const size_t header=6+std::string("cMarket").size();m.resize(header+182+266);m[header+113]=1-rotation;m[header+182+96]=rotation;append(bytes,m);append(bytes,record(0x1234,"UnknownTail",5));auto parsed=parseOriginalSavedBuildings(bytes,bytes.size(),marketGrid);
  need(parsed.footprints.size()==1,"market closes exact266-byte tail andvalidates every rectangle cell");const auto& f=parsed.footprints[0];const uint8_t side=type==0x3b?4:6;need(originalFootprintWidth(f)==(rotation?7:side)&&originalFootprintHeight(f)==(rotation?side:7)&&f.size==1,"tail+96 controls rectangle;common rotation andraw size do not");
  OriginalMapCamera c;c.width=800;c.height=600;c.centerX=f.x+(originalFootprintWidth(f)-1)/2.0;c.centerY=f.y+(originalFootprintHeight(f)-1)/2.0;auto view=renderOriginalBuildingFootprints(marketGrid,{f},c);need(view.valid==1&&view.visible==1&&view.outlinePixels==80u*(7+side),"independent rectangular lattice perimeter oracle");auto missing=marketGrid;missing.flags[(67+originalFootprintHeight(f)-1)*228+65+originalFootprintWidth(f)-1]=0;need(!validOriginalBuildingFootprint(missing,f),"market refuses a missing far-corner terrain cell");auto focus=originalBuildingFocusCamera({f},c);need(focus.width>=40u*(7+side)+40&&focus.height>=20u*(7+side)+40&&renderOriginalBuildingFootprints(marketGrid,{f},focus).outlinePixels==80u*(7+side),"focused viewport includes the entire rectangular outline with margin");
 }
 // Independent MAP-local TN002 class/object PID fixture. Class IDs are1,3,6;
 // object IDs2,4,5,7,8 must never be treated as class IDs or building slots.
 for(unsigned v:{3u,4u,5u}){
  Bytes map(1093607);map[0]=5;map[2]=0xfe;map[3]=0xca;map[6]=2;append(map,{1,0,0xa0,0x0f,0,0});append(map,record(0,"Building",v));
  const auto addCleared=[&](const std::string& name,uint16_t ref){auto b=record(0,name,v);if(name.empty())word(b,0,ref);b.resize(b.size()+132);append(map,b);};
  const size_t first=map.size();addCleared("cResWall",0);const size_t wallEnd=map.size();addCleared("",0x8003);addCleared("cResGate",0);addCleared("",0x8006);for(size_t i=5;i<4000;++i)append(map,record(0,"",v));
  auto p=parseOriginalMapBuildings(map,grid);need(p.status==OriginalBuildingStatus::Complete&&p.parsedRecords==4000&&p.clearedClassRecords==4&&p.classReferencesVerified==3997&&p.footprints.empty(),"cleared wall/gate classes preserve132-byte tails through explicit andbackward class references");
  auto truncated=map;truncated.resize(wallEnd-1);p=parseOriginalMapBuildings(truncated,grid);need(p.status==OriginalBuildingStatus::Malformed&&p.parsedRecords==0,"one-byte-short cleared class tail refuses the whole result");
  auto forward=map;word(forward,wallEnd,0x8002);p=parseOriginalMapBuildings(forward,grid);need(p.status==OriginalBuildingStatus::Malformed&&p.footprints.empty(),"object PID cannot masquerade as a class PID");
  auto live=map;const size_t body=first+6+std::string("cResWall").size();live[body+2]=1;p=parseOriginalMapBuildings(live,grid);need(p.status==OriginalBuildingStatus::UnsupportedTail&&p.parsedRecords==1,"cleared-class exception is restricted toinactive size-zero records");
 }
 auto role=table();need(parseOriginalBuildingTable(role,31,role.size(),grid,true).status==OriginalBuildingStatus::Malformed,"SAV table never inherits a guessed local MAP class namespace");
 // Focus is a preset viewport;previous viewport size does not replace the
 // scaled lattice extent. Centre andrequested scale remain unchanged.
 for(unsigned width:{4u,6u})for(bool transpose:{false,true})for(unsigned scale:{1u,2u,3u,4u})for(auto viewport:{std::pair<unsigned,unsigned>{0,0},{64,40},{800,600},{2048,2048}}){
  OriginalBuildingFootprint f;f.type=width==4?0x3b:0x3c;f.x=7;f.y=9;f.size=1;f.width=transpose?width:7;f.height=transpose?7:width;
  OriginalMapCamera c;c.width=viewport.first;c.height=viewport.second;c.scale=scale;c.centerX=f.x+(f.width-1)/2.0;c.centerY=f.y+(f.height-1)/2.0;auto focus=originalBuildingFocusCamera({f},c);
  need(focus.scale==scale&&focus.centerX==c.centerX&&focus.centerY==c.centerY,"focus preserves requested centre andscale across prior viewports");
  if(width==6&&scale==4){need(focus.width==0&&focus.height==0&&!renderOriginalBuildingFootprints(marketGrid,{f},focus).image.valid(),"unrepresentable2080-pixel footprint refuses budget overflow without stale image");}
  else{auto view=renderOriginalBuildingFootprints(marketGrid,{f},focus);need(view.valid==1&&view.visible==1&&view.outlinePixels==80u*(7+width)*scale,"all representable scales show the entire market perimeter without clipping");}
 }
 for(unsigned scale:{0u,5u,std::numeric_limits<unsigned>::max()}){OriginalMapCamera c;c.scale=scale;const auto focus=originalBuildingFocusCamera({},c);need(focus.width==0&&focus.height==0,"invalid scales refuse before multiplying dimensions");}
 OriginalMapCamera camera;camera.width=320;camera.height=240;camera.centerX=7.5;camera.centerY=9.5;
 auto view=renderOriginalBuildingFootprints(grid,profile.footprints,camera);auto base=renderOriginalMapSpatialFlags(grid,camera);need(view.image.valid()&&view.valid==1&&view.visible==1&&view.outlinePixels==320,"one authored2x2 footprint has320 unique annotation pixels");
 const uint32_t amber=0xffffb000u;for(auto xy:{std::pair<size_t,size_t>{160,80},{240,120},{160,160},{80,120}})need(view.image.pixelsBGRA[xy.second*320+xy.first]==amber,"independent40/20 lattice corner oracle");need(view.image.pixelsBGRA[120*320+160]!=amber,"no guessed fill/artwork in footprint interior");bool exact=true;size_t marked=0;for(size_t i=0;i<base.pixelsBGRA.size();++i){if(view.image.pixelsBGRA[i]==amber)++marked;else exact&=view.image.pixelsBGRA[i]==base.pixelsBGRA[i];}need(exact&&marked==320,"all non-outline pixels remain the same SAV numerical terrain");
 camera.scale=2;view=renderOriginalBuildingFootprints(grid,profile.footprints,camera);need(view.image.valid()&&view.outlinePixels==639,"scaled lattice clips exactly the rightmost pixel without relocating footprint");camera.scale=1;camera.centerX=0;camera.centerY=0;view=renderOriginalBuildingFootprints(grid,profile.footprints,camera);need(view.image.valid()&&view.valid==1&&view.visible==0&&view.outlinePixels==0,"offscreen footprint counted but not relocated");
 camera.centerX=7.5;camera.centerY=9.5;auto two=profile.footprints;two.push_back(two[0]);view=renderOriginalBuildingFootprints(grid,two,camera);need(view.visible==2&&view.outlinePixels==320,"overlapping records do not inflate unique pixels");
 auto f=profile.footprints[0];for(int kind=0;kind<4;++kind){auto invalid=f;if(kind==0)invalid.x=112;if(kind==1)invalid.size=0;if(kind==2)invalid.type=73;if(kind==3)invalid.size=9;view=renderOriginalBuildingFootprints(grid,{invalid},camera);need(view.image.valid()&&view.valid==0&&view.rejected==1&&view.outlinePixels==0,"invalid instance never receives replacement coordinates");}
 camera.centerX=std::numeric_limits<double>::quiet_NaN();need(!renderOriginalBuildingFootprints(grid,profile.footprints,camera).image.valid(),"nonfinite camera no stale view");camera.centerX=7.5;camera.scale=0;need(!renderOriginalBuildingFootprints(grid,profile.footprints,camera).image.valid(),"invalid scale no stale view");camera.scale=1;need(!renderOriginalBuildingFootprints(grid,std::vector<OriginalBuildingFootprint>(4001,f),camera).image.valid(),"record list budget");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"original_data\":false}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
