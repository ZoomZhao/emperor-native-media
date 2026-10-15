#pragma once
// Reviewer-only seam. Every AudioQueue call in the build is redirected before
// production headers are included. No output device or UI is opened.
#include <AudioToolbox/AudioToolbox.h>
decltype(AudioQueueNewOutput) reviewAudioQueueNewOutput;
decltype(AudioQueueSetParameter) reviewAudioQueueSetParameter;
decltype(AudioQueueCreateTimeline) reviewAudioQueueCreateTimeline;
decltype(AudioQueueAllocateBuffer) reviewAudioQueueAllocateBuffer;
decltype(AudioQueueStart) reviewAudioQueueStart;
decltype(AudioQueuePause) reviewAudioQueuePause;
decltype(AudioQueueStop) reviewAudioQueueStop;
decltype(AudioQueueGetProperty) reviewAudioQueueGetProperty;
decltype(AudioQueueGetCurrentTime) reviewAudioQueueGetCurrentTime;
decltype(AudioQueueDispose) reviewAudioQueueDispose;
decltype(AudioQueueEnqueueBuffer) reviewAudioQueueEnqueueBuffer;
#define AudioQueueNewOutput reviewAudioQueueNewOutput
#define AudioQueueSetParameter reviewAudioQueueSetParameter
#define AudioQueueCreateTimeline reviewAudioQueueCreateTimeline
#define AudioQueueAllocateBuffer reviewAudioQueueAllocateBuffer
#define AudioQueueStart reviewAudioQueueStart
#define AudioQueuePause reviewAudioQueuePause
#define AudioQueueStop reviewAudioQueueStop
#define AudioQueueGetProperty reviewAudioQueueGetProperty
#define AudioQueueGetCurrentTime reviewAudioQueueGetCurrentTime
#define AudioQueueDispose reviewAudioQueueDispose
#define AudioQueueEnqueueBuffer reviewAudioQueueEnqueueBuffer
bool reviewConsumeOne();
unsigned reviewLiveQueues();

unsigned reviewCreatedQueues();
unsigned reviewDisposedQueues();
unsigned reviewMaximumLiveQueues();
bool reviewLastPCMValid();
uint64_t reviewLastPCMFrames();
void reviewResetCounters();
