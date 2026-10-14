"""Read-only SAV framing/anchor audit from published data-format facts.

No game code, building/walker parser, sprite selector or full save restoration.
Original inputs come only from EMPEROR_GAMEDATA_ROOT; unset/empty succeeds SKIP.
"""
from pathlib import Path
from dataclasses import dataclass
import argparse,json,os,struct,zlib
from map_sg3_audit import decode_container as decode_map,city_fields

MAX_INPUT=4*1024*1024
MAX_DECODED=8*1024*1024
CELLS=228*228
BRIEF=b'Brief descriptio'
TERRAIN_MARKER=bytes.fromhex('00000000050008000500080005000800')


def decode_save_container(data):
    if not 4<=len(data)<=MAX_INPUT or struct.unpack_from('<I',data)[0]!=0xfedcbaaa:
        raise ValueError('SAV outer header/budget')
    decoded=bytearray();position=4;frames=0
    while position<len(data):
        if len(data)-position<12 or frames>=256:raise ValueError('SAV frame header/count')
        marker,encoded,declared=struct.unpack_from('<III',data,position)
        if (marker!=0xabcdefff or not 0<encoded<=65536 or not 0<declared<=32768
                or declared>MAX_DECODED-len(decoded)):
            raise ValueError('SAV frame budget/marker')
        end=position+12+encoded
        if end>len(data):raise ValueError('SAV frame truncation')
        try:
            stream=zlib.decompressobj();part=stream.decompress(data[position+12:end],declared+1)
        except zlib.error as error:raise ValueError('SAV zlib/checksum')from error
        if len(part)!=declared or not stream.eof or stream.unused_data or stream.unconsumed_tail:
            raise ValueError('SAV complete zlib stream/length')
        decoded+=part;position=end;frames+=1
    if not frames:raise ValueError('SAV empty container')
    return bytes(decoded),frames


def marker_positions(data,needle,start=0):
    hits=[]
    while True:
        position=data.find(needle,start)
        if position<0:return hits
        if len(hits)>=1024:raise ValueError('SAV marker cardinality budget')
        hits.append(position);start=position+1


@dataclass(frozen=True)
class SaveDiagnostic:
    payload:bytes
    frames:int
    side:int
    size_offset:int
    terrain_offset:int
    following_offset:int

    def grid(self,field):
        import numpy as np
        if field not in ('terrain','following_u32'):raise ValueError('unknown diagnostic field')
        offset=self.terrain_offset if field=='terrain'else self.following_offset
        return np.frombuffer(self.payload,'<u4',CELLS,offset).reshape(228,228)

    def selected(self,field,cells):
        cells=list(cells)
        if len(cells)>128 or any(len(c)!=2 or any(type(v)is not int for v in c)
                or not all(0<=v<self.side for v in c)for c in cells):
            raise ValueError('bounded canonical cells required')
        grid=self.grid(field);origin=(228-self.side)//2
        return [int(grid[y+origin,x+origin])for x,y in cells]


def parse_save(data):
    payload,frames=decode_save_container(data)
    if payload[:2]!=b'\x13\0':raise ValueError('unsupported SAV role')
    anchors=[(position-16,struct.unpack_from('<I',payload,position-16)[0])
             for position in marker_positions(payload,BRIEF)if position>=16]
    anchors=[(offset,side)for offset,side in anchors if 1<=side<=228]
    if len(anchors)!=1:raise ValueError('SAV size anchor ambiguous/unavailable')
    size_offset,side=anchors[0]
    markers=marker_positions(payload,TERRAIN_MARKER,size_offset+4)
    # Conservative scope: do not choose among duplicate data markers by MAP similarity.
    if len(markers)!=1:raise ValueError('SAV terrain anchor ambiguous/unavailable')
    terrain_offset=markers[0]+4;following_offset=terrain_offset+4*CELLS
    if following_offset+4*CELLS>len(payload):raise ValueError('SAV complete grid spans unavailable')
    result=SaveDiagnostic(payload,frames,side,size_offset,terrain_offset,following_offset)
    import numpy as np
    terrain=result.grid('terrain');active=np.argwhere((terrain&0x80000)==0);origin=(228-side)//2
    if (not len(active) or not np.array_equal(active.min(axis=0),[origin,origin])
            or not np.array_equal(active.max(axis=0),[origin+side-1,origin+side-1])):
        raise ValueError('SAV declared centered crop disagrees')
    return result


def rooted(root,relative):
    root=Path(root).resolve()
    name=Path(relative)
    if name.is_absolute()or any(part in ('.','..')for part in name.parts):raise ValueError('relative root path required')
    path=(root/name).resolve()
    if not path.is_relative_to(root)or not path.is_file():raise ValueError('root escape/missing file')
    return path


def read_bounded(path):
    with path.open('rb')as stream:data=stream.read(MAX_INPUT+1)
    if len(data)>MAX_INPUT:raise ValueError('input budget')
    return data


def audit(root,points=()):
    import numpy as np
    root=Path(root).resolve()
    catalog=[];map_rejected=0
    for name in sorted((root/'Cities').glob('*.map')):
        path=rooted(root,str(name.relative_to(root)))
        payload=decode_map(read_bounded(path))
        if payload[:8]!=bytes.fromhex('0500feca00000200'):map_rejected+=1;continue
        city_fields(payload)  # Existing complete city role/side/grid span validation.
        catalog.append((name.name,np.frombuffer(payload,'<u4',CELLS,261455).reshape(228,228).copy()))
    saves=sorted(p for p in (root/'Save').rglob('*')if p.is_file()and p.name.lower().endswith('.sav'))
    if len(saves)>256:raise ValueError('save file count budget')
    output=[]
    for name in saves:
        path=rooted(root,str(name.relative_to(root)));before=path.stat();data=read_bounded(path)
        result=parse_save(data);second=read_bounded(path);after=path.stat()
        if data!=second or(before.st_size,before.st_mtime_ns)!=(after.st_size,after.st_mtime_ns):
            raise ValueError('save changed during audit')
        terrain=result.grid('terrain')
        matches=[label for label,grid in catalog if np.array_equal(terrain,grid)]
        row={'relative':str(name.relative_to(root)),'bytes':len(data),'mtime_ns':before.st_mtime_ns,
             'logical_role':'SAV13','decoded_bytes':len(result.payload),'frames':result.frames,
             'side':result.side,'terrain_offset':result.terrain_offset,'following_u32_offset':result.following_offset,
             'exact_terrain_identity_matches':matches,'city_identity_denominator':len(catalog),'two_read_stable':True}
        if points:
            values=result.selected('following_u32',points)
            row['selected_following_u32']={'cells':len(values),'zero_count':sum(v==0 for v in values),'distinct_values':len(set(values))}
        output.append(row)
    return {'save_files':len(output),'containers_and_grid_spans_verified':len(output),
            'city_MAPs':len(catalog),'non_city_MAPs':map_rejected,'files':output,'graphics_grid_semantics_verified':False}


def main():
    env=os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not env:print(json.dumps({'skipped':True,'reason':'EMPEROR_GAMEDATA_ROOT unset'}));return 0
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--sample-manifest',type=Path);args=parser.parse_args()
    root=Path(env)
    if not root.is_absolute()or not root.is_dir():parser.error('existing absolute environment root required')
    points=json.loads(args.sample_manifest.read_text())['cells']if args.sample_manifest else ()
    try:print(json.dumps(audit(root.resolve(),points),ensure_ascii=False))
    except ValueError as error:print(str(error),file=__import__('sys').stderr);return 1
    return 0

if __name__=='__main__':raise SystemExit(main())
