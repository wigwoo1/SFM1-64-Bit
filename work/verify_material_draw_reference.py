"""Whole direct/forwarded original material draw vs existing owned adapter."""
from pathlib import Path
from datetime import datetime, timezone
import difflib, hashlib, itertools, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/material-draw-reference-20261028'
MATERIAL = ROOT / 'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def corpus():
    queries, rng = [], random.Random(2026102832)
    alphas = [0, 1, 255, 0x100, 0xffffff00, 0xffffffff]
    for flags, parameters, shader, forward, mutation in itertools.product([0, 4, 0x100, 0x104, 0xffffffff, 0x80000000], range(3), range(2), range(2), range(64)):
        i = len(queries)
        queries.append([i, shader, parameters, forward, mutation, flags, alphas[i % len(alphas)], *[rng.getrandbits(32) for _ in range(4)]])
    for _ in range(512):
        queries.append([len(queries), rng.randrange(2), rng.randrange(3), rng.randrange(2), rng.randrange(64), rng.getrandbits(32), rng.choice(alphas + [rng.getrandbits(32)]), *[rng.getrandbits(32) for _ in range(4)]])
    return queries

def prediction(queries):
    data = bytearray()
    callbacks = missing = event_count = 0
    for index, shader, parameters, forward, mutation, flags, alpha, compression, mode, cookie, encoded in queries:
        allowed = bool(shader and not (parameters == 2 and flags & 4))
        sid, lid = int(bool(shader)), int(bool(parameters))
        f0, f1 = flags, flags ^ 4
        v0, v1 = 0x11110000 + index, 0x22220000 + index
        block = [index, compression, mode]
        events = []
        if not shader:
            missing += 1
            events.append([0] * 9)
        elif allowed:
            callbacks += 1
            events.append([1, 1, 0, 0, 0, 0, 0, 0, 0])
            if mutation & 1:
                sid = 2
            if mutation & 2:
                lid = 2
            if mutation & 4:
                cookie ^= 0xf1e2d3c4
                encoded ^= 0x12345678
            if mutation & 8:
                f0, f1, v0, v1 = f0 ^ 4, f1 ^ 4, v0 + 7, v1 + 11
            if mutation & 16:
                block = [v ^ 0xdeadbeef for v in block]
            if mutation & 32:
                sid = 0
            entry = lid == 2 or (lid == 1 and parameters == 2)
            observed_flags = (f1 if lid == 2 else f0) if entry else 0
            observed_value = (v1 if lid == 2 else v0) if entry else 0
            events.append([2, sid, lid, observed_flags, observed_value, compression, encoded ^ cookie, int(bool(alpha & 255)), mode])
        header = [index, int(not shader), int(allowed), int(allowed), sid, lid, f0, v0, cookie, encoded, *block, f1, v1, len(events)]
        data += struct.pack('<16I', *header)
        for event in events:
            data += struct.pack('<9I', *event)
        event_count += len(events)
    return bytes(data), dict(cases=len(queries), callback_draws=callbacks, missing=missing, events=event_count, independent_checks=len(queries) + callbacks)

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/shader-api-instance-draw-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == 'ccd0841a2430971ce42b1cff22be06a073a2376ee948f3ab142d9cb9354eb6f0', 'accepted instance draw protocol receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent)}
    for e in prior['executables'].values():
        protected[e['path']] = e['sha256']
    originals = dict(prior['original_inputs'])
    need(all(sha(p) == h for p, h in protected.items()), 'previous source/build artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()) and originals[str(MATERIAL)] == 'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8', 'pinned original/private inputs before')
    header = ROOT / 'work/studio_material_draw_candidate.h'
    before = OUT / 'header-before.bin'
    preflight = json.loads((OUT / 'preflight.json').read_text())
    need(sha(before) == preflight['header_sha256_before'], 'saved pre-change owned header')
    old = before.read_text()
    new = header.read_text()
    expected = old.replace('// Logical equivalent of10004ae0. Encoding cookie widened with encoded pointer.\n', '// Logical adapter for10004ae0. Original token/cookie XOR is32bit; uintptr_t\n// transport here is an owned extension, not proof the original token is a\n// pointer. Native shader-state storage is embedded at material+30; the relation\n// to this caller-supplied shaderState address needs ownership/integration proof.\n', 1)
    need(old != expected and new == expected and str(header) not in protected and str(header) not in prior['editor_dependencies'], 'only unsupported pointer-semantics comment changed; executable code unchanged')
    (run / 'comment-change.json').write_text(json.dumps(dict(before_sha256=sha(before), after_sha256=sha(header), diff=''.join(difflib.unified_diff(old.splitlines(True), new.splitlines(True), fromfile='before', tofile='after'))), indent=2) + '\n')
    sources = closure([ROOT / 'work/probe_material_draw_reference.cpp', ROOT / 'work/build_material_draw_reference.cmd', ROOT / 'work/test_studio_material_draw.cpp', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT / 'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    image, static = check_ranges(MATERIAL, 'materialsystem-disassembly.txt', [('whole_direct_material_draw', 0x10004ae0, 0x10004b5a), ('whole_forwarding_material_draw', 0x1000a910, 0x1000a921)])
    need(image.word(0x10095d74 + 71 * 4) == 0x10004ae0 and image.word(0x10095a9c + 71 * 4) == 0x1000a910, 'actual direct/forward material vtable slot71')
    (run / 'static-ranges.private.json').write_text(json.dumps(static, indent=2) + '\n')
    queries = corpus()
    need(len(queries) == 5120, 'complete deterministic direct/forward/gate/callback corpus')
    query = run / 'queries.bin'
    query.write_bytes(b''.join(struct.pack('<11I', *q) for q in queries))
    expected, coverage = prediction(queries)
    (run / 'analytical.bin').write_bytes(expected)
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_material_draw_reference.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh direct/forward reference, owned and existing-regression builds')
    executables = {}
    for tag, name, machine, magic, logname in [('reference', 'material_draw_reference_x86.exe', 0x14c, 0x10b, 'reference-build.log'), ('owned', 'probe_material_draw_reference_x64.exe', 0x8664, 0x20b, 'probe_material_draw_reference-build.log'), ('regression', 'test_studio_material_draw_x64.exe', 0x8664, 0x20b, 'test_studio_material_draw-build.log')]:
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
        args = [executables[tag]['path'], *([str(MATERIAL)] if tag == 'reference' else []), str(query), str(output)]
        result = subprocess.run(args, cwd=run, capture_output=True, text=True, timeout=90)
        (run / (output.stem + '.log')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, tag + ' exit=' + str(result.returncode) + '; inspect ' + output.stem + '.log')
        need(output.read_bytes() == expected, 'entire normalized original/owned trace and final live state vs independent prediction ' + output.stem)
        need(f'PASS material_draw_reference_cases={coverage["cases"]} events={coverage["events"]} independent_checks={coverage["independent_checks"]}' in result.stdout, 'exact independent gate/live-field coverage ' + tag)
        need(('target=OWNED_X64' if tag == 'owned' else 'original_code=UNCHANGED_RX') in result.stdout, 'owned/reference identity ' + tag)
        runs.append(dict(tag=tag, output=str(output), sha256=sha(output), stdout=result.stdout.strip()))
    regression = subprocess.run([executables['regression']['path']], cwd=run, capture_output=True, text=True, timeout=60)
    (run / 'existing-owned-regression.log').write_text(regression.stdout + regression.stderr)
    need(regression.returncode == 0 and 'material_draw_cases=8192 missing_shader_gate=1 flags_gate=1 live_state_reload=1 full_width_context_decode=1 low_byte_alpha=1' in regression.stdout, 'fresh existing8192 owned cases retained; wide-token transport is owned extension')
    need(all(sha(p) == h for p, h in sources.items()) and all(sha(p) == h for p, h in protected.items()), 'new/current source and all old tested artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p, h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], 'editor313 dependencies and exact executable unchanged')
    outcome = dict(status='VERIFIED existing owned material draw adapter vs original direct/forwarded slot71; shader-system body INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', parent=str(parent), parent_sha256=sha(parent), source=sources, protected=protected, original_inputs=originals, executables=executables, runs=runs, coverage=coverage, complete_trace_bytes=len(expected), query_sha256=sha(query), analytical_sha256=sha(run / 'analytical.bin'), compiler_warnings=0, fresh_regression=regression.stdout, comment_change_sha256=sha(run / 'comment-change.json'), static_ranges=[{k: v for k, v in s.items() if k != 'instructions'} for s in static], private_static_sha256=sha(run / 'static-ranges.private.json'), editor=prior['editor'], editor_sha256=prior['editor_sha256'], editor_dependencies=prior['editor_dependencies'], editor_rebuilt=False, findings=[
        'Whole original CMaterial slot71 and queue-friendly forwarding slot71 execute through actual material vtables with unchanged RX instructions. Shader slot0, shader-system slot18 and missing-shader diagnostic are controlled observers, not original shader execution.',
        'All5120 native/owned records match exact gate/callback/receiver/list/value/token/flag order and final state against independent prediction, repeated twice per architecture. Both native entry routes have the same observed behavior. The unchanged existing x64 adapter matches this controlled original32bit domain.',
        'Missing shader gate precedes parameter gate. First parameter bit4 is checked only before shader callback. The shader and parameter-list pointer are reread afterward; pointer/value changes and newly-set bit4 are forwarded without another gate. No material-parameter value snapshot is created by this boundary.',
        'The native shader-state argument is always the address of embedded material+30, whose live bytes can change during callback. Fixture uses stable owned block storage for x64 parity. General external shaderState pointer ownership/integration remains unverified; old wide-ID tests alone do not prove embedded-state equivalence.',
        'Original encoded token and cookie are32bit XOR operands. Actual semantic type is unverified. Corrected only the old pointer-semantics comment; existing code remains unchanged. Wider uintptr_t token/cookie transport remains an owned extension, not proof of an original pointer ABI. Alpha is normalized from the low byte; mode transports full32bits.',
        'Removing shader during callback is forwarded as null to our observer. Real shader-system null safety is not inferred; no recovery behavior is invented. Whole shader-system10037D40 parameter consumption, shader callback body, resource/thread ownership, full draw-chain composition and GPU output remain unverified.',
        'New probe/build/verifier and comment-only clarification. All original files, prior source/artifacts, editor313 dependencies and exact editor binary unchanged. Fresh8192 existing owned tests pass. No SFM process access/enumeration, original writes, dependencies or publication.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(outcome, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=outcome['status'], receipt=str(receipt), coverage=coverage, complete_trace_bytes=len(expected), fresh_regression_cases=8192, editor_rebuilt=False), indent=2), flush=True)

if __name__ == '__main__':
    main()
