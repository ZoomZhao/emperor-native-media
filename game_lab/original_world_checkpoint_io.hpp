// SPDX-License-Identifier: MIT
#pragma once
#include "original_world_checkpoint_codec.hpp"
namespace game_lab {
enum class OriginalWorldPublication {NotPublished,PublishedDurable,PublishedDurabilityUnknown};
enum class OriginalWorldIOFault {None,Create,Write,FileSync,Close,Publish,DirectorySync,AfterPublish,CancelBeforePublish,ParentChanged,ShortWrite,InterruptedWrite};
struct OriginalWorldPublicationResult {OriginalWorldPublication state=OriginalWorldPublication::NotPublished;std::string error;uint64_t revision=0,bytes=0;OriginalWorldDigest digest{};};
// A receipt acknowledges only the revision captured by this synchronous caller.
// Load/source switches discard the receipt; uncertain publication never marks saved.
struct OriginalWorldSaveReceipt {
    std::optional<uint64_t> savedRevision;
    bool acknowledge(const OriginalWorldPublicationResult& result,uint64_t captured,uint64_t current){
        if(result.state!=OriginalWorldPublication::PublishedDurable||result.revision!=captured||current!=captured)return false;
        savedRevision=captured;return true;
    }
};
inline OriginalWorldPublicationResult writeOriginalWorldCheckpointNew(const std::string&,const std::string&,const OriginalWorldSnapshot&,OriginalWorldIOFault=OriginalWorldIOFault::None,const std::function<void(const char*)>& probe={});
inline std::optional<std::string> readOriginalWorldCheckpointFile(const std::string&,const std::string&,std::string&);
}
#include "original_world_checkpoint_io.cpp"
