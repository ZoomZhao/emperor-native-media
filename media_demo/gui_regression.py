#!/usr/bin/python3
"""Actual own-window AX regression; private injected events are clearly labelled.
No physical mouse/key claims, screen capture, audio IO, system writes or other apps.
"""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "evidence/integration-gui"
if OUT.exists():
    OUT.rename(ROOT / "evidence" / ("previous-integration-gui-" + str(time.time_ns())))
OUT.mkdir(parents=True)
DEADLINE = time.monotonic() + 50
HELPER = ROOT / "build/gui-native"
active = False
count = 0

def command(*args):
    global count
    count += 1
    started = time.monotonic()
    completed = subprocess.run([str(HELPER), *args], capture_output=True, timeout=max(.1, min(11, DEADLINE - started)))
    prefix = OUT / (str(count).zfill(3) + "-" + args[0])
    prefix.with_suffix(".stdout.log").write_bytes(completed.stdout)
    prefix.with_suffix(".stderr.log").write_bytes(completed.stderr)
    prefix.with_suffix(".status.json").write_text(json.dumps({"returncode": completed.returncode, "seconds": time.monotonic() - started}) + "\n")
    if completed.returncode:
        raise RuntimeError(completed.stderr.decode(errors="replace"))
    return completed.stdout

def inspect(name):
    command("inspect", name)
    source = ROOT / "evidence/gui-session" / (name + ".json")
    shutil.copy2(source, OUT / source.name)
    rows = json.loads(source.read_text())["window_elements"]
    status = next(row["AXValue"] for row in rows if row.get("AXRole") == "AXStaticText" and
                  (" · 帧 " in str(row.get("AXValue", "")) or str(row.get("AXValue", "")).startswith("待机 · 未打开") or row.get("AXValue") == "正在处理…"))
    slider = next(row["AXValue"] for row in rows if row.get("AXRole") == "AXSlider")
    return status, slider, rows

def wait_state(prefix, seconds):
    until = min(DEADLINE, time.monotonic() + seconds)
    while time.monotonic() < until:
        value = inspect("integrationWait")
        if value[0].startswith(prefix):
            return value
        time.sleep(.04)
    raise RuntimeError("own UI state timeout: " + prefix)

def ensure(value, message):
    if not value:
        raise RuntimeError(message)

