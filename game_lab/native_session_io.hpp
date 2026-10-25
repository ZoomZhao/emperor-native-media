// SPDX-License-Identifier: MIT
#pragma once
#include "native_session.hpp"
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
namespace game_lab {
// Header inspection locates the original seed; it never accepts a checkpoint.
// The caller must still load that seed and run the complete strict decoder.
struct NativeSessionReference { std::string source;uint32_t version=0; };
inline std::optional<NativeSessionReference> nativeSessionReference(const std::string& bytes){
    if(bytes.size()>512000||bytes.find('\0')!=std::string::npos)return std::nullopt;
    std::istringstream in(bytes);std::string magic,rules,path;uint32_t version=0;uint64_t source=0,model=0;
    if(!(in>>magic>>version>>rules>>std::quoted(path)>>source>>model)||magic!="ENCITY"||!nativeRulesName(version)||rules!=nativeRulesName(version)||path.empty()||path.size()>4096)return std::nullopt;
    const std::filesystem::path relative(path);if(relative.has_root_path()||relative.extension()!=".sav")return std::nullopt;
    for(const auto& part:relative)if(part=="."||part=="..")return std::nullopt;
    return NativeSessionReference{path,version};
}
// Unsaved protection compares persistent game content, not pause/resume history.
// This normalization is comparison-only; the actual saved bytes are unchanged.
inline std::string nativeSessionContent(const NativeSessionSeed& seed,const NativeSessionState& state){
    if(!validateNativeSession(seed,state).empty())return {};
    auto copy=state;copy.paused=true;copy.revision=copy.ticks;return encodeNativeSession(seed,copy);
}
inline bool nativeSessionPathOutsideData(const std::string& path,const std::string& dataRoot){
    std::error_code ec;const auto parent=std::filesystem::canonical(std::filesystem::path(path).parent_path(),ec);if(ec)return false;
    const auto root=std::filesystem::canonical(dataRoot,ec);if(ec)return false;
    auto a=root.begin(),b=parent.begin();for(;a!=root.end()&&b!=parent.end()&&*a==*b;++a,++b){}return a!=root.end();
}
inline bool writeNativeSessionNew(const std::string& path,const std::string& dataRoot,const std::string& bytes,std::string& error){
    error="存档必须是原始数据目录以外的新 .encity 文件；已有文件不会覆盖。";
    if(!std::filesystem::path(path).is_absolute()||std::filesystem::path(path).extension()!=".encity"||bytes.empty()||bytes.size()>512000||!nativeSessionPathOutsideData(path,dataRoot))return false;
    std::string pattern=path+".tmp-XXXXXX";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
    const int fd=mkstemp(name.data());if(fd<0){error="无法创建同目录临时文件。";return false;}
    bool okay=true;size_t done=0;while(done<bytes.size()){
        const auto n=::write(fd,bytes.data()+done,bytes.size()-done);if(n<0&&errno==EINTR)continue;if(n<=0){okay=false;break;}done+=size_t(n);}
    if(okay&&fsync(fd))okay=false;if(close(fd))okay=false;
    // link publishes a fully written file atomically and fails on any existing
    // destination, including symlinks; original inputs are never opened writable.
    if(okay&&::link(name.data(),path.c_str()))okay=false;::unlink(name.data());
    if(!okay){error="保存失败或文件已存在；未替换任何文件。";return false;}error.clear();return true;
}
inline std::optional<std::string> readNativeSessionFile(const std::string& path,const std::string& dataRoot,std::string& error){
    error="会话文件不可读、超出上限或位于原始数据目录。";std::error_code ec;
    const auto canonical=std::filesystem::canonical(path,ec);
    if(ec||canonical.extension()!=".encity"||!nativeSessionPathOutsideData(canonical.string(),dataRoot)||!std::filesystem::is_regular_file(canonical,ec)||ec)return std::nullopt;
    const auto size=std::filesystem::file_size(canonical,ec);if(ec||size>512000)return std::nullopt;
    std::ifstream in(canonical,std::ios::binary);std::string text(size,'\0');if(!in.read(text.data(),text.size())||in.peek()!=EOF)return std::nullopt;
    error.clear();return text;
}
} // namespace game_lab
