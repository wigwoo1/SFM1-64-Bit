"""Private installed-method comparison for immediate resource-free numeric values.

Never starts/attaches SFM, loads its DLLs, or writes original files. The PE32
reference is a private verification process; the owned implementation is x64.
"""
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import math
import random
import re
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/material-numeric-value-20261025'
LIB = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/materialsystem.dll')
PRIVATE = ROOT / 'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
PIN = 'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8'
OPS = ['SetInt', 'SetFloat', 'SetVector2', 'SetVector3', 'SetVector4', 'SetVectorArray', 'SetUndefined']
RANGES = [
    ('constructor_vtable_store', 0x100176bb, 0x100176c1),
    ('complete_SetFloat', 0x10019350, 0x10019456),
    ('complete_SetInt', 0x10019460, 0x10019560),
    ('complete_vector_core', 0x10019960, 0x10019af9),
    ('complete_SetVectorArray', 0x10019b00, 0x10019cc7),
    ('complete_SetVector2', 0x10019cd0, 0x10019d08),
    ('complete_SetVector3', 0x10019d10, 0x10019d4d),
    ('complete_SetVector4', 0x10019d50, 0x10019d8f),
    ('complete_SetUndefined', 0x1001a0a0, 0x1001a168),
    ('resource_cleanup', 0x10017ad0, 0x10017b46),
    ('debug_log_owner_null_prefix', 0x10018460, 0x10018480),
    ('debug_log_owner_null_epilogue', 0x100187db, 0x100187ea),
    ('stack_cookie_check', 0x1007d8df, 0x1007d8ee),
    ('small_nonoverlapping_memcpy_prefix', 0x1007de80, 0x1007dea8),
]


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def f32(bits):
    return struct.unpack('<f', struct.pack('<I', bits))[0]