try:
    preflight = subprocess.run([str(ROOT / "build/gui-permission-probe")], capture_output=True, timeout=3)
    (OUT / "permission-preflight.json").write_bytes(preflight.stdout)
    permissions = json.loads(preflight.stdout)
    ensure(preflight.returncode == 0 and permissions["accessibility_trusted"], "existing permissions missing; no request made")
    command("launchIntegration", "windowClose", "inject")
    active = True
    idle = inspect("integrationIdle")
    ensure(idle[0].startswith("待机") and idle[1] == 0, "default remains idle")
    command("play")
    wait_state("播放中", 2)
    loss = wait_state("已暂停", 3)
    ensure("帧队列 0 · PCM来源 0 · GPU票据 0" in loss[0], "injected loss releases actual app owners")
    inspect("integrationDeviceLost")
    wait_state("播放中", 2)
    inspect("integrationDeviceRecovered")
    command("pause")
    paused = inspect("integrationPaused")
    time.sleep(.18)
    ensure(paused[:2] == inspect("integrationPausedLater")[:2], "actual window pause stable")
    command("seek", "120")
    seek = inspect("integrationSeek")
    ensure(seek[0].startswith("已暂停") and "帧 120/239" in seek[0] and seek[1] == 120, "AX seek paused at actual target")
    command("play")
    command("seek", "0")
    wait_state("播放结束", 8.5)
    eof = inspect("integrationEOF")
    ensure(eof[1] == 239 and "帧队列 0 · PCM来源 0 · GPU票据 0" in eof[0], "actual EOF last frame, slider and released owners")
    command("play")
    replay = inspect("integrationReplay")
    ensure(replay[0].startswith("播放中") and replay[1] < 30, "actual replay from start")
    command("stop")
    stopped = inspect("integrationStopped")
    ensure(stopped[0].startswith("待机") and stopped[1] == 0 and "帧队列 0 · PCM来源 0 · GPU票据 0" in stopped[0], "actual stop resets slider and releases owners")
    command("play")
    command("close")
    active = False
    command("launchIntegration", "quitPaused")
    active = True
    ensure(inspect("integrationReopened")[0].startswith("待机"), "reopen idle")
    command("play")
    command("pause")
    command("seek", "60")
    ensure(inspect("integrationQuitPaused")[0].startswith("已暂停"), "quit while paused")
    command("quit")
    active = False
    command("verifyExited")
    reports = [json.loads((OUT / (name + ".json")).read_text()) for name in ("windowClose", "quitPaused")]
    for report in reports:
        ensure(report["passed"] and report["resources_zero"] and report["device_listeners"] == 0 and report["initial_device_listeners"] == 3, "actual app exit resource/listener regression")
        ensure(report["runtime_opens"] == report["runtime_releases"], "actual app all media sessions released")
    ensure(reports[0]["injected_device_events"] == 2 and reports[0]["device_recoveries"] == 1, "two private events through actual app monitor/worker/UI")
    rows = [json.loads(line) for line in (OUT / "windowClose.states.jsonl").read_text().splitlines()]
    for row in rows:
        if row["state"] in ("播放中", "已暂停"):
            ensure(row["frame"] == row["media_pcm"] // 1600, "actual UI frame/PCM phase")
    lost = next(row for row in rows if row["device_blocked"])
    recovered = next(row for row in rows if row["message"].startswith("输出设备变化"))
    ensure(lost["resources_zero"] and recovered["media_pcm"] == lost["media_pcm"] and recovered["state"] == "播放中", "actual event callback preserves exact frozen PCM")
    origin = next(row for row in rows if row["message"].startswith("Seek：跳转到帧 0"))
    ended = next(row for row in rows if row["state"] == "播放结束")
    seconds = (ended["UI_ns"] - origin["UI_ns"]) / 1e9
    ensure(8 <= seconds <= 8.15, "actual own window EOF within 8.15 real seconds")
    latency = max((row["UI_ns"] - row["host_ns"]) / 1e6 for row in rows if row["tick"] and row["state"] == "播放中")
    ensure(latency <= 100, "actual tick-to-UI delivery <=100ms")
    summary = {"passed": True, "actual_app_instances": len(reports), "private_injected_device_events": 2,
               "physical_mouse_or_keyboard_tested": False, "real_device_switch_induced": False, "real_sound": False,
               "screenshots_taken": False, "full_clip_UI_seconds": seconds, "max_tick_UI_delivery_ms": latency,
               "UI_frame_PCM_phase_below_one_frame": True, "all_registered_instances_exited": True,
               "each_app_45s_auto_quit_guard": True, "commands": count, "process_seconds": 50 - (DEADLINE - time.monotonic())}
    (OUT / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n")
    sources = [ROOT / p for p in ("main.mm", "controller.hpp", "controller.cpp", "host_clock.hpp", "device_monitor.hpp", "device_monitor.mm", "gui_native.mm", "gui_regression.py")]
    sources += [ROOT.parent / "stream_lab/movie_io.hpp", ROOT.parent / "stream_lab/movie_io.mm"]
    sources += [HELPER, ROOT / "build/NativeMediaDemo.app/Contents/MacOS/NativeMediaDemo", OUT / "summary.json", OUT / "windowClose.json", OUT / "quitPaused.json", OUT / "windowClose.states.jsonl"]
    (OUT / "source-and-result.sha256").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p) + "\n" for p in sources))
    print("PASS actual silent app AX/event/clock/exit integration: " + json.dumps(summary, ensure_ascii=False))
except Exception as error:
    (OUT / "summary.json").write_text(json.dumps({"passed": False, "error": str(error), "commands": count}, ensure_ascii=False, indent=2) + "\n")
    failure = OUT / "failure-sources"
    failure.mkdir(exist_ok=True)
    for source in ROOT.glob("*.mm"):
        shutil.copy2(source, failure / source.name)
    for source in (ROOT / "controller.hpp", ROOT / "controller.cpp", ROOT / "host_clock.hpp", ROOT / "device_monitor.hpp", Path(__file__)):
        shutil.copy2(source, failure / source.name)
    for source in (ROOT.parent / "stream_lab/movie_io.hpp", ROOT.parent / "stream_lab/movie_io.mm"):
        shutil.copy2(source, failure / source.name)
    raise
finally:
    if active:
        command("quit")
