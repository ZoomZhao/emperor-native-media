import hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];copy=R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]/Audio';original=Path('<LOCAL_MACHINE_PATH>]/Audio');rows=[]
for p in sorted(p for p in copy.rglob('*') if p.is_file()):
 relative=p.relative_to(copy);src=original/relative;b=p.read_bytes();assert b==src.read_bytes();h=hashlib.sha256(b).hexdigest();rows.append(dict(relative_path='Audio/'+str(relative),copy_path=str(p),original_path=str(src),copy_size=len(b),original_size=len(b),copy_sha256=h,original_sha256=h,source_copy_equal=True))
assert len(rows)==880
(R/'evidence/stage40-audio-text-catalog.readonly.json').write_text(json.dumps(dict(rows=rows,original_DLL_executed=False,original_audio_played=False),indent=2));print(json.dumps([dict(path=r['relative_path'],bytes=r['copy_size'],sha256=r['copy_sha256'])for r in rows]))
