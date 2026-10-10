"""Bounded in-memory version of the authored TLV codec; no files or HostStore.

Tags and version retain the data-only format in archived binary_host.py.
Only exact builtin data types are accepted. This module imports no legacy code.
"""
import array
import collections
import hashlib
import struct
import sys

MAGIC = b'S83H\x02\0\0\0'
DEFAULT_MAX_BYTES = 16 * 1024 * 1024
DEFAULT_MAX_NODES = 100_000
DEFAULT_MAX_DEPTH = 64
DEFAULT_MAX_INT_DIGITS = 4096


class _Budget:
    def __init__(self, max_bytes, max_nodes, max_depth, max_int_digits):
        values = ((max_bytes, 8, 64 * 1024 * 1024),
                  (max_nodes, 1, 1_000_000), (max_depth, 0, 256),
                  (max_int_digits, 1, 4096))
        if any(type(v) is not int or not low <= v <= high for v, low, high in values):
            raise ValueError('BINARY_INVALID_LIMITS')
        self.max_bytes = max_bytes
        self.nodes = max_nodes
        self.max_depth = max_depth
        self.max_int_digits = max_int_digits

    def consume(self, count=1):
        if count > self.nodes:
            raise ValueError('BINARY_NODE_BUDGET')
        self.nodes -= count

    def depth(self, value):
        if value > self.max_depth:
            raise ValueError('BINARY_DEPTH')


def dumps(value, *, max_bytes=DEFAULT_MAX_BYTES, max_nodes=DEFAULT_MAX_NODES,
          max_depth=DEFAULT_MAX_DEPTH, max_int_digits=DEFAULT_MAX_INT_DIGITS):
    """Return TLV bytes; reject cycles and excess bytes/nodes/depth before growth."""
    budget = _Budget(max_bytes, max_nodes, max_depth, max_int_digits)
    output = bytearray(MAGIC)
    active = set()

    def write(data):
        if len(data) > budget.max_bytes - len(output):
            raise ValueError('BINARY_BYTE_BUDGET')
        output.extend(data)

    def emit(v, depth=0):
        budget.depth(depth)
        budget.consume()
        kind = type(v)
        if v is None:
            write(b'n')
        elif kind is bool:
            write(b'y' if v else b'z')
        elif kind is int:
            if 0 <= v <= 0xffffffff:
                write(b'i' + struct.pack('<I', v))
            elif -0x8000000000000000 <= v <= 0x7fffffffffffffff:
                write(b'I' + struct.pack('<q', v))
            else:
                if v.bit_length() > budget.max_int_digits * 4 + 1:
                    raise ValueError('BINARY_INTEGER_DIGITS')
                try:
                    data = str(v).encode('ascii')
                except ValueError as error:
                    raise ValueError('BINARY_INTEGER_DIGITS') from error
                if len(data.lstrip(b'-')) > budget.max_int_digits:
                    raise ValueError('BINARY_INTEGER_DIGITS')
                write(b'G' + struct.pack('<Q', len(data)))
                write(data)
        elif kind is float:
            write(b'f' + struct.pack('<d', v))
        elif kind in (bytes, str):
            if kind is bytes:
                data = v
            else:
                # A UTF-8 string needs at least one byte per Python character.
                if len(v) > budget.max_bytes - len(output) - 9:
                    raise ValueError('BINARY_BYTE_BUDGET')
                try:
                    data = v.encode('utf8')
                except UnicodeError as error:
                    raise ValueError('BINARY_UTF8') from error
            write((b'b' if kind is bytes else b's') + struct.pack('<Q', len(data)))
            write(data)
        elif kind in (dict, list, tuple, set, collections.deque):
            identity = id(v)
            if identity in active:
                raise ValueError('BINARY_CYCLE')
            count = len(v)
            minimum_nodes = count * 2 if kind is dict else count
            if minimum_nodes > budget.nodes:
                raise ValueError('BINARY_NODE_BUDGET')
            if count:
                budget.depth(depth + 1)
            active.add(identity)
            try:
                if kind is list and count and all(type(x) is int and 0 <= x <= 0xffffffff for x in v):
                    if count > (budget.max_bytes - len(output) - 9) // 4:
                        raise ValueError('BINARY_BYTE_BUDGET')
                    budget.consume(count)
                    write(b'a' + struct.pack('<Q', count))
                    packed = array.array('I', v)
                    if packed.itemsize != 4:
                        for x in v:
                            write(struct.pack('<I', x))
                    else:
                        if sys.byteorder != 'little':
                            packed.byteswap()
                        write(packed.tobytes())
                else:
                    tag = {dict: b'd', list: b'l', tuple: b't', set: b'e', collections.deque: b'q'}[kind]
                    write(tag + struct.pack('<Q', count))
                    if kind is collections.deque:
                        write(struct.pack('<q', -1 if v.maxlen is None else v.maxlen))
                    if kind is dict:
                        for key, item in v.items():
                            emit(key, depth + 1)
                            emit(item, depth + 1)
                    else:
                        for item in v:
                            emit(item, depth + 1)
            finally:
                active.remove(identity)
        else:
            raise TypeError('BINARY_UNSUPPORTED_TYPE')

    emit(value)
    return bytes(output)


