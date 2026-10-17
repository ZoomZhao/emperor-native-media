#include "original_random.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {using Bytes=std::vector<uint8_t>;size_t checks=0;void need(bool x,const char*s){++checks;if(!x)throw std::runtime_error(s);}
// Emitted state of FUN_004189B0 @0x004189B0 (Emperor[CH].exe sha256 0ca8fc07...c658).
// Fresh generator = seeds 0x010C714C=0x54657687 / 0x010C7148=0x72641663 from
// FUN_0052B590 @0x0052B5F8-0x0052B60C; one output = low byte of 0x010C7138 (state&0x7FFF,
// 0x00418A28/0x00418A36) after the 31 steps at 0x004189E6-0x00418A22.
const Bytes fresh{0xef,0xf1,0xce,0xe2,0xcc,0x40,0x44,0x70,0x17,0x96,0xbf,0x84,0xec,0xe2,0x7c,0x7b};
void generatorTests(){
 game_lab::OriginalCellRandom fresh0;
 for(size_t i=0;i<fresh.size();++i)need(fresh0.next()==fresh[i],"first 16 outputs of a fresh generator match the disassembled state transition");
 need(fresh0.streamIndex()==fresh.size(),"streamIndex counts every consumed draw");
 game_lab::OriginalCellRandom one;
 need(one.next()==0xef,"fresh first draw byte");
 need(one.rawState()==0x292321efu,"0x010C714C after one 31-step call");
 need(one.rawSecondState()==0x5d425705u,"0x010C7148 after one 31-step call");
 need(one.secondField()==0x5705u&&one.secondField()==(one.rawSecondState()&0x7fffu),"0x010C712C is the second register masked 0x7FFF @0x00418A54");
 game_lab::OriginalCellRandom a,b;
 auto first=a.fillGrid(),second=b.fillGrid();
 need(first.size()==game_lab::kOriginalGridCells&&first.size()==51984,"fillGrid returns 228*228 bytes @0x004B0AC3-0x004B0AE4");
 need(first==second,"two fills of a fresh generator are byte-identical");
 need(second.size()==51984&&std::equal(fresh.begin(),fresh.begin()+8,first.begin()),"fill order is cell 0..51983 with one draw per byte @0x004B0ACF-0x004B0ADF");
 need(a.streamIndex()==51984&&b.streamIndex()==51984,"one fill consumes exactly 51984 draws");
 game_lab::OriginalCellRandom c;c.advance(100);c.reset();
 need(c.fillGrid()==first&&c.streamIndex()==51984,"reset returns to the executable's initial state @0x0052B5F8");
 game_lab::OriginalCellRandom positioned(2341);
 need(positioned.rawState()==0x339e16dfu,"0x010C714C after 2341 draws");
 need(positioned.next()==0xb2,"FUN_004B0AC0 @0x004B0AC0 first grid byte at stream index 2341, as persisted by real saves (SAV random field, FUN_0052FDA0 @0x00530770)");
 need(positioned.rawState()==0x20a7f7b2u,"0x010C714C after 2342 draws");
}
void gridTests(){
 game_lab::OriginalCellRandom generator;
 auto grid=generator.fillGrid();
 std::array<size_t,256> histogram{};for(uint8_t v:grid)++histogram[v];
 size_t distinct=0;for(size_t n:histogram)distinct+=n!=0;
 need(distinct==256,"the 51984-byte grid covers every byte value");
 size_t longest=1,run=1;for(size_t i=1;i<grid.size();++i){run=grid[i]==grid[i-1]?run+1:1;longest=std::max(longest,run);}
 need(longest<=4,"no degenerate equal-byte run in the grid");
 need(histogram[0]==210&&histogram[255]==207,"measured byte counts at the distribution tails");
 std::array<size_t,8> low3{};for(uint8_t v:grid)++low3[v&7];
 need(*std::min_element(low3.begin(),low3.end())>6000&&*std::max_element(low3.begin(),low3.end())<7000,"random&7 (FUN_004B3970 @0x004B39A8, FUN_005AD663 @0x005AD663) is near-uniform");
 std::array<size_t,24> mod24{};for(uint8_t v:grid)++mod24[v%0x18];
 for(size_t r=0;r<16;++r)need(mod24[r]>2100,"random%0x18 residue 0..15 covers 11 of 256 values");
 for(size_t r=16;r<24;++r)need(mod24[r]<2150&&mod24[r]>1900,"random%0x18 residue 16..23 covers 10 of 256 values");
 std::array<size_t,3> mod3{};for(uint8_t v:grid)++mod3[v%3];
 need(*std::min_element(mod3.begin(),mod3.end())>16500,"random%3 residues stay within one percent");
 need(grid.back()==0x5c,"last grid byte at cell 51983");
 need(std::count(grid.begin(),grid.end(),grid.front())==200,"first byte 0xEF occurs at the uniform 1/256 rate");
}
}
int main(){try{
 generatorTests();
 gridTests();
 std::cout<<"{\"passed\":true,\"assertions\":"<<checks<<"}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
