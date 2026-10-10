#include "emperor_assets.hpp"
#include "asset_bytes.hpp"
#include <cstring>
#include <new>

namespace game_lab {
DecodedWav decodeWAV(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 12 || bytes.size() > kMaxWavBytes ||
        std::memcmp(bytes.data(), "RIFF", 4) || std::memcmp(bytes.data() + 8, "WAVE", 4))
        return {};
    const uint32_t riffSize = detail::u32(bytes.data() + 4);
    if (riffSize < 4 || static_cast<uint64_t>(riffSize) + 8 != bytes.size()) return {};

    uint16_t channels = 0, bits = 0, align = 0;
    uint32_t rate = 0;
    size_t dataOffset = 0, dataSize = 0;
    bool haveFormat = false, haveData = false;
    size_t cursor = 12;
    while (cursor < bytes.size()) {
        if (bytes.size() - cursor < 8) return {};
        const uint8_t* header = bytes.data() + cursor;
        const size_t length = detail::u32(header + 4);
        cursor += 8;
        if (length > bytes.size() - cursor) return {};
        const size_t pad = length & 1u;
        if (pad > bytes.size() - cursor - length) return {};
        const uint8_t* payload = bytes.data() + cursor;
        if (!std::memcmp(header, "fmt ", 4)) {
            if (haveFormat || length < 16 || detail::u16(payload) != 1) return {};
            if (length != 16 && (length < 18 || detail::u16(payload + 16) != length - 18))
                return {}; // Extension length field and body must both be complete.
            channels = detail::u16(payload + 2);
            rate = detail::u32(payload + 4);
            const uint32_t byteRate = detail::u32(payload + 8);
            align = detail::u16(payload + 12);
            bits = detail::u16(payload + 14);
            if (channels < 1 || channels > 8 || rate < 8000 || rate > 192000 || bits != 16 ||
                align != channels * 2u || static_cast<uint64_t>(rate) * align != byteRate)
                return {};
            haveFormat = true;
        } else if (!std::memcmp(header, "data", 4)) {
            if (haveData) return {};
            dataOffset = cursor;
            dataSize = length;
            haveData = true;
        }
        cursor += length + pad;
    }
    if (!haveFormat || !haveData || !dataSize || dataSize % align != 0) return {};

    try {
        DecodedWav result;
        result.samples.resize(dataSize / 2);
        for (size_t i = 0; i < result.samples.size(); ++i)
            result.samples[i] = detail::i16(bytes.data() + dataOffset + i * 2);
        result.channels = channels;
        result.sampleRate = rate;
        result.bitsPerSample = bits;
        return result;
    } catch (const std::bad_alloc&) {
        return {};
    }
}
} // namespace game_lab
