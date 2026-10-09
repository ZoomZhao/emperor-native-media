#!/usr/bin/python3
"""Actual own App, native AX only; each invocation <=50s. No screen/audio/input IO."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
if len(sys.argv) != 2 or sys.argv[1] not in ("long", "reopen", "loading"):
    raise SystemExit("usage: reliability_gui.py long|reopen|loading")
MODE = sys.argv[1]
OUT = ROOT / "evidence/reliability-gui"
RUN = OUT / (MODE + "-" + str(time.time_ns()))
RUN.mkdir(parents=True)
DEADLINE = time.monotonic() + 50
HELPER = ROOT / "build/gui-native"
active = False
commands = 0
reports = []

def command(*args):
    global commands
    commands += 1
    began = time.monotonic()
    completed = subprocess.run([str(HELPER), *args], capture_output=True, timeout=max(.1, min(11, DEADLINE - began)))
    prefix = RUN / (str(commands).zfill(3) + "-" + args[0])
    prefix.with_suffix(".stdout.log").write_bytes(completed.stdout)
    prefix.with_suffix(".stderr.log").write_bytes(completed.stderr)
    prefix.with_suffix(".status.json").write_text(json.dumps({"returncode": completed.returncode, "seconds": time.monotonic() - began}) + "\n")
    if completed.returncode:
        raise RuntimeError(completed.stderr.decode(errors="replace"))
    return completed.stdout

def inspect():
    rows = json.loads(command("inspect"))["window_elements"]
    status = next(row["AXValue"] for row in rows if row.get("AXRole") == "AXStaticText" and
                  (" · 帧 " in str(row.get("AXValue", "")) or str(row.get("AXValue", "")).startswith("待机 · 未打开") or row.get("AXValue") == "正在处理…"))
    slider = next(row["AXValue"] for row in rows if row.get("AXRole") == "AXSlider")
    return status, slider

def ensure(value, label):
    if not value:
        raise RuntimeError(label)

def wait(prefix, timeout):
    end = min(DEADLINE, time.monotonic() + timeout)
    while time.monotonic() < end:
        value = inspect()
        if value[0].startswith(prefix):
            return value
        time.sleep(.05)
    raise RuntimeError("actual own UI state timeout: " + prefix)

def launch(tag, label):
    global active
    name = MODE + label + str(time.time_ns())
    command("launchReliability", name, tag)
    active = True
    ensure(inspect()[0].startswith("待机"), "each actual instance opens idle")
    return name

def close(name, action):
    global active
    command(action)
    active = False
    report = json.loads((OUT / (name + ".json")).read_text())
    ensure(report["passed"] and report["resources_zero"] and report["device_listeners"] == 0 and report["initial_device_listeners"] == 3, "actual exit owns zero media/listeners")
    ensure(report["runtime_opens"] == report["runtime_releases"], "all attempts including partial opens released")
    reports.append(report)
    return report

try:
    preflight = subprocess.run([str(ROOT / "build/gui-permission-probe")], capture_output=True, timeout=3)
    (RUN / "permissions.json").write_bytes(preflight.stdout)
    permissions = json.loads(preflight.stdout)
    ensure(preflight.returncode == 0 and permissions["accessibility_trusted"], "existing permissions missing; no new request")
    if MODE == "long":
        name = launch("long", "Full")
        command("play")
        wait("播放中", 2)
        while time.monotonic() < DEADLINE:
            value = inspect()
            ensure(not value[0].startswith("发生错误"), "actual long App reported failure")
            if value[0].startswith("播放结束"):
                break
            time.sleep(.25)
        ensure(value[0].startswith("播放结束") and value[1] == 959, "actual 32-second App ends at frame 959")
        report = close(name, "close")
        ensure(report["controlled_worker_delay_ms"] == 2000 and report["max_catchup_frames"] >= 96000, "actual App recovered from own 2-second worker delay")
        ensure(report["max_video_bytes"] <= 62640 and report["max_PCM_bytes"] <= 16384 and report["max_PCM_sources"] <= 2 and report["max_readers"] <= 1 and report["max_GPU_buffer_bytes"] <= 34632, "unchanged actual App ownership bounds")
        rows = [json.loads(line) for line in (OUT / (name + ".states.jsonl")).read_text().splitlines()]
        for row in rows:
            if row["state"] in ("播放中", "已暂停"):
                ensure(row["frame"] == row["media_pcm"] // 1600, "actual long App PTS matches PCM")
        playing = next(row for row in rows if row["message"].startswith("播放："))
        ended = next(row for row in rows if row["state"] == "播放结束")
        seconds = (ended["UI_ns"] - playing["UI_ns"]) / 1e9
        ensure(32 <= seconds <= 32.15, "same duration+0.15s UI threshold")
        latency = max((r["UI_ns"] - r["host_ns"]) / 1e6 for r in rows if r["tick"] and r["state"] == "播放中")
        ensure(latency <= 100, "same actual sampled-clock-to-UI <=100ms threshold")
        summary = {"passed": True, "UI_seconds": seconds, "max_tick_UI_ms": latency, "controlled_worker_delay_ms": 2000,
                   "actual_app_instances": 1, "max_catchup_frames": report["max_catchup_frames"], "all_resources_zero": True}
    elif MODE == "loading":
        name = launch("loading", "Timeout")
        started = time.monotonic()
        command("play")
        processing = inspect()
        ensure(processing[0] == "正在处理…", "controlled actual asset metadata wait visible")
        value = wait("发生错误", 2.3)
        timeout_seconds = time.monotonic() - started
        ensure(timeout_seconds <= 2.5 and "帧队列 0 · PCM来源 0 · GPU票据 0" in value[0], "metadata deadline returns cleared actual window")
        command("stop")
        ensure(inspect()[0].startswith("待机"), "Stop clears timeout")
        report = close(name, "quit")
        ensure(report["metadata_timeouts"] == 1 and report["metadata_cancellations"] == 0, "actual App timeout counter")
        trace = [json.loads(line) for line in (OUT / (name + ".states.jsonl")).read_text().splitlines()]
        ensure(any("metadata load timeout (2000 ms)" in row["message"] for row in trace), "actual timeout error delivered to own UI log")
        name = launch("loading", "Stop")
        command("play")
        ensure(inspect()[0] == "正在处理…", "loading before Stop")
        started = time.monotonic()
        command("stop")
        wait("待机", .5)
        stop_seconds = time.monotonic() - started
        ensure(stop_seconds <= .5, "actual Stop cancels loading within .5s")
        report = close(name, "quit")
        ensure(report["metadata_cancellations"] == 1 and report["metadata_timeouts"] == 0, "Stop used cancel path, no forced exit")
        name = launch("loading", "Quit")
        command("play")
        ensure(inspect()[0] == "正在处理…", "loading before Quit")
        started = time.monotonic()
        report = close(name, "quit")
        quit_seconds = time.monotonic() - started
        ensure(quit_seconds <= .8 and report["metadata_cancellations"] == 1 and report["metadata_timeouts"] == 0, "actual Quit cancels pending metadata within .8s")
        summary = {"passed": True, "actual_app_instances": 3, "timeout_UI_seconds": timeout_seconds, "stop_seconds": stop_seconds,
                   "quit_seconds": quit_seconds, "metadata_timeout_limit_ms": 2000, "controlled_completion_gate": True,
                   "external_path_fixed": False, "all_resources_zero": True}
    else:
        for tag in ("corrupt", "truncated"):
            name = launch(tag, tag)
            command("play")
            value = wait("发生错误", 3)
            ensure("帧队列 0 · PCM来源 0 · GPU票据 0" in value[0], "real bad file fails with cleared actual App queues")
            command("stop")
            ensure(inspect()[0].startswith("待机") and inspect()[1] == 0, "Stop recovers actual failure state")
            command("play")
            wait("发生错误", 3)
            close(name, "quit")
        for i in range(6):
            name = launch("normal", "Cycle" + str(i))
            command("play")
            wait("播放中", 2)
            if i % 2:
                command("pause")
                command("seek", str(30 + i * 10))
                ensure(inspect()[0].startswith("已暂停"), "reopened actual instance paused seek")
            close(name, "close" if i % 2 == 0 else "quit")
        summary = {"passed": True, "actual_bad_media_window_cases": 2, "continuous_normal_app_reopens": 6,
                   "actual_app_instances": len(reports), "all_resources_zero": True}
    command("verifyExited")
    summary.update({"commands": commands, "process_seconds": 50 - (DEADLINE - time.monotonic()), "physical_input": False,
                    "real_sound": False, "real_device_switch": False, "run_directory": str(RUN)})
    (RUN / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n")
    (OUT / (MODE + ".summary.json")).write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n")
    sources = [ROOT / p for p in ("main.mm", "controller.hpp", "controller.cpp", "host_clock.hpp", "device_monitor.hpp", "device_monitor.mm", "gui_native.mm", "reliability_gui.py")]
    sources += [ROOT.parent / "stream_lab/movie_io.hpp", ROOT.parent / "stream_lab/movie_io.mm"]
    sources += [HELPER, ROOT / "build/NativeMediaDemo.app/Contents/MacOS/NativeMediaDemo", RUN / "summary.json"]
    (RUN / "source-and-result.sha256").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p) + "\n" for p in sources))
    print("PASS actual App " + MODE + ": " + json.dumps(summary, ensure_ascii=False))
except Exception as error:
    (RUN / "summary.json").write_text(json.dumps({"passed": False, "error": str(error), "commands": commands}, ensure_ascii=False, indent=2) + "\n")
    import shutil
    failure = RUN / "failure-sources"
    failure.mkdir(exist_ok=True)
    for name in ("main.mm", "controller.cpp", "controller.hpp", "host_clock.hpp", "reliability_gui.py", "gui_native.mm"):
        shutil.copy2(ROOT / name, failure / name)
    for name in ("movie_io.hpp", "movie_io.mm"):
        shutil.copy2(ROOT.parent / "stream_lab" / name, failure / name)
    raise
finally:
    if active:
        command("quit")
