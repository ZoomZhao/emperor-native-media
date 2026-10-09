#import <AVFoundation/AVFoundation.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <vector>

namespace {
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
NSArray<AVAssetTrack*>* tracks(AVAsset* asset,AVMediaType kind){__block NSArray* result=nil;__block NSError* error=nil;auto done=dispatch_semaphore_create(0);
    [asset loadTracksWithMediaType:kind completionHandler:^(NSArray* loaded,NSError* failure){result=loaded;error=failure;dispatch_semaphore_signal(done);}];
    require(!dispatch_semaphore_wait(done,dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_SEC))&&!error&&result.count==1,"synthetic track metadata");return result;}
void write(const std::string& path,const std::vector<char>& bytes){std::ofstream file(path,std::ios::binary);file.write(bytes.data(),std::streamsize(bytes.size()));require(bool(file),"fixture write");}
uint64_t big(const std::vector<char>& bytes,size_t position,size_t count){uint64_t value=0;for(size_t i=0;i<count;++i)value=(value<<8)|uint8_t(bytes.at(position+i));return value;}
}
int main(int argc,const char* argv[]){@autoreleasepool{std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(50));std::_Exit(124);}).detach();try{
    require(argc==3,"self-authored source and fixture directory required");const std::string source=argv[1],out=argv[2];
    require(std::filesystem::path(out).filename()=="reliability-fixtures"&&std::filesystem::path(source).lexically_normal()==std::filesystem::path(out).parent_path()/"self_authored_8s.mov","only generated self-authored fixture source");
    std::filesystem::create_directories(out);
    auto* asset=[AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:source.c_str()]] options:nil];
    auto* video=tracks(asset,AVMediaTypeVideo).firstObject;auto* audio=tracks(asset,AVMediaTypeAudio).firstObject;
    AVMutableComposition* composition=[AVMutableComposition composition];
    auto* v=[composition addMutableTrackWithMediaType:AVMediaTypeVideo preferredTrackID:kCMPersistentTrackID_Invalid];
    auto* a=[composition addMutableTrackWithMediaType:AVMediaTypeAudio preferredTrackID:kCMPersistentTrackID_Invalid];
    for(unsigned i=0;i<4;++i){NSError* error=nil;const auto range=CMTimeRangeMake(kCMTimeZero,CMTimeMake(8,1));
        require([v insertTimeRange:range ofTrack:video atTime:CMTimeMake(i*8,1) error:&error]&&!error,"video composition insert");
        require([a insertTimeRange:range ofTrack:audio atTime:CMTimeMake(i*8,1) error:&error]&&!error,"PCM composition insert");}
    NSString* path=[NSString stringWithUTF8String:(out+"/self_authored_32s.mov").c_str()];
    require(![NSFileManager.defaultManager fileExistsAtPath:path],"archive old fixtures before regeneration");
    AVAssetExportSession* session=[[AVAssetExportSession alloc] initWithAsset:composition presetName:AVAssetExportPresetPassthrough];
    require(session!=nil&&[session.supportedFileTypes containsObject:AVFileTypeQuickTimeMovie],"passthrough MOV supported");
    session.outputURL=[NSURL fileURLWithPath:path];session.outputFileType=AVFileTypeQuickTimeMovie;auto done=dispatch_semaphore_create(0);
    [session exportAsynchronouslyWithCompletionHandler:^{dispatch_semaphore_signal(done);}];
    require(!dispatch_semaphore_wait(done,dispatch_time(DISPATCH_TIME_NOW,25*NSEC_PER_SEC)),"passthrough export timeout");
    if(session.status!=AVAssetExportSessionStatusCompleted){const char* reason=session.error.description.UTF8String;throw std::runtime_error(reason?reason:"export failed");}
    std::ifstream input(source,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(input)),{});require(bytes.size()>1024,"self-authored fixture size");
    write(out+"/valid8.mov",bytes);write(out+"/truncated.mov",std::vector<char>(bytes.begin(),bytes.begin()+128));write(out+"/empty.mov",{});
    write(out+"/invalid.mov",std::vector<char>(256,'X'));
    size_t dataStart=0,dataEnd=0;
    for(size_t offset=0;offset+8<=bytes.size();){uint64_t size=big(bytes,offset,4);size_t header=8;
        if(size==1){size=big(bytes,offset+8,8);header=16;}if(size==0)size=bytes.size()-offset;
        require(size>=header&&size<=bytes.size()-offset,"own MOV atom bounds");
        if(std::string(bytes.data()+offset+4,4)=="mdat"){dataStart=offset+header;dataEnd=offset+size;break;}offset+=size;}
    require(dataEnd>dataStart,"own MOV payload found");auto corrupted=bytes;
    std::fill(corrupted.begin()+std::ptrdiff_t(dataStart),corrupted.begin()+std::ptrdiff_t(dataEnd),0);write(out+"/corrupt.mov",corrupted);
    auto late=bytes;std::fill(late.begin()+std::ptrdiff_t(dataStart+(dataEnd-dataStart)/2),late.begin()+std::ptrdiff_t(dataEnd),0);write(out+"/latecorrupt.mov",late);
    std::cout<<"PASS 32-second passthrough self-made fixture and actual corrupt/truncated variants\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}}
