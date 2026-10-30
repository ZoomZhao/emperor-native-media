// SPDX-License-Identifier: MIT
#ifndef ORIGINAL_WORLD_BINDING_IMPLEMENTATION
#define ORIGINAL_WORLD_BINDING_IMPLEMENTATION
#include "original_world_checkpoint_binding.hpp"
#include "original_evidence_figures.hpp"
#include "../file_io/include/city19_goal_archive.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <filesystem>
namespace game_lab {
inline OriginalWorldDigest originalWorldDigest(const void* bytes,size_t size){CC_SHA256_CTX ctx;CC_SHA256_Init(&ctx);const auto* p=static_cast<const uint8_t*>(bytes);while(size){const auto n=CC_LONG(std::min(size,size_t(1u<<20)));CC_SHA256_Update(&ctx,p,n);p+=n;size-=n;}OriginalWorldDigest out;CC_SHA256_Final(out.data(),&ctx);return out;}
inline bool originalWorldRelative(const std::string& path){if(path.empty()||path.size()>4096||path.find('\0')!=std::string::npos||path.find('\\')!=std::string::npos)return false;const std::filesystem::path p(path);if(p.is_absolute())return false;for(const auto& c:p)if(c=="."||c==".."||c.empty())return false;return true;}
namespace original_world_binding_detail {
struct FD{int n=-1;explicit FD(int value):n(value){}~FD(){if(n>=0)::close(n);}FD(const FD&)=delete;};
inline bool same(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_size==b.st_size&&a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec;}
inline std::optional<OriginalWorldFileBinding> file(const ResourceRoot& root,const std::string& relative,std::string& error,std::vector<uint8_t>* copy=nullptr){
 if(!originalWorldRelative(relative)){error="world_binding_relative_path";return std::nullopt;}const auto path=root.resolve(relative);if(path.empty()){error="world_binding_missing_file";return std::nullopt;}
 // Walk the named relative path through directory descriptors. Root::resolve
 // alone canonicalizes symlinks and therefore cannot enforce this boundary.
 FD parent(::open(root.configuredDirectory().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
 const std::filesystem::path rel(relative);auto it=rel.begin();std::string leaf;
 if(parent.n<0){error="world_binding_root_open";return std::nullopt;}
 for(;it!=rel.end();++it){auto next=it;++next;if(next==rel.end()){leaf=it->string();break;}
  const int directory=::openat(parent.n,it->c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
  if(directory<0){error="world_binding_directory";return std::nullopt;}::close(parent.n);parent.n=directory;
 }
 FD fd(::openat(parent.n,leaf.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW));struct stat before{},after{},named{};
 if(fd.n<0||fstat(fd.n,&before)||!S_ISREG(before.st_mode)||before.st_size<0||uint64_t(before.st_size)>uint64_t(2)*1024*1024*1024){error="world_binding_file_bounds";return std::nullopt;}
 if(copy&&before.st_size>4194304){error="world_binding_source_budget";return std::nullopt;}
 OriginalWorldFileBinding out;out.relative=relative;out.bytes=uint64_t(before.st_size);std::array<uint8_t,65536> buffer;std::array<OriginalWorldDigest,2> digests;
 if(copy)copy->clear();
 for(unsigned pass=0;pass<2;++pass){if(pass&&lseek(fd.n,0,SEEK_SET)<0){error="world_binding_seek";return std::nullopt;}CC_SHA256_CTX ctx;CC_SHA256_Init(&ctx);uint64_t count=0;
  for(;;){const auto n=::read(fd.n,buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;if(n<0){error="world_binding_read";return std::nullopt;}if(!n)break;count+=uint64_t(n);if(count>out.bytes){error="world_binding_changed";return std::nullopt;}CC_SHA256_Update(&ctx,buffer.data(),CC_LONG(n));if(!pass&&copy)copy->insert(copy->end(),buffer.begin(),buffer.begin()+n);}
  CC_SHA256_Final(digests[pass].data(),&ctx);if(count!=out.bytes){error="world_binding_changed";return std::nullopt;}}
 if(fstat(fd.n,&after)||fstatat(parent.n,leaf.c_str(),&named,AT_SYMLINK_NOFOLLOW)||!same(before,after)||!same(before,named)||digests[0]!=digests[1]){error="world_binding_changed";return std::nullopt;}
 out.digest=digests[0];error.clear();return out;
}
inline std::vector<std::string> resources(){std::vector<std::string> out{"Model/EmperorBuildingModels.txt","Model/EmperorFigureModels.txt","EmperorText.eng"};for(const char* name:{"China_General","China_Terrain","China_Elevation","SprAmbient","Destruction","SprMain","SprMain2","SprMain3"})for(const char* suffix:{".sg3",".555"})out.emplace_back(std::string("DATA/")+name+suffix);return out;}
}
inline std::optional<OriginalWorldBoundSource> bindOriginalWorldSource(const ResourceRoot& root,const std::string& relative,std::string& error){
 if(!originalWorldRelative(relative)||relative.rfind("Save/",0)!=0||std::filesystem::path(relative).extension()!=".sav"){error="world_binding_source_path";return std::nullopt;}
 OriginalWorldBoundSource out;std::vector<uint8_t> bytes;const auto source=original_world_binding_detail::file(root,relative,error,&bytes);if(!source)return std::nullopt;
 out.save=decodeOriginalSave(bytes);const auto decoded=decodeOriginalMapContainer(bytes);
 if(out.save.status!=OriginalSaveStatus::LoadedSnapshot||!decoded.complete||!out.save.layers.figures||out.save.layers.figures->status!=OriginalFigureStatus::Complete||out.save.layers.figures->slots.size()!=2000){error="world_binding_incomplete_source";return std::nullopt;}
 emperor::file_io::research::GoalLimits limits;limits.enable_homage=limits.enable_trading=limits.enable_yearly=limits.enable_population=true;
 const auto goals=emperor::file_io::research::walk_city19_housing_goals({decoded.payload.data(),std::min(size_t(16384),decoded.payload.size())},limits);
 if(goals.completed_members!=10||goals.pending.present){error="world_binding_goal_prefix";return std::nullopt;}
 const auto pool=evidenceFigurePrefix(decoded.payload,out.save.buildings.tableEnd,out.save.terrainOffset,goals.cache.size()+4000+out.save.buildings.declaredClasses);
 if(!pool.complete||pool.completed!=2000){error="world_binding_unknown_fsa_or_class";return std::nullopt;}
 out.binding.source=*source;out.binding.legacyRevision=out.save.snapshotRevision;out.payload=decoded.payload;out.binding.payloadDigest=originalWorldDigest(out.payload.data(),out.payload.size());out.save.readStable=true;
 for(const auto& name:original_world_binding_detail::resources()){const auto item=original_world_binding_detail::file(root,name,error);if(!item)return std::nullopt;out.binding.resources.push_back(*item);}error.clear();return out;
}
inline bool verifyOriginalWorldBinding(const ResourceRoot& root,const OriginalWorldSourceBinding& binding,std::string& error){
 if(binding.schema!=1||binding.contract!="original-world-1"||!binding.legacyRevision||binding.source.relative.rfind("Save/",0)!=0||std::filesystem::path(binding.source.relative).extension()!=".sav"){error="world_binding_header";return false;}
 const auto names=original_world_binding_detail::resources();if(binding.resources.size()!=names.size()){error="world_binding_resource_set";return false;}
 auto verify=[&](const OriginalWorldFileBinding& f){const auto actual=original_world_binding_detail::file(root,f.relative,error);if(!actual)return false;if(actual->bytes!=f.bytes||actual->digest!=f.digest){error="world_binding_content_changed";return false;}return true;};
 if(!verify(binding.source))return false;for(size_t n=0;n<names.size();++n){if(binding.resources[n].relative!=names[n]){error="world_binding_resource_set";return false;}if(!verify(binding.resources[n]))return false;}error.clear();return true;
}
}
#endif
