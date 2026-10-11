#include "framed_reader.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <streambuf>
#include <vector>
using namespace emperor::file_io;
namespace {void *borrowed=nullptr;bool released=false,prevented=false;}
void*operator new(size_t n){if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void*operator new[](size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{if(p==borrowed)released=true;std::free(p);}
void operator delete[](void*p)noexcept{::operator delete(p);}
void operator delete(void*p,size_t)noexcept{::operator delete(p);}
void operator delete[](void*p,size_t)noexcept{::operator delete(p);}
void word(std::vector<uint8_t>&v,uint32_t x){for(int i=0;i<4;++i)v.push_back(x>>(8*i));}
class BorrowedVector:public std::streambuf {
public:explicit BorrowedVector(std::vector<uint8_t>&v){char*p=reinterpret_cast<char*>(v.data());setg(p,p,p+v.size());}
protected:std::streamsize xsgetn(char*p,std::streamsize n)override{
    // Observe the deallocation event, then stop before any expired memory read.
    if(released){prevented=true;throw std::runtime_error("borrowed input storage was freed byoutputcommit");}
    return std::streambuf::xsgetn(p,n);
}
};
int main(){
    // Two wholly synthetic raw frames. At reviewed16b4c8c the public contract lacked
    // this premise; now it is explicit. This negativeprobe is unsupported use.
    std::vector<uint8_t> data;word(data,3);word(data,1);data.push_back('A');word(data,3);word(data,1);data.push_back('B');
    borrowed=data.data();
    BorrowedVector buffer(data);std::istream input(&buffer);
    FramedReader reader(input,{1,2,3,32,32,128,128,8},StartPosition::RequiredFrame);
    auto first=reader.next(data);assert(first.status==ReadStatus::Frame&&data==std::vector<uint8_t>{'A'});
    const bool afterfirst=released;
    auto second=reader.next(data); // Guard detects freed borrowed storage first.
    std::cout<<"{\"borrowed_storage_freed_after_first\":"<<(afterfirst?"true":"false")
             <<",\"expired_read_prevented\":"<<(prevented?"true":"false")
             <<",\"second_status\":\""<<read_status_name(second.status)<<"\"}\n";
    return afterfirst&&prevented&&second.status==ReadStatus::IoError?0:1;
}
