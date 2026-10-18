# SPDX-License-Identifier: MIT
"""Authored-only black-box checks for the published residual classifier.
Expectations: published per-cell accounting, two supported rejection reasons,
payload validation before classification success, and logical mirror identity.
No original GameData or executable is read.
"""
from pathlib import Path
import json,os,struct,subprocess,tempfile,zlib

def put(b,p,n,w=4):b[p:p+w]=int(n).to_bytes(w,'little',signed=n<0)
def fixture(root,kind):
    (root/'Cities').mkdir(parents=True);(root/'DATA').mkdir()
    b=bytearray(1093607);b[0]=5;b[2]=254;b[3]=202;b[6]=2;put(b,84,84)
    for i in range(51984):put(b,261455+4*i,0x80000)
    for y in range(2):
        for x in range(2):
            i=(82+y)*228+82+x;put(b,261455+4*i,0);put(b,1535+4*i,(3<<14)|(1 if kind=='mirror' and x else 0));b[209471+i]=8*y+x;b[677327+i]=1 if kind in ['accepted','mixed_sides','other_reason'] else 0
            if kind=='mixed_sides' and x==1 and y==1:b[677327+i]=0
            if kind=='other_reason':b[209471+i]=255
    if kind in ['plain','rle','mirror']:
        typ=256 if kind=='rle' else 0;width=height=2;pixels=bytes([4,0xe0,3,0xe0,3,0xe0,3,0xe0,3]) if typ==256 else bytes([0xe0,3])*4;uncompressed=0
    else:typ=30;width=158;height=80;pixels=bytes([0xe0,3])*6400;uncompressed=12800
    record_count=202 if kind=='mirror' else 201;capacity=record_count+1;index=bytearray(40680+capacity*64);put(index,0,len(index));put(index,4,213);put(index,12,capacity);put(index,16,record_count)
    q=40680+201*64;put(index,q+4,len(pixels));put(index,q+8,uncompressed);put(index,q+20,width,2);put(index,q+22,height,2);put(index,q+50,typ,2);index[q+55]=2 if typ==30 else 0
    if kind=='mirror':put(index,40680+202*64+16,-1)
    (root/'DATA/China_Terrain.sg3').write_bytes(index)
    if kind!='missing':(root/'DATA/China_Terrain.555').write_bytes(pixels if kind!='truncated' else pixels[:2])
    packed=bytearray(struct.pack('<I',0xfedcbaaa))
    for start in range(0,len(b),32768):
        data=b[start:start+32768];compressed=zlib.compress(data,9);packed+=struct.pack('<III',0xabcdefff,len(compressed),len(data))+compressed
    (root/'Cities/own.map').write_bytes(packed)

def run(binary,build):
    results=[];out=Path(tempfile.mkdtemp(prefix="owned-residual-cases-",dir=build))
    for kind in ['conflict','mixed_sides','accepted','plain','rle','mirror','missing','truncated','other_reason']:
        root=out/kind;root.mkdir();fixture(root,kind)
        env=dict(os.environ,EMPEROR_GAMEDATA_ROOT=str(root),TMPDIR=str(out),PYTHONDONTWRITEBYTECODE='1')
        r=subprocess.run([str(binary),'--metrics'],capture_output=True,text=True,env=env,timeout=20)
        row={'kind':kind,'returncode':r.returncode,'stderr':r.stderr}
        if kind in ['missing','truncated','other_reason']:
            expected_error='residual classification needs an additional geometry reason' if kind=='other_reason' else 'residual payload does not decode'
            row['passed']=r.returncode!=0 and expected_error in r.stderr
        else:
            d=json.loads(r.stdout);row['result']=d
            residual=0 if kind=='accepted' else 1 if kind=='mixed_sides' else 4
            logical=0 if kind=='accepted' else 2 if kind=='mirror' else 1
            expected_type=0 if kind in ['plain','mirror'] else 256 if kind=='rle' else 30
            row['passed']=r.returncode==0 and d['residue']==d['residual_classified_cells']==d['residual_payload_decoded_cells']==residual and d['residual_unique_decoded_records']==logical and d['decoded_cells']==4-residual and d['residual_mirrored_cells']==(2 if kind=='mirror' else 0) and d['located_issue_cells']==residual and d['unlocated_issue_cells']==0
            if residual:row['passed']=row['passed'] and d['residual_classes'][0]['cells']==residual and d['residual_classes'][0]['raw_type_flags']==expected_type
        results.append(row)
    if not all(row['passed'] for row in results):
        raise RuntimeError(json.dumps({'residual_audit_failures':[r for r in results if not r['passed']]}))
    return len(results)
