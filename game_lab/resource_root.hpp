#pragma once
#include <filesystem>
#include <string>

namespace game_lab {
// Explicit user configuration only. No cwd search, bundle guessing or permission changes.
class ResourceRoot {
public:
    static ResourceRoot fromAbsolute(const std::string& path);
    bool configured() const { return !root_.empty(); }
    std::string resolve(const std::string& relativePath) const;
private:
    std::filesystem::path root_;
};
} // namespace game_lab
