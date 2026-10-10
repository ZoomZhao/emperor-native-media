#pragma once
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

// Test artifact capture only. Paths are fixed below the controller's task-owned cwd.
namespace self_fixture {
inline void bytes(const char* kind, const std::vector<uint8_t>& value) {
    const char* configured = std::getenv("SELF_FIXTURE_DIR");
    if (!configured || !*configured) return;
    const auto directory = std::filesystem::path(configured);
    const auto current = std::filesystem::current_path();
    if (directory != current / "fixtures") throw std::runtime_error("unexpected fixture output root");
    std::filesystem::create_directories(directory);
    static size_t sequence = 0;
    const std::string name = std::to_string(sequence++) + "-" + kind + ".bin";
    std::ofstream payload(directory / name, std::ios::binary);
    if (!value.empty()) payload.write(reinterpret_cast<const char*>(value.data()), value.size());
    payload.close();
    if (!payload) throw std::runtime_error("fixture payload write failed");
    std::ofstream trace(current / "inputs.jsonl", std::ios::app);
    trace << "{\"kind\":\"" << kind << "\",\"file\":\"fixtures/" << name
          << "\",\"bytes\":" << value.size() << "}\n";
    trace.close();
    if (!trace) throw std::runtime_error("fixture trace write failed");
}
inline void numbers(const char* kind, const std::vector<uint64_t>& values) {
    std::vector<uint8_t> encoded;
    encoded.reserve(values.size() * 8);
    for (uint64_t value : values)
        for (unsigned shift = 0; shift < 64; shift += 8) encoded.push_back((value >> shift) & 255u);
    bytes(kind, encoded);
}
inline void numbers(const char* kind, std::initializer_list<uint64_t> values) {
    numbers(kind, std::vector<uint64_t>(values));
}
} // namespace self_fixture
