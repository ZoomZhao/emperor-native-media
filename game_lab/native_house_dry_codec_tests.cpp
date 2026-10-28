// SPDX-License-Identifier: MIT
// Pure synthetic codec contracts; no resource root, asset or original process.
#include "native_session.hpp"
#include "native_walker_codec.hpp"
#include <iostream>
using namespace game_lab;
namespace {
size_t checks=0;void need(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
std::string replaced(std::string s,const std::string& a,const std::string& b){auto at=s.find(a);need(at!=std::string::npos,"mutation anchor present");s.replace(at,a.size(),b);return s;}
struct Fill {size_t n=0;template<class T>void operator()(const char*,T& x){++n;
 if constexpr(std::is_same_v<T,std::optional<int32_t>>)x=INT32_MIN;
 else if constexpr(std::is_same_v<T,bool>)x=true;
 else if constexpr(std::is_same_v<T,OriginalFigureClass>)x=OriginalFigureClass::Figure;
 else if constexpr(std::is_signed_v<T>)x=T(std::numeric_limits<T>::min()+int(n));
 else x=T(std::numeric_limits<T>::max()-n);}};
std::string figureFields(OriginalFigure f){native_walker_detail::Writer a;native_walker_detail::visitWellWalker(a,f);return a.out;}
std::string runtime(const NativeSessionState& s){std::ostringstream out;native_runtime_wire::encode(out,s);need(bool(out),"wire encoder accepts legal synthetic DTO");return out.str();}
bool decodeRuntime(const std::string& text,NativeSessionState& s){std::istringstream in(text);if(!native_runtime_wire::decode(in,s))return false;in>>std::ws;return in.eof();}
void migrants(){
 const char* oldWell=R"FIG(NATIVE-WELL-WALKER 1 118
slot 7
recordOffset 0
image 0
secondaryImage 0
cell 0
x 0
y 0
subX 0
subY 0
extraX 0
extraY 0
next 0
type 28
alive 1
frame 0
action 1
progress 0
direction 0
previousDirection 0
freeMovement 0
height 0
hidden 0
drawClass 0
state85 0
raise 0
subtype 0
attackDirection 0
attackFrame 0
resource 0
cartStyle 0
stationary 0
counter 0
owner 5
variant 0
amount 0
leaderLink 0
target 0
faction 0
employmentAmount 0
watchCoverage 0
effectKind 0
effectVariant 0
effectFlags 0
effectX 0
effectY 0
effectVelocityX 0
effectVelocityY 0
effectHeight 0
effectVelocityZ 0
effectDraw 0
effectAnimation -
effectFrame 0
updateOwner 0
threat 0
threatDistance 0
herdTarget 0
herdDistance 0
idleCounter 0
cooldown0 0
cooldown1 0
cooldown2 0
linkedBuilding 0
movementMode 0
attackerCount 0
route 0
routeCursor 0
routeLength 0
destinationX 0
destinationY 0
destinationCell 0
age 0
priorProgress 0
reciprocalTarget 0
follower 0
routePhase 0
speed 0
priorX 0
priorY 0
homeX 0
homeY 0
home 0
delivery 0
priorCell 0
flags120 0
water 0
roamLimit 0
roamSteps 0
roamState 0
roamTurns 0
roamTurn 0
roamCountdown 0
searchCounter 0
roamOriginX 0
roamOriginY 0
freeTargetX 0
freeTargetY 0
freeRemainingX 0
freeRemainingY 0
freeError 0
freeAxis 0
cellMovement 0
previousAction 0
allocationSerial 0
allocationPhase 0
allocationPhaseCopy 0
projectile 0
projectileDistance 0
projectileStartHeight 0
projectileTargetHeight 0
projectileEndHeight 0
attacker0 0
attacker1 0
damage 0
projectileHits 0
formation 0
formationOrdinal 0
formationStationary 0
runtimeClass 0
)FIG";
 OriginalFigure well;well.slot=7;well.type=28;well.alive=1;well.action=1;well.owner=5;
 std::string e;auto legacy=encodeNativeWellWalker(well,e);need(legacy&&*legacy==oldWell,"Well fixed118-field golden is byte unchanged");auto old=decodeNativeWellWalker(oldWell,e);need(old&&figureFields(*old)==figureFields(well),"Well fixed golden loads");
 OriginalFigure migrant;Fill fill;native_walker_detail::visitWellWalker(fill,migrant);need(fill.n==118,"shared fixed118 named fields");migrant.type=13;migrant.alive=1;migrant.owner=0;migrant.runtimeClass=OriginalFigureClass::Figure;
 for(uint8_t action:{6,7,8,9}){migrant.action=action;migrant.home=action==6?0:7;const auto bytes=encodeNativeMigrant(migrant,e);need(bytes&&bytes->rfind("NATIVE-MIGRANT 1 118\n",0)==0,"type13 uses independent fixed magic");const auto read=decodeNativeMigrant(*bytes,e);need(read&&figureFields(*read)==figureFields(migrant),"all118 fields survive migrant action6..9 roundtrip");need(!encodeNativeWellWalker(migrant,e)&&!decodeNativeWellWalker(*bytes,e),"Well codec does not accept type13");}
 migrant.action=6;auto wire=encodeNativeMigrant(migrant,e);need(bool(wire),"negative fixture encodes");
 need(!decodeNativeMigrant(*legacy,e),"migrant decoder refuses Well magic");
 for(const auto& bad:{replaced(*wire,"NATIVE-MIGRANT 1 118","NATIVE-MIGRANT 2 118"),replaced(*wire,"NATIVE-MIGRANT 1 118","NATIVE-MIGRANT 1 117"),replaced(*wire,"type 13\n","type 28\n"),replaced(*wire,"alive 1\n","alive 0\n"),replaced(*wire,"owner 0\n","owner 1\n"),replaced(*wire,"action 6\n","action 5\n"),replaced(*wire,"action 6\n","action 10\n"),replaced(*wire,"type 13\n","type 256\n"),replaced(*wire,"owner 0\n","owner 32768\n"),replaced(*wire,"type 13\n","type 013\n"),replaced(*wire,"type 13\n","type 13\ntype 13\n"),replaced(*wire,"alive 1\n","unknown 1\n"),wire->substr(0,wire->size()-1),*wire+"trailing 1\n",std::string(16385,'x')})need(!decodeNativeMigrant(bad,e),"migrant unknown/duplicate/truncated/overflow data refused");
 for(uint8_t action:{0,5,10,255}){auto bad=migrant;bad.action=action;need(!encodeNativeMigrant(bad,e),"migrant encoder only actions6..9");}
 auto bad=migrant;bad.type=28;need(!encodeNativeMigrant(bad,e),"migrant encoder only type13");bad=migrant;bad.owner=-1;need(!encodeNativeMigrant(bad,e),"migrant owner must be zero");bad=migrant;bad.alive=2;need(!encodeNativeMigrant(bad,e),"migrant must be alive1");bad=migrant;bad.runtimeClass=OriginalFigureClass::Pheasant;need(!encodeNativeMigrant(bad,e),"migrant must be Figure class");
}
void wireVersions(){
 const std::string legacy="RUNTIME8 1\nschedule 0 \nfigures 0 \nhook 0 \nheldHouses 0 \nheldWalkers 0 \nstrictRecords 0 \nstrictContext 0 \npolicies 0 \nobjects 0\nENDRUNTIME8\n";
 NativeSessionState empty;need(runtime(empty)==legacy,"old RUNTIME8 1 fixed literal is unchanged");
 auto stale=empty;stale.observedDry.emplace();stale.observedDry->pendingExcess[7]=4;stale.figureRuntime.emplace();stale.figureRuntime->migrantPayloads={"stale"};need(decodeRuntime(legacy,stale)&&!stale.observedDry&&!stale.figureRuntime,"v1 decode clears absent dry and stale migrants");
 NativeSessionState full;full.figureRuntime.emplace();full.observedDry.emplace();full.observedDry->pendingExcess={{7,4},{UINT64_MAX,UINT32_MAX}};full.observedDry->departedSinceMeal=UINT32_MAX;full.observedDry->arrivedSinceMeal=19;
 OriginalFigure f;f.type=13;f.alive=1;f.action=6;f.slot=9;std::string e;const auto migrant=encodeNativeMigrant(f,e);need(bool(migrant),"wire migrant fixture encodes");full.figureRuntime->migrantPayloads={*migrant};full.runtimePolicies={"native_house_observed_dry_type4_to3_v1"};
 const auto v2=runtime(full);need(v2.rfind("RUNTIME8 2\n",0)==0,"explicit dry chooses v2");need(v2.find("policies ")<v2.find("observedDry ")&&v2.find("observedDry ")<v2.find("migrantPayloads ")&&v2.find("migrantPayloads ")<v2.find("objects "),"extension follows old stateFields before object records");
 NativeSessionState restored;need(decodeRuntime(v2,restored)&&restored.observedDry&&restored.figureRuntime,"v2 restores explicit state");need(restored.observedDry->pendingExcess==full.observedDry->pendingExcess&&restored.observedDry->departedSinceMeal==UINT32_MAX&&restored.observedDry->arrivedSinceMeal==19&&restored.figureRuntime->migrantPayloads==full.figureRuntime->migrantPayloads&&runtime(restored)==v2,"v2 full state and migrant bytes exact");
 auto reject=[&](const std::string& bytes){NativeSessionState out;need(!decodeRuntime(bytes,out),"invalid version/extension refuses");};
 reject(replaced(v2,"RUNTIME8 2","RUNTIME8 3"));reject(replaced(v2,"observedDry 1 pendingExcess","observedDry 0 pendingExcess"));reject(replaced(v2,"migrantPayloads 1 ","migrantPayloads 500001 "));reject(replaced(v2,"arrivedSinceMeal 19 ","arrivedSinceMeal 4294967296 "));reject(replaced(v2,"pendingExcess 2 7 4 18446744073709551615 4294967295 ","pendingExcess 2 7 4 7 5 "));reject(replaced(v2,"observedDry ","unknownDry "));reject(replaced(v2,"RUNTIME8 2","RUNTIME8 1"));reject(v2+"trailing");
 auto missing=full;missing.observedDry.reset();std::ostringstream denied;native_runtime_wire::encode(denied,missing);need(denied.fail()&&denied.str().empty(),"no-policy migrants cannot silently serialize as v1");
 auto noFigures=full;noFigures.figureRuntime.reset();std::ostringstream denied2;native_runtime_wire::encode(denied2,noFigures);need(denied2.fail()&&denied2.str().empty(),"explicit v2 requires figure pool");
 auto noMigrants=full;noMigrants.figureRuntime->migrantPayloads.clear();auto explicitEmpty=runtime(noMigrants);NativeSessionState decoded;need(decodeRuntime(explicitEmpty,decoded)&&decoded.observedDry&&decoded.figureRuntime->migrantPayloads.empty(),"v2 may have explicit empty migrants");
 auto oldFigures=full;oldFigures.observedDry.reset();oldFigures.figureRuntime->migrantPayloads.clear();const auto oldWire=runtime(oldFigures);auto reused=full;need(decodeRuntime(oldWire,reused)&&!reused.observedDry&&reused.figureRuntime->migrantPayloads.empty(),"v1 nonempty figure DTO clears stale migrant extension");
}

void nonemptyVersions(){
 // Captured BEFORE the v3 codec change: 4366-byte RUNTIME8 2 synthetic wire.
 // This is an independent fixed expected value, not encode/decode self-proof.
 NativeSessionState r2;r2.rulesVersion=8;r2.figureRuntime.emplace();r2.observedDry.emplace();
 r2.observedDry->pendingExcess={{7,4}};r2.observedDry->departedSinceMeal=8;r2.observedDry->arrivedSinceMeal=3;
 r2.runtimePolicies={"native_house_observed_dry_type4_to3_v1"};const auto frozen=runtime(r2);
 uint64_t expectedHash=14695981039346656037ULL;for(unsigned char c:frozen){expectedHash^=c;expectedHash*=1099511628211ULL;}
 need(frozen.size()==4366&&expectedHash==3574717259951154621ULL,"pre-v3 RUNTIME8 2 bytes remain identical");
 NativeSessionState oldRead;oldRead.rulesVersion=8;need(decodeRuntime(frozen,oldRead)&&!nativeUsesNonemptyMigrantRehousing(oldRead)&&runtime(oldRead)==frozen,"old v2 does not acquire nonempty policy");
 auto r3=r2;r3.runtimePolicies.push_back(nativeMigrantNonemptyTag);const auto bytes=runtime(r3);
 need(bytes.rfind("RUNTIME8 3\n",0)==0,"nonempty policy selects v3");
 const std::string r2Policies="policies 1 \"native_house_observed_dry_type4_to3_v1\" ";
 const std::string r3Policies="policies 2 \"native_house_observed_dry_type4_to3_v1\" \""+std::string(nativeMigrantNonemptyTag)+"\" ";
 need(replaced(replaced(bytes,"RUNTIME8 3\n","RUNTIME8 2\n"),r3Policies,r2Policies)==frozen,"v3 changes only version and explicit policy; v2 field layout stays fixed");
 NativeSessionState decoded;decoded.rulesVersion=8;need(decodeRuntime(bytes,decoded)&&decoded.observedDry&&nativeUsesNonemptyMigrantRehousing(decoded)&&runtime(decoded)==bytes,"v3 exact roundtrip carries policy and explicit state");
 auto reject=[&](const std::string& wire){NativeSessionState out;out.rulesVersion=8;need(!decodeRuntime(wire,out),"bad v3/version-policy combination rejected");};
 reject(replaced(bytes,"RUNTIME8 3\n","RUNTIME8 2\n"));
 reject(replaced(bytes,"RUNTIME8 3\n","RUNTIME8 1\n"));
 reject(replaced(bytes,r3Policies,r2Policies));
 reject(replaced(bytes,nativeMigrantNonemptyTag,"native_migrant_nonempty_rehousing_policy_v99"));
 reject(replaced(bytes,"observedDry 1 pendingExcess","observedDry 0 pendingExcess"));
 reject(replaced(bytes,"RUNTIME8 3\n","RUNTIME8 4\n"));
 reject(replaced(bytes,"pendingExcess 1 7 4 ","pendingExcess 2 7 4 7 5 "));
 reject(replaced(bytes,"arrivedSinceMeal 3 ","arrivedSinceMeal 4294967296 "));
 const std::string legacy="RUNTIME8 1\nschedule 0 \nfigures 0 \nhook 0 \nheldHouses 0 \nheldWalkers 0 \nstrictRecords 0 \nstrictContext 0 \npolicies 0 \nobjects 0\nENDRUNTIME8\n";
 reject(replaced(legacy,"policies 0 ","policies 1 \""+std::string(nativeMigrantNonemptyTag)+"\" "));
 auto noDry=r3;noDry.observedDry.reset();std::ostringstream refused;native_runtime_wire::encode(refused,noDry);need(refused.fail()&&refused.str().empty(),"nonempty tag cannot emit v1 without explicit dry state");
 auto wrongRules=r3;wrongRules.rulesVersion=7;std::ostringstream wrong;native_runtime_wire::encode(wrong,wrongRules);need(wrong.fail()&&wrong.str().empty(),"nonempty policy cannot encode outside rules8");
 auto migrant=r3;OriginalFigure f;f.type=13;f.alive=1;f.action=7;f.home=7;std::string error;auto payload=encodeNativeMigrant(f,error);need(bool(payload),"existing type13 codec supports linked action7 without changes");migrant.figureRuntime->migrantPayloads={*payload};const auto migration=runtime(migrant);NativeSessionState back;back.rulesVersion=8;need(decodeRuntime(migration,back)&&back.figureRuntime->migrantPayloads==migrant.figureRuntime->migrantPayloads,"v3 reuses v2 migrant field with linked home intact");
}

void recoveryVersions(){
 NativeSessionState r3;r3.rulesVersion=8;r3.figureRuntime.emplace();r3.observedDry.emplace();r3.runtimePolicies={nativeObservedDryTag,nativeMigrantNonemptyTag};const auto old=runtime(r3);
 auto r4=r3;r4.runtimePolicies.push_back(nativeObservedRewaterTag);const auto bytes=runtime(r4);need(bytes.rfind("RUNTIME8 4\n",0)==0,"R4 explicit recovery wire");NativeSessionState back;back.rulesVersion=8;need(decodeRuntime(bytes,back)&&nativeUsesObservedRewatering(back)&&runtime(back)==bytes,"R4 exact policy roundtrip");
 for(const char* v:{"RUNTIME8 1\n","RUNTIME8 2\n","RUNTIME8 3\n"}){NativeSessionState out;out.rulesVersion=8;need(!decodeRuntime(replaced(bytes,"RUNTIME8 4\n",v),out),"downgrade cannot silently enable recovery");}
 NativeSessionState out;out.rulesVersion=8;need(!decodeRuntime(replaced(old,"RUNTIME8 3\n","RUNTIME8 4\n"),out),"old R3 cannot acquire R4 through version change");
 auto missing=r4;missing.runtimePolicies.erase(std::remove(missing.runtimePolicies.begin(),missing.runtimePolicies.end(),nativeMigrantNonemptyTag),missing.runtimePolicies.end());std::ostringstream denied;native_runtime_wire::encode(denied,missing);need(denied.fail()&&denied.str().empty(),"recovery without R3 framing rejected");
 need(runtime(r3)==old,"R3 bytes unchanged after new policy codec");
}
}
int main(){try{migrants();wireVersions();nonemptyVersions();recoveryVersions();std::cout<<"{\"dry_codec_checks\":"<<checks<<",\"passed\":true}\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL:"<<e.what()<<"\n";return 1;}}
