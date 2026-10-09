#include "backend.hpp"
#include "../surface_lab/oracle.hpp"
#import <Foundation/Foundation.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

using surface_lab::Desc;using SurfaceRect=surface_lab::Rect;using surface_lab::Format;using SurfaceHandle=surface_lab::Handle;using surface_lab::Store;
using metal_lab::Backend;using metal_lab::Ticket;using metal_lab::Status;
namespace {
size_t assertions=0,cases=0,bytes=0,callbacks=0,cycles=0,randomCases=0;
uint64_t rng=0x6d37a15eed202601ULL;std::string directory;
void check(bool ok,const char* what){++assertions;if(!ok)throw std::runtime_error(what);}
#define CHECK(x) check((x),#x)
uint64_t next(){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return rng;}int below(int n){return int(next()%uint64_t(n));}
std::vector<uint8_t> pattern(Desc d,int salt){std::vector<uint8_t> out(d.stride*size_t(d.height),uint8_t(0xc0+salt%32));
    for(int y=0;y<d.height;++y)for(int x=0;x<d.width;++x){const size_t p=size_t(y)*d.stride+size_t(x)*(d.format==Format::RGB565LE?2:4);
        if(d.format==Format::RGB565LE){const unsigned w=unsigned(x*31+y*2053+salt*109)%65536;out[p]=uint8_t(w);out[p+1]=uint8_t(w>>8);}
        else{out[p]=uint8_t(x*7+salt);out[p+1]=uint8_t(y*29+salt*3);out[p+2]=uint8_t(x*17+y*11+salt*5);out[p+3]=uint8_t(x+y+salt*13);}}return out;}
void save(const char* name,const std::vector<uint8_t>& v){std::ofstream out(directory+"/"+name,std::ios::binary);out.write(reinterpret_cast<const char*>(v.data()),std::streamsize(v.size()));CHECK(bool(out));}
void one(Backend& gpu,Desc sd,Desc dd,SurfaceRect sr,SurfaceRect dr,bool scale,bool overlap=false,bool golden=false){
    Store store;SurfaceHandle src=0,dst=0;CHECK(store.create(sd,src)==surface_lab::Status::Ok);
    if(overlap){CHECK(sd.width==dd.width&&sd.height==dd.height&&sd.stride==dd.stride&&sd.format==dd.format);dst=src;}
    else CHECK(store.create(dd,dst)==surface_lab::Status::Ok);
    const auto input=pattern(sd,3),original=overlap?input:pattern(dd,17);
    CHECK(store.replaceBytes(src,input)==surface_lab::Status::Ok);CHECK(store.replaceBytes(dst,original)==surface_lab::Status::Ok);
    const auto expected=scalar_oracle::expected(input,sd,original,dd,sr,dr);Ticket ticket;
    CHECK(gpu.submit(store,dst,src,sr,dr,scale,ticket)==Status::Ok);std::vector<uint8_t> result;
    CHECK(Backend::finish(store,ticket,&result)==Status::Ok);CHECK(Backend::callbackCount(ticket)==1);++callbacks;
    if(result!=expected){save("failure_expected.bin",expected);save("failure_actual.bin",result);}CHECK(result==expected);
    CHECK(Backend::finish(store,ticket)==Status::AlreadyApplied);
    std::vector<uint8_t> committed;CHECK(store.readBytes(dst,committed)==surface_lab::Status::Ok);CHECK(committed==result);
    // Same public operation on the CPU component: both must independently match the scalar oracle.
    Store cpu;SurfaceHandle cs=0,cd=0;CHECK(cpu.create(sd,cs)==surface_lab::Status::Ok);
    if(overlap)cd=cs;else CHECK(cpu.create(dd,cd)==surface_lab::Status::Ok);
    CHECK(cpu.replaceBytes(cs,input)==surface_lab::Status::Ok);CHECK(cpu.replaceBytes(cd,original)==surface_lab::Status::Ok);
    CHECK((scale?cpu.nearest(cd,cs,sr,dr):cpu.copy(cd,cs,sr,dr))==surface_lab::Status::Ok);
    CHECK(cpu.readBytes(cd,committed)==surface_lab::Status::Ok);CHECK(committed==expected);
    ++cases;bytes+=result.size();
    if(golden){save("fixture_source.bin",input);save("fixture_destination_before.bin",original);save("fixture_expected.bin",expected);save("fixture_actual.bin",result);}
}
void formats(Backend& gpu){for(auto sf:{Format::RGB565LE,Format::BGRA8})for(auto df:{Format::RGB565LE,Format::BGRA8}){
    const int sb=sf==Format::RGB565LE?2:4,db=df==Format::RGB565LE?2:4;
    one(gpu,{7,5,size_t(7*sb+3),sf},{11,8,size_t(11*db+5),df},{-2,-1,9,7},{-3,-2,14,10},true);
    one(gpu,{7,5,size_t(7*sb+3),sf},{11,8,size_t(11*db+5),df},{0,0,7,5},{8,6,7,5},false);}
    one(gpu,{3,2,17,Format::BGRA8},{5,4,23,Format::BGRA8},{0,0,3,2},{-1,-1,5,4},true,false,true);
    one(gpu,{17,13,71,Format::BGRA8},{13,11,55,Format::BGRA8},{0,0,17,13},{0,0,13,11},true);
    for(auto f:{Format::RGB565LE,Format::BGRA8}){const size_t stride=9*(f==Format::RGB565LE?2u:4u)+3;
        one(gpu,{9,7,stride,f},{9,7,stride,f},{0,0,7,5},{1,1,7,5},false,true);
        one(gpu,{9,7,stride,f},{9,7,stride,f},{1,1,7,5},{-1,0,9,7},true,true);}
    // Exhaust all 565 words on GPU; independent arithmetic oracle and canonical endpoints.
    Store store;SurfaceHandle s=0,d=0;Desc sd{256,256,515,Format::RGB565LE},dd{256,256,1029,Format::BGRA8};
    CHECK(store.create(sd,s)==surface_lab::Status::Ok);CHECK(store.create(dd,d)==surface_lab::Status::Ok);
    auto in=pattern(sd,1),original=pattern(dd,2);for(unsigned w=0;w<65536;++w){const size_t p=(w/256)*sd.stride+(w%256)*2;in[p]=uint8_t(w);in[p+1]=uint8_t(w>>8);}
    CHECK(store.replaceBytes(s,in)==surface_lab::Status::Ok);CHECK(store.replaceBytes(d,original)==surface_lab::Status::Ok);
    const auto expected=scalar_oracle::expected(in,sd,original,dd,{0,0,256,256},{0,0,256,256});Ticket t;
    CHECK(gpu.submit(store,d,s,{0,0,256,256},{0,0,256,256},false,t)==Status::Ok);std::vector<uint8_t> out;
    CHECK(Backend::finish(store,t,&out)==Status::Ok);CHECK(out==expected);CHECK(out[0]==0&&out[1]==0&&out[2]==0&&out[3]==255);
    const size_t last=255*dd.stride+255*4;CHECK(out[last]==255&&out[last+1]==255&&out[last+2]==255);
    CHECK(Backend::callbackCount(t)==1);++callbacks;++cases;bytes+=out.size();
}
void randomPixels(Backend& gpu){for(int n=0;n<512;++n){const auto sf=below(2)?Format::BGRA8:Format::RGB565LE,df=below(2)?Format::BGRA8:Format::RGB565LE;
    const int sw=1+below(25),sh=1+below(20),dw=1+below(25),dh=1+below(20);const bool scale=below(2);
    Desc sd{sw,sh,size_t(sw*(sf==Format::BGRA8?4:2)+1+below(9)),sf},dd{dw,dh,size_t(dw*(df==Format::BGRA8?4:2)+1+below(9)),df};
    SurfaceRect sr{-5+below(sw+10),-5+below(sh+10),1+below(28),1+below(23)},dr{-5+below(dw+10),-5+below(dh+10),1+below(28),1+below(23)};
    if(!scale){dr.width=sr.width;dr.height=sr.height;}one(gpu,sd,dd,sr,dr,scale);++randomCases;}}
void lifecycle(Backend& gpu){Store store;Desc d{7,5,31,Format::BGRA8};SurfaceHandle s=0,h=0;CHECK(store.create(d,s)==surface_lab::Status::Ok);CHECK(store.create(d,h)==surface_lab::Status::Ok);
    const auto in=pattern(d,3),original=pattern(d,9);CHECK(store.replaceBytes(s,in)==surface_lab::Status::Ok);CHECK(store.replaceBytes(h,original)==surface_lab::Status::Ok);
    auto submit=[&](Ticket& t){CHECK(gpu.submit(store,h,s,{0,0,7,5},{0,0,7,5},false,t)==Status::Ok);};
    Ticket t;submit(t);CHECK(store.destroy(h)==surface_lab::Status::Ok);SurfaceHandle fresh=0;CHECK(store.create(d,fresh)==surface_lab::Status::Ok);CHECK(fresh!=h);
    CHECK(Backend::finish(store,t)==Status::SurfaceRejected);CHECK(Backend::callbackCount(t)==1);++callbacks;
    std::vector<uint8_t> v;CHECK(store.readBytes(fresh,v)==surface_lab::Status::Ok);CHECK(v==std::vector<uint8_t>(d.stride*5,0));h=fresh;
    submit(t);CHECK(store.lose(h)==surface_lab::Status::Ok);CHECK(Backend::finish(store,t)==Status::SurfaceRejected);CHECK(Backend::callbackCount(t)==1);++callbacks;
    CHECK(store.restore(h)==surface_lab::Status::Ok);CHECK(store.readBytes(h,v)==surface_lab::Status::Ok);CHECK(v==std::vector<uint8_t>(d.stride*5,0));
    submit(t);CHECK(store.replaceBytes(h,original)==surface_lab::Status::Ok);CHECK(Backend::finish(store,t)==Status::ChangedDestination);CHECK(Backend::callbackCount(t)==1);++callbacks;
    CHECK(store.readBytes(h,v)==surface_lab::Status::Ok);CHECK(v==original);
    Ticket untouched;CHECK(gpu.submit(store,h,s,{0,0,0,2},{0,0,2,2},true,untouched)==Status::InvalidRect);
    CHECK(gpu.submit(store,h,s,{0,0,1,2},{0,0,2,2},false,untouched)==Status::InvalidRect);
    CHECK(gpu.submit(store,h,s,{2147483647,0,2,2},{0,0,2,2},true,untouched)==Status::UnsupportedRect);
    CHECK(!untouched.job);CHECK(store.readBytes(h,v)==surface_lab::Status::Ok);CHECK(v==original);
    // Snapshot ownership: source can be overwritten immediately after submit.
    const auto immutableExpected=scalar_oracle::expected(in,d,original,d,{0,0,7,5},{0,0,7,5});
    submit(t);CHECK(store.replaceBytes(s,original)==surface_lab::Status::Ok);CHECK(Backend::finish(store,t,&v)==Status::Ok);
    CHECK(v==immutableExpected);CHECK(Backend::callbackCount(t)==1);++callbacks;
    for(int i=0;i<8;++i){auto owner=std::make_unique<Backend>();CHECK(owner->available());
        CHECK(owner->submit(store,h,s,{0,0,7,5},{0,0,7,5},false,t)==Status::Ok);owner.reset();
        CHECK(Backend::finish(store,t,&v)==Status::Ok);CHECK(v==original);CHECK(Backend::callbackCount(t)==1);++callbacks;++cycles;
        std::weak_ptr<metal_lab::Job> weak=t.job;t.job.reset();CHECK(weak.expired());}
    // Eight outstanding command buffers target different handles; each completion exactly once.
    std::vector<Ticket> pending(8);std::vector<SurfaceHandle> dests(8);
    for(size_t i=0;i<pending.size();++i){CHECK(store.create(d,dests[i])==surface_lab::Status::Ok);
        CHECK(gpu.submit(store,dests[i],s,{0,0,7,5},{0,0,7,5},false,pending[i])==Status::Ok);}
    const auto freshExpected=scalar_oracle::expected(original,d,std::vector<uint8_t>(d.stride*5,0),d,{0,0,7,5},{0,0,7,5});
    for(auto& job:pending){CHECK(Backend::finish(store,job,&v)==Status::Ok);CHECK(v==freshExpected);CHECK(Backend::callbackCount(job)==1);++callbacks;}
}
void summary(bool pass,bool available,NSString* reason,NSString* device){NSDictionary* dict=@{@"passed":@(pass),@"available":@(available),@"reason":reason,
    @"device":device,@"assertions":@(assertions),@"pixel_cases":@(cases),@"bytes_compared":@(bytes),@"completion_callbacks":@(callbacks),
    @"backend_rebuild_cycles":@(cycles),@"random_cases":@(randomCases),@"seed_hex":@"0x6d37a15eed202601",@"final_rng_hex":[NSString stringWithFormat:@"0x%016llx",(unsigned long long)rng],
    @"GPU_compute_and_readback":@(available),@"window_or_display_used":@NO,@"actual_device_loss_tested":@NO};
    NSData* data=[NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:nil];
    [data writeToFile:[NSString stringWithUTF8String:(directory+"/summary.json").c_str()] atomically:YES];}
}
int main(int argc,char** argv){@autoreleasepool{if(argc!=2)return 2;directory=argv[1];Backend gpu;
    if(!gpu.available()){summary(false,false,[NSString stringWithUTF8String:gpu.reason().c_str()],@"");std::cout<<"BLOCKED Metal: "<<gpu.reason()<<"\n";return 3;}
    try{formats(gpu);std::cout<<"PASS Metal format/stride/clip/nearest/overlap and 65536 RGB565 values\n";
        randomPixels(gpu);std::cout<<"PASS Metal 512 deterministic pixel cases\n";
        lifecycle(gpu);std::cout<<"PASS Metal asynchronous completion, stale destination, source ownership and backend rebuild\n";
        summary(true,true,@"",[NSString stringWithUTF8String:gpu.deviceName().c_str()]);std::cout<<"PASS Metal: "<<cases<<" cases, "<<bytes<<" bytes, "<<callbacks<<" completion callbacks\n";return 0;
    }catch(const std::exception& e){summary(false,true,[NSString stringWithUTF8String:e.what()],[NSString stringWithUTF8String:gpu.deviceName().c_str()]);std::cerr<<"FAIL Metal "<<e.what()<<"\n";return 1;}}}
