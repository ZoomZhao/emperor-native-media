// SPDX-License-Identifier: MIT
#pragma once
#include "native_session_feedback.hpp"
#include "native_house_evolution.hpp"
#include <algorithm>
#include <sstream>
namespace game_lab {
inline double nativePresentationInterval(unsigned speed){return 1.0/(speed==2?2:speed==4?4:1);}
inline unsigned nativeToolCategory(unsigned tool){return tool==2?0:tool==4?1:tool==3?2:3;}
inline bool nativeToolAvailable(unsigned tool,unsigned version){return tool<=5&&!(tool==3&&version<2)&&!(tool==4&&version<3)&&!(tool==5&&version<4);}
inline bool nativeCategoryAvailable(unsigned category,unsigned version){return category<4&&(category!=1||version>=3)&&(category!=2||version>=2);}
inline std::string nativeToolUnavailableReason(unsigned tool,unsigned version){
 if(tool>5)return "此工具不存在。";
 if(nativeToolAvailable(tool,version))return {};
 const std::string rules="当前规则 v"+std::to_string(version);
 if(tool==3)return rules+" 不支持水井；载入旧档不会自动升级。";
 if(tool==4)return rules+" 不支持猎人营地；载入旧档不会自动升级。";
 if(tool==5)return rules+" 不支持拆除；载入旧档不会自动升级。";
 return rules+" 不支持此工具；载入旧档不会自动升级。";
}
inline std::string nativeCategoryUnavailableReason(unsigned category,unsigned version){
 if(category>=4)return "此原版类别尚未实现，不能建造。";
 if(nativeCategoryAvailable(category,version))return {};
 const std::string rules="当前规则 v"+std::to_string(version);
 if(category==1)return rules+" 不支持食物生产；载入旧档不会自动升级。";
 if(category==2)return rules+" 不支持供水；载入旧档不会自动升级。";
 return rules+" 不支持此类别；载入旧档不会自动升级。";
}
inline std::string nativeCategoryHint(unsigned category){
 if(category==0)return "住宅\n点地图上的自建住宅查看入住、邻路和供水状态；选择“住宅”建造 2×2。";
 if(category==1)return "食物\n点猎人营地查看工人与产出；营地邻路且生产开启才会产肉。";
 if(category==2)return "水井\n点水井查看是否邻路启用；供水覆盖范围外扩 4 格。";
 return "道路与拆除\n拖动铺路；点自建对象查看状态，拆除需暂停并确认。";
}
inline std::string nativeCategorySummary(const NativeSessionSeed& seed,const NativeSessionState& state,unsigned category,const NativeCityFeedback& q){
 if(!q.error.empty())return "当前状态无法评估："+q.error;
 std::ostringstream text;
 if(category==0){
  std::size_t houses=0,connected=0,watered=0,blocked=0;unsigned long long residents=0;
  for(const auto& o:q.objects)if(o.kind==NativeObjectKind::House){
   ++houses;residents+=o.residents;connected+=o.connected;watered+=o.watered;
   blocked+=(!o.connected||(state.rulesVersion>=2&&!o.watered)||o.foodShortage);}
  text<<"住宅 "<<houses<<" 座 · 住民 "<<residents<<"/"<<houses*seed.houseCapacity<<"\n";
  if(state.rulesVersion>=2)text<<"邻路 "<<connected<<" · 有水 "<<watered<<" · 入住受阻 "<<blocked<<"\n";
  else text<<"邻路 "<<connected<<" · 入住受阻 "<<blocked<<"\n";
  text<<(state.paused?"已暂停；可查看和建造。":"运行中；点住宅查看入住条件。");
 }else if(category==1){
  std::size_t hunters=0;for(const auto& o:q.objects)if(o.kind==NativeObjectKind::Hunter)++hunters;
  text<<"肉 "<<q.foodStock<<"/"<<nativeFoodCapacity<<" · 营地 "<<hunters<<"\n";
  if(state.rulesVersion>=3){
   text<<"下次供餐 "<<q.untilMeal<<" 更新 · 产 "<<q.food.cycle.produced<<" / 需 "<<q.food.cycle.demand<<"\n";
   text<<(state.huntingEnabled?"生产开启":"生产暂停");
   if(state.rulesVersion>=6)text<<" · 劳力 "<<q.labor.available<<" · 岗位 "<<q.labor.demand;
  }else text<<"旧规则不运行食物生产/消费";
 }else if(category==2){
  if(state.rulesVersion<2)text<<"旧规则：供水不参与入住，水井不可建。";
  else{
   std::size_t wells=0,active=0,wet=0,dry=0;unsigned long long workers=0,required=0;
   for(const auto& o:q.objects){
    if(o.kind==NativeObjectKind::Well){++wells;active+=o.connected;workers+=o.workers;required+=o.requiredWorkers;}
    else if(o.kind==NativeObjectKind::House){wet+=o.watered;dry+=!o.watered;}
   }
   text<<"井 "<<wells<<" · 启用 "<<active<<" · 断路 "<<wells-active<<"\n";
   text<<"住宅有水 "<<wet<<" / 无水 "<<dry<<"\n";
   if(state.rulesVersion>=6)text<<"水井工人 "<<workers<<"/"<<required<<" · ";
   text<<"井需邻路，供水外扩 "<<nativeWaterRadius<<" 格";
  }
 }else{
  std::size_t roads=0,connected=0;
  for(const auto& o:q.objects){roads+=o.kind==NativeObjectKind::Road;if(o.kind==NativeObjectKind::House&&o.connected)++connected;}
  text<<"道路 "<<roads<<" 格 · 邻路住宅 "<<connected<<"\n";
  if(state.rulesVersion>=5){
   const auto& t=q.tax.cycle;
   text<<"税倒计 "<<q.untilTax<<"：";
   if(q.tax.error.empty())text<<"约"<<t.income<<"金";else text<<"溢出拒绝";
   text<<" · 邻路 "<<t.eligible<<" 人\n";
  }
  text<<(state.paused?"已暂停；拆除无退款。":"运行中；拆除前需暂停。");
 }
 return text.str();
}
inline std::string nativeObjectDescription(const NativeSessionSeed& seed,const NativeSessionState& state,uint64_t id,const NativeCityFeedback& q){
 if(!q.error.empty())return "当前会话无法评估："+q.error;
 const auto*f=q.find(id);if(!f)return "点击自建建筑查看详情。原始地表只读，不可拆改。";
 std::ostringstream text;text<<(f->kind==NativeObjectKind::Road?"道路":f->kind==NativeObjectKind::House?"普通住宅":f->kind==NativeObjectKind::Well?"水井":"猎人营地")<<" #"<<id<<"\n";
 if(f->kind==NativeObjectKind::House){
  text<<"住民 "<<f->residents<<" / "<<seed.houseCapacity<<"\n";
  text<<(f->connected?"邻路":"断路");if(state.rulesVersion>=2)text<<" · "<<(f->watered?"有水":"缺水");text<<"\n";
  if(f->full)text<<"已满员，不再入住。";
  else if(!f->connected||!f->watered||f->foodShortage){
   text<<"入住受阻：";bool comma=false;const auto cause=[&](const char*t){if(comma)text<<"、";text<<t;comma=true;};
   if(!f->connected)cause("断路");if(!f->watered)cause("缺水");if(f->foodShortage)cause("缺粮");
   if(f->foodShortage)text<<(q.food.cycle.fed<q.food.cycle.demand?"\n先满足现有住民供餐。":"\n余粮不足；按住宅编号分配。");
  }else if(q.mealBlockedByTax)text<<"本轮将拒绝：税收溢出。";
  else text<<"等待 "<<q.untilMeal<<" 次更新后入住+1。";
  if(state.paused)text<<"\n已暂停；恢复后推进。";
  if(state.rulesVersion>=6&&q.food.laborOverride)text<<"\n缺粮但营地缺工：继续入住以补充劳动力。";
  if(state.rulesVersion>=3)text<<"\n按现状估计；缺粮不迁出。";
 }else if(f->kind==NativeObjectKind::Well){
  text<<(f->connected?"供水启用 · 邻路":"供水停用 · 断路（未邻路）")<<"\n覆盖占地外扩 "<<nativeWaterRadius<<" 格。\n无需食物；不受生产开关影响。";
  if(state.rulesVersion>=6)text<<"\n工人 "<<f->workers<<" / "<<f->requiredWorkers<<"（原版模型；v6不影响供水范围）";
 }else if(f->kind==NativeObjectKind::Hunter){
  if(!f->connected||!state.huntingEnabled){text<<"生产停用：";if(!f->connected)text<<"断路";if(!state.huntingEnabled)text<<(!f->connected?"、":"")<<"狩猎已暂停";}
  else if(q.foodStock==nativeFoodCapacity)text<<"库存已满，本轮不产。";
  else text<<"已启用，等待供餐轮生产。";
  if(state.rulesVersion>=6)text<<"\n工人 "<<f->workers<<" / "<<f->requiredWorkers<<" · 本轮可产 "<<(f->requiredWorkers?nativeFoodYield*f->workers/f->requiredWorkers:0)<<" 肉";
  text<<"\n每2更新最多产"<<nativeFoodYield<<"肉"<<(state.rulesVersion>=6?"（按工人比例取整）":"")<<"。\n城市先生产，再供餐。";
  if(state.paused)text<<"\n已暂停；恢复后推进。";
 }else text<<"本会话道路；已有路不重复收费。";
 text<<(state.rulesVersion>=4?"\n拆除无退款。":"\n旧版本不支持拆除，不自动升级。");return text.str();
}
inline std::string nativeObjectDescription(const NativeSessionSeed& seed,const NativeSessionState& state,uint64_t id){return nativeObjectDescription(seed,state,id,queryNativeCityFeedback(seed,state));}
inline std::string nativeCityFeedbackSummary(const NativeSessionState& state,const NativeCityFeedback& q){
 if(!q.error.empty())return "无法评估："+q.error;
 std::ostringstream text;
 if(state.rulesVersion>=3){text<<"肉"<<q.foodStock<<"/"<<nativeFoodCapacity<<" · 餐倒计"<<q.untilMeal<<"\n预计产"<<q.food.cycle.produced<<" / 需"<<q.food.cycle.demand<<"\n";
  if(q.mealBlockedByTax)text<<"税溢出，本轮不更新";
  else if(q.food.cycle.fed<q.food.cycle.demand)text<<"缺"<<q.food.cycle.demand-q.food.cycle.fed<<"份，"<<(q.food.laborOverride?"营地缺工，继续入住":"入住停增");
  else text<<"预计新入住"<<q.food.cycle.arrivals<<"人";
 }else text<<"旧规则：不运行食物消费";
 if(state.rulesVersion>=6)text<<"\n劳力"<<q.labor.available<<" · 岗位"<<q.labor.demand<<" · 就业"<<q.labor.employed<<(q.labor.shortage?" · 缺工"+std::to_string(q.labor.shortage):std::string());
 if(state.rulesVersion>=5){text<<"\n税倒计"<<q.untilTax<<"：";if(q.tax.error.empty())text<<"约"<<q.tax.cycle.income<<"金";else text<<"溢出拒绝";text<<"\n邻路"<<q.tax.cycle.eligible<<"人×"<<nativeTaxPerResident;}
 return text.str();
}
inline std::string nativeCityFeedbackExplanation(const NativeSessionState& state,const NativeCityFeedback& q){
 if(!q.error.empty())return "无法评估："+q.error;
 std::ostringstream text;text<<"按当前人口、道路、供水与生产状态估计；倒计单位为城市更新次数；期间建设或人口改变会重新计算。暂停不推进周期。";
 if(state.rulesVersion>=3)text<<"\n下次供餐距"<<q.untilMeal<<"更新："<<q.activeHunters<<"个启用营地最多产"<<q.productionPotential<<"肉；库存上限使实际可产"<<q.food.cycle.produced<<"。现有住民需求"<<q.food.cycle.demand<<"，可供"<<q.food.cycle.fed<<"，再按住宅编号安排"<<q.food.cycle.arrivals<<"人入住，预计余粮"<<q.food.stock<<"。";
 if(state.rulesVersion>=6)text<<"\n劳动力"<<q.labor.available<<"人＝住民×原版就业比例；岗位"<<q.labor.demand<<"（原版模型：营地/水井各需定员），就业"<<q.labor.employed<<"，缺工"<<q.labor.shortage<<"，失业"<<q.labor.unemployed<<"。营地按工人比例产肉；营地缺工时缺粮不阻止入住。";
 if(state.rulesVersion>=5)text<<"\n下次税收距"<<q.untilTax<<"更新：按当前邻路"<<q.tax.cycle.eligible<<"人 × "<<nativeTaxPerResident<<"金 = "<<q.tax.cycle.income<<"金；断路"<<q.tax.cycle.disconnected<<"人不缴，空房"<<q.tax.cycle.emptyHouses<<"户。税收先于同轮新入住；届时人口可能改变。";
 return text.str();
}
inline std::string nativeHouseEvolutionMissingLabel(const std::string& name){
 if(name=="house_water_phase28")return "原版房屋供水状态未接通";
 if(name=="house_desirability_bonus_60")return "原版运行时吸引力加成未接通";
 if(name.rfind("house_runtime_col",0)==0)return "原版房屋运行服务/库存状态未接通";
 if(name=="house_model_row1")return "目标住宅模型行未接通";
 if(name=="house_model_row0")return "当前住宅模型行未接通";
 if(name=="invalid_saved_building_update_inputs")return "原版建筑输入不完整";
 if(name=="explicit_phase_terrain_disabled_context_required")return "原版条件上下文不完整";
 if(name=="house_evolution_519120_518ef0")return "原版住宅演化条件不足";
 return "原版条件输入未接通";
}
inline std::string nativeHouseEvolutionErrorText(const std::string& error){
 if(error=="source_unreadable")return "原版来源不可读取";
 if(error=="source_changed")return "原版来源已变化";
 if(error=="model_changed")return "住宅模型已变化";
 if(error=="saved_difficulty_unreadable")return "存档难度不可读取";
 if(error=="models_unparsed")return "住宅模型未解析";
 if(error=="scene_invalid")return "原生场景不可读取";
 return "原版条件输入未接通";
}
inline const char* nativeHouseDirectionLabel(int direction){return direction>0?"上升":direction<0?"下降":"不变化";}
inline const char* nativeHouseDirectionShortLabel(int direction){return direction>0?"上升":direction<0?"下降":"不变";}
struct NativeHouseEvolutionText {std::string summary,detail;};
inline NativeHouseEvolutionText nativeHouseEvolutionText(const NativeHouseEvolutionView& v){
 NativeHouseEvolutionText out;
 std::ostringstream d;d<<"原版公式判定 · 未调度 · 不改变城市";
 const auto appendNative=[&](std::ostringstream& text,const std::optional<bool>& watered){
  text<<"\n原生供水覆盖："<<(!watered?"不可判定":*watered?"有":"无")<<"（自写规则，非原版）";
 };
 if(v.emptyEarlyReturn){
  out.summary="空住宅·只读·不评估升降";
  d<<"\n空住宅：原版在住民为0时提前返回，不评估升降。";
  appendNative(d,v.nativeWatered);
  d<<"\n原版实机一致仍未验证；本会话不自动升级。";
  out.detail=d.str();return out;
 }
 // One physical line at the existing 154pt panel width. Full thresholds and
 // requirement detail stay in the tooltip.
 std::ostringstream s;s<<"只读·";
 if(v.desirability)s<<"吸引力"<<*v.desirability;
 else s<<"吸引力未算";
 s<<"·";
 if(v.direction)s<<nativeHouseDirectionShortLabel(*v.direction);
 else s<<"升降未定";
 out.summary=s.str();
 if(v.desirability){
  d<<"\n吸引力：基础 "<<*v.desirability;
  if(v.desirabilityWithBonus)d<<"；运行时加成后 "<<*v.desirabilityWithBonus;
  else d<<"；运行时加成后不可算";
 }
 if(v.currentThresholdDown&&v.currentThresholdUp)
  d<<"\n当前阈值：下降 ≤ "<<*v.currentThresholdDown<<"；上升 ≥ "<<*v.currentThresholdUp;
 if(v.currentMet)d<<"\n当前需求："<<(*v.currentMet?"满足":"不满足");
 if(v.nextMet)d<<"\n目标需求："<<(*v.nextMet?"满足":"不满足");
 if(v.direction)d<<"\n方向："<<nativeHouseDirectionLabel(*v.direction)<<"（严格函数）";
 else d<<"\n方向：未判定";
 if(v.directionIfWater||v.directionIfNoWater){
  d<<"\n原版判定·若有水："<<(v.directionIfWater?nativeHouseDirectionLabel(*v.directionIfWater):"不可判定");
  d<<"\n原版判定·若无水："<<(v.directionIfNoWater?nativeHouseDirectionLabel(*v.directionIfNoWater):"不可判定");
  if(v.directionIfWater&&v.directionIfNoWater){
   if(*v.directionIfWater==*v.directionIfNoWater)d<<"\n原版供水两种结果相同；实际供水状态仍未接通。";
   else d<<"\n差别仅在原版供水；原版供水由送水人行经写入，原生无行人，未接通。";
  }
 }
 if(v.currentMetIfWater&&v.currentMetIfNoWater&&*v.currentMetIfWater!=*v.currentMetIfNoWater)
  d<<"\n当前需求：若有水"<<(*v.currentMetIfWater?"满足":"不满足")<<"，若无水"<<(*v.currentMetIfNoWater?"满足":"不满足");
 if(v.nextMetIfWater&&v.nextMetIfNoWater&&*v.nextMetIfWater!=*v.nextMetIfNoWater)
  d<<"\n目标需求：若有水"<<(*v.nextMetIfWater?"满足":"不满足")<<"，若无水"<<(*v.nextMetIfNoWater?"满足":"不满足");
 appendNative(d,v.nativeWatered);
 std::vector<std::string> missing;
 for(const auto& name:v.missing){const auto label=nativeHouseEvolutionMissingLabel(name);
  if(std::find(missing.begin(),missing.end(),label)==missing.end())missing.push_back(label);}
 if(!missing.empty()){d<<"\n未接通：";for(size_t n=0;n<missing.size();++n){if(n)d<<"；";d<<missing[n];}}
 std::vector<std::string> notNeeded;
 for(const auto& name:v.notNeeded)if(name=="house_water_phase28")notNeeded.push_back("供水状态");
 for(const auto& name:v.notNeeded)if(name=="house_desirability_bonus_60")notNeeded.push_back("运行时加成");
 if(!notNeeded.empty()){d<<"\n已确定不影响方向判定：";for(size_t n=0;n<notNeeded.size();++n){if(n)d<<"、";d<<notNeeded[n];}}
 if(v.direction&&*v.direction<0)d<<"\n原版降级有9次延迟；原生无此状态。";
 d<<"\n原版实机一致仍未验证；本会话不自动升级。";
 out.detail=d.str();return out;
}

} // namespace game_lab
