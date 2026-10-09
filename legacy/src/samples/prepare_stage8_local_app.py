#!/usr/bin/env python3
"""Inspect and clone the local game without executing any bundled code."""
import hashlib, json, os, plistlib, shutil, struct, subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = Path('<LOCAL_MACHINE_PATH>')
destination = root / 'stage8-local/EmperorBaseline.app'
assert source.is_dir() and not destination.exists()
destination.parent.mkdir(parents=True, exist_ok=True)
contents = source / 'Contents'
info = plistlib.loads((contents/'Info.plist').read_bytes())
game = contents/'Resources/drive_c/EmperorRotMK[ZeaS]'
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def version(path):
    b=path.read_bytes(); offset=b.find(struct.pack('<II',0xfeef04bd,0x10000))
    if offset<0:return None
    ms,ls=struct.unpack_from('<II',b,offset+8)
    return [ms>>16,ms&65535,ls>>16,ls&65535]
source_exes=[dict(name=p.name,sha256=sha(p),size=p.stat().st_size,version=version(p)) for p in game.glob('Emperor*.exe')]
source_saves=[dict(path=str(p.relative_to(game)),size=p.stat().st_size,sha256=sha(p)) for p in game.rglob('*.sav')]
source_configs={str(p.relative_to(contents)):sha(p) for p in [contents/'Info.plist',contents/'Resources/user.reg',contents/'Resources/system.reg',contents/'Resources/userdef.reg',game/'Emperor.ini']}
launcher=contents/'MacOS/wineskinlauncher'
wine=contents/'SharedSupport/wine/bin/wine64'
signatures={}
for label,p in [('app',source),('wine64',wine)]:
    result=subprocess.run(['/usr/bin/codesign','--display','--verbose=3',str(p)],capture_output=True,text=True,timeout=10)
    signatures[label]=dict(returncode=result.returncode,output=result.stderr)
native_hashes=dict(launcher=sha(launcher),wine64=sha(wine))
source_scripts={p.name:dict(sha256=sha(p),text=p.read_text()) for p in (contents/'Resources/Scripts').iterdir() if p.is_file() and p.read_bytes().startswith(b'#!')}
print('read-only source audit complete; cloning active Contents only',flush=True)
# APFS copy-on-write clone preserves symlinks without following host folders.
destination.mkdir()
result=subprocess.run(['/bin/cp','-cR',str(contents),str(destination/'Contents')],capture_output=True,text=True,timeout=40)
assert result.returncode==0,result.stderr
copy_contents=destination/'Contents'
assert sha(copy_contents/'MacOS/wineskinlauncher')==native_hashes['launcher']
assert sha(copy_contents/'SharedSupport/wine/bin/wine64')==native_hashes['wine64']
edits=[]
# Only change the copy. Avoid sharing macOS preference identity and host folders.
copy_info=dict(info)
copy_info['CFBundleIdentifier']='local.codex.emperor.stage8.baseline'
copy_info['CFBundleName']='Emperor Baseline'
copy_info['Symlinks In User Folder']=False
for key in list(copy_info):
    if key.startswith('Symlink '):copy_info[key]=''
(copy_contents/'Info.plist').write_bytes(plistlib.dumps(copy_info))
edits.append('copy Info.plist: unique preference identifier; disable user-folder symlink creation; preserve original Chinese game launch target')
removed=[]
for base,dirs,files in os.walk(copy_contents,followlinks=False):
    for name in dirs+files:
        p=Path(base)/name
        if not p.is_symlink():continue
        relative=str(p.relative_to(copy_contents))
        if relative.startswith('Resources/dosdevices/') and name!='c:':
            removed.append(dict(path=relative,target=os.readlink(p)));p.unlink()
        elif relative.startswith('Resources/drive_c/users/'):
            try:p.resolve().relative_to(copy_contents.resolve())
            except ValueError:
                removed.append(dict(path=relative,target=os.readlink(p)));p.unlink();p.mkdir()
# The installed shutdown script opens a third-party website. Disable this action
# in the copy; never execute it as part of offline baseline collection.
shutdown=copy_contents/'Resources/Scripts/WineskinShutdownScript'
shutdown.write_text('#!/bin/sh\n# Offline baseline copy: no browser action.\nexit 0\n')
edits.append('copy shutdown script: remove open https://www.konami.cc/')
copy_game=copy_contents/'Resources/drive_c/EmperorRotMK[ZeaS]'
frozen=root/'stage7-data/save-input.sav'
save_copy=copy_game/'Save/zoom/Stage8-FixedInput.sav'
assert sha(frozen)=='e002894ce7dfa91d374715356708433afa70129bff673ce667f47b3520a12845'
assert not save_copy.exists()
shutil.copy2(frozen,save_copy)
os.chmod(save_copy,0o644)
for key,expected in source_configs.items():assert sha(contents/key)==expected
for item in source_saves:assert sha(game/item['path'])==item['sha256']
pending=['/usr/bin/open','-n',str(destination)]
result=dict(source_app=str(source),prepared_copy=str(destination),source_launcher_target=info['Program Name and Path'],
            source_bundle_identifier=info['CFBundleIdentifier'],source_wineskin_version=info['CFBundleVersion'],
            source_exes=source_exes,source_saves=source_saves,source_config_hashes=source_configs,
            native_hashes=native_hashes,signatures=signatures,source_scripts=source_scripts,
            copy_only_edits=edits,removed_external_mappings=removed,fixed_save_copy=str(save_copy),fixed_save_sha256=sha(save_copy),
            source_configs_and_saves_unchanged=True,pending_launch_command=pending,
            bundled_code_executed=False,game_started=False,game_save_loaded=False,game_advanced=False,new_game_save_created=False,
            security_boundary='filesystem mappings reduced; native bundle execution is not sandboxed and provenance is unverified')
(root/'evidence/stage8-local-app-preflight.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
print(json.dumps({k:result[k] for k in ['prepared_copy','source_launcher_target','fixed_save_copy','pending_launch_command','bundled_code_executed','source_configs_and_saves_unchanged']},indent=2,ensure_ascii=False))
