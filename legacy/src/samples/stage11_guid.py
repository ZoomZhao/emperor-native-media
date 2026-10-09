#!/usr/bin/env python3
"""Measured GUID layout, production OS entropy; injected providers are tests."""
import os,uuid

def create_guid():
    r=bytearray(os.urandom(16));r[6]=(r[6]&15)|0x40;r[8]=(r[8]&63)|0x80
    return uuid.UUID(bytes=bytes(r)).bytes_le

def guid_hresult(rpc_status):
    if rpc_status in (0,1824):return 0
    return rpc_status if rpc_status&0x80000000 else 0x80070000|(rpc_status&65535)
