#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include "movie_io.hpp"
#include "bink_process.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace stream_lab {
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string detail(NSError* error){return error?std::string(error.description.UTF8String):"no NSError";}
NSURL* url(const std::string& path){return [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];}
template<class T>struct CF {
    T value=nullptr;~CF(){if(value)CFRelease(value);}operator T() const{return value;}
    CF()=default;CF(const CF&)=delete;CF& operator=(const CF&)=delete;
};
NSDictionary* audioSettings(uint32_t sampleRate=rate){return @{AVFormatIDKey:@(kAudioFormatLinearPCM),AVSampleRateKey:@(sampleRate),
    AVNumberOfChannelsKey:@(channels),AVLinearPCMBitDepthKey:@16,AVLinearPCMIsFloatKey:@NO,
    AVLinearPCMIsBigEndianKey:@NO,AVLinearPCMIsNonInterleaved:@NO};}
using Deadline=std::chrono::steady_clock::time_point;
void waitMetadata(AVAsset* asset,dispatch_semaphore_t done,const std::shared_ptr<OpenControl>& control,
                  Deadline deadline,IOStats& stats){
    for(;;){
        if(control->cancelled.load(std::memory_order_acquire)){
            [asset cancelLoading];++stats.metadataCancellations;throw std::runtime_error("metadata load cancelled");}
        if(std::chrono::steady_clock::now()>=deadline){
            [asset cancelLoading];++stats.metadataTimeouts;throw std::runtime_error("metadata load timeout (2000 ms)");}
        if(control->holdMetadataCompletionForTest){std::this_thread::sleep_for(std::chrono::milliseconds(10));continue;}
        if(!dispatch_semaphore_wait(done,dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_MSEC))){
            // Cancellation/deadline also win if they arrive with a completion.
            if(control->cancelled.load(std::memory_order_acquire))continue;
            if(std::chrono::steady_clock::now()>=deadline)continue;return;}
    }
}
void keys(id<AVAsynchronousKeyValueLoading> object,NSArray<NSString*>* names,AVAsset* asset,
          const std::shared_ptr<OpenControl>& control,Deadline deadline,IOStats& stats){
    dispatch_semaphore_t done=dispatch_semaphore_create(0);
    [object loadValuesAsynchronouslyForKeys:names completionHandler:^{dispatch_semaphore_signal(done);}];
    waitMetadata(asset,done,control,deadline,stats);
    for(NSString* key in names){NSError* failure=nil;
        if([object statusOfValueForKey:key error:&failure]!=AVKeyValueStatusLoaded)
            throw std::runtime_error("metadata key: "+std::string(key.UTF8String)+": "+detail(failure));}
}
NSArray<AVAssetTrack*>* tracks(AVAsset* asset,AVMediaType type,const std::shared_ptr<OpenControl>& control,Deadline deadline,IOStats& stats){
    __block NSArray<AVAssetTrack*>* result=nil;__block NSError* failure=nil;
    dispatch_semaphore_t done=dispatch_semaphore_create(0);
    [asset loadTracksWithMediaType:type completionHandler:^(NSArray<AVAssetTrack*>* loaded,NSError* error){
        result=loaded;failure=error;dispatch_semaphore_signal(done);}];
    waitMetadata(asset,done,control,deadline,stats);
    if(failure)throw std::runtime_error("track metadata: "+detail(failure));return result;
}
}
WriterStats writeSyntheticMovie(const std::string& path){@autoreleasepool{
    WriterStats stats;NSError* error=nil;
    AVAssetWriter* writer=[[AVAssetWriter alloc] initWithURL:url(path) fileType:AVFileTypeQuickTimeMovie error:&error];
    if(!writer)throw std::runtime_error("writer init: "+detail(error));
    NSDictionary* settings=@{AVVideoCodecKey:AVVideoCodecTypeH264,AVVideoWidthKey:@(width),AVVideoHeightKey:@(height),
        AVVideoCompressionPropertiesKey:@{AVVideoAverageBitRateKey:@1000000,AVVideoExpectedSourceFrameRateKey:@(fps),
            AVVideoMaxKeyFrameIntervalKey:@30,AVVideoAllowFrameReorderingKey:@NO},
        AVVideoColorPropertiesKey:@{AVVideoColorPrimariesKey:AVVideoColorPrimaries_ITU_R_709_2,
            AVVideoTransferFunctionKey:AVVideoTransferFunction_ITU_R_709_2,AVVideoYCbCrMatrixKey:AVVideoYCbCrMatrix_ITU_R_709_2}};
    require([writer canApplyOutputSettings:settings forMediaType:AVMediaTypeVideo],"H264 settings rejected");
    AVAssetWriterInput* video=[[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:settings];
    AVAssetWriterInput* audio=[[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:audioSettings()];
    require([writer canAddInput:video]&&[writer canAddInput:audio],"writer input rejected");
    [writer addInput:video];[writer addInput:audio];
    AVAssetWriterInputPixelBufferAdaptor* adaptor=[[AVAssetWriterInputPixelBufferAdaptor alloc] initWithAssetWriterInput:video
        sourcePixelBufferAttributes:@{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA),
            (id)kCVPixelBufferWidthKey:@(width),(id)kCVPixelBufferHeightKey:@(height)}];
    require([writer startWriting],"writer start failed");[writer startSessionAtSourceTime:kCMTimeZero];
    AudioStreamBasicDescription desc{};desc.mSampleRate=rate;desc.mFormatID=kAudioFormatLinearPCM;
    desc.mFormatFlags=kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;
    desc.mBytesPerPacket=4;desc.mFramesPerPacket=1;desc.mBytesPerFrame=4;desc.mChannelsPerFrame=channels;desc.mBitsPerChannel=16;
    CF<CMAudioFormatDescriptionRef> format;
    require(CMAudioFormatDescriptionCreate(kCFAllocatorDefault,&desc,0,nullptr,0,nullptr,nullptr,&format.value)==noErr,"PCM format");
    uint64_t nextVideo=0,nextAudio=0;
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(nextVideo<frameCount||nextAudio<frameCount){bool progressed=false;@autoreleasepool{
      // Writer readiness can depend on progress from the other track. Never wait on
      // video while a ready audio input is starved (or the reverse).
      if(nextVideo<frameCount&&video.readyForMoreMediaData){const uint64_t i=nextVideo;CF<CVPixelBufferRef> pb;
        const CVReturn status=CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(kCFAllocatorDefault,adaptor.pixelBufferPool,
            (__bridge CFDictionaryRef)@{(id)kCVPixelBufferPoolAllocationThresholdKey:@4},&pb.value);
        if(status!=kCVReturnWouldExceedAllocationThreshold){require(status==kCVReturnSuccess,"writer pixel pool allocation failed");
        CVBufferSetAttachment(pb,kCVImageBufferColorPrimariesKey,kCVImageBufferColorPrimaries_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);
        CVBufferSetAttachment(pb,kCVImageBufferTransferFunctionKey,kCVImageBufferTransferFunction_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);
        CVBufferSetAttachment(pb,kCVImageBufferYCbCrMatrixKey,kCVImageBufferYCbCrMatrix_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);
        require(CVPixelBufferLockBaseAddress(pb,0)==kCVReturnSuccess,"writer pixel lock");
        auto* base=static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(pb));const size_t pitch=CVPixelBufferGetBytesPerRow(pb);
        stats.maxVideoBufferBytes=std::max(stats.maxVideoBufferBytes,pitch*height);
        for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){
            const bool bit=(i>>(x/8))&1;auto* p=base+size_t(y)*pitch+size_t(x)*4;
            p[0]=bit?160:40;p[1]=bit?160:80;p[2]=bit?40:160;p[3]=255;}
        require(CVPixelBufferUnlockBaseAddress(pb,0)==kCVReturnSuccess,"writer pixel unlock");
        if(![adaptor appendPixelBuffer:pb withPresentationTime:CMTimeMake(int64_t(i),fps)])
            throw std::runtime_error("writer video append: "+detail(writer.error));
        ++stats.videoFrames;++nextVideo;progressed=true;if(nextVideo==frameCount)[video markAsFinished];
        }
      }
      if(nextAudio<frameCount&&audio.readyForMoreMediaData){const uint64_t i=nextAudio;constexpr size_t count=rate/fps;std::vector<int16_t> pcm(count*channels);
        for(size_t j=0;j<count;++j)for(size_t c=0;c<channels;++c)
            pcm[j*channels+c]=int16_t(int((uint64_t(i*count+j)*37+c*103)&0x7fff)-16384);
        CF<CMBlockBufferRef> block;CF<CMSampleBufferRef> sample;const size_t bytes=pcm.size()*sizeof(int16_t);
        stats.maxPCMBytes=std::max(stats.maxPCMBytes,bytes);
        require(CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault,nullptr,bytes,kCFAllocatorDefault,nullptr,0,bytes,0,&block.value)==noErr,"writer PCM block");
        require(CMBlockBufferReplaceDataBytes(pcm.data(),block,0,bytes)==noErr,"writer PCM copy");
        CMSampleTimingInfo timing{CMTimeMake(1,rate),CMTimeMake(int64_t(i*count),rate),kCMTimeInvalid};const size_t sampleSize=4;
        require(CMSampleBufferCreateReady(kCFAllocatorDefault,block,format,count,1,&timing,1,&sampleSize,&sample.value)==noErr,"writer PCM sample");
        if(![audio appendSampleBuffer:sample])throw std::runtime_error("writer PCM append: "+detail(writer.error));
        stats.audioFrames+=count;++nextAudio;progressed=true;if(nextAudio==frameCount)[audio markAsFinished];
      }
    }
      if(progressed)deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
      else{
        if(writer.status==AVAssetWriterStatusFailed)throw std::runtime_error("writer: "+detail(writer.error));
        require(std::chrono::steady_clock::now()<deadline,"writer made no progress for 10 seconds");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
    [writer endSessionAtSourceTime:CMTimeMake(frameCount,fps)];
    dispatch_semaphore_t done=dispatch_semaphore_create(0);
    [writer finishWritingWithCompletionHandler:^{dispatch_semaphore_signal(done);}];
    require(!dispatch_semaphore_wait(done,dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_SEC)),"writer finish timeout");
    if(writer.status!=AVAssetWriterStatusCompleted)throw std::runtime_error("writer finish: "+detail(writer.error));
    return stats;
}}
struct MovieReader::Impl {
    video_lab::StreamMetadata meta{},expected{};
    std::unique_ptr<BinkProcess> bink;uint32_t fpsDenominator=1;
    AVURLAsset* asset=nil;AVAssetReader* reader=nil;AVAssetReaderTrackOutput* video=nil;AVAssetReaderTrackOutput* audio=nil;
    CMSampleBufferRef pcm=nullptr;size_t pcmFrames=0,pcmOffset=0,pcmBytes=0;
    uint64_t nextVideo=0,nextAudio=0;bool videoEof=false,audioEof=false;IOStats stats;
    void releasePCM(){if(pcm)CFRelease(pcm);pcm=nullptr;pcmFrames=pcmOffset=pcmBytes=0;}
};
MovieReader::MovieReader(video_lab::StreamMetadata metadata):impl_(new Impl){
    video_lab::StreamingPlayer validator(4,maxBinkFrameBytes);require(validator.open(metadata)==video_lab::Status::Ok&&metadata.audioRate%metadata.fps==0,"reader metadata bounds/rate ratio");
    impl_->meta=impl_->expected=metadata;
}MovieReader::~MovieReader(){close();}
void MovieReader::close(){auto& s=*impl_;
    if(s.bink){if(s.bink->active())++s.stats.readerCancels;s.bink->close();s.bink.reset();}
    s.releasePCM();if(s.reader){[s.reader cancelReading];++s.stats.readerCancels;}
    if(s.asset)[s.asset cancelLoading];
    s.reader=nil;s.video=nil;s.audio=nil;s.asset=nil;s.videoEof=s.audioEof=true;}
