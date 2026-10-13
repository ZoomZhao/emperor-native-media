#!/usr/bin/env python3
"""Build modern source only; test the explicit original-data corpus read-only.

--window shows and naturally plays the real silent video in an AppKit window.
Existing FFmpeg is an external dependency; nothing is installed or bundled.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys

SOURCES = [
    "game_lab/movie_session.mm", "game_lab/app_media.cpp",
    "game_lab/emperor_assets.mm", "game_lab/sg3.cpp", "game_lab/original_map.cpp", "game_lab/asset_pixels.cpp",
    "game_lab/resource_root.cpp", "game_lab/wav_pcm.cpp",
    "file_io/src/strict_zlib.cpp", "file_io/src/framed_reader.cpp",
    "surface_lab/surface.cpp", "stream_lab/movie_io.mm",
    "stream_lab/presenter.cpp", "video_lab/stream.cpp", "metal_lab/backend.mm",
    "audio_lab/audio.cpp", "media_demo/controller.cpp",
]
FRAMEWORKS = ["AppKit", "Foundation", "AVFoundation", "AudioToolbox",
              "CoreMedia", "CoreVideo", "CoreGraphics", "ImageIO", "Metal"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--window", action="store_true")
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument("--legacy-mov", action="store_true")
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--app-build-check", action="store_true")
    options = parser.parse_args()
    supplied = os.environ.get("EMPEROR_GAMEDATA_ROOT", "")
    if not supplied and not options.build_only and not options.app_build_check:
        print("SKIP: EMPEROR_GAMEDATA_ROOT is unset")
        return 0
    root = Path(__file__).resolve().parents[1]
    build = Path(os.environ.get("EMPEROR_BUILD_DIR", str(root / "build/bink")))
    build.mkdir(parents=True, exist_ok=True)
    if options.app_build_check:
        entry, target = "game_lab/app/main.mm", build / "EmperorApp-source-check"
        extra = ["game_lab/app/av_audio_backend.mm", "game_lab/candidate_import.cpp"]
    else:
        entry = "game_lab/bink_window_test.mm" if options.window else "game_lab/bink_data_tests.mm"
        target = build / ("bink-window-test" if options.window else "bink-data-tests")
        extra = []
    command = ["/usr/bin/clang++", "-std=c++17", "-fobjc-arc", "-fno-modules",
               "-O2", "-mmacosx-version-min=15.0", "-Wall", "-Wextra", "-Werror",
               "-I", str(root), "-I", str(root / "file_io/include"),
               *[str(root / source) for source in [entry, *extra, *SOURCES]]]
    for framework in FRAMEWORKS:
        command.extend(["-framework", framework])
    command.extend(["-lz", "-o", str(target)])
    subprocess.run(command, check=True, timeout=60)
    print("modern_source_build=PASS", flush=True)
    if options.build_only or options.app_build_check:
        return 0
    corpus = Path(supplied).resolve(strict=True)
    paths = sorted(path for path in corpus.rglob("*")
                   if path.suffix.lower() in {".bik", ".bk2"}
                   and path.is_file() and not path.is_symlink()
                   and path.resolve().is_relative_to(corpus))
    if len(paths) != 1:
        print("BLOCKED: minimal video acceptance requires exactly one discovered Bink file")
        return 2
    arguments = [str(target), str(paths[0].relative_to(corpus))]
    if options.smoke:
        if not options.window:
            parser.error("--smoke requires --window")
        arguments.append("--smoke")
    if options.legacy_mov:
        if options.window:
            parser.error("--legacy-mov is a separate data-test regression")
        arguments.append("--legacy-mov")
    # Test executables own their watchdog and normal resource cleanup. A hung
    # cleanup remains visible as BLOCKED; this supervisor never kills it.
    return subprocess.run(arguments).returncode


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError) as failure:
        print("BLOCKED:", failure, file=sys.stderr)
        sys.exit(2)
