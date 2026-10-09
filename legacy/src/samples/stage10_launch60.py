#!/usr/bin/env python3
"""60-second owner of the already approved English baseline process."""
import hashlib
import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / 'stage8-local/EmperorBaseline.app/Contents'
source = (ROOT / 'samples/stage8_mac_ui.py').read_text().split('p=argparse.ArgumentParser();', 1)[0]
ns = {'__file__': str(ROOT / 'samples/stage8_mac_ui.py')}
exec(compile(source, ns['__file__'], 'exec'), ns)
command = [str(APP / 'MacOS/wineskinlauncher')]
started = time.monotonic()
tag = sys.argv[1] if len(sys.argv)>1 else 'initial'
assert tag.isalnum()
automatic = len(sys.argv)>2 and sys.argv[2]=='auto'
suffix = '' if tag=='initial' else '-'+tag
record = dict(command=command, cwd=str(APP), budget_seconds=60)
game = None
log = ROOT / f'evidence/stage10-EN{suffix}-launcher.log'
with log.open('w') as output:
    child = subprocess.Popen(command, cwd=APP, stdout=output, stderr=subprocess.STDOUT)
    record['launcher_pid'] = child.pid
    print(json.dumps(record), flush=True)
    try:
        if automatic:
            while time.monotonic()-started<10:
                game=next((w for w in ns['windows']() if w['owner']=='wine32on64-preloader' and w['bounds']['Width']==1024 and w['bounds']['Height']==796),None)
                if game:break
                time.sleep(.1)
            assert game,'no baseline window within ten seconds'
            time.sleep(1)
            ui_command=['/usr/bin/python3','-I',str(ROOT/'samples/stage10_ui_batch.py'),'all','--window',str(game['id'])]
            ui=subprocess.run(ui_command,capture_output=True,text=True,timeout=min(35,max(1,50-(time.monotonic()-started))))
            record['automatic_UI']=dict(command=ui_command,returncode=ui.returncode,stdout=ui.stdout,stderr=ui.stderr)
            if ui.returncode==0:
                record['end_reason']='automatic normal UI exit'
                (ROOT/'evidence/stage10-UI-exit.json').write_text((ROOT/'evidence/stage10-UI-all.json').read_text())
        while time.monotonic()-started < 55:
            windows = ns['windows']()
            game = next((w for w in windows if w['owner'] == 'wine32on64-preloader' and w['bounds']['Width'] == 1024 and w['bounds']['Height'] == 796), game)
            if game and (ROOT / 'evidence/stage10-UI-exit.json').exists() and not any(w['id'] == game['id'] for w in windows):
                record['end_reason'] = 'normal UI exit and game window gone'
                break
            time.sleep(.1)
        else:
            record['end_reason'] = 'deadline cleanup'
            # Only a window that this launched baseline actually produced is
            # considered. Normal exit is attempted if it reached the city.
            if game and any(w['id'] == game['id'] for w in ns['windows']()):
                if (ROOT / 'evidence/stage10-UI-compare.json').exists() or (ROOT/'evidence/stage10-EN-after-city.png').exists():
                    ns['focus'](game);ns['click'](game,52,183);time.sleep(.2)
                    ns['click'](game,471,445);time.sleep(.3)
                if any(w['id'] == game['id'] for w in ns['windows']()):
                    os.kill(game['pid'], signal.SIGTERM)
                    record['owned_game_deadline_SIGTERM'] = game['pid']
        remaining = max(.1, 59.5-(time.monotonic()-started))
        try:
            child.wait(timeout=remaining)
        except subprocess.TimeoutExpired:
            child.terminate();record['owned_launcher_deadline_SIGTERM'] = child.pid
            try:child.wait(timeout=.3)
            except subprocess.TimeoutExpired:child.kill();child.wait(timeout=.1)
    finally:
        record.update(game_window=game, launcher_returncode=child.poll(), elapsed_seconds=round(time.monotonic()-started,3), final_windows=ns['windows']())
        wine_log = APP / 'Resources/Logs/LastRunWine.log'
        if wine_log.is_file():
            target = ROOT / f'evidence/stage10-EN{suffix}-module-Wine.log'
            target.write_bytes(wine_log.read_bytes())
            record['wine_log_sha256'] = hashlib.sha256(target.read_bytes()).hexdigest()
        (ROOT / f'evidence/stage10-EN{suffix}-launch.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n')
        print(json.dumps(record,ensure_ascii=False), flush=True)
