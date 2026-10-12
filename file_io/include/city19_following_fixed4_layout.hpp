#pragma once
#include <cstddef>
#include <cstdint>
namespace emperor::file_io::research {
struct FollowingFixed4Field {bool temporary;uint32_t destination_offset;size_t bytes;const char *source_call;};
inline constexpr FollowingFixed4Field following_fixed4_fields[]={
    {false,0x8,4,"0x512004"},{false,0xc,4,"0x51200f"},{false,0x10,4,"0x51201a"},
    {true,0x12,1,"0x512025"},{false,0x2c,4,"0x512030"},{false,0x1c,4,"0x51203b"},
    {false,0x20,1,"0x512045"},{false,0x24,4,"0x512050"},{false,0x28,1,"0x51205a"},
    {false,0x14,1,"0x512069"},{false,0x15,1,"0x512069"},{false,0x16,1,"0x512069"},
    {false,0x17,1,"0x512069"},{false,0x18,1,"0x512069"},
};
}
