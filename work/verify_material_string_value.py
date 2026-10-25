"""Compare real installed string/numeric state transitions in private fixtures.

Private PE32 oracle: no loader, DllMain, original service initialization or SFM
process access. Test memory allocation and C-locale thread access are controlled;
the complete setters and their installed integer/float parsers execute unchanged.
"""
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import random
import re
import struct
import subprocess
import tempfile
import zlib
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import RANGES, ROOT, LIB, PRIVATE, PIN, require, sha

OUT = ROOT / 'outputs/material-string-value-20261025'
OP_NAMES = ['int', 'float', 'vector2', 'vector3', 'vector4', 'vector_array', 'undefined', 'string']


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: '+str(run), flush=True)
    require(sha(LIB) == sha(PRIVATE) == PIN, 'Original/private material library identity')
    image, evidence = check_ranges(LIB, 'materialsystem-disassembly.txt', RANGES + [
        ('complete_SetString', 0x1001a580, 0x1001a6e1),
        ('allocator_free', 0x10032320, 0x10032337),
        ('allocator_allocate', 0x10032340, 0x10032357),
        ('string_length', 0x1006e2a0, 0x1006e2bb),
        ('string_bounded_copy', 0x1006e560, 0x1006e586),
        ('C_locale_accessor', 0x1007e306, 0x1007e38d),
        ('complete_float_parser_entry', 0x1007e58c, 0x1007e64a),
        ('complete_integer_parser_and_entries', 0x1007e64a, 0x1007e8c1),
        ('thread_lookup', 0x10086530, 0x100865c3),
        ('thread_function_lookup', 0x1008640b, 0x1008643f),
        ('float_decimal_conversion_entry', 0x100868e0, 0x10086987),
    ])
    require(image.word(0x10098f34+5*4) == 0x1001a580, 'Installed SetString slot')
    imports = {}
    for address in [0x10095078, 0x1009511c, 0x1009506c, 0x100951ec]:
        imports[hex(address)] = image.string(image.base+image.word(address)+2)
    require(list(imports.values()) == ['TlsGetValue', 'GetLastError', 'SetLastError', 'g_pMemAlloc'], 'Controlled import identities')
    require(image.word(0x10104bd8) == image.word(0x10104bdc) == 0x10104b00
            and image.word(0x10104898) == image.word(0x10104be0) == 0x10104470
            and image.word(0x10104b00+0xac) == 1, 'Original static C-locale links')
    exes = []
    for tag, machine, magic in [('native', 0x14c, 0x10b), ('owned', 0x8664, 0x20b)]:
        exe = OUT / ('material_string_'+tag+('_x86.exe' if tag == 'native' else '_x64.exe'))
        raw = exe.read_bytes()
        offset = struct.unpack_from('<I', raw, 60)[0]
        require(raw[offset:offset+4] == b'PE\0\0' and struct.unpack_from('<H', raw, offset+4)[0] == machine
                and struct.unpack_from('<H', raw, offset+24)[0] == magic, 'Executable architecture '+tag)
        log = OUT / (tag+'-build.log')
        require(not re.search(r'\b(?:warning|error)\b', log.read_text(), re.I), 'Compiler diagnostics '+tag)
        exes.append(dict(path=str(exe), sha256=sha(exe), machine=hex(machine), build_log_sha256=sha(log)))
    analytical = subprocess.run([exes[1]['path'], '--test'], capture_output=True, text=True, timeout=30)
    (run / 'analytical.log').write_text(analytical.stdout+analytical.stderr)
    require(analytical.returncode == 0 and 'checks=25 refusal_cases=16' in analytical.stdout, 'Analytical refusals')
    rows = []
    labels = []
    rng = random.Random(2026102520)
    def step(op=7, string=b'', count=0, args=(0, 0, 0, 0)):
        return op, count, args, string.encode() if isinstance(string, str) else string
    def add(steps, typeword=6, text=b'', integer=0x12345678, vector=(1, 2, 3, 4), label='controlled'):
        rows.append((typeword, integer, vector, text, steps))
        labels.append(label)
    strings = [b'', b'textures/pyro', b'$basetexture', b'12.5suffix', b'-0', b'+0', b'-nonsense',
        b'inf', b'-inf', b'nan', b'1.#INF', b'-1.#INF', b'1.#QNAN', b'1e309', b'-1e309',
        b'1e-999', b'-1e-999', b'0x1p2', b'1e+', b'  .5', b'1.', b'.', b'+.', b'-.',
        b'-2147483649', b'2147483648', b'99999999999999999999999', b'3\0ignored',
        b'[1 0.5 0]', b'{255 128 0}', b'1,5', b'\t\r\n\v\f -2.5E-1suffix',
        b'1' * 4095, b'a' * 4095, b'\xc3\xa9/texture', b' \xa0-1', b'0.000000000000000000000000000000000000000000001']
    # Exercise every high type nibble across every admitted old type. Each case
    # replaces an existing string twice, then frees it through a numeric setter.
    for high in range(16):
        for old in [0, 1, 2, 4, 6]:
            for string in strings:
                add([step(string=string), step(string=string), step(0, args=(7, 0, 0, 0))],
                    0xb79a5c00 | high*16 | old, b'previous/texture' if old == 1 else b'', label='metadata_categories_and_repeated_replacement')
    # All 256 leading byte values, in the original static C locale. Embedded
    # NUL is intentionally included; native inputs have independently owned storage.
    for byte in range(256):
        add([step(string=bytes([byte])+b'123.75suffix')], label='every_leading_byte')
    for n in [-2147483650, -2147483649, -2147483648, -2147483647, -16777217, -1, 0, 1,
              16777217, 2147483646, 2147483647, 2147483648, 2147483649, 4294967295]:
        for suffix in ['', '.5', 'e-2', 'junk']:
            add([step(string=str(n)+suffix)], label='integer_bounds')
    # Decimal strings around float32 and binary64/subnormal limits; test parser
    # behavior without narrowing the corpus to cases that happen to match.
    for exponent in [-9999, -325, -324, -309, -308, -46, -45, -44, -38, -1, 0, 1, 38, 39, 308, 309, 9999]:
        for significand in ['0', '-0', '1', '-1', '1.17549435', '1.40129846', '3.40282347', '9.999999999999999999']:
            add([step(string=significand+'e'+str(exponent))], label='decimal_extremes')
    for op, count in [(0, 0), (1, 0), (2, 0), (3, 0), (4, 0), *[(5, n) for n in range(1, 5)], (6, 0)]:
        for i in range(256):
            add([step(op, count=count, args=tuple(rng.getrandbits(32) for _ in range(4))),
                 step(string='restored/'+str(i)), step(6)], typeword=0x12345601 | ((i&15)<<4),
                text=b'existing', label='string_to_every_numeric_and_restore')
    for _ in range(4096):
        # Include very long mantissas and malformed exponent/prefix tails.
        digits = ''.join(str(rng.randrange(10)) for _ in range(rng.randrange(1, 50)))
        split = rng.randrange(len(digits)+1)
        string = rng.choice(['', ' ', '\t', '\r\n'])+rng.choice(['', '+', '-'])+digits[:split]+'.'+digits[split:]
        string += rng.choice(['', 'e'+str(rng.randrange(-400, 401)), 'E+', 'suffix', '\x00ignored'])
        add([step(string=string), step(6)], label='deterministic_decimal_strings')
    material_receipt = json.loads((ROOT / 'outputs/shader-skin-material-predicate-20261025/verification-v0hsdy0a/results.json').read_text())
    authored = []
    for material in material_receipt['authored_materials']:
        path = Path(material['path'])
        pin = material_receipt['originals_sha256'][str(path)]
        require(sha(path) == pin, 'Benchmark VMT changed')
        # These are literal quoted token strings, not a claim of full KeyValues
        # interpretation, shader parameter type selection, includes or patches.
        tokens = re.findall(rb'"([^"\r\n]*)"', path.read_bytes())
        authored.append(dict(path=str(path), sha256=pin, literal_quoted_tokens=len(tokens)))
        for token in tokens:
            require(len(token) <= 4095, 'Benchmark literal size')
            add([step(string=token), step(6)], label='benchmark_authored_literal_tokens')
    query = run / 'queries.bin'
    encoded = []
    for typeword, integer, vector, text, steps in rows:
        encoded.append(struct.pack('<8I', typeword, integer, *vector, len(text), len(steps))+text)
        for op, count, args, string in steps:
            encoded.append(struct.pack('<7I', op, count, *args, len(string))+string)
    query.write_bytes(b''.join(encoded))
    data = []
    logs = []
    for tag, exe in zip(['native', 'owned'], exes):
        target = run / (tag+'.bin')
        result = subprocess.run([exe['path'], str(PRIVATE), str(query), str(target)], cwd=ROOT,
                                capture_output=True, text=True, timeout=120)
        (run / (tag+'.log')).write_text(result.stdout+result.stderr)
        require(result.returncode == 0 and 'PASS material_string_cases='+str(len(rows)) in result.stdout,
                tag+' oracle exit='+str(result.returncode)+' '+result.stdout+result.stderr)
        data.append(target.read_bytes())
        logs.append(result.stdout.strip())
    def records(raw):
        at = 0
        parsed = []
        for case in rows:
            current = []
            for _ in case[4]:
                require(at+36 <= len(raw), 'Output header')
                h = struct.unpack_from('<9I', raw, at)
                size = 36+h[6]+12*h[7]
                require(at+size <= len(raw) and h[8] == 1, 'Output payload/queue queries')
                current.append((h, raw[at+36:at+36+h[6]], raw[at+36+h[6]:at+size], raw[at:at+size]))
                at += size
            parsed.append(current)
        require(at == len(raw), 'Output trailing bytes')
        return parsed
    native, owned = map(records, data)
    differences = []
    ops, transitions = Counter(), Counter()
    allocation_events = 0
    freed_strings = 0
    for i, (n, o) in enumerate(zip(native, owned)):
        old_type = rows[i][0] & 15
        for k, (nr, or_) in enumerate(zip(n, o)):
            op, _, _, text = rows[i][4][k]
            ops[OP_NAMES[op]] += 1
            transitions[str(old_type)+'->'+str(nr[0][0]&15)] += 1
            if nr[3] != or_[3]:
                differences.append(dict(case=i, step=k, label=labels[i], operation=op, input_hex=text.hex(),
                    native_words=nr[0], owned_words=or_[0], native_text_hex=nr[1].hex(), owned_text_hex=or_[1].hex(),
                    native_events_hex=nr[2].hex(), owned_events_hex=or_[2].hex()))
            require(nr[1] == text.split(b'\0', 1)[0] if op == 7 else nr[1] == b'', 'Native active text')
            expected_events = int(old_type == 1)+int(op == 7)
            require(nr[0][7] == expected_events, 'Unexpected native allocations/releases')
            events = list(struct.iter_unpack('<3I', nr[2]))
            require([e[0] for e in events] == ([2] if old_type == 1 else [])+([1] if op == 7 else []), 'Release-before-allocation order')
            if op == 7:
                require(events[-1][2] == len(nr[1])+1, 'Native string allocation size')
                allocation_events += 1
            freed_strings += int(old_type == 1)
            old_type = nr[0][0]&15
    comparison = dict(differences=len(differences), details=differences[:8])
    (run / 'comparison.json').write_text(json.dumps(comparison, indent=2)+'\n')
    require(not differences, 'String transition mismatch '+json.dumps(comparison))
    require(set(ops) == set(OP_NAMES), 'Setter coverage')
    previous_path = ROOT / 'outputs/material-numeric-value-20261025/completion-check.json'
    previous = json.loads(previous_path.read_text())
    for name, pin in previous['sources_sha256'].items():
        require(sha(ROOT / 'work' / name) == pin, 'Earlier numeric source changed '+name)
    require(sha(previous['editor']['path']) == previous['editor']['sha256'], 'Editor changed')
    require(sha(LIB) == sha(PRIVATE) == PIN and all(sha(m['path']) == m['sha256'] for m in authored), 'Originals changed')
    tracked = ['material_string_value_candidate.h', 'probe_material_string_value.cpp',
               'build_material_string_value.cmd', 'verify_material_string_value.py']
    receipt = dict(status='PASS', utc=datetime.now(timezone.utc).isoformat(), cases=len(rows),
        steps=sum(ops.values()), exact_state_text_trace_bytes=len(data[0]), differences=0,
        analytical_checks=25, refusal_cases=16, operations=dict(ops), transitions=dict(transitions),
        allocations=allocation_events, string_releases=freed_strings, corpus_groups=dict(Counter(labels)),
        authored_material_tokens=authored, logs=logs, executables=exes,
        static_ranges=evidence, static_instruction_bytes=sum(x['bytes'] for x in evidence),
        controlled_imports=imports, static_C_locale='0x10104b00',
        controlled_services=['Owner null, no call queue; exactly one query/step',
            'Bounded tracked host allocator; release-before-allocate trace and final no-live-block checks',
            'Private thread/TLS lookup returning installed static C-locale links; original CRT initialization skipped',
            'GetLastError/SetLastError test shims; MXCSR0x1f80, x87 nearest-even/53-bit'],
        sources_sha256={n: sha(ROOT / 'work' / n) for n in tracked},
        numeric_header_sha256=sha(ROOT / 'work/material_numeric_value_candidate.h'),
        fixture_sha256=sha(ROOT / 'work/native_private_world_image_fixture.h'),
        query_sha256=sha(query), native_output_sha256=sha(run / 'native.bin'), owned_output_sha256=sha(run / 'owned.bin'),
        original_materialsystem_sha256=PIN, originals_unchanged=True, previous_components_unchanged=True,
        previous_completion=str(previous_path), previous_completion_sha256=sha(previous_path), editor=previous['editor'],
        computer_use=False, sfm_process_used=False, dll_loader_used=False, gpu_used=False,
        limitations=[
            'Actual complete public methods and original numeric parsers execute; test allocator and C-locale thread services are controlled.',
            'Only ownerless immediate old types float/string/vector/int/undefined; other resource types, owners and queues refused/unverified.',
            'Owned strings use RAII storage; stale freed x86 pointer bytes are deliberately not copied into the x64 object.',
            'Exact normalized active state and allocation identity/order/size, not allocator implementation or raw object pointer bytes.',
            'Owned event records are a synthetic logical lifecycle ledger, not std::string allocation instrumentation. Native events capture actual callbacks; owned RAII/SSO allocation timing is intentionally different.',
            'C-locale conversion only. Other locale/codepage behavior and FP/errno/status side effects unverified.',
            'Authored benchmark tokens are string-assignment inputs, not full KeyValues parsing, effective shader parameter typing or live material loading.',
            'Shared/Phong initializer composition, texture initialization, renderer/editor integration and full shading remain unverified.',
            'No production dependency changed and no complete editor build/run occurred.'])
    archive = run / 'native-normalized-records.bin.zlib'
    archive.write_bytes(zlib.compress(data[0], 9))
    require(zlib.decompress(archive.read_bytes()) == data[0] == data[1], 'Exact output archive round trip')
    receipt['exact_output_archive'] = dict(path=str(archive), sha256=sha(archive),
        bytes=archive.stat().st_size, decompressed_bytes=len(data[0]),
        normalized_native_owned_identical=True, duplicate_uncompressed_outputs_removed=True)
    for tag in ['native', 'owned']:
        (run / (tag+'.bin')).unlink()
    (run / 'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status='PASS', receipt=str(run / 'results.json'), cases=len(rows), steps=receipt['steps'],
        exact_bytes=len(data[0]), allocations=allocation_events, string_releases=freed_strings,
        analytical_checks=25, refusal_cases=16)), flush=True)


if __name__ == '__main__':
    main()
