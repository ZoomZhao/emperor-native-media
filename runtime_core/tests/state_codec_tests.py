"""Standard unittest cases for self-authored in-memory TLV data only."""
import collections
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import state_codec as codec


_original_dumps = codec.dumps
_original_loads = codec.loads

def _record(kind, payload, settings):
    configured = os.environ.get('SELF_FIXTURE_DIR')
    if not configured:
        return
    directory = Path(configured)
    if directory != Path.cwd() / 'fixtures':
        raise RuntimeError('unexpected fixture output root')
    directory.mkdir(exist_ok=True)
    sequence = _record.sequence
    _record.sequence += 1
    name = str(sequence) + '-' + kind + '.bin'
    (directory / name).write_bytes(payload)
    with (Path.cwd() / 'inputs.jsonl').open('a') as trace:
        trace.write(json.dumps({'kind': kind, 'file': 'fixtures/' + name,
                                'bytes': len(payload), 'settings': settings}, sort_keys=True) + '\n')

_record.sequence = 0

def _observed_dumps(value, **settings):
    representation = repr(value).encode('utf8', errors='backslashreplace')
    metadata = dict(settings)
    metadata['value_type'] = type(value).__name__
    _record('tlv-value-repr', representation, metadata)
    return _original_dumps(value, **settings)

def _observed_loads(data, expected=None, **settings):
    if type(data) in (bytes, bytearray, memoryview):
        payload = bytes(data)
    else:
        payload = str(type(data).__name__).encode('ascii')
    metadata = dict(settings)
    metadata['expected_sha256'] = expected if type(expected) is str else type(expected).__name__
    _record('tlv-wire', payload, metadata)
    return _original_loads(data, expected, **settings)

codec.dumps = _observed_dumps
codec.loads = _observed_loads

MAGIC = b'S83H\x02\0\0\0'


def sized(tag, count, body=b''):
    return tag + struct.pack('<Q', count) + body


