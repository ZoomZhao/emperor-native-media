// SPDX-License-Identifier: MIT
#pragma once
#include "original_world_snapshot.hpp"
#include <string_view>
namespace game_lab {
inline constexpr size_t originalWorldFileLimit=128u*1024u*1024u;
inline constexpr size_t originalWorldAllocationLimit=512u*1024u*1024u;
inline std::optional<std::string> encodeOriginalWorldCheckpoint(const OriginalWorldSnapshot&,std::string&);
inline std::optional<OriginalWorldSnapshot> decodeOriginalWorldCheckpoint(std::string_view,std::string&);
inline std::string originalWorldCheckpointValueIssue(const OriginalWorldSnapshot&);
inline bool originalWorldPersistentEqual(const OriginalWorldSnapshot&,const OriginalWorldSnapshot&,std::string&);
}
#include "original_world_checkpoint_codec.cpp"
