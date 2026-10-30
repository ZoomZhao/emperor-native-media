// SPDX-License-Identifier: MIT
#pragma once
#include "original_save.hpp"
#include "original_pak.hpp"
#include "resource_root.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <array>
#include <optional>
#include <string>
#include <vector>
namespace game_lab {
using OriginalWorldDigest=std::array<uint8_t,32>;
struct OriginalWorldFileBinding {std::string relative;uint64_t bytes=0;OriginalWorldDigest digest{};};
struct OriginalWorldSourceBinding {uint32_t schema=1;std::string contract="original-world-1";OriginalWorldFileBinding source;uint64_t legacyRevision=0;OriginalWorldDigest payloadDigest{};std::vector<OriginalWorldFileBinding> resources;};
struct OriginalWorldBoundSource {OriginalWorldSourceBinding binding;OriginalSaveResult save;std::vector<uint8_t> payload;};
inline OriginalWorldDigest originalWorldDigest(const void*,size_t);
inline bool originalWorldRelative(const std::string&);
inline std::optional<OriginalWorldBoundSource> bindOriginalWorldSource(const ResourceRoot&,const std::string&,std::string&);
inline bool verifyOriginalWorldBinding(const ResourceRoot&,const OriginalWorldSourceBinding&,std::string&);
}
#include "original_world_checkpoint_binding.cpp"
