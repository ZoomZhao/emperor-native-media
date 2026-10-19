#include "sg3_sequence.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
using namespace game_lab;
size_t checks=0;
void need(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
int main(){try{
 auto ids=parseSG3SequenceIds(" 2, 1,2 ",3);need(ids&&*ids==std::vector<uint32_t>({2,1,2}),"explicit order/repeats preserved");
 for(const auto* text:{"","0","-1","+1","1.0","1,,2","1,","1, ","4294967296","4","1 2","1x","1,0"})need(!parseSG3SequenceIds(text,3),"invalid physical-ID list refused");
 need(!parseSG3SequenceIds("1",0)&&!parseSG3SequenceIds(std::string(4097,'1'),3),"parser budgets");
 std::string many="1";for(size_t i=1;i<256;++i)many+=",1";need(parseSG3SequenceIds(many,3)->size()==256,"256 explicit IDs accepted");need(!parseSG3SequenceIds(many+",1",3),"257 IDs refused");
 need(sg3SequenceStep(0,3,true)==2&&sg3SequenceStep(2,3)==0&&sg3SequenceStep(0,3)==1,"manual preview endpoints wrap explicitly");need(sg3SequenceStep(0,0)==0&&sg3SequenceStep(4,3)==0,"empty/stale playhead safe");
 for(double rate:{1.,8.,30.})need(validSG3PreviewRate(rate),"user preview rate accepted");
 for(double rate:{0.,-1.,30.01,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})need(!validSG3PreviewRate(rate),"invalid preview rate refused");
 auto dir=std::filesystem::temp_directory_path()/("sg3-sequence-authored-"+std::to_string(getpid()));need(std::filesystem::create_directory(dir),"fresh authored directory");auto data=dir/"own.555";
 {std::ofstream f(data,std::ios::binary);const unsigned char bytes[]={0,0x7c,0xe0,3,0x1f,0,0xff,0x7f};f.write(reinterpret_cast<const char*>(bytes),sizeof(bytes));need(bool(f),"authored pixels written");}
 SG3Archive a;a.version=213;a.numImages=4;a.capacity=5;SG3Sprite first;first.width=2;first.height=1;first.type=1;first.length=4;first.animationSpriteCount=65535;first.animationSpeedId=255;first.animationReversible=255;
 auto second=first;second.offset=4;second.animationOffsetX=1;second.animationOffsetY=-1;auto mirror=first;mirror.mirrorOffset=-2;auto empty=first;empty.width=0;empty.height=0;empty.length=0;a.sprites={first,second,mirror,empty};
 auto seq=prepareSG3Sequence(a,{2,1,2},data.string(),false);need(seq.valid()&&seq.physicalIds==std::vector<uint32_t>({2,1,2})&&seq.frames.size()==3,"physical ID1 maps vector0;no count or direction expansion");
 need(seq.frames[0].pixelsBGRA[0]==0xff0000ff&&seq.frames[1].pixelsBGRA[0]==0xffff0000&&seq.frames[2].pixelsBGRA==seq.frames[0].pixelsBGRA,"explicit decoded colors and repetitions exact");
 auto aligned=prepareSG3Sequence(a,{1,2},data.string(),true);need(aligned.valid()&&aligned.frames[0].width==3&&aligned.frames[0].height==2&&aligned.frames[1].width==3,"all frames use fixed union canvas");
 need(aligned.frames[0].pixelsBGRA==std::vector<uint32_t>({0,0xffff0000,0xff00ff00,0,0,0})&&aligned.frames[1].pixelsBGRA==std::vector<uint32_t>({0,0,0,0xff0000ff,0xffffffff,0}),"signed metadata alignment convention exact;transparent pixels preserved");
 auto mirrored=prepareSG3Sequence(a,{3},data.string(),true);need(mirrored.valid()&&mirrored.frames[0].pixelsBGRA==std::vector<uint32_t>({0xff00ff00,0xffff0000}),"mirror record flips pixels;not time/order");
 for(auto list:{std::vector<uint32_t>{},std::vector<uint32_t>{0},std::vector<uint32_t>{5},std::vector<uint32_t>{1,4,2},std::vector<uint32_t>(257,1)}){auto fail=prepareSG3Sequence(a,list,data.string(),true);need(!fail.valid()&&fail.frames.empty()&&fail.physicalIds.empty(),"any failed/empty frame clears whole list;no ID compaction");}
 need(!prepareSG3Sequence(a,{1},(dir/"missing.555").string(),true).valid(),"missing source no stale frames");
 auto extremes=a;extremes.sprites[0].animationOffsetX=32767;extremes.sprites[1].animationOffsetX=-32768;need(!prepareSG3Sequence(extremes,{1,2},data.string(),true).valid(),"signed offset canvas span bounded before allocation");
 auto large=a;large.sprites[1].animationOffsetX=-1024;large.sprites[1].animationOffsetY=-1024;std::vector<uint32_t> repeat(256);for(size_t i=0;i<256;++i)repeat[i]=uint32_t(i%2+1);need(!prepareSG3Sequence(large,repeat,data.string(),true).valid(),"common frame cache budget");
 std::filesystem::remove_all(dir);std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"original_data\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