void MovieReader::open(const std::string& path,uint64_t first,std::shared_ptr<OpenControl> control){@autoreleasepool{
    auto& s=*impl_;close();
    if(!control)control=std::make_shared<OpenControl>();
    if(hasBinkSignature(path)){
        if(control->cancelled)throw std::runtime_error("Bink open cancelled");
        auto process=std::make_unique<BinkProcess>();process->open(path,first);
        const auto info=process->info();
        s.meta={info.width,info.height,info.stride(),info.fpsNumerator,0,info.frames};
        s.fpsDenominator=info.fpsDenominator;s.bink=std::move(process);
        s.videoEof=false;s.audioEof=true;++s.stats.readerOpens;return;
    }
    s.meta=s.expected;const auto m=s.meta;s.fpsDenominator=1;
    require(first<m.frames,"seek outside movie");
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    try{
    s.asset=[AVURLAsset URLAssetWithURL:url(path) options:nil];
    keys(s.asset,@[@"duration"],s.asset,control,deadline,s.stats);
    NSArray<AVAssetTrack*>* videos=tracks(s.asset,AVMediaTypeVideo,control,deadline,s.stats);
    NSArray<AVAssetTrack*>* audios=tracks(s.asset,AVMediaTypeAudio,control,deadline,s.stats);
    require(videos.count==1&&audios.count==1,"one video and one audio track required");
    keys(videos.firstObject,@[@"naturalSize"],s.asset,control,deadline,s.stats);
    require(CMTimeCompare(s.asset.duration,CMTimeMake(int64_t(m.frames),int32_t(m.fps)))==0,"declared duration");
    require(videos.firstObject.naturalSize.width==m.width&&videos.firstObject.naturalSize.height==m.height,"declared dimensions");
    NSError* error=nil;s.reader=[[AVAssetReader alloc] initWithAsset:s.asset error:&error];
    if(!s.reader)throw std::runtime_error("reader init: "+detail(error));
    s.video=[[AVAssetReaderTrackOutput alloc] initWithTrack:videos.firstObject
        outputSettings:@{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA)}];
    s.audio=[[AVAssetReaderTrackOutput alloc] initWithTrack:audios.firstObject outputSettings:audioSettings(m.audioRate)];
    s.video.alwaysCopiesSampleData=NO;s.audio.alwaysCopiesSampleData=NO;
    require([s.reader canAddOutput:s.video]&&[s.reader canAddOutput:s.audio],"reader outputs");
    [s.reader addOutput:s.video];[s.reader addOutput:s.audio];
    s.reader.timeRange=CMTimeRangeMake(CMTimeMake(int64_t(first),int32_t(m.fps)),CMTimeMake(int64_t(m.frames-first),int32_t(m.fps)));
    if(![s.reader startReading])throw std::runtime_error("reader start: "+detail(s.reader.error));
    s.nextVideo=first;s.nextAudio=first*m.audioRate/m.fps;s.videoEof=s.audioEof=false;++s.stats.readerOpens;
    }catch(...){close();throw;}
}}
bool MovieReader::nextFrame(video_lab::Frame& output,std::shared_ptr<OpenControl> control){@autoreleasepool{
    auto& s=*impl_;
    if(s.bink){const bool found=s.bink->nextFrame(output,[control]{return control&&control->cancelled.load();});
        if(found){++s.stats.videoPackets;s.stats.maxVideoBufferBytes=std::max(s.stats.maxVideoBufferBytes,output.bgra.size());}
        return found;}
    const auto m=s.meta;require(s.reader!=nil,"reader closed");if(s.videoEof)return false;
    CF<CMSampleBufferRef> sample;
    while((sample.value=[s.video copyNextSampleBuffer])&&CMSampleBufferGetNumSamples(sample)==0){
        ++s.stats.markers;CFRelease(sample.value);sample.value=nullptr;}
    if(!sample){if(s.reader.status==AVAssetReaderStatusFailed)throw std::runtime_error("video read: "+detail(s.reader.error));
        require(s.nextVideo==m.frames,"premature video EOF");s.videoEof=true;return false;}
    require(s.nextVideo<m.frames&&CMTimeCompare(CMSampleBufferGetPresentationTimeStamp(sample),CMTimeMake(int64_t(s.nextVideo),int32_t(m.fps)))==0,"video PTS discontinuity");
    CVPixelBufferRef pb=CMSampleBufferGetImageBuffer(sample);require(pb!=nullptr,"decoder returned no pixel buffer");
    require(CVPixelBufferGetWidth(pb)==m.width&&CVPixelBufferGetHeight(pb)==m.height&&CVPixelBufferGetPixelFormatType(pb)==kCVPixelFormatType_32BGRA,"decoded pixel format/dimensions");
    require(CVPixelBufferLockBaseAddress(pb,kCVPixelBufferLock_ReadOnly)==kCVReturnSuccess,"reader pixel lock");
    const auto* base=static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pb));const size_t pitch=CVPixelBufferGetBytesPerRow(pb);
    if(pitch<size_t(m.width)*4||pitch>1048576/m.height){CVPixelBufferUnlockBaseAddress(pb,kCVPixelBufferLock_ReadOnly);throw std::runtime_error("decoder stride bound");}
    s.stats.maxVideoBufferBytes=std::max(s.stats.maxVideoBufferBytes,pitch*m.height);
    video_lab::Frame frame;frame.pts=s.nextVideo;frame.bgra.assign(m.stride*m.height,0xad);
    for(uint32_t y=0;y<m.height;++y)std::copy_n(base+size_t(y)*pitch,size_t(m.width)*4,frame.bgra.begin()+std::ptrdiff_t(size_t(y)*m.stride));
    require(CVPixelBufferUnlockBaseAddress(pb,kCVPixelBufferLock_ReadOnly)==kCVReturnSuccess,"reader pixel unlock");
    ++s.nextVideo;++s.stats.videoPackets;output=std::move(frame);return true;
}}
bool MovieReader::nextPCM(PCM& output,size_t maxFrames){@autoreleasepool{
    require(maxFrames>0&&maxFrames<=pcmPacketFrames,"PCM slice limit");auto& s=*impl_;
    if(s.bink)throw std::runtime_error("Bink path is a silent video preview; PCM/synchronization is not implemented");const auto m=s.meta;require(s.reader!=nil,"reader closed");if(s.audioEof)return false;
    if(!s.pcm){
        while((s.pcm=[s.audio copyNextSampleBuffer])&&CMSampleBufferGetNumSamples(s.pcm)==0){++s.stats.markers;s.releasePCM();}
        if(!s.pcm){if(s.reader.status==AVAssetReaderStatusFailed)throw std::runtime_error("audio read: "+detail(s.reader.error));
            require(s.nextAudio==m.frames*m.audioRate/m.fps,"premature PCM EOF");s.audioEof=true;return false;}
        const auto* desc=CMAudioFormatDescriptionGetStreamBasicDescription(CMSampleBufferGetFormatDescription(s.pcm));
        require(desc&&desc->mSampleRate==m.audioRate&&desc->mChannelsPerFrame==channels&&desc->mBitsPerChannel==16&&desc->mBytesPerFrame==4&&
            desc->mFormatID==kAudioFormatLinearPCM&&desc->mFormatFlags==(kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked),"decoded PCM format");
        require(CMTimeCompare(CMSampleBufferGetPresentationTimeStamp(s.pcm),CMTimeMake(int64_t(s.nextAudio),int32_t(m.audioRate)))==0,"PCM PTS discontinuity");
        const CMItemCount count=CMSampleBufferGetNumSamples(s.pcm);require(count>0&&uint64_t(count)<=maxNativePCMBytes/4,"native PCM packet bound");
        s.pcmFrames=size_t(count);s.pcmOffset=0;s.pcmBytes=CMBlockBufferGetDataLength(CMSampleBufferGetDataBuffer(s.pcm));
        require(s.nextAudio<=m.frames*m.audioRate/m.fps&&s.pcmFrames<=m.frames*m.audioRate/m.fps-s.nextAudio,"PCM exceeds declared duration");
        require(s.pcmBytes==s.pcmFrames*4&&s.pcmBytes<=maxNativePCMBytes,"native PCM bytes");
        s.stats.maxNativePCMBytes=std::max(s.stats.maxNativePCMBytes,s.pcmBytes);++s.stats.audioPackets;
    }
    const size_t count=std::min(maxFrames,s.pcmFrames-s.pcmOffset);PCM packet;packet.firstFrame=s.nextAudio;packet.samples.resize(count*channels);
    require(CMBlockBufferCopyDataBytes(CMSampleBufferGetDataBuffer(s.pcm),s.pcmOffset*4,count*4,packet.samples.data())==noErr,"PCM slice copy");
    s.pcmOffset+=count;s.nextAudio+=count;if(s.pcmOffset==s.pcmFrames)s.releasePCM();output=std::move(packet);return true;
}}
video_lab::StreamMetadata MovieReader::metadata() const{return impl_->meta;}
uint32_t MovieReader::frameRateDenominator() const{return impl_->fpsDenominator;}
IOUsage MovieReader::usage() const{const auto& s=*impl_;return {(s.reader||(s.bink&&s.bink->active()))?size_t(1):size_t(0),s.pcm?size_t(1):size_t(0),s.pcmBytes};}
IOStats MovieReader::stats() const{return impl_->stats;}
} // namespace stream_lab
