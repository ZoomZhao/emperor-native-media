#pragma once
#include <cstdint>
#include <functional>
#include <memory>

namespace media_demo {
enum class DeviceOrigin {Native,Injected};
struct DeviceEvent {uint32_t device=0;double nominalRate=0;bool available=false;
    uint64_t sequence=0;DeviceOrigin origin=DeviceOrigin::Native;};
struct DeviceMonitorStats {size_t activeListeners=0,nativeEvents=0,injectedEvents=0,lateEventsSuppressed=0;
    int32_t readStatus=0,listenerStatus=0,removeStatus=0;bool closed=false;DeviceEvent current;};
// Output-device ID/alive/rate property reads and notifications only. No hardware IO,
// microphone, property writes, device selection or permission request.
// close fences the private serial callback queue and unregisters its listeners.
class DeviceMonitor {
public:
    explicit DeviceMonitor(std::function<void(DeviceEvent)> handler);~DeviceMonitor();
    DeviceMonitor(const DeviceMonitor&)=delete;DeviceMonitor& operator=(const DeviceMonitor&)=delete;
    void inject(DeviceEvent event); // Same delivery path; never mutates CoreAudio.
    void close();DeviceMonitorStats stats() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace media_demo
