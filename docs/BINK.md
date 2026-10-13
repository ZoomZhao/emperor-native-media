# Bink video

Format fields use the public [FFmpeg demuxer](https://ffmpeg.org/doxygen/trunk/libavformat_2bink_8c_source.html); no original-game code. `BIK` plus revision is Bink1; `KB2` is separately rejected. Little-endian header gives frame count, dimensions and rational FPS. Bounds: dimensions ≤4096, frames ≤1,000,000, FPS ≤240, BGRA frame ≤8 MiB and declared/physical size agreement.

`BinkProcess` → `MovieReader` → video-only `BinkPreview` → existing bounded `MovieSession` → AppKit/CoreGraphics. Original 640×480 frames, 30fps, duration170.033s reach the presenter without the old32×24 crop. Pause/seek/EOF use the native monotonic timeline; audio decoding and A/V synchronization are not implemented.

FFmpeg8.0.1 arm64 is a separately installed external dependency, locally GPLv3-or-later; no FFmpeg executable/library is bundled or linked. Absolute `EMPEROR_FFMPEG_PATH` overrides the existing default `/opt/homebrew/bin/ffmpeg`. Missing dependency is an error, never an install. Direct argv, file/pipe protocols only, no shell/network/output files; `-an` suppresses audio. License notice is in THIRD_PARTY_NOTICES.md.

One worker/latest pending/latest delivery, pipe backpressure. The8MiB limit is per client frame, not total RSS or FFmpeg internal memory. Closing cancels only the owned child with pipe closure/SIGTERM and waits for reaping; no SIGKILL fallback. Main termination deadline can refuse exit while cleanup continues. Window tests likewise wait for safe close; their watchdog initiates closure and never kills the test process. A wholly authored child protocol regression waits for natural completion beyond250ms; it is not Bink decoder evidence.

`python3 game_lab/run_bink_tests.py`, `--legacy-mov`, `--window --smoke`, and `--window` are focused entry points. Real data read only `EMPEROR_GAMEDATA_ROOT`; unset is SKIP. `--app-build-check` builds modern source without running the App. No original or derived asset is exported to Git.

Corpus: one BIKi intro, 79,427,112 bytes, 640×480,30fps,5,101 frames. Independent full window run decoded/delivered/drew5,101/5,101 in170.007s with0due-frame drops. Integrated code decoded5,101/5,101; integrated smoke decoded59/drew58 visibly in1.916s,0due-frame drops, queue peaks1/1/1 and close resources zero. Smoke is not a full replay. Legacy MOV and normal child cleanup also passed; ordinary sandbox MOV writing failed, then passed with the specifically approved native-codec permission. Original renderer agreement, hearing/audio output and original-game playability remain unmeasured.

同一代次的Tick共用取消令牌，避免第二个待处理Tick遮蔽正在读取的令牌。子进程显式清空信号屏蔽字并设TERM/INT/PIPE默认处理，不继承GCD worker的阻塞。整合后的自造协作子进程反例在Release/UBSan分别47/61ms关闭，记录PID已消失，资源归零；这是生命周期回归，不计真实帧覆盖率。c51dfb4最终生命周期独审已通过：Release/UBSan各9/0，协作关闭49/54ms、TERM/INT屏蔽字0且PID回收，忽略TERM的短命子进程在80ms截止保持主队列响应并如实报未完成、实际退出后才归零；未独立重跑全片或真实窗口。
