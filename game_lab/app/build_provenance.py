"""Embed the exact reviewed production source list; no resource scan or external tools."""
from pathlib import Path
import hashlib
import json
import sys

repository = Path(__file__).resolve().parents[2]
sources = [
    'game_lab/app/main.mm', 'game_lab/app/build.sh', 'game_lab/app/build_provenance.py',
    'game_lab/app/Info.plist', 'game_lab/app/av_audio_backend.hpp', 'game_lab/app/av_audio_backend.mm',
    'game_lab/app_media.hpp', 'game_lab/app_media.cpp',
    'game_lab/emperor_assets.hpp', 'game_lab/emperor_assets.mm',
    'game_lab/asset_bytes.hpp', 'game_lab/asset_limits.hpp', 'game_lab/asset_pixels.cpp',
    'game_lab/wav_pcm.cpp', 'game_lab/resource_root.hpp', 'game_lab/resource_root.cpp',
    'game_lab/audio_policy.hpp', 'surface_lab/surface.hpp', 'surface_lab/surface.cpp',
]
compiled = [
    'game_lab/app/main.mm', 'game_lab/app/av_audio_backend.mm', 'game_lab/app_media.cpp',
    'game_lab/emperor_assets.mm', 'game_lab/wav_pcm.cpp', 'game_lab/asset_pixels.cpp',
    'game_lab/resource_root.cpp', 'surface_lab/surface.cpp',
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
    'external_game_inputs': False,
    'app_executed_by_build': False,
}
(bundle/'Contents/Resources/source-manifest.json').write_text(
    json.dumps(manifest, ensure_ascii=False, indent=2)+'\n')
print('Embedded reviewed production source SHA-256 manifest')
