#include "backend.hpp"
#import <Metal/Metal.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace metal_lab {
using namespace surface_lab;
using SurfaceHandle=surface_lab::Handle;using SurfaceRect=surface_lab::Rect;
struct Completion { std::mutex mutex; std::condition_variable cv; bool done=false, success=false; unsigned callbacks=0; };
struct Job {
    id<MTLCommandBuffer> command;
    id<MTLBuffer> readback;
    SurfaceHandle destination=0; Snapshot original; bool applied=false;size_t bufferBytes=0;
};
struct Backend::Impl { id<MTLDevice> device; id<MTLCommandQueue> queue; id<MTLComputePipelineState> pipeline;
    std::string reason; };
namespace {
const char* shader = R"METAL(
#include <metal_stdlib>
using namespace metal;
struct P { int sw,sh,ss,sf,dw,dh,ds,df,sx,sy,rw,rh,dx,dy,tw,th; };
kernel void transfer(device const uchar* src [[buffer(0)]], device uchar* dst [[buffer(1)]],
    constant P& p [[buffer(2)]], uint2 at [[thread_position_in_grid]]) {
    if(at.x>=uint(p.dw)||at.y>=uint(p.dh))return;
    int x=int(at.x),y=int(at.y);
    if(x<p.dx||y<p.dy||x>=p.dx+p.tw||y>=p.dy+p.th)return;
    int sx=p.sx+(x-p.dx)*p.rw/p.tw,sy=p.sy+(y-p.dy)*p.rh/p.th;
    if(sx<0||sy<0||sx>=p.sw||sy>=p.sh)return;
    int sb=p.sf==2?4:2,db=p.df==2?4:2;
    int si=sy*p.ss+sx*sb,di=y*p.ds+x*db; uint4 c;
    if(p.sf==2)c=uint4(src[si],src[si+1],src[si+2],src[si+3]);
    else if(p.sf==3){uint w=uint(src[si])|(uint(src[si+1])<<8),b=w&31,g=(w>>5)&31,r=(w>>10)&31;
        c=uint4((b<<3)|(b>>2),(g<<3)|(g>>2),(r<<3)|(r>>2),255);}
    else { uint w=uint(src[si])|(uint(src[si+1])<<8),b=w&31,g=(w>>5)&63,r=(w>>11)&31;
        c=uint4((b<<3)|(b>>2),(g<<2)|(g>>4),(r<<3)|(r>>2),255); }
    if(p.df==2){dst[di]=uchar(c.x);dst[di+1]=uchar(c.y);dst[di+2]=uchar(c.z);dst[di+3]=uchar(c.w);}
    else if(p.df==3){uint w=((c.z>>3)<<10)|((c.y>>3)<<5)|(c.x>>3);dst[di]=uchar(w&255);dst[di+1]=uchar(w>>8);}
    else{uint w=((c.z>>3)<<11)|((c.y>>2)<<5)|(c.x>>3);dst[di]=uchar(w&255);dst[di+1]=uchar(w>>8);}
}
)METAL";
struct Params { int32_t sw,sh,ss,sf,dw,dh,ds,df,sx,sy,rw,rh,dx,dy,tw,th; };
bool supported(SurfaceRect r){return r.x>=-4096&&r.x<=4096&&r.y>=-4096&&r.y<=4096&&r.width<=4096&&r.height<=4096;}
bool same(const Snapshot& a,const Snapshot& b){return a.desc.width==b.desc.width&&a.desc.height==b.desc.height&&
    a.desc.stride==b.desc.stride&&a.desc.format==b.desc.format&&a.lost==b.lost&&a.bytes==b.bytes;}
}
Backend::Backend():impl_(new Impl){
    impl_->device=MTLCreateSystemDefaultDevice();
    if(!impl_->device){NSArray<id<MTLDevice>>* devices=MTLCopyAllDevices();
        if(devices.count)impl_->device=devices.firstObject;
        else{impl_->reason="MTLCreateSystemDefaultDevice returned nil; MTLCopyAllDevices returned 0 devices in this execution context";return;}}
    impl_->queue=[impl_->device newCommandQueue];NSError* error=nil;
    id<MTLLibrary> library=[impl_->device newLibraryWithSource:[NSString stringWithUTF8String:shader] options:nil error:&error];
    if(!library){impl_->reason=error?error.localizedDescription.UTF8String:"Metal source compiler unavailable";return;}
    id<MTLFunction> fn=[library newFunctionWithName:@"transfer"];
    impl_->pipeline=[impl_->device newComputePipelineStateWithFunction:fn error:&error];
    if(!impl_->pipeline||!impl_->queue)impl_->reason=error?error.localizedDescription.UTF8String:"Metal queue/pipeline creation failed";
}
Backend::~Backend()=default;
bool Backend::available() const{return bool(impl_->pipeline)&&bool(impl_->queue);}
std::string Backend::reason() const{return impl_->reason;}
std::string Backend::deviceName() const{return impl_->device?std::string(impl_->device.name.UTF8String):"";}
Status Backend::submit(Store& store,SurfaceHandle dst,SurfaceHandle src,SurfaceRect sr,SurfaceRect dr,bool nearest,Ticket& ticket,std::function<void(bool)> completed){
    if(!available())return Status::Unavailable;
    Snapshot in,out;if(store.snapshot(src,in)!=surface_lab::Status::Ok||store.snapshot(dst,out)!=surface_lab::Status::Ok||in.lost||out.lost)return Status::SurfaceRejected;
    if(sr.width<=0||sr.height<=0||dr.width<=0||dr.height<=0||(!nearest&&(sr.width!=dr.width||sr.height!=dr.height)))return Status::InvalidRect;
    // Metal int arithmetic and buffer indices are explicitly bounded. Larger CPU-only rectangles are rejected.
    if(!supported(sr)||!supported(dr)||in.desc.stride>1048576||out.desc.stride>1048576||
        in.bytes.size()>67108864||out.bytes.size()>67108864)return Status::UnsupportedRect;
    auto job=std::make_shared<Job>();job->destination=dst;job->original=out;
    id<MTLBuffer> uploadIn=[impl_->device newBufferWithBytes:in.bytes.data() length:in.bytes.size() options:MTLResourceStorageModeShared];
    id<MTLBuffer> uploadOut=[impl_->device newBufferWithBytes:out.bytes.data() length:out.bytes.size() options:MTLResourceStorageModeShared];
    id<MTLBuffer> gpuIn=[impl_->device newBufferWithLength:in.bytes.size() options:MTLResourceStorageModePrivate];
    id<MTLBuffer> gpuOut=[impl_->device newBufferWithLength:out.bytes.size() options:MTLResourceStorageModePrivate];
    job->readback=[impl_->device newBufferWithLength:out.bytes.size() options:MTLResourceStorageModeShared];
    if(!uploadIn||!uploadOut||!gpuIn||!gpuOut||!job->readback)return Status::GpuError;
    job->bufferBytes=uploadIn.length+uploadOut.length+gpuIn.length+gpuOut.length+job->readback.length;
    job->command=[impl_->queue commandBuffer];if(!job->command)return Status::GpuError;
    id<MTLBlitCommandEncoder> blit=[job->command blitCommandEncoder];
    [blit copyFromBuffer:uploadIn sourceOffset:0 toBuffer:gpuIn destinationOffset:0 size:in.bytes.size()];
    [blit copyFromBuffer:uploadOut sourceOffset:0 toBuffer:gpuOut destinationOffset:0 size:out.bytes.size()];[blit endEncoding];
    Params p{in.desc.width,in.desc.height,int32_t(in.desc.stride),int32_t(in.desc.format),out.desc.width,out.desc.height,
        int32_t(out.desc.stride),int32_t(out.desc.format),sr.x,sr.y,sr.width,sr.height,dr.x,dr.y,dr.width,dr.height};
    id<MTLComputeCommandEncoder> compute=[job->command computeCommandEncoder];
    [compute setComputePipelineState:impl_->pipeline];[compute setBuffer:gpuIn offset:0 atIndex:0];
    [compute setBuffer:gpuOut offset:0 atIndex:1];[compute setBytes:&p length:sizeof(p) atIndex:2];
    [compute dispatchThreads:MTLSizeMake(size_t(out.desc.width),size_t(out.desc.height),1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[compute endEncoding];
    blit=[job->command blitCommandEncoder];[blit copyFromBuffer:gpuOut sourceOffset:0 toBuffer:job->readback destinationOffset:0 size:out.bytes.size()];[blit endEncoding];
    auto done=std::make_shared<Completion>();
    [job->command addCompletedHandler:^(id<MTLCommandBuffer> command){
        bool success=command.status==MTLCommandBufferStatusCompleted;
        // Client callbacks publish metadata only and must not wait for this same ticket.
        try{if(completed)completed(success);}catch(...){success=false;}
        {std::lock_guard<std::mutex> lock(done->mutex);++done->callbacks;done->success=success;done->done=true;}done->cv.notify_all();
    }];
    ticket.job=job;ticket.completion=done;[job->command commit];return Status::Ok;
}
Status Backend::wait(const Ticket& ticket){if(!ticket.completion)return Status::GpuError;
    auto& done=*ticket.completion;std::unique_lock<std::mutex> lock(done.mutex);
    if(!done.cv.wait_for(lock,std::chrono::seconds(10),[&done]{return done.done;}))return Status::Timeout;
    return done.success?Status::Ok:Status::GpuError;
}
unsigned Backend::callbackCount(const Ticket& ticket){if(!ticket.completion)return 0;std::lock_guard<std::mutex> lock(ticket.completion->mutex);return ticket.completion->callbacks;}
size_t Backend::retainedBufferBytes(const Ticket& ticket){return ticket.job?ticket.job->bufferBytes:0;}
size_t Backend::retainedBufferCount(const Ticket& ticket){return ticket.job?5:0;}
Status Backend::finish(Store& store,Ticket& ticket,std::vector<uint8_t>* readback){
    if(!ticket.job)return Status::GpuError;if(ticket.job->applied)return Status::AlreadyApplied;
    const auto status=wait(ticket);if(status!=Status::Ok)return status;ticket.job->applied=true;
    Snapshot current;if(store.snapshot(ticket.job->destination,current)!=surface_lab::Status::Ok||current.lost)return Status::SurfaceRejected;
    if(!same(current,ticket.job->original))return Status::ChangedDestination;
    const auto* bytes=static_cast<const uint8_t*>(ticket.job->readback.contents);
    std::vector<uint8_t> result(bytes,bytes+ticket.job->original.bytes.size());
    if(store.replaceBytes(ticket.job->destination,result)!=surface_lab::Status::Ok)return Status::SurfaceRejected;
    if(readback)*readback=std::move(result);return Status::Ok;
}
} // namespace metal_lab
