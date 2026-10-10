#include "resource_root.hpp"

namespace game_lab {
ResourceRoot ResourceRoot::fromAbsolute(const std::string& path) {
    ResourceRoot result;
    if (path.empty() || path.find('\0') != std::string::npos) return result;
    const std::filesystem::path candidate(path);
    if (!candidate.is_absolute()) return result;
    std::error_code error;
    auto canonical = std::filesystem::canonical(candidate, error);
    if (error || !std::filesystem::is_directory(canonical, error) || error) return result;
    result.root_ = std::move(canonical);
    return result;
}

std::string ResourceRoot::resolve(const std::string& relativePath) const {
    if (!configured() || relativePath.empty() || relativePath.find('\0') != std::string::npos) return {};
    const std::filesystem::path relative(relativePath);
    if (relative.has_root_path()) return {};
    for (const auto& part : relative)
        if (part == ".." || part == ".") return {};
    std::error_code error;
    const auto candidate = std::filesystem::canonical(root_ / relative, error);
    if (error) return {};
    auto rootPart = root_.begin(), candidatePart = candidate.begin();
    for (; rootPart != root_.end(); ++rootPart, ++candidatePart)
        if (candidatePart == candidate.end() || *rootPart != *candidatePart) return {};
    if (candidatePart == candidate.end() || !std::filesystem::is_regular_file(candidate, error) || error)
        return {};
    return candidate.string();
}
} // namespace game_lab
