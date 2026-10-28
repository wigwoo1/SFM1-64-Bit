"""Original connected material bind/flush/manager/pending-dispatch vs owned x64."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/shader-material-flush-composition-20261028'
SHADER = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def decode(data):
    cursor, records = 0, []
    while cursor < len(data):
        need(cursor + 44 <= len(data), 'complete trace header')
        header = struct.unpack_from('<11I', data, cursor)
        cursor += 44
        need(header[0] == len(records) and header[-1] <= 64 and cursor + header[-1] * 16 <= len(data), 'trace index/event bounds')
        events = list(struct.iter_unpack('<4I', data[cursor:cursor + header[-1] * 16]))
        cursor += header[-1] * 16
        records.append((header, events))
    return records

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/shader-api-material-binding-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == '505000877c239f13235f78e6ea914a672ec5179a5e9ddf54abe3c0db1e470ea9', 'accepted material-binding receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent)}
    for e in prior['executables'].values():
        protected[e['path']] = e['sha256']
    originals = dict(prior['original_inputs'])
    need(all(sha(p) == h for p, h in protected.items()), 'all old source/build artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()) and originals[str(SHADER)] == '07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa', 'pinned original/private inputs before')
    sources = closure([ROOT / 'work/probe_shader_material_flush_composition.cpp', ROOT / 'work/build_shader_material_flush_composition.cmd', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT / 'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    # Retain the older narrower fixed-endpoint component without changing it.
    legacy = ROOT / 'work/pending_draw_flush_candidate.h'
    sources[str(legacy)] = sha(legacy)
    image, static = check_ranges(SHADER, 'shaderapidx9-disassembly.txt', [
        ('whole_binding', 0x10013f30, 0x10013fcb),
        ('whole_flush', 0x10019a10, 0x10019a54),
        ('whole_manager_getter', 0x10005bc0, 0x10005bc6),
        ('whole_manager_pending_thunk', 0x10008310, 0x1000831b),
        ('whole_pending_dispatch', 0x10007f50, 0x10007fa0)])
    need([image.word(0x100fcedc + i * 4) for i in [83, 84]] == [0x10013f30, 0x10019a10] and image.word(0x100fbce4 + 5 * 4) == 0x10008310, 'actual interface bindings in connected chain')
    (run / 'static-ranges.private.json').write_text(json.dumps(static, indent=2) + '\n')
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_shader_material_flush_composition.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh connected original/owned builds')
    executables = {}
    for tag, name, machine, magic in [('reference', 'flush_composition_reference_x86.exe', 0x14c, 0x10b), ('owned', 'flush_composition_owned_x64.exe', 0x8664, 0x20b)]:
        exe = OUT / name
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', raw, pe + 4)[0] == machine and struct.unpack_from('<H', raw, pe + 24)[0] == magic and b'asInvoker' in raw, 'fresh architecture/manifest ' + tag)
        log = (OUT / (tag + '-build.log')).read_text()
        (run / (tag + '-build.log')).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', log, re.I), 'clean compile ' + tag)
        inspection = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        need(inspection.returncode == 0, 'imports ' + tag)
        (run / (tag + '-imports.txt')).write_text(inspection.stdout)
        imports = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', inspection.stdout, re.I | re.M)))
        need(imports == ['KERNEL32.dll'], 'system-only direct imports ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), machine=hex(machine), imports=imports)
    runs, accepted = [], None
    for tag in ['reference', 'owned', 'reference', 'owned']:
        output = run / (tag + '-' + str(len(runs)) + '.bin')
        args = [executables[tag]['path'], *([str(SHADER)] if tag == 'reference' else []), str(output)]
        result = subprocess.run(args, cwd=run, capture_output=True, text=True, timeout=90)
        (run / (output.stem + '.log')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, tag + ' exit=' + str(result.returncode) + '; inspect ' + output.stem + '.log')
        data = output.read_bytes()
        if accepted is None:
            accepted = data
        need(data == accepted, 'every native/owned event and final state matches ' + output.stem)
        need('PASS bind_flush_composition_cases=6912 events=25984 pending_draws=256 gate_calls=1600 reentrant_binds=128 independent_checks=28544' in result.stdout, 'independent combinatorial coverage/eligibility/restoration checks ' + tag)
        need(('target=OWNED_X64' if tag == 'owned' else 'original_code=UNCHANGED_RX') in result.stdout, 'owned/original code identity ' + tag)
        records = decode(data)
        need(len(records) == 6912 and sum(h[6] for h, _ in records) == 256 and sum(h[7] for h, _ in records) == 256 and sum(h[8] for h, _ in records) == 1600, 'complete decoded independent event/final-state totals')
        draw_events = [e for _, ev in records for e in ev if e[0] in [4, 5]]
        need(len(draw_events) == 256 and all(e[1:] == (1, 1, 0xaaa00001) for e in draw_events), 'all actual pending draw boundaries see previous material and its current synthetic value')
        # Fresh cleanup target and nested bind observations are recorded by
        # the real original dispatcher, not synthesized from owned results.
        cleanup = [e for _, ev in records for e in ev if e[0] == 8]
        need(len(cleanup) == 256 and {e[1] for e in cleanup} == {1, 2} and {e[2] for e in cleanup} == {1, 3}, 'cleanup rereads replaced endpoint and nested material binding')
        need(all(e[1] == 0 for _, ev in records for e in ev if e[0] == 9), 'all cleanup callbacks observe cleared guard/pending/alternate flags')
        runs.append(dict(tag=tag, output=str(output), sha256=sha(output), stdout=result.stdout.strip()))
    # The preceding whole binding protocol remains authenticated and runs all
    #4096 prior cases without replacing its accepted build or receipts.
    regression_output = run / 'prior-binding-owned.bin'
    regression = subprocess.run([prior['executables']['owned']['path'], str(regression_output)], cwd=run, capture_output=True, text=True, timeout=60)
    (run / 'prior-binding-owned.log').write_text(regression.stdout + regression.stderr)
    need(regression.returncode == 0 and sha(regression_output) == prior['analytical_sha256'], 'previous4096 binding cases preserved')
    need(all(sha(p) == h for p, h in sources.items()) and all(sha(p) == h for p, h in protected.items()), 'new/old sources and prior artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p, h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], 'editor313 dependencies and exact executable unchanged')
    need(all(str(ROOT / 'work' / name) not in prior['editor_dependencies'] for name in ['shader_api_material_flush_candidate.h', 'pending_draw_dispatch_candidate.h']), 'new helpers not activated in editor')
    outcome = dict(status='VERIFIED owned x64 material bind/flush/pending-dispatch composition; actual mesh/shader consumption INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', parent=str(parent), parent_sha256=sha(parent), source=sources, protected=protected, original_inputs=originals, executables=executables, runs=runs, coverage=dict(cases=6912, events=25984, pending_draws=256, gate_calls=1600, reentrant_binds=128, independent_checks=28544), complete_trace_bytes=len(accepted), compiler_warnings=0, reused_regression=prior['executables']['owned'], reused_regression_sha256=sha(regression_output), static_ranges=[{k: v for k, v in s.items() if k != 'instructions'} for s in static], private_static_sha256=sha(run / 'static-ranges.private.json'), editor=prior['editor'], editor_sha256=prior['editor_sha256'], editor_dependencies=prior['editor_dependencies'], editor_rebuilt=False, findings=[
        'Original binding10013F30 calls actual flush10019A10 through actual ShaderApi030 vtable, actual manager singleton getter10005BC0, actual manager-vtable slot5 thunk10008310 and actual pending draw dispatcher10007F50. These methods execute together unchanged RX, not a stitched trace of separately stubbed flush calls.',
        'Owned x64 composition preserves the optional service predicate, save-after-predicate/restore-after-dispatch order of two32bit fields, guard, alternate/ordinary draw routing, native -1/0 argument conventions, cleared flags before cleanup and rereading a replacement endpoint for cleanup.',
        '6912 cases cover same/equivalent/different binding, optional gate absent/block/allow, missing/present mesh, guard0/1, pending0/1/255, ordinary/alternate, all combinations of gate mutations, mesh replacement/draw-state mutations, reentrant binding and cleanup mutations.256 pending draws and128 nested binds execute per run; each native/owned pair matches completely twice.',
        'All pending draw observers see the previous material before the outer assignment. Nested binding can change the material observed by cleanup, and replacing the mesh during draw changes cleanup receiver. Guard suppresses duplicate nested drawing. The outer binding assignment still occurs last.',
        'Mesh draw/finish methods, material predicate/key methods and optional service predicate are controlled observers. Their synthetic reads do not prove actual mesh implementation shader-parameter consumption, GPU sampling, full queued StudioRender100502E0 behavior or SFM mode selection. No general material-value snapshot policy is admitted.',
        'Older fixed-endpoint PendingDrawFlushCandidate remains unchanged; new pointer-aware helper explicitly covers endpoint replacement omitted from that older fixture. Borrowed endpoint/metadata validity and serialized ownership remain caller preconditions; exceptions/concurrency are not newly guaranteed.',
        'No production editor integration, dependency/serialization/document changes, original writes, live SFM/process enumeration or publication. Editor313 dependencies and exact x64 binary remain unchanged. Prior4096 owned binding cases rerun successfully.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(outcome, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=outcome['status'], receipt=str(receipt), coverage=outcome['coverage'], runs=len(runs), complete_trace_bytes=len(accepted), reused_regression_cases=4096, editor_rebuilt=False), indent=2), flush=True)

if __name__ == '__main__':
    main()
