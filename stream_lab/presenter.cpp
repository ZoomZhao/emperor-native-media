#include "presenter.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <mutex>
#include <limits>
#include <stdexcept>

namespace stream_lab {
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Event {uint64_t epoch,id;bool success;};
struct Mailbox {mutable std::mutex mutex;std::array<Event,2> events{};size_t count=0,peak=0;bool overflow=false;
    void post(Event event){std::lock_guard<std::mutex> lock(mutex);if(count==events.size()){overflow=true;return;}events[count++]=event;peak=std::max(peak,count);}
    size_t size() const{std::lock_guard<std::mutex> lock(mutex);return count;}
};
struct Pending {uint64_t epoch,id,pts;surface_lab::Handle target;metal_lab::Ticket ticket;};
}
struct EpochPresenter::Impl {
    uint64_t epoch;std::unique_ptr<surface_lab::Store> store;std::unique_ptr<metal_lab::Backend> backend;
    surface_lab::Handle source=0;std::array<surface_lab::Handle,2> targets{};
    std::shared_ptr<Mailbox> mailbox=std::make_shared<Mailbox>();std::deque<Pending> pending;
    RenderStats stats;std::string device;
    video_lab::StreamMetadata meta;surface_lab::Desc destination;surface_lab::Rect rect;
    Impl(uint64_t value,video_lab::StreamMetadata m,surface_lab::Desc d,surface_lab::Rect r):epoch(value),meta(m),destination(d),rect(r){}
    void make(){
        backend=std::make_unique<metal_lab::Backend>();require(backend->available(),backend->reason().c_str());device=backend->deviceName();
        store=std::make_unique<surface_lab::Store>(3,meta.stride*meta.height+2*destination.stride*size_t(destination.height));
        require(store->create({int32_t(meta.width),int32_t(meta.height),meta.stride,surface_lab::Format::BGRA8},source)==surface_lab::Status::Ok,"create source");
        for(auto& target:targets){require(store->create(destination,target)==surface_lab::Status::Ok,"create target");
            require(store->replaceBytes(target,std::vector<uint8_t>(destination.stride*size_t(destination.height),0xc7))==surface_lab::Status::Ok,"initialize target");}
        ++stats.rebuilds;
    }
    void releaseSurfaces(){if(store){require(store->destroy(source)==surface_lab::Status::Ok,"destroy source");
        for(const auto target:targets)require(store->destroy(target)==surface_lab::Status::Ok,"destroy target");
        require(store->liveCount()==0&&store->activeBytes()==0&&store->savedBytes()==0,"surface release");}
        source=0;targets={};store.reset();backend.reset();}
};
EpochPresenter::EpochPresenter(uint64_t epoch,video_lab::StreamMetadata meta,surface_lab::Desc destination,surface_lab::Rect rect):impl_(new Impl(epoch,meta,destination,rect)){
    require(epoch!=0,"zero epoch");video_lab::StreamingPlayer validator;require(validator.open(meta)==video_lab::Status::Ok,"source metadata bounds");
    require(destination.width>0&&destination.height>0&&destination.width<=4096&&destination.height<=4096&&destination.format==surface_lab::Format::BGRA8&&
        destination.stride>=size_t(destination.width)*4&&destination.stride<=size_t(destination.width)*4+64&&destination.stride<=1048576/size_t(destination.height),"destination bounds");
    require(rect.width>0&&rect.width<=4096&&rect.height>0&&rect.height<=4096&&rect.x>=-4096&&rect.x<=4096&&rect.y>=-4096&&rect.y<=4096,"destination rectangle bounds");impl_->make();}
