#pragma once
#include <cstddef>
#include <cstdint>
namespace emperor::file_io::research {
struct Goods1Field{uint32_t receiver_offset;size_t bytes;const char *source_call;};
inline constexpr Goods1Field goods1_fields[]={
    {4,4,"0x506ac9"},
    {8,2,"0x506ad6"},
    {10,2,"0x506ae3"},
    {12,1,"0x506af0"},
    {13,1,"0x506afd"},
    {14,36,"0x506b0a"},
    {50,1,"0x506b17"},
    {51,1,"0x506b24"},
    {52,1,"0x506b31"},
    {53,1,"0x506b3e"},
    {54,2,"0x506b4b"},
    {56,72,"0x506b58"},
    {128,72,"0x506b68"},
    {200,36,"0x506b78"},
    {236,36,"0x506b88"},
};
}
