#pragma once
#include <array>
#include <cerrno>
#include <cstdint>
#include <optional>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace game_lab {
// UI slots are 1..4 ASCII decimal digits, with a value in 0..3999.
inline std::optional<size_t> parseOriginalCandidateSlot(const char* text){
    if(!text||!*text)return std::nullopt;
    size_t value=0,count=0;
    for(;*text;++text){if(++count>4||*text<'0'||*text>'9')return std::nullopt;value=value*10+size_t(*text-'0');}
    return value<4000?std::optional<size_t>(value):std::nullopt;
}
inline constexpr size_t originalCandidateMetadataBudget=16384;
// A single fixed budget+1 buffer bounds reads even if a regular file grows
// after its initial size check. Short reads repeat; changed lengths refuse.
template<class Read> inline std::optional<std::vector<uint8_t>> readOriginalCandidateBudget(size_t declared,Read read){
    if(declared>originalCandidateMetadataBudget)return std::nullopt;
    std::array<uint8_t,originalCandidateMetadataBudget+1> bytes{};size_t used=0;
    while(used<bytes.size()){
        const auto n=read(bytes.data()+used,bytes.size()-used);
        if(n<0||size_t(n)>bytes.size()-used)return std::nullopt;
        if(!n)break;used+=size_t(n);
        if(used>originalCandidateMetadataBudget)return std::nullopt;
    }
    if(used!=declared)return std::nullopt;
    return std::vector<uint8_t>(bytes.begin(),bytes.begin()+used);
}
inline std::optional<std::vector<uint8_t>> readOriginalCandidateFile(const char* path){
    if(!path||!*path)return std::nullopt;
    const int fd=open(path,O_RDONLY|O_CLOEXEC|O_NONBLOCK);if(fd<0)return std::nullopt;
    struct Close{int fd;~Close(){close(fd);}} closeFile{fd};
    struct stat before{},after{};
    if(fstat(fd,&before)||!S_ISREG(before.st_mode)||before.st_size<0||uint64_t(before.st_size)>originalCandidateMetadataBudget)return std::nullopt;
    auto bytes=readOriginalCandidateBudget(size_t(before.st_size),[&](uint8_t* p,size_t n){ssize_t got;do{got=::read(fd,p,n);}while(got<0&&errno==EINTR);return got;});
    if(!bytes||fstat(fd,&after)||before.st_size!=after.st_size||before.st_mtimespec.tv_sec!=after.st_mtimespec.tv_sec||before.st_mtimespec.tv_nsec!=after.st_mtimespec.tv_nsec)return std::nullopt;
    return bytes;
}
}
