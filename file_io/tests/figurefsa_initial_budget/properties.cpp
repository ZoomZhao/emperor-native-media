#include "city19_figurefsa_objects.hpp"
#include <iostream>
using namespace emperor::file_io::research;
using Cache=std::vector<FigureFsaWireCacheEntry>;
std::vector<uint8_t> record(){std::vector<uint8_t>b;
    auto w=[&](uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));};
    w(2);b.insert(b.end(),5,0);w(0);w(0);w(0);b.insert(b.end(),4,0);w(2);
    for(unsigned i=0;i<3;++i){w(1);b.insert(b.end(),17,0);}return b;}
Cache cache(size_t prefix,size_t added){Cache c(prefix,{FigureFsaCacheKind::Class,FigureFsaWireType::OpaquePrefix,0,7});c[0]={};
    c.insert(c.end(),added,{FigureFsaCacheKind::Class,FigureFsaWireType::State,0,0});return c;}
bool equal(const Cache&a,const Cache&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].kind!=b[i].kind||a[i].family!=b[i].family||a[i].class_id!=b[i].class_id||a[i].schema!=b[i].schema)return false;return true;}
int main(){auto bytes=record();FigureFsaObjectLimits l;l.max_cache=8192;
    unsigned passed=0,failed=0;std::cout<<"{\"checks\":[";bool comma=false;
    auto report=[&](std::string id,bool ok){if(comma)std::cout<<',';comma=true;std::cout<<"{\"id\":\""<<id<<"\",\"passed\":"<<(ok?"true":"false")<<'}';ok?++passed:++failed;};
    for(size_t prefix:{1u,6020u}) {
        auto good=cache(prefix,1024);auto before=good;
        auto output=walk_city19_figurefsa_with_archive_prefix({bytes.data(),bytes.size()},FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10,good,prefix,0,l);
        report("initial1024_prefix"+std::to_string(prefix),bytes.size()==76&&output.end==76&&output.cache.size()==prefix+1024&&equal(good,before));
        auto bad=cache(prefix,1025);auto immutable=bad;bool rejected=false;
        try{(void)walk_city19_figurefsa_with_archive_prefix({bytes.data(),bytes.size()},FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10,bad,prefix,0,l);}catch(const std::runtime_error&){rejected=true;}
        report("initial1025_rejected_prefix"+std::to_string(prefix),rejected&&equal(bad,immutable));
        output.end=12345;const auto saved=output.cache;rejected=false;
        try{output=walk_city19_figurefsa_with_archive_prefix({bytes.data(),bytes.size()},FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10,bad,prefix,0,l);}catch(const std::runtime_error&){rejected=true;}
        report("initial_failure_atomic_prefix"+std::to_string(prefix),rejected&&output.end==12345&&equal(output.cache,saved)&&equal(bad,immutable));
    }
    std::cout<<"],\"passed\":"<<passed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
