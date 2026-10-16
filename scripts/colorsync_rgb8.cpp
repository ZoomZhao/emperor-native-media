// Authored minimal public-API RGB8 converter. No game, UI or image acquisition.
#include <ColorSync/ColorSync.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
static std::vector<uint8_t> read(const char* path) {
 std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("input open");auto n=f.tellg();if(n<0||n>16*1024*1024)throw std::runtime_error("input budget");std::vector<uint8_t> v(static_cast<size_t>(n));f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n);if(!f)throw std::runtime_error("input read");return v;
}
static ColorSyncProfileRef profile(const std::vector<uint8_t>& bytes) {
 CFDataRef data=CFDataCreate(kCFAllocatorDefault,bytes.data(),static_cast<CFIndex>(bytes.size()));CFErrorRef error=nullptr;auto p=ColorSyncProfileCreate(data,&error);CFRelease(data);if(error)CFRelease(error);if(!p)throw std::runtime_error("ColorSyncProfileCreate");return p;
}
static CFDictionaryRef stage(ColorSyncProfileRef p,CFStringRef tag) {
 const void* keys[]={kColorSyncProfile,kColorSyncRenderingIntent,kColorSyncTransformTag,kColorSyncBlackPointCompensation};const void* values[]={p,kColorSyncRenderingIntentPerceptual,tag,kCFBooleanFalse};return CFDictionaryCreate(kCFAllocatorDefault,keys,values,4,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
}
int main(int argc,char** argv) {
 const char* root=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!root||!*root){std::cout<<"{\"skipped\":true}\n";return 0;}
 try {
  if(argc!=5)throw std::runtime_error("source-profile target-profile input-RGB8 output-RGB8 required");const auto sb=read(argv[1]),db=read(argv[2]),input=read(argv[3]);if(input.empty()||input.size()%3||input.size()>3*1000000)throw std::runtime_error("RGB8 extent");auto source=profile(sb),dest=profile(db);auto s=stage(source,kColorSyncTransformDeviceToPCS),d=stage(dest,kColorSyncTransformPCSToDevice);const void* stages[]={s,d};auto sequence=CFArrayCreate(kCFAllocatorDefault,stages,2,&kCFTypeArrayCallBacks);
  const void* keys[]={kColorSyncConvertQuality};const void* values[]={kColorSyncBestQuality};auto options=CFDictionaryCreate(kCFAllocatorDefault,keys,values,1,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);auto transform=ColorSyncTransformCreate(sequence,options);if(!transform)throw std::runtime_error("ColorSyncTransformCreate");
  const size_t width=input.size()/3;std::vector<uint8_t> output(input.size());const ColorSyncDataLayout layout=kColorSyncAlphaNone|kColorSyncByteOrderDefault;bool ok=ColorSyncTransformConvert(transform,width,1,output.data(),kColorSync8BitInteger,layout,width*3,input.data(),kColorSync8BitInteger,layout,width*3,nullptr);if(!ok)throw std::runtime_error("ColorSyncTransformConvert");
  std::ofstream f(argv[4],std::ios::binary);f.write(reinterpret_cast<const char*>(output.data()),static_cast<std::streamsize>(output.size()));if(!f)throw std::runtime_error("output write");f.close();std::cout<<"{\"converted_RGB8_triplets\":"<<width<<",\"profiles\":2,\"perceptual_intent_both_stages\":true,\"BPC\":false,\"quality\":\"Best-default\",\"input_output_alpha\":\"None\",\"input_output_depth\":\"8BitInteger\",\"convert_options\":null}\n";
  CFRelease(transform);CFRelease(options);CFRelease(sequence);CFRelease(s);CFRelease(d);CFRelease(source);CFRelease(dest);
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}return 0;
}
