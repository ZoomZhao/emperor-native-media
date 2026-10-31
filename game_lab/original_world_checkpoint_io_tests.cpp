// SPDX-License-Identifier: MIT
#include "original_world_checkpoint_test_fixture.hpp"
#include <fstream>
#include <sys/wait.h>
using namespace game_lab;using namespace game_lab::world_test;
int main(int argc,char** argv){try{
 need(argc>=2,"explicit outside-source fixture directory required");const auto root=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(root);const auto data=root/"authored-data";std::filesystem::create_directory(data);auto s=fixture();
 if(argc==3&&std::string(argv[2])=="--fifo-probe"){
  alarm(2);const auto begin=std::chrono::steady_clock::now();std::string error;
  const auto bytes=readOriginalWorldCheckpointFile((root/"fifo.emworld").string(),data.string(),error);
  const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count();alarm(0);
  need(!bytes&&error=="world_io_read_bounds"&&elapsed<1000,"FIFO without writer promptly rejects before any read");
  alarm(2);need(!original_world_binding_detail::file(ResourceRoot::fromAbsolute(data.string()),"fifo.sav",error)&&(error=="world_binding_file_bounds"||error=="world_binding_missing_file"),"sourceFIFO promptly rejects as nonregular");alarm(0);std::cout<<"{\"fifo_no_writer_ms\":"<<elapsed<<",\"nonregular_refused\":true,\"source_FIFO_refused\":true}\n";return 0;
 }
 if(argc==3){const std::string stage=argv[2];writeOriginalWorldCheckpointNew((root/"crash.emworld").string(),data.string(),s,OriginalWorldIOFault::None,[&](const char* now){if(now==stage)_exit(41);});return 9;}
 std::string error;const auto path=(root/"one.emworld").string();const auto saved=writeOriginalWorldCheckpointNew(path,data.string(),s);need(saved.state!=OriginalWorldPublication::NotPublished,"new complete file published");
 const auto bytes=readOriginalWorldCheckpointFile(path,data.string(),error);need(bool(bytes)&&decodeOriginalWorldCheckpoint(*bytes,error).has_value(),"actual disk file reads and decodes");
 need(originalWorldPersistentEqual(s,*decodeOriginalWorldCheckpoint(*bytes,error),error),"actual disk persistent value exact");
 // Full-path NUL must never resolve to a shorter, different real directory.
 // Before the guard, canonical() accepted a NUL in the parent, and both APIs
 // silently operated on that parent's prefix instead of the supplied path.
 const auto nulParent=root.string()+std::string(1,'\0')+"/ignored/";
 const auto nulWrite=writeOriginalWorldCheckpointNew(nulParent+"nul-parent.emworld",data.string(),s);
 need(nulWrite.state==OriginalWorldPublication::NotPublished&&nulWrite.error=="world_io_target_path","NUL parent refuses publication before filesystem access");
 need(!std::filesystem::exists(root/"nul-parent.emworld"),"NUL parent cannot create truncated-path target");
 need(!readOriginalWorldCheckpointFile(nulParent+"one.emworld",data.string(),error)&&error=="world_io_target_path","NUL parent cannot alias an existing readable checkpoint");
 const auto nulLeaf=(root/"nul-leaf").string()+std::string(1,'\0')+".emworld";
 const auto leafWrite=writeOriginalWorldCheckpointNew(nulLeaf,data.string(),s);
 need(leafWrite.state==OriginalWorldPublication::NotPublished&&leafWrite.error=="world_io_target_path","NUL leaf also refuses before publication");
 need(!readOriginalWorldCheckpointFile(nulLeaf,data.string(),error)&&error=="world_io_target_path","NUL leaf read refuses the same target contract");
 const auto nulData=data.string()+std::string(1,'\0')+"/ignored";
 const auto rootWrite=writeOriginalWorldCheckpointNew((root/"nul-root.emworld").string(),nulData,s);
 need(rootWrite.state==OriginalWorldPublication::NotPublished&&rootWrite.error=="world_io_data_root","NUL data root cannot silently change the protected root");
 need(!std::filesystem::exists(root/"nul-root.emworld"),"invalid data root has no output side effect");
 need(!readOriginalWorldCheckpointFile(path,nulData,error)&&error=="world_io_data_root","read rejects invalid data root before opening checkpoint");
 need(readOriginalWorldCheckpointFile(path,data.string(),error)==bytes,"all malformed-path refusals preserve original checkpoint bytes");
 const auto unicode=root/std::filesystem::u8path(u8"authored-存档");std::filesystem::create_directory(unicode);
 const auto unicodePath=(unicode/"one.emworld").string();
 const auto unicodeWrite=writeOriginalWorldCheckpointNew(unicodePath,data.string(),s);
 need(unicodeWrite.state==OriginalWorldPublication::PublishedDurable,"valid Unicode directory remains a normal durable output");
 need(readOriginalWorldCheckpointFile(unicodePath,data.string(),error)==bytes,"valid Unicode checkpoint reads exact existing format bytes");
 need(writeOriginalWorldCheckpointNew(path,data.string(),s).state==OriginalWorldPublication::NotPublished,"existing target never replaced");
 std::filesystem::create_symlink(path,root/"alias.emworld");need(writeOriginalWorldCheckpointNew((root/"alias.emworld").string(),data.string(),s).state==OriginalWorldPublication::NotPublished,"existing symlink never replaced");
 need(!readOriginalWorldCheckpointFile((root/"alias.emworld").string(),data.string(),error),"read never follows leaf symlink");
 need(writeOriginalWorldCheckpointNew((data/"bad.emworld").string(),data.string(),s).state==OriginalWorldPublication::NotPublished,"data-root write rejected");
 need(!std::filesystem::exists(data/"bad.emworld"),"data-root has no created file");
 unsigned unpublished=0,publishedUnknown=0;
 for(auto fault:{OriginalWorldIOFault::Create,OriginalWorldIOFault::Write,OriginalWorldIOFault::FileSync,OriginalWorldIOFault::Close,OriginalWorldIOFault::Publish,OriginalWorldIOFault::CancelBeforePublish,OriginalWorldIOFault::ParentChanged,OriginalWorldIOFault::DirectorySync,OriginalWorldIOFault::AfterPublish,OriginalWorldIOFault::ShortWrite,OriginalWorldIOFault::InterruptedWrite}){
  const auto target=root/("fault-"+std::to_string(unsigned(fault))+".emworld");const auto r=writeOriginalWorldCheckpointNew(target.string(),data.string(),s,fault);
  const bool late=fault==OriginalWorldIOFault::DirectorySync||fault==OriginalWorldIOFault::AfterPublish;
  const bool complete=late||fault==OriginalWorldIOFault::ShortWrite||fault==OriginalWorldIOFault::InterruptedWrite;
  need(std::filesystem::exists(target)==complete,"injected publication stage has exact visibility");
  if(late){need(r.state==OriginalWorldPublication::PublishedDurabilityUnknown,"post-link failure is uncertain durability, never false not-written");++publishedUnknown;}
  else if(!complete){need(r.state==OriginalWorldPublication::NotPublished,"pre-link failure not published");++unpublished;}
  if(complete){const auto b=readOriginalWorldCheckpointFile(target.string(),data.string(),error);need(bool(b)&&*b==*bytes,"published target is all bytes, no prefix");}
 }
 const auto raceParent=root/"parent";std::filesystem::create_directory(raceParent);
 auto race=writeOriginalWorldCheckpointNew((raceParent/"race.emworld").string(),data.string(),s,OriginalWorldIOFault::None,[&](const char* stage){if(std::string(stage)=="before_publish"){std::filesystem::rename(raceParent,root/"parent-moved");std::filesystem::create_directory(raceParent);}});
 need(race.state==OriginalWorldPublication::NotPublished&&!std::filesystem::exists(raceParent/"race.emworld")&&!std::filesystem::exists(root/"parent-moved/race.emworld"),"actual parent replacement refuses anchored publication");
 // Two genuinely separate writers compete for the same final leaf.
 const auto collision=root/"collision.emworld";const auto child=fork();need(child>=0,"own competing writer");
 if(!child){const auto r=writeOriginalWorldCheckpointNew(collision.string(),data.string(),s);_exit(r.state==OriginalWorldPublication::NotPublished?3:0);}
 const auto own=writeOriginalWorldCheckpointNew(collision.string(),data.string(),s);int collisionStatus=0;need(waitpid(child,&collisionStatus,0)==child&&WIFEXITED(collisionStatus),"competing writer ends");
 const bool ours=own.state!=OriginalWorldPublication::NotPublished;need((WEXITSTATUS(collisionStatus)==0)!=ours,"exactly one writer wins");
 const auto collisionBytes=readOriginalWorldCheckpointFile(collision.string(),data.string(),error);need(bool(collisionBytes)&&*collisionBytes==*bytes,"collision winner has all bytes");
 OriginalWorldSaveReceipt receipt;need(!receipt.acknowledge(saved,s.commitRevision,s.commitRevision+1)&&!receipt.savedRevision,"stale completion cannot mark newer revision saved");need(receipt.acknowledge(saved,s.commitRevision,s.commitRevision),"exact durable revision acknowledged");
 // This FIFO is owned by this test directory and has no writer. A child
 // watchdog makes the regression fail in2s instead of hanging the test runner.
 const auto fifo=root/"fifo.emworld";need(mkfifo(fifo.c_str(),0600)==0,"self-authored FIFO created");
 need(mkfifo((data/"fifo.sav").c_str(),0600)==0,"own sourcebindingFIFO");
 const auto fifoChild=fork();need(fifoChild>=0,"own FIFO regression child");if(!fifoChild){execl(argv[0],argv[0],root.c_str(),"--fifo-probe",static_cast<char*>(nullptr));_exit(42);}
 int fifoStatus=0;need(waitpid(fifoChild,&fifoStatus,0)==fifoChild&&WIFEXITED(fifoStatus)&&WEXITSTATUS(fifoStatus)==0,"nonblocking regular-file gate completes withoutwriter");
 const auto old=encodeOriginalWorldCheckpoint(s,error);const auto savedReceipt=receipt.savedRevision;
 need(!readOriginalWorldCheckpointFile(fifo.string(),data.string(),error)&&error=="world_io_read_bounds","FIFO refuses as wrongtype");
 need(encodeOriginalWorldCheckpoint(s,error)==old&&receipt.savedRevision==savedReceipt,"rejectedFIFO preserves currentworld andsave receipt");
 std::filesystem::create_directory_symlink(root,data/"outside-alias");
 need(!original_world_binding_detail::file(ResourceRoot::fromAbsolute(data.string()),"outside-alias/fifo.emworld",error),"source path alias remainsrefused");
 need(!original_world_binding_detail::file(ResourceRoot::fromAbsolute(data.string()),"fifo.sav",error)&&(error=="world_binding_file_bounds"||error=="world_binding_missing_file"),"sourcebinding regular-file gate also rejectsFIFO promptly");
 auto uncertain=saved;uncertain.state=OriginalWorldPublication::PublishedDurabilityUnknown;OriginalWorldSaveReceipt none;need(!none.acknowledge(uncertain,s.commitRevision,s.commitRevision),"unknown durability never acknowledged");
 std::filesystem::create_directory(root/"Protected.app");need(writeOriginalWorldCheckpointNew((root/"Protected.app/no.emworld").string(),data.string(),s).state==OriginalWorldPublication::NotPublished,"application package not a checkpoint output");
 // Read-only source binding negatives use self-authored bytes only.
 std::filesystem::create_directories(data/"Save");{std::ofstream f(data/"Save/fixture.sav");f<<"authored-source";}
 const auto rr=ResourceRoot::fromAbsolute(data.string());const auto binding=original_world_binding_detail::file(rr,"Save/fixture.sav",error);need(bool(binding),"authored source hash");
 const auto originalTime=std::filesystem::last_write_time(data/"Save/fixture.sav");{std::ofstream f(data/"Save/fixture.sav");f<<"authored-sourcE";}std::filesystem::last_write_time(data/"Save/fixture.sav",originalTime);
 const auto changed=original_world_binding_detail::file(rr,"Save/fixture.sav",error);need(bool(changed)&&changed->bytes==binding->bytes&&changed->digest!=binding->digest,"same size andmtime cannot hide changed bytes");
 std::filesystem::create_symlink(data/"Save/fixture.sav",data/"Save/alias.sav");need(!original_world_binding_detail::file(rr,"Save/alias.sav",error),"source leafsymlink rejected");
 need(!original_world_binding_detail::file(rr,"../one.emworld",error),"source pathescape rejected");need(!bindOriginalWorldSource(rr,"Save/fixture.sav",error),"unknown authored wire/FSA never receives full-world flag");
 need(std::filesystem::remove(data/"fifo.sav"),"remove onlythe sourceFIFO createdbythisfixture before regular-file relocation test");
 OriginalWorldSourceBinding full;full.source=*changed;full.legacyRevision=1;
 for(const auto& name:original_world_binding_detail::resources()){
  const auto file=data/name;std::filesystem::create_directories(file.parent_path());{std::ofstream out(file);out<<"authored-resource";}
  const auto item=original_world_binding_detail::file(rr,name,error);need(bool(item),"authored resource binds");full.resources.push_back(*item);
 }
 need(verifyOriginalWorldBinding(rr,full,error),"full exact resource list binds");
 const auto relocated=root/"relocated-data";std::filesystem::copy(data,relocated,std::filesystem::copy_options::recursive|std::filesystem::copy_options::skip_symlinks);
 need(verifyOriginalWorldBinding(ResourceRoot::fromAbsolute(relocated.string()),full,error),"same relative content binds at explicit relocated root");
 const auto model=data/"Model/EmperorBuildingModels.txt";const auto modelTime=std::filesystem::last_write_time(model);{std::ofstream out(model);out<<"authored-resourcE";}std::filesystem::last_write_time(model,modelTime);
 need(!verifyOriginalWorldBinding(rr,full,error)&&error=="world_binding_content_changed","model same-size/mtime content edit refuses");
 std::filesystem::create_directory_symlink(relocated/"Model",data/"ModelAlias");need(!original_world_binding_detail::file(rr,"ModelAlias/EmperorBuildingModels.txt",error),"source directory symlink refuses");
 unsigned crashCases=0;
 for(const char* stage:{"temp_created","bytes_written","file_synced","published","directory_synced"}){
  const auto dir=root/("crash-"+std::string(stage));std::filesystem::create_directory(dir);
  const auto child=fork();need(child>=0,"spawn own authored checkpoint test child");if(child==0){execl(argv[0],argv[0],dir.c_str(),stage,static_cast<char*>(nullptr));_exit(42);}int status=0;need(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==41,"own subprocess terminates at actual IO seam");
  const bool final=std::string(stage)=="published"||std::string(stage)=="directory_synced";need(std::filesystem::exists(dir/"crash.emworld")==final,"interruption visibility matches publication point");
  if(final){const auto b=readOriginalWorldCheckpointFile((dir/"crash.emworld").string(),(dir/"authored-data").string(),error);need(bool(b)&&decodeOriginalWorldCheckpoint(*b,error).has_value(),"fresh process output is complete after publication");}
  ++crashCases;
 }
 for(const auto& f:std::filesystem::directory_iterator(root))need(f.path().filename().string().rfind(".emworld-tmp-",0)!=0,"controlled failure cleans only its own temp");
 std::cout<<"{\"passed\":true,\"IO_checks\":"<<checks<<",\"unpublished_faults\":"<<unpublished<<",\"published_unknown_faults\":"<<publishedUnknown<<",\"own_process_interruptions\":"<<crashCases<<",\"normal_durable\":"<<(saved.state==OriginalWorldPublication::PublishedDurable?"true":"false")<<",\"power_failure_tested\":false}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
