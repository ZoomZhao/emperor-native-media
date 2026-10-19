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
 size_t header=name.empty()?2:6+name.size();const size_t common=version==3?179:version==4?181:182;const size_t tail=type==72?130:0;
 Bytes b(header+common+tail);word(b,0,name.empty()?0x8001:0xffff);if(!name.empty()){word(b,4,uint16_t(name.size()));std::copy(name.begin(),name.end(),b.begin()+6);}word(b,header,version);b[header+2]=alive;b[header+5]=size;word(b,header+8,x);word(b,header+10,y);word(b,header+16,type);return b;
}
void append(Bytes& b,const Bytes& r){b.insert(b.end(),r.begin(),r.end());}
Bytes table(unsigned version=5,bool complete=false){Bytes b(31);b[0]=0x13;append(b,{1,0,0xa0,0x0f,0,0});append(b,record(0,"Building",version));append(b,record(72,"cWellBldg",version,7,9,2,1));if(complete)for(size_t i=2;i<4000;++i)append(b,record(0,"",version));else append(b,record(0x7c,"cInspectorBldg",version));return b;}
OriginalMapTerrain terrain(){OriginalMapTerrain t;t.activeSide=112;t.flags.assign(51984,0x80000u);for(size_t y=58;y<170;++y)for(size_t x=58;x<170;++x)t.flags[y*228+x]=0;for(size_t y=67;y<69;++y)for(size_t x=65;x<67;++x)t.flags[y*228+x]=8;return t;}
int main(){try{
 auto grid=terrain();OriginalBuildingProfile profile;
 for(unsigned v:{3u,4u,5u}){auto data=table(v);profile=parseOriginalSavedBuildings(data,data.size(),grid);need(profile.status==OriginalBuildingStatus::UnsupportedTail&&profile.parsedRecords==2&&profile.namedWellRecords==1&&profile.footprints.size()==1,"independent public-span fixtures close Well before unknown tail");const auto& f=profile.footprints[0];need(f.type==72&&f.x==7&&f.y==9&&f.size==2&&f.slot==1,"physical source fields preserved;no sprite ID inference");}
 auto complete=table(5,true);auto all=parseOriginalSavedBuildings(complete,complete.size(),grid);need(all.status==OriginalBuildingStatus::Complete&&all.parsedRecords==4000&&all.footprints.size()==1,"4000 closed authored slots distinguished from prefix");
 auto missing=Bytes{0x13,0};need(parseOriginalSavedBuildings(missing,missing.size(),grid).status==OriginalBuildingStatus::MissingAnchor,"no fabricated table");auto duplicate=table();append(duplicate,table());auto bad=parseOriginalSavedBuildings(duplicate,duplicate.size(),grid);need(bad.status==OriginalBuildingStatus::AmbiguousAnchor&&bad.footprints.empty(),"ambiguous table refuses all footprints");
 auto many=table();for(size_t i=1;i<17;++i)append(many,table());need(parseOriginalSavedBuildings(many,many.size(),grid).status==OriginalBuildingStatus::BudgetExceeded,"anchor cardinality bound");
 auto truncated=table();truncated.pop_back();bad=parseOriginalSavedBuildings(truncated,truncated.size(),grid);need(bad.status==OriginalBuildingStatus::Malformed&&bad.footprints.empty()&&bad.parsedRecords==0,"late truncated common body clears earlier Well");
 auto wrongRole=table();wrongRole[0]=5;need(parseOriginalSavedBuildings(wrongRole,wrongRole.size(),grid).footprints.empty(),"MAP role not borrowed as SAV instances");need(parseOriginalSavedBuildings(table(),99999999,grid).footprints.empty(),"terrain extent bound");
 auto rejected=grid;rejected.flags[67*228+65]=0;auto data=table();bad=parseOriginalSavedBuildings(data,data.size(),rejected);need(bad.namedWellRecords==1&&bad.rejectedWellRecords==1&&bad.footprints.empty(),"named Well still requires complete saved terrain footprint");
 OriginalMapCamera camera;camera.width=320;camera.height=240;camera.centerX=7.5;camera.centerY=9.5;
 auto view=renderOriginalBuildingFootprints(grid,profile.footprints,camera);auto base=renderOriginalMapSpatialFlags(grid,camera);need(view.image.valid()&&view.valid==1&&view.visible==1&&view.outlinePixels==320,"one authored2x2 footprint has320 unique annotation pixels");
 const uint32_t amber=0xffffb000u;for(auto xy:{std::pair<size_t,size_t>{160,80},{240,120},{160,160},{80,120}})need(view.image.pixelsBGRA[xy.second*320+xy.first]==amber,"independent40/20 lattice corner oracle");need(view.image.pixelsBGRA[120*320+160]!=amber,"no guessed fill/artwork in footprint interior");bool exact=true;size_t marked=0;for(size_t i=0;i<base.pixelsBGRA.size();++i){if(view.image.pixelsBGRA[i]==amber)++marked;else exact&=view.image.pixelsBGRA[i]==base.pixelsBGRA[i];}need(exact&&marked==320,"all non-outline pixels remain the same SAV numerical terrain");
 camera.scale=2;view=renderOriginalBuildingFootprints(grid,profile.footprints,camera);need(view.image.valid()&&view.outlinePixels==639,"scaled lattice clips exactly the rightmost pixel without relocating footprint");camera.scale=1;camera.centerX=0;camera.centerY=0;view=renderOriginalBuildingFootprints(grid,profile.footprints,camera);need(view.image.valid()&&view.valid==1&&view.visible==0&&view.outlinePixels==0,"offscreen footprint counted but not relocated");
 camera.centerX=7.5;camera.centerY=9.5;auto two=profile.footprints;two.push_back(two[0]);view=renderOriginalBuildingFootprints(grid,two,camera);need(view.visible==2&&view.outlinePixels==320,"overlapping records do not inflate unique pixels");
 auto f=profile.footprints[0];for(int kind=0;kind<4;++kind){auto invalid=f;if(kind==0)invalid.x=112;if(kind==1)invalid.size=0;if(kind==2)invalid.type=73;if(kind==3)invalid.size=9;view=renderOriginalBuildingFootprints(grid,{invalid},camera);need(view.image.valid()&&view.valid==0&&view.rejected==1&&view.outlinePixels==0,"invalid instance never receives replacement coordinates");}
 camera.centerX=std::numeric_limits<double>::quiet_NaN();need(!renderOriginalBuildingFootprints(grid,profile.footprints,camera).image.valid(),"nonfinite camera no stale view");camera.centerX=7.5;camera.scale=0;need(!renderOriginalBuildingFootprints(grid,profile.footprints,camera).image.valid(),"invalid scale no stale view");camera.scale=1;need(!renderOriginalBuildingFootprints(grid,std::vector<OriginalBuildingFootprint>(4001,f),camera).image.valid(),"record list budget");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"original_data\":false}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
