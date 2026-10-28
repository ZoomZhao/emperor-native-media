// SPDX-License-Identifier: MIT
#include "native_immigrant_codec.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
using namespace game_lab;
namespace {
size_t checks=0;void need(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
std::string replaced(std::string s,const std::string& a,const std::string& b){const auto p=s.find(a);need(p!=std::string::npos,"mutation anchor");s.replace(p,a.size(),b);return s;}
std::string fields(OriginalFigure f){native_walker_detail::Writer w;native_walker_detail::visitWellWalker(w,f);return w.out;}
struct Fill{size_t n=0;template<class T>void operator()(const char*,T& x){++n;
 if constexpr(std::is_same_v<T,std::optional<int32_t>>)x=INT32_MIN;
 else if constexpr(std::is_same_v<T,bool>)x=true;
 else if constexpr(std::is_same_v<T,OriginalFigureClass>)x=OriginalFigureClass::Figure;
 else if constexpr(std::is_signed_v<T>)x=T(std::numeric_limits<T>::min()+int(n));
 else x=T(std::numeric_limits<T>::max()-n);}};
// Fixed legacy golden captured before this codec existed. Expected newcomer
// bytes are a documented field substitution on this independent constant,
// never the result of encodeNativeImmigrant itself.
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
}
int main(){try{
 std::string e;OriginalFigure old;old.slot=7;old.type=28;old.alive=1;old.action=1;old.owner=5;const auto well=encodeNativeWellWalker(old,e);need(well&&*well==oldWell,"old Well golden byte exact");
 auto expected=replaced(oldWell,"NATIVE-WELL-WALKER","NATIVE-IMMIGRANT");expected=replaced(expected,"type 28\n","type 11\n");expected=replaced(expected,"action 1\n","action 6\n");expected=replaced(expected,"owner 5\n","owner 0\n");expected=replaced(expected,"home 0\n","home 7\n");expected=replaced(expected,"employmentAmount 0\n","employmentAmount 4\n");
 OriginalFigure f=old;f.type=11;f.action=6;f.owner=0;f.home=7;f.employmentAmount=4;const auto golden=encodeNativeImmigrant(f,e);need(golden&&*golden==expected,"independent newcomer golden");auto read=decodeNativeImmigrant(expected,e);need(read&&fields(*read)==fields(f),"golden decodes all fields");
 Fill fill;OriginalFigure all;native_walker_detail::visitWellWalker(fill,all);need(fill.n==118,"all118 fields visited");all.slot=7;all.type=11;all.alive=1;all.owner=0;all.home=7;all.employmentAmount=4;all.runtimeClass=OriginalFigureClass::Figure;
 for(uint8_t action:{6,7,8}){all.action=action;const auto bytes=encodeNativeImmigrant(all,e);need(bool(bytes),"newcomer action encodes");const auto back=decodeNativeImmigrant(*bytes,e);need(back&&fields(*back)==fields(all),"all118 extreme fields exact roundtrip");}
 need(!decodeNativeWellWalker(expected,e)&&!decodeNativeMigrant(expected,e),"new magic never aliases old families");need(!decodeNativeImmigrant(oldWell,e),"new decoder rejects Well magic");auto migrant=f;migrant.type=13;const auto mb=encodeNativeMigrant(migrant,e);auto migrantExpected=replaced(expected,"NATIVE-IMMIGRANT","NATIVE-MIGRANT");migrantExpected=replaced(migrantExpected,"type 11\n","type 13\n");need(mb&&*mb==migrantExpected,"existing type13 fixed layout unchanged");need(!decodeNativeImmigrant(*mb,e),"new decoder rejects type13 magic");
 for(const auto& bad:{replaced(expected,"NATIVE-IMMIGRANT 1 118","NATIVE-IMMIGRANT 2 118"),replaced(expected,"NATIVE-IMMIGRANT 1 118","NATIVE-IMMIGRANT 1 117"),replaced(expected,"type 11\n","type 12\n"),replaced(expected,"type 11\n","type 011\n"),replaced(expected,"slot 7\n","slot 0\n"),replaced(expected,"slot 7\n","slot 2000\n"),replaced(expected,"home 7\n","home 0\n"),replaced(expected,"home 7\n","home 4000\n"),replaced(expected,"home 7\n","home 32768\n"),replaced(expected,"action 6\n","action 5\n"),replaced(expected,"action 6\n","action 9\n"),replaced(expected,"alive 1\n","alive 0\n"),replaced(expected,"owner 0\n","owner 1\n"),replaced(expected,"employmentAmount 4\n","employmentAmount 0\n"),replaced(expected,"employmentAmount 4\n","employmentAmount 256\n"),replaced(expected,"type 11\n","type 11\ntype 11\n"),replaced(expected,"type 11\n","unknown 11\n"),expected.substr(0,expected.size()-1),expected+"trailing 1\n",expected+std::string(1,'\0'),replaced(expected,"alive 1\n","alive 1\r\n"),std::string(16385,'x')})need(!decodeNativeImmigrant(bad,e),"malformed/foreign/overflow fields refused");
 for(uint8_t action:{0,5,9,255}){auto bad=f;bad.action=action;need(!encodeNativeImmigrant(bad,e),"encoder only normal newcomer actions6..8");}
 auto bad=f;bad.type=13;need(!encodeNativeImmigrant(bad,e),"encoder refuses other family");bad=f;bad.home=0;need(!encodeNativeImmigrant(bad,e),"encoder requires destination");bad=f;bad.owner=1;need(!encodeNativeImmigrant(bad,e),"encoder rejects unrelated owner");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"all_fields\":118,\"old_wire_unchanged\":true,\"native_runtime_not_enabled\":true}\n";return 0;
 }catch(const std::exception& x){std::cerr<<"FAIL:"<<x.what()<<'\n';return 1;}}
