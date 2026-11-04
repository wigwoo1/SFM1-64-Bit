"""Fresh owned x64 value test versus authenticated static replay and formula.

Requires the local read-only audit/disassembly fixtures. Never loads or executes
original DLLs. No editor integration, GPU parity, or FPU exception-status claim.
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
import verify_half_conversion_contract as reference
import verify_lightmap_page_allocator as pages


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    outputs = ROOT / 'outputs'
    run = Path(tempfile.mkdtemp(prefix='native-lightmap-half-', dir=outputs)).resolve()
    require(run.is_relative_to(outputs.resolve()), 'Tests must stay in workspace outputs')
    print('Verification directory: ' + str(run), flush=True)
    receipt = dict(status='FAILED', started_utc=datetime.now(timezone.utc).isoformat(),
                   directory=str(run), commands=[])

    def command(args, label, expected=0):
        result = subprocess.run(args, cwd=ROOT, capture_output=True, timeout=180, check=False)
        (run / (label + '.stdout.log')).write_bytes(result.stdout)
        (run / (label + '.stderr.log')).write_bytes(result.stderr)
        receipt['commands'].append(dict(command=[str(a) for a in args], exit_code=result.returncode,
                                        expected_exit_code=expected, label=label))
        require(result.returncode == expected, label + ' failed; inspect logs in ' + str(run))
        return result

    try:
        build_status = json.loads((ROOT / 'publication/stage/docs/build-status.json').read_text())
        protected = [ROOT / p for p in build_status['editor_source_dependencies']]
        protected += [ROOT / 'outputs/sfm64-editor-x64/sfm64_session_ui.exe',
                      AUDIT / 'verify_half_conversion_contract.py',
                      AUDIT / 'verify_lightmap_page_allocator.py', pages.DLL, pages.SHADER_DLL]
        before = {str(p): sha(p) for p in protected}
        for script in ('verify_half_conversion_contract.py', 'verify_lightmap_page_allocator.py'):
            command([sys.executable, str(AUDIT / script)], script.removesuffix('.py'))

        image = reference.Image(reference.DLL)
        require(sha(reference.DLL) == reference.EXPECTED_DLL_SHA256, 'Installed DLL hash differs')
        instructions = reference.collect_instructions(reference.DISASM, reference.RANGE)
        evidence = reference.authenticate_ranges(image, instructions, reference.RANGE)
        require(image.word(0x100993D8) == 0x477FE000 and image.word(0x100993E0) == 0xC77FE000,
                'Half saturation constants changed')
        rows = {row['address']: row for row in instructions}

        # Explicit examples supplement the unchanged 9,216-case shared corpus.
        examples = {
            'positive_zero': (0x00000000, 0x0000), 'negative_zero': (0x80000000, 0x8000),
            'positive_infinity': (0x7f800000, 0x7bff), 'negative_infinity': (0xff800000, 0xfbff),
            'positive_quiet_nan': (0x7fc00000, 0x0000), 'negative_quiet_nan': (0xffc00000, 0x8000),
            'positive_signaling_nan': (0x7f800001, 0x0000), 'negative_signaling_nan': (0xff800001, 0x8000),
            'maximum_finite_half': (0x477fe000, 0x7bff), 'above_maximum_half': (0x477fe001, 0x7bff),
            'below_maximum_half': (0x477fdfff, 0x7bfe), 'negative_saturation': (0xc77fe001, 0xfbff),
            'minimum_half_subnormal': (0x33800000, 0x0001), 'below_minimum_subnormal': (0x337fffff, 0x0000),
            'negative_minimum_subnormal': (0xb3800000, 0x8001),
            'negative_below_minimum_subnormal': (0xb37fffff, 0x8000),
            'minimum_half_normal': (0x38800000, 0x0400), 'below_minimum_half_normal': (0x387fffff, 0x03ff),
            'truncation_below_next_half': (0x3f801fff, 0x3c00), 'next_half': (0x3f802000, 0x3c01),
        }
        corpus = reference.make_cases()
        require(len(corpus) == 9216, 'Shared corpus changed unexpectedly')
        cases = sorted(set(corpus) | {u for u, _ in examples.values()})
        replay_values = [reference.replay(u, rows, image) for u in cases]
        formula_values = [reference.reduced_contract(u) for u in cases]
        require(replay_values == formula_values, 'Static replay and formula disagree')
        expected_bytes = struct.pack('<' + 'H' * len(cases), *replay_values)
        input_file = run / 'input-u32le.bin'
        input_file.write_bytes(struct.pack('<' + 'I' * len(cases), *cases))
        (run / 'expected-u16le.bin').write_bytes(expected_bytes)

        executable = run / 'test_native_lightmap_half_x64.exe'
        require(not executable.exists(), 'Fresh build required')
        script = ROOT / 'work/build_native_lightmap_half_test.cmd'
        command([os.environ.get('COMSPEC', 'cmd.exe'), '/d', '/c', str(script), str(run)], 'build')
        raw = executable.read_bytes()
        require(raw[:2] == b'MZ', 'Missing executable header')
        pe = struct.unpack_from('<I', raw, 0x3c)[0]
        require(raw[pe:pe+4] == b'PE\0\0', 'Missing PE signature')
        machine = struct.unpack_from('<H', raw, pe+4)[0]
        magic = struct.unpack_from('<H', raw, pe+24)[0]
        require(machine == 0x8664 and magic == 0x20b, 'Executable must be AMD64 PE32+')
        build_log = (run / 'build.log').read_text(errors='replace')
        warnings = re.findall(r'warning [A-Z]+\d+:[^\r\n]*', build_log)

        for label in ('owned-first', 'owned-repeat'):
            output = run / (label + '-u16le.bin')
            command([str(executable), str(input_file), str(output)], label)
            actual = output.read_bytes()
            require(len(actual) == len(cases) * 2, 'Short or extra output: ' + label)
            require(actual == expected_bytes, 'Owned output differs from both references: ' + label)
        observed = dict(zip(cases, struct.unpack('<' + 'H' * len(cases), actual)))
        for label, (u, expected) in examples.items():
            require(observed[u] == expected, 'Explicit example failed: ' + label)

        empty = run / 'empty-input.bin'
        empty.write_bytes(b'')
        empty_output = run / 'empty-output.bin'
        command([str(executable), str(empty), str(empty_output)], 'empty-input')
        require(empty_output.read_bytes() == b'', 'Empty input must produce empty output')
        malformed = run / 'partial-u32-input.bin'
        malformed.write_bytes(b'\x00\x00\x80')
        command([str(executable), str(malformed), str(run / 'partial-output.bin')], 'partial-input', 6)

        after = {str(p): sha(p) for p in protected}
        require(before == after, 'Protected editor/reference files changed')
        receipt.update(status='VERIFIED', completed_utc=datetime.now(timezone.utc).isoformat(),
                       executable=str(executable), executable_sha256=sha(executable),
                       architecture=dict(machine=hex(machine), optional_header_magic=hex(magic)),
                       compiler_warnings=warnings, shared_corpus_cases=len(corpus), total_cases=len(cases),
                       repeated_runs=2, input_sha256=sha(input_file), output_sha256=hashlib.sha256(actual).hexdigest(),
                       reference_dll_sha256=sha(reference.DLL),
                       reference_range_sha256=evidence[0]['authenticated_bytes_sha256'],
                       protected_files_unchanged=True, protected_sha256=after,
                       examples={k:dict(input_bits=hex(u), half_bits=hex(v)) for k,(u,v) in examples.items()},
                       limits=['Static instruction replay and independent formula; no original DLL execution',
                               'No FPU exception/trap/status parity', 'No renderer integration or visual parity'])
    finally:
        (run / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('commands','protected_sha256','examples')}, indent=2))


if __name__ == '__main__':
    main()
