"""Original whole ShaderApi030 instance draw vs owned orchestration protocol."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/shader-api-instance-draw-20261028'
SHADER = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def corpus():
    out, rng = [], random.Random(2026102831)
    alphas = [0x3f800000, 0x3f000000, 0x7fc00000]
    for present, count, first, second, mutations in itertools.product(range(2), [0xffffffff, 0, 1], [1, 2, 3], [1, 2, 3], range(16)):
        i = len(out)
        out.append([i, present, count, first, second, mutations, int(i % 13 == 0), i & 1, (i >> 1) & 1, alphas[i % 3], i % 3, rng.getrandbits(32), (i >> 2) & 1, (i >> 3) & 1, rng.getrandbits(32)])
    for _ in range(512):
        out.append([len(out), rng.randrange(2), rng.choice([0xffffffff, 0x80000000, 0, 1, 2]), rng.choice([0, 1, 2, 0xffffffff]), rng.choice([0, 1, 2, 3]), rng.randrange(16), rng.randrange(2), rng.randrange(2), rng.randrange(2), rng.choice(alphas), rng.choice([0, 1, 2, 0xffffffff]), rng.getrandbits(32), rng.randrange(2), rng.randrange(2), rng.getrandbits(32)])
    return out

def prediction(queries):
    data = bytearray()
    draw_count = setup_count = event_count = 0
    for index, present, count, first, second, mutations, disabled, third, restore, alpha, config, compression, matflag, meshflag, alpha_result in queries:
        events, modes, suppressions, material = [(1, 0, 0, 0)], 0, 0, 1
        if disabled:
            header = [index, 0, 0, 0, 3, 0x1111, 1, 1, 1, 0x80, 0x7f, 99, 0x9b, alpha, 1]
            snapshot = [0xabcdef00 + i for i in range(7)]
        else:
            positive = 0 < count < 0x80000000
            mode = bool(present and positive and (first == 1 or second == 2))
            if present:
                selected_mesh = 2 if mutations & 1 else 1
                events.extend([(2, 1, 0, 0), (3, selected_mesh, count, 0)])
                if positive:
                    events.append((4, 0, first, 0))
                    modes += 1
                    if first != 1:
                        events.append((4, 0, second, 0))
                        modes += 1
                events.append((5, 1, matflag, 0))
                if mutations & 2:
                    material, selected_mesh = 2, 3
                captured_material = material
                events.append((6, selected_mesh, 0, 0))
                if mutations & 4:
                    material = 3
                events.extend([(7, captured_material, int(mode), alpha_result), (8, selected_mesh, meshflag, 0), (9, 0, matflag | (meshflag << 8) | ((alpha_result & 255) << 16), 0)])
                setup_count += 1
                if mutations & 8:
                    alpha = 0x3e800000
            if third:
                events.extend([(11, 0, 0, 0), (11, 0, 0, 0)])
            events.append((10, material, 0, 0))
            suppressions += 1
            if mutations & 4:
                material = 2
            events.append((12, material, compression, int(alpha != 0x3f800000) | (int(mode) << 8)))
            if restore:
                for _ in range(8 if config == 1 else 1):
                    events.append((10, material, 0, 0))
                    suppressions += 1
            draw_count += 1
            header = [index, modes, suppressions, 1, 0, 0, 0, 0, 0, 0, 0, 0xface, int(config == 1), alpha, len(events)]
            snapshot = [index * 7 + i for i in range(7)]
        data += struct.pack('<22I', *header, *snapshot)
        for event in events:
            data += struct.pack('<4I', *event)
        event_count += len(events)
    return bytes(data), dict(cases=len(queries), events=event_count, draws=draw_count, setups=setup_count, checks=len(queries) * 2 + draw_count + setup_count)

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/dynamic-mesh-submit-reference-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == '14f14e32c4104ef19cf3eb8a540eff6bdf425f61c088e612861dbea060545f14', 'accepted original/owned mesh submit receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent)}
    for e in prior['executables'].values():
        protected[e['path']] = e['sha256']
    originals = dict(prior['original_inputs'])
    need(all(sha(p) == h for p, h in protected.items()), 'all old source/test artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()), 'all original/private inputs unchanged before')
    sources = closure([ROOT / 'work/probe_shader_api_instance_draw.cpp', ROOT / 'work/build_shader_api_instance_draw.cmd', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT / 'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    image, static = check_ranges(SHADER, 'shaderapidx9-disassembly.txt', [
        ('whole_instance_draw', 0x10025b50, 0x10025da2),
        ('whole_quiescent_prepare_A', 0x10016530, 0x100167b0),
        ('whole_quiescent_prepare_B', 0x1001e950, 0x1001ede6),
        ('whole_prepare_C_zero_capability', 0x10013490, 0x100136af),
        ('whole_callback_drain', 0x1001f820, 0x1001f89c),
        ('whole_restore_settings', 0x10015230, 0x100152bc),
        ('whole_suppressed_render_state_submit', 0x1000e330, 0x1000e3fc),
        ('normal_cookie_check', 0x100df5ad, 0x100df5b7)])
    need(image.word(0x100fcedc + 253 * 4) == 0x10025b50, 'actual ShaderApi030 instance draw slot')
    (run / 'static-ranges.private.json').write_text(json.dumps(static, indent=2) + '\n')
    queries = corpus()
    need(len(queries) == 1376, 'complete deterministic corpus')
    query = run / 'queries.bin'
    query.write_bytes(b''.join(struct.pack('<15I', *q) for q in queries))
    expected, coverage = prediction(queries)
    (run / 'analytical.bin').write_bytes(expected)
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_shader_api_instance_draw.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh original/owned builds')
    executables = {}
    for tag, name, machine, magic in [('reference', 'instance_draw_reference_x86.exe', 0x14c, 0x10b), ('owned', 'instance_draw_owned_x64.exe', 0x8664, 0x20b)]:
        exe = OUT / name
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', raw, pe + 4)[0] == machine and struct.unpack_from('<H', raw, pe + 24)[0] == magic and b'asInvoker' in raw, 'fresh architecture/manifest ' + tag)
        log = (OUT / (tag + '-build.log')).read_text()
        (run / (tag + '-build.log')).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', log, re.I), 'clean compile ' + tag)
        inspected = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        need(inspected.returncode == 0, 'imports ' + tag)
        (run / (tag + '-imports.txt')).write_text(inspected.stdout)
        imports = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', inspected.stdout, re.I | re.M)))
        need(imports == ['KERNEL32.dll'], 'system-only imports ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), machine=hex(machine), imports=imports)
    runs = []
    for tag in ['reference', 'owned', 'reference', 'owned']:
        output = run / (tag + '-' + str(len(runs)) + '.bin')
        args = [executables[tag]['path'], *([str(SHADER)] if tag == 'reference' else []), str(query), str(output)]
        result = subprocess.run(args, cwd=run, capture_output=True, text=True, timeout=90)
        (run / (output.stem + '.log')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, tag + ' exit=' + str(result.returncode) + '; inspect ' + output.stem + '.log')
        need(output.read_bytes() == expected, 'every control-flow event and final published state vs independent analytical trace ' + output.stem)
        need(f'PASS shader_instance_draw_cases={coverage["cases"]} events={coverage["events"]} independent_checks={coverage["checks"]}' in result.stdout, 'independent scope/late-read/cleanup coverage ' + tag)
        need(('target=OWNED_X64' if tag == 'owned' else 'original_code=UNCHANGED_RX') in result.stdout, 'owned/reference identity ' + tag)
        runs.append(dict(tag=tag, output=str(output), sha256=sha(output), stdout=result.stdout.strip()))
    # Reuse the previously accepted owned mesh submission trace for regression;
    # its original executable remains frozen and is not rebuilt in this task.
    old_query = Path(prior['runs'][0]['output']).parent / 'queries.bin'
    need(sha(old_query) == prior['query_sha256'], 'prior mesh submit query identity')
    regression_output = run / 'prior-mesh-submit-owned.bin'
    regression = subprocess.run([prior['executables']['owned']['path'], str(old_query), str(regression_output)], cwd=run, capture_output=True, text=True, timeout=60)
    (run / 'prior-mesh-submit-owned.log').write_text(regression.stdout + regression.stderr)
    need(regression.returncode == 0 and sha(regression_output) == prior['analytical_sha256'], 'all3072 preceding mesh submission cases unchanged')
    need(all(sha(p) == h for p, h in protected.items()) and all(sha(p) == h for p, h in sources.items()), 'all old/new source and old build artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p, h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], 'editor313 dependencies and exact binary unchanged')
    need(str(ROOT / 'work/shader_api_instance_draw_candidate.h') not in prior['editor_dependencies'], 'new orchestration not activated in editor')
    outcome = dict(status='VERIFIED owned x64 instance-draw control protocol; graphics preparation/material shader body INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', parent=str(parent), parent_sha256=sha(parent), source=sources, protected=protected, original_inputs=originals, executables=executables, runs=runs, coverage=coverage, complete_trace_bytes=len(expected), query_sha256=sha(query), analytical_sha256=sha(run / 'analytical.bin'), compiler_warnings=0, reused_regression=prior['executables']['owned'], reused_regression_sha256=sha(regression_output), static_ranges=[{k: v for k, v in s.items() if k != 'instructions'} for s in static], private_static_sha256=sha(run / 'static-ranges.private.json'), editor=prior['editor'], editor_sha256=prior['editor_sha256'], editor_dependencies=prior['editor_dependencies'], editor_rebuilt=False, findings=[
        'Whole original ShaderApi030 slot25310025B50 executes unchanged RX, including original preparation helper entries, callback drain, optional restore helper and suppressed render-state submission. Dirty graphics state is quiescent; capability returns0 and primary suppression returns true. This is control-protocol parity, not GPU-preparation parity.',
        '1376 native/owned cases match every published-state result, saved configuration word and callback event against an independent analytical trace, twice per architecture. Cases include null/non-null geometry, signed count gates, changing mode query results, early config skip, restore mode, callback receiver replacement, alpha1/.5/quietNaN and raw32bit compression values.',
        'Material/mesh reads occur at distinct phases: format and material-alpha use captured receivers, while count and eventual material draw reread live state. The first mode query equals1 short-circuits; otherwise a second query must equal2. Borrowed instance alpha uses original argument pointer after setup, even if published state.instances was replaced.',
        'Native bool argument stack words retain unrelated upper bytes from pointer locals. Fixture observers interpret the meaningful low byte, and the owned protocol transports logical booleans rather than inventing widened pointer-containing scalar semantics.',
        'Successful cleanup clears published mesh/count/instance/metadata pointers and two flag bytes. Lighting-state word is reset at entry but is not overwritten by cleanup; callback writes remain. Early disabled configuration preserves prior state. Exceptions/overlapping nested instance scopes are not proven.',
        'Ops delegates actual dirty preparation, callback execution and render-state restore services. Mesh/material interfaces and final material71 draw remain observers. The original material shader, full dirty preparation, resource lifetimes/thread safety, queued cross-model behavior and final GPU output remain unverified. Do not activate the editor or generalize this to material-value snapshot equivalence.',
        'New header/probe/build/verifier only. All previous components, original inputs, editor313 dependency hashes and exact x64 binary remain unchanged. Prior3072 owned mesh submission cases rerun. No SFM process access/enumeration, original writes, dependency installation or publication.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(outcome, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=outcome['status'], receipt=str(receipt), coverage=coverage, complete_trace_bytes=len(expected), runs=len(runs), reused_regression_cases=3072, editor_rebuilt=False), indent=2), flush=True)

if __name__ == '__main__':
    main()
