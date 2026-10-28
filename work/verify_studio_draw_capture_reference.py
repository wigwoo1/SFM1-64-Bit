"""Offline original StudioRender queue argument copying; no live SFM or GPU."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/studio-draw-capture-reference-20261028'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
STUDIO = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/model-material-draw-mapping-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == '78104e6c892e8584aa4871abb8b17cb32f18bf53a6e0cf20ffb0aa5cba0756d4', 'accepted preceding material mapping receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['sources']}
    protected[str(parent)] = sha(parent)
    for e in prior['executables'].values():
        protected[e['path']] = e['sha256']
    originals = dict(prior['original_inputs'])
    need(sha(STUDIO) == originals[str(STUDIO)] == 'dc7eef7620f2a687c3cdd4182f2f17f0d78890e2748abcbe923b459b5fa340f7', 'pinned original StudioRender')
    need(all(sha(p) == h for p, h in protected.items()), 'previous sources and artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()), 'read-only originals unchanged before')
    sources = closure([ROOT / 'work/probe_studio_draw_capture_reference.cpp', ROOT / 'work/build_studio_draw_capture_reference.cmd', Path(__file__), ROOT / 'work/pe_image.py'])
    # Include the actual imported verification helpers, including transitive
    # Python dependencies; C/C++ include discovery alone does not cover them.
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT / 'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    ranges = [
        ('allocator_forwarder', 0x10001860, 0x10001877),
        ('queued_draw_submit', 0x10057080, 0x100570d8),
        ('queued_draw_constructor', 0x100552f0, 0x10055367),
        ('inline_state_copy', 0x10045070, 0x1004524e),
        ('queued_draw_invoke', 0x100553b0, 0x100553db),
        ('caller_prebind_then_enqueue', 0x100574aa, 0x1005751f)]
    image, static = check_ranges(STUDIO, 'studiorender-disassembly.txt', ranges)
    need([image.word(0x100712b8 + i * 4) for i in range(3)] == [0x10055370, 0x10055380, 0x100553b0], 'actual constructor functor vtable invoke binding')
    (run / 'static-ranges.private.json').write_text(json.dumps(static, indent=2) + '\n')
    started = time.time()
    result = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_studio_draw_capture_reference.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run / 'build-driver.log').write_text(result.stdout + result.stderr)
    need(result.returncode == 0, 'fresh reference build; inspect build-driver.log')
    exe = OUT / 'studio_draw_capture_reference_x86.exe'
    raw = exe.read_bytes()
    pe = struct.unpack_from('<I', raw, 60)[0]
    need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', raw, pe + 4)[0] == 0x14c and struct.unpack_from('<H', raw, pe + 24)[0] == 0x10b and b'asInvoker' in raw, 'fresh non-elevated I386 PE32 REFERENCE identity')
    log = (OUT / 'reference-build.log').read_text()
    (run / 'reference-build.log').write_text(log)
    need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', log, re.I), 'clean reference compile')
    inspected = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
    need(inspected.returncode == 0, 'reference import inspection')
    (run / 'reference-imports.txt').write_text(inspected.stdout)
    imports = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', inspected.stdout, re.I | re.M)))
    need(imports == ['KERNEL32.dll'], 'system-only direct reference imports')
    records = []
    expected = [f'{i} copied=2152 scalar_words=8 shared_pointers=4 delayed_invokes=2 packet_unchanged=1' for i in range(512)] + ['null_allocation forwarded=1 real_queue_recovery=UNVERIFIED']
    for repeat in range(2):
        output = run / f'capture-{repeat}.txt'
        executed = subprocess.run([str(exe), str(STUDIO), str(output)], cwd=run, capture_output=True, text=True, timeout=60)
        (run / f'capture-{repeat}.log').write_text(executed.stdout + executed.stderr)
        need(executed.returncode == 0, f'reference child {repeat} exit={executed.returncode}; inspect capture log')
        need(output.read_text().splitlines() == expected, 'complete two-functor/source-mutation/alias/order corpus')
        need('PASS studio_draw_capture_cases=512 queued_functors=1024 delayed_invocations=1024 copied_bytes_checked=1101824 independent_checks=7681 null_forwarding=1 original_code=UNCHANGED_RX material_value_snapshot=UNVERIFIED renderer=NOT_RUN' in executed.stdout, 'independent native argument/copy/identity assertions')
        records.append(dict(path=str(output), sha256=sha(output), stdout=executed.stdout.strip()))
    need(records[0]['sha256'] == records[1]['sha256'], 'repeatable ASLR-independent complete observation records')
    # Reuse, authenticate and execute the current owned x64 identity-mapping
    # safety test. No old artifact is rebuilt/overwritten for this reference task.
    owned = prior['executables']['test_model_material_draw_mapping']
    regression = subprocess.run([owned['path']], cwd=run, capture_output=True, text=True, timeout=60)
    (run / 'owned-mapping-regression.txt').write_text(regression.stdout + regression.stderr)
    need(regression.returncode == 0 and regression.stdout == prior['safety'], 'all owned mapping179/allocation71 cases preserved')
    need(all(sha(p) == h for p, h in sources.items()) and all(sha(p) == h for p, h in protected.items()), 'new/previous sources and previous build artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all originals unchanged after')
    need(all(sha(p) == h for p, h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], 'editor313 dependencies and exact executable unchanged')
    outcome = dict(status='VERIFIED_REFERENCE_ONLY queued argument copying; material parameter capture/rendering INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', parent=str(parent), parent_sha256=sha(parent), executable=dict(path=str(exe), sha256=sha(exe), target='I386 PE32 isolated original REFERENCE; not application runtime', imports=imports), source=sources, protected=protected, original_inputs=originals, static_ranges=[{k: v for k, v in s.items() if k != 'instructions'} for s in static], private_static_evidence_sha256=sha(run / 'static-ranges.private.json'), runs=records, compiler_warnings=0, reused_x64_regression=owned, owned_regression=regression.stdout, editor=prior['editor'], editor_sha256=prior['editor_sha256'], editor_dependencies=prior['editor_dependencies'], editor_rebuilt=False, findings=[
        'Whole original enqueue10057080, allocator-forwarder10001860, constructor100552F0, inline-copy10045070 and invoke100553B0 executed without original instruction patches; no DLL entry/import initialization.',
        'Constructor captures two inline blocks: 0x1E0 and0x254 bytes, a scalar, a two-word pair, flags and receiver/method identities. Caller overwrites all source buffers/scalars before replay; captured copies remain exact on finite synthetic float fields.',
        'Pointer-address words copied inside the blocks remain shallow: later pointee changes are visible to the controlled replay callback, including two queued calls sharing those pointees. No deep material-variable snapshot is performed by these helpers.',
        'This does NOT prove which actual fields are materials, what100502E0 or downstream render-context Bind captures, or whether real queue operations insert parameter snapshots elsewhere. Do not adopt per-visit/final-state material rendering policy yet.',
        'Allocation failure forwards null to a controlled queue observer. The real queue cannot be assumed safe; no null packet is drained, no native recovery or native destructor/refcount/lifetime parity is claimed.',
        'Actual SFM runtime objects, thread scheduling, resource lifetime, source destruction, NaNs/alternate FP modes, GPU draw and editor integration remain unverified. Test reference uses original x86 code privately; production editor remains independent x64.',
        'No SFM process launch/control/enumeration, original writes, production/document/serialization/material changes or publication. Earlier 179 owned mapping safety checks and71 allocation cases rerun unchanged.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(outcome, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=outcome['status'], receipt=str(receipt), reference_cases_per_run=512, reference_runs=2, copied_bytes_per_run=1101824, independent_checks_per_run=7681, reused_x64_checks=179, editor_rebuilt=False), indent=2), flush=True)

if __name__ == '__main__':
    main()
