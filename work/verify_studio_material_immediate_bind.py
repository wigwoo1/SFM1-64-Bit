"""Original whole immediate Bind/base helper vs owned x64 live-state protocol."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/studio-material-immediate-bind-20261028'
MATERIAL = ROOT / 'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def prediction():
    """Separate abstract state machine with normalized identity event records."""
    data = bytearray()
    state, events, cases, calls = 0x28202610, 0, 0, 0
    def word():
        nonlocal state
        state = (state * 1664525 + 1013904223) & 0xffffffff
        return state
    combinations = itertools.product(range(2), range(2), [0, 1, 2, 0xffffffff], range(2), range(2), range(3), range(8), range(2))
    for supplied, defaults, mode, debug, cached, initial, mutations, forward in combinations:
        token, values = word(), [word(), word(), word()]
        current, proxy, trace = initial + 1, 0xf0f0f0f0, []
        if supplied or defaults:
            source = 1 if supplied else 2
            selected = 2 if source == 2 or forward else 1
            if not supplied:
                trace.append((1, 0, 0))
            trace.append((3, source, 1))
            if debug:
                trace.append((4, selected, 0))
            if not supplied:
                trace.append((2, 0, 0))
            trace.append((3, source, 2))
            if current != selected:
                trace.append((5, selected, cached))
                if not cached:
                    trace.extend([(6, selected, 0), (7, 0, 0), (8, selected, 0), (9, selected, 0)])
                current = selected
            proxy = token
            trace.append((10, current, 0))
            if mutations & 1:
                current = 3
            trace.append((11, 0, mode))
            if mutations & 2:
                current = 2
            if mode == 0:
                trace.append((12, current, values[current - 1]))
                values[current - 1] = (values[current - 1] + 17) & 0xffffffff
                calls += 1
                if mutations & 4:
                    current = 1
            trace.append((13, current, values[current - 1]))
        data += struct.pack('<4I', cases, current, proxy, len(trace))
        for event in trace:
            data += struct.pack('<3I', *event)
        cases += 1
        events += len(trace)
    return bytes(data), dict(cases=cases, events=events, proxy_calls=calls, checks=cases * 2 + 2304 * 2 + calls)

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/studio-draw-capture-reference-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == '852fc0d6cbb306dfa066a323829ef3f596160f2ba3c83b7cf8673f3a1b0b3d6d', 'accepted queued-copy receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent), prior['executable']['path']: prior['executable']['sha256']}
    originals = dict(prior['original_inputs'])
    originals[str(MATERIAL)] = 'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8'
    need(all(sha(p) == h for p, h in protected.items()), 'preceding source/artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()), 'pinned original/private inputs unchanged before')
    sources = closure([ROOT / 'work/probe_studio_material_immediate_bind.cpp', ROOT / 'work/build_studio_material_immediate_bind.cmd', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT / 'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    image, static = check_ranges(MATERIAL, 'materialsystem-disassembly.txt', [
        ('whole_immediate_context_bind', 0x100265e0, 0x1002669c),
        ('whole_base_bind_helper', 0x10025dd0, 0x10025e70),
        ('actual_material_pointer_setter', 0x10009f80, 0x10009f8d),
        ('actual_material_pointer_getter', 0x1002e490, 0x1002e494),
        ('actual_proxy_data_setter', 0x10012ab0, 0x10012abd)])
    need([image.word(0x1009a6ac + i * 4) for i in [9, 198, 199, 221]] == [0x100265e0, 0x10009f80, 0x1002e490, 0x10012ab0], 'actual immediate context vtable methods')
    (run / 'static-ranges.private.json').write_text(json.dumps(static, indent=2) + '\n')
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_studio_material_immediate_bind.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh original-reference and owned-x64 builds; inspect build-driver.log')
    executables = {}
    for tag, name, machine, magic in [('reference', 'immediate_bind_reference_x86.exe', 0x14c, 0x10b), ('owned', 'immediate_bind_owned_x64.exe', 0x8664, 0x20b)]:
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
        need(imports == ['KERNEL32.dll'], 'system-only direct imports ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), machine=hex(machine), imports=imports)
    expected, counts = prediction()
    need(counts['cases'] == 3072 and counts['proxy_calls'] == 576 and counts['checks'] == 11328, 'complete analytical case coverage')
    (run / 'analytical.bin').write_bytes(expected)
    runs = []
    for tag in ['reference', 'owned', 'reference', 'owned']:
        record = run / (tag + '-' + str(len(runs)) + '.bin')
        args = [executables[tag]['path'], *([str(MATERIAL)] if tag == 'reference' else []), str(record)]
        result = subprocess.run(args, cwd=run, capture_output=True, text=True, timeout=90)
        (run / (record.stem + '.log')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, tag + ' exit=' + str(result.returncode) + '; inspect ' + record.stem + '.log')
        need(record.read_bytes() == expected, 'every native/owned event, final material and proxy-data word vs independent model ' + record.stem)
        need(f'PASS immediate_bind_cases={counts["cases"]} events={counts["events"]} independent_checks={counts["checks"]}' in result.stdout, 'runtime analytical counts ' + tag)
        need(('wide_ids=PASS' if tag == 'owned' else 'original_code=UNCHANGED_RX') in result.stdout, 'native RX / owned full-width identity checks ' + tag)
        runs.append(dict(tag=tag, path=str(record), sha256=sha(record), stdout=result.stdout.strip()))
    need(all(sha(p) == h for p, h in protected.items()) and all(sha(p) == h for p, h in sources.items()), 'all source and prior artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p, h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], 'editor313 source closure and executable unchanged')
    need(str(ROOT / 'work/studio_material_immediate_bind_candidate.h') not in prior['editor_dependencies'], 'new protocol is not yet integrated into editor')
    outcome = dict(status='VERIFIED owned x64 immediate material Bind protocol; full shader/material snapshot and editor integration INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', parent=str(parent), parent_sha256=sha(parent), source=sources, protected=protected, original_inputs=originals, executables=executables, runs=runs, analytical=counts, analytical_sha256=sha(run / 'analytical.bin'), complete_trace_bytes=len(expected), compiler_warnings=0, static_ranges=[{k: v for k, v in s.items() if k != 'instructions'} for s in static], private_static_sha256=sha(run / 'static-ranges.private.json'), editor=prior['editor'], editor_sha256=prior['editor_sha256'], editor_dependencies=prior['editor_dependencies'], editor_rebuilt=False, findings=[
        'Entire original immediate context Bind100265E0/base10025DD0 and original material getter/setter/proxy-data setter execute unchanged RX in separate private x86 child. Owned protocol uses pointer-width IDs and no original runtime.',
        '3072 cases cover supplied/null/default availability, mode0/1/2/FFFFFFFF, debug, precached/uncached, three initial materials, forwarding identity, every combination of context replacement at flush/mode/proxy callbacks. Every event and final state equals an independent analytical prediction in repeated native and owned runs.',
        'The binding helper stores the live resolved material pointer and proxy-data token. It does not retain a parameter value snapshot. Material resolution occurs twice, including the diagnostic lookup. Null without a default exits before callbacks; null with a default diagnoses in both wrapper and base helper.',
        'Exact system slot14 return zero causes current material slot53 to run after flush; nonzero skips it. Shader API slot83 receives the current material read again AFTER the proxy callback. Callback mutation of live material identity is preserved. These numerical modes are not assigned enum names.',
        'Material methods, flush, system slot14 and shader API slot83 are controlled boundary observers. Synthetic material values seen at that boundary are NOT original shader parameter snapshots, GPU rendering or actual material proxy execution. Exceptions/ownership/stale pointers/thread-safety have no added guarantee.',
        'Need determine actual SFM system slot14 mode at queued consumption and recover shader API slot83/downstream parameter capture before choosing per-prebind versus final/shared-state policy or connecting to renderer. Full StudioRender draw100502E0 is not executed here.',
        'No SFM process launch/control/enumeration, original writes, production renderer/document/serialization changes, editor rebuild or publication. Earlier sources, artifacts and original inputs remain unchanged.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(outcome, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=outcome['status'], receipt=str(receipt), analytical=counts, complete_trace_bytes=len(expected), runs=len(runs), editor_rebuilt=False), indent=2), flush=True)

if __name__ == '__main__':
    main()
