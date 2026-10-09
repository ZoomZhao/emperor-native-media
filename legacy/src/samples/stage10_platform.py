#!/usr/bin/env python3
"""Measured finite GetCPInfo contract, with unknown semantics left unsupported.

Official GetCPInfo has two parameters and no flags or buffer-length parameter.
This profile reproduces the approved Wine baseline, including untouched padding.
It is not a general NLS implementation or a claim about every Windows build.
"""
import hashlib
import json
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
MATRIX_SHA='738feb4bc3b767c6beb8f990b0819be4d4d52aec1c70ead558651ebdebd2d944'

class UnsupportedNls(Exception):
    pass

class VersionProfile:
    def __init__(self):
        data=(ROOT/'evidence/stage10-VERSIONCASES-output.bin').read_bytes()
        assert hashlib.sha256(data).hexdigest()=='53323199e36d825a292f457692c5a50d5aa4b2b8c90a8b8764d8101004580939'
        record=json.loads((ROOT/'evidence/stage10-VERSIONCASES-result.json').read_text())
        assert record['GetVersion']=='0x1db10106'
        self.packed=int(record['GetVersion'],16)
        self.rows={row['declared_size']:row for row in record['rows']}
        self.fields=[6,1,7601,2]
        self.service_pack=b'Service Pack 1\0'
        for size in [148,156]:
            row=self.rows[size];body=bytes.fromhex(row['buffer_hex'])[4:164]
            assert row['return_value']==1 and row['last_error']==0x13579
            assert body[4:20]==struct.pack('<4I',*self.fields)
            assert body[20:35]==self.service_pack and body[35:148]==bytes([0xa5])*113
            if size==156:assert body[148:155]==b'\x01\x00\x00\x00\x00\x00\x01' and body[155]==0xa5
        for size,row in self.rows.items():
            if size not in (148,156):
                assert row['return_value']==0 and row['last_error']==122
                assert bytes.fromhex(row['buffer_hex'])==bytes([0xa5])*4+struct.pack('<I',size)+bytes([0xa5])*160

    def info(self,size):
        if size not in (148,156):return dict(return_value=0,last_error=122,writes=[])
        writes=[(4,struct.pack('<4I',*self.fields)),(20,self.service_pack)]
        if size==156:writes.append((148,b'\x01\x00\x00\x00\x00\x00\x01'))
        return dict(return_value=1,last_error=None,writes=writes)

