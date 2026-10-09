#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>
#include "device_monitor.hpp"
#include <array>
#include <cmath>
#include <utility>

namespace media_demo {
namespace {
AudioObjectPropertyAddress address(AudioObjectPropertySelector selector){return {selector,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};}
struct Registration {AudioObjectID object=0;AudioObjectPropertyAddress property{};bool active=false;};
struct MonitorState:std::enable_shared_from_this<MonitorState> {
    dispatch_queue_t queue=dispatch_queue_create("local.media-demo.device",DISPATCH_QUEUE_SERIAL);
    AudioObjectPropertyListenerBlock block=nil;std::array<Registration,3> registrations{};
    std::function<void(DeviceEvent)> handler;DeviceMonitorStats report;uint64_t nextSequence=1;
    explicit MonitorState(std::function<void(DeviceEvent)> h):handler(std::move(h)){}
    void add(size_t slot,AudioObjectID object,AudioObjectPropertySelector selector){auto& r=registrations[slot];r={object,address(selector),false};
        const auto status=AudioObjectAddPropertyListenerBlock(object,&r.property,queue,block);
        if(status!=noErr){report.listenerStatus=status;return;}r.active=true;++report.activeListeners;}
    void remove(size_t slot){auto& r=registrations[slot];if(!r.active)return;
        const auto status=AudioObjectRemovePropertyListenerBlock(r.object,&r.property,queue,block);
        if(status!=noErr){report.removeStatus=status;return;}r.active=false;--report.activeListeners;}
    template<class T>bool read(AudioObjectID object,AudioObjectPropertySelector selector,T& value){auto p=address(selector);UInt32 bytes=sizeof(T);
        const auto status=AudioObjectGetPropertyData(object,&p,0,nullptr,&bytes,&value);
        if(status!=noErr||bytes!=sizeof(T)){report.readStatus=status==noErr?-1:status;return false;}return true;}
    void deliver(DeviceEvent event){if(report.closed){++report.lateEventsSuppressed;return;}
        event.sequence=nextSequence++;if(event.origin==DeviceOrigin::Native)++report.nativeEvents;else ++report.injectedEvents;
        if(handler)handler(event);}
    void refresh(){if(report.closed){++report.lateEventsSuppressed;return;}report.readStatus=0;DeviceEvent event;UInt32 alive=0;
        if(read(kAudioObjectSystemObject,kAudioHardwarePropertyDefaultOutputDevice,event.device)&&event.device){
            const bool ok=read(event.device,kAudioDevicePropertyDeviceIsAlive,alive)&&read(event.device,kAudioDevicePropertyNominalSampleRate,event.nominalRate);
            event.available=ok&&alive==1&&std::isfinite(event.nominalRate)&&event.nominalRate>0;}
        if(event.device!=report.current.device){remove(1);remove(2);
            if(event.device){add(1,event.device,kAudioDevicePropertyDeviceIsAlive);add(2,event.device,kAudioDevicePropertyNominalSampleRate);}}
        report.current=event;deliver(event);}
    void start(){std::weak_ptr<MonitorState> weak=shared_from_this();
        block=^(UInt32 count,const AudioObjectPropertyAddress* properties){(void)count;(void)properties;if(auto state=weak.lock())state->refresh();};
        add(0,kAudioObjectSystemObject,kAudioHardwarePropertyDefaultOutputDevice);refresh();}
    void finish(){if(report.closed)return;report.closed=true;for(size_t i=0;i<registrations.size();++i)remove(i);handler={};}
};
}
struct DeviceMonitor::Impl {std::shared_ptr<MonitorState> state;explicit Impl(std::function<void(DeviceEvent)> handler):state(std::make_shared<MonitorState>(std::move(handler))){}
};
DeviceMonitor::DeviceMonitor(std::function<void(DeviceEvent)> handler):impl_(new Impl(std::move(handler))){const auto state=impl_->state;dispatch_sync(state->queue,^{state->start();});}
DeviceMonitor::~DeviceMonitor(){close();}
void DeviceMonitor::inject(DeviceEvent event){const auto state=impl_->state;event.origin=DeviceOrigin::Injected;dispatch_sync(state->queue,^{state->deliver(event);});}
void DeviceMonitor::close(){if(!impl_)return;const auto state=impl_->state;dispatch_sync(state->queue,^{state->finish();});}
DeviceMonitorStats DeviceMonitor::stats() const{const auto state=impl_->state;__block DeviceMonitorStats result;dispatch_sync(state->queue,^{result=state->report;});return result;}
} // namespace media_demo
