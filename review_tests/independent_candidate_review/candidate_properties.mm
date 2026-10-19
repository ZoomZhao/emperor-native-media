#include "game_lab/app/building_candidate_config.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace game_lab;
static size_t checks=0,views=0,pixels=0;
static void need(bool b,const char*s){++checks;if(!b)throw std::runtime_error(s);}
int main(){@autoreleasepool{try{
 OriginalMapTerrain t;t.activeSide=112;t.flags.assign(51984,0x80000);for(size_t y=58;y<170;++y)for(size_t x=58;x<170;++x)t.flags[y*228+x]=8|((x+y)%2?64:0);
 OriginalBuildingCandidate c;c.source="Save/authored.sav";c.archive="DATA/authored.sg3";c.physicalId=1;c.sourceRevision=77;c.target.type=72;c.target.x=7;c.target.y=9;c.target.size=2;c.target.slot=1;c.target.recordOffset=100;c.pixels.width=3;c.pixels.height=2;c.pixels.pixelsBGRA={0,0xffff0000u,0x80008000u,0x01010000u,0x7f3f7f01u,0xfe0000feu};c.imageRevision=originalCandidateImageRevision(c.pixels);
 for(bool market:{false,true})for(unsigned scale:{1u,2u,3u,4u})for(double phase:{0.,.25,.5,.75})for(bool clip:{false,true}){
  auto f=c.target;if(market){f.type=59;f.size=1;f.width=7;f.height=4;}auto q=c;q.target=f;OriginalMapCamera camera;camera.scale=scale;camera.width=clip?9:320;camera.height=clip?7:240;
  double wx=f.x+(originalFootprintWidth(f)-1)/2.,wy=f.y+(originalFootprintHeight(f)-1)/2.;camera.centerX=wx+phase;camera.centerY=wy-phase;
  auto base=renderOriginalBuildingFootprints(t,{f},camera);auto got=renderOriginalBuildingCandidate(t,{f},camera,q.source,77,q);need(got.accepted&&got.image.valid(),"valid pinned candidate");
  auto want=base.image.pixelsBGRA;size_t writes=0;double cx=camera.width/2.+40*scale*((wx-camera.centerX)-(wy-camera.centerY)),cy=camera.height/2.+20*scale*((wx-camera.centerX)+(wy-camera.centerY));
  int left=int(std::floor(cx-1.5*scale+.5)),top=int(std::floor(cy-2*scale+.5));
  for(size_t y=0;y<2;++y)for(size_t x=0;x<3;++x){auto src=q.pixels.pixelsBGRA[y*3+x];unsigned alpha=src>>24;if(!alpha)continue;
   for(unsigned dy=0;dy<scale;++dy)for(unsigned dx=0;dx<scale;++dx){int px=left+int(x*scale+dx),py=top+int(y*scale+dy);if(px<0||py<0||px>=int(camera.width)||py>=int(camera.height))continue;auto&dst=want[size_t(py)*camera.width+px];uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u})mixed|=uint32_t(std::floor(double((src>>shift)&255)+double((dst>>shift)&255)*(1.-double(alpha)/255)+.5))<<shift;dst=mixed;++writes;}
  }
  need(got.image.pixelsBGRA==want&&got.paintedPixels==writes,"whole premultiplied source-over and nearest-scale oracle");need(q.pixels.pixelsBGRA==c.pixels.pixelsBGRA,"source pixels immutable");++views;pixels+=want.size();
 }
 OriginalMapCamera camera;camera.width=320;camera.height=240;camera.centerX=7.5;camera.centerY=9.5;
 for(int field=0;field<8;++field){auto f=c.target;if(field==0)f.slot++;if(field==1)f.recordOffset++;if(field==2)f.type++;if(field==3)f.x++;if(field==4)f.y++;if(field==5)f.size++;if(field==6)f.width=3;if(field==7)f.height=3;need(!renderOriginalBuildingCandidate(t,{f},camera,c.source,77,c).accepted,"every target-identity field binds candidate");}
 need(!renderOriginalBuildingCandidate(t,{c.target},camera,"Save/other.sav",77,c).accepted&&!renderOriginalBuildingCandidate(t,{c.target},camera,c.source,78,c).accepted,"source and revision required");
 for(uint32_t dim:{0u,2049u,4097u,std::numeric_limits<uint32_t>::max()}){auto q=c;q.pixels.width=dim;need(!validOriginalCandidate(q)&&!renderOriginalBuildingCandidate(t,{c.target},camera,c.source,77,q).accepted,"huge or invalid image dimensions rejected before render");}
 NSDictionary*config=encodeOriginalCandidate(c);need(config.count==16&&!config[@"confirmed"]&&!config[@"pixels"],"reference-only metadata");
 for(NSString*key in @[@"schema",@"physicalId",@"slot",@"recordOffset",@"type",@"x",@"y",@"size",@"width",@"height"]){
  for(id value in @[@YES,@(-1),@1.5,@"1",NSNull.null,@[],@{},[NSDecimalNumber decimalNumberWithString:@"1.0"],[NSDecimalNumber notANumber]]){auto q=[config mutableCopy];q[key]=value;need(!decodeOriginalCandidate(q),"wrong numeric runtime type,negative ordecimal refused");}
  auto q=[config mutableCopy];q[key]=@(std::numeric_limits<unsigned long long>::max());need(!decodeOriginalCandidate(q),"huge unsigned metadata refused");
 }
 for(NSString*key in @[@"sourceRevision",@"imageRevision"]){for(NSString*v in @[@"",@"0",@"-000000000000001",@"+000000000000001",@"gggggggggggggggg",@"00000000000000000"]){auto q=[config mutableCopy];q[key]=v;need(!decodeOriginalCandidate(q),"revision text bounds");}}
 for(NSString*key in @[@"source",@"archive"]){for(NSString*v in @[@"/absolute",@"../escape",@"a/../b",@"./x",@""]){auto q=[config mutableCopy];q[key]=v;need(!decodeOriginalCandidate(q),"root-relative reference required");}auto q=[config mutableCopy];q[key]=[[NSString alloc]initWithBytes:"a\0b" length:3 encoding:NSUTF8StringEncoding];need(!decodeOriginalCandidate(q),"embedded NUL refused");}
 for(NSString*v in @[@"confirmed",@"verified"]){auto q=[config mutableCopy];q[@"status"]=v;need(!decodeOriginalCandidate(q),"cannot promote candidate");}auto q=[config mutableCopy];q[@"confirmed"]=@YES;need(!decodeOriginalCandidate(q),"extra confirmed key refused");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"whole_pixel_views\":"<<views<<",\"pixels_compared\":"<<pixels<<",\"confirmed\":0,\"GUI_operations\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}return 0;}
