#include "owned_reader.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <zlib.h>

// Test-only deallocation observation, never an allocator used by production.
// Observe an address as an integer; never inspect storage after deallocation.
namespace allocation_observer {
struct Watch {
    uintptr_t address = 0;
    unsigned frees = 0;
    bool active = false;
    void begin(const void *pointer) {
        address = reinterpret_cast<uintptr_t>(pointer); frees = 0; active = pointer != nullptr;
    }
    void notice(void *pointer) noexcept {
        if (active && reinterpret_cast<uintptr_t>(pointer) == address) {
            ++frees; active = false; // Ignore later reuse of the same address.
        }
    }
} watch;
}

void *operator new(std::size_t size) {
    if (void *pointer = std::malloc(size ? size : 1)) return pointer;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *pointer) noexcept {
    allocation_observer::watch.notice(pointer); std::free(pointer);
}
void operator delete[](void *pointer) noexcept { ::operator delete(pointer); }
void operator delete(void *pointer, std::size_t) noexcept { ::operator delete(pointer); }
void operator delete[](void *pointer, std::size_t) noexcept { ::operator delete(pointer); }

using namespace emperor::file_io;
using Bytes = std::vector<uint8_t>;
using Owned = OwnedFramedReader;

static_assert(!std::is_copy_constructible_v<Owned>, "object copy must be prohibited");
static_assert(!std::is_move_constructible_v<Owned>, "object move must be prohibited");
static_assert(!std::is_copy_assignable_v<Owned>, "object copy assignment must be prohibited");
static_assert(!std::is_move_assignable_v<Owned>, "object move assignment must be prohibited");
static_assert(std::is_constructible_v<Owned, const Bytes &, FrameProfile>, "input lvalue copy is supported");
static_assert(std::is_constructible_v<Owned, Bytes &&, FrameProfile>, "input rvalue transfer is supported");

