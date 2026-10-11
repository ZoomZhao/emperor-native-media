#include "framed_reader.hpp"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
using namespace emperor::file_io;
using Bytes=std::vector<uint8_t>;
namespace allocations { bool watch=false; size_t calls=0, maximum=0, deny=SIZE_MAX; }
void *operator new(size_t n) {
    if (allocations::watch) {
        ++allocations::calls; allocations::maximum=std::max(allocations::maximum,n);
        if (n>=allocations::deny) throw std::bad_alloc();
    }
    if (void *p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc();
}
void *operator new[](size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p,size_t) noexcept { std::free(p); }
void operator delete[](void *p,size_t) noexcept { std::free(p); }
namespace {
int passed=0,failed=0;
void test(const std::string &id,bool ok,const char *source) {
    ok?++passed:++failed;
    std::cout<<"{\"id\":"<<std::quoted(id)<<",\"passed\":"<<(ok?"true":"false")
             <<",\"expectation_source\":"<<std::quoted(source)<<"}\n";
}
void start_watch(size_t deny=SIZE_MAX) {
    allocations::calls=allocations::maximum=0; allocations::deny=deny; allocations::watch=true;
}
void stop_watch() { allocations::watch=false; }
void word(Bytes &b,uint32_t n) { for(unsigned i=0;i<4;++i)b.push_back(static_cast<uint8_t>(n>>(8*i))); }
constexpr uint32_t HEAD=0x01020304,COMP=0x10203040,RAW=0x11223344;
FrameProfile profile() { return {HEAD,COMP,RAW,36144,36144,1<<20,1<<20,1024}; }
Bytes stored(const Bytes &plain) {
    // Own RFC-style zlib stored block construction, no mainline encoder oracle.
    Bytes b{0x78,0x01,0x01}; uint16_t n=static_cast<uint16_t>(plain.size());
    b.push_back(n&255);b.push_back(n>>8);n=static_cast<uint16_t>(~n);
    b.push_back(n&255);b.push_back(n>>8);b.insert(b.end(),plain.begin(),plain.end());
    uint32_t a=1,c=0;for(uint8_t x:plain){a=(a+x)%65521;c=(c+a)%65521;}
    uint32_t adler=(c<<16)|a;for(int i=3;i>=0;--i)b.push_back(static_cast<uint8_t>(adler>>(i*8)));
    return b;
}
Bytes frame(const Bytes &payload,bool compressed=false,uint32_t opaque=0xfedcba98) {
    Bytes b;word(b,compressed?COMP:RAW);word(b,static_cast<uint32_t>(payload.size()));
    if(compressed)word(b,opaque);b.insert(b.end(),payload.begin(),payload.end());return b;
}
class CountBuffer:public std::streambuf {
public:
    explicit CountBuffer(const Bytes &bytes):bytes_(bytes) {}
    size_t delivered=0,requests=0,peeks=0;
protected:
    std::streamsize xsgetn(char *p,std::streamsize n) override {
        ++requests;size_t k=std::min(static_cast<size_t>(n),bytes_.size()-delivered);
        if(k)std::memcpy(p,bytes_.data()+delivered,k);delivered+=k;return static_cast<std::streamsize>(k);
    }
    int_type underflow() override {
        ++peeks;return delivered==bytes_.size()?traits_type::eof():traits_type::to_int_type(bytes_[delivered]);
    }
private:const Bytes &bytes_;
};
bool same(const FrameResult &a,const FrameResult &b) {
    return a.status==b.status&&a.compressed==b.compressed&&a.opaque_word==b.opaque_word&&
      a.frame_start==b.frame_start&&a.input_offset==b.input_offset&&a.blocks_completed==b.blocks_completed&&
      a.total_output==b.total_output&&a.output_available==b.output_available&&
      a.codec.status==b.codec.status&&a.codec.codec_status==b.codec.codec_status&&
      a.codec.input_consumed==b.codec.input_consumed&&a.codec.output_produced==b.codec.output_produced&&
      a.codec.output_available==b.codec.output_available&&a.codec.trailing_input==b.codec.trailing_input;
}
void allocation_rejection(const char *id,Bytes data,FrameProfile p,ReadStatus expected,size_t delivered) {
    CountBuffer buf(data);std::istream source(&buf);FramedReader reader(source,p,StartPosition::RequiredFrame);Bytes output{9,8,7};auto before=output;
    start_watch();auto r=reader.next(output);stop_watch();auto calls=allocations::calls;
    auto reads=buf.requests;auto repeated=reader.next(output);
    test(id,r.status==expected&&buf.delivered==delivered&&calls==0&&same(r,repeated)&&buf.requests==reads&&output==before,
         "file_io/README.md: declared input/output budgets reject before payload allocation/read; failure sticky and caller output unchanged");
}
void resource_tests() {
    Bytes huge;word(huge,RAW);word(huge,UINT_MAX);auto p=profile();
    allocation_rejection("allocation/declared-U32MAX-before-payload",huge,p,ReadStatus::InputLimit,8);
    auto raw=frame(Bytes(9,1));p.max_input_bytes=8;
    allocation_rejection("allocation/aggregate-input-before-payload",raw,p,ReadStatus::InputLimit,8);
    p=profile();p.max_output_bytes=8;
    allocation_rejection("allocation/raw-output-before-payload",raw,p,ReadStatus::OutputLimit,8);
    auto zip=frame(stored(Bytes{1,2}),true);p=profile();p.max_input_bytes=12;
    allocation_rejection("allocation/compressed-input-budget",zip,p,ReadStatus::InputLimit,12);
    p=profile();p.compressed_marker=p.raw_marker;
    allocation_rejection("profile/duplicate-markers-no-read",raw,p,ReadStatus::InvalidProfile,0);
    p=profile();p.max_encoded_block=static_cast<size_t>(UINT_MAX)+1;
    allocation_rejection("profile/U32-input-no-read",raw,p,ReadStatus::InvalidProfile,0);
    p=profile();p.max_decoded_block=UINT_MAX;
    allocation_rejection("profile/U32-probe-output-no-read",raw,p,ReadStatus::InvalidProfile,0);
    auto encoded=stored(Bytes{1,2,3});Bytes destination(8,0xa5);auto before=destination;
    start_watch();auto limit=emfr_decode_zlib(encoded.data(),encoded.size(),destination.data(),destination.size(),{encoded.size()-1,8});stop_watch();
    test("adapter/input-resource-limit-no-allocation",limit.status==EMFR_INPUT_LIMIT&&allocations::calls==0&&destination==before&&limit.output_available==0,
         "strict_zlib.h trusted resource limit and transactional destination: reject source_size above max_encoded before scratch");
    start_watch(9);auto oom=emfr_decode_zlib(encoded.data(),encoded.size(),destination.data(),8,{100,8});stop_watch();
    test("adapter/private-scratch-bad_alloc",oom.status==EMFR_NO_MEMORY&&destination==before&&oom.output_available==0,
         "emfr_status EMFR_NO_MEMORY and strict_zlib.h failure-output transaction; controlled allocation failure, not real memory exhaustion");
    CountBuffer buf(raw);std::istream input(&buf);FramedReader reader(input,profile(),StartPosition::RequiredFrame);Bytes out{4,5};auto old=out;
    start_watch(9);auto r=reader.next(out);stop_watch();auto count=buf.requests;auto repeat=reader.next(out);
    test("reader/encoded-allocation-failure-sticky",r.status==ReadStatus::NoMemory&&r.output_available==0&&buf.delivered==8&&out==old&&same(r,repeat)&&count==buf.requests,
         "FramedReader NoMemory/status plus README failure sticky, no caller publication; injected encoded allocation failure");
    auto good=emfr_decode_zlib(encoded.data(),encoded.size(),destination.data(),8,{100,8});
    test("adapter/retry-after-allocation-failure",good.status==EMFR_OK&&good.output_available==3&&std::equal(destination.begin(),destination.begin()+3,Bytes{1,2,3}.begin()),
         "strict_zlib C adapter has per-call session, failure must not poison later valid calls");
}
void codec_properties() {
    uint32_t seed=0x9abcdef0;
    for(size_t n: {size_t(0),size_t(1),size_t(2),size_t(23),size_t(255),size_t(36144)}) {
        Bytes plain(n);for(auto &x:plain){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;x=seed&255;}
        auto encoded=stored(plain);Bytes dst(n+16,0xa5);
        auto r=emfr_decode_zlib(encoded.data(),encoded.size(),dst.data()+8,n,{65536,n});
        bool bytes=std::equal(plain.begin(),plain.end(),dst.begin()+8);
        bool guards=std::all_of(dst.begin(),dst.begin()+8,[](uint8_t x){return x==0xa5;})&&std::all_of(dst.end()-8,dst.end(),[](uint8_t x){return x==0xa5;});
        test("stored/exact-capacity/n="+std::to_string(n),r.status==EMFR_OK&&r.input_consumed==encoded.size()&&r.output_available==n&&bytes&&guards,
             "hand-constructed complete zlib stored block; plaintext, capacity and guards independent of reader; exact empty/capacity acceptance from strict_zlib.h");
        if(n) {
            std::fill(dst.begin(),dst.end(),0xa5);auto before=dst;
            auto shortcap=emfr_decode_zlib(encoded.data(),encoded.size(),dst.data()+8,n-1,{65536,n-1});
            test("stored/one-byte-over-capacity/n="+std::to_string(n),shortcap.status==EMFR_OUTPUT_LIMIT&&shortcap.output_available==0&&dst==before,
                 "strict_zlib.h output resource bound and all-failure transaction; generated exact plaintext length exceeds chosen limit by1");
        }
    }
    Bytes plain(40,'Q');auto encoded=stored(plain);Bytes dst(64,0xa5),second=stored(Bytes{7,8});
    auto withtail=encoded;withtail.insert(withtail.end(),second.begin(),second.end());
    auto r=emfr_decode_zlib(withtail.data(),withtail.size(),dst.data(),64,{1000,64});
    test("codec/second-member-is-tail",r.status==EMFR_OK&&r.output_available==plain.size()&&r.input_consumed==encoded.size()&&r.trailing_input==second.size()&&std::equal(plain.begin(),plain.end(),dst.begin()),
         "strict_zlib.h decodes only first stream and reports trailing bytes, never combines second member");
    auto bad=encoded;bad.back()^=0x80;std::fill(dst.begin(),dst.end(),0xa5);auto old=dst;
    auto error=emfr_decode_zlib(bad.data(),bad.size(),dst.data(),64,{1000,64});
    test("codec/adler-error-after-private-prefix",error.status==EMFR_INVALID_DATA&&error.output_produced==plain.size()&&error.output_available==0&&dst==old,
         "valid stored plaintext then deliberately wrong Adler: prefix diagnostic allowed but destination never published on failure");
    Bytes dict{0x78,0x20,0x12,0x34,0x56,0x78,0x00,0x00,0x00};
    auto fdict=emfr_decode_zlib(dict.data(),dict.size(),dst.data(),64,{1000,64});
    test("codec/FDICT-consumes-header-and-ID",fdict.status==EMFR_DICTIONARY_REQUIRED&&fdict.codec_status==2&&fdict.input_consumed==6&&fdict.trailing_input==3&&fdict.output_available==0&&dst==old,
         "synthetic FDICTheader FCHECK-valid plus4-byte dictionaryID; consumed source_size-avail_in, not host total_in");
    auto repeat=emfr_decode_zlib(encoded.data(),encoded.size(),dst.data(),64,{1000,64});
    test("codec/retry-after-invalid-data-and-dictionary",repeat.status==EMFR_OK&&repeat.output_available==40&&std::equal(plain.begin(),plain.end(),dst.begin()),
         "independent C calls own session; failure isolation and later valid output");
    // Input/output overlap is valid for this transactional adapter: it finishes
    // reading into independent scratch before committing destination.
    for(size_t offset:{size_t(0),size_t(16),size_t(20),size_t(46)}) {
        Bytes memory(128,0xa5);std::copy(encoded.begin(),encoded.end(),memory.begin()+16);auto before=memory;
        auto a=emfr_decode_zlib(memory.data()+16,encoded.size(),memory.data()+offset,40,{1000,40});
        bool outside=true;for(size_t i=0;i<memory.size();++i)if(i<offset||i>=offset+40)outside&=memory[i]==before[i];
        test("alias/adapter-success-offset="+std::to_string(offset),a.status==EMFR_OK&&a.input_consumed==encoded.size()&&a.output_available==40&&std::equal(plain.begin(),plain.end(),memory.begin()+offset)&&outside,
             "strict_zlib.h valid input/output slices and all-private decode; overlapping caller slices checked for exact output and unchanged outside region");
        std::copy(bad.begin(),bad.end(),memory.begin()+16);before=memory;
        auto fail=emfr_decode_zlib(memory.data()+16,bad.size(),memory.data()+offset,40,{1000,40});
        test("alias/adapter-failure-offset="+std::to_string(offset),fail.status==EMFR_INVALID_DATA&&fail.output_available==0&&memory==before,
             "strict_zlib.h every failure leaves destination unchanged; overlap failure additionally preserves source and guards");
    }
}
void reader_state_tests() {
    Bytes a{0x12,0x34},b{0x56};auto data=frame(a);auto empty=frame(stored({}),true);auto last=frame(b);
    data.insert(data.end(),empty.begin(),empty.end());data.insert(data.end(),last.begin(),last.end());
    CountBuffer buf(data);std::istream stream(&buf);auto p=profile();p.max_input_bytes=data.size();p.max_output_bytes=3;p.max_blocks=3;
    FramedReader reader(stream,p,StartPosition::RequiredFrame);Bytes out{0xa5};auto one=reader.next(out);bool first=one.status==ReadStatus::Frame&&out==a;
    auto two=reader.next(out);bool middle=two.status==ReadStatus::Frame&&out.empty()&&two.output_available==0&&two.blocks_completed==2;
    auto three=reader.next(out);bool third=three.status==ReadStatus::Frame&&out==b&&three.total_output==3&&three.input_offset==data.size();
    auto end=reader.next(out);auto reads=buf.requests,peeks=buf.peeks;stream.clear();out={9,8,7};auto repeated=reader.next(out);
    test("stream/adjacent-raw-empty-zlib-raw-exact-budgets",first&&middle&&third&&end.status==ReadStatus::End&&same(end,repeated)&&out==Bytes({9,8,7})&&buf.requests==reads&&buf.peeks==peeks,
         "README frame-event policy: emptyFrame notEOF, aggregate counts exact, reuse previous output, End sticky even after caller clearsstream");
    auto goodbad=frame(a);auto bad=stored(b);bad.back()^=1;auto next=frame(bad,true);goodbad.insert(goodbad.end(),next.begin(),next.end());
    CountBuffer badbuf(goodbad);std::istream badsource(&badbuf);FramedReader badreader(badsource,profile(),StartPosition::RequiredFrame);out={0xa5};auto success=badreader.next(out);auto keep=out;
    auto failure=badreader.next(out);reads=badbuf.requests;badsource.clear();auto sticky=badreader.next(out);
    test("stream/late-failure-preserves-prior-output-vector",success.status==ReadStatus::Frame&&failure.status==ReadStatus::CodecError&&failure.total_output==a.size()&&failure.blocks_completed==1&&failure.output_available==0&&out==keep&&same(failure,sticky)&&badbuf.requests==reads,
         "README blocktransaction scope: already-delivered prior frame retained, currentbadblock never published, failure no retry reads");
    auto twoblocks=frame(a);auto another=frame(b);twoblocks.insert(twoblocks.end(),another.begin(),another.end());
    CountBuffer limitsbuf(twoblocks);std::istream limited(&limitsbuf);p=profile();p.max_output_bytes=2;
    FramedReader limitedreader(limited,p,StartPosition::RequiredFrame);out={0xa5};limitedreader.next(out);keep=out;
    start_watch();auto over=limitedreader.next(out);stop_watch();
    test("stream/aggregate-output-limit-before-second-payload",over.status==ReadStatus::OutputLimit&&over.total_output==2&&over.blocks_completed==1&&limitsbuf.delivered==frame(a).size()+8&&allocations::calls==0&&out==keep,
         "README aggregate output budget: next rawN above remaining rejectedbeforeallocation, previouslycommitted output retained");
    for(bool headerless:{false,true}) {
        Bytes none;if(!headerless)word(none,HEAD);CountBuffer eb(none);std::istream es(&eb);p=profile();p.max_output_bytes=0;p.max_blocks=0;p.max_input_bytes=none.size();
        FramedReader er(es,p,headerless?StartPosition::RequiredFrame:StartPosition::ContainerHeader);out={3};auto rr=er.next(out);
        test(std::string("stream/empty-")+(headerless?"required-frame":"container"),rr.status==(headerless?ReadStatus::InputLimit:ReadStatus::End)&&out==Bytes{3},
             "README RequiredFrame requiresfirstmarker; maxinput0 rejectsrequired4 beforeEOF classification, qualifiedheader permitszeroblocks; budget precedence intentional");
    }
    Bytes four;word(four,RAW);for(size_t n=0;n<4;++n){Bytes prefix(four.begin(),four.begin()+n);CountBuffer cb(prefix);std::istream in(&cb);FramedReader pr(in,profile(),StartPosition::RequiredFrame);out={1};auto rr=pr.next(out);
      test("stream/first-marker-truncation="+std::to_string(n),rr.status==ReadStatus::Truncated&&rr.input_offset==n&&out==Bytes{1},
           "README requiredmarker shortincludingempty =>Truncated when inputbudget allowsrequest, not End");}
    p=profile();p.max_input_bytes=p.max_output_bytes=p.max_blocks=UINT64_MAX;auto raw=frame(a);CountBuffer cb(raw);std::istream in(&cb);FramedReader ur(in,p,StartPosition::RequiredFrame);out={1};auto rr=ur.next(out);
    test("stream/U64-max-profile-small-counters",rr.status==ReadStatus::Frame&&rr.total_output==2&&rr.input_offset==raw.size()&&out==a,
         "FrameProfile uint64 budgets must notnarrowtosigned; small actual counters only, doesnotclose unavailable nearU64 external-counter API");
}
}
int main() {
    resource_tests();codec_properties();reader_state_tests();
    std::cout<<"{\"summary\":true,\"passed\":"<<passed<<",\"failed\":"<<failed
             <<",\"zlib_version\":"<<std::quoted(emfr_zlib_runtime_version())<<"}\n";
    return failed?1:0;
}
