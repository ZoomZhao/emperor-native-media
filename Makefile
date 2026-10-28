.PHONY: final-dispatch-test final-dispatch-test-undefined bink-cancel-test bink-cancel-test-undefined all game-app run-game sg3-test sg3-test-undefined data-metrics map-test map-test-undefined map-metrics map-sg3-audit map-sg3-audit-test map-raw-spans map-raw-spans-test map-reference-test map-reference-test-undefined building-image-test building-image-test-undefined original-random-test original-random-test-undefined map-ground-sweep save-state-audit save-state-audit-test pak-payload-audit pak-payload-audit-test pak-native-test pak-native-test-undefined pak-native-metrics save-native-test save-native-test-undefined save-native-metrics base-content-audit-test reference-compare-test input-test input-test-undefined input-window-test
all: game-app
.PHONY: figure-scene-test figure-scene-test-undefined
figure-scene-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene.py
figure-scene-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene.py --mode undefined
.PHONY: scene-timeline-test scene-timeline-test-undefined scene-timeline-bite
.PHONY: scene-combined-test scene-combined-test-undefined scene-combined-supplied-test scene-combined-supplied-test-undefined scene-combined-bite
scene-combined-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene.py --timeline-mode combined --city-clocks 0,0,0,0,0,0,0,0,0,0
scene-combined-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene.py --mode undefined --timeline-mode combined --city-clocks 0,0,0,0,0,0,0,0,0,0
scene-combined-supplied-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene.py --timeline-mode combined --city-clocks 0,0,0,0,0,0,0,0,0,0 --farmer-wall-clock 1000 --crt-random-state 1
scene-combined-supplied-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene.py --mode undefined --timeline-mode combined --city-clocks 0,0,0,0,0,0,0,0,0,0 --farmer-wall-clock 1000 --crt-random-state 1
scene-combined-bite:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_annual_notice
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_annual_premise
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_migrant_tie
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_mill_incoming
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_music_tier
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_industry_only
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_supplier_speech
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_supplier_chain
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_farmer_retry
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_supplier_fifo
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_supplier_withdraw
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_supplier_delay
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_debris_lifetime
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_effect_count
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_flame_frame
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_supplier_reservation
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_inspector_delay
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_phase
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_clock
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_event
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_growth
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_tail
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_rng
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_slots
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_atomic
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_bridge
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_building
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_accounting
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_post_provider
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_network
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_worker_delay
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_worker_exit
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_worker_cleanup
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_worker_binding
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_well_start
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_lifecycle
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_risk
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_cycle_tail
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_road_plain
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_emigrant_wait
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only combined_fire_bridge
scene-timeline-test: figure-scene-test
scene-timeline-test-undefined: figure-scene-test-undefined
scene-timeline-bite:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only timeline_repeat
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only engine_route_tie
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only movement_full_mask
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only delivery_flags
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only watch_risk
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only elevated_lookahead
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only watch_category
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only service_shadow
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only crop_season
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only market_smallest
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only goods_smallest
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only goods_remainder
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only goods_accounting
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only goods_delay
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only goods_policy
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only goods_cache
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only storage_reselection
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only watch_criminal
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only delegated_target
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only well_water
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only roam_owner
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only projectile_fifo
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only projectile_steps
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only projectile_selector
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only projectile_arc
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only well_festival
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only roadblock_gate
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only crt_seed_required
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only crt_step
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only crt_status
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only crt_zero
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only saved_scenario_gate
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only warehouse_fullest
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only warehouse_credit
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only inspector_cleanup
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only hunter_search_order
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only goods_return
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only delegated_expiry
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only cart_orphan
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only road_route_mask
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only projectile_empty_shadow
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only delegated_route_mask
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only warehouse_return
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only hunter_return_goods
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only hunter_retarget
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only impact_health
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only impact_damage
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only impact_attacker
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only impact_audio
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only corpse_audio
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only corpse_limit
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only retained_cargo
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only impact_formation
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only query_route_restart
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only supplier_reservation
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only supplier_network
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only road_network_gate
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only difficulty_scale
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only station_credit
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only inspector_expiry
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only projectile_cleanup
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only hunter_collection
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only kiln_delay
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only kiln_reservation_debit
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only market_return_target
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only impact_soldier_priority
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only supplier_score
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only trader_arrival
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only trader_empire
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only mill_food_gate
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only mill_reservation
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only worker_settle
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only worker_edit_required
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only hunter_nearby
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only crew_orphan
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only watch_return
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only pheasant_failed_route
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only corpse_owner_zero
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only house_reselection
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only house_surface
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only projectile_building
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/figure_scene_bite.py --only hunter_prey_cell
game-app:
	/bin/sh game_lab/app/build.sh
