#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#include "../video_lab/video.hpp"
#include "../video_lab/reference.hpp"
#include "../audio_lab/audio.hpp"
#include "../surface_lab/oracle.hpp"
#include "../metal_lab/backend.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
constexpr int width=64,height=48,frames=12,fps=30,rate=48000,channels=2;
constexpr int audioFrames=frames*rate/fps;
// Fixed before running: every decoded RGB channel <=16 error, aggregate mean <=4, alpha exactly 255.
constexpr int maximumColorError=16;constexpr double maximumMeanError=4.0;
std::string directory,stage="setup",movieFile;size_t assertions=0,decodedFrames=0,decodedAudio=0,audioChunks=0,colorValues=0;
size_t scheduled=0,surfaceBytes=0,gpuFrames=0,badAssets=0;unsigned observedMax=0;uint64_t totalError=0;
size_t videoMarkers=0,audioMarkers=0;
bool gpuAvailable=false,encoded=false,decoded=false,rawPath=false,imageIOPath=false;std::string gpuReason,codec="H264 + LPCM S16LE";
struct Blocked:std::runtime_error{using std::runtime_error::runtime_error;};
void check(bool ok,const char* text){++assertions;if(!ok)throw std::runtime_error(stage+": "+text);}
#define CHECK(x) check((x),#x)
std::string message(NSError* e){return e?std::string(e.description.UTF8String):"no NSError detail";}
NSURL* url(const std::string& path){return [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];}
void bytes(const std::string& name,const std::vector<uint8_t>& data){std::ofstream out(directory+"/"+name,std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));CHECK(bool(out));}
void ready(AVAssetWriterInput* input,AVAssetWriter* writer){const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(!input.readyForMoreMediaData){if(writer.status==AVAssetWriterStatusFailed)throw Blocked("AVAssetWriter failed: "+message(writer.error));
        if(std::chrono::steady_clock::now()>=until)throw Blocked("AVAssetWriter input readiness timed out after 10 seconds");std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
std::vector<uint8_t> jpegEncode(const uint8_t* bgra){
    CGColorSpaceRef color=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);CHECK(color!=nullptr);
    CGDataProviderRef provider=CGDataProviderCreateWithData(nullptr,bgra,width*height*4,nullptr);CHECK(provider!=nullptr);
    CGImageRef image=CGImageCreate(width,height,8,32,width*4,color,kCGBitmapByteOrder32Little|kCGImageAlphaNoneSkipFirst,provider,nullptr,false,kCGRenderingIntentDefault);CHECK(image!=nullptr);
    CFMutableDataRef data=CFDataCreateMutable(kCFAllocatorDefault,0);CGImageDestinationRef destination=CGImageDestinationCreateWithData(data,CFSTR("public.jpeg"),1,nullptr);
    if(!destination){CGImageRelease(image);CGDataProviderRelease(provider);CGColorSpaceRelease(color);CFRelease(data);throw Blocked("ImageIO JPEG destination unavailable");}
    CGImageDestinationAddImage(destination,image,(__bridge CFDictionaryRef)@{(id)kCGImageDestinationLossyCompressionQuality:@1.0});
    const bool success=CGImageDestinationFinalize(destination);std::vector<uint8_t> result;
    if(success)result.assign(CFDataGetBytePtr(data),CFDataGetBytePtr(data)+CFDataGetLength(data));
    CFRelease(destination);CFRelease(data);CGImageRelease(image);CGDataProviderRelease(provider);CGColorSpaceRelease(color);
    if(!success)throw Blocked("ImageIO JPEG finalize failed");return result;
}
std::vector<uint8_t> jpegDecode(const std::vector<uint8_t>& data){
    CFDataRef bytes=CFDataCreate(kCFAllocatorDefault,data.data(),CFIndex(data.size()));CGImageSourceRef source=CGImageSourceCreateWithData(bytes,nullptr);CHECK(source!=nullptr);
    CHECK(CGImageSourceGetCount(source)==1);CGImageRef image=CGImageSourceCreateImageAtIndex(source,0,nullptr);CHECK(image!=nullptr);
    CHECK(CGImageGetWidth(image)==width&&CGImageGetHeight(image)==height);std::vector<uint8_t> out(width*height*4,0);
    CGColorSpaceRef color=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef context=CGBitmapContextCreate(out.data(),width,height,8,width*4,color,kCGBitmapByteOrder32Little|kCGImageAlphaPremultipliedFirst);CHECK(context!=nullptr);
    CGContextDrawImage(context,CGRectMake(0,0,width,height),image);
    CGContextRelease(context);CGColorSpaceRelease(color);CGImageRelease(image);CFRelease(source);CFRelease(bytes);return out;
}
void makeMovie(const std::string& path,AVVideoCodecType videoCodec){stage="encode";
    const bool raw=[videoCodec isEqualToString:@"BGRA_RAW"];
    const bool jpeg=[videoCodec isEqualToString:@"IMAGEIO_JPEG"],passthrough=raw||jpeg;
    NSError* error=nil;AVAssetWriter* writer=[[AVAssetWriter alloc] initWithURL:url(path) fileType:AVFileTypeQuickTimeMovie error:&error];
    if(!writer)throw Blocked("AVAssetWriter initialization: "+message(error));
    NSDictionary* compression=[videoCodec isEqualToString:AVVideoCodecTypeH264]?
        @{AVVideoAverageBitRateKey:@1000000,AVVideoExpectedSourceFrameRateKey:@(fps),AVVideoMaxKeyFrameIntervalKey:@1,AVVideoAllowFrameReorderingKey:@NO}:
        @{AVVideoQualityKey:@1.0};
    NSDictionary* vs=@{AVVideoCodecKey:videoCodec,AVVideoWidthKey:@(width),AVVideoHeightKey:@(height),AVVideoCompressionPropertiesKey:compression,
        AVVideoColorPropertiesKey:@{AVVideoColorPrimariesKey:AVVideoColorPrimaries_ITU_R_709_2,
            AVVideoTransferFunctionKey:AVVideoTransferFunction_ITU_R_709_2,AVVideoYCbCrMatrixKey:AVVideoYCbCrMatrix_ITU_R_709_2}};
    NSDictionary* as=@{AVFormatIDKey:@(kAudioFormatLinearPCM),AVSampleRateKey:@(rate),AVNumberOfChannelsKey:@(channels),
        AVLinearPCMBitDepthKey:@16,AVLinearPCMIsFloatKey:@NO,AVLinearPCMIsBigEndianKey:@NO,AVLinearPCMIsNonInterleaved:@NO};
    if((!passthrough&&![writer canApplyOutputSettings:vs forMediaType:AVMediaTypeVideo])||![writer canApplyOutputSettings:as forMediaType:AVMediaTypeAudio])
        throw Blocked("AVAssetWriter canApplyOutputSettings rejected selected video codec or LPCM settings");
    CMVideoFormatDescriptionRef rawFormat=nullptr;
    if(passthrough)CHECK(CMVideoFormatDescriptionCreate(kCFAllocatorDefault,raw?kCVPixelFormatType_24RGB:kCMVideoCodecType_JPEG,width,height,nullptr,&rawFormat)==noErr);
    AVAssetWriterInput* video=passthrough?[[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:nil sourceFormatHint:rawFormat]:
        [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:vs];
    AVAssetWriterInput* audio=[[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:as];
    video.expectsMediaDataInRealTime=NO;audio.expectsMediaDataInRealTime=NO;
    if(![writer canAddInput:video]||![writer canAddInput:audio])throw Blocked("AVAssetWriter cannot add video/audio input");
    [writer addInput:video];[writer addInput:audio];
    AVAssetWriterInputPixelBufferAdaptor* adaptor=passthrough?nil:[[AVAssetWriterInputPixelBufferAdaptor alloc] initWithAssetWriterInput:video sourcePixelBufferAttributes:
        @{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA),(id)kCVPixelBufferWidthKey:@(width),(id)kCVPixelBufferHeightKey:@(height)}];
    if(![writer startWriting])throw Blocked("AVAssetWriter startWriting: "+message(writer.error));[writer startSessionAtSourceTime:kCMTimeZero];
    // Encode-side palette, independently checked below by a literal RGB table (different channel order).
    constexpr uint8_t palette[6][3]={{32,64,192},{192,48,32},{48,176,64},{192,176,48},{80,144,208},{160,80,144}};
    std::vector<uint8_t> originals;
    for(int i=0;i<frames;++i){ready(video,writer);CVPixelBufferRef pb=nullptr;
        CHECK(CVPixelBufferCreate(kCFAllocatorDefault,width,height,kCVPixelFormatType_32BGRA,nullptr,&pb)==kCVReturnSuccess);
        CVBufferSetAttachment(pb,kCVImageBufferColorPrimariesKey,kCVImageBufferColorPrimaries_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);
        CVBufferSetAttachment(pb,kCVImageBufferTransferFunctionKey,kCVImageBufferTransferFunction_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);
        CVBufferSetAttachment(pb,kCVImageBufferYCbCrMatrixKey,kCVImageBufferYCbCrMatrix_ITU_R_709_2,kCVAttachmentMode_ShouldPropagate);
        CHECK(CVPixelBufferLockBaseAddress(pb,0)==kCVReturnSuccess);auto* base=static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(pb));const size_t stride=CVPixelBufferGetBytesPerRow(pb);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto* p=base+size_t(y)*stride+size_t(x)*4;
            for(int c=0;c<3;++c)p[c]=palette[i%6][c];p[3]=255;originals.insert(originals.end(),p,p+4);}
        CHECK(CVPixelBufferUnlockBaseAddress(pb,0)==kCVReturnSuccess);BOOL appended=NO;
        if(passthrough){CMBlockBufferRef rawBlock=nullptr;CMSampleBufferRef rawSample=nullptr;const size_t begin=size_t(i)*width*height*4;
            std::vector<uint8_t> rgb;if(jpeg)rgb=jpegEncode(originals.data()+begin);else{rgb.resize(width*height*3);
                for(size_t pixel=0;pixel<size_t(width*height);++pixel){rgb[pixel*3]=originals[begin+pixel*4+2];rgb[pixel*3+1]=originals[begin+pixel*4+1];rgb[pixel*3+2]=originals[begin+pixel*4];}}
            const size_t frameSize=rgb.size();
            CHECK(CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault,nullptr,frameSize,kCFAllocatorDefault,nullptr,0,frameSize,0,&rawBlock)==noErr);
            CHECK(CMBlockBufferReplaceDataBytes(rgb.data(),rawBlock,0,frameSize)==noErr);
            CMSampleTimingInfo timing{CMTimeMake(1,fps),CMTimeMake(i,fps),kCMTimeInvalid};
            CHECK(CMSampleBufferCreateReady(kCFAllocatorDefault,rawBlock,rawFormat,1,1,&timing,1,&frameSize,&rawSample)==noErr);
            appended=[video appendSampleBuffer:rawSample];CFRelease(rawSample);CFRelease(rawBlock);
        }else appended=[adaptor appendPixelBuffer:pb withPresentationTime:CMTimeMake(i,fps)];CVPixelBufferRelease(pb);
        if(!appended)throw Blocked("AVAssetWriter appendPixelBuffer: "+message(writer.error));}
    [video markAsFinished];if(rawFormat)CFRelease(rawFormat);bytes("source_frames.bgra",originals);
    ready(audio,writer);AudioStreamBasicDescription desc{};desc.mSampleRate=rate;desc.mFormatID=kAudioFormatLinearPCM;
    desc.mFormatFlags=kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;desc.mBytesPerPacket=4;desc.mFramesPerPacket=1;
    desc.mBytesPerFrame=4;desc.mChannelsPerFrame=2;desc.mBitsPerChannel=16;
    CMAudioFormatDescriptionRef format=nullptr;CHECK(CMAudioFormatDescriptionCreate(kCFAllocatorDefault,&desc,0,nullptr,0,nullptr,nullptr,&format)==noErr);
    CMBlockBufferRef block=nullptr;const size_t size=audioFrames*4;CHECK(CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault,nullptr,size,kCFAllocatorDefault,nullptr,0,size,0,&block)==kCMBlockBufferNoErr);
    std::vector<uint8_t> silence(size,0);CHECK(CMBlockBufferReplaceDataBytes(silence.data(),block,0,size)==kCMBlockBufferNoErr);
    CMSampleTimingInfo timing{CMTimeMake(1,rate),kCMTimeZero,kCMTimeInvalid};const size_t sampleSize=4;CMSampleBufferRef sample=nullptr;
    CHECK(CMSampleBufferCreateReady(kCFAllocatorDefault,block,format,audioFrames,1,&timing,1,&sampleSize,&sample)==noErr);
    const BOOL appended=[audio appendSampleBuffer:sample];CFRelease(sample);CFRelease(block);CFRelease(format);
    if(!appended)throw Blocked("AVAssetWriter append LPCM: "+message(writer.error));[audio markAsFinished];bytes("source_audio.s16le",silence);
    [writer endSessionAtSourceTime:CMTimeMake(frames,fps)];dispatch_semaphore_t finished=dispatch_semaphore_create(0);
    [writer finishWritingWithCompletionHandler:^{dispatch_semaphore_signal(finished);}];
    if(dispatch_semaphore_wait(finished,dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_SEC)))throw Blocked("AVAssetWriter finishWriting timed out after 10 seconds");
    if(writer.status!=AVAssetWriterStatusCompleted)throw Blocked("AVAssetWriter finishWriting: "+message(writer.error));encoded=true;
    rawPath=raw;imageIOPath=jpeg;std::cout<<"PASS self-authored MOV "<<(raw?"uncompressed passthrough":(jpeg?"ImageIO JPEG + AVFoundation mux":"encoded"))<<": 12 color frames + 19200 stereo silence frames\n";
}
struct Decoded {video_lab::Clip clip;std::vector<int16_t> pcm;};
std::vector<uint8_t> referenceBytes(CMSampleBufferRef sample,const std::string& path){
    NSURL* reference=(__bridge NSURL*)CMGetAttachment(sample,kCMSampleBufferAttachmentKey_SampleReferenceURL,nullptr);
    NSNumber* offset=(__bridge NSNumber*)CMGetAttachment(sample,kCMSampleBufferAttachmentKey_SampleReferenceByteOffset,nullptr);
    CHECK(reference&&offset);CHECK([reference.path isEqualToString:url(path).path]);CHECK(offset.longLongValue>=0);
    NSData* file=[NSData dataWithContentsOfURL:url(path)];CHECK(file!=nil);const size_t size=CMSampleBufferGetTotalSampleSize(sample);
    const uint64_t begin=offset.unsignedLongLongValue;CHECK(begin<=file.length&&size<=file.length-begin);
    const auto* bytes=static_cast<const uint8_t*>(file.bytes);return {bytes+begin,bytes+begin+size};
}
Decoded readMovie(const std::string& path){stage=rawPath?"raw sample reference reading":"compressed decode";AVURLAsset* asset=[AVURLAsset URLAssetWithURL:url(path) options:nil];
    NSArray<AVAssetTrack*>* vt=[asset tracksWithMediaType:AVMediaTypeVideo];NSArray<AVAssetTrack*>* at=[asset tracksWithMediaType:AVMediaTypeAudio];
    CHECK(vt.count==1&&at.count==1);CHECK(CMTimeCompare(asset.duration,CMTimeMake(frames,fps))==0);
    CHECK(vt.firstObject.naturalSize.width==width&&vt.firstObject.naturalSize.height==height);
    NSError* error=nil;AVAssetReader* reader=[[AVAssetReader alloc] initWithAsset:asset error:&error];if(!reader)throw Blocked("AVAssetReader initialization: "+message(error));
    // Native sample references give PTS/format/offset metadata without requiring a CoreVideo pixel bridge.
    // Only the known, self-written raw movie is read by offset. This is demux + raw-byte reading, never compressed decoding.
    const bool references=rawPath||imageIOPath;
    AVAssetReaderOutput* video=references?[[AVAssetReaderSampleReferenceOutput alloc] initWithTrack:vt.firstObject]:
        [[AVAssetReaderTrackOutput alloc] initWithTrack:vt.firstObject outputSettings:@{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA)}];
    AVAssetReaderOutput* audio=references?[[AVAssetReaderSampleReferenceOutput alloc] initWithTrack:at.firstObject]:
        [[AVAssetReaderTrackOutput alloc] initWithTrack:at.firstObject outputSettings:
        @{AVFormatIDKey:@(kAudioFormatLinearPCM),AVLinearPCMBitDepthKey:@16,AVLinearPCMIsFloatKey:@NO,AVLinearPCMIsBigEndianKey:@NO,AVLinearPCMIsNonInterleaved:@NO}];
    CHECK([reader canAddOutput:video]&&[reader canAddOutput:audio]);[reader addOutput:video];[reader addOutput:audio];
    if(![reader startReading])throw Blocked("AVAssetReader startReading: "+message(reader.error));
    Decoded out;out.clip.width=width;out.clip.height=height;out.clip.stride=width*4+5;out.clip.ticksPerSecond=fps;out.clip.duration=frames;out.clip.declaredFrames=frames;
    // Independent hand-written RGB expectation, never derived from encoded bytes or decoder output.
    constexpr int rgb[6][3]={{192,64,32},{32,48,192},{64,176,48},{48,176,192},{208,144,80},{144,80,160}};
    std::vector<uint8_t> decodedBytes;std::ofstream trace(directory+"/decoded_pts.jsonl");
    while(CMSampleBufferRef sb=[video copyNextSampleBuffer]){if(CMSampleBufferGetNumSamples(sb)==0){++videoMarkers;CFRelease(sb);continue;}
        const auto i=out.clip.frames.size();CHECK(i<size_t(frames));
        const CMTime pts=CMSampleBufferGetPresentationTimeStamp(sb);CHECK(CMTIME_IS_NUMERIC(pts));CHECK(CMTimeCompare(pts,CMTimeMake(int64_t(i),fps))==0);
        trace<<"{\"frame\":"<<i<<",\"pts_value\":"<<pts.value<<",\"pts_timescale\":"<<pts.timescale<<",\"expected_numerator\":"<<i<<",\"expected_denominator\":30}\n";
        CVPixelBufferRef pb=nullptr;std::vector<uint8_t> rawBytes;const uint8_t* base=nullptr;size_t pitch=0;
        if(references){const auto format=CMSampleBufferGetFormatDescription(sb);const auto size=CMVideoFormatDescriptionGetDimensions(format);
            trace<<"{\"raw_sample\":"<<i<<",\"samples\":"<<CMSampleBufferGetNumSamples(sb)<<",\"width\":"<<size.width<<",\"height\":"<<size.height
                <<",\"subtype\":"<<CMFormatDescriptionGetMediaSubType(format)<<"}\n";trace.flush();
            CHECK(size.width==width&&size.height==height);CHECK(CMFormatDescriptionGetMediaSubType(format)==(rawPath?kCVPixelFormatType_24RGB:kCMVideoCodecType_JPEG));
            CHECK(CMSampleBufferGetNumSamples(sb)==1);std::vector<uint8_t> rgbBytes=referenceBytes(sb,path);
            if(imageIOPath)rawBytes=jpegDecode(rgbBytes);else{CHECK(rgbBytes.size()==width*height*3);
                rawBytes.resize(width*height*4);for(size_t pixel=0;pixel<size_t(width*height);++pixel){rawBytes[pixel*4]=rgbBytes[pixel*3+2];
                    rawBytes[pixel*4+1]=rgbBytes[pixel*3+1];rawBytes[pixel*4+2]=rgbBytes[pixel*3];rawBytes[pixel*4+3]=255;}}base=rawBytes.data();pitch=width*4;
        }else{pb=CMSampleBufferGetImageBuffer(sb);CHECK(pb!=nullptr);CHECK(CVPixelBufferGetWidth(pb)==width&&CVPixelBufferGetHeight(pb)==height);
            CHECK(CVPixelBufferGetPixelFormatType(pb)==kCVPixelFormatType_32BGRA);CHECK(CVPixelBufferLockBaseAddress(pb,kCVPixelBufferLock_ReadOnly)==kCVReturnSuccess);
            base=static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pb));pitch=CVPixelBufferGetBytesPerRow(pb);CHECK(pitch>=width*4);}
        trace<<"{\"color_frame\":"<<i<<",\"first_BGRA\":["<<unsigned(base[0])<<","<<unsigned(base[1])<<","<<unsigned(base[2])<<","<<unsigned(base[3])<<"],\"expected_RGB\":["<<rgb[i%6][0]<<","<<rgb[i%6][1]<<","<<rgb[i%6][2]<<"]}\n";trace.flush();
        video_lab::Frame frame;frame.pts=i;frame.bgra.assign(out.clip.stride*height,0xad);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x){const auto* p=base+size_t(y)*pitch+size_t(x)*4;
            std::copy(p,p+4,frame.bgra.begin()+std::ptrdiff_t(size_t(y)*out.clip.stride+size_t(x)*4));
            for(int c=0;c<3;++c){const unsigned diff=unsigned(std::abs(int(p[c])-rgb[i%6][2-c]));observedMax=std::max(observedMax,diff);totalError+=diff;++colorValues;CHECK(diff<=maximumColorError);}CHECK(p[3]==255);}
        if(pb)CHECK(CVPixelBufferUnlockBaseAddress(pb,kCVPixelBufferLock_ReadOnly)==kCVReturnSuccess);CFRelease(sb);
        decodedBytes.insert(decodedBytes.end(),frame.bgra.begin(),frame.bgra.end());out.clip.frames.push_back(std::move(frame));++decodedFrames;}
    CHECK(out.clip.frames.size()==frames);CHECK(double(totalError)/double(colorValues)<=maximumMeanError);if(rawPath)CHECK(observedMax==0);bytes("decoded_frames.bgra",decodedBytes);
    std::vector<uint8_t> audioBytes;
    while(CMSampleBufferRef sb=[audio copyNextSampleBuffer]){const CMItemCount count=CMSampleBufferGetNumSamples(sb);if(count==0){++audioMarkers;CFRelease(sb);continue;}CHECK(count>0);
        CHECK(CMTimeCompare(CMSampleBufferGetPresentationTimeStamp(sb),CMTimeMake(int64_t(decodedAudio),rate))==0);
        const AudioStreamBasicDescription* asbd=CMAudioFormatDescriptionGetStreamBasicDescription(CMSampleBufferGetFormatDescription(sb));
        CHECK(asbd&&asbd->mSampleRate==rate&&asbd->mChannelsPerFrame==2&&asbd->mBitsPerChannel==16&&asbd->mBytesPerFrame==4);
        CHECK(asbd->mFormatID==kAudioFormatLinearPCM);CHECK((asbd->mFormatFlags&kAudioFormatFlagIsSignedInteger)!=0);
        CHECK((asbd->mFormatFlags&(kAudioFormatFlagIsFloat|kAudioFormatFlagIsBigEndian|kAudioFormatFlagIsNonInterleaved))==0);
        std::ofstream(directory+"/audio_format.json")<<"{\"rate\":48000,\"channels\":2,\"bits\":16,\"bytes_per_frame\":4,\"ASBD_flags\":"<<asbd->mFormatFlags<<",\"signed_integer\":true,\"little_endian\":true,\"interleaved\":true}";
        std::vector<uint8_t> raw;if(references)raw=referenceBytes(sb,path);
        CMBlockBufferRef block=references?nullptr:CMSampleBufferGetDataBuffer(sb);if(!references)CHECK(block!=nullptr);
        const size_t size=references?raw.size():CMBlockBufferGetDataLength(block);CHECK(size==size_t(count)*4);
        const size_t offset=audioBytes.size();audioBytes.resize(offset+size);
        if(references)std::copy(raw.begin(),raw.end(),audioBytes.begin()+std::ptrdiff_t(offset));else CHECK(CMBlockBufferCopyDataBytes(block,0,size,audioBytes.data()+offset)==noErr);
        for(size_t j=offset;j<audioBytes.size();++j)CHECK(audioBytes[j]==0);decodedAudio+=size_t(count);++audioChunks;CFRelease(sb);}
    if(reader.status!=AVAssetReaderStatusCompleted)throw Blocked("AVAssetReader completion: "+message(reader.error));
    CHECK(decodedAudio==audioFrames);CHECK(audioBytes.size()==audioFrames*4);out.pcm.assign(audioFrames*channels,0);bytes("decoded_audio.s16le",audioBytes);decoded=true;
    std::cout<<"PASS offline "<<(rawPath?"raw-frame demux/read":"decode")<<": exact PTS/dimensions/audio framecount; max RGB error "<<observedMax<<", mean "<<double(totalError)/double(colorValues)<<"\n";return out;
}
void pipeline(const Decoded& input){stage="component integration";video_lab::Player video(rate);scalar_reference::Reference oracle(rate);
    CHECK(video.load(input.clip)==video_lab::Status::Ok);CHECK(oracle.load(input.clip)==video_lab::Status::Ok);
    audio_lab::Player audio({rate,channels});audio_lab::Handle source=0;uint64_t ticket=0;
    CHECK(audio.createSource({rate,channels},input.pcm,source)==audio_lab::Status::Ok);CHECK(audio.enqueue(source,1,ticket)==audio_lab::Status::Ok);CHECK(audio.start()==audio_lab::Status::Ok);
    for(int i=0;i<4;++i){CHECK(video.enqueue(i,video.state().epoch)==video_lab::Status::Ok);CHECK(oracle.enqueue(i,oracle.state().epoch)==video_lab::Status::Ok);}
    CHECK(video.start({video.state().epoch,0,rate})==video_lab::Status::Ok);CHECK(oracle.start({oracle.state().epoch,0,rate})==video_lab::Status::Ok);
    metal_lab::Backend gpu;gpuAvailable=gpu.available();gpuReason=gpu.reason();surface_lab::Store store;surface_lab::Handle src=0,dst=0;
    const surface_lab::Desc sd{width,height,input.clip.stride,surface_lab::Format::BGRA8},dd{32,24,133,surface_lab::Format::BGRA8};
    CHECK(store.create(sd,src)==surface_lab::Status::Ok);CHECK(store.create(dd,dst)==surface_lab::Status::Ok);
    std::vector<uint8_t> rendered,expectedRendered;std::ofstream trace(directory+"/pipeline.jsonl");
    for(int i=0;i<frames;++i){video_lab::Decision actual,expected;const video_lab::Clock clock{video.state().epoch,uint64_t(i)*1600,rate};
        CHECK(video.advance(clock,actual)==video_lab::Status::Ok);CHECK(oracle.advance(clock,expected)==video_lab::Status::Ok);
        CHECK(actual.kind==video_lab::Kind::Presented&&actual.index==i&&actual.mediaTicks==uint64_t(i)&&actual.pts==uint64_t(i));
        CHECK(actual.kind==expected.kind&&actual.index==expected.index&&actual.mediaTicks==expected.mediaTicks&&actual.bgra==expected.bgra);
        CHECK(actual.bgra==input.clip.frames[size_t(i)].bgra);++scheduled;
        CHECK(store.replaceBytes(src,actual.bgra)==surface_lab::Status::Ok);std::vector<uint8_t> original(dd.stride*dd.height,0xba);
        CHECK(store.replaceBytes(dst,original)==surface_lab::Status::Ok);const auto pixels=scalar_oracle::expected(actual.bgra,sd,original,dd,{0,0,width,height},{-1,-1,34,26});
        std::vector<uint8_t> result;if(gpuAvailable){metal_lab::Ticket job;CHECK(gpu.submit(store,dst,src,{0,0,width,height},{-1,-1,34,26},true,job)==metal_lab::Status::Ok);
            CHECK(metal_lab::Backend::finish(store,job,&result)==metal_lab::Status::Ok);CHECK(metal_lab::Backend::callbackCount(job)==1);++gpuFrames;}
        else{CHECK(store.nearest(dst,src,{0,0,width,height},{-1,-1,34,26})==surface_lab::Status::Ok);CHECK(store.readBytes(dst,result)==surface_lab::Status::Ok);}
        CHECK(result==pixels);surfaceBytes+=result.size();rendered.insert(rendered.end(),result.begin(),result.end());expectedRendered.insert(expectedRendered.end(),pixels.begin(),pixels.end());
        const auto state=video.state();if(state.nextFeed<frames){CHECK(video.enqueue(state.nextFeed,state.epoch)==video_lab::Status::Ok);CHECK(oracle.enqueue(state.nextFeed,oracle.state().epoch)==video_lab::Status::Ok);}
        audio_lab::Pull pcm;CHECK(audio.consume(1600,pcm)==audio_lab::Status::Ok);CHECK(pcm.sourceFrames==1600&&pcm.underrunFrames==0&&pcm.blockedFrames==0);
        CHECK(pcm.samples.size()==3200);for(auto sample:pcm.samples)CHECK(sample==0);
        trace<<"{\"frame\":"<<i<<",\"media_tick\":"<<actual.mediaTicks<<",\"audio_frame_clock\":"<<uint64_t(i)*1600<<",\"source_pcm_frames\":1600,\"GPU\":"<<(gpuAvailable?"true":"false")<<"}\n";}
    video_lab::Decision end,ref;CHECK(video.advance({video.state().epoch,audioFrames,rate},end)==video_lab::Status::Ok);
    CHECK(oracle.advance({oracle.state().epoch,audioFrames,rate},ref)==video_lab::Status::Ok);CHECK(end.kind==video_lab::Kind::Eof&&ref.kind==end.kind&&end.index==frames-1);
    const auto events=audio.pollEvents();CHECK(events.size()==1&&events[0].type==audio_lab::EventType::Completed&&events[0].deliveredFrames==audioFrames);
    CHECK(audio.state().counters.sourceFrames==audioFrames&&audio.state().counters.underrunFrames==0);CHECK(audio.releaseSource(source)==audio_lab::Status::Ok);
    // Reuse the decoded clip through seek generation isolation and new clock binding.
    const auto old=video.state().epoch;CHECK(video.seek(6)==video_lab::Status::Ok);CHECK(video.enqueue(6,old)==video_lab::Status::StaleEpoch);
    CHECK(video.enqueue(6,video.state().epoch)==video_lab::Status::Ok);CHECK(video.start({video.state().epoch,0,rate})==video_lab::Status::Ok);
    CHECK(video.advance({video.state().epoch,0,rate},end)==video_lab::Status::Ok);CHECK(end.index==6&&end.pts==6&&end.bgra==input.clip.frames[6].bgra);
    bytes("pipeline_rendered.bgra",rendered);bytes("pipeline_expected.bgra",expectedRendered);
    std::cout<<"PASS file frames -> video scheduler -> "<<(gpuAvailable?"Metal":"CPU surface (GPU unavailable)")<<"; file PCM -> audio queue, exact EOF\n";
}
bool rejects(const std::string& path){AVURLAsset* asset=[AVURLAsset URLAssetWithURL:url(path) options:nil];NSError* error=nil;
    AVAssetReader* reader=[[AVAssetReader alloc] initWithAsset:asset error:&error];NSArray<AVAssetTrack*>* tracks=[asset tracksWithMediaType:AVMediaTypeVideo];
    if(!reader||!tracks.count)return true;AVAssetReaderTrackOutput* output=[[AVAssetReaderTrackOutput alloc] initWithTrack:tracks.firstObject outputSettings:@{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA)}];
    if(![reader canAddOutput:output])return true;[reader addOutput:output];if(![reader startReading])return true;
    CMSampleBufferRef sample=[output copyNextSampleBuffer];if(sample){CFRelease(sample);[reader cancelReading];return false;}return true;
}
void malformed(const std::string& movie){stage="self-authored malformed assets";bytes("empty.mov",{});
    NSData* full=[NSData dataWithContentsOfURL:url(movie)];CHECK(full.length>32);NSData* shortData=[full subdataWithRange:NSMakeRange(0,16)];
    CHECK([shortData writeToURL:url(directory+"/short.mov") atomically:YES]);bytes("corrupt.mov",std::vector<uint8_t>(64,0x5a));
    for(const char* name:{"empty.mov","short.mov","corrupt.mov"}){CHECK(rejects(directory+"/"+name));++badAssets;}
    std::cout<<"PASS empty/16-byte truncated/self-authored invalid MOV rejected\n";
}
void summary(bool passed,bool available,const std::string& reason){NSDictionary* d=@{@"passed":@(passed),@"available":@(available),@"reason":[NSString stringWithUTF8String:reason.c_str()],
    @"stage":[NSString stringWithUTF8String:stage.c_str()],@"codec":[NSString stringWithUTF8String:codec.c_str()],@"movie_written":@(encoded),@"media_samples_read":@(decoded),
    @"movie_file":[NSString stringWithUTF8String:movieFile.c_str()],
    @"uncompressed_passthrough":@(rawPath&&encoded),@"compressed_video_codec_verified":@(!rawPath&&decoded),
    @"raw_movie_sample_reference_read":@(rawPath&&decoded),
    @"ImageIO_JPEG_decode_passed":@(imageIOPath&&decoded),@"AVFoundation_pixel_decode_passed":@(!rawPath&&!imageIOPath&&decoded),
    @"video_zero_sample_markers_skipped":@(videoMarkers),@"audio_zero_sample_markers_skipped":@(audioMarkers),
    @"assertions":@(assertions),@"video_frames_read":@(decodedFrames),@"audio_frames_read":@(decodedAudio),@"audio_chunks":@(audioChunks),
    @"width":@(width),@"height":@(height),@"fps":@(fps),@"audio_rate":@(rate),@"audio_channels":@(channels),@"color_values":@(colorValues),
    @"color_max_tolerance":@(maximumColorError),@"color_mean_tolerance":@(maximumMeanError),@"observed_max_color_error":@(observedMax),
    @"observed_mean_color_error":@(colorValues?double(totalError)/double(colorValues):0),@"scheduled_frames":@(scheduled),@"surface_bytes_compared":@(surfaceBytes),
    @"GPU_available":@(gpuAvailable),@"GPU_frames":@(gpuFrames),@"GPU_reason":[NSString stringWithUTF8String:gpuReason.c_str()],@"malformed_assets_rejected":@(badAssets),
    @"actual_audio_output_or_real_AV_sync":@NO,@"game_media_or_code_used":@NO,@"UI_network_used":@NO};
    NSData* data=[NSJSONSerialization dataWithJSONObject:d options:NSJSONWritingPrettyPrinted error:nil];[data writeToURL:url(directory+"/summary.json") atomically:YES];}
}
int main(int argc,char** argv){@autoreleasepool{if(argc!=2)return 2;directory=argv[1];std::string movie;
    try{std::ofstream attempts(directory+"/codec_attempts.log");std::string failures;
        for(AVVideoCodecType candidate in @[AVVideoCodecTypeH264,AVVideoCodecTypeJPEG,@"IMAGEIO_JPEG",@"BGRA_RAW"]){
            const bool h264=[candidate isEqualToString:AVVideoCodecTypeH264],raw=[candidate isEqualToString:@"BGRA_RAW"];
            const bool jpeg=[candidate isEqualToString:@"IMAGEIO_JPEG"];
            codec=raw?"uncompressed RGB24 + LPCM S16LE":(jpeg?"ImageIO JPEG + AVFoundation MOV/LPCM":(h264?"H264 + LPCM S16LE":"JPEG + LPCM S16LE"));
            const std::string path=directory+(raw?"/self_authored_raw.mov":(jpeg?"/self_authored_imageio_jpeg.mov":(h264?"/self_authored_h264.mov":"/self_authored_jpeg.mov")));
            // Only replace this run's generated file; no external media is accepted by the harness.
            [[NSFileManager defaultManager] removeItemAtURL:url(path) error:nil];
            try{makeMovie(path,candidate);movie=path;movieFile=path.substr(path.find_last_of('/')+1);attempts<<codec<<" : SUCCESS\n";break;}
            catch(const Blocked& e){attempts<<codec<<" : "<<e.what()<<"\n";attempts.flush();failures+=codec+" : "+e.what()+"; ";}}
        if(movie.empty())throw Blocked(failures);
        const auto decodedClip=readMovie(movie);pipeline(decodedClip);malformed(movie);summary(true,true,"");return 0;}
    catch(const Blocked& e){summary(false,false,e.what());std::cout<<"BLOCKED AVFoundation "<<e.what()<<"\n";return 3;}
    catch(const std::exception& e){summary(false,true,e.what());std::cerr<<"FAIL AVFoundation "<<e.what()<<"\n";return 1;}}}
