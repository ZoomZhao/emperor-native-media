#pragma once
#include "city_simulation.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace game_lab {
constexpr size_t kCityStateBytes = CitySimulation::kLegacySnapshotBytes;
constexpr size_t kCityWarehouseStateBytes = CitySimulation::kWarehouseSnapshotBytes;
inline bool saveCityState(const CitySimulation& city, const std::filesystem::path& path, std::string& error) {
    error.clear();
    if (path.empty()) { error = "No city save path selected"; return false; }
    const auto bytes = city.saveSnapshot();
    if (bytes.size() != kCityStateBytes && bytes.size() != kCityWarehouseStateBytes) { error = "Invalid self-authored city state"; return false; }
    const auto pattern = path.string() + ".tmp.XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    const int fd = mkstemp(temporary.data());
    if (fd < 0) { error = std::string("Cannot create city save: ") + std::strerror(errno); return false; }
    bool ok = true; size_t offset = 0;
    while (offset < bytes.size()) {
        const auto count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += static_cast<size_t>(count);
    }
    if (ok && fsync(fd) != 0) ok = false;
    if (close(fd) != 0) ok = false;
    if (ok && rename(temporary.data(), path.c_str()) != 0) ok = false;
    if (!ok) { error = "Cannot write or replace city save"; unlink(temporary.data()); return false; }
    return true;
}
inline bool loadCityState(CitySimulation& city, const std::filesystem::path& path, std::string& error) {
    error.clear();
    if (path.empty()) { error = "No city save path selected"; return false; }
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
    if (fd < 0) { error = "Cannot open city save"; return false; }
    struct stat metadata{};
    if (fstat(fd, &metadata) != 0 || !S_ISREG(metadata.st_mode) || (metadata.st_size != static_cast<off_t>(kCityStateBytes) && metadata.st_size != static_cast<off_t>(kCityWarehouseStateBytes))) {
        close(fd); error = "City save must be one complete EMP1 state"; return false;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(metadata.st_size)); size_t offset = 0; bool ok = true;
    while (offset < bytes.size()) {
        const auto count = read(fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += static_cast<size_t>(count);
    }
    uint8_t extra = 0; ssize_t tail;
    do { tail = read(fd, &extra, 1); } while (tail < 0 && errno == EINTR);
    if (tail != 0) ok = false;
    if (close(fd) != 0) ok = false;
    if (!ok || !city.loadSnapshot(bytes)) { error = "Invalid or incomplete self-authored city save"; return false; }
    return true;
}
} // namespace game_lab
