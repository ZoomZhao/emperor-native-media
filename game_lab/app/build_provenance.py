"""Embed the exact reviewed production source list; no resource scan or external tools."""
from pathlib import Path
import hashlib
import json
import sys

repository = Path(__file__).resolve().parents[2]
sources = [
    'game_lab/original_building_scene.hpp',
    'game_lab/original_building_animation.hpp',
    'game_lab/original_figure.hpp',
    'file_io/include/city19_figure7_layout.hpp',
    'game_lab/original_candidate_input.hpp',
    'game_lab/app/building_candidate_config.hpp', 'game_lab/original_building_candidate.hpp', 'game_lab/original_building.hpp', 'game_lab/original_building_view.hpp',
    'game_lab/sg3_sequence.hpp',
    'game_lab/app/map_surface.hpp',
    'game_lab/app/map_surface.cpp',
    'game_lab/app/map_surface_preview.cpp',
    'game_lab/map_reference.hpp',
    'game_lab/map_reference.cpp',
    'game_lab/original_terrain.hpp',
    'game_lab/original_terrain.cpp',
    'game_lab/original_ground.hpp',
    'game_lab/original_ground.cpp',
    'LICENSE', 'THIRD_PARTY_NOTICES.md', 'README.md', 'game_lab/city_simulation.hpp', 'game_lab/city_state_io.hpp',
    'game_lab/city_game_state.hpp', 'game_lab/app/city_scenario.hpp', 'game_lab/app/city_scenario_checkpoint.hpp',
    'game_lab/app/city_warehouse_training.hpp', 'game_lab/app/city_warehouse_transfer.hpp', 'game_lab/app/city_warehouse_routes.hpp',
    'game_lab/app/main.mm', 'game_lab/app/city_goal_feedback.hpp', 'game_lab/app/city_goal_feedback_adapter.hpp',
    'game_lab/app/build.sh', 'game_lab/app/build_provenance.py',
    'game_lab/app/Info.plist', 'game_lab/app/av_audio_backend.hpp', 'game_lab/app/av_audio_backend.mm',
    'game_lab/app_media.hpp', 'game_lab/app_media.cpp', 'game_lab/app_pak.cpp',
    'game_lab/original_pak.hpp', 'game_lab/original_pak.cpp', 'game_lab/original_save.hpp', 'game_lab/original_save.cpp',
    'game_lab/movie_session.hpp', 'game_lab/movie_session.mm',
    'game_lab/bink_preview.hpp', 'game_lab/bink_audio.hpp', 'stream_lab/bink_process.hpp',
    'game_lab/image_presentation.hpp', 'game_lab/cg_image_conversion.hpp',
    'game_lab/app_launch.hpp',
    'game_lab/map_geometry.hpp', 'game_lab/map_viewport.hpp',
    'game_lab/emperor_assets.hpp', 'game_lab/emperor_assets.mm',
    'game_lab/asset_bytes.hpp', 'game_lab/asset_limits.hpp', 'game_lab/asset_pixels.cpp',
    'game_lab/wav_pcm.cpp', 'game_lab/resource_root.hpp', 'game_lab/resource_root.cpp',
    'game_lab/audio_policy.hpp', 'surface_lab/surface.hpp', 'surface_lab/surface.cpp',
    'stream_lab/movie_io.hpp', 'stream_lab/movie_io.mm',
    'stream_lab/presenter.hpp', 'stream_lab/presenter.cpp',
    'video_lab/stream.hpp', 'video_lab/stream.cpp', 'video_lab/video.hpp',
    'metal_lab/backend.hpp', 'metal_lab/backend.mm',
    'audio_lab/audio.hpp', 'audio_lab/audio.cpp',
    'media_demo/host_clock.hpp',
    'media_demo/controller.hpp', 'media_demo/controller.cpp',
    'media_demo/device_monitor.hpp',
    'game_lab/candidate_import.hpp',
    'game_lab/candidate_import.cpp', 'game_lab/sg3.cpp', 'game_lab/original_map.cpp', 'game_lab/original_map.hpp',
    'file_io/include/strict_zlib.h',
    'file_io/include/framed_reader.hpp',
    'file_io/include/owned_reader.hpp',
    'file_io/include/city19_goal_archive.hpp',
    'file_io/src/strict_zlib.cpp',
    'file_io/src/framed_reader.cpp',
]
compiled = [
    'game_lab/app/map_surface.cpp',
    'game_lab/app/map_surface_preview.cpp',
    'game_lab/map_reference.cpp',
    'game_lab/original_terrain.cpp',
    'game_lab/original_ground.cpp',
    'game_lab/app/main.mm', 'game_lab/app/av_audio_backend.mm',
    'game_lab/app_media.cpp', 'game_lab/app_pak.cpp', 'game_lab/original_pak.cpp', 'game_lab/original_save.cpp', 'game_lab/movie_session.mm',
    'game_lab/emperor_assets.mm', 'game_lab/wav_pcm.cpp', 'game_lab/asset_pixels.cpp',
    'game_lab/resource_root.cpp', 'surface_lab/surface.cpp',
    'game_lab/candidate_import.cpp', 'game_lab/sg3.cpp', 'game_lab/original_map.cpp', 'file_io/src/strict_zlib.cpp', 'file_io/src/framed_reader.cpp',
    'stream_lab/movie_io.mm', 'stream_lab/presenter.cpp',
    'video_lab/stream.cpp', 'metal_lab/backend.mm',
    'audio_lab/audio.cpp',
    'media_demo/controller.cpp',
]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
assert len(sys.argv) == 2
bundle = Path(sys.argv[1])
assert bundle.is_absolute() and bundle.name == 'EmperorApp.app'
assert (bundle/'Contents/Resources').is_dir()
records = {}
for name in sources:
    path = repository/name
    assert path.is_file() and not path.is_symlink(), name
    records[name] = sha(path)
manifest = {
    'schema': 'EMPEROR_SELF_AUTHORED_APP_BUILD_V1',
    'sources_sha256': records,
    'compiled_sources': compiled,
    'binary_sha256_before_signing': sha(bundle/'Contents/MacOS/EmperorApp'),
    'application_mode': 'city-and-media',
    'city_model_included': True,
    'city_model_origin': 'self-authored; original rules not compared',
    'external_game_inputs': False,
    'app_executed_by_build': False,
    'original_map_view': 'MAP-and-PAK-city-stored-surface-candidate-preview;SAV19-seeded-ground-building-bases-entertainment-saved-phase-building-animation-and-saved-figure-images',
    'candidate_placement_verified': False,
    'strict_metrics_separate': True,
    'original_screen_accuracy_verified': False,
}
(bundle/'Contents/Resources/source-manifest.json').write_text(
    json.dumps(manifest, ensure_ascii=False, indent=2)+'\n')
print('Embedded reviewed production source SHA-256 manifest')