run-game: game-app
	./game_lab/build/EmperorApp.app/Contents/MacOS/EmperorApp
sg3-test:
	/usr/bin/python3 scripts/data_first.py sg3-test
sg3-test-undefined:
	/usr/bin/python3 scripts/data_first.py sg3-test --mode undefined
data-metrics:
	/usr/bin/python3 scripts/data_first.py metrics
map-test:
	/usr/bin/python3 scripts/map_data.py test
map-test-undefined:
	/usr/bin/python3 scripts/map_data.py test --mode undefined
map-metrics:
	/usr/bin/python3 scripts/map_data.py metrics
map-sg3-audit:
	/usr/bin/python3 scripts/map_sg3_audit.py
map-sg3-audit-test:
	/usr/bin/python3 scripts/map_sg3_audit_tests.py
map-raw-spans:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/map_raw_spans.py
map-raw-spans-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/map_raw_spans_tests.py
map-reference-test:
	/usr/bin/python3 scripts/map_reference.py test
map-reference-test-undefined:
	/usr/bin/python3 scripts/map_reference.py test --mode undefined
building-image-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/building_image_binding.py --mode release
building-image-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/building_image_binding.py --mode undefined
.PHONY: building-scene-test building-scene-test-undefined
building-scene-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/building_scene.py --mode release
building-scene-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/building_scene.py --mode undefined
.PHONY: app-surface-test app-surface-test-undefined app-playback-test app-playback-test-undefined
app-surface-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/app_map_surface.py test
app-surface-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/app_map_surface.py test --mode undefined
app-playback-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/app_map_surface.py playback
app-playback-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/app_map_surface.py playback --mode undefined
.PHONY: saved-ground-test saved-ground-test-undefined
saved-ground-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/saved_ground.py
saved-ground-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/saved_ground.py --mode undefined
original-random-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/original_random.py test
original-random-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/original_random.py test --mode undefined
map-ground-sweep:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/map_ground_sweep.py sweep --data-root "$$EMPEROR_GAMEDATA_ROOT"
save-native-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_data.py test
save-native-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_data.py test --mode undefined
save-native-metrics:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_data.py metrics
pak-native-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_data.py test
pak-native-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_data.py test --mode undefined
pak-native-metrics:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_data.py metrics
pak-payload-audit:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_payload_audit.py
pak-payload-audit-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_payload_audit_tests.py
save-state-audit:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_state_audit.py
save-state-audit-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_state_audit_tests.py
base-content-audit-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/base_content_audit_tests.py
reference-compare-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/reference_compare_tests.py
input-test:
	/usr/bin/python3 input_lab/run_tests.py
input-test-undefined:
	/usr/bin/python3 input_lab/run_tests.py --mode undefined
input-window-test:
	/usr/bin/python3 input_lab/run_tests.py --window

bink-cancel-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 game_lab/bink_cancellation/run.py
bink-cancel-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 game_lab/bink_cancellation/run.py --mode undefined

final-dispatch-test:
	PYTHONDONTWRITEBYTECODE=1 python3 input_lab/final_dispatch_tests.py

final-dispatch-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 python3 input_lab/final_dispatch_tests.py --mode undefined

.PHONY: building-update-test building-update-test-undefined
building-update-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/building_update.py
building-update-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/building_update.py --mode undefined

.PHONY: city-clocks-test city-clocks-test-undefined
city-clocks-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/city_clocks.py
city-clocks-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/city_clocks.py --mode undefined

.PHONY: post-figure-pass-test post-figure-pass-test-undefined
post-figure-pass-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/post_figure_pass.py
post-figure-pass-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/post_figure_pass.py --mode undefined

.PHONY: native-population-test native-population-test-undefined
native-population-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/native_population.py
native-population-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/native_population.py --mode undefined

.PHONY: native-city-context-test native-city-context-test-undefined
native-city-context-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/native_city_context.py
native-city-context-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/native_city_context.py --mode undefined


.PHONY: original-evidence-test original-evidence-test-undefined
original-evidence-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/original_evidence.py
original-evidence-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/original_evidence.py --mode undefined