class NlsProfile:
    def __init__(self):
        data=(ROOT/'evidence/stage10-nls-matrix-output.bin').read_bytes()
        assert hashlib.sha256(data).hexdigest()==MATRIX_SHA
        record=json.loads((ROOT/'evidence/stage10-nls-matrix-guestpath-result.json').read_text())
        assert record['output_sha256']==MATRIX_SHA and record['ACP']==936
        self.ACP=record['ACP'];self.success={};self.invalid=set()
        for row in record['rows']:
            cp=row['code_page']
            if row['return_value']==0:
                assert cp==0xffffffff and row['last_error']==87
                assert bytes.fromhex(row['canary_buffer_hex'])==bytes([0xa5])*32
                self.invalid.add(cp)
            else:
                assert not row['null_pointer'] and row['last_error']==0x13579
                b=bytes.fromhex(row['CPINFO_hex'])
                length=8 if cp==65001 else 18
                assert b[length:]==bytes([0xa5])*(20-length)
                surrounding=bytes.fromhex(row['canary_buffer_hex'])
                assert surrounding[:4]==bytes([0xa5])*4 and surrounding[24:]==bytes([0xa5])*8
                self.success[cp]=b[:length]
        assert set(self.success)=={936,0,1,2,3,65001,1252}
        self.ctype=(ROOT/'evidence/stage10-ctype1-table.bin').read_bytes()
        assert len(self.ctype)==131072
        assert hashlib.sha256(self.ctype).hexdigest()=='94c72ac4d272d81f1bf51fee5e96add606df7e9faf7cbd8b44c78e2b972de4af'
        mb=(ROOT/'evidence/stage10-MB936-output.bin').read_bytes()
        assert hashlib.sha256(mb).hexdigest()=='ee2c3014030779f6ef36e7ab39c0084b6a6d5e3b2d72f56ae301fb2264566766'
        self.mb={}
        for row in json.loads((ROOT/'evidence/stage10-MB936-result.json').read_text())['rows']:
            if row['label'].startswith('single-'):
                byte=int(row['label'][7:]);b=bytes.fromhex(row['canary_buffer_hex'])
                assert row['return_value']==1 and row['last_error']==0x13579
                assert b[:4]==bytes([0xa5])*4 and b[6:]==bytes([0xa5])*10
                self.mb[byte]=b[4:6]
        assert len(self.mb)==256
        lc=(ROOT/'evidence/stage10-LCMAP-output.bin').read_bytes()
        assert hashlib.sha256(lc).hexdigest()=='db55ef50041f8f1c26b0bf15331526ca9e34bda745155e848c90de018abe000b'
        self.casing={0x100:{},0x200:{}}
        for row in json.loads((ROOT/'evidence/stage10-LCMAP-result.json').read_text())['rows']:
            if row['label'].startswith(('actual-output-','nul-output-')):
                count=row['length'];assert row['return_value']==count and row['last_error']==0x13579
                source=bytes.fromhex(row['source_hex']);result=bytes.fromhex(row['canary_buffer_hex'])[4:4+count*2]
                mapping=self.casing[row['flags']]
                for a,b in zip(struct.unpack('<'+str(count)+'H',source),struct.unpack('<'+str(count)+'H',result)):
                    assert a not in mapping or mapping[a]==b;mapping[a]=b
        assert all(set(m)==set(range(128))|{0x20ac,0xf8f5} for m in self.casing.values())
        lc804=(ROOT/'evidence/stage10-LCMAP804-output.bin').read_bytes()
        assert lc804==lc
        probe804=json.loads((ROOT/'evidence/stage10-LCMAP804-source.json').read_text())
        assert all(op['args'][0]==0x804 for op in probe804['operations'])
        wc=(ROOT/'evidence/stage10-WC936-output.bin').read_bytes()
        assert hashlib.sha256(wc).hexdigest()=='7f1e13e4b84bf4443b4f2292b11cb2865f1cf895d2b9156f788cc630de893d08'
        self.wc={}
        for row in json.loads((ROOT/'evidence/stage10-WC936-result.json').read_text())['rows']:
            if row['label'].startswith('single-'):
                char=int(row['label'][7:]);b=bytes.fromhex(row['canary_buffer_hex'])
                assert row['return_value']==1 and row['last_error']==0x13579 and row['used_default']==0
                assert b[:4]==bytes([0xa5])*4 and b[5:]==bytes([0xa5])*7
                self.wc[char]=b[4]
        assert set(self.wc)==set(range(128))|{0x20ac,0xf8f5}

    def get_cp_info(self,code_page):
        if code_page in self.invalid:
            return dict(return_value=0,last_error=87,payload=b'')
        if code_page not in self.success:
            raise UnsupportedNls('code page has no measured platform contract: '+hex(code_page))
        return dict(return_value=1,last_error=None,payload=self.success[code_page])

    def string_type(self,info_type,source):
        if info_type in (0,3):return dict(return_value=0,last_error=87,payload=b'')
        if info_type!=1:raise UnsupportedNls('unmeasured character info type: '+hex(info_type))
        assert len(source)%2==0
        words=struct.unpack('<'+str(len(source)//2)+'H',source) if source else ()
        if any(0xd800<=c<=0xdfff for c in words):
            raise UnsupportedNls('surrogate context has no independent profile verification')
        payload=b''.join(self.ctype[c*2:c*2+2] for c in words)
        return dict(return_value=1,last_error=None,payload=payload)

    def mb_to_wide(self,cp,flags,source,capacity):
        if cp not in (0,936):raise UnsupportedNls('unmeasured conversion code page: '+hex(cp))
        if flags==0xffffffff:return dict(return_value=0,last_error=1004,payload=b'')
        if flags==8 and source==b'\xff':return dict(return_value=0,last_error=1113,payload=b'')
        if flags not in (0,1):raise UnsupportedNls('unmeasured conversion flags: '+hex(flags))
        if not source or capacity<0:return dict(return_value=0,last_error=87,payload=b'')
        if len(source)>1 and any(0x81<=b<=0xfe for b in source):
            raise UnsupportedNls('DBCS pairs need independently measured conversion, not singleton-byte substitution')
        wide=b''.join(self.mb[b] for b in source)
        if capacity==0:return dict(return_value=len(source),last_error=None,payload=b'')
        if capacity<len(source):return dict(return_value=0,last_error=122,payload=wide[:2*capacity])
        return dict(return_value=len(source),last_error=None,payload=wide)

    def lc_map(self,locale,flags,source,capacity):
        if locale not in (0,0x804):raise UnsupportedNls('LCID has no measured casing contract: '+hex(locale))
        if flags==0xffffffff:return dict(return_value=0,last_error=1004,payload=b'')
        if flags not in self.casing:raise UnsupportedNls('unmeasured mapping flags: '+hex(flags))
        if not source:return dict(return_value=0,last_error=87,payload=b'')
        if capacity<0:raise UnsupportedNls('unmeasured negative mapping capacity')
        words=struct.unpack('<'+str(len(source)//2)+'H',source);mapping=self.casing[flags]
        if any(c not in mapping for c in words):raise UnsupportedNls('Unicode casing outside independently measured CRT subset')
        if capacity==0:return dict(return_value=len(words),last_error=None,payload=b'')
        payload=struct.pack('<'+str(min(capacity,len(words)))+'H',*[mapping[c] for c in words[:capacity]])
        if capacity<len(words):return dict(return_value=0,last_error=122,payload=payload)
        return dict(return_value=len(words),last_error=None,payload=payload)

    def wide_to_mb(self,cp,flags,source,capacity):
        if cp not in (0,936):raise UnsupportedNls('unmeasured reverse conversion code page')
        if flags==0xffffffff:return dict(return_value=0,last_error=1004,payload=b'',used_default=None)
        if flags!=0x220:raise UnsupportedNls('unmeasured reverse conversion flags: '+hex(flags))
        if not source or capacity<0:return dict(return_value=0,last_error=87,payload=b'',used_default=None)
        words=struct.unpack('<'+str(len(source)//2)+'H',source)
        if any(c not in self.wc for c in words):raise UnsupportedNls('unmeasured DBCS/best-fit/default-character reverse conversion')
        if capacity==0:return dict(return_value=len(words),last_error=None,payload=b'',used_default=None)
        payload=bytes(self.wc[c] for c in words[:capacity])
        if capacity<len(words):return dict(return_value=0,last_error=122,payload=payload,used_default=None)
        return dict(return_value=len(words),last_error=None,payload=payload,used_default=0)
