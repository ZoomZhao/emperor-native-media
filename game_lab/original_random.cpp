#include "original_random.hpp"
namespace game_lab {
namespace {
// FUN_004189B0 @0x004189EB-0x00418A1B: one step of the 31-bit reflected LFSR.
// XOR of bit4 and bit0 shifts into bit30; the state shifts right. The registers are
// seeded below 2^31 and bit31 is never written, so 31 bits are exact.
uint32_t step(uint32_t s){return (s>>1)|((((s>>4)^s)&1u)<<30);}
}
OriginalCellRandom::OriginalCellRandom(uint64_t streamIndex){
    reset();
    advance(streamIndex);
}
void OriginalCellRandom::reset(){
    state_=kSeed;
    second_=kSecondSeed;
    draws_=0;
}
uint64_t OriginalCellRandom::streamIndex() const{return draws_;}
void OriginalCellRandom::advance(uint64_t draws){while(draws--)next();}
uint8_t OriginalCellRandom::next(){
    // Loop count 0x1F at 0x004189E6/0x00418A21-0x00418A22: exactly 31 steps.
    for(int i=0;i<31;++i){
        state_=step(state_);
        second_=step(second_);
    }
    ++draws_;
    // 0x010C7138 = state&0x7FFF (0x00418A28); the byte store at 0x004B0AD9 reads its
    // low byte (0x004B0AD4, little-endian).
    return static_cast<uint8_t>(state_&0xFFu);
}
uint32_t OriginalCellRandom::secondField() const{return second_&0x7FFFu;}
uint32_t OriginalCellRandom::rawState() const{return state_;}
uint32_t OriginalCellRandom::rawSecondState() const{return second_;}
std::vector<uint8_t> OriginalCellRandom::fillGrid(){
    std::vector<uint8_t> grid(kOriginalGridCells);
    for(auto& cell:grid)cell=next();            // 0x004B0ACF-0x004B0AE4
    return grid;
}
}
