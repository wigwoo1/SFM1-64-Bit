"""Build and verify the isolated owned HDR2 RGBA pixel encoder."""
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parent.parent
AUDIT = ROOT / 'outputs/native-hdr-selector-20261102'
sys.path.insert(0, str(AUDIT))
import verify_lightmap_page_allocator as pages
import verify_half_conversion_contract as half
from verify_hdr2_single_pack import replay

FORMATS = (36, 113)
CANARY = bytes([0xA5] * 8)
RANDOM_SEED = 0x48445232


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def literal_bytes(fmt, bits, converted_half=None):
    words = []
    for i, value in enumerate(bits):
        words.append((value & 0xffff) if fmt == 36 else converted_half[i])
    return struct.pack('<4H', *words)


def main():
    outputs = ROOT / 'outputs'
    run = Path(tempfile.mkdtemp(prefix='native-lightmap-hdr2-pixel-', dir=outputs)).resolve()
    require(run.is_relative_to(outputs.resolve()), 'Output escaped workspace outputs')
    print('Verification directory: ' + str(run), flush=True)
    receipt = dict(status='FAILED', started_utc=datetime.now(timezone.utc).isoformat(),
                   directory=str(run), commands=[])

    def command(args, label, expected=0):
        result = subprocess.run(args, cwd=ROOT, capture_output=True, timeout=240, check=False)
        (run / (label + '.stdout.log')).write_bytes(result.stdout)
        (run / (label + '.stderr.log')).write_bytes(result.stderr)
        receipt['commands'].append(dict(command=[str(x) for x in args], label=label,
                                        exit_code=result.returncode, expected_exit_code=expected))
        require(result.returncode == expected,
                label + ' exit ' + str(result.returncode) + ', expected ' + str(expected))
        return result

    try:
        build_status = json.loads((ROOT / 'publication/stage/docs/build-status.json').read_text(encoding='utf-8'))
        protected = [ROOT / name for name in build_status['editor_source_dependencies']]
        protected += [ROOT / 'outputs/sfm64-editor-x64/sfm64_session_ui.exe', pages.DLL, pages.SHADER_DLL,
                      ROOT / 'work/native_lightmap_half_candidate.h',
                      ROOT / 'work/native_lightmap_pixel_layout_candidate.h']
        protected_before = {str(path): sha(path) for path in protected}

        command([sys.executable, str(AUDIT / 'verify_hdr2_bump_pack.py')], 'hdr2-bump-prerequisite')

        image = pages.Image(pages.DLL)
        shader = pages.Image(pages.SHADER_DLL)
        require(sha(pages.DLL) == pages.EXPECTED_DLL_SHA256, 'Pinned MaterialSystem hash mismatch')
        require(sha(pages.SHADER_DLL) == pages.EXPECTED_SHADER_DLL_SHA256, 'Pinned ShaderAPI hash mismatch')
        rows_list = pages.collect_instructions(pages.DISASM, pages.RANGES)
        ranges = pages.authenticate_ranges(image, rows_list, pages.RANGES)
        rows = {row['address']: row for row in rows_list}
        shader_rows = pages.collect_instructions(pages.SHADER_DISASM, pages.SHADER_RANGES)
        pages.authenticate_ranges(shader, shader_rows, pages.SHADER_RANGES)
        formats = pages.verify_pixel_writer_formats(shader, shader_rows)['supported_formats']
        require(set(formats) == {'21', '26', '36', '113'}, 'Authenticated format set changed')
        require(formats['36']['bytes_per_pixel'] == 8 and formats['36']['flags'] == 0,
                'Format 36 metadata no longer matches reviewed raw-word path')
        require(formats['113']['bytes_per_pixel'] == 8 and formats['113']['flags'] == 3,
                'Format 113 metadata no longer selects reviewed half path')
        require(rows[0x1001F050]['instruction'] == 'test        byte ptr [esi+4Bh],2',
                'HDR2 single half-branch selector changed')
        require(rows[0x1001F076]['instruction'] == 'je          1001F169',
                'HDR2 single raw-bit branch changed')

        half_rows_list = pages.collect_instructions(pages.DISASM, half.RANGE)
        half_rows = {row['address']: row for row in half_rows_list}
        bits = half.make_cases()
        require(len(bits) == 9216, 'Shared half input corpus changed')
        vectors = [tuple(bits[i:i + 4]) for i in range(0, len(bits), 4)]
        rng = random.Random(RANDOM_SEED)
        vectors += [tuple(rng.getrandbits(32) for _ in range(4)) for _ in range(256)]
        require(len(vectors) == 2560, 'Expected 2,560 RGBA vectors')
        all_bits = {value for vector in vectors for value in vector}
        converted = {value: half.replay(value, half_rows, image) for value in all_bits}
        require(all(converted[value] == half.reduced_contract(value) for value in all_bits),
                'Authenticated half replay disagrees with independent reduced formula')

        supported_records = []
        supported_expectations = []
        replay_case_count = 0
        for fmt in FORMATS:
            layout = formats[str(fmt)]
            for vector in vectors:
                half_words = tuple(converted[value] for value in vector)
                expected = literal_bytes(fmt, vector, half_words)
                # The owned executable receives raw float bits for both formats.
                supported_records.append(struct.pack('<5I', fmt, *vector))
                supported_expectations.append((fmt, vector, half_words, expected))
                replay_input = vector if fmt == 36 else half_words
                for fill in (0xA5, 0x5A):
                    single = replay(rows, layout, replay_input, fill)
                    require(single == expected, 'Authenticated single block differs from literal bytes')
                    replay_case_count += 1
                    for plane in range(4):
                        bump = replay(rows, layout, replay_input, fill, bump_plane=plane)
                        require(bump == expected,
                                'Authenticated bump block differs: format=%d plane=%d' % (fmt, plane))
                        replay_case_count += 1

        accepted = {36, 113}
        rejected = [x for x in range(256) if x not in accepted]
        rejected += [256, 65535, 65536, 0x7fffffff, 0x80000000, 0xffffffff]
        require(len(rejected) == 260 and len(set(rejected)) == 260,
                'Rejected-format corpus is not exactly 260 unique IDs')
        rejected_vectors = [(0x00000000, 0x7f800000, 0x7fc00000, 0x80000000)] * len(rejected)
        rejected_records = [struct.pack('<5I', fmt, *vector)
                           for fmt, vector in zip(rejected, rejected_vectors)]
        inputs = supported_records + rejected_records
        input_bytes = b''.join(inputs)
        input_file = run / 'rgba-format-floatbits-u32le.bin'
        input_file.write_bytes(input_bytes)
        executable = run / 'test_native_lightmap_hdr2_pixel_x64.exe'
        require(not executable.exists(), 'Fresh executable path unexpectedly exists')
        build_script = ROOT / 'work/build_native_lightmap_hdr2_pixel_test.cmd'
        command([os.environ.get('COMSPEC', 'cmd.exe'), '/d', '/c', str(build_script), str(run)], 'build')

        exe_bytes = executable.read_bytes()
        require(exe_bytes[:2] == b'MZ', 'Executable lacks MZ header')
        pe_offset, = struct.unpack_from('<I', exe_bytes, 0x3c)
        require(exe_bytes[pe_offset:pe_offset + 4] == b'PE\0\0', 'Executable lacks PE signature')
        machine, = struct.unpack_from('<H', exe_bytes, pe_offset + 4)
        magic, = struct.unpack_from('<H', exe_bytes, pe_offset + 24)
        require(machine == 0x8664 and magic == 0x20b, 'Executable is not AMD64 PE32+')
        build_log = (run / 'build.log').read_text(encoding='utf-8', errors='replace')
        warnings = re.findall(r'warning [A-Z]+\d+:[^\r\n]*', build_log)
        require(not warnings, 'Compiler warnings present: ' + repr(warnings))
        object_file = run / 'test_native_lightmap_hdr2_pixel.obj'
        cl_command = ('cl /nologo /std:c++17 /EHsc /W4 /O2 '
                      'work\\test_native_lightmap_hdr2_pixel.cpp '
                      '/Fo:"' + str(object_file) + '" /Fe:"' + str(executable) + '" '
                      '/link /MACHINE:X64 /MANIFEST:EMBED '
                      '/MANIFESTUAC:"level=\'asInvoker\' uiAccess=\'false\'"')

        expected = b''.join(bytes([1]) + data for _, _, _, data in supported_expectations)
        expected += b''.join(bytes([0]) + CANARY for _ in rejected)
        first_outputs = []
        for label in ('owned-first', 'owned-repeat'):
            output_file = run / (label + '-records.bin')
            command([str(executable), str(input_file), str(output_file)], label)
            actual = output_file.read_bytes()
            require(len(actual) == (5120 + 260) * 9, 'Output length is not exact 9-byte records')
            require(actual == expected, label + ' output differs from literal predictions/canaries')
            first_outputs.append(actual)
        require(first_outputs[0] == first_outputs[1], 'Repeated output bytes differ')

        empty = run / 'empty.bin'
        empty.write_bytes(b'')
        empty_out = run / 'empty-out.bin'
        command([str(executable), str(empty), str(empty_out)], 'empty-input')
        require(empty_out.read_bytes() == b'', 'Empty input should produce empty output')
        partial = run / 'partial.bin'
        partial.write_bytes(b'\x24\x00\x00\x00\x00')
        command([str(executable), str(partial), str(run / 'partial-out.bin')], 'partial-input', 6)

        protected_after = {str(path): sha(path) for path in protected}
        require(protected_before == protected_after, 'Protected editor/reference/helper file changed')
        receipt.update(status='VERIFIED ISOLATED OWNED HDR2 PIXEL ENCODER',
                       completed_utc=datetime.now(timezone.utc).isoformat(),
                       executable=str(executable), executable_sha256=sha(executable),
                       architecture={'machine': hex(machine), 'optional_header_magic': hex(magic)},
                       exact_build_command=['call "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat"',
                                            cl_command],
                       compiler_warnings=warnings, vectors=len(vectors), supported_formats=list(FORMATS),
                       owned_supported_cases=5120, rejected_ids=len(rejected),
                       rejected_cases_per_run=len(rejected), reference_replay_cases=replay_case_count,
                       record_bytes=20, output_record_bytes=9, repeated_runs=2,
                       prerequisite_exit_code=receipt['commands'][0]['exit_code'],
                       input_sha256=sha(input_file), output_sha256=hashlib.sha256(first_outputs[0]).hexdigest(),
                       half_reference='Pinned instruction replay compared against independent reduced_contract',
                       authenticated_shaderapi_sha256=sha(pages.SHADER_DLL),
                       protected_files_unchanged=True, protected_file_count=len(protected_after),
                       protected_sha256=protected_after,
                       limits=['Isolated pixel bytes only; no row/pitch/device/renderer integration',
                               'Format acceptance follows actual D3D IDs only; no live fallback selection',
                               'Static replay checks single and four bump packing blocks, not full strip loops',
                               'No original DLL was loaded or executed; no editor rebuild or GPU parity test'])
    finally:
        (run / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ('commands', 'protected_sha256')}, indent=2))


if __name__ == '__main__':
    main()
