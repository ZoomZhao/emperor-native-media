#include "fake_audio_hooks.hpp"
#include <vector>
#include <deque>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <stdexcept>
namespace {
struct Queue { AudioQueueOutputCallback callback; void* context; UInt32 bytesPerFrame;
 std::vector<AudioQueueBufferRef> buffers; std::deque<AudioQueueBufferRef> pending;
 bool running=false,paused=true,draining=false; double clock=0; uint64_t submitted=0;bool pattern=true; };
std::mutex lock; Queue* current=nullptr;unsigned live=0,created=0,disposed=0,peak=0;uint64_t lastFrames=0;bool lastValid=true;
Queue* get(AudioQueueRef q){return reinterpret_cast<Queue*>(q);}
AudioQueueRef ref(Queue* q){return reinterpret_cast<AudioQueueRef>(q);}
}
OSStatus reviewAudioQueueNewOutput(const AudioStreamBasicDescription* f,AudioQueueOutputCallback c,void* ctx,CFRunLoopRef,CFStringRef,UInt32,AudioQueueRef* out){
 std::lock_guard<std::mutex> guard(lock);auto q=new Queue{c,ctx,f->mBytesPerFrame,{},{},false,true,false,0};current=q;++created;peak=std::max(peak,++live);*out=ref(q);return noErr;}
OSStatus reviewAudioQueueSetParameter(AudioQueueRef,AudioQueueParameterID,AudioQueueParameterValue){return noErr;}
OSStatus reviewAudioQueueCreateTimeline(AudioQueueRef q,AudioQueueTimelineRef* out){*out=reinterpret_cast<AudioQueueTimelineRef>(q);return noErr;}
OSStatus reviewAudioQueueAllocateBuffer(AudioQueueRef q,UInt32 capacity,AudioQueueBufferRef* out){
 std::lock_guard<std::mutex> guard(lock);*out=new AudioQueueBuffer(std::calloc(1,capacity),capacity,new AudioStreamPacketDescription{},0);get(q)->buffers.push_back(*out);return noErr;}
OSStatus reviewAudioQueueEnqueueBuffer(AudioQueueRef q,AudioQueueBufferRef b,UInt32,const AudioStreamPacketDescription*){
 std::lock_guard<std::mutex> guard(lock);auto item=get(q);auto samples=static_cast<const int16_t*>(b->mAudioData);
 for(uint64_t i=0;i<b->mAudioDataByteSize/4;++i){const auto expected=int16_t((item->submitted+i)%32767);item->pattern=item->pattern&&samples[2*i]==expected&&samples[2*i+1]==-expected;}
 item->submitted+=b->mAudioDataByteSize/4;item->pending.push_back(b);return noErr;}
OSStatus reviewAudioQueueStart(AudioQueueRef q,const AudioTimeStamp*){
 std::lock_guard<std::mutex> guard(lock);get(q)->running=true;get(q)->paused=false;return noErr;}
OSStatus reviewAudioQueuePause(AudioQueueRef q){std::lock_guard<std::mutex> guard(lock);get(q)->paused=true;return noErr;}
OSStatus reviewAudioQueueStop(AudioQueueRef handle,Boolean immediate){
 std::lock_guard<std::mutex> guard(lock);auto q=get(handle);q->draining=true;
 if(immediate){while(!q->pending.empty()){auto b=q->pending.front();q->pending.pop_front();q->callback(q->context,handle,b);}q->running=false;}
 else if(q->pending.empty())q->running=false;return noErr;}
OSStatus reviewAudioQueueGetProperty(AudioQueueRef q,AudioQueuePropertyID id,void* out,UInt32* size){
 std::lock_guard<std::mutex> guard(lock);if(id!=kAudioQueueProperty_IsRunning)return -1;
 *static_cast<UInt32*>(out)=get(q)->running;*size=sizeof(UInt32);return noErr;}
OSStatus reviewAudioQueueGetCurrentTime(AudioQueueRef q,AudioQueueTimelineRef,AudioTimeStamp* out,Boolean* discontinuity){
 std::lock_guard<std::mutex> guard(lock);std::memset(out,0,sizeof(*out));out->mFlags=kAudioTimeStampSampleTimeValid;out->mSampleTime=get(q)->clock;*discontinuity=false;return noErr;}
OSStatus reviewAudioQueueDispose(AudioQueueRef handle,Boolean){
 std::lock_guard<std::mutex> guard(lock);auto q=get(handle);for(auto b:q->buffers){std::free(b->mAudioData);delete b->mPacketDescriptions;delete b;}if(current==q)current=nullptr;lastFrames=q->submitted;lastValid=q->pattern;++disposed;--live;delete q;return noErr;}
bool reviewConsumeOne(){std::lock_guard<std::mutex> guard(lock);auto q=current;if(!q||q->paused||q->pending.empty())return false;
 auto b=q->pending.front();q->pending.pop_front();q->clock+=b->mAudioDataByteSize/q->bytesPerFrame;q->callback(q->context,ref(q),b);
 if(q->draining&&q->pending.empty())q->running=false;return true;}
unsigned reviewLiveQueues(){std::lock_guard<std::mutex> guard(lock);return live;}

unsigned reviewCreatedQueues(){std::lock_guard<std::mutex> guard(lock);return created;}
unsigned reviewDisposedQueues(){std::lock_guard<std::mutex> guard(lock);return disposed;}
unsigned reviewMaximumLiveQueues(){std::lock_guard<std::mutex> guard(lock);return peak;}
bool reviewLastPCMValid(){std::lock_guard<std::mutex> guard(lock);return lastValid;}
uint64_t reviewLastPCMFrames(){std::lock_guard<std::mutex> guard(lock);return lastFrames;}
void reviewResetCounters(){std::lock_guard<std::mutex> guard(lock);if(live)throw std::runtime_error("fake queue still live at reset");created=disposed=peak=0;lastFrames=0;lastValid=true;}
