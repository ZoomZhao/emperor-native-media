#include "game_lab/sg3_sequence.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace game_lab;
static size_t checks=0, canvases=0;
static void need(bool x,const char* why){++checks;if(!x)throw std::runtime_error(why);}
int main(int argc,char**argv){try{
 if(argc!=2)return 2;
 const std::filesystem::path path=std::filesystem::path(argv[1])/"authored.555";
 const uint16_t palette[]={0x7c00,0x03e0,0x001f,0x7fff,0};
 const uint32_t expected[]={0xffff0000,0xff00ff00,0xff0000ff,0xffffffff,0xff000000};
 std::vector<uint8_t> bytes;for(size_t i=0;i<4096;++i){auto v=palette[i%5];bytes.push_back(v&255);bytes.push_back(v>>8);}
 {std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(bytes.data()),bytes.size());need(bool(out),"fixture write");}
 for(size_t c=0;c<96;++c){
  SG3Archive a;a.version=213;
  SG3Sprite s;s.type=1;s.width=1+c%7;s.height=1+(c/7)%5;s.length=2*s.width*s.height;s.animationOffsetX=int(c%17)-8;s.animationOffsetY=int(c%13)-6;
  auto t=s;t.width=1+(c*3)%5;t.height=1+(c*7)%6;t.offset=256;t.length=2*t.width*t.height;t.animationOffsetX=9-int(c%19);t.animationOffsetY=5-int(c%11);
  auto m=s;m.mirrorOffset=-2;m.animationOffsetX=-s.animationOffsetX;m.animationOffsetY=-s.animationOffsetY;
  auto twice=s;twice.mirrorOffset=-1;twice.animationOffsetX=3;twice.animationOffsetY=-4;
  a.sprites={s,t,m,twice};const std::vector<uint32_t> ids={3,2,1,4,3};
  for(bool align:{false,true}){
   auto result=prepareSG3Sequence(a,ids,path.string(),align);need(result.valid(),"authored sequence valid");need(result.physicalIds==ids,"explicit IDs retained");
   int left=999,top=999,right=-999,bottom=-999;
   for(auto id:ids){auto r=a.sprites[id-1];int x=align?-r.animationOffsetX:0,y=align?-r.animationOffsetY:0;left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+r.width);bottom=std::max(bottom,y+r.height);}
   size_t w=right-left,h=bottom-top;
   need(result.cacheBytes==w*h*4*ids.size(),"normalized cache accounting");
   for(size_t k=0;k<ids.size();++k){auto id=ids[k];auto r=a.sprites[id-1];std::vector<uint32_t> want(w*h,0);
    int dx=(align?-r.animationOffsetX:0)-left,dy=(align?-r.animationOffsetY:0)-top;
    for(int y=0;y<r.height;++y)for(int x=0;x<r.width;++x){size_t sx=id==3?size_t(r.width-1-x):size_t(x);size_t start=id==2?128:0;want[(y+dy)*w+x+dx]=expected[(start+y*r.width+sx)%5];}
    need(result.frames[k].width==w&&result.frames[k].height==h&&result.frames[k].pixelsBGRA==want,"independent scalar placement/mirror oracle");++canvases;
   }
   auto baseline=result.frames;
   for(auto& r:a.sprites){r.animationSpriteCount=65535;r.animationReversible=255;r.animationSpeedId=255;}
   auto changed=prepareSG3Sequence(a,ids,path.string(),align);need(changed.valid(),"raw metadata cannot expand sequence");
   for(size_t k=0;k<ids.size();++k)need(changed.frames[k].pixelsBGRA==baseline[k].pixelsBGRA,"metadata not playback semantics");
  }
 }
 SG3Archive a;a.version=213;SG3Sprite s;s.width=1;s.height=1;s.type=1;s.length=2;a.sprites={s,s};
 for(int offset:{-32768,32767}){a.sprites[0].animationOffsetX=offset;a.sprites[0].animationOffsetY=offset;auto r=prepareSG3Sequence(a,{1},path.string(),true);need(r.valid()&&r.frames[0].width==1&&r.frames[0].height==1,"absolute offset translation cancels on single frame");}
 a.sprites[1].animationOffsetX=-32768;need(!prepareSG3Sequence(a,{1,2},path.string(),true).valid(),"extreme offset span rejected");
 a.sprites[0]=s;a.sprites[1]=s;a.sprites[1].animationOffsetX=-255;a.sprites[1].animationOffsetY=-255;
 std::vector<uint32_t> limit(128,1);limit.back()=2;auto exact=prepareSG3Sequence(a,limit,path.string(),true);need(exact.valid()&&exact.cacheBytes==32u*1024u*1024u,"32MiB common canvas exact boundary accepted");limit.push_back(1);auto over=prepareSG3Sequence(a,limit,path.string(),true);need(!over.valid()&&over.frames.empty()&&over.physicalIds.empty(),"32MiB plus one frame atomically rejected");
 for(int bad=0;bad<8;++bad){a.sprites={s,s};auto& r=a.sprites[1];switch(bad){case 0:r.width=0;break;case 1:r.width=-1;break;case 2:r.length=1;break;case 3:r.offset=999999;break;case 4:r.type=65535;break;case 5:r.mirrorOffset=-99;break;case 6:r.mirrorOffset=1;break;case 7:r.mirrorOffset=-1;a.sprites[0].mirrorOffset=1;break;}auto fail=prepareSG3Sequence(a,{1,2,1},path.string(),true);need(!fail.valid()&&fail.frames.empty()&&fail.physicalIds.empty(),"late corrupt frame cannot publish prefix");}
 for(size_t n=1;n<=256;++n){std::string text;std::vector<uint32_t> want;for(size_t k=0;k<n;++k){if(k)text+=" , ";auto id=1+(k*17)%99;text+=std::to_string(id);want.push_back(id);}auto got=parseSG3SequenceIds(text,99);need(got&&*got==want,"parser preserves authored arbitrary order at every supported length");}
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"independent_pixel_canvases\":"<<canvases<<",\"authored_only\":true}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
