"""Compare owned headless Vulkan graphics with retained, pinned D3D9 GPU data.

Local private fixtures only. No original files, SFM process, computer control,
full editor, material shader or active original variant is exercised here.
"""
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import math
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/phong-vertex-vulkan-20261025'
REFERENCE = ROOT / 'outputs/phong-vertex-gpu-20261025/verification-bbs4a0j7'
VARIANTS = ROOT / 'outputs/phong-vertex-variants-20261025/verification-9wa6792k'
# Identical predeclared bounds to the D3D9 fixture. Never widen after failure.
POSITION_ABS_TOLERANCE = 0.005
DIRECTION_ABS_TOLERANCE = 0.00002
PINS = {
    REFERENCE / 'results.json': 'd5bbde610816c43f623b251378321a98243885ad32817bfee6d1b4c830a9013c',
    REFERENCE / 'gpu-queries.bin': '47f2b1e51749d3ef05b007d4a716568d503900f9e93406e18843a076a88a3d74',
    REFERENCE / 'gpu/gpu-world-frames.bin': '0563e4bc7a58faf7879bd6399ef90dd3283dcc3b004157217512ff224930755e',
    VARIANTS / 'results.json': '2147dc585653ea41573146c8d5804797eed2d31799d737b0d0f244e9be21e031',
    VARIANTS / 'catalog.json': '7560bc957beb389f166e603da652c6a802a2d3584e30862d414c1b91a502bba8',
}
EDITOR = ROOT / 'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe'
EDITOR_PIN = '2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def key_values(path):
    return dict(line.split('=', 1) for line in Path(path).read_text().splitlines())