class TLVBoundaries(unittest.TestCase):
    def test_independent_wire_anchors(self):
        self.assertEqual(codec.dumps(None), MAGIC + b'n')
        self.assertEqual(codec.dumps(True), MAGIC + b'y')
        self.assertEqual(codec.dumps(False), MAGIC + b'z')
        self.assertEqual(codec.dumps(0xffffffff), MAGIC + b'i\xff\xff\xff\xff')
        self.assertEqual(codec.dumps(-1), MAGIC + b'I' + b'\xff' * 8)
        self.assertEqual(codec.dumps(b'AB'), MAGIC + b'b\x02\0\0\0\0\0\0\0AB')
        self.assertEqual(codec.dumps([0, 0xffffffff]), MAGIC + sized(b'a', 2, b'\0' * 4 + b'\xff' * 4))

    def test_builtin_roundtrip_types(self):
        values = [None, True, False, 0, 0xffffffff, 0x100000000, -0x8000000000000000,
                  0x7fffffffffffffff, 10 ** 80, -(10 ** 80), 1.25, b'\0\xff', '自造文本',
                  [], [0, 1, 0xffffffff], [None, 'x'], (), (1, 'x'), set(), {1, 2},
                  {}, {'bytes': b'\x03', 'items': [None, True, 3]},
                  collections.deque([None, 1], maxlen=3), collections.deque([], maxlen=0)]
        for value in values:
            with self.subTest(type=type(value).__name__, value=value):
                decoded = codec.loads(codec.dumps(value))
                self.assertEqual(decoded, value)
                self.assertIs(type(decoded), type(value))
                if isinstance(value, collections.deque):
                    self.assertEqual(decoded.maxlen, value.maxlen)
        self.assertTrue(math.isnan(codec.loads(codec.dumps(float('nan')))))
        self.assertEqual(codec.loads(codec.dumps(float('-inf'))), float('-inf'))

    def test_all_truncation_points_and_trailing_bytes(self):
        raw = codec.dumps({'a': [None, 'xyz'], 'b': b'\0\x7f'})
        for end in range(len(raw)):
            with self.subTest(end=end), self.assertRaises(ValueError):
                codec.loads(raw[:end])
        with self.assertRaisesRegex(ValueError, 'BINARY_TRAILING_DATA'):
            codec.loads(raw + b'n')
        with self.assertRaisesRegex(ValueError, 'BINARY_VERSION'):
            codec.loads(b'BADH\x02\0\0\0n')
        with self.assertRaisesRegex(ValueError, 'BINARY_TAG'):
            codec.loads(MAGIC + b'?')

    def test_huge_lengths_and_counts(self):
        for tag in (b'b', b's', b'G', b'a', b'd', b'l', b't', b'e', b'q'):
            with self.subTest(tag=tag), self.assertRaises(ValueError):
                codec.loads(MAGIC + sized(tag, 0xffffffffffffffff))
        with self.assertRaisesRegex(ValueError, 'BINARY_BYTE_BUDGET'):
            codec.dumps(b'x' * 24, max_bytes=32)
        with self.assertRaisesRegex(ValueError, 'BINARY_BYTE_BUDGET'):
            codec.loads(MAGIC + b'n' * 25, max_bytes=32)
        with self.assertRaisesRegex(ValueError, 'BINARY_BYTE_BUDGET'):
            codec.dumps('界' * 8, max_bytes=32)

    def test_node_and_depth_budgets_include_packed_elements(self):
        data = [1, 2, 3]
        with self.assertRaisesRegex(ValueError, 'BINARY_NODE_BUDGET'):
            codec.dumps(data, max_nodes=3)
        raw = codec.dumps(data, max_nodes=4)
        with self.assertRaisesRegex(ValueError, 'BINARY_NODE_BUDGET'):
            codec.loads(raw, max_nodes=3)
        self.assertEqual(codec.loads(raw, max_nodes=4), data)
        with self.assertRaisesRegex(ValueError, 'BINARY_DEPTH'):
            codec.dumps(data, max_depth=0)
        with self.assertRaisesRegex(ValueError, 'BINARY_DEPTH'):
            codec.loads(raw, max_depth=0)
        nested = [[None]]
        with self.assertRaisesRegex(ValueError, 'BINARY_DEPTH'):
            codec.dumps(nested, max_depth=1)
        with self.assertRaisesRegex(ValueError, 'BINARY_DEPTH'):
            codec.loads(codec.dumps(nested), max_depth=1)
        self.assertEqual(codec.loads(codec.dumps([[]], max_depth=1), max_depth=1), [[]])

    def test_cycles_rejected_shared_values_copy_as_data(self):
        loop = []; loop.append(loop)
        with self.assertRaisesRegex(ValueError, 'BINARY_CYCLE'):
            codec.dumps(loop)
        mapping = {}; mapping['self'] = mapping
        with self.assertRaisesRegex(ValueError, 'BINARY_CYCLE'):
            codec.dumps(mapping)
        shared = [1, 2]
        decoded = codec.loads(codec.dumps([shared, shared]))
        self.assertEqual(decoded, [shared, shared])
        self.assertIsNot(decoded[0], decoded[1])

    def test_duplicate_and_unhashable_container_entries(self):
        key = sized(b's', 1, b'x')
        duplicate = MAGIC + sized(b'd', 2, key + b'n' + key + b'n')
        with self.assertRaisesRegex(ValueError, 'BINARY_DUPLICATE_KEY'):
            codec.loads(duplicate)
        equality_collision = MAGIC + sized(b'd', 2, b'y' + b'n' + b'i\x01\0\0\0' + b'n')
        with self.assertRaisesRegex(ValueError, 'BINARY_DUPLICATE_KEY'):
            codec.loads(equality_collision)
        unhashable = MAGIC + sized(b'd', 1, sized(b'l', 0) + b'n')
        with self.assertRaisesRegex(ValueError, 'BINARY_UNHASHABLE_KEY'):
            codec.loads(unhashable)
        with self.assertRaisesRegex(ValueError, 'BINARY_DUPLICATE_SET_ITEM'):
            codec.loads(MAGIC + sized(b'e', 2, b'nn'))
        with self.assertRaisesRegex(ValueError, 'BINARY_UNHASHABLE_SET'):
            codec.loads(MAGIC + sized(b'e', 1, sized(b'l', 0)))

    def test_deque_capacity_never_silently_discards_data(self):
        for count, cap, body in [(2, 1, b'nn'), (0, -2, b'')]:
            with self.subTest(cap=cap), self.assertRaisesRegex(ValueError, 'BINARY_DEQUE_MAXLEN'):
                codec.loads(MAGIC + sized(b'q', count, struct.pack('<q', cap) + body))
        decoded = codec.loads(MAGIC + sized(b'q', 0, struct.pack('<q', 0)))
        self.assertEqual(list(decoded), [])
        self.assertEqual(decoded.maxlen, 0)

    def test_utf8_and_integer_digits(self):
        with self.assertRaisesRegex(ValueError, 'BINARY_UTF8'):
            codec.loads(MAGIC + sized(b's', 1, b'\xff'))
        with self.assertRaisesRegex(ValueError, 'BINARY_UTF8'):
            codec.dumps('\ud800')
        for raw in (b'', b'-', b'+1', b'1x', b'1000'):
            with self.subTest(raw=raw), self.assertRaisesRegex(ValueError, 'BINARY_INTEGER_DIGITS'):
                codec.loads(MAGIC + sized(b'G', len(raw), raw), max_int_digits=3)
        with self.assertRaisesRegex(ValueError, 'BINARY_INTEGER_DIGITS'):
            codec.dumps(10 ** 40, max_int_digits=3)

    def test_digest_and_bytes_only_interface(self):
        raw = codec.dumps({'self': b'abc'})
        self.assertEqual(codec.loads(raw, hashlib.sha256(raw).hexdigest()), {'self': b'abc'})
        with self.assertRaisesRegex(ValueError, 'BINARY_CONTENT_HASH_MISMATCH'):
            codec.loads(raw, '0' * 64)
        with self.assertRaisesRegex(TypeError, 'BINARY_HASH_STRING_REQUIRED'):
            codec.loads(raw, object())
        self.assertEqual(codec.loads(bytearray(raw)), {'self': b'abc'})
        self.assertEqual(codec.loads(memoryview(raw)), {'self': b'abc'})
        with self.assertRaisesRegex(ValueError, 'BINARY_BYTES_LAYOUT'):
            codec.loads(memoryview(raw)[::2])
        with self.assertRaisesRegex(TypeError, 'BINARY_BYTES_REQUIRED'):
            codec.loads('/not/a/file/interface')

    def test_unsupported_hooks_and_invalid_limits(self):
        class Hook(dict):
            def items(self):
                raise AssertionError('custom hooks must not be called')
        with self.assertRaisesRegex(TypeError, 'BINARY_UNSUPPORTED_TYPE'):
            codec.dumps(Hook())
        for settings in ({'max_bytes': 7}, {'max_bytes': 65 * 1024 * 1024},
                         {'max_nodes': 0}, {'max_depth': 257}, {'max_int_digits': 4097},
                         {'max_nodes': True}):
            with self.subTest(settings=settings), self.assertRaisesRegex(ValueError, 'BINARY_INVALID_LIMITS'):
                codec.dumps(None, **settings)
            with self.subTest(settings=settings), self.assertRaisesRegex(ValueError, 'BINARY_INVALID_LIMITS'):
                codec.loads(MAGIC + b'n', **settings)


if __name__ == '__main__':
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(TLVBoundaries)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    print(json.dumps({'passed': result.wasSuccessful(), 'tests_run': result.testsRun,
                      'failures': len(result.failures), 'errors': len(result.errors),
                      'fixtures': 'self-authored-memory', 'file_input': False,
                      'legacy_imported': False}))
    raise SystemExit(0 if result.wasSuccessful() else 1)
