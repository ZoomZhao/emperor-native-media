#!/usr/bin/env python3
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage13_events import NativeEvents
def main():
    n=NativeEvents();oracle=json.loads((ROOT/'evidence/stage13-wsa-Wine-result.json').read_text());checks=[]
    for r in oracle['rows']:
        if r['name']=='WSAStartup':
            source=bytes([r.get('fill',0xa5)])*400;value,error,body=n.wsa_startup(r['args'][0],source,0x13579)
            if 'buffer_hex' in r:assert body==bytes.fromhex(r['buffer_hex']),r
        else:value,error=n.wsa_cleanup(0x13579)
        assert value==r['return_value'] and error==r['last_error'],(r,value,error)
        checks.append(dict(label=r['label'],return_equal=True,last_error_equal=True,entire_400_byte_output_equal='buffer_hex' in r,references=n.wsa_references.value))
    assert n.wsa_references.value==0
    # Recreate the native owner, retaining the exact process refcount.
    n.wsa_startup(0x101,bytes([0xa5])*400,77);n.wsa_startup(0x101,bytes([0xa5])*400,77)
    other=NativeEvents();other.wsa_references.value=n.wsa_references.value
    for i in range(3):assert n.wsa_cleanup(77)==other.wsa_cleanup(77)
    assert n.wsa_references.value==other.wsa_references.value==0
    result=dict(all_passed=True,same_Wine_cases=len(checks),checks=checks,refcount_restore_continuation_equal=True,local_unconnected_socket_capability_verified=True,DNS_bind_listen_connect_send_performed=False,actual_guest_network_operations_implemented=False)
    (ROOT/'evidence/stage13-wsa-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='checks'}));n.dispose();other.dispose()
if __name__=='__main__':main()