def inspect_spirv(path, entry, model):
    """Bounded envelope/layout checks, NOT a substitute for spirv-val."""
    blob = path.read_bytes()
    require(len(blob) >= 20 and len(blob) % 4 == 0, 'SPIR-V size')
    words = struct.unpack('<' + 'I' * (len(blob) // 4), blob)
    require(words[0] == 0x07230203 and words[1] == 0x10000 and words[4] == 0,
            'SPIR-V 1.0 envelope')
    instructions = []
    offset = 5
    while offset < len(words):
        count, opcode = words[offset] >> 16, words[offset] & 65535
        require(count > 0 and offset + count <= len(words), 'SPIR-V instruction bounds')
        instructions.append((opcode, words[offset+1:offset+count]))
        offset += count
    entries = [(args[0], struct.pack('<' + 'I' * len(args[2:]), *args[2:]).split(b'\0')[0].decode())
               for opcode, args in instructions if opcode == 15]
    require(entries == [(model, entry)], 'SPIR-V entry identity')
    decorations = [args for op, args in instructions if op == 71]
    no_contraction = sum(args[1] == 42 for args in decorations)
    require(not any(op == 12 and len(args) > 3 and args[3] == 50 for op, args in instructions),
            'Unexpected fused multiply-add extension instruction')
    offsets = [args for op, args in instructions if op == 72 and len(args) == 4 and args[2] == 35]
    if model == 0:
        require(no_contraction > 20, 'Strict arithmetic decorations missing')
        require(any(a[1:] == (6, 16) for a in decorations), 'Palette row ArrayStride')
        structures = {a[0] for a in offsets if a[1:] == (0, 35, 0)}
        require(any(a[0] in structures and a[1:] == (1, 35, 2544) for a in offsets),
                'Uniform member offsets must be 0 and 2544')
        require(any(a[1:] == (11, 1) for a in decorations), 'PointSize builtin missing')
    return dict(path=str(path), sha256=sha(path), instructions=len(instructions),
                no_contraction_decorations=no_contraction, entry=entry,
                validation='bounded envelope/entry/decorations/layout checks; not spirv-val')


def compare(rows, metadata, actual, oracle, cpu=False):
    names = ['position', 'normal', 'tangent', 'binormal']
    groups = {name: dict(components=0, exact_components=0, max_abs_error=0.0,
                         worst_case=None) for name in names}
    violations = []
    analytical_count = analytical_exact = 0
    for i in range(len(metadata)):
        gpu = struct.unpack_from('<16f', actual, i * 64)
        expected = (struct.unpack_from('<12f', rows, i * 288 + 16 + 224) if cpu
                    else struct.unpack_from('<16f', oracle, i * 64))
        for group, name in enumerate(names):
            require(gpu[group*4+3] == 1, 'Vulkan capture alpha/correspondence')
            if not cpu:
                require(expected[group*4+3] == 1, 'D3D9 capture alpha/correspondence')
            stats = groups[name]
            for c in range(3):
                a, e = gpu[group*4+c], expected[group*(3 if cpu else 4)+c]
                require(math.isfinite(a) and math.isfinite(e), 'Nonfinite comparison input')
                error = abs(a-e)
                stats['components'] += 1
                stats['exact_components'] += a == e
                if error > stats['max_abs_error']:
                    stats['max_abs_error'] = error
                    stats['worst_case'] = dict(case=i, component=c, expected=e,
                                              actual=a, profile=metadata[i])
                bound = POSITION_ABS_TOLERANCE if group == 0 else DIRECTION_ABS_TOLERANCE
                if error > bound:
                    violations.append(dict(case=i, group=name, component=c, error=error, bound=bound))
                if metadata[i]['kind'] == 'analytical' and group == 0:
                    analytical_count += 1
                    analytical_exact += a == e
    return dict(status='PASS' if not violations and analytical_exact == analytical_count else 'FAIL',
                position_abs_tolerance=POSITION_ABS_TOLERANCE,
                direction_abs_tolerance=DIRECTION_ABS_TOLERANCE, groups=groups,
                violations=violations, analytical_position_components=analytical_count,
                analytical_position_exact=analytical_exact)


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    try:
        for path, pin in PINS.items():
            require(sha(path) == pin, 'Retained reference changed: ' + str(path))
        require(sha(EDITOR) == EDITOR_PIN, 'Production editor changed')
        prior = json.loads((REFERENCE / 'results.json').read_text())
        require(prior['status'] == 'PASS', 'D3D9 oracle status')
        for name, pin in prior['sources_sha256'].items():
            require(sha(ROOT / 'work' / name) == pin, 'D3D9 reference source changed: ' + name)
        require(sha(REFERENCE / 'query-metadata.json') == prior['metadata_sha256'], 'Metadata pin')
        catalog = json.loads((VARIANTS / 'catalog.json').read_text())
        admitted = {(p['version'], p['static'], p['dynamic']) for p in catalog
                    if p['status'] == 'STREAM_SKIN_ADMITTED'}
        require(len(admitted) == 88, 'Admission count')
        admission = run / 'admitted-slots.bin'
        admission.write_bytes(b''.join(struct.pack('<3I', *p) for p in sorted(admitted)))
        rows = (REFERENCE / 'gpu-queries.bin').read_bytes()
        oracle = (REFERENCE / 'gpu/gpu-world-frames.bin').read_bytes()
        metadata = json.loads((REFERENCE / 'query-metadata.json').read_text())
        require(len(rows) == 13696 * 288 and len(oracle) == 13696 * 64 and len(metadata) == 13696,
                'Pinned input/output case extent')
        require({struct.unpack_from('<3I', rows, i*288) for i in range(len(metadata))} == admitted,
                'All 88 admitted contracts must be exercised')
        counts = dict(Counter(m['kind'] for m in metadata))
        require(counts == dict(analytical=848, native_synthetic=5632, benchmark=7216), 'Case group coverage')
        exe = OUT / 'phong_vertex_vulkan_x64.exe'
        pe_bytes = exe.read_bytes()
        pe = struct.unpack_from('<I', pe_bytes, 60)[0]
        require(pe_bytes[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', pe_bytes, pe+4)[0] == 0x8664
                and struct.unpack_from('<H', pe_bytes, pe+24)[0] == 0x20b, 'Native AMD64 PE32+ required')
        logs = ['build.log', 'plain-build.log', 'packed-build.log', 'capture-build.log']
        for name in logs:
            log = (OUT / name).read_text()
            require('warning' not in log.lower() and 'error' not in log.lower(), 'Build diagnostic: ' + name)
        spirv = [inspect_spirv(OUT / name, entry, model) for name, entry, model in [
            ('phong-plain.spv', 'plainMain', 0), ('phong-packed.spv', 'packedMain', 0),
            ('phong-capture.spv', 'captureMain', 4)]]
        # Caps only: creates/releases its own hidden window. Never attaches to SFM.
        d3d_exe = Path(prior['executable']['path'])
        require(sha(d3d_exe) == prior['executable']['sha256'], 'D3D9 caps executable pin')
        caps = subprocess.run([str(d3d_exe), '--caps', str(run / 'current-d3d9-caps')],
                              cwd=ROOT, capture_output=True, text=True, timeout=60)
        (run / 'caps.log').write_text(caps.stdout + caps.stderr)
        require(caps.returncode == 0 and 'PASS capabilities only' in caps.stdout, 'Current adapter/driver check failed')
        device = key_values(run / 'current-d3d9-caps/device.txt')
        require(all(device[k] == prior['device'][k] for k in
                    ['vendor', 'device', 'description', 'driver', 'driver_version']),
                'Current D3D9 adapter/driver differs from captured reference')
        command = [str(exe), str(OUT), str(REFERENCE / 'gpu-queries.bin'), str(admission),
                   str(run / 'gpu'), device['vendor'], device['device']]
        print('Running headless Vulkan graphics: 13696 cases, 88 admitted contracts.', flush=True)
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=300)
        (run / 'gpu.log').write_text(result.stdout + result.stderr)
        print(result.stdout, flush=True)
        require(result.returncode == 0, 'Vulkan fixture failed: ' + result.stderr)
        require('PASS vulkan_vertex_cases=13696 admitted_slots_used=88 float_targets=4 exact_point_pixels=1' in result.stdout
                and 'PASS resources released' in result.stdout, 'Vulkan coverage/cleanup')
        gpu_path = run / 'gpu/vulkan-world-frames.bin'
        actual = gpu_path.read_bytes()
        require(len(actual) == len(oracle), 'Vulkan output extent')
        comparisons = dict(vulkan_vs_d3d9=compare(rows, metadata, actual, oracle),
                           vulkan_vs_cpu=compare(rows, metadata, actual, oracle, cpu=True))
        (run / 'comparison.json').write_text(json.dumps(comparisons, indent=2) + '\n')
        require(all(c['status'] == 'PASS' for c in comparisons.values()),
                'GPU comparison failed; inspect comparison.json; do not widen bounds')
        vk_device = key_values(run / 'gpu/device.txt')
        validation = key_values(run / 'gpu/validation-status.txt')
        require(vk_device['window_created'] == '0' and validation['errors'] == '0', 'Headless/validation error check')
        require(all(vk_device[k] == device[k] for k in ['vendor', 'device']), 'Same physical adapter required')
        # Fail-closed admission/input tests use fresh disposable copies only.
        # No result may be emitted for any refused first record.
        negative_cases = []
        first = rows[:288]
        morph = bytearray(first)
        struct.pack_into('<3I', morph, 0, 30, 4, 6)
        profile = bytearray(first)
        struct.pack_into('<I', profile, 16, 99)
        palette = bytearray(first)
        palette[12] = 53
        bad_admission = run / 'duplicate-admission.bin'
        bad_admission.write_bytes(admission.read_bytes() + admission.read_bytes()[:12])
        for label, data, manifest, error in [
            ('texture-morph', morph, admission, 'unadmitted Vulkan shader contract'),
            ('profile-mismatch', profile, admission, 'Vulkan query profile'),
            ('palette-out-of-bounds', palette, admission, 'Vulkan palette input bounds'),
            ('truncated-record', first[:-1], admission, 'Vulkan query framing'),
            ('duplicate-admission', first, bad_admission, 'admission entry'),
        ]:
            negative_input = run / (label + '.bin')
            negative_input.write_bytes(data)
            negative_output = run / ('refusal-' + label)
            negative_command = [str(exe), str(OUT), str(negative_input), str(manifest),
                                str(negative_output), device['vendor'], device['device']]
            refused = subprocess.run(negative_command, cwd=ROOT, capture_output=True, text=True, timeout=60)
            (run / (label + '.log')).write_text(refused.stdout + refused.stderr)
            emitted = negative_output / 'vulkan-world-frames.bin'
            require(refused.returncode != 0 and error in refused.stderr and
                    'PASS vulkan_vertex_cases=' not in refused.stdout and
                    (not emitted.exists() or emitted.stat().st_size == 0),
                    'Input was not safely refused: ' + label)
            negative_cases.append(dict(case=label, status='PASS', error=error,
                                       returncode=refused.returncode, emitted_frames=0))
        for path, pin in PINS.items():
            require(sha(path) == pin, 'Reference mutated during verification')
        require(sha(EDITOR) == EDITOR_PIN, 'Editor mutated during verification')
        sources = ['phong_vertex_vulkan_candidate.hlsl', 'probe_phong_vertex_vulkan.cpp',
                   'build_phong_vertex_vulkan.cmd', 'verify_phong_vertex_vulkan.py']
        receipt = dict(status='PASS', utc=datetime.now(timezone.utc).isoformat(),
                       cases=len(metadata), groups=counts, admitted_contracts=88, owned_vertex_pipelines=2,
                       gpu_comparison=comparisons, reference_pins={str(p): pin for p, pin in PINS.items()},
                       reference_metadata_sha256=sha(REFERENCE / 'query-metadata.json'),
                       admission_sha256=sha(admission), gpu_output_sha256=sha(gpu_path),
                       executable=dict(path=str(exe), sha256=sha(exe), machine='AMD64', format='PE32+'),
                       sources_sha256={name: sha(ROOT / 'work' / name) for name in sources},
                       spirv=spirv, build_logs={name: sha(OUT / name) for name in logs},
                       dxc_sha256=sha(ROOT / 'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe'),
                       command=command, current_d3d9_device=device, vulkan_device=vk_device,
                       validation=validation, gpu_log=str(run / 'gpu.log'),
                       safety_refusals=negative_cases,
                       production_editor=dict(path=str(EDITOR), sha256=EDITOR_PIN, rebuilt=False, run=False, unchanged=True),
                       computer_use=False, sfm_process_used=False, original_files_accessed=False,
                       limitations='Only bounded world P/N/T/B with controlled point projection and capture-only fragment shader. '
                       'Two input pipelines implement the common core of 88 admitted original contracts; this is not 88 complete Vulkan shaders. '
                       'Not bitwise equality, cross-driver proof, original material initialization, active shader choice, full Phong shading, '
                       'strip drawing, camera raster parity, production integration or full benchmark rendering. '
                       'Validation-layer status is explicit; bounded SPIR-V inspection is not spirv-val. '
                       'Retained private source-derived query/capture data must not be published.')
        (run / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n')
        print(json.dumps(dict(status='PASS', receipt=str(run / 'results.json'), cases=len(metadata),
                              max_error={label: {g: s['max_abs_error'] for g, s in c['groups'].items()}
                                         for label, c in comparisons.items()}, validation=validation)), flush=True)
    except Exception as exc:
        (run / 'failure.json').write_text(json.dumps(dict(status='FAIL', error=str(exc),
                                                         utc=datetime.now(timezone.utc).isoformat()), indent=2) + '\n')
        raise


if __name__ == '__main__':
    main()
