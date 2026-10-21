"""Private figure mutations must fail authored pixel/wire/index regressions."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--only', help='Run one named mutation')
    args = parser.parse_args()
    work = Path(tempfile.mkdtemp(prefix='figure-scene-bite-'))
    mutations = [
        ('index', 'original_building_scene.hpp', '(word&0x3fff)+1;', '(word&0x3fff)+201;'),
        ('position', 'original_building_scene.hpp', 'ox-s.animationOffsetX,oy-s.animationOffsetY', 'ox+s.animationOffsetX,oy-s.animationOffsetY'),
        ('order', 'original_building_scene.hpp', 'return a.depth<b.depth;', 'return b.depth<a.depth;'),
        ('stride', 'original_figure.hpp', 'base+frame*directions+direction;', 'base+direction*frames+frame;'),
        ('wire', 'original_figure.hpp', 'f.action=r[64];', 'f.action=r[65];'),
        ('selector', 'original_figure_image.hpp', 'code=f.subtype?0x4c13:0x4c15;', 'code=f.subtype?0x4c15:0x4c13;'),
        ('leader', 'original_figure_image.hpp', 'leader->amount>400;', 'leader->amount>=400;'),
        ('attack', 'original_figure_image.hpp', 'code=distance>2?0x4ebf:0x4ec0;', 'code=distance>3?0x4ebf:0x4ec0;'),
        ('cargo', 'original_figure_image.hpp', 'goods->amount<=200?0:', 'goods->amount<200?0:'),
        ('first_tick', 'original_figure_image.hpp', 'f.frame=uint8_t(f.frame+1);', 'f.frame=uint8_t(f.frame);'),
        ('post_load', 'original_figure_image.hpp', 'out.images=std::make_pair(0u,0u);', 'out.images=std::make_pair(0u,1u);'),
        ('type11_callback', 'original_figure_image.hpp', 'f.action==4?0x4c14:0x4c13,archives', 'f.action==4?0x4c16:0x4c15,archives'),
        ('frame_reset', 'original_figure_update.hpp', 'if(int(out.counter)>=frames)out.counter=0;\n    if(speedArgument)', 'if(int(out.counter)>=frames)out.counter=2;\n    if(speedArgument)'),
        ('effect_position', 'original_figure_update.hpp', 'int(f.effectX)/8+int(f.x)*15', 'int(f.effectX)/8+int(f.x)*20'),
        ('scheduler', 'original_figure_update.hpp', ']+2)return out;', ']+1)return out;'),
        ('pheasant_idle', 'original_figure_update.hpp', 'if(idle>=200)f.action=30;', 'if(idle>200)f.action=30;'),
        ('pheasant_cooldown', 'original_figure_update.hpp', 'if(cooldown>0)--cooldown;', 'if(cooldown>=0)--cooldown;'),
        ('scheduler_load', 'original_figure_update.hpp', 'if(!networkActive)live.speed=personalSpeed;', 'if(networkActive)live.speed=personalSpeed;'),
        ('route_phase', 'original_figure_update.hpp', 'phase>(speed==4?1:2)?0:1;', 'phase>(speed==4?1:3)?0:1;'),
        ('route_step', 'original_figure_update.hpp', '++f.progress;', 'f.progress+=2;'),
        ('pool_order', 'original_figure_update.hpp', 'for(size_t slot=1;slot<2000;++slot)', 'for(size_t slot=1999;slot>0;--slot)'),
        ('rng_saved', 'original_figure.hpp', 'first=1299340,second=1299344;', 'first=1299344,second=1299340;'),
        ('rng_step', 'original_figure_update.hpp', 'value>>4)^value', 'value>>3)^value'),
        ('passability_water', 'original_figure_update.hpp', 'else if(t&0x40)mask=4;', 'else if(t&0x40)mask=0;'),
        ('reachability', 'original_figure_update.hpp', 'constexpr int offsets[]={-228,1,228,-1};', 'constexpr int offsets[]={-229,1,229,-1};'),
        ('spawn_offset', 'original_figure.hpp', 'at+853+4*n', 'at+853+2*n'),
        ('destination_rng', 'original_figure_update.hpp', 'x+=int(14*scale/4)', 'x+=int(14*scale/2)'),
        ('ordered_rng', 'original_figure_update.hpp', 'if(result.services)context->services=result.services;', 'if(result.services)context->services=before.services;'),
        ('crossing_prior', 'original_figure_update.hpp', 'next.priorX=f.x;next.priorY=f.y;', 'next.priorX=f.x+1;next.priorY=f.y;'),
        ('cart_delay', 'original_figure_update.hpp', 'saved.action==9?13:18;', 'saved.action==9?13:17;'),
        ('segment_cursor', 'original_figure_update.hpp', 'f.routeCursor=originalFigureWord(int(f.routeCursor)+1);', 'f.routeCursor=originalFigureWord(int(f.routeCursor)+2);'),
        ('worker_accepting', 'original_figure.hpp', 'link.accepting=data[at+7];', 'link.accepting=data[at+8];'),
        ('supplier_countdown', 'original_figure_update.hpp', 'base+unsigned(f.action==8)', 'base+unsigned(f.action==7)'),
        ('roam_accounting', 'original_figure_update.hpp', 'int(f.roamSteps)+int(speed)', 'int(f.roamSteps)+1'),
        ('free_arrival', 'original_figure_update.hpp', 'const bool arrived=int(f.freeRemainingX)+f.freeRemainingY<1;', 'const bool arrived=int(f.freeRemainingX)+f.freeRemainingY<=1;'),
        ('follower_speed', 'original_figure_update.hpp', 'case 13:return 2-unsigned(phase>2);', 'case 13:return 2+unsigned(phase>2);'),
        ('saved_fire_offset', 'original_figure.hpp', 'fireCount=fireBlocks80+2*80;', 'fireCount=fireBlocks80+2*80-4;'),
        ('inspector_fire_gate', 'original_figure_update.hpp', 'if(*context->services->fireCount>0)', 'if(*context->services->fireCount>=0)'),
        ('inspector_fire_threshold', 'original_figure_update.hpp', 'if(f.searchCounter>=20)', 'if(f.searchCounter>20)'),
        ('worker_employment', 'original_figure_update.hpp', 'int(house.residents)+f.employmentAmount', 'int(house.residents)+1'),
        ('entertainment_grid', 'original_figure_update.hpp', 'serviceGrid[f.cell]|=7u<<9;', 'serviceGrid[f.cell]|=7u<<6;'),
        ('population_frame', 'original_figure.hpp', 'const size_t at=sizeAnchor-28-152-bytes;', 'const size_t at=sizeAnchor-28-152-bytes+4;'),
        ('worker_removal', 'original_figure_update.hpp', 'reset.priorProgress=f.priorProgress;', 'reset.priorProgress=0;'),
        ('load_population', 'original_figure_update.hpp', 'services.population->total=int32_t(total);', 'services.population->total+=int32_t(total);'),
        ('zero_gate', 'original_building_scene.hpp', 'if(!f.image){', 'if(!f.image&&source==OriginalFigureRenderSource::Saved){'),
        ('effect_draw_cell', 'original_figure.hpp', '(f.type==1?229u:0u)', '(f.type==1?228u:0u)'),
        ('effect_extra_height', 'original_figure.hpp', 'x+y-int(f.raise)+f.extraY-int(f.height)', 'x+y-int(f.raise)+f.extraY'),
        ('animation_clock', 'original_building_animation.hpp', 'uint32_t(20*k)', 'uint32_t(19*k)'),
        ('timeline_repeat', 'original_scene_timeline.hpp', 'context=*out.lastPass.context;++out.updates;', 'context=initial;++out.updates;'),
        ('timeline_draw', 'original_scene_timeline.hpp', 'out.animationClock.flags);', 'std::array<bool,51>{});'),
        ('engine_route_tie', 'original_figure_update.hpp', 'd==toward||chosen==8', 'd==((toward+4)&7)||chosen==8'),
        ('movement_full_mask', 'original_figure_update.hpp', 'out.cells[cell]=mask;', 'out.cells[cell]=mask&0x51c;'),
        ('delivery_flags', 'original_figure_update.hpp', 'cropDelivery->flags|=0x40;', 'cropDelivery->flags|=0x20;'),
        ('watch_risk', 'original_figure_update.hpp', 'uint32_t(h.watchRisk)-uint32_t(*context.watchtowerReduction)', 'uint32_t(h.watchRisk)+uint32_t(*context.watchtowerReduction)'),
        ('elevated_lookahead', 'original_figure_update.hpp', '((flags&0x800000)?2:1)', '((flags&0x800000)?1:1)'),
        ('watch_category', 'original_figure_update.hpp', 'const unsigned shift=f.type==29?6:(f.type==28||f.type==30)?0:f.type==39?3:f.type==23?21:f.type==35?15:9; //84E78C', 'const unsigned shift=f.type==29?9:(f.type==28||f.type==30)?0:f.type==39?3:f.type==23?21:f.type==35?15:9; //84E78C'),
        ('service_shadow', 'original_figure_update.hpp', 'std::abs(dx)+std::abs(dy)>shadows[size_t(sector)]', 'std::abs(dx)+std::abs(dy)>100'),
        ('crop_season', 'original_figure_update.hpp', 'month<ends[type-0xc3]', 'month<=ends[type-0xc3]'),
        ('market_smallest', 'original_figure_update.hpp', 'storage[k].amount<minimum', 'storage[k].amount>minimum'),
        ('goods_smallest', 'original_figure_update.hpp', 'c.amount<minimum){minimum=c.amount;selected=k;}', 'c.amount>minimum){minimum=c.amount;selected=k;}'),
        ('goods_remainder', 'original_figure_update.hpp', 'return wanted;\n}\n// Warehouse', 'return 0;\n}\n// Warehouse'),
        ('goods_accounting', 'original_figure_update.hpp', 'goods.warehouse[resource]-=take;', 'goods.warehouse[resource]+=take;'),
        ('goods_delay', 'original_figure_update.hpp', 'if(f.counter>4){if(!local.goods)', 'if(f.counter>=4){if(!local.goods)'),
        ('goods_policy', 'original_figure.hpp', 'at+198,36,policy.trade.begin()', 'at+197,36,policy.trade.begin()'),
        ('goods_cache', 'original_figure_update.hpp', 'context->goods->savedPhase==2', 'context->goods->savedPhase==1'),
        ('storage_reselection', 'original_scene_timeline.hpp', 'out.buildingStorage[p.footprint.slot]=*after->second.storage;++out.buildingImageReselections;', 'out.buildingStorage[p.footprint.slot]=*after->second.storage;out.buildingImageReselections+=0;'),
        ('watch_criminal', 'original_figure_update.hpp', 'if(f.type==29&&(!local.services||!local.services->criminalCount||*local.services->criminalCount>0))', 'if(f.type==29&&(!local.services||!local.services->criminalCount||*local.services->criminalCount>=0))'),
        ('delegated_target', 'original_figure_update.hpp', 'f.freeRemainingX==f.freeRemainingY?0:', 'f.freeRemainingX==f.freeRemainingY?1:'),
        ('well_water', 'original_figure_update.hpp', 'h.waterService->at(index)=96;', 'h.waterService->at(index)=95;'),
        ('roam_owner', 'original_figure_update.hpp', 'b->second.roamDirection=*d>6?0:uint8_t(*d);', 'b->second.roamDirection=0;'),
        ('projectile_fifo', 'original_figure_update.hpp', 'const size_t candidate=queue.front();queue.erase(queue.begin());', 'const size_t candidate=queue.back();queue.pop_back();'),
        ('projectile_steps', 'original_figure_update.hpp', 'for(unsigned n=0;n<7;++n){const auto moved=', 'for(unsigned n=0;n<6;++n){const auto moved='),
        ('projectile_selector', 'original_figure_image.hpp', 'direction=(direction+4-4*int(view)+32)%32;', 'direction=(direction+3-4*int(view)+32)%32;'),
        ('projectile_arc', 'original_figure.hpp', 'int(ratio*f.projectileStartHeight);', 'int(ratio*f.projectileStartHeight)+1;'),
        ('well_festival', 'original_figure.hpp', 'detail::u32(data.data()+festival+71)==4;', 'detail::u32(data.data()+festival+71)==3;'),
        ('roadblock_gate', 'original_figure_update.hpp', 'if(!*context.roadblockGate)return true;', 'if(*context.roadblockGate)return true;'),
        ('crt_seed_required', 'original_figure_update.hpp', 'if(!context.crtRandom)return OriginalFigureUpdateStatus::CrtRandomRequired;', 'if(!context.crtRandom)context.crtRandom=1;'),
        ('crt_step', 'original_figure_update.hpp', '*context.crtRandom*214013u+2531011u', '*context.crtRandom*214013u+2531012u'),
        ('crt_status', 'original_figure_update.hpp', 'if(!context.crtRandom)return OriginalFigureUpdateStatus::CrtRandomRequired;', 'if(!context.crtRandom)return OriginalFigureUpdateStatus::Projectile;'),
        ('crt_zero', 'original_figure_update.hpp', 'if(!context.crtRandom)return std::nullopt;', 'if(!context.crtRandom||!*context.crtRandom)return std::nullopt;'),
        ('saved_scenario_gate', 'original_figure.hpp', 'data.data()+scenario+704', 'data.data()+scenario+702'),
        ('warehouse_fullest', 'original_figure_update.hpp', 'int(c.amount)>maximum', 'int(c.amount)<maximum'),
        ('warehouse_credit', 'original_figure_update.hpp', '++goods.warehouse[resource];', '--goods.warehouse[resource];'),
        ('inspector_cleanup', 'original_figure_update.hpp', 'if(owner==context.buildings->end()||owner->second.type!=(f.type==39?0x7c:0x7f))return false;\n        if(owner->second.primaryFigure==int(f.slot))owner->second.primaryFigure=0;\n        else if(owner->second.secondaryFigures[1]==int(f.slot))owner->second.secondaryFigures[1]=0;', 'if(owner==context.buildings->end()||owner->second.type!=(f.type==39?0x7c:0x7f))return false;\n        if(owner->second.primaryFigure==int(f.slot))owner->second.primaryFigure=0;\n        else if(owner->second.secondaryFigures[1]==int(f.slot))owner->second.secondaryFigures[0]=0;'),
        ('hunter_search_order', 'original_figure_update.hpp', 'for(auto p=candidates.rbegin();p!=candidates.rend();++p)', 'for(auto p=candidates.begin();p!=candidates.end();++p)'),
        ('goods_return', 'original_figure_update.hpp', 'f.action=13;f.counter=0;f.destinationX=f.homeX;f.destinationY=f.homeY;f.amount=0;', 'f.action=12;f.counter=0;f.destinationX=f.homeX;f.destinationY=f.homeY;f.amount=0;'),
        ('delegated_expiry', 'original_figure_update.hpp', 'if(f.roamSteps>=f.roamLimit){ //51D13C', 'if(f.roamSteps>f.roamLimit){ //51D13C'),
        ('cart_orphan', 'original_figure_update.hpp', 'if(n==7||goods->second.leaderLink==0)return missingGoods();', 'if(n==7||goods->second.leaderLink==0)return out;'),
        ('road_route_mask', 'original_figure_update.hpp', 'f.destinationY,0xb0c,4,&reached)', 'f.destinationY,0x51c,4,&reached)'),
        ('projectile_empty_shadow', 'original_figure_update.hpp', 'f.secondaryImage=arrived?0:cargo.image;', 'f.secondaryImage=cargo.image;'),
        ('delegated_route_mask', 'original_figure_update.hpp', 'f.destinationY,0x12c,4,&reached)', 'f.destinationY,0xb0c,4,&reached)'),
        ('warehouse_return', 'original_figure_update.hpp', 'if(f.type==22&&f.action==9&&f.direction==8)', 'if(f.type==22&&f.action==9&&f.direction==9)'),
        ('hunter_return_goods', 'original_figure_update.hpp', 'int(b.hunterReturnGoods)+50', 'int(b.hunterReturnGoods)+49'),
        ('hunter_retarget', 'original_figure_update.hpp', '(f.direction==9||f.direction==8)&&f.action==7', 'f.direction==9&&f.action==7'),
        ('impact_health', 'original_figure_update.hpp', 'prey.damage>uint32_t(defense->second[1])', 'prey.damage>=uint32_t(defense->second[1])'),
        ('impact_damage', 'original_figure_update.hpp', 'prey.damage+=uint32_t(std::max<int64_t>(0,raw));', 'prey.damage+=uint32_t(std::max<int64_t>(0,raw))+1;'),
        ('impact_attacker', 'original_figure_update.hpp', 'prey.attackers[0]=f.owner;', 'prey.attackers[0]=int16_t(slot);'),
        ('impact_audio', 'original_figure_update.hpp', '!context.soundEffectsEnabled||!context.services', '!context.soundEffectsEnabled||*context.soundEffectsEnabled||!context.services'),
        ('corpse_audio', 'original_figure_update.hpp', 'if(!f.counter&&!context->soundEffectsEnabled)return out;', 'if(!f.counter&&(!context->soundEffectsEnabled||*context->soundEffectsEnabled))return out;'),
        ('corpse_limit', 'original_figure_update.hpp', 'if(f.counter>500)f.action=4;', 'if(f.counter>=500)f.action=4;'),
        ('retained_cargo', 'original_figure_update.hpp', 'f.amount=originalFigureWord(int(f.amount)-int(*delivered));', 'f.amount=0;'),
        ('impact_formation', 'original_figure_update.hpp', 'formation.alert=6;formation.source=f.formation;', 'formation.alert=5;formation.source=f.formation;'),
        ('query_route_restart', 'original_figure_update.hpp', '(f.direction>7&&f.progress<19)', '(f.direction>7)'),
        ('supplier_reservation', 'original_figure_update.hpp', '400-int(b.incomingFigures)*100-b.industryInput<=0', '400-int(b.incomingFigures)*100-b.industryInput<0'),
        ('supplier_network', 'original_figure_update.hpp', 'if(b.category!=context.roadNetworks[f.cell])continue;', 'if(b.category==context.roadNetworks[f.cell])continue;'),
        ('road_network_gate', 'original_figure_update.hpp', '(!(mask&8)||(context.terrain[size_t(next)]&0x400))', '(!(mask&8)||(context.terrain[size_t(next)]&0x40))'),
        ('difficulty_scale', 'original_figure_update.hpp', 'uint32_t(row[field])/100u', 'uint32_t(row[field])/99u'),
        ('station_credit', 'original_figure_update.hpp', 'else if(policy.trade[resource]==7)++goods.trade7[resource];', 'else if(policy.trade[resource]==7)--goods.trade7[resource];'),
        ('inspector_expiry', 'original_figure_update.hpp', 'if(f.roamSteps>=f.roamLimit){ //4CD41A', 'if(f.roamSteps>f.roamLimit){ //4CD41A'),
        ('projectile_cleanup', 'original_figure_update.hpp', 'shooter->second.destinationCell&=0xffffu;', 'shooter->second.destinationCell&=0xffff0000u;'),
        ('hunter_collection', 'original_figure_update.hpp', 'prey->second.alive=2;f.action=8;', 'prey->second.alive=1;f.action=8;'),
        ('kiln_delay', 'original_figure_update.hpp', 'if(f.counter>5){ //5F2140', 'if(f.counter>=5){ //5F2140'),
        ('kiln_reservation_debit', 'original_figure_update.hpp', 'std::max<int>(1,b.incomingFigures)-1', 'std::max<int>(1,b.incomingFigures)'),
        ('market_return_target', 'original_figure_update.hpp', 'const int tx=owner->second.x+3,ty=owner->second.y+3;', 'const int tx=owner->second.x+2,ty=owner->second.y+3;'),
        ('impact_soldier_priority', 'original_figure_update.hpp', '(other.type>=58&&other.type<=62)||other.type==78', '(other.type>=59&&other.type<=62)||other.type==78'),
        ('supplier_score', 'original_figure_update.hpp', 'if(score>bestScore){bestScore=score;best=e.first;}', 'if(score<bestScore){bestScore=score;best=e.first;}'),
        ('trader_arrival', 'original_figure_update.hpp', 'if(f.direction==8){f.action=6;f.alive=2;} //4CE972', 'if(f.direction==8){f.action=6;f.alive=1;} //4CE972'),
        ('trader_empire', 'original_figure_update.hpp', 'empire.active=0;', 'empire.active=1;'),
        ('mill_food_gate', 'original_figure_update.hpp', 'if(b.type==0x35&&(resource<1||resource>9))return 0;', 'if(b.type==0x35&&(resource<1||resource>28))return 0;'),
        ('mill_reservation', 'original_figure_update.hpp', 'int(destination->second.incomingAmount)-f.amount', 'int(destination->second.incomingAmount)'),
        ('worker_settle', 'original_figure_update.hpp', 'house.type=3;house.houseModel=0;', 'house.type=2;house.houseModel=0;'),
        ('worker_edit_required', 'original_figure_update.hpp', 'if(!local.buildingEditMode)return out;', 'if(!local.buildingEditMode)local.buildingEditMode=false;'),
        ('hunter_nearby', 'original_figure_update.hpp', 'nearby=distance<=2;', 'nearby=distance<2;'),
        ('crew_orphan', 'original_figure_update.hpp', 'if(removedLeader)f.alive=2;', 'if(removedLeader)f.alive=1;'),
        ('watch_return', 'original_figure_update.hpp', '}else {f.frame=1;f.freeMovement=f.hidden=1;', '}else {f.frame=2;f.freeMovement=f.hidden=1;'),
        ('pheasant_failed_route', 'original_figure_update.hpp', 'if(f.direction==10)f.action=30; //59F4C3', 'if(f.direction==10)f.action=31; //59F4C3'),
        ('corpse_owner_zero', 'original_figure_update.hpp', 'if(owner==context.buildings->end())return false;owner->second.worker=0;', 'if(owner==context.buildings->end())return false;owner->second.worker=1;'),
        ('house_reselection', 'original_scene_timeline.hpp', 'out.buildingHouses[p.footprint.slot]={after->second.type,after->second.residents};++out.buildingImageReselections;', 'out.buildingHouses[p.footprint.slot]={after->second.type,after->second.residents};out.buildingImageReselections+=0;'),
        ('house_surface', 'original_scene_timeline.hpp', 'next.surface.image[cell]=(p.binding.tileset<<14)|(p.binding.physicalId-201);', 'next.surface.image[cell]=(p.binding.tileset<<14)|(p.binding.physicalId-200);'),
        ('projectile_building', 'original_figure_update.hpp', 'if(!local.buildingOwners[f.cell])f.home=std::max<int16_t>', 'if(true)f.home=std::max<int16_t>'),
        ('hunter_prey_cell', 'original_figure_update.hpp', 'model->second[0]>=0&&model->second[0]<=2&&otherModel->second[0]==6', 'model->second[0]>=0&&model->second[0]<2&&otherModel->second[0]==6'),
    ]
    if args.only:
        mutations = [mutation for mutation in mutations if mutation[0] == args.only]
        if not mutations:
            parser.error('unknown mutation: ' + args.only)
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)
    for name, header, before, after in mutations:
        private = work / name
        lab = private / 'game_lab'
        for path in (ROOT / 'game_lab').rglob('*'):
            if path.suffix not in ('.hpp', '.h', '.cpp', '.mm') or 'build' in path.relative_to(ROOT / 'game_lab').parts:
                continue
            target = lab / path.relative_to(ROOT / 'game_lab')
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
        for layout_name in ['city19_figure7_layout.hpp', 'city19_goods1_layout.hpp',
                      'city19_empire2_layout.hpp', 'city19_empire_grid15_layout.hpp', 'city19_later7_layout.hpp']:
            layout = private / 'file_io/include' / layout_name
            layout.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / 'file_io/include' / layout_name, layout)
        original = (lab / header).read_text()
        if original.count(before) != 1:
            raise ValueError('mutation must address exactly one site: ' + name)
        (lab / header).write_text(original.replace(before, after))
        units = ['original_figure_tests.cpp', 'app/map_surface.cpp', 'map_reference.cpp',
                 'original_map.cpp', 'original_save.cpp', 'original_pak.cpp', 'original_terrain.cpp',
                 'original_ground.cpp', 'resource_root.cpp', 'sg3.cpp']
        binary = private / 'test'
        subprocess.run(['/usr/bin/clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                        *[str(lab / s) for s in units], '-framework', 'CoreGraphics',
                        '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], env=env, capture_output=True, text=True)
        print(name, 'exit', result.returncode, result.stderr.strip(), flush=True)
        if result.returncode != 1:
            raise RuntimeError('mutation survived: ' + name)
    print(f'{len(mutations)}/{len(mutations)} private figure mutations rejected; production source unchanged')


if __name__ == '__main__':
    main()