def loads(data, expected=None, *, max_bytes=DEFAULT_MAX_BYTES, max_nodes=DEFAULT_MAX_NODES,
          max_depth=DEFAULT_MAX_DEPTH, max_int_digits=DEFAULT_MAX_INT_DIGITS):
    """Decode bounded bytes, optionally verify their SHA-256; never open a path."""
    budget = _Budget(max_bytes, max_nodes, max_depth, max_int_digits)
    if type(data) not in (bytes, bytearray, memoryview):
        raise TypeError('BINARY_BYTES_REQUIRED')
    try:
        view = memoryview(data).cast('B')
    except (TypeError, ValueError) as error:
        raise ValueError('BINARY_BYTES_LAYOUT') from error
    if len(view) > budget.max_bytes:
        raise ValueError('BINARY_BYTE_BUDGET')
    if type(data) is not bytes:
        view = memoryview(bytes(view))
    if expected is not None and type(expected) is not str:
        raise TypeError('BINARY_HASH_STRING_REQUIRED')
    if expected is not None and hashlib.sha256(view).hexdigest() != expected:
        raise ValueError('BINARY_CONTENT_HASH_MISMATCH')
    cursor = 0

    def read(count):
        nonlocal cursor
        if count < 0 or count > len(view) - cursor:
            raise ValueError('BINARY_LENGTH_OR_TRUNCATION')
        result = view[cursor:cursor + count]
        cursor += count
        return result

    if bytes(read(8)) != MAGIC:
        raise ValueError('BINARY_VERSION')

    def item(depth=0):
        budget.depth(depth)
        budget.consume()
        tag = bytes(read(1))
        if tag == b'n':
            return None
        if tag in (b'y', b'z'):
            return tag == b'y'
        if tag == b'i':
            return struct.unpack('<I', read(4))[0]
        if tag in (b'I', b'f'):
            return struct.unpack('<q' if tag == b'I' else '<d', read(8))[0]
        if tag not in (b'b', b's', b'G', b'a', b'd', b'l', b't', b'e', b'q'):
            raise ValueError('BINARY_TAG')
        count = struct.unpack('<Q', read(8))[0]
        if tag in (b'b', b's', b'G'):
            if tag == b'G' and count > budget.max_int_digits + 1:
                raise ValueError('BINARY_INTEGER_DIGITS')
            raw = bytes(read(count))
            if tag == b'b':
                return raw
            if tag == b's':
                try:
                    return raw.decode('utf8')
                except UnicodeError as error:
                    raise ValueError('BINARY_UTF8') from error
            digits = raw[1:] if raw.startswith(b'-') else raw
            if not digits or len(digits) > budget.max_int_digits or not all(48 <= b <= 57 for b in digits):
                raise ValueError('BINARY_INTEGER_DIGITS')
            try:
                return int(raw)
            except ValueError as error:
                raise ValueError('BINARY_INTEGER_DIGITS') from error
        minimum_nodes = count * 2 if tag == b'd' else count
        if minimum_nodes > budget.nodes:
            raise ValueError('BINARY_NODE_BUDGET')
        if count:
            budget.depth(depth + 1)
        if tag == b'a':
            raw = read(count * 4)
            budget.consume(count)
            return [struct.unpack_from('<I', raw, index * 4)[0] for index in range(count)]
        cap = struct.unpack('<q', read(8))[0] if tag == b'q' else None
        if cap is not None and (cap < -1 or cap > sys.maxsize or (cap >= 0 and count > cap)):
            raise ValueError('BINARY_DEQUE_MAXLEN')
        if tag == b'd':
            result = {}
            for _ in range(count):
                key = item(depth + 1)
                try:
                    if key in result:
                        raise ValueError('BINARY_DUPLICATE_KEY')
                    result[key] = item(depth + 1)
                except TypeError as error:
                    raise ValueError('BINARY_UNHASHABLE_KEY') from error
            return result
        result = [item(depth + 1) for _ in range(count)]
        if tag == b'l':
            return result
        if tag == b't':
            return tuple(result)
        if tag == b'e':
            try:
                converted = set(result)
            except TypeError as error:
                raise ValueError('BINARY_UNHASHABLE_SET') from error
            if len(converted) != count:
                raise ValueError('BINARY_DUPLICATE_SET_ITEM')
            return converted
        return collections.deque(result, maxlen=None if cap == -1 else cap)

    result = item()
    if cursor != len(view):
        raise ValueError('BINARY_TRAILING_DATA')
    return result