EpochPresenter::~EpochPresenter(){try{close();}catch(...){/* explicit close reports failures to the caller */}}
bool EpochPresenter::submit(uint64_t epoch,uint64_t id,const video_lab::Frame& frame){auto& s=*impl_;
    if(epoch!=s.epoch)return false;if(s.pending.size()==2)return false;
    if(std::any_of(s.pending.begin(),s.pending.end(),[&](const Pending& p){return p.epoch==epoch&&p.id==id;}))return false;
    require(bool(s.store)&&bool(s.backend),"presenter closed");
    require(frame.bgra.size()==s.meta.stride*s.meta.height&&frame.pts<s.meta.frames,"frame shape");
    size_t slot=0;for(const auto& p:s.pending)if(p.target==s.targets[0])slot=1;
    const auto target=s.targets[slot];require(s.store->replaceBytes(s.source,frame.bgra)==surface_lab::Status::Ok,"upload frame");
    require(s.store->replaceBytes(target,std::vector<uint8_t>(s.destination.stride*size_t(s.destination.height),0xc7))==surface_lab::Status::Ok,"reset destination");
    Pending item{epoch,id,frame.pts,target,{}};const auto mailbox=s.mailbox;
    require(s.backend->submit(*s.store,target,s.source,{0,0,int32_t(s.meta.width),int32_t(s.meta.height)},s.rect,true,item.ticket,
        [mailbox,epoch,id](bool success){mailbox->post({epoch,id,success});})==metal_lab::Status::Ok,"GPU submit");
    s.pending.push_back(std::move(item));++s.stats.submissions;s.stats.maxTickets=std::max(s.stats.maxTickets,s.pending.size());
    size_t bytes=0,count=0;for(const auto& p:s.pending){bytes+=metal_lab::Backend::retainedBufferBytes(p.ticket);count+=metal_lab::Backend::retainedBufferCount(p.ticket);}
    s.stats.maxGPUBufferEnvelopeBytes=std::max(s.stats.maxGPUBufferEnvelopeBytes,bytes);
    s.stats.maxGPUBufferEnvelopeCount=std::max(s.stats.maxGPUBufferEnvelopeCount,count);return true;
}
void EpochPresenter::drain(const std::function<void(const RenderResult&)>& visitor){auto& s=*impl_;
    for(const auto& p:s.pending){require(metal_lab::Backend::wait(p.ticket)==metal_lab::Status::Ok,"GPU fence");
        require(metal_lab::Backend::callbackCount(p.ticket)==1,"one native completion");++s.stats.callbacks;}
    std::array<Event,2> events{};size_t count=0;
    {std::lock_guard<std::mutex> lock(s.mailbox->mutex);require(!s.mailbox->overflow,"callback mailbox overflow");
        events=s.mailbox->events;count=s.mailbox->count;s.stats.maxMailbox=std::max(s.stats.maxMailbox,s.mailbox->peak);s.mailbox->count=0;}
    require(count==s.pending.size(),"one published completion per ticket");
    for(size_t i=0;i<count;++i){const auto& event=events[i];require(event.success,"GPU callback failure");
        auto found=std::find_if(s.pending.begin(),s.pending.end(),[&](const Pending& p){return p.epoch==event.epoch&&p.id==event.id;});
        require(found!=s.pending.end(),"unknown completion");auto& p=*found;
        if(event.epoch!=s.epoch)++s.stats.stale;
        else{require(bool(s.store),"active surfaces missing");RenderResult result{p.epoch,p.id,p.pts,{}};
            require(metal_lab::Backend::finish(*s.store,p.ticket,&result.bytes)==metal_lab::Status::Ok,"apply GPU result");
            ++s.stats.applied;visitor(result);}
        std::weak_ptr<metal_lab::Job> lifetime=p.ticket.job;p.ticket={};require(lifetime.expired(),"GPU job retained after ticket release");
        ++s.stats.releasedJobs;s.pending.erase(found);
    }
    require(s.pending.empty()&&s.mailbox->size()==0,"drain resource zero");
}
void EpochPresenter::rebuild(uint64_t epoch){auto& s=*impl_;require(epoch>s.epoch,"epoch must increase");s.epoch=epoch;
    s.releaseSurfaces();s.make();
    // Deliberately build the new session before old callbacks are delivered to the owner.
    // Numeric handles may repeat in a new Store, so the epoch check must precede finish().
    const std::vector<uint8_t> untouched(s.destination.stride*size_t(s.destination.height),0xc7);std::vector<uint8_t> pixels;
    drain([](const RenderResult&){throw std::runtime_error("stale completion reached visitor");});
    for(const auto target:s.targets){require(s.store->readBytes(target,pixels)==surface_lab::Status::Ok,"new target read");
        require(pixels==untouched,"old completion modified new session");}
}
void EpochPresenter::close(){auto& s=*impl_;if(!s.store&&!s.backend&&s.pending.empty())return;
    require(s.epoch<std::numeric_limits<uint64_t>::max(),"epoch exhausted");++s.epoch;s.releaseSurfaces();drain([](const RenderResult&){throw std::runtime_error("closed completion reached visitor");});}
RenderUsage EpochPresenter::usage() const{const auto& s=*impl_;size_t bytes=0;
    for(const auto& p:s.pending)bytes+=metal_lab::Backend::retainedBufferBytes(p.ticket);
    return {s.pending.size(),s.mailbox->size(),s.store?s.store->liveCount():0,s.store?s.store->activeBytes():0,bytes};}
RenderStats EpochPresenter::stats() const{return impl_->stats;}std::string EpochPresenter::deviceName() const{return impl_->device;}
} // namespace stream_lab
