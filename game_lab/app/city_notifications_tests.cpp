// SPDX-License-Identifier: MIT
#include "city_playback.hpp"
#include "../original_pak.hpp"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace game_lab;
namespace {
unsigned checks=0;
void need(bool b,const char* what){++checks;if(!b)throw std::runtime_error(what);}
void authored(){
    OriginalBuildingScenarioMessages m;
    auto box=AppCityNotifications::fromSaved(m);need(bool(box),"empty saved table");
    need(box->saved().empty()&&box->generated().empty(),"no invented messages");
    OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
    c.month=0;c.elapsedYears=3;c.scenarioStartYears=1;c.treasury=900;
    need(originalBuildingAnnualMessage(c,m,{100,29},5,2)==OriginalBuildingUpdateBlocker::None,"existing producer");
    need(m.count==1&&m.notices.size()==1,"producer receipt");
    need(!AppCityNotifications::fromSaved(m),"runtime receipt is not saved history");
    const auto records=m.records;
    need(box->observeCommitted(m)&&box->generated().size()==1&&box->saved().empty(),"committed receipt delivered");
    const auto first=box->report();
    need(box->observeCommitted(m)&&box->report()==first,"retained receipt idempotent");
    need(c.treasury==900&&m.records==records,"view never credits or marks original records");
    need(originalBuildingAnnualMessage(c,m,{100,29},8,2)==OriginalBuildingUpdateBlocker::None,"second producer");
    auto bad=m;bad.notices[1].slot=150;
    need(!box->observeCommitted(bad)&&box->report()==first,"out of bounds atomic rejection");
    bad=m;bad.notices[1].slot=-1;
    need(!box->observeCommitted(bad)&&box->report()==first,"negative slot atomic rejection");
    bad=m;bad.notices[1].amount=101;
    need(!box->observeCommitted(bad)&&box->report()==first,"receipt and record mismatch rejected");
    bad=m;bad.count=151;
    need(!box->observeCommitted(bad)&&box->report()==first,"invalid table count rejected");
    bad=m;bad.notices[0].amount=101;bad.records[0][12]=101;
    need(!box->observeCommitted(bad)&&box->report()==first,"changed payload cannot reuse receipt");
    bad=m;bad.notices[1].id=226;
    need(!box->observeCommitted(bad)&&box->report()==first,"unknown notice contract rejected");
    need(box->observeCommitted(m)&&box->generated().size()==2,"two producer notices delivered");
    const auto both=box->report();for(unsigned n=0;n<3;++n)need(box->observeCommitted(m)&&box->report()==both,"repeated view/replay has no new effect");
    auto loaded=m;loaded.notices.clear();
    auto reload=AppCityNotifications::fromSaved(loaded);
    need(reload&&reload->saved().size()==2&&reload->generated().empty(),"reload turns records into history, no payable reconstruction");
    const auto savedReport=reload->report();
    need(!reload->observeCommitted(m)&&reload->report()==savedReport,"old-load receipts cannot be replayed as new");
    need(reload->observeCommitted(loaded)&&reload->report()==savedReport,"no receipt means no queue insertion");
    loaded.count=0;need(!reload->observeCommitted(loaded)&&reload->report()==savedReport,"table regression rejected");
    loaded.count=150;loaded.records[149][2]=29;
    need(AppCityNotifications::fromSaved(loaded)->saved().size()==3,"last bounded slot readable");
    loaded.count=151;need(!AppCityNotifications::fromSaved(loaded),"saved table bounds checked");
    need(c.treasury==900,"all display/replay/invalid operations preserve treasury");
    std::cout<<"{\"authored_checks\":"<<checks<<",\"original_data\":false,\"passed\":true}\n";
}
void real(int argc,char** argv){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    const auto root=ResourceRoot::fromAbsolute(env);AppCityPlaybackPreferences prefs;std::string error;
    need(readAppCityPlaybackPreferences(root,prefs,error),"real preference file");
    need(argc>2,"explicit relative saves required");
    for(int i=2;i<argc;++i){
        const auto save=loadOriginalSave(root.resolve(argv[i]));std::vector<uint8_t> raw;
        need(detail::readFile(root.resolve(argv[i]),4194304,raw),"save read");
        const auto decoded=decodeOriginalMapContainer(raw);need(decoded.complete,"save decode");
        const auto messages=originalBuildingSavedScenarioMessages(decoded.payload,save.sizeOffset);need(bool(messages),"saved records");
        OriginalBuildingUpdateContext context(OriginalBuildingUpdateScope::AdmittedTicks,true);
        need(originalBuildingSavedCityContext(decoded.payload,save.sizeOffset,context)&&context.treasury.has_value(),"saved treasury");
        AppMapSurface surface(root);AppCitySessionState session;session.crtRandom=7;
        AppCityPlayback playback(surface,save,decoded.payload,prefs,7,0,false,&session,false,nullptr,true);
        need(playback.prepared()&&playback.notifications(),"normal App notification preparation");
        size_t count=0;for(unsigned n=0;n<messages->count;++n)if(messages->records[n][2]==29)++count;
        need(playback.notifications()->saved().size()==count&&playback.notifications()->generated().empty(),"all real type29 records displayed, none queued");
        for(const auto& row:playback.notifications()->saved()){
            const auto* r=messages->records[row.slot].data();
            need(row.resource==detail::i16(r+4)&&row.amount==detail::i16(r+12)&&row.faction==detail::i16(r+28),"real record fields preserved");
        }
        const auto before=playback.notifications()->report();const auto digest=surface.installedSceneDigest();
        need(!playback.step(1000)&&playback.status().refusal=="event_ui_5eff20_not_closed"&&playback.status().updates==0,"normal event refusal retained");
        need(playback.notifications()->report()==before&&surface.installedSceneDigest()==digest&&session.admissionClock==0,"refused update publishes no scene/session/notification changes");
        surface.clearCityPlayback();need(!surface.cityNotifications(),"clearing city removes its inbox");
        AppCityPlayback replay(surface,save,decoded.payload,prefs,7,0,false,&session,false,nullptr,true);
        need(replay.prepared()&&replay.notifications()->report()==before&&replay.notifications()->generated().empty(),"same saved history on readback, no duplicate queue");
        std::cout<<"{\"source\":"<<std::quoted(argv[i])<<",\"saved_type29\":"<<count<<",\"generated\":0,\"treasury\":"<<*context.treasury<<",\"normal_first_update\":false,\"display_checks_passed\":true}\n";
    }
}
// A caller-supplied original-game pair with different observed payment outcomes.
// Identical saved records cannot identify which offer was accepted. Do not
// reconstruct a payable queue from their processing flags or arrival order.
void outcomePair(int argc,char** argv){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    need(argc==4,"two explicit relative outcome saves required");
    const auto root=ResourceRoot::fromAbsolute(env);
    struct Outcome {OriginalBuildingScenarioMessages messages;int32_t treasury=0,income=0;std::array<int32_t,4> components{};};
    const auto read=[&](const char* path){
        const auto save=loadOriginalSave(root.resolve(path));std::vector<uint8_t> raw;
        need(detail::readFile(root.resolve(path),4194304,raw),"outcome save read");
        const auto decoded=decodeOriginalMapContainer(raw);need(decoded.complete,"outcome save decode");
        const auto messages=originalBuildingSavedScenarioMessages(decoded.payload,save.sizeOffset);need(bool(messages),"outcome message table");
        OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
        need(originalBuildingSavedCityContext(decoded.payload,save.sizeOffset,c)&&c.treasury&&c.accounting,"outcome accounting");
        Outcome out;out.messages=*messages;out.treasury=*c.treasury;out.income=c.accounting->incomeOther;
        const size_t p=save.sizeOffset-12287;unsigned n=0;
        for(size_t wire:{12027u,11440u,11061u,10498u})out.components[n++]=originalBuildingI32(detail::u32(decoded.payload.data()+p+wire));
        return out;
    };
    const auto a=read(argv[2]),b=read(argv[3]);
    need(a.messages.count==b.messages.count&&a.messages.records==b.messages.records,"outcome pair must have identical complete saved message tables");
    const int64_t paid=int64_t(a.treasury)-b.treasury;
    need(paid!=0&&paid==int64_t(a.income)-b.income&&paid==int64_t(a.components[0])-b.components[0],"outcome pair must differ in both funds and the observed income component");
    for(unsigned n=1;n<4;++n)need(a.components[n]==b.components[n],"other loaded income components equal");
    const auto x=AppCityNotifications::fromSaved(a.messages),y=AppCityNotifications::fromSaved(b.messages);
    need(x&&y&&!x->saved().empty(),"outcome histories available");
    need(x->report()==y->report()&&x->generated().empty()&&y->generated().empty(),"same history with different payments never reconstructs payable notifications");
    std::cout<<"{\"outcome_pair_passed\":true,\"equal_saved_records\":"<<a.messages.records.size()
             <<",\"history_records\":"<<x->saved().size()<<",\"funds_income_difference\":"<<paid<<",\"generated\":0}\n";
}

void transactions(){
    const unsigned start=checks;
    OriginalBuildingUpdateContext city(OriginalBuildingUpdateScope::AdmittedTicks,true);
    city.month=0;city.elapsedYears=3;city.scenarioStartYears=1;city.treasury=900;city.accounting.emplace();
    city.accounting->incomeOther=40;city.accounting->tributeIncomeComponent=10;
    OriginalBuildingScenarioMessages m;auto box=AppCityNotifications::fromSaved(m);
    need(bool(box),"transaction inbox");
    need(originalBuildingAnnualMessage(city,m,{100,29},5,2)==OriginalBuildingUpdateBlocker::None,"transaction producer one");
    need(originalBuildingAnnualMessage(city,m,{100,29},8,2)==OriginalBuildingUpdateBlocker::None,"transaction producer two");
    need(box->observeCommitted(m),"committed pending pair");const auto records=m.records;const auto notices=m.notices.size();
    const auto id=box->nextPending();need(bool(id),"pending identity");const auto before=box->report();
    auto stale=*id;++stale.load;
    need(box->respond(stale,AppCityTributeDecision::Accept,city)==AppCityTributeResult::Stale&&box->report()==before&&city.treasury==900,"foreign load cannot transact");
    stale=*id;stale.slot=150;need(box->respond(stale,AppCityTributeDecision::Accept,city)==AppCityTributeResult::Stale,"invalid slot cannot transact");
    need(box->respond(*id,AppCityTributeDecision::Accept,city)==AppCityTributeResult::Applied,"accept applied");
    need(city.treasury==1000&&city.accounting->incomeOther==140&&city.accounting->tributeIncomeComponent==110,"all observed ledger effects applied");
    need(box->find(*id)->state==AppCityTributeState::Accepted,"terminal accepted identity");
    need(box->respond(*id,AppCityTributeDecision::Accept,city)==AppCityTributeResult::AlreadyCompleted&&box->respond(*id,AppCityTributeDecision::Reject,city)==AppCityTributeResult::AlreadyCompleted&&city.treasury==1000,"repeated/opposite decision has no effect");
    const auto second=box->nextPending();need(second&&second->slot!=id->slot,"next distinct pending identity");
    need(box->respond(*second,AppCityTributeDecision::Reject,city)==AppCityTributeResult::Applied&&city.treasury==1000&&city.accounting->incomeOther==140&&city.accounting->tributeIncomeComponent==110,"reject leaves all money unchanged");
    need(!box->nextPending()&&box->find(*second)->state==AppCityTributeState::Rejected,"reject removes pending item");
    need(box->respond(*second,AppCityTributeDecision::Accept,city)==AppCityTributeResult::AlreadyCompleted,"rejected identity cannot later be paid");
    const auto done=box->report();need(box->observeCommitted(m)&&box->report()==done&&!box->nextPending(),"retained receipts do not reopen completed offers");
    need(m.records==records&&m.notices.size()==notices&&m.notificationPending,"native decisions leave original records/receipts/pending untouched");
    auto saved=m;saved.notices.clear();auto reload=AppCityNotifications::fromSaved(saved);
    need(reload&&!reload->nextPending()&&reload->respond(*id,AppCityTributeDecision::Accept,city)==AppCityTributeResult::Stale,"reload history cannot pay old identity");
    auto regressed=m;regressed.count=1;need(!box->observeCommitted(regressed)&&box->report()==done,"committed count cannot regress after decisions");
    auto changed=m;changed.records[0][12]=101;changed.notices.clear();need(!box->observeCommitted(changed)&&box->report()==done,"record change without receipt still rejects atomically");
    for(unsigned missing=0;missing<3;++missing){
        OriginalBuildingScenarioMessages empty;auto test=AppCityNotifications::fromSaved(empty);need(test->observeCommitted(m),"missing ledger setup");auto c=city;
        if(missing==0)c.treasury.reset();if(missing==1)c.accounting.reset();if(missing==2)c.accounting->tributeIncomeComponent.reset();
        const auto report=test->report();need(test->respond(*test->nextPending(),AppCityTributeDecision::Accept,c)==AppCityTributeResult::MissingLedger&&test->report()==report,"missing ledger keeps notification pending");
        need(test->respond(*test->nextPending(),AppCityTributeDecision::Reject,c)==AppCityTributeResult::Applied,"reject needs no financial fields");
    }
    for(unsigned field=0;field<3;++field){
        OriginalBuildingScenarioMessages empty;auto test=AppCityNotifications::fromSaved(empty);need(test->observeCommitted(m),"overflow setup");auto c=city;
        *c.treasury=field==0?INT32_MAX-99:0;c.accounting->incomeOther=field==1?INT32_MAX-99:0;*c.accounting->tributeIncomeComponent=field==2?INT32_MAX-99:0;
        const auto money=*c.treasury,income=c.accounting->incomeOther,component=*c.accounting->tributeIncomeComponent;const auto report=test->report();
        need(test->respond(*test->nextPending(),AppCityTributeDecision::Accept,c)==AppCityTributeResult::Overflow&&*c.treasury==money&&c.accounting->incomeOther==income&&*c.accounting->tributeIncomeComponent==component&&test->report()==report,"overflow rejects every ledger/status write");
        *c.treasury=INT32_MAX-100;c.accounting->incomeOther=INT32_MAX-100;*c.accounting->tributeIncomeComponent=INT32_MAX-100;
        need(test->respond(*test->nextPending(),AppCityTributeDecision::Accept,c)==AppCityTributeResult::Applied&&c.treasury==INT32_MAX&&c.accounting->incomeOther==INT32_MAX&&c.accounting->tributeIncomeComponent==INT32_MAX,"exact arithmetic boundary accepted");
    }
    OriginalBuildingScenarioMessages empty;auto unsupported=AppCityNotifications::fromSaved(empty);auto bad=m;bad.records[0][12]=bad.records[0][13]=0;bad.notices[0].amount=0;bad.notices[0].title=560;bad.notices[0].dialogFlags=3;
    need(unsupported->observeCommitted(bad)&&unsupported->generated()[0].state==AppCityTributeState::Unsupported,"nonpositive incoming funds are not payable");
    const auto any=unsupported->nextPending();need(any&&any->slot==1,"unsupported amount excluded from pending selection");
    auto z=city;z.treasury=0;z.accounting->incomeOther=0;z.accounting->tributeIncomeComponent=0;
    need(unsupported->respond(*any,AppCityTributeDecision::Accept,z)==AppCityTributeResult::Applied&&z.treasury==100,"known zero ledger remains valid");
    std::cout<<"{\"transaction_checks\":"<<checks-start<<",\"authored_only\":true,\"passed\":true}\n";
}
void runtimeTransactions(int argc,char** argv){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true}\n";return;}
    need(argc==3,"explicit relative runtime source required");const auto root=ResourceRoot::fromAbsolute(env);
    const auto save=loadOriginalSave(root.resolve(argv[2]));std::vector<uint8_t> raw;need(detail::readFile(root.resolve(argv[2]),4194304,raw),"runtime source read");
    const auto d=decodeOriginalMapContainer(raw);need(d.complete,"runtime source decode");AppCityPlaybackPreferences prefs;std::string error;
    need(readAppCityPlaybackPreferences(root,prefs,error),"runtime preferences");AppMapSurface surface(root);AppCitySessionState session;session.crtRandom=1;
    // Existing explicit-premise harness only; never supplied to the GUI.
    AppCityPlayback ctl(surface,save,d.payload,prefs,1,0,true,&session);need(ctl.prepared(),"runtime prepared");
    for(unsigned n=1;n<=102;++n)need(ctl.step(uint64_t(n)*ctl.status().intervalMs),"runtime update before annual pair");
    need(ctl.notifications()->generated().size()==2,"actual annual producer has two receipts");
    const auto before=ctl.tributeFinances();const auto first=ctl.notifications()->nextPending();need(before&&first,"first actionable receipt and loaded ledger");
    const int amount=ctl.notifications()->find(*first)->amount;const auto digest=surface.installedSceneDigest();
    need(ctl.respondNotification(*first,AppCityTributeDecision::Accept)==AppCityTributeResult::Applied,"active chain accepted");
    const auto accepted=ctl.tributeFinances();need(accepted&&accepted->treasury==int64_t(before->treasury)+amount&&accepted->incomeOther==int64_t(before->incomeOther)+amount&&accepted->tributeIncomeComponent==int64_t(before->tributeIncomeComponent)+amount,"active chain ledger updated");
    need(ctl.respondNotification(*first,AppCityTributeDecision::Accept)==AppCityTributeResult::AlreadyCompleted,"active chain double click ignored");
    const auto second=ctl.notifications()->nextPending();need(bool(second),"second receipt remains pending");
    need(ctl.respondNotification(*second,AppCityTributeDecision::Reject)==AppCityTributeResult::Applied&&!ctl.notifications()->nextPending(),"active chain rejected second");
    need(surface.installedSceneDigest()==digest&&ctl.status().updates==102&&ctl.replayFrames().empty(),"UI decisions do not advance/render/change journal");
    const auto done=ctl.notifications()->report();
    for(unsigned n=103;n<=236;++n)need(ctl.step(uint64_t(n)*ctl.status().intervalMs),"continued chain after decisions");
    const auto retained=ctl.tributeFinances();need(retained&&retained->treasury==accepted->treasury&&retained->incomeOther==accepted->incomeOther&&retained->tributeIncomeComponent==accepted->tributeIncomeComponent&&ctl.notifications()->report()==done,"financial decisions survive continuation without replay");
    need(!ctl.step(uint64_t(237)*ctl.status().intervalMs)&&ctl.status().refusal=="explicit_phase_terrain_disabled_context_required"&&ctl.notifications()->report()==done,"unrelated next refusal retained");
    AppCityPlayback reload(surface,save,d.payload,prefs,1,0,false,&session,false,nullptr,true);need(reload.prepared()&&!reload.notifications()->nextPending()&&reload.respondNotification(*first,AppCityTributeDecision::Accept)==AppCityTributeResult::Stale,"normal reload cannot replay old runtime identity");
    need(!reload.step(1000000)&&reload.status().refusal=="event_ui_5eff20_not_closed","normal event gate still enforced");
    std::cout<<"{\"runtime_transactions_passed\":true,\"committed_receipts\":2,\"accepted\":1,\"rejected\":1,\"retained_through_update\":236,\"explicit_test_premises\":true,\"normal_GUI\":false}\n";
}

}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--real")real(argc,argv);else if(argc>1&&std::string(argv[1])=="--outcome-pair")outcomePair(argc,argv);else if(argc>1&&std::string(argv[1])=="--runtime-transactions")runtimeTransactions(argc,argv);else {authored();transactions();}return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
