"""Whole original ShaderApi030 slot83 vs owned pointer-width binding state."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/shader-api-material-binding-20261028'
SHADER = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def prediction():
    result, seed = bytearray(), 0x20261029
    cases = events_total = flush_total = retained = checks = 0
    def word():
        nonlocal seed
        seed = (1664525 * seed + 1013904223) & 0xffffffff
        return seed
    for initial, desired, mask, equal, mutations in itertools.product(range(4), range(4), range(16), range(2), range(8)):
        value, values = word(), [word(), word(), word()]
        current, predicates, keys, flushes = initial, 0, 0, 0
        trace = []
        def predicate(material):
            nonlocal current, predicates
            flag = (mask >> predicates) & 1
            predicates += 1
            trace.append((1, material, flag))
            if mutations & 1:
                current = 3
            return flag
        def key(material):
            nonlocal current, keys
            answer = value ^ (1 if keys and not equal else 0)
            keys += 1
            trace.append((2, material, answer))
            if mutations & 2:
                current = 2
            return answer
        def replace():
            nonlocal current, flushes
            trace.append((3, current, 0))
            flushes += 1
            if mutations & 4:
                current = 1
            current = desired
        compare_keys = bool(initial and desired and predicate(initial) and predicate(desired))
        if compare_keys:
            left, right = key(current), key(desired)
            if left != right:
                replace()
        else:
            live = current
            if live != desired or (live and predicate(live)) or (desired and predicate(desired)):
                replace()
        live_value = (values[current - 1] + 101) & 0xffffffff if current else 0
        checks += 1 + bool(current)
        if initial and desired and initial != desired and not mutations and mask & 3 == 3 and equal:
            need(current == initial and flushes == 0, 'analytical distinct-key equivalence retention')
            retained += 1
            checks += 1
        result += struct.pack('<7I', cases, current, predicates, keys, flushes, live_value, len(trace))
        for event in trace:
            result += struct.pack('<3I', *event)
        events_total += len(trace)
        flush_total += flushes
        cases += 1
    return bytes(result), dict(cases=cases, events=events_total, flushes=flush_total, retained_distinct_same_key=retained, checks=checks)

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/studio-material-immediate-bind-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == 'f6105f95d10704b75303f85317a137a56503b3707fb734b08b5f024dd0f0397a', 'accepted immediate material Bind receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent)}
    for executable in prior['executables'].values():
        protected[executable['path']] = executable['sha256']
    originals = dict(prior['original_inputs'])
    originals[str(SHADER)] = '07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
    need(all(sha(p) == h for p, h in protected.items()), 'prior source and tested binaries unchanged before')
    need(all(sha(p) == h for p, h in originals.items()), 'pinned original/private inputs unchanged before')
    sources = closure([ROOT / 'work/probe_shader_api_material_binding.cpp', ROOT / 'work/build_shader_api_material_binding.cmd', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT / 'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    image, static = check_ranges(SHADER, 'shaderapidx9-disassembly.txt', [
        ('complete_shader_material_bind', 0x10013f30, 0x10013fcb),
        ('complete_cached_material_getter', 0x10013fd0, 0x10013fd7),
        ('ShaderApi030_factory', 0x1000df20, 0x1000df26),
        ('constructor_interface_vtable_store', 0x10027275, 0x1002727f)])
    need(image.word(0x100fcedc + 83 * 4) == 0x10013f30 and image.word(0x100fcedc + 84 * 4) == 0x10019a10, 'observed ShaderApi030 Bind/flush vtable entries')
    (run / 'static-ranges.private.json').write_text(json.dumps(static, indent=2) + '\n')
    expected, counts = prediction()
    need(counts['cases'] == 4096 and counts['retained_distinct_same_key'] == 24, 'analytical corpus and pointer retention cases')
    (run / 'analytical.bin').write_bytes(expected)
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_shader_api_material_binding.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh native reference / owned x64 builds')
    executables = {}
    for tag, name, machine, magic in [('reference', 'shader_material_binding_reference_x86.exe', 0x14c, 0x10b), ('owned', 'shader_material_binding_owned_x64.exe', 0x8664, 0x20b)]:
        exe = OUT / name
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', raw, pe + 4)[0] == machine and struct.unpack_from('<H', raw, pe + 24)[0] == magic and b'asInvoker' in raw, 'fresh machine/manifest ' + tag)
        log = (OUT / (tag + '-build.log')).read_text()
        (run / (tag + '-build.log')).write_text(log)
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
        args = [executables[tag]['path'], *([str(SHADER)] if tag == 'reference' else []), str(output)]
        executed = subprocess.run(args, cwd=run, capture_output=True, text=True, timeout=90)
        (run / (output.stem + '.log')).write_text(executed.stdout + executed.stderr)
        need(executed.returncode == 0, tag + ' exit=' + str(executed.returncode) + '; inspect ' + output.stem + '.log')
        need(output.read_bytes() == expected, 'all normalized events, cache fields and later synthetic live values vs independent model ' + output.stem)
        need(f'PASS shader_api_material_binding_cases={counts["cases"]} events={counts["events"]} flushes={counts["flushes"]} retained_distinct_same_key={counts["retained_distinct_same_key"]} independent_checks={counts["checks"]}' in executed.stdout, 'exact coverage/independent assertion counts ' + tag)
        need(('wide_ids=PASS' if tag == 'owned' else 'original_code=UNCHANGED_RX') in executed.stdout, 'wide IDs/original code integrity ' + tag)
        runs.append(dict(tag=tag, output=str(output), sha256=sha(output), stdout=executed.stdout.strip()))
    # Keep the previously proved immediate context protocol covered without
    # rebuilding or overwriting its accepted executable/receipts.
    regression_output = run / 'previous-immediate-bind-owned.bin'
    regression = subprocess.run([prior['executables']['owned']['path'], str(regression_output)], cwd=run, capture_output=True, text=True, timeout=60)
    (run / 'previous-immediate-bind-owned.log').write_text(regression.stdout + regression.stderr)
    need(regression.returncode == 0 and sha(regression_output) == prior['analytical_sha256'], 'prior3072 immediate Bind cases preserved')
    need(all(sha(p) == h for p, h in protected.items()) and all(sha(p) == h for p, h in sources.items()), 'all sources and old artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p, h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], 'editor313 dependencies and executable unchanged')
    need(str(ROOT / 'work/shader_api_material_binding_candidate.h') not in prior['editor_dependencies'], 'new binding component not activated in editor')
    outcome = dict(status='VERIFIED owned x64 ShaderApi030 material-pointer binding; downstream flush/draw value capture INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', parent=str(parent), parent_sha256=sha(parent), source=sources, protected=protected, original_inputs=originals, executables=executables, runs=runs, analytical=counts, analytical_sha256=sha(run / 'analytical.bin'), complete_trace_bytes=len(expected), compiler_warnings=0, reused_regression=prior['executables']['owned'], reused_regression_sha256=sha(regression_output), static_ranges=[{k: v for k, v in s.items() if k != 'instructions'} for s in static], private_static_sha256=sha(run / 'static-ranges.private.json'), editor=prior['editor'], editor_sha256=prior['editor_sha256'], editor_dependencies=prior['editor_dependencies'], editor_rebuilt=False, findings=[
        'Whole original ShaderApi030 slot83 method10013F30 and direct cached-field getter body10013FD0 execute unchanged RX privately. Getter public vtable membership is unverified; no slot number is assigned. Constructor interface-vtable store and factory return are current-image byte-checked static evidence, not a live service connection.',
        '4096 cases cover null/same/distinct identities, predicate result sequences through four calls, equal/different32bit keys and every combination of predicate/key/flush callback changes to cached identity. Repeated native and owned traces exactly match an independent analytical model.',
        'When old/new predicates both succeed and compared keys match, original returns without flushing OR assigning the requested material.24 static cases retain a distinct previous identity; callbacks can also change which identity remains. Never replace this with unconditional cache=requested or pointer-only equality.',
        'Actual method accesses material predicates/keys, calls flush slot84 as needed and assigns a raw material pointer afterward. Other owner bytes and synthetic material object bytes remain unchanged in the native fixture. It creates no material-parameter value snapshot at this boundary.',
        'Opaque material IDs and cached state are pointer-width in owned implementation; compared key remains explicit32bit as observed in native EAX comparison. Key/predicate semantic meaning is unverified and not named as a material/shader format.',
        'Original material predicates/keys and flush slot84 are boundary observers. Real flush implementation and downstream draw/shader parameter capture, actual SFM mode selection, resource ownership/threading/exception behavior and combined StudioRender draw remain unverified. Synthetic later pointee observation is not GPU shader sampling.',
        'New component is not connected to the production editor. Editor source313 dependencies, binary, old sources and original inputs unchanged; earlier3072 owned immediate Bind cases rerun. No live SFM access/enumeration, original writes, new dependency or publication.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(outcome, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=outcome['status'], receipt=str(receipt), analytical=counts, runs=len(runs), complete_trace_bytes=len(expected), reused_regression_cases=3072, editor_rebuilt=False), indent=2), flush=True)

if __name__ == '__main__':
    main()
