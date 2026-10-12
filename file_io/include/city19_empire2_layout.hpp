#pragma once
#include <cstddef>
#include <cstdint>
namespace emperor::file_io::research {
struct Empire2Field{uint32_t receiver_offset;size_t bytes;const char *source_call;};
inline constexpr Empire2Field empire2_fields[]={
    {4,1,"0x480485"},
    {5,1,"0x480492"},
    {6,1,"0x48049f"},
    {7,1,"0x4804ac"},
    {8,2,"0x4804b9"},
    {10,2,"0x4804c6"},
    {12,2,"0x4804d3"},
    {14,2,"0x4804e0"},
    {16,2,"0x4804ed"},
    {18,2,"0x4804fa"},
    {20,1,"0x480507"},
    {21,1,"0x480514"},
    {22,1,"0x480521"},
    {23,1,"0x48052e"},
    {24,2,"0x48053b"},
    {26,2,"0x480548"},
    {28,1,"0x480555"},
    {29,1,"0x480562"},
    {32,4,"0x48056f"},
    {36,4,"0x48057c"},
    {40,4,"0x48059d"},
};
}
