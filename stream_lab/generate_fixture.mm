#import <Foundation/Foundation.h>
#include "movie_io.hpp"
#include <filesystem>
#include <iostream>
#include <thread>
#include <cstdlib>
int main(int argc,const char* argv[]){@autoreleasepool{
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(40));std::_Exit(124);}).detach();
    if(argc!=2)return 2;const std::filesystem::path output=argv[1];
    if(output.filename()!="self_authored_8s.mov"||output.parent_path().filename()!=".generated")return 2;
    try{std::filesystem::create_directories(output.parent_path());const auto stats=stream_lab::writeSyntheticMovie(output.string());
        if(stats.videoFrames!=240||stats.audioFrames!=384000)return 1;
        std::cout<<"PASS generated own 8-second BGRA/H264/S16 stereo fixture\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}}
