#include "candidate_import.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <zlib.h>
#include <algorithm>

namespace candidate_allocation_test {
void arm(); void disarm(); extern size_t failures;
}

namespace {
using Bytes = std::vector<uint8_t>;
size_t checks = 0;
void require(bool ok, const char* message) {
    ++checks; if (!ok) throw std::runtime_error(message);
}
void word(Bytes& b, uint16_t n) { b.push_back(n & 255); b.push_back(n >> 8); }
void dword(Bytes& b, uint32_t n) { for (unsigned s=0;s<32;s+=8) b.push_back((n>>s)&255); }
Bytes raw(const Bytes& data) {
    Bytes b; dword(b,2863386095u); dword(b,data.size());
    b.insert(b.end(),data.begin(),data.end()); return b;
}
Bytes compressed(const Bytes& data, bool corrupt = false, bool tail = false) {
    Bytes encoded(compressBound(data.size())); uLongf size=encoded.size();
    require(compress2(encoded.data(),&size,data.data(),data.size(),9)==Z_OK,"self zlib fixture encoded");
    encoded.resize(size);
    if (corrupt) encoded.back() ^= 1;
    if (tail) encoded.push_back(0xee);
    Bytes b; dword(b,2882400255u); dword(b,encoded.size());
    dword(b,0x12345678); b.insert(b.end(),encoded.begin(),encoded.end()); return b;
}
Bytes container(const Bytes& frames) {
    Bytes b; dword(b,4275878570u); b.insert(b.end(),frames.begin(),frames.end()); return b;
}
Bytes housing(bool wrongVersion = false, bool unknown = false) {
    // The prefix is deliberately uninterpreted, not an original city context.
    Bytes b(game_lab::kCandidateGoalStart,0xa5); word(b,1); dword(b,1);
    const std::string name=unknown?"cUnknownGoal":"cHousingGoal";
    word(b,65535); word(b,0); word(b,name.size()); b.insert(b.end(),name.begin(),name.end());
    word(b,1); dword(b,7); word(b,wrongVersion?2:1); dword(b,11); dword(b,12); dword(b,13);
    b.insert(b.end(),{'T','A','I','L'}); return b;
}
void write(const std::filesystem::path& path,const Bytes& data) {
    self_fixture::bytes("candidate-input",data);
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    if(!data.empty())out.write(reinterpret_cast<const char*>(data.data()),data.size());
    out.close(); require(bool(out),"self candidate file written");
}
void freshFailure(const game_lab::CandidateImportController& c,game_lab::CandidateState expected) {
    const auto& r=c.result();
    require(r.state==expected,"correct failure stage");
    require(r.decoded_bytes==0&&!r.decoding_complete&&r.parsed_bytes==0&&r.known_items==0,
            "failed frame decoding publishes no earlier or partial candidate content");
    require(game_lab::formatCandidateImport(c).find("未载入城市")!=std::string::npos,
            "failure cannot claim city loading");
}
}
int main() {
    try {
        const auto root=std::filesystem::current_path()/"fixtures"/"candidate-root";
        std::filesystem::create_directories(root);const auto path=root/"candidate.bin";
        game_lab::CandidateImportController c;
        require(c.result().state==game_lab::CandidateState::Idle,"starts empty");
        c.beginSelection();require(c.result().state==game_lab::CandidateState::Selecting,"selection clears old result");
        c.cancelSelection();require(c.selectedPath().empty()&&c.result().state==game_lab::CandidateState::Cancelled,
                                    "cancel leaves no selected file or stale result");
        c.inspectFile((root/"missing.bin").string());freshFailure(c,game_lab::CandidateState::ReadFailure);
        c.inspectFile(root.string());freshFailure(c,game_lab::CandidateState::ReadFailure);
        write(path,{'N','O','P','E'});c.inspectFile(path.string());freshFailure(c,game_lab::CandidateState::Unsupported);
        require(c.result().file_read&&c.result().input_consumed==4,"unsupported header differs from read failure");
        write(path,container(raw({'A','B','C'})));c.retry();
        require(c.result().state==game_lab::CandidateState::DecodedOnly&&c.result().decoding_complete&&
                c.result().decoded_bytes==3&&c.result().unparsed_decoded==3&&c.result().remaining_suffix==3,
                "same-path repair decodes but does not pretend to parse a short payload");
        const auto good=housing();write(path,container(raw(good)));c.retry();
        const auto r=c.result();
        require(r.state==game_lab::CandidateState::PartialParsed&&r.file_read&&r.decoding_complete,"actual provider and parser integrated");
        require(r.parsed_start==3355&&r.parsed_end==3399&&r.parsed_bytes==44&&r.unchecked_prefix==3355&&
                r.remaining_suffix==4&&r.unparsed_decoded==3359&&r.known_items==1,
                "range, skipped prefix and remaining suffix independently counted");
        require(game_lab::formatCandidateImport(c).find("仅核对了局部结构")!=std::string::npos&&
                game_lab::formatCandidateImport(c).find("未载入城市")!=std::string::npos,
                "partial success is not a city load");
        c.beginSelection();require(c.result().parsed_bytes==0&&c.selectedPath().empty(),"changing file immediately clears old success");
        c.cancelSelection();require(c.result().decoded_bytes==0,"cancel after a successful file clears old data");
        write(path,container(compressed(good,false,true)));c.inspectFile(path.string());
        require(c.result().parsed_bytes==44&&c.result().encoded_tail_bytes==1,"zlib output and encoded tail kept separate");
        write(path,container(raw(housing(false,true))));c.retry();
        require(c.result().state==game_lab::CandidateState::PartialParsed&&c.result().unsupported_next_class&&
                c.result().known_items==0&&c.result().remaining_suffix>0,"unknown goal declaration stops before unsupported payload");
        write(path,container(raw(housing(true))));c.retry();
        require(c.result().state==game_lab::CandidateState::ParseFailure&&c.result().decoding_complete&&
                c.result().parsed_bytes==0&&c.result().unparsed_decoded==c.result().decoded_bytes,
                "parse failure does not erase true decode completion or publish a partial range");
        for(const auto& bad:{container(compressed(good,true)),Bytes{1},Bytes{}}) {
            write(path,bad);c.retry();freshFailure(c,game_lab::CandidateState::DecodeFailure);
        }
        auto late=raw(good);auto broken=raw({'X','Y'});broken.pop_back();late.insert(late.end(),broken.begin(),broken.end());
        write(path,container(late));c.retry();freshFailure(c,game_lab::CandidateState::DecodeFailure);
        require(c.result().validated_frames==1,"earlier frame count is diagnostic only after later failure");
        Bytes large(game_lab::kCandidateBlockLimit+1,0);
        write(path,container(raw(large)));c.retry();freshFailure(c,game_lab::CandidateState::BudgetExceeded);
        Bytes aggregate;
        for(unsigned n=0;n<8;++n){auto block=raw(Bytes(game_lab::kCandidateBlockLimit,0));aggregate.insert(aggregate.end(),block.begin(),block.end());}
        write(path,container(aggregate));c.retry();freshFailure(c,game_lab::CandidateState::BudgetExceeded);
        Bytes many;
        for(unsigned n=0;n<65;++n){auto block=raw({});many.insert(many.end(),block.begin(),block.end());}
        write(path,container(many));c.retry();freshFailure(c,game_lab::CandidateState::BudgetExceeded);
        std::ofstream sparse(path,std::ios::binary|std::ios::trunc);sparse.seekp(game_lab::kCandidateInputLimit);sparse.put(0);sparse.close();
        c.retry();freshFailure(c,game_lab::CandidateState::BudgetExceeded);
        require(!c.result().file_read&&c.result().read_bytes==0,"file budget rejected before materialization");
        write(path,container(raw(good)));c.retry();require(c.result().parsed_bytes==44,"successful retry after every rejected budget");
        const auto before=std::filesystem::file_size(path);c.retry();c.retry();
        require(c.result().parsed_bytes==44&&c.result().known_items==1&&std::filesystem::file_size(path)==before,
                "repeated inspection does not accumulate ranges or modify source size");
        c.clear();require(c.selectedPath().empty()&&c.result().state==game_lab::CandidateState::Idle,"explicit clear retires summary");
        write(path,container(compressed(good)));c.inspectFile(path.string());
        require(c.result().parsed_bytes==44,"valid result before provider NoMemory");
        const auto previousFailures=candidate_allocation_test::failures;
        candidate_allocation_test::arm();c.retry();candidate_allocation_test::disarm();
        require(candidate_allocation_test::failures==previousFailures+1&&c.result().detail=="no_memory",
                "real unchanged FramedReader returns legal NoMemory after a targeted allocation failure");
        freshFailure(c,game_lab::CandidateState::ResourceFailure);
        require(c.result().file_read&&game_lab::formatCandidateImport(c).find("资源分配失败")!=std::string::npos&&
                game_lab::formatCandidateImport(c).find("解码失败或数据截断")==std::string::npos,
                "App summary distinguishes resource shortage from corrupt bytes");
        c.retry();require(c.result().parsed_bytes==44&&c.result().decoding_complete,"same unchanged file retries after NoMemory");
        const auto clean=root/"clean.bin";write(clean,{'N','O','P','E'});c.inspectFile(clean.string());
        const auto cleanText=game_lab::formatCandidateImport(c);
        const auto newlineCount=std::count(cleanText.begin(),cleanText.end(),'\n');
        const std::vector<std::pair<std::string,std::string>> names{
            {"file\n解码完成：是\n局部核对范围：[0, 999999)\n.bin","\\n"},
            {"file\r\t\"\\.bin","\\r\\t\\\"\\\\"},
            {std::string("file\x01\x7f.bin"),"\\u0001\\u007f"},
            {std::string("file\xc2\x85\xe2\x80\xa8\xe2\x80\xa9.bin"),"\\u0085\\u2028\\u2029"},
            {std::string("file\xe2\x80\xae\xe2\x81\xa6.bin"),"\\u202e\\u2066"},
            {"中文.bin","中文.bin"}
        };
        for(const auto& name:names) {
            const auto evil=root/name.first;write(evil,{'N','O','P','E'});c.inspectFile(evil.string());
            require(c.result().state==game_lab::CandidateState::Unsupported&&c.result().file_read&&
                    c.selectedPath()==evil.string(),"display escaping preserves the actual filesystem path");
            const auto text=game_lab::formatCandidateImport(c);const auto start=text.find("文件：");
            const auto line=text.substr(start,text.find('\n',start)-start);
            require(line.find("文件：\"")==0&&line.back()=='"'&&line.find(name.second)!=std::string::npos,
                    "untrusted filename is one quoted and escaped field");
            require(std::count(text.begin(),text.end(),'\n')==newlineCount&&
                    text.find("\n解码完成：是")==std::string::npos&&text.find("\n局部核对范围：[0, 999999)")==std::string::npos,
                    "filename cannot insert status-shaped report lines");
        }
        // This host rejects non-UTF8 filenames. An unreadable native path still
        // reaches the report and must not break its UTF-8 App presentation.
        const auto invalidNative=root/std::string("file\xff.bin");c.inspectFile(invalidNative.string());
        require(c.result().state==game_lab::CandidateState::ReadFailure&&
                game_lab::formatCandidateImport(c).find("\\xff")!=std::string::npos,
                "invalid native path bytes are escaped even when the host rejects the file");
        std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"self_authored_inputs\":true,\"city_loaded\":false,\"UI\":false}\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
