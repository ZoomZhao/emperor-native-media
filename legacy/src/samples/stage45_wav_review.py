import hashlib,json,struct,io,wave
from pathlib import Path
R=Path(__file__).resolve().parents[1]
original=Path('<LOCAL_MACHINE_PATH>]/Audio/Wavs/button.wav')
copy=R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]/Audio/Wavs/button.wav'
body=copy.read_bytes();assert body==original.read_bytes()
with wave.open(io.BytesIO(body),'rb')as f:
 pcm=f.readframes(f.getnframes());params=dict(channels=f.getnchannels(),rate=f.getframerate(),sample_bytes=f.getsampwidth(),frames=f.getnframes(),compression=f.getcomptype())
out=dict(guest_path='audio\\wavs\\button.wav',original_path=str(original),copy_path=str(copy),bytes=len(body),sha256=hashlib.sha256(body).hexdigest(),PCM_sha256=hashlib.sha256(pcm).hexdigest(),PCM_bytes=len(pcm),wave=params,original_and_frozen_copy_equal=True,readonly=True,original_MSS_code_executed=False,source_PCM_sent_to_device=False,contract_source='existing fixed legal mss32.dll readonly disassembly: file_read core 21110280; dest0 alloc size+4, raw bytes at base; length prefix only dest=-1; exact read then CloseHandle; set_sample_file core21114ca0 not yet bound')
(R/'evidence/stage45-button-wav-review.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
