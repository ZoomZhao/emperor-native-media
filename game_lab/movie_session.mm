#import <Foundation/Foundation.h>
#include "movie_session.hpp"
#include "app_media.hpp"
#include <mutex>
#include <algorithm>
#include <thread>
#include <chrono>
#include <stdexcept>

namespace game_lab {
const char* movieErrorKindName(MovieErrorKind k) {
    switch(k) {
    case MovieErrorKind::None:return "";
    case MovieErrorKind::PathSecurity:return "路径被拒绝";
    case MovieErrorKind::ResourceUnavailable:return "文件缺失或不可访问";
    case MovieErrorKind::MetadataTimeout:return "读取媒体信息超时";
    case MovieErrorKind::DecodeFailure:return "媒体读取失败";
    case MovieErrorKind::PresenterFailure:return "画面转换失败";
    case MovieErrorKind::CloseFailure:return "关闭失败";
    } return "媒体错误";
}
std::string formatMovieStatus(const MovieDelivery& d) {
    if (!d.accepted) return "请求已被更新";
    if (d.errorKind != MovieErrorKind::None) return std::string(movieErrorKindName(d.errorKind))+"："+d.errorMessage;
    if (d.presentation == PresentationResult::ConversionFailure) return "画面转换失败，请重试";
    if (d.snapshot.state == media_demo::State::Failed) return d.snapshot.message;
    return d.poster ? "首帧已加载" : std::string(media_demo::stateName(d.snapshot.state))+" · 静默预览";
}
static MovieErrorKind decodeKind(const std::string& detail) {
    return detail.find("metadata load timeout")!=std::string::npos ? MovieErrorKind::MetadataTimeout : MovieErrorKind::DecodeFailure;
}
struct MovieSessionState : std::enable_shared_from_this<MovieSessionState> {
    ResourceRoot root;
    dispatch_queue_t worker=dispatch_queue_create("local.emperor.movie-worker",DISPATCH_QUEUE_SERIAL);
    std::unique_ptr<media_demo::Controller> controller; // worker only
    std::shared_ptr<MovieSessionStats> stats=std::make_shared<MovieSessionStats>();
    std::atomic<uint64_t> revision{1};
    uint32_t delayMs=0; bool holdMetadata=false, failClose=false; // owner only
    std::mutex lock;
    std::function<void(MovieSessionState&)> pendingJob;
    std::function<void()> mainDelivery;
    bool pumping=false, delivering=false;
    explicit MovieSessionState(ResourceRoot value):root(std::move(value)){}
    void enqueue(std::function<void(MovieSessionState&)> job) {
        stats->workerEnqueues.fetch_add(1);
        bool start=false;
        { std::lock_guard<std::mutex> guard(lock);
          pendingJob=std::move(job); stats->pending=1;stats->maxPending=1;
          if(!pumping){pumping=true;stats->workerFinished=false;start=true;}}
        if(start){auto self=shared_from_this();dispatch_async(worker,^{self->pump();});}
    }
    void pump() {
        for(;;){std::function<void(MovieSessionState&)> job;
            {std::lock_guard<std::mutex> guard(lock);
             if(!pendingJob){pumping=false;stats->workerFinished=true;return;}
             job=std::move(pendingJob);pendingJob={};stats->pending=0;const auto active=stats->active.fetch_add(1)+1;stats->maxActive=std::max(stats->maxActive.load(),active);}
            @autoreleasepool {job(*this);}
            stats->active.fetch_sub(1);
        }
    }
    void publish(std::function<void()> delivery) {
        bool start=false;
        {std::lock_guard<std::mutex> guard(lock);mainDelivery=std::move(delivery);stats->delivery=1;stats->maxDelivery=1;
         if(!delivering){delivering=true;start=true;}}
        if(start){stats->mainSchedules.fetch_add(1);auto self=shared_from_this();dispatch_async(dispatch_get_main_queue(),^{
            std::function<void()> item;
            {std::lock_guard<std::mutex> guard(self->lock);item=std::move(self->mainDelivery);self->mainDelivery={};
             self->delivering=false;self->stats->delivery=0;}
            if(item)item();
        });}
    }
    void retire(bool injectFailure=false) {
        if(injectFailure) throw std::runtime_error("self-authored close fault");
        if(controller){controller->close();
            if(!controller->snapshot().resourcesZero())throw std::runtime_error("close resource invariant");
            controller.reset();}
        stats->hasController=false;stats->resourcesZero=true;
    }
    void delayed(uint32_t ms) {
        stats->delayEntered=true;
        if(ms)std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    MovieDelivery load(uint64_t rev,const std::string& relative,bool poster,
                       const std::shared_ptr<stream_lab::OpenControl>& control,uint32_t delay,bool onlyOpen=false) {
        MovieDelivery d;d.revision=rev;d.poster=poster;d.snapshot.clearImage=true;
        delayed(delay);
        if(revision!=rev)return d;
        try {
            try {retire();} catch(const std::exception& e){d.errorKind=MovieErrorKind::CloseFailure;throw std::runtime_error(e.what());}
            const auto resolved=root.resolve(relative);
            if(resolved.empty()) {
                const std::filesystem::path path(relative);bool unsafe=relative.empty()||relative.find('\0')!=std::string::npos||path.has_root_path();
                for(const auto& part:path)unsafe|=(part==".."||part==".");
                d.errorKind=unsafe?MovieErrorKind::PathSecurity:MovieErrorKind::ResourceUnavailable;
                throw std::runtime_error("ResourceRoot 拒绝路径或文件不存在："+relative);
            }
            d.resolvedPath=resolved;
            if(poster){stats->resourcesZero=false;const auto image=readMoviePosterBytes(resolved,control);
                stats->resourcesZero=true;
                d.width=image.width;d.height=image.height;d.pixelsBGRA=image.pixelsBGRA;
                d.snapshot.state=media_demo::State::Ready;
            }else{
                controller=std::make_unique<media_demo::Controller>(resolved);stats->hasController=true;
                d.snapshot=onlyOpen?controller->snapshot():controller->perform(media_demo::Action::Play,0,control);
                stats->resourcesZero=d.snapshot.resourcesZero();
                if(d.snapshot.state==media_demo::State::Failed){d.errorMessage=d.snapshot.message;d.errorKind=decodeKind(d.errorMessage);}
            }
        }catch(const std::exception& e){d.snapshot.state=media_demo::State::Failed;d.snapshot.clearImage=true;
            // The stack-local poster Reader has completed its RAII cleanup.
            if(poster && d.errorKind!=MovieErrorKind::CloseFailure)stats->resourcesZero=true;
            d.errorMessage=e.what();d.snapshot.message=d.errorMessage;
            if(d.errorKind==MovieErrorKind::None)d.errorKind=decodeKind(d.errorMessage);}
        return d;
    }
    MovieDelivery action(uint64_t rev,media_demo::Action action,uint64_t frame,const std::shared_ptr<stream_lab::OpenControl>& control) {
        MovieDelivery d;d.revision=rev;
        if(revision!=rev)return d;
        try {if(!controller)throw std::runtime_error("未打开有效媒体文件");
             d.snapshot=controller->perform(action,frame,control);stats->resourcesZero=d.snapshot.resourcesZero();
             if(d.snapshot.state==media_demo::State::Failed){d.errorMessage=d.snapshot.message;d.errorKind=decodeKind(d.errorMessage);}
        }catch(const std::exception& e){d.snapshot.state=media_demo::State::Failed;d.snapshot.clearImage=true;
            d.errorMessage=e.what();d.snapshot.message=d.errorMessage;d.errorKind=decodeKind(d.errorMessage);}
        return d;
    }
};
struct MovieTermination {
    TerminationResult result;
    uint64_t generation = 0;
    std::chrono::steady_clock::time_point deadline;
    dispatch_source_t timer = nullptr;
    std::function<void(const TerminationResult&)> completion;
    bool closeZero = false;
    std::string closeError;
};
static bool workerIdle(const MovieSessionStats& s) {
    return s.workerFinished && s.active == 0 && s.pending == 0 && s.delivery == 0;
}
std::string formatTerminationResult(const TerminationResult& r) {
    switch(r.status) {
    case TerminationStatus::Completed: return "媒体关闭完成";
    case TerminationStatus::Failed: return "退出失败：" + r.reason;
    case TerminationStatus::TimedOut: return "退出等待超时：" + r.reason;
    case TerminationStatus::Pending: return "正在关闭媒体…";
    case TerminationStatus::NotRequested: return "尚未请求退出";
    } return "退出状态未知";
}
MovieSession::MovieSession(ResourceRoot root):root_(root),state_(std::make_shared<MovieSessionState>(std::move(root))){}
void MovieSession::invalidate(){if(loadControl_)loadControl_->cancelled=true;++revision_;state_->revision=revision_;}
MovieSession::~MovieSession(){*deliveryAlive_=false;cancelTerminationTimer();invalidate();auto s=state_;
    if(workerIdle(*s->stats) && s->stats->resourcesZero && !s->stats->hasController)return;
    s->stats->resourcesZero=false;
    s->enqueue([](MovieSessionState& worker){try{worker.retire();}catch(...){
        // Final fallback destruction remains on the worker. Do not claim success.
        worker.controller.reset();worker.stats->hasController=false;worker.stats->resourcesZero=false;}});
}
std::shared_ptr<const MovieSessionStats> MovieSession::stats()const{return state_->stats;}
void MovieSession::setTestDelayMs(uint32_t ms){state_->delayMs=std::min(ms,500u);state_->stats->delayEntered=false;}
void MovieSession::holdMetadataForTest(bool hold){state_->holdMetadata=hold;}
void MovieSession::failNextCloseForTest(){state_->failClose=true;}
bool MovieSession::hasActiveController()const{return state_->stats->hasController;}
bool MovieSession::resourcesZero()const{return state_->stats->resourcesZero;}
void MovieSession::accept(MovieDelivery& d,ImagePresenter* presenter){
    if(d.revision!=revision_){++staleDeliveries_;return;}
    if(d.poster){d.accepted=true;if(d.snapshot.clearImage&&presenter)presenter->clear();
        if(!d.pixelsBGRA.empty()&&presenter){d.presentation=presentImage(*presenter,d.width,d.height,d.pixelsBGRA);d.presented=d.presentation==PresentationResult::Presented;}}
    else{processMovieDelivery(revision_,d.revision,d.snapshot,presenter,d);}
    if(d.presentation==PresentationResult::ConversionFailure){d.errorKind=MovieErrorKind::PresenterFailure;d.errorMessage="当前图片无法转换";}
    lastSnapshot_=d.snapshot;
}
void MovieSession::loadAsync(const std::string& relative,bool poster,std::function<void(const MovieDelivery&)> callback,ImagePresenter* presenter){
    if(terminationLocksCommands())return;
    invalidate();relativePath_=relative;resolvedPath_.clear();lastSnapshot_={};lastSnapshot_.clearImage=true;
    if(presenter)presenter->clear();
    const auto rev=revision_;auto control=std::make_shared<stream_lab::OpenControl>();control->holdMetadataCompletionForTest=state_->holdMetadata;loadControl_=control;
    const auto delay=state_->delayMs;auto alive=deliveryAlive_;state_->stats->resourcesZero=false;
    state_->enqueue([this,alive,rev,relative,poster,control,delay,callback,presenter](MovieSessionState& worker){
        auto d=worker.load(rev,relative,poster,control,delay);
        worker.publish([this,alive,rev,d=std::move(d),relative,callback,presenter]()mutable{
            if(!*alive)return;if(revision_!=rev){++staleDeliveries_;return;}
            // Actual resolved path was produced by the worker.
            resolvedPath_=d.snapshot.state==media_demo::State::Failed?"":d.resolvedPath;
            accept(d,presenter);if(callback)callback(d);
        });
    });
}
void MovieSession::openAndPlay(const std::string& p,std::function<void(const MovieDelivery&)> c,ImagePresenter* view){loadAsync(p,false,std::move(c),view);}
void MovieSession::requestPoster(const std::string& p,std::function<void(const MovieDelivery&)> c,ImagePresenter* view){loadAsync(p,true,std::move(c),view);}
void MovieSession::request(media_demo::Action action,uint64_t frame,std::function<void(const MovieDelivery&)> callback,ImagePresenter* presenter){
    if(terminationLocksCommands())return;
    if(action!=media_demo::Action::Tick)invalidate();
    const auto rev=revision_;auto control=std::make_shared<stream_lab::OpenControl>();loadControl_=control;
    auto alive=deliveryAlive_;
    state_->enqueue([this,alive,rev,action,frame,control,callback,presenter](MovieSessionState& worker){
        auto d=worker.action(rev,action,frame,control);
        worker.publish([this,alive,rev,d=std::move(d),callback,presenter]()mutable{
            if(!*alive)return;if(revision_!=rev){++staleDeliveries_;return;}
            accept(d,presenter);if(callback)callback(d);
        });
    });
}
void MovieSession::closeAsync(std::function<void(bool,const std::string&)> callback){
    if(terminationLocksCommands())return;
    closeAsyncImpl(std::move(callback));
}
void MovieSession::closeAsyncImpl(std::function<void(bool,const std::string&)> callback){
    invalidate();const auto rev=revision_;auto alive=deliveryAlive_;relativePath_.clear();resolvedPath_.clear();lastSnapshot_={};lastSnapshot_.clearImage=true;
    state_->stats->resourcesZero=false;const bool fault=state_->failClose;state_->failClose=false;
    state_->enqueue([this,alive,rev,fault,callback](MovieSessionState& worker){
        std::string error;bool zero=false;try{worker.retire(fault);zero=worker.stats->resourcesZero;}catch(const std::exception& e){error=e.what();worker.stats->resourcesZero=false;}
        worker.publish([this,alive,rev,zero,error,callback]{if(!*alive)return;if(revision_!=rev){++staleDeliveries_;return;}if(callback)callback(zero,error);});
    });
}
bool MovieSession::terminationLocksCommands() const {
    return termination_ && (termination_->result.status == TerminationStatus::Pending || termination_->result.status == TerminationStatus::Completed);
}
TerminationResult MovieSession::terminationResult() const {
    return termination_ ? termination_->result : TerminationResult{};
}
void MovieSession::cancelTerminationTimer() {
    if(!termination_ || !termination_->timer)return;
    dispatch_source_cancel(termination_->timer);
    termination_->timer=nullptr;
    state_->stats->terminationTimers.fetch_sub(1);
}
void MovieSession::finishTermination(TerminationStatus status,const std::string& reason) {
    auto& t=*termination_;
    t.result.status=status;t.result.reason=reason;
    t.result.resourcesZero=state_->stats->resourcesZero;
    t.result.workerIdle=workerIdle(*state_->stats);
    cancelTerminationTimer();
    auto callback=std::move(t.completion);
    const auto result=t.result;
    state_->stats->terminationReplies.fetch_add(1);
    // The App may destroy this session in its completion. No member access after.
    if(callback)callback(result);
}
void MovieSession::checkTermination(uint64_t generation) {
    if(!termination_ || termination_->generation!=generation || termination_->result.status!=TerminationStatus::Pending)return;
    auto& t=*termination_;
    if(std::chrono::steady_clock::now()>=t.deadline){
        finishTermination(TerminationStatus::TimedOut,"未在截止时间内确认关闭；后台清理可能仍在进行");return;
    }
    if(!t.result.closeObserved)return;
    if(!t.closeZero || !t.closeError.empty()){
        finishTermination(TerminationStatus::Failed,t.closeError.empty()?"关闭未确认资源归零":t.closeError);return;
    }
    if(workerIdle(*state_->stats) && state_->stats->resourcesZero && !state_->stats->hasController){
        finishTermination(TerminationStatus::Completed,"");return;
    }
}
TerminationStatus MovieSession::requestTermination(uint32_t timeoutMs,std::function<void(const TerminationResult&)> callback) {
    if(terminationLocksCommands())return termination_->result.status;
    if(!termination_)termination_=std::make_unique<MovieTermination>();
    auto& t=*termination_;++t.generation;t.result={};t.result.status=TerminationStatus::Pending;
    t.closeZero=false;t.closeError.clear();t.completion=std::move(callback);
    t.deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(std::clamp(timeoutMs,1u,60000u));
    const auto generation=t.generation;auto alive=deliveryAlive_;
    state_->stats->terminationAttempts.fetch_add(1);
    t.timer=dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_get_main_queue());
    dispatch_source_set_timer(t.timer,dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_MSEC),5*NSEC_PER_MSEC,NSEC_PER_MSEC);
    dispatch_source_set_event_handler(t.timer,^{if(!*alive)return;checkTermination(generation);});
    state_->stats->terminationTimers.fetch_add(1);state_->stats->maxTerminationTimers=std::max(state_->stats->maxTerminationTimers.load(),state_->stats->terminationTimers.load());
    dispatch_resume(t.timer);
    closeAsyncImpl([this,alive,generation](bool zero,const std::string& error){
        if(!*alive || !termination_ || termination_->generation!=generation || termination_->result.status!=TerminationStatus::Pending)return;
        termination_->result.closeObserved=true;termination_->closeZero=zero;termination_->closeError=error;
        checkTermination(generation);
    });
    return TerminationStatus::Pending;
}
// Legacy synchronization is confined to explicit headless helpers. Do not mix
// these helpers with owner callbacks that wait recursively. App uses only async methods.
bool MovieSession::openMovie(const std::string& relative){
    if(terminationLocksCommands())return false;
    invalidate();const auto rev=revision_;auto control=std::make_shared<stream_lab::OpenControl>();loadControl_=control;
    __block MovieDelivery d;auto s=state_;dispatch_sync(s->worker,^{d=s->load(rev,relative,false,control,0,true);});
    relativePath_=relative;resolvedPath_=d.snapshot.state==media_demo::State::Failed?"":d.resolvedPath;accept(d,nullptr);return !resolvedPath_.empty();
}
MovieDelivery MovieSession::performSync(media_demo::Action action,uint64_t frame,ImagePresenter* presenter){
    if(terminationLocksCommands())throw std::runtime_error("termination locks media commands");
    if(action!=media_demo::Action::Tick)invalidate();const auto rev=revision_;auto control=std::make_shared<stream_lab::OpenControl>();loadControl_=control;
    __block MovieDelivery d;auto s=state_;dispatch_sync(s->worker,^{d=s->action(rev,action,frame,control);});accept(d,presenter);return d;
}
void MovieSession::close(){if(terminationLocksCommands())return;invalidate();auto s=state_;__block std::string error;dispatch_sync(s->worker,^{try{s->retire();}catch(const std::exception& e){error=e.what();s->stats->resourcesZero=false;}});
    relativePath_.clear();resolvedPath_.clear();lastSnapshot_={};lastSnapshot_.clearImage=true;
    if(!error.empty())throw std::runtime_error(error);
}
} // namespace game_lab
