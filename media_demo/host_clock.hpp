#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>

namespace media_demo {
// Single owner thread. Real steady_clock only; no wall-clock or injected time.
class HostClock {
public:
    using Host=std::chrono::steady_clock;
    static_assert(Host::is_steady,"playback needs a monotonic clock");
    struct Reading {uint64_t samples;int64_t observedNS,anchorNS;uint64_t anchorSamples;bool running;};
    static int64_t nowNS(){return std::chrono::duration_cast<std::chrono::nanoseconds>(Host::now().time_since_epoch()).count();}
    void reset(uint64_t limit=0,uint64_t samples=0){limit_=limit;stored_=std::min(samples,limit);anchor_=nowNS();running_=false;}
    Reading resume(){anchor_=nowNS();running_=true;return {stored_,anchor_,anchor_,stored_,true};}
    void pause(){const auto value=read();stored_=value.samples;anchor_=value.observedNS;running_=false;}
    Reading read() const{const auto now=nowNS();uint64_t value=stored_;
        if(running_){const auto elapsed=uint64_t(std::max(int64_t(0),now-anchor_));
            const uint64_t remaining=limit_-stored_,seconds=elapsed/1000000000;
            uint64_t advance=remaining;
            if(seconds<=remaining/48000){advance=seconds*48000;
                advance+=std::min(remaining-advance,(elapsed%1000000000)*48000/1000000000);}
            value=stored_+advance;}
        return {value,now,anchor_,stored_,running_};}
private:uint64_t stored_=0,limit_=0;int64_t anchor_=0;bool running_=false;
};
} // namespace media_demo
