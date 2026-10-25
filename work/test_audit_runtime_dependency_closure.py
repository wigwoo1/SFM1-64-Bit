"""Synthetic PE fixtures validate ordinary/delay import parsing without executing code."""
from pathlib import Path
import struct
import tempfile
import unittest

from audit_runtime_dependency_closure import inspect_pe


def fixture(bits=32, legacy_delay=False, terminated=True):
    data = bytearray(2048)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 60, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    optional = 0x98
    optional_size = 240 if bits == 64 else 224
    struct.pack_into('<HH', data, 0x84, 0x8664 if bits == 64 else 0x14C, 1)
    struct.pack_into('<H', data, 0x94, optional_size)
    struct.pack_into('<H', data, optional, 0x20B if bits == 64 else 0x10B)
    image_base = 0x140000000 if bits == 64 else 0x10000000
    struct.pack_into('<Q' if bits == 64 else '<I', data, optional + (24 if bits == 64 else 28), image_base)
    struct.pack_into('<I', data, optional + 60, 0x200)
    directory = optional + (112 if bits == 64 else 96)
    struct.pack_into('<I', data, directory - 4, 16)
    struct.pack_into('<II', data, directory + 8, 0x1000, 40 if terminated else 20)
    struct.pack_into('<II', data, directory + 13 * 8, 0x1040, 64)
    section = optional + optional_size
    struct.pack_into('<IIII', data, section + 8, 0x600, 0x1000, 0x600, 0x200)
    struct.pack_into('<I', data, 0x200 + 12, 0x1100)
    struct.pack_into('<II', data, 0x240, 0 if legacy_delay else 1,
                     0x1120 + image_base if legacy_delay else 0x1120)
    data[0x300:0x30D] = b'ordinary.dll\0'
    data[0x320:0x32C] = b'delayed.dll\0'
    return data


class ImportParserTests(unittest.TestCase):
    def parse(self, data):
        directory = Path(__file__).resolve().parents[1] / 'outputs/runtime-dependency-closure-20261024/tests'
        directory.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=directory) as temp:
            path = Path(temp) / 'fixture.dll'
            path.write_bytes(data)
            return inspect_pe(path)

    def test_x86_normal_and_rva_delay_imports(self):
        result = self.parse(fixture())
        self.assertEqual(result['architecture'], 'x86')
        self.assertEqual(result['imports'], ['ordinary.dll'])
        self.assertEqual(result['delay_imports'], ['delayed.dll'])

    def test_x64_normal_and_rva_delay_imports(self):
        result = self.parse(fixture(bits=64))
        self.assertEqual(result['architecture'], 'x64')
        self.assertEqual(result['imports'], ['ordinary.dll'])
        self.assertEqual(result['delay_imports'], ['delayed.dll'])

    def test_x86_legacy_absolute_address_delay_imports(self):
        self.assertEqual(self.parse(fixture(legacy_delay=True))['delay_imports'], ['delayed.dll'])

    def test_reject_unterminated_import_descriptor_array(self):
        with self.assertRaisesRegex(ValueError, 'terminator'):
            self.parse(fixture(terminated=False))

    def test_reject_non_pe_file(self):
        with self.assertRaisesRegex(ValueError, 'MZ'):
            self.parse(b'not a portable executable')


if __name__ == '__main__':
    unittest.main(verbosity=2)
