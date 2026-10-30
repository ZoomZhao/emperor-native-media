// SPDX-License-Identifier: MIT
#include "original_kernel_checkpoint.hpp"
#include "app/city_notifications.hpp"
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool x,const char* why){++checks;if(!x)throw std::runtime_error(why);}
OriginalCombinedState world(){OriginalCombinedState s;s.figures.terrain.resize(51984);s.figures.cellHeads.resize(51984);
    for(size_t slot=0;slot<2000;++slot){OriginalFigure f;f.slot=slot;s.figures.figures.emplace(slot,f);if(slot)s.figures.freeSlots.push_back(slot);}return s;}
void live(OriginalCombinedState& s,size_t slot,uint16_t serial,uint8_t type=28){auto& f=s.figures.figures.at(slot);f.type=type;f.alive=1;f.allocationSerial=serial;
    s.figures.freeSlots.erase(std::find(s.figures.freeSlots.begin(),s.figures.freeSlots.end(),slot));s.figures.highestLiveSlot=std::max(slot,s.figures.highestLiveSlot);}
void identities(){auto s=world();live(s,1,65535);std::string error;const auto first=OriginalKernelIdentities::loaded(s.figures.figures,error);need(bool(first),"load identity");const auto h=*first->handle(1);
    need(first->matches(h)&&first->consistent(s.figures.figures),"initial identity matches");
    s.figures.figures.at(1).allocationSerial=0;auto next=first->advanced(s.figures.figures,{{1,0}},error);need(bool(next)&&!next->matches(h),"65535 to zero is legal but invalidates handle");
    const auto zero=*next->handle(1);auto stable=next->advanced(s.figures.figures,{},error);need(bool(stable)&&stable->matches(zero),"unchanged identity stable");
    auto repeated=next->advanced(s.figures.figures,{{1,0},{1,0}},error);need(bool(repeated)&&!repeated->matches(zero),"repeated same wire serial allocations invalidate host handle");
    auto unknown=first->advanced(s.figures.figures,{},error);need(!unknown&&error=="kernel_untracked_allocation","unrecorded reuse refuses");
    auto load=repeated->reloaded(error);need(bool(load)&&!load->matches(*repeated->handle(1)),"restore invalidates old epoch");
    s.figures.figures.at(1).type=23;const auto converted=next->advanced(s.figures.figures,{},error);need(bool(converted)&&!converted->matches(zero),"in-place type conversion invalidates commands");
    need(!next->advanced(s.figures.figures,{{2000,0}},error),"allocation out of pool refused");
    s.figures.figures.at(1).alive=0;const auto removed=next->advanced(s.figures.figures,{},error);need(bool(removed)&&!removed->handle(1),"retired identity removed");}
void structure(){auto s=world();need(originalKernelAuthorityIssue(s).empty(),"empty complete pool valid");live(s,9,0);need(originalKernelAuthorityIssue(s).empty(),"zero serial valid");
    auto bad=s;bad.figures.freeSlots.push_back(9);need(originalKernelAuthorityIssue(bad)=="kernel_free_fifo_mismatch","live actor cannot be free");
    bad=s;bad.figures.freeSlots.push_back(bad.figures.freeSlots.front());need(originalKernelAuthorityIssue(bad)=="kernel_free_fifo_mismatch","duplicate FIFO refused");
    bad=s;bad.figures.figures.erase(0);need(originalKernelAuthorityIssue(bad)=="kernel_pool_dimensions","missing sentinel refused");
    bad=s;bad.figures.figures.at(9).slot=8;need(originalKernelAuthorityIssue(bad)=="kernel_pool_identity","slot mismatch refused");
    bad=s;bad.figures.highestLiveSlot=8;need(originalKernelAuthorityIssue(bad)=="kernel_highest_live_mismatch","stale high-water refused");
    bad=s;bad.figures.cellHeads[0]=9;bad.figures.figures.at(9).next=9;need(originalKernelAuthorityIssue(bad)=="kernel_cell_chain_mismatch","cyclic draw chain refused");
    bad=s;bad.phase=51;need(originalKernelAuthorityIssue(bad)=="kernel_phase_bounds","bad phase refused");
    OriginalCombinedBuilding b{{},OriginalBuildingUpdateContext(OriginalBuildingUpdateScope::AdmittedTicks,true)};b.state.building.slot=7;b.state.building.type=59;b.state.inputs.alive=1;
    s.buildings.emplace(7,b);need(originalKernelAuthorityIssue(s)=="kernel_building_links_missing","missing links refused");
    s.figures.buildings.emplace();OriginalFigureBuildingLink l;l.alive=1;l.type=59;s.figures.buildings->emplace(7,l);
    need(originalKernelAuthorityIssue(s).empty(),"shared authority aligned");s.buildings.at(7).state.inputs.workers=1;
    need(originalKernelAuthorityIssue(s)=="kernel_building_authority_mismatch","population worker divergence refused");
    s.buildings.at(7).state.inputs.workers=0;s.buildings.at(7).state.inputs.fireRisk=2;
    need(originalKernelAuthorityIssue(s)=="kernel_building_shared_fields_mismatch","risk divergence refused");}
