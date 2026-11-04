"""Build and verify the isolated x64 lightmap format descriptor.

Original SFM binaries are only read as pinned static data by the audit module;
this script never loads or executes them and never edits the installation.
"""
import hashlib
import json
import os
from pathlib import Path
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


CANARY = (0xDEADBEEF, 0xA5, 0x5A, (-32768, 32767, 17, -23),
          (0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00))
PREDICTIONS = {
    21: (12, 4, 0, (16, 8, 0, 24), (255, 255, 255, 255)),
    26: (19, 2, 0, (4, 0, -4, 8), (240, 240, 240, 240)),
    36: (25, 8, 0, (0, 16, 32, 48), (65535, 65535, 65535, 65535)),
    113: (24, 8, 3, (0, 16, 32, 48), (65535, 65535, 65535, 65535)),
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def record(descriptor, success):
    source, bpp, flags, shifts, masks = descriptor
    return (bytes((int(success),)) + struct.pack('<I', source) + bytes((bpp, flags)) +
            struct.pack('<4h4I', *shifts, *masks))


def descriptor_from_evidence(value):
    return (value['source_image_format'], value['bytes_per_pixel'], value['flags'],
            tuple(value['channel_shifts_rgba']), tuple(value['channel_masks_rgba']))


def main():
    outputs = ROOT / 'outputs'
    run = Path(tempfile.mkdtemp(prefix='native-lightmap-layout-', dir=outputs)).resolve()
    require(run.is_relative_to(outputs.resolve()), 'Verification output escaped workspace outputs')
    print('Verification directory: ' + str(run), flush=True)
    receipt = dict(status='FAILED', started_utc=datetime.now(timezone.utc).isoformat(),
                   directory=str(run), commands=[])

    def command(args, label, expected=0):
        result = subprocess.run(args, cwd=ROOT, capture_output=True, timeout=180, check=False)
        (run / (label + '.stdout.log')).write_bytes(result.stdout)
        (run / (label + '.stderr.log')).write_bytes(result.stderr)
        receipt['commands'].append(dict(command=[str(a) for a in args], exit_code=result.returncode,
                                        expected_exit_code=expected, label=label))
        require(result.returncode == expected,
                label + ' returned ' + str(result.returncode) + '; inspect ' + str(run))
        return result

    try:
        build_status = json.loads((ROOT / 'publication/stage/docs/build-status.json').read_text(encoding='utf-8'))
        protected = [ROOT / p for p in build_status['editor_source_dependencies']]
        protected += [ROOT / 'outputs/sfm64-editor-x64/sfm64_session_ui.exe',
                      AUDIT / 'verify_lightmap_page_allocator.py', pages.DLL, pages.SHADER_DLL]
        before = {str(p): sha(p) for p in protected}

        # Keep the pre-existing static contract checker unchanged and passing.
        command([sys.executable, str(AUDIT / 'verify_lightmap_page_allocator.py')], 'static-audit')

        image = pages.Image(pages.SHADER_DLL)
        require(sha(pages.SHADER_DLL) == pages.EXPECTED_SHADER_DLL_SHA256,
                'Pinned ShaderAPI input hash mismatch')
        rows = pages.collect_instructions(pages.SHADER_DISASM, pages.SHADER_RANGES)
        pages.authenticate_ranges(image, rows, pages.SHADER_RANGES)
        evidence = pages.verify_pixel_writer_formats(image, rows)
        supported = evidence['supported_formats']
        require(set(supported) == {str(k) for k in PREDICTIONS},
                'Authenticated supported-format set differs from the four reviewed formats')
        authenticated = {int(k): descriptor_from_evidence(v) for k, v in supported.items()}
        require(authenticated == PREDICTIONS,
                'Independent literal descriptor table differs from authenticated static audit')

        inputs = list(range(256)) + [256, 65535, 65536, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF]
        require(len(inputs) == 262 and len(set(inputs)) == 262, 'Input corpus is not exactly 262 IDs')
        require(sum(value in PREDICTIONS for value in inputs) == 4,
                'Expected exactly four supported and 258 unsupported inputs')
        input_file = run / 'formats-u32le.bin'
        input_file.write_bytes(struct.pack('<262I', *inputs))
        executable = run / 'test_native_lightmap_pixel_layout_x64.exe'
        require(not executable.exists(), 'Fresh isolated executable path already exists')
        build_script = ROOT / 'work/build_native_lightmap_pixel_layout_test.cmd'
        command([os.environ.get('COMSPEC', 'cmd.exe'), '/d', '/c', str(build_script), str(run)], 'build')

        raw = executable.read_bytes()
        require(raw[:2] == b'MZ', 'Output is missing an executable header')
        pe = struct.unpack_from('<I', raw, 0x3C)[0]
        require(raw[pe:pe + 4] == b'PE\0\0', 'Output is missing a PE signature')
        machine, = struct.unpack_from('<H', raw, pe + 4)
        magic, = struct.unpack_from('<H', raw, pe + 24)
        require(machine == 0x8664 and magic == 0x20B,
                'Test executable is not AMD64 PE32+')
        build_log = (run / 'build.log').read_text(encoding='utf-8', errors='replace')
        warnings = re.findall(r'warning [A-Z]+\d+:[^\r\n]*', build_log)
        require(not warnings, 'Compiler emitted warnings: ' + repr(warnings))

        expected_chunks = []
        for value in inputs:
            if value in PREDICTIONS:
                expected_chunks.append(record(PREDICTIONS[value], True))
            else:
                expected_chunks.append(record(CANARY, False))
        expected = b''.join(expected_chunks)
        outputs_seen = []
        for label in ('owned-first', 'owned-repeat'):
            output_file = run / (label + '-records.bin')
            command([str(executable), str(input_file), str(output_file)], label)
            actual = output_file.read_bytes()
            require(len(actual) == 262 * 31,
                    label + ' emitted a short or extra descriptor record')
            require(actual == expected,
                    label + ' output differs from authenticated formats/canary expectations')
            outputs_seen.append(actual)
        require(outputs_seen[0] == outputs_seen[1], 'Repeated executable runs were not identical')

        empty_input = run / 'empty.bin'
        empty_input.write_bytes(b'')
        empty_output = run / 'empty-records.bin'
        command([str(executable), str(empty_input), str(empty_output)], 'empty-input')
        require(empty_output.read_bytes() == b'', 'Empty input should produce no output records')
        partial_input = run / 'partial-u32.bin'
        partial_input.write_bytes(b'\x15\x00\x00')
        command([str(executable), str(partial_input), str(run / 'partial-records.bin')],
                'partial-input', 6)

        after = {str(p): sha(p) for p in protected}
        require(before == after, 'Protected editor/reference dependency hash changed')
        receipt.update(status='VERIFIED', completed_utc=datetime.now(timezone.utc).isoformat(),
                       executable=str(executable), executable_sha256=sha(executable),
                       architecture=dict(machine=hex(machine), optional_header_magic=hex(magic)),
                       compiler_warnings=warnings, input_count=len(inputs), supported_count=4,
                       rejected_count=258, record_size_bytes=31, repeated_runs=2,
                       input_sha256=sha(input_file), output_sha256=hashlib.sha256(outputs_seen[0]).hexdigest(),
                       reference_shaderapi_sha256=sha(pages.SHADER_DLL),
                       reference_metadata_cases=evidence['metadata_cases'],
                       protected_files_unchanged=True, protected_sha256=after,
                       limits=['Owned metadata lookup only; no pixel writes or editor integration',
                               'No original DLL loaded or executed',
                               'No device fallback, GPU sampling or rendered parity claim'])
    finally:
        (run / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in receipt.items()
                      if k not in ('commands', 'protected_sha256')}, indent=2))


if __name__ == '__main__':
    main()
