#!/usr/bin/env python3
"""Own Apple UI script for previously inspected normal menu coordinates."""
import argparse
import json
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'samples/stage8_mac_ui.py').read_text().split('p=argparse.ArgumentParser();', 1)[0]
ns = {'__file__': str(ROOT / 'samples/stage8_mac_ui.py')}
exec(compile(source, ns['__file__'], 'exec'), ns)
p = argparse.ArgumentParser()
p.add_argument('action', choices=['navigate', 'compare', 'exit', 'all'])
p.add_argument('--window', required=True, type=int)
a = p.parse_args()
started = time.monotonic()
w = ns['game_window'](a.window)
assert ns['cg'].CGPreflightScreenCaptureAccess()
assert ns['cg'].CGPreflightPostEventAccess()
events = []


def click(x, y, label, delay=.3):
    ns['game_window'](w['id'])
    ns['focus'](w)
    ns['click'](w, x, y)
    time.sleep(delay)
    events.append(dict(action='click', x=x, y=y, label=label, elapsed=round(time.monotonic()-started, 3)))


def capture(label):
    path = ROOT / f'evidence/stage10-{label}.png'
    r = subprocess.run(['/usr/sbin/screencapture', '-x', '-o', '-l', str(w['id']), str(path)], capture_output=True, text=True, timeout=5)
    assert r.returncode == 0 and path.is_file(), r.stderr
    events.append(dict(action='capture', label=label, path=str(path), elapsed=round(time.monotonic()-started, 3)))


def modules(label):
    results=[]
    for command in [['/usr/sbin/lsof','-p',str(w['pid']),'-Fpn'],['/usr/bin/vmmap','-w',str(w['pid'])]]:
        try:
            r=subprocess.run(command,capture_output=True,text=True,timeout=3)
            result=dict(command=command,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr)
        except subprocess.TimeoutExpired as e:
            result=dict(command=command,timed_out=True,stderr='three-second read-only snapshot budget')
        results.append(result)
    out=ROOT/f'evidence/stage10-{label}-module-snapshot.json'
    out.write_text(json.dumps(dict(game_pid=w['pid'],results=results),ensure_ascii=False,indent=2)+'\n')
    events.append(dict(action='read_only_module_snapshot',path=str(out),elapsed=round(time.monotonic()-started,3)))


try:
    if a.action in ['navigate','all']:
        click(360, 363, 'normal single player')
        capture('EN-player-menu')
        click(310, 321, 'existing zoom player')
        click(650, 546, 'continue existing player')
        capture('EN-player-submenu')
        click(510, 467, 'normal Load game')
        capture('EN-save-list')
    if a.action in ['compare','all']:
        click(350, 408, 'select ST9BASE fourth row')
        capture('EN-base-selected-name')
        click(650, 547, 'Load ST9BASE', .25)
        click(23, 53, 'open normal File menu', .05)
        capture('EN-base-city')
        modules('EN-base')
        click(52, 124, 'normal Load dialog')
        capture('EN-after-load-list')
        click(350, 392, 'select ST9AFTR third row')
        capture('EN-after-selected-name')
        click(650, 547, 'Load ST9AFTR', .25)
        click(23, 53, 'open normal File menu', .05)
        capture('EN-after-city')
        modules('EN-after')
    if a.action in ['exit','all']:
        click(52, 183, 'normal File Exit')
        capture('EN-exit-confirmation')
        click(471, 445, 'normal Exit Yes')
        time.sleep(.3)
        events.append(dict(action='windows_after_exit', windows=ns['windows']()))
finally:
    out = dict(window=w, action=a.action, events=events, elapsed_seconds=round(time.monotonic()-started, 3), coordinate_source='Actual inspected stage9 normal menus; stage10 screenshots retained for verification')
    (ROOT / f'evidence/stage10-UI-{a.action}.json').write_text(json.dumps(out, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps(out, ensure_ascii=False), flush=True)
