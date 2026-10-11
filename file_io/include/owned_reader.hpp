#ifndef EMPEROR_FILE_IO_OWNED_READER_HPP
#define EMPEROR_FILE_IO_OWNED_READER_HPP

#include "framed_reader.hpp"
#include <streambuf>
#include <utility>

namespace emperor::file_io {

// Owns a complete caller-provided byte vector; copies an lvalue, moves an rvalue.
// Its private source storage cannot alias the caller's output vector. Input
// materialization/copy can throw before construction; parser limits apply to
// next(), not the caller's already allocated input or total process heap.
class OwnedFramedReader final {
    class Buffer final : public std::streambuf {
    public:
        explicit Buffer(std::vector<uint8_t> input) : input_(std::move(input)) {
            if (input_.empty()) setg(nullptr, nullptr, nullptr);
            else {
                char *begin = reinterpret_cast<char *>(input_.data());
                setg(begin, begin, begin + input_.size());
            }
        }
    private:
        std::vector<uint8_t> input_;
    };
public:
    OwnedFramedReader(std::vector<uint8_t> input, FrameProfile profile,
                      StartPosition start = StartPosition::ContainerHeader)
        : buffer_(std::move(input)), source_(&buffer_), reader_(source_, profile, start) {}
    // Internal stream/reference/get-area addresses must remain stable.
    OwnedFramedReader(const OwnedFramedReader &) = delete;
    OwnedFramedReader &operator=(const OwnedFramedReader &) = delete;
    OwnedFramedReader(OwnedFramedReader &&) = delete;
    OwnedFramedReader &operator=(OwnedFramedReader &&) = delete;
    FrameResult next(std::vector<uint8_t> &output) { return reader_.next(output); }
private:
    // Reverse destruction order keeps the source alive through reader teardown.
    Buffer buffer_;
    std::istream source_;
    FramedReader reader_;
};

}
#endif