def trunc32(bits):
    value = f32(bits)
    return (int(value) & 0xffffffff) if math.isfinite(value) and -2147483648 <= value < 2147483648 else 0x80000000


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    require(sha(LIB) == sha(PRIVATE) == PIN, 'Installed/private identity')
    image, evidence = check_ranges(LIB, 'materialsystem-disassembly.txt', RANGES)
    table = 0x10098f34
    locator = image.word(table - 4)
    descriptor = image.word(locator + 12)
    require(image.word(locator) == 0 and image.word(locator + 4) == 0 and
            image.string(descriptor + 8) == '.?AVCMaterialVar@@', 'Vtable RTTI identity')
    slots = [4, 3, 11, 10, 9, 12, 18]
    entries = [0x10019460, 0x10019350, 0x10019cd0, 0x10019d10, 0x10019d50, 0x10019b00, 0x1001a0a0]
    require([image.word(table + 4*s) for s in slots] == entries, 'Installed setter identities')
    # Type0 takes the unsigned out-of-range cleanup return. Other admitted types
    # dispatch to that same return without invoking resource release.
    cleanup_table = [image.word(0x10017b48 + 4*i) for i in range(8)]
    require(all(cleanup_table[t-1] == 0x10017b44 for t in [2, 4, 6]), 'Resource-free cleanup branches')
    # memcpy contains inline jump tables, so this is a raw-byte digest rather
    # than pretending a linear disassembler proves all table-target boundaries.
    offset = image.offset(0x1007de80)
    memcpy_sha = hashlib.sha256(image.b[offset:offset+0x1c0]).hexdigest()
    exes = []
    for tag, machine, magic in [('native', 0x14c, 0x10b), ('owned', 0x8664, 0x20b)]:
        path = OUT / ('material_numeric_' + tag + ('_x86.exe' if tag == 'native' else '_x64.exe'))
        raw = path.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        require(raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', raw, pe+4)[0] == machine
                and struct.unpack_from('<H', raw, pe+24)[0] == magic, 'Architecture ' + tag)
        log = (OUT / (tag+'-build.log')).read_text()
        require(not re.search(r'\b(?:warning|error)\b', log, re.I), 'Build diagnostics ' + tag)
        exes.append(dict(path=str(path), sha256=sha(path), machine=hex(machine),
                         build_log_sha256=sha(OUT / (tag+'-build.log'))))
    analytical = subprocess.run([exes[1]['path'], '--test'], capture_output=True, text=True, timeout=30)
    (run / 'analytical.log').write_text(analytical.stdout + analytical.stderr)
    require(analytical.returncode == 0 and 'checks=26 refusal_cases=16' in analytical.stdout, 'Analytical checks')

    rng = random.Random(2026102519)
    rows = []
    labels = []
    def add(op, count=0, metadata=6, integer=0x12345678, vector=(0x3f800000, 0xc0000000, 0x40400000, 0xc0800000),
            args=(0, 0, 0, 0), label='controlled'):
        rows.append((op, count, metadata, integer, *vector, *args))
        labels.append(label)

    specials = [0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff, 0x00800000, 0x80800000,
                0x3f000000, 0xbf000000, 0x3f7fffff, 0x3f800000, 0x3f800001, 0xbf800000,
                0xbfcccccd, 0x40000000, 0x4b7fffff, 0x4b800000, 0x4b800001, 0x4effffff,
                0x4f000000, 0x4f000001, 0xceffffff, 0xcf000000, 0xcf000001,
                0x7f7fffff, 0xff7fffff, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f800001, 0xffc12345]
    forms = [(0, 0), (1, 0), (2, 0), (3, 0), (4, 0), *[(5, c) for c in range(1, 5)], (6, 0)]
    for high in range(16):
        for old_type in [0, 2, 4, 6]:
            for op, count in forms:
                for i, bits in enumerate(specials):
                    add(op, count, 0xb79a5c00 | (high << 4) | old_type,
                        args=(bits, specials[(i+7) % len(specials)], specials[(i+13) % len(specials)],
                              specials[(i+19) % len(specials)]), label='all_type_metadata_and_numeric_categories')
    # Same-value assignments must preserve deliberately stale other caches and
    # vector length metadata; signed zero compares equal, NaN does not.
    for high in range(16):
        for bits in specials:
            add(0, metadata=0xdeadbe00 | high*16 | 4, integer=bits, args=(bits, 8, 9, 10), label='same_int_cache_preservation')
            add(1, metadata=0xdeadbe00 | high*16, vector=(bits, 10, 11, 12),
                args=(bits ^ 0x80000000 if bits & 0x7fffffff == 0 else bits, 0, 0, 0), label='same_float_or_nan')
            for op, count in forms[2:-1]:
                n = count if op == 5 else op
                args = (bits, 0x3f800000, 0x80000000, 0)[:n] + (0,) * (4-n)
                old = tuple((0 if a == 0x80000000 else 0x80000000 if a == 0 else a) for a in args)
                add(op, count, 0xdeadbe00 | high*16 | 2, vector=old, args=args,
                    label='same_vector_or_nan_count_preservation')
    # Independent conversion comparisons cover integer float rounding ties near
    # powers of two and float truncation across every exponent and both signs.
    for exponent in range(32):
        for delta in range(-9, 10):
            for sign in [-1, 1]:
                value = sign * ((1 << exponent) + delta)
                if -2147483648 <= value <= 2147483647:
                    add(0, args=(value & 0xffffffff, 0, 0, 0), label='independent_integer_rounding')
    for exponent in range(256):
        for mantissa in [0, 1, 0x3fffff, 0x7fffff]:
            for sign in [0, 0x80000000]:
                add(1, args=(sign | exponent*0x800000 | mantissa, 0, 0, 0), label='independent_float_truncation')
    for _ in range(12000):
        op, count = rng.choice(forms)
        add(op, count, (rng.getrandbits(32) & ~15) | rng.choice([0, 2, 4, 6]), rng.getrandbits(32),
            tuple(rng.getrandbits(32) for _ in range(4)), tuple(rng.getrandbits(32) for _ in range(4)), 'deterministic_random_bits')
    query = run / 'queries.bin'
    query.write_bytes(b''.join(struct.pack('<12I', *r) for r in rows))
    outputs = []
    logs = []
    for tag, exe in zip(['native', 'owned'], exes):
        target = run / (tag+'.bin')
        result = subprocess.run([exe['path'], str(PRIVATE), str(query), str(target)], cwd=ROOT,
                                capture_output=True, text=True, timeout=120)
        logs.append(result.stdout.strip())
        (run / (tag+'.log')).write_text(result.stdout+result.stderr)
        require(result.returncode == 0 and 'PASS material_numeric_cases='+str(len(rows)) in result.stdout,
                'Numeric oracle ' + tag + ' exit=' + str(result.returncode) + ' ' + result.stdout + result.stderr)
        outputs.append(target.read_bytes())
        require(len(outputs[-1]) == 28*len(rows), 'Output framing ' + tag)
    differences = [i for i in range(len(rows)) if outputs[0][28*i:28*i+28] != outputs[1][28*i:28*i+28]]
    comparison = dict(differences=len(differences), details=[dict(case=i, label=labels[i], query=rows[i],
        native=struct.unpack_from('<7I', outputs[0], 28*i), owned=struct.unpack_from('<7I', outputs[1], 28*i)) for i in differences[:5]])
    (run / 'comparison.json').write_text(json.dumps(comparison, indent=2)+'\n')
    require(not differences, 'Native numeric outputs differ: ' + json.dumps(comparison))
    conversions = Counter()
    noops = Counter()
    changes = Counter()
    for i, (row, label) in enumerate(zip(rows, labels)):
        output = struct.unpack_from('<7I', outputs[0], 28*i)
        before = row[2:8]
        require(output[6] == int(output[:6] != before), 'Changed flag truth')
        (changes if output[6] else noops)[OPS[row[0]]] += 1
        if label == 'independent_integer_rounding':
            signed = struct.unpack('<i', struct.pack('<I', row[8]))[0]
            expected = struct.unpack('<I', struct.pack('<f', float(signed)))[0]
            require(output[1] == row[8] and output[2:6] == (expected,)*4, 'Independent integer conversion')
            conversions[label] += 1
        elif label == 'independent_float_truncation':
            require(output[1] == trunc32(row[8]) and output[2:6] == (row[8],)*4, 'Independent float conversion')
            conversions[label] += 1
        if label == 'same_int_cache_preservation':
            require(output[:6] == before, 'Same integer destroyed cache')
        if label == 'same_float_or_nan' and not math.isnan(f32(row[8])):
            require(output[:6] == before, 'Equal scalar destroyed cache or signed zero')
        if label == 'same_vector_or_nan_count_preservation' and not any(math.isnan(f32(v)) for v in row[8:]):
            require(output[:6] == before, 'Equal vector destroyed count/cache/signed zero')
        if row[0] == 6:
            require(output[0] == ((row[2] & ~15) | 6) and output[1:6] == before[1:], 'Undefined changed numeric caches')
    require(set(changes) == set(OPS) and len(noops) == len(OPS), 'Changed and unchanged coverage')
    previous_path = ROOT / 'outputs/vertexlit-shared-init-20261025/completion-check.json'
    previous = json.loads(previous_path.read_text())
    for name, pin in previous['sources_sha256'].items():
        require(sha(ROOT / 'work' / name) == pin, 'Existing initializer dependency changed: '+name)
    require(sha(previous['editor']['path']) == previous['editor']['sha256'], 'Editor changed')
    require(sha(LIB) == sha(PRIVATE) == PIN, 'Original/private library changed')
    tracked = ['material_numeric_value_candidate.h', 'probe_material_numeric_value.cpp',
               'build_material_numeric_value.cmd', 'verify_material_numeric_value.py']
    receipt = dict(status='PASS', utc=datetime.now(timezone.utc).isoformat(), cases=len(rows),
        exact_value_and_changed_bytes=len(outputs[0]), differences=0, analytical_checks=26, refusal_cases=16,
        corpus_groups=dict(Counter(labels)), per_operation=dict(Counter(OPS[r[0]] for r in rows)),
        changed_value_records=dict(changes), unchanged_value_records=dict(noops),
        independent_conversion_checks=dict(conversions), native_logs=logs, executables=exes,
        vtable=dict(address=hex(table), class_name='CMaterialVar',
                    setters={name: dict(slot=s, address=hex(a)) for name, s, a in zip(OPS, slots, entries)}),
        static_ranges=evidence, static_instruction_bytes=sum(r['bytes'] for r in evidence),
        cleanup_table=[hex(v) for v in cleanup_table], small_memcpy_raw_span_sha256=memcpy_sha,
        sources_sha256={n: sha(ROOT / 'work' / n) for n in tracked},
        fixture_sha256=sha(ROOT / 'work/native_private_world_image_fixture.h'), query_sha256=sha(query),
        native_output_sha256=sha(run / 'native.bin'), owned_output_sha256=sha(run / 'owned.bin'),
        original_materialsystem_sha256=PIN, originals_unchanged=True, previous_components_unchanged=True,
        previous_completion=str(previous_path), previous_completion_sha256=sha(previous_path), editor=previous['editor'],
        controlled_services=dict(call_queue='null; exactly one query per case', owner='null', cached_string='null',
                                 resource_aux='null', mxcsr='0x1f80; nearest-even, no DAZ/FTZ'),
        computer_use=False, sfm_process_used=False, dll_loader_used=False, gpu_used=False,
        limitations=[
            'Actual public methods and vector core execute unchanged; numeric writes are not callback substitutions.',
            'Only old types float0/vector2/int4/undefined6 with owner/cached-string/resource pointers null; other types refused.',
            'Controlled queue getter returns null. Queued updates, owner notification, resource/string lifetime and VMT construction remain unverified.',
            'NaNs/infinities/subnormals and cache/no-op behavior are value-byte tests, not FP exception/status parity.',
            'Earlier initializer callback fixtures remain unchanged; this does not yet prove native setters composed with initializers.',
            'Private PE32 oracle is test-only; owned helper is x64 and is not yet an editor dependency.',
            'No editor build/run, actual effective benchmark material load, full shading or visual parity claimed.'])
    (run / 'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status='PASS', receipt=str(run / 'results.json'), cases=len(rows),
                         exact_bytes=len(outputs[0]), analytical_checks=26, refusal_cases=16,
                         independent_conversions=sum(conversions.values()))), flush=True)


if __name__ == '__main__':
    main()