void marketCheckpoint(){OriginalKernelCheckpoint cp;cp.sourceRevision=99;cp.modelRevision=88;cp.city=world();live(cp.city,1,0,24);live(cp.city,2,65535,25);
    auto& a=cp.city.figures.figures.at(1);auto& b=cp.city.figures.figures.at(2);a.owner=b.owner=66;a.delivery=15;a.follower=2;b.leaderLink=1;
    a.route=4;a.routeCursor=a.routeLength=500; // exhausted route is legal without reciprocal owner
    for(const auto* actor:{&a,&b}){OriginalMarketKnownBase raw;raw.sourceRevision=99;raw.figure.slot=actor->slot;raw.figure.recordComplete=true;raw.figure.fields.resize(std::size(emperor::file_io::research::figure7_fields));
        cp.marketBases.emplace(actor->slot,originalMarketKnownBaseAt(raw,*actor));}
    std::string error;cp.identities=*OriginalKernelIdentities::loaded(cp.city.figures.figures,error);
    need(originalKernelCheckpointIssue(cp).empty(),"complete known market pair saves with no follower route");
    auto restored=prepareOriginalKernelLoad(OriginalKernelLoadKind::CompleteValueCheckpoint,cp,99,88,error);need(bool(restored)&&restored->city.figures.figures.at(2).delivery==0,"companion zero delivery remains raw zero");
    auto bad=cp;bad.city.figures.figures.at(1).follower=1;bad.marketBases.at(1)=originalMarketKnownBaseAt(bad.marketBases.at(1),bad.city.figures.figures.at(1));
    need(originalKernelCheckpointIssue(bad)=="kernel_checkpoint_market_pair","inconsistent pair refuses after all records loaded");
    bad=cp;bad.city.figures.figures.at(1).amount=2;need(originalKernelCheckpointIssue(bad)=="kernel_checkpoint_market_mirror","typed/raw contradiction refuses");
    bad=cp;bad.city.figures.figures.at(1).owner=4000;bad.marketBases.at(1)=originalMarketKnownBaseAt(bad.marketBases.at(1),bad.city.figures.figures.at(1));
    need(originalKernelCheckpointIssue(bad)=="kernel_checkpoint_market_building_reference","building range checked");
    auto drawn=cp;drawn.city.figures.cellHeads[0]=2;drawn.city.figures.figures.at(2).next=1;
    for(auto& row:drawn.marketBases)row.second=originalMarketKnownBaseAt(row.second,drawn.city.figures.figures.at(row.first));
    need(originalKernelCheckpointIssue(drawn).empty()&&drawn.marketBases.at(2).figure.field(16)==1,"post-draw chain order is preserved rather than sorted by slot");
    auto transition=cp;transition.city.figures.figures.at(2).alive=2;transition.marketBases.at(2)=originalMarketKnownBaseAt(transition.marketBases.at(2),transition.city.figures.figures.at(2));
    need(originalKernelCheckpointIssue(transition).empty(),"known retiring base preserved without borrowing live1 observation qualification");
    const auto pass=originalFigureOrderedNextPass(cp.city.figures,{});need(!pass.complete&&!pass.context,"base restore never installs unsupported market updater");
}
void notices(){OriginalBuildingScenarioMessages m;auto box=AppCityNotifications::fromSaved(m);OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
    c.month=0;c.elapsedYears=3;c.scenarioStartYears=1;
    need(originalBuildingAnnualMessage(c,m,{100,29},5,2)==OriginalBuildingUpdateBlocker::None&&box->observeCommitted(m),"runtime notice candidate");
    const auto id=*box->nextPending();const auto restored=box->reloaded();need(bool(restored)&&!restored->find(id)&&restored->nextPending()->load!=id.load,"checkpoint reload invalidates old notice IDs");
    need(restored->report()==box->report(),"pending disposition preserved");auto rejected=*box;
    need(rejected.respond(id,AppCityTributeDecision::Reject,c)==AppCityTributeResult::Applied,"reject notice");const auto done=rejected.reloaded();
    need(bool(done)&&done->generated()[0].state==AppCityTributeState::Rejected&&!done->nextPending(),"completed disposition is not payable after restore");}
