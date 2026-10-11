#include "owned_reader.hpp"
#include <iostream>
#include <memory>
#include <type_traits>

using namespace emperor::file_io;
using Bytes = std::vector<uint8_t>;
static_assert(!std::is_copy_constructible_v<OwnedFramedReader>);
static_assert(!std::is_move_constructible_v<OwnedFramedReader>);
namespace {
int passed = 0, failed = 0;
FrameProfile profile() { return {1, 2, 3, 32, 32, 128, 128, 8}; }
Bytes two_frames() { return {3,0,0,0,1,0,0,0,'A',3,0,0,0,1,0,0,0,'B'}; }
void check(const char *id, bool good) {
    good ? ++passed : ++failed;
    std::cout << "{\"id\":\"" << id << "\",\"passed\":" << (good ? "true" : "false") << "}\n";
}
bool read_two(OwnedFramedReader &reader, Bytes &output) {
    auto a = reader.next(output);
    const bool first = a.status == ReadStatus::Frame && output == Bytes{'A'};
    auto b = reader.next(output);
    const bool second = b.status == ReadStatus::Frame && output == Bytes{'B'};
    auto end = reader.next(output);
    auto again = reader.next(output);
    return first && second && end.status == ReadStatus::End && again.status == ReadStatus::End &&
        end.input_offset == 18 && end.blocks_completed == 2 && end.total_output == 2 && output == Bytes{'B'};
}
}
int main() {
    {
        auto data = two_frames();
        OwnedFramedReader reader(data, profile(), StartPosition::RequiredFrame);
        check("owned/copy-input-reuse-original-vector-as-output", read_two(reader, data));
    }
    {
        auto data = two_frames();
        OwnedFramedReader reader(std::move(data), profile(), StartPosition::RequiredFrame);
        check("owned/move-input-reuse-moved-from-vector-as-output", read_two(reader, data));
    }
    {
        std::unique_ptr<OwnedFramedReader> reader;
        {
            auto data = two_frames();
            reader = std::make_unique<OwnedFramedReader>(data, profile(), StartPosition::RequiredFrame);
            data.assign(64, 0xff); // Mutate then destroy caller's source object.
        }
        Bytes output;
        check("owned/caller-input-mutated-and-destroyed", read_two(*reader, output));
    }
    {
        auto data = two_frames(); data.pop_back();
        OwnedFramedReader reader(data, profile(), StartPosition::RequiredFrame);
        auto first = reader.next(data); const bool good = first.status == ReadStatus::Frame && data == Bytes{'A'};
        auto failure = reader.next(data); auto repeated = reader.next(data);
        check("owned/later-truncation-keeps-output-and-is-sticky", good && failure.status == ReadStatus::Truncated &&
            repeated.status == ReadStatus::Truncated && failure.output_available == 0 &&
            failure.blocks_completed == 1 && failure.total_output == 1 && data == Bytes{'A'});
    }
    {
        auto data = two_frames(); auto p = profile(); p.max_output_bytes = 1;
        OwnedFramedReader reader(data, p, StartPosition::RequiredFrame);
        auto first = reader.next(data); auto limit = reader.next(data); auto repeated = reader.next(data);
        check("owned/aggregate-output-budget-preserves-prior-frame", first.status == ReadStatus::Frame &&
            limit.status == ReadStatus::OutputLimit && repeated.status == ReadStatus::OutputLimit &&
            limit.output_available == 0 && limit.total_output == 1 && data == Bytes{'A'});
    }
    {
        Bytes data{1,0,0,0}; auto p = profile(); p.max_input_bytes = 4; p.max_blocks = 0; p.max_output_bytes = 0;
        OwnedFramedReader reader(data, p);
        auto end = reader.next(data); auto again = reader.next(data);
        check("owned/empty-container-end-keeps-output", end.status == ReadStatus::End &&
            again.status == ReadStatus::End && data == Bytes({1,0,0,0}));
    }
    std::cout << "{\"summary\":true,\"passed\":" << passed << ",\"failed\":" << failed << "}\n";
    return failed ? 1 : 0;
}
