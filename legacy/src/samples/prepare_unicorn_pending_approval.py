#!/usr/bin/env python3
"""Prepared only: fixed download/hash/safe extraction. Not executed in stage 3.
Requires explicit approval for the new source and subsequent library execution.
"""
from pathlib import Path,PurePosixPath
import urllib.request,json,hashlib,zipfile,stat
root=Path(__file__).resolve().parents[1]
filename='unicorn-2.1.4-cp37-abi3-macosx_11_0_arm64.whl'
digest='d6c93e0f60328d8f4a1792af3f834137a28050fcc2305f2ec01efe8558a9844e'
with urllib.request.urlopen('https://pypi.org/pypi/unicorn/2.1.4/json',timeout=20) as response:metadata=json.load(response)
asset=next(a for a in metadata['urls'] if a['filename']==filename)
assert asset['digests']['sha256']==digest
assert asset['url'].startswith('https://files.pythonhosted.org/')
with urllib.request.urlopen(asset['url'],timeout=20) as response:
    assert response.url.startswith('https://files.pythonhosted.org/')
    data=response.read(14*1024*1024+1)
assert len(data)<=14*1024*1024 and len(data)==asset['size']
assert hashlib.sha256(data).hexdigest()==digest
path=root/'evidence'/filename;path.write_bytes(data)
target=root/'vendor-tools/unicorn-2.1.4';target.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(path) as archive:
    entries=archive.infolist();assert sum(i.file_size for i in entries)<=256*1024*1024
    for item in entries:
        p=PurePosixPath(item.filename);mode=item.external_attr>>16
        assert not p.is_absolute() and '..' not in p.parts and not stat.S_ISLNK(mode)
    for item in entries:
        p=target.joinpath(*PurePosixPath(item.filename).parts)
        if item.is_dir():p.mkdir(parents=True,exist_ok=True)
        else:p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(archive.read(item))
print(json.dumps(dict(filename=filename,bytes=len(data),sha256=digest,extracted=str(target),package_code_executed=False)))