void restore(){OriginalKernelCheckpoint cp;cp.sourceRevision=11;cp.modelRevision=12;cp.city=world();live(cp.city,2,0);
    cp.city.figures.cellHeads[7]=2;cp.city.figures.figures.at(2).cell=7;cp.city.figures.figures.at(2).next=0;cp.city.phase=31;
    cp.city.process.eventSlots=std::array<uint32_t,8>{{1,2,3,4,5,6,7,8}};cp.city.process.eventGate17.reset();cp.city.process.eventGate16=9;
    cp.city.process.cityClocks.emplace();cp.city.process.cityClocks->value[7]=99;cp.city.process.crtRandom=73;cp.city.figures.services.emplace();cp.city.figures.services->allocationSerial=65536;
    cp.city.figures.freeSlots.erase(cp.city.figures.freeSlots.begin());cp.city.figures.freeSlots.push_back(1); // retain legal FIFO order
    cp.admissionClock=999;std::string error;cp.identities=*OriginalKernelIdentities::loaded(cp.city.figures.figures,error);
    auto restored=prepareOriginalKernelLoad(OriginalKernelLoadKind::CompleteValueCheckpoint,cp,11,12,error);need(bool(restored),"complete checkpoint restores");
    need(restored->city.process.eventSlots==cp.city.process.eventSlots&&!restored->city.process.eventGate17&&restored->city.process.eventGate16==9,"unknown and nonzero slots preserved, never initialized");
    need(restored->city.figures.freeSlots==cp.city.figures.freeSlots&&restored->city.figures.cellHeads==cp.city.figures.cellHeads&&restored->city.figures.services->allocationSerial==65536,"FIFO chain and full allocation counter preserved");
    need(restored->city.phase==31&&restored->admissionClock==999&&restored->city.process.cityClocks->value[7]==99,"all clocks preserved");
    need(!restored->identities.matches(*cp.identities.handle(2)),"checkpoint restore has new host epoch");
    need(!prepareOriginalKernelLoad(OriginalKernelLoadKind::CompleteValueCheckpoint,cp,99,12,error)&&error=="kernel_checkpoint_source_changed","source mismatch refuses");
    auto missing=cp;missing.identities={};need(!prepareOriginalKernelLoad(OriginalKernelLoadKind::CompleteValueCheckpoint,missing,11,12,error)&&error=="kernel_checkpoint_identity_ledger","incomplete checkpoint refuses, no SAV fallback");
    need(bool(prepareOriginalKernelLoad(OriginalKernelLoadKind::OriginalSAV,missing,11,12,error)),"explicit SAV load creates fresh host identities only");
    auto market=cp;market.city.figures.figures.at(2).type=24;need(!prepareOriginalKernelLoad(OriginalKernelLoadKind::OriginalSAV,market,11,12,error)&&error=="kernel_checkpoint_market_base_missing","market projection alone cannot save");
    // Saved nonzero registry is retained, and its unported branch still refuses.
    auto events=restored->city;events.scenarioMessages.emplace();need(originalCombinedScenarioEvents(events)=="scenario_slot_467680_not_closed","restoration retains nonempty slots and their exact refusal");
}
}
int main(){try{identities();structure();restore();notices();marketCheckpoint();std::cout<<"{\"passed\":true,\"kernel_contract_checks\":"<<checks<<",\"original_data\":false}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