namespace {
unsigned passed = 0, failed = 0;
void check(const std::string &id, bool value) {
    if (value) ++passed; else ++failed;
    std::cout << "{\"id\":\"" << id << "\",\"passed\":" << (value ? "true" : "false") << "}\n";
}

FrameProfile profile() {
    // Reviewer synthetic profile only. No original magic/offsets are tested.
    return {0x314e574f, 0x3250495a, 0x32574152, 1024, 1024, 65536, 65536, 128};
}
void word(Bytes &bytes, uint32_t value) {
    for (unsigned shift = 0; shift != 32; shift += 8) bytes.push_back(static_cast<uint8_t>(value >> shift));
}
Bytes raw(const Bytes &payload) {
    Bytes result; word(result, profile().raw_marker); word(result, static_cast<uint32_t>(payload.size()));
    result.insert(result.end(), payload.begin(), payload.end()); return result;
}
Bytes zip_payload(const Bytes &payload) {
    Bytes result(compressBound(static_cast<uLong>(payload.size())));
    uLongf size = result.size();
    const uint8_t empty = 0;
    if (compress2(result.data(), &size, payload.empty() ? &empty : payload.data(),
                  static_cast<uLong>(payload.size()), Z_BEST_COMPRESSION) != Z_OK) throw std::runtime_error("fixture compression failed");
    result.resize(size); return result;
}
Bytes zipped(const Bytes &payload, bool bad_adler = false) {
    Bytes encoded = zip_payload(payload);
    if (bad_adler) encoded.back() ^= 1;
    Bytes result; word(result, profile().compressed_marker); word(result, static_cast<uint32_t>(encoded.size()));
    word(result, 0xfedc4321); result.insert(result.end(), encoded.begin(), encoded.end()); return result;
}
Bytes container(const std::vector<Bytes> &frames) {
    Bytes result; word(result, profile().container_marker);
    for (const auto &frame : frames) result.insert(result.end(), frame.begin(), frame.end());
    return result;
}

const Bytes first{1, 2, 3, 0, 255};
const Bytes second(79, 0x62);
const Bytes sentinel{0xca, 0xfe, 0x01};

void require_observed_owned_backing_alive() {
    if (allocation_observer::watch.frees != 0) {
        throw std::runtime_error("tracked owned backing was freed; stopped before further read");
    }
}
void frame_checks(const std::string &id, const FrameResult &result, const Bytes &output,
                  const Bytes &expected, uint64_t blocks, uint64_t total, bool compressed) {
    check(id + "/frame", result.status == ReadStatus::Frame);
    check(id + "/bytes", output == expected);
    check(id + "/available", result.output_available == expected.size());
    check(id + "/counts", result.blocks_completed == blocks && result.total_output == total);
    check(id + "/compressed", result.compressed == compressed);
}
bool identical(const FrameResult &a, const FrameResult &b) {
    return a.status == b.status && a.compressed == b.compressed && a.opaque_word == b.opaque_word &&
           a.frame_start == b.frame_start && a.input_offset == b.input_offset && a.blocks_completed == b.blocks_completed &&
           a.total_output == b.total_output && a.output_available == b.output_available &&
           a.codec.status == b.codec.status && a.codec.codec_status == b.codec.codec_status &&
           a.codec.input_consumed == b.codec.input_consumed && a.codec.output_produced == b.codec.output_produced &&
           a.codec.output_available == b.codec.output_available && a.codec.trailing_input == b.codec.trailing_input;
}
void sticky_checks(const std::string &id, Owned &reader, FrameResult initial, Bytes &output) {
    output.assign(41, 0x73); // A new caller value must also survive repeat calls.
    const Bytes before = output;
    const auto *address = output.data();
    const auto capacity = output.capacity();
    for (unsigned retry = 0; retry < 3; ++retry) {
        const auto result = reader.next(output);
        check(id + "/result-" + std::to_string(retry), identical(initial, result));
        check(id + "/output-" + std::to_string(retry), output == before && output.data() == address && output.capacity() == capacity);
    }
}

void traits() {
    check("traits/copy-object-forbidden", !std::is_copy_constructible_v<Owned>);
    check("traits/move-object-forbidden", !std::is_move_constructible_v<Owned>);
    check("traits/copy-assignment-forbidden", !std::is_copy_assignable_v<Owned>);
    check("traits/move-assignment-forbidden", !std::is_move_assignable_v<Owned>);
    check("traits/input-copy-and-transfer-supported", std::is_constructible_v<Owned, const Bytes &, FrameProfile> && std::is_constructible_v<Owned, Bytes &&, FrameProfile>);
}

void copy_path() {
    Bytes input = container({raw(first), zipped(second)});
    allocation_observer::watch.begin(input.data());
    Owned reader(static_cast<const Bytes &>(input), profile());
    check("copy/caller-storage-live-after-construction", allocation_observer::watch.frees == 0);
    Bytes().swap(input); // Immediately free original source, safely observed.
    check("copy/caller-storage-released", allocation_observer::watch.frees == 1);
    input.assign(4096, 0xee); // Reuse and overwrite original vector as output.
    frame_checks("copy/first-after-source-free", reader.next(input), input, first, 1, first.size(), false);
    input.clear(); input.shrink_to_fit(); input.assign(33, 0xbb);
    frame_checks("copy/second-after-output-reallocation", reader.next(input), input, second, 2, first.size() + second.size(), true);
    const auto end = reader.next(input);
    check("copy/end-preserves-last-output", end.status == ReadStatus::End && input == second);
    sticky_checks("copy/repeated-end", reader, end, input);
}

void move_path() {
    Bytes input = container({raw(first), zipped(second)});
    const auto encoded_size = input.size();
    allocation_observer::watch.begin(input.data());
    auto owner = std::make_unique<Owned>(std::move(input), profile());
    check("move/storage-retained-by-owner", allocation_observer::watch.frees == 0);
    require_observed_owned_backing_alive();
    input.assign(encoded_size * 3, 0xa1); // Supported moved-from-vector mutation.
    frame_checks("move/reused-original-first", owner->next(input), input, first, 1, first.size(), false);
    check("move/source-not-freed-by-output-swap", allocation_observer::watch.frees == 0);
    require_observed_owned_backing_alive();
    input.assign(8192, 0xa2); input.shrink_to_fit();
    frame_checks("move/reused-original-second", owner->next(input), input, second, 2, first.size() + second.size(), true);
    check("move/source-live-after-second-frame", allocation_observer::watch.frees == 0);
    require_observed_owned_backing_alive();
    const auto end = owner->next(input);
    check("move/end-and-storage-live", end.status == ReadStatus::End && allocation_observer::watch.frees == 0);
    owner.reset();
    check("move/owned-input-released-on-destruction", allocation_observer::watch.frees == 1);
    check("move/output-survives-owner-destruction", input == second);
}

std::unique_ptr<Owned> copied_from_local() {
    Bytes local = container({raw(first), zipped(second)});
    return std::make_unique<Owned>(local, profile());
}
void local_and_temporary() {
    auto reader = copied_from_local(); Bytes output = sentinel;
    frame_checks("local/source-destroyed-first", reader->next(output), output, first, 1, first.size(), false);
    frame_checks("local/source-destroyed-second", reader->next(output), output, second, 2, first.size() + second.size(), true);
    Owned temporary(container({zipped(second)}), profile());
    frame_checks("temporary/argument-destroyed", temporary.next(output), output, second, 1, second.size(), true);
}

void handoff_and_profile_copy() {
    FrameProfile settings = profile();
    auto original = std::make_unique<Owned>(container({raw(first), zipped(second)}), settings);
    Owned *identity = original.get();
    auto next_owner = std::move(original);
    check("unique-ptr/before-read-address-stable", !original && next_owner.get() == identity);
    settings.container_marker = 0; settings.raw_marker = 0; settings.max_blocks = 0;
    Bytes output = sentinel;
    frame_checks("profile/caller-mutation-isolated", next_owner->next(output), output, first, 1, first.size(), false);
    original = std::move(next_owner);
    check("unique-ptr/between-frames-address-stable", !next_owner && original.get() == identity);
    frame_checks("unique-ptr/continue-after-handoff", original->next(output), output, second, 2, first.size() + second.size(), true);
    const auto end = original->next(output);
    next_owner = std::move(original);
    check("unique-ptr/after-end-address-stable", !original && next_owner.get() == identity);
    sticky_checks("unique-ptr/end-after-handoff", *next_owner, end, output);
}

void independent_clones() {
    Bytes source = container({raw(first), zipped(second)});
    Owned left(source, profile()), right(source, profile());
    Bytes().swap(source);
    Bytes a = sentinel, b = sentinel;
    frame_checks("clones/left-first", left.next(a), a, first, 1, first.size(), false);
    frame_checks("clones/left-second", left.next(a), a, second, 2, first.size() + second.size(), true);
    frame_checks("clones/right-independent-position", right.next(b), b, first, 1, first.size(), false);
    frame_checks("clones/right-second", right.next(b), b, second, 2, first.size() + second.size(), true);
}

void empty_cases() {
    for (unsigned kind = 0; kind < 4; ++kind) {
        Bytes input = kind == 1 ? container({}) : kind == 3 ? raw({}) : Bytes{};
        const auto start = kind >= 2 ? StartPosition::RequiredFrame : StartPosition::ContainerHeader;
        Owned reader(std::move(input), profile(), start); Bytes output = sentinel;
        const auto result = reader.next(output);
        const std::string id = "empty/kind-" + std::to_string(kind);
        if (kind == 3) {
            frame_checks(id + "/zero-frame", result, output, {}, 1, 0, false);
            const auto end = reader.next(output);
            check(id + "/end-after-zero-frame", end.status == ReadStatus::End && output.empty());
            sticky_checks(id + "/terminal", reader, end, output);
        } else {
            check(id + "/status", result.status == (kind == 1 ? ReadStatus::End : ReadStatus::Truncated));
            check(id + "/no-output-change", output == sentinel && result.output_available == 0);
            sticky_checks(id + "/terminal", reader, result, output);
        }
    }
}

void long_mixed_stream() {
    std::vector<Bytes> outputs; std::vector<Bytes> frames;
    for (unsigned index = 0; index < 48; ++index) {
        Bytes payload = index % 5 == 0 ? Bytes{} : Bytes(1 + index * 3, static_cast<uint8_t>(index));
        outputs.push_back(payload); frames.push_back(index % 2 ? zipped(payload) : raw(payload));
    }
    Bytes original = container(frames);
    allocation_observer::watch.begin(original.data());
    Owned reader(std::move(original), profile()); uint64_t total = 0;
    for (size_t index = 0; index < outputs.size(); ++index) {
        require_observed_owned_backing_alive();
        original.assign(256 + index, 0x7e); // Same moved-from vector, changing capacities.
        total += outputs[index].size();
        frame_checks("mixed/block-" + std::to_string(index), reader.next(original), original, outputs[index], index + 1, total, index % 2 != 0);
    }
    require_observed_owned_backing_alive();
    const auto end = reader.next(original);
    check("mixed/end-after-48-blocks", end.status == ReadStatus::End && end.blocks_completed == 48 && end.total_output == total);
    sticky_checks("mixed/end", reader, end, original);
}

void error_cases() {
    for (unsigned kind = 0; kind < 5; ++kind) {
        Bytes suffix;
        FrameProfile settings = profile(); ReadStatus expected = ReadStatus::Truncated;
        if (kind == 0) { suffix = raw(second); suffix.pop_back(); }
        if (kind == 1) { word(suffix, 0x55555555); expected = ReadStatus::UnknownMarker; }
        if (kind == 2) { suffix = zipped(second, true); expected = ReadStatus::CodecError; }
        if (kind == 3) { suffix = raw(second); settings.max_output_bytes = first.size(); expected = ReadStatus::OutputLimit; }
        if (kind == 4) { suffix = raw(second); settings.max_blocks = 1; expected = ReadStatus::BlockLimit; }
        Bytes original = container({raw(first), suffix});
        allocation_observer::watch.begin(original.data());
        auto owner = std::make_unique<Owned>(std::move(original), settings);
        const std::string id = "error/kind-" + std::to_string(kind);
        require_observed_owned_backing_alive();
        frame_checks(id + "/prior-good-frame", owner->next(original), original, first, 1, first.size(), false);
        require_observed_owned_backing_alive();
        const auto address = original.data(); const auto capacity = original.capacity();
        const auto failure = owner->next(original);
        check(id + "/status", failure.status == expected);
        check(id + "/prior-frame-preserved", original == first && original.data() == address && original.capacity() == capacity && failure.output_available == 0);
        check(id + "/source-live-at-failure", allocation_observer::watch.frees == 0);
        require_observed_owned_backing_alive();
        auto handed_off = std::move(owner);
        check(id + "/unique-handoff-after-error", !owner && static_cast<bool>(handed_off));
        sticky_checks(id + "/repeat", *handed_off, failure, original);
        handed_off.reset();
        check(id + "/destruction-releases-source", allocation_observer::watch.frees == 1);
    }
    auto invalid = profile(); invalid.raw_marker = invalid.compressed_marker;
    Owned reader(container({raw(first)}), invalid); Bytes output = sentinel;
    const auto failure = reader.next(output);
    check("invalid-profile/owned-status", failure.status == ReadStatus::InvalidProfile && output == sentinel);
    sticky_checks("invalid-profile/repeat", reader, failure, output);
}

// Deliberately borrowed source: positive disjoint and negative forbidden alias.
// No get-area is exposed; each attempted read checks deallocation before
// touching saved storage, including underflow/peek.
class GuardedBorrowedBuffer final : public std::streambuf {
public:
    explicit GuardedBorrowedBuffer(const Bytes &bytes) : bytes_(bytes.data()), size_(bytes.size()) {}
    unsigned prevented = 0;
protected:
    std::streamsize xsgetn(char *destination, std::streamsize count) override {
        ensure_live();
        if (count <= 0) return 0;
        const size_t amount = std::min(size_ - position_, static_cast<size_t>(count));
        if (amount) std::memcpy(destination, bytes_ + position_, amount);
        position_ += amount; return static_cast<std::streamsize>(amount);
    }
    int_type underflow() override {
        ensure_live();
        return position_ == size_ ? traits_type::eof() : traits_type::to_int_type(static_cast<char>(bytes_[position_]));
    }
private:
    void ensure_live() {
        if (allocation_observer::watch.frees) {
            ++prevented; throw std::ios_base::failure("prevented expired borrowed storage access");
        }
    }
    const uint8_t *bytes_; size_t size_, position_ = 0;
};

void borrowed_support_boundary() {
    {
        Bytes source = container({raw(first), zipped(second)}); allocation_observer::watch.begin(source.data());
        GuardedBorrowedBuffer buffer(source); std::istream stream(&buffer); FramedReader reader(stream, profile());
        Bytes output = sentinel;
        frame_checks("borrowed-disjoint/first", reader.next(output), output, first, 1, first.size(), false);
        frame_checks("borrowed-disjoint/second", reader.next(output), output, second, 2, first.size() + second.size(), true);
        check("borrowed-disjoint/live-valid-source", allocation_observer::watch.frees == 0 && buffer.prevented == 0);
        allocation_observer::watch.active = false;
    }
    {
        Bytes shared = container({raw(first), raw(second)}); allocation_observer::watch.begin(shared.data());
        GuardedBorrowedBuffer buffer(shared); std::istream stream(&buffer); FramedReader reader(stream, profile());
        const auto first_result = reader.next(shared);
        check("borrowed-forbidden/first-frame", first_result.status == ReadStatus::Frame && shared == first);
        check("borrowed-forbidden/backing-storage-freed", allocation_observer::watch.frees == 1);
        // Do not even attempt the negative read without confirmed observer
        // coverage. A broken observer must fail the harness, never permit UAF.
        if (first_result.status != ReadStatus::Frame || allocation_observer::watch.frees != 1) {
            throw std::runtime_error("borrowed deallocation observation missing; skipped unsafe probe");
        }
        const auto result = reader.next(shared);
        check("borrowed-forbidden/guard-before-expired-read", buffer.prevented == 1 && result.status == ReadStatus::IoError && shared == first);
        std::cout << "{\"borrowed_alias_probe\":true,\"actual_expired_read\":false,\"guard_prevented\":" << buffer.prevented << "}\n";
    }
}

void destruction_cycles() {
    for (unsigned index = 0; index < 24; ++index) {
        Bytes source = container({raw(first), zipped(second)});
        allocation_observer::watch.begin(source.data());
        auto owner = std::make_unique<Owned>(std::move(source), profile());
        Bytes output = sentinel;
        require_observed_owned_backing_alive();
        if (index % 2) {
            const auto frame = owner->next(output);
            check("destroy/partial-frame-" + std::to_string(index), frame.status == ReadStatus::Frame && output == first);
        }
        check("destroy/source-live-" + std::to_string(index), allocation_observer::watch.frees == 0);
        owner.reset();
        check("destroy/one-release-" + std::to_string(index), allocation_observer::watch.frees == 1);
        check("destroy/output-remains-valid-" + std::to_string(index), output == (index % 2 ? first : sentinel));
    }
}
}

int main() {
    try {
        traits(); copy_path(); move_path(); local_and_temporary(); handoff_and_profile_copy();
        independent_clones(); empty_cases(); long_mixed_stream(); error_cases();
        borrowed_support_boundary(); destruction_cycles();
    } catch (const std::exception &error) {
        check("harness/stopped-before-unsafe-or-invalid-test-path", false);
        std::cerr << error.what() << '\n';
    }
    std::cout << "{\"summary\":true,\"passed\":" << passed << ",\"failed\":" << failed
              << ",\"zlib_runtime\":\"" << emfr_zlib_runtime_version() << "\"}\n";
    return failed ? 1 : 0;
}
