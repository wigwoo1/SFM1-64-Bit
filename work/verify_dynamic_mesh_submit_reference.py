"""Actual original mesh submit/instance building vs existing owned x64 adapter."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/dynamic-mesh-submit-reference-20261028'
SHADER = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def fbits(v):
    return struct.unpack('<I', struct.pack('<f', v))[0]

def number(bits):
    return struct.unpack('<f', struct.pack('<I', bits))[0]

def corpus():
    seed = 0x28261030
    def word():
        nonlocal seed
        seed = (seed * 1664525 + 1013904223) & 0xffffffff
        return seed
    out = []
    for supplied, mutation, pending, batch, colors, compressed in itertools.product(range(2), range(32), [0, 1, 0xffffffff], range(2), range(4), range(2)):
        count, low, high, offa, offb = [word() for _ in range(5)]
        low = (low & ~0x400) | (0x400 if compressed else 0)
        color = [fbits(v) for v in [(1, 1, 1, 1), (2, .25, -.5, .75), (0.0, -0.0, 0.0, -0.0), (.1, -.2, .3, -.4)][colors]]
        mods = [0x3f000000 | (word() & 0x807fffff) for _ in range(4)] if colors == 3 else [fbits(v) for v in [(1, 1, 1, 1), (.5, -2, 4, 0), (-0.0, 0.0, -1, 1)][colors]]
        out.append([len(out), supplied, mutation, pending, batch, count, low, high, offa, offb, *color, *mods])
    return out

def prediction(queries):
    out = bytearray()
    total_events = 0
    for q in queries:
        index, supplied, mutation, pending, batch, count, low, high, offa, offb, *values = q
        color, mods = values[:4], values[4:]
        active_batch = bool(pending and batch)
        scalars = [0] * 19
        scalars[2] = (pending + 1) & 0xffffffff if active_batch else 1
        scalars[11:13] = [offa, offb]
        captured_mod = [b ^ (0x80000000 if mutation & 1 else 0) for b in mods]
        captured_color = color[:]
        if mutation & 4:
            captured_color[3] ^= 0x80000000
        # Products of two finite binary32 values are exact in binary64 before
        # this explicit binary32 rounding. Corpus avoids overflow/underflow.
        scalars[15:19] = [fbits(number(a) * number(b)) for a, b in zip(captured_color, captured_mod)] if supplied else captured_color
        matrix = [index * 31 + i for i in range(12)] if active_batch else [0x3f000000 + ((index * 31 + i * 17) & 0x7fffff) for i in [0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14]]
        final_color = captured_color[:]
        if mutation & 16:
            final_color[:3] = [b ^ 0x80000000 for b in final_color[:3]]
        final_mod = [fbits(99)] * 4 if mutation & 2 else captured_mod
        events = ([] if active_batch else [(1, 0, 0)]) + [(2, 0, 0), (3, 0, 0), (4, 1, 0), (5, 1, 0), (6, 0, 0), (8, 0, 0), (9, 0, 0)]
        header = [index, 0 if active_batch else pending, count, int(bool((low ^ (0x400 if mutation & 8 else 0)) & 0x400)), 2 if mutation & 4 else 1, *final_color, *final_mod, *scalars, *matrix, 1, 2, 3, 4, len(events)]
        need(len(header) == 49, 'analytical record shape')
        out += struct.pack('<49I', *header)
        for event in events:
            out += struct.pack('<3I', *event)
        total_events += len(events)
    return bytes(out), total_events

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/shader-material-flush-composition-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == '436c05c636ae409ba4c9d2d4e7f40c1546fa741653bc6608e456b1ba774cb8c3', 'accepted binding/flush composition receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent)}
    for executable in prior['executables'].values():
        protected[executable['path']] = executable['sha256']
    originals = dict(prior['original_inputs'])
    need(all(sha(p) == h for p, h in protected.items()), 'old source/test artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()) and originals[str(SHADER)] == '07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa', 'pinned original/private inputs unchanged before')
    sources = closure([ROOT / 'work/probe_dynamic_mesh_submit_reference.cpp', ROOT / 'work/build_dynamic_mesh_submit_reference.cmd', ROOT / 'work/test_studio_dynamic_mesh_submit.cpp', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT / 'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    image, static = check_ranges(SHADER, 'shaderapidx9-disassembly.txt', [
        ('whole_dynamic_mesh_submit', 0x100093f0, 0x1000956c),
        ('whole_instance_builder', 0x1000ffb0, 0x100100a0),
        ('whole_default_matrix_conversion', 0x10057cc0, 0x10057d31),
        ('original_small_memset', 0x100df3a0, 0x100df41a),
        ('original_cookie_normal_return', 0x100df5ad, 0x100df5b7),
        ('actual_count_setter', 0x1000ea60, 0x1000ea70)])
    need([image.word(0x100fcedc + i * 4) for i in [253, 260, 274, 277]] == [0x10025b50, 0x10013fd0, 0x1000ffb0, 0x1000ea60], 'actual ShaderApi030 submit/instance/count bindings')
    need(image.word(0x1016f8c8) == 0x10057ca7, 'original CPU-dispatch data entry before controlled pin')
    (run / 'static-ranges.private.json').write_text(json.dumps(static, indent=2) + '\n')
    queries = corpus()
    need(len(queries) == 3072, 'complete mutation/builder/modulation/compression corpus')
    query = run / 'queries.bin'
    query.write_bytes(b''.join(struct.pack('<18I', *q) for q in queries))
    expected, event_count = prediction(queries)
    need(event_count == 23552 and len(expected) == 884736, 'complete independent prediction size/count')
    (run / 'analytical.bin').write_bytes(expected)
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_dynamic_mesh_submit_reference.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh native/owned/regression builds')
    executables = {}
    for tag, name, machine, magic, logname in [('reference', 'mesh_submit_reference_x86.exe', 0x14c, 0x10b, 'reference-build.log'), ('owned', 'probe_dynamic_mesh_submit_reference_x64.exe', 0x8664, 0x20b, 'probe_dynamic_mesh_submit_reference-build.log'), ('regression', 'test_studio_dynamic_mesh_submit_x64.exe', 0x8664, 0x20b, 'test_studio_dynamic_mesh_submit-build.log')]:
        exe = OUT / name
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', raw, pe + 4)[0] == machine and struct.unpack_from('<H', raw, pe + 24)[0] == magic and b'asInvoker' in raw, 'fresh architecture/manifest ' + tag)
        log = (OUT / logname).read_text()
        (run / logname).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', log, re.I), 'clean compile ' + tag)
        inspected = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        need(inspected.returncode == 0, 'imports ' + tag)
        (run / (tag + '-imports.txt')).write_text(inspected.stdout)
        imports = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', inspected.stdout, re.I | re.M)))
        need(imports == ['KERNEL32.dll'], 'system-only direct imports ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), machine=hex(machine), imports=imports)
    runs = []
    for tag in ['reference', 'owned', 'reference', 'owned']:
        output = run / (tag + '-' + str(len(runs)) + '.bin')
        args = [executables[tag]['path'], *([str(SHADER)] if tag == 'reference' else []), str(query), str(output)]
        result = subprocess.run(args, cwd=run, capture_output=True, text=True, timeout=90)
        (run / (output.stem + '.log')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, tag + ' exit=' + str(result.returncode) + '; inspect ' + output.stem + '.log')
        need(output.read_bytes() == expected, 'entire normalized instance/matrix/scalar/event trace vs independent finite-float prediction ' + output.stem)
        need('PASS dynamic_mesh_submit_cases=3072 events=23552 independent_checks=27648' in result.stdout, 'exact coverage/independent assertion counts ' + tag)
        need(('target=OWNED_X64' if tag == 'owned' else 'original_code=UNCHANGED_RX cpu_matrix_path=PINNED_DEFAULT') in result.stdout, 'owned/native reference identity ' + tag)
        runs.append(dict(tag=tag, output=str(output), sha256=sha(output), stdout=result.stdout.strip()))
    regression = subprocess.run([executables['regression']['path']], cwd=run, capture_output=True, text=True, timeout=60)
    (run / 'existing-owned-regression.log').write_text(regression.stdout + regression.stderr)
    need(regression.returncode == 0 and 'dynamic_mesh_submit_cases=2048 captured_modulation=1 matrix_instance_integration=1 full_width_pointers=1 callback_order=1' in regression.stdout, 'fresh existing2048 owned tests preserved')
    need(all(sha(p) == h for p, h in protected.items()) and all(sha(p) == h for p, h in sources.items()), 'all old/new source and previous tested artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p, h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], 'editor313 dependencies and binary unchanged')
    outcome = dict(status='VERIFIED existing x64 dynamic mesh submission vs whole original helper; shader consumption INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', parent=str(parent), parent_sha256=sha(parent), source=sources, protected=protected, original_inputs=originals, executables=executables, runs=runs, coverage=dict(cases=3072, events=23552, independent_checks=27648, instance_bytes=76, repeated_native_owned_pairs=2), query_sha256=sha(query), analytical_sha256=sha(run / 'analytical.bin'), complete_trace_bytes=len(expected), compiler_warnings=0, fresh_regression=regression.stdout, static_ranges=[{k: v for k, v in s.items() if k != 'instructions'} for s in static], private_static_sha256=sha(run / 'static-ranges.private.json'), editor=prior['editor'], editor_sha256=prior['editor_sha256'], editor_dependencies=prior['editor_dependencies'], editor_rebuilt=False, findings=[
        'Whole original dynamic mesh submit100093F0, actual instance builder1000FFB0, original small memset, cookie check, selected default matrix transpose10057CC0 and count setter1000EA60 execute with unchanged RX instructions in private reference child.',
        'All3072 native/owned cases match full normalized76byte scalar record, retained48byte matrix, pointer identities, state mutations and callback trace against a separate analytical prediction. The existing owned adapters are unchanged, with newly established original execution parity for this controlled domain.',
        'Modulation is captured after mesh offsets and before the material getter. RGB and alpha use the same captured material even when RGB changes the current binding. RGB callback changes to alpha affect the record; later changes to RGB after colour capture do not. Format/compression is read later, after those callbacks.',
        'The stack instance owns captured RGBA scalar values, but matrices, lighting and metadata are borrowed full-width retained pointers consumed synchronously. Pointer scalar slots are normalized only for cross-architecture evidence; owned adapter is a logical type, not native76byte ABI.',
        'Both pending matrix-batch and default conversion paths execute. Only a mutable CPU-dispatch table entry is pinned to the actual default x87 implementation; no original instruction is patched and no automatic CPU selection is inferred. Finite normal/zero colour/modulation corpus avoids overflow/underflow and does not establish arbitrary NaN/FP-mode parity.',
        'Material getter/RGB/alpha, mesh offsets/count/format, matrix source and final ShaderAPI253 submit are controlled boundary observers. Original dynamic draw preparation10009FD0, full ShaderAPI25310025B50, CMaterial71 and shader/GPU bodies do not execute here. Instance RGBA capture cannot be generalized to other material parameters/textures.',
        'Three new test/build/verification files only. Existing owned mesh/instance/matrix adapters and editor unchanged. Fresh2048-case existing regression passes. No live SFM/process enumeration, original writes, production integration, dependencies or publication.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(outcome, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=outcome['status'], receipt=str(receipt), coverage=outcome['coverage'], complete_trace_bytes=len(expected), fresh_regression_cases=2048, editor_rebuilt=False), indent=2), flush=True)

if __name__ == '__main__':
    main()
