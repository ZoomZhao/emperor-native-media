// SPDX-License-Identifier: MIT
#ifndef ORIGINAL_WORLD_IO_IMPLEMENTATION
#define ORIGINAL_WORLD_IO_IMPLEMENTATION
#include "original_world_checkpoint_io.hpp"
#include <atomic>
#include <chrono>
namespace game_lab { namespace original_world_io_detail {
inline bool outside(const std::filesystem::path& parent,const std::filesystem::path& data){auto p=parent.begin(),r=data.begin();for(;p!=parent.end()&&r!=data.end()&&*p==*r;++p,++r){}return r!=data.end();}
struct Parent {
 original_world_binding_detail::FD fd;std::filesystem::path canonical;std::string leaf;struct stat identity{};
 Parent():fd(-1){}
 bool current()const{struct stat named{};std::error_code ec;const auto actual=std::filesystem::canonical(canonical,ec);return !ec&&actual==canonical&&!stat(canonical.c_str(),&named)&&named.st_dev==identity.st_dev&&named.st_ino==identity.st_ino;}
};
inline std::unique_ptr<Parent> parent(const std::string& path,const std::string& root,std::string& error){
 const std::filesystem::path target(path);std::error_code ec;if(!target.is_absolute()||target.extension()!=".emworld"||target.filename().empty()){error="world_io_target_path";return {};}
 auto out=std::make_unique<Parent>();out->canonical=std::filesystem::canonical(target.parent_path(),ec);if(ec){error="world_io_parent";return {};}
 const auto data=std::filesystem::canonical(root,ec);if(ec||!outside(out->canonical,data)){error="world_io_data_root";return {};}
 for(const auto& part:out->canonical)if(part.extension()==".app"){error="world_io_package_path";return {};}
 out->leaf=target.filename().string();if(out->leaf.find('\0')!=std::string::npos){error="world_io_target_path";return {};}
 out->fd.n=::open(out->canonical.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);if(out->fd.n<0||fstat(out->fd.n,&out->identity)||!out->current()){error="world_io_parent_changed";return {};}return out;
}
}
inline OriginalWorldPublicationResult writeOriginalWorldCheckpointNew(const std::string& path,const std::string& dataRoot,const OriginalWorldSnapshot& s,OriginalWorldIOFault fault,const std::function<void(const char*)>& probe){
 OriginalWorldPublicationResult result;result.revision=s.commitRevision;std::string temp;std::unique_ptr<original_world_io_detail::Parent> p;
 try{
  const auto encoded=encodeOriginalWorldCheckpoint(s,result.error);if(!encoded)return result;result.bytes=encoded->size();result.digest=originalWorldDigest(encoded->data(),encoded->size());
  p=original_world_io_detail::parent(path,dataRoot,result.error);if(!p)return result;struct stat existing{};if(!fstatat(p->fd.n,p->leaf.c_str(),&existing,AT_SYMLINK_NOFOLLOW)||errno!=ENOENT){result.error="world_io_exists";return result;}
  if(fault==OriginalWorldIOFault::Create){result.error="world_io_create";return result;}
  static std::atomic<uint64_t> serial{1};int fd=-1;
  for(unsigned attempt=0;attempt<16;++attempt){temp=".emworld-tmp-"+std::to_string(getpid())+"-"+std::to_string(serial.fetch_add(1));fd=openat(p->fd.n,temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if(fd>=0)break;if(errno!=EEXIST)break;}
  if(fd<0){result.error="world_io_create";return result;}
  original_world_binding_detail::FD owner(fd);
  if(probe)probe("temp_created");
  bool good=true;size_t written=0;bool interrupted=false;
  while(written<encoded->size()){
   if(fault==OriginalWorldIOFault::Write){good=false;break;}
   if(fault==OriginalWorldIOFault::InterruptedWrite&&!interrupted){interrupted=true;continue;}
   const size_t count=fault==OriginalWorldIOFault::ShortWrite?std::min(size_t(17),encoded->size()-written):encoded->size()-written;
   const auto n=::write(fd,encoded->data()+written,count);if(n<0&&errno==EINTR)continue;if(n<=0){good=false;break;}written+=size_t(n);
  }
  if(probe)probe("bytes_written");
  if(good&&(fault==OriginalWorldIOFault::FileSync||fsync(fd)))good=false;
  if(probe)probe("file_synced");
  const auto closeResult=close(fd);owner.n=-1;if(closeResult||fault==OriginalWorldIOFault::Close)good=false;
  if(!good){unlinkat(p->fd.n,temp.c_str(),0);temp.clear();result.error="world_io_write_sync_close";return result;}
  if(probe)probe("before_publish");
  if(fault==OriginalWorldIOFault::CancelBeforePublish||fault==OriginalWorldIOFault::ParentChanged||!p->current()){unlinkat(p->fd.n,temp.c_str(),0);temp.clear();result.error="world_io_cancel_or_parent_changed";return result;}
  if(fault==OriginalWorldIOFault::Publish||linkat(p->fd.n,temp.c_str(),p->fd.n,p->leaf.c_str(),0)){unlinkat(p->fd.n,temp.c_str(),0);temp.clear();result.error="world_io_publish";return result;}
  result.state=OriginalWorldPublication::PublishedDurabilityUnknown;if(probe)probe("published");unlinkat(p->fd.n,temp.c_str(),0);temp.clear();
  if(fault==OriginalWorldIOFault::DirectorySync||fault==OriginalWorldIOFault::AfterPublish||!p->current()||fsync(p->fd.n)){result.error="world_io_published_durability_unknown";return result;}
  if(probe)probe("directory_synced");
  result.state=OriginalWorldPublication::PublishedDurable;result.error.clear();return result;
 }catch(const std::exception& e){if(p&&!temp.empty())unlinkat(p->fd.n,temp.c_str(),0);result.error=e.what();return result;}
}
inline std::optional<std::string> readOriginalWorldCheckpointFile(const std::string& path,const std::string& dataRoot,std::string& error){try{
 // O_NONBLOCK prevents an attacker-controlled FIFO from waiting for a writer
 // before fstat can enforce the regular-file gate. It has no effect on regular files.
 auto p=original_world_io_detail::parent(path,dataRoot,error);if(!p)return std::nullopt;original_world_binding_detail::FD fd(openat(p->fd.n,p->leaf.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW));struct stat before{},after{},named{};
 if(fd.n<0||fstat(fd.n,&before)||!S_ISREG(before.st_mode)||before.st_size<888||uint64_t(before.st_size)>originalWorldFileLimit){error="world_io_read_bounds";return std::nullopt;}
 std::string bytes(size_t(before.st_size),'\0');size_t done=0;while(done<bytes.size()){const auto n=::read(fd.n,bytes.data()+done,bytes.size()-done);if(n<0&&errno==EINTR)continue;if(n<=0){error="world_io_read";return std::nullopt;}done+=size_t(n);}
 if(lseek(fd.n,0,SEEK_SET)<0){error="world_io_read";return std::nullopt;}CC_SHA256_CTX ctx;CC_SHA256_Init(&ctx);std::array<uint8_t,65536> buffer;uint64_t total=0;
 for(;;){const auto n=::read(fd.n,buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;if(n<0){error="world_io_read";return std::nullopt;}if(!n)break;total+=uint64_t(n);if(total>bytes.size()){error="world_io_read_changed";return std::nullopt;}CC_SHA256_Update(&ctx,buffer.data(),CC_LONG(n));}
 OriginalWorldDigest second;CC_SHA256_Final(second.data(),&ctx);
 if(total!=bytes.size()||second!=originalWorldDigest(bytes.data(),bytes.size())||fstat(fd.n,&after)||fstatat(p->fd.n,p->leaf.c_str(),&named,AT_SYMLINK_NOFOLLOW)||!original_world_binding_detail::same(before,after)||!original_world_binding_detail::same(before,named)||!p->current()){error="world_io_read_changed";return std::nullopt;}
 error.clear();return bytes;
 }catch(const std::exception& e){error=e.what();return std::nullopt;}}
}
#endif
