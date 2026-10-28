"""Normal/missing snapshot routes: actual original method vs independent x64."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/shader-system-normal-draw-20261028'
MATERIAL = ROOT / 'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def corpus():
    rng, rows = random.Random(2026102841), []
    selections = [0, 1, 2, 254, 255, 256, 257, 510]
    for mutation in range(256):
        for selection in selections:
            for available in range(2):
                i = len(rows)
                rows.append([i, selection, [0, 1, 256, 257][(i//2)%4], available,
                             [0, 0x1000, 0xffffffff, 8][(i//16)%4], mutation,
                             (i//8)%4, rng.getrandbits(32), rng.getrandbits(32)])
    for _ in range(512):
        rows.append([len(rows), rng.randrange(512), rng.randrange(512), rng.randrange(2),
                     rng.getrandbits(32), rng.randrange(256), rng.randrange(4),
                     rng.getrandbits(32), rng.getrandbits(32)])
    return rows

def prediction(rows):
    result, total_events, normal = bytearray(), 0, 0
    for serial, selection, alpha, available, flags, mutation, cmode, token, compression in rows:
        full = selection | alpha
        api = 2 if mutation & 1 else 1
        events = [[1, 1], [2, api]]
        array_for_state = [int(bool(mutation & 2)), 1]
        context_keys = [token if cmode & 1 else token ^ 0xabcddcba] * 4
        dirty = [0x77 if cmode & 2 else 0] * 4
        current, index, flag = 2, 0xee, 0xa7
        selected = None
        if available:
            normal += 1
            events += [[3, flags], [4, api]]
            if flags & 0x1000:
                events.append([5, api, 0])
            if mutation & 4:
                flags ^= 0x1000
            events += [[6, 1, 1], [7, 1], [8, 1]]
            index, flag, current = full & 255, 0, 1
            events.append([9, api, array_for_state[0]*1000 + index])
            current = 2 if mutation & 8 else 1
            index = (index + bool(mutation & 16)) & 255
            if mutation & 32:
                array_for_state[current-1] ^= 1
            if mutation & 64:
                api = 3-api
            a = array_for_state[current-1]
            cid = 0 if index % 5 == 4 else (index+a) % 4 + 1
            if cid and context_keys[cid-1] != token:
                context_keys[cid-1], dirty[cid-1] = token, 1
            marker = a*0x10000 + index
            events.append([10, full, compression, cid, marker,
                           context_keys[cid-1] if cid else 0, dirty[cid-1] if cid else 0, api])
            selected = a, index
            if mutation & 128:
                current, index, flag = 2, 77, 0xb5
            events += [[11, 1, current, index, flag], [12, 0, 0, index, flag]]
            current = 0
        digests = []
        for a in range(2):
            h = 2166136261
            for i in range(512):
                cid = 0 if i % 5 == 4 else (i+a) % 4 + 1
                marker = a*0x10000+i
                if selected == (a, i):
                    marker += 7
                    if mutation & 128:
                        cid = 4
                for word in [cid, marker]:
                    h = ((h ^ word)*16777619) & 0xffffffff
            digests.append(h)
        header = [serial, available, flags, api, current, index, flag,
                  *[v for pair in zip(context_keys, dirty) for v in pair],
                  len(events), *array_for_state, *digests]
        result += struct.pack('<20I', *header)
        for event in events:
            result += struct.pack('<8I', *(event+[0]*(8-len(event))))
        total_events += len(events)
    return bytes(result), dict(cases=len(rows), normal_draws=normal, missing=len(rows)-normal,
                              events=total_events, independent_checks=len(rows)+3*normal)

def imports(image):
    pe = image.u32(60)
    descriptor = image.offset(image.base + image.u32(pe+24+104))
    entries = {}
    while image.u32(descriptor):
        original, _, _, name, iat = struct.unpack_from('<5I', image.b, descriptor)
        dll = image.string(image.base+name)
        i = 0
        while (entry := image.word(image.base+(original or iat)+i*4)):
            entries[image.base+iat+i*4] = (dll, str(entry) if entry & 0x80000000 else image.string(image.base+entry+2))
            i += 1
        descriptor += 20
    return entries

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: '+str(run), flush=True)
    parent = ROOT / 'outputs/material-draw-reference-20261028/verification-0s9ar_i1/results.json'
    need(sha(parent) == '7385e7521b2cd03e8bd1e29ee0c6d39f9b30fa9934c8e6329b45637c668d12a6', 'accepted material draw receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent)}
    for e in prior['executables'].values():
        protected[e['path']] = e['sha256']
    originals = dict(prior['original_inputs'])
    need(all(sha(p) == h for p, h in protected.items()), 'prior source/artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()), 'original/private inputs unchanged before')
    sources = closure([ROOT/'work/probe_shader_system_normal_draw.cpp',
                       ROOT/'work/build_shader_system_normal_draw.cmd',
                       ROOT/'work/test_studio_material_draw.cpp', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT/'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    image, static = check_ranges(MATERIAL, 'materialsystem-disassembly.txt', [
        ('whole_shader_system_draw', 0x10037d40, 0x10037f35),
        ('whole_diagnostic_drain_empty_profile', 0x10076bf0, 0x10076d7f),
        ('whole_buffer_terminate_external_profile', 0x1006eeb0, 0x1006eef2)])
    need(sha(MATERIAL) == 'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8' and image.word(0x1009c9dc+18*4) == 0x10037d40, 'original identity and actual slot18')
    resolved = imports(image)
    expected_imports = {0x10095144:'LoggingSystem_PopLoggingState', 0x10095148:'LoggingSystem_PushLoggingState', 0x1009514c:'LoggingSystem_RegisterLoggingListener', 0x10095150:'LoggingSystem_SetLoggingResponsePolicy'}
    need(all(resolved[a] == ('tier0.dll', n) for a, n in expected_imports.items()), 'real import identities are logging, not locking')
    (run/'static-ranges.private.json').write_text(json.dumps(static, indent=2)+'\n')
    rows = corpus()
    query = run/'queries.bin'
    query.write_bytes(b''.join(struct.pack('<9I', *row) for row in rows))
    expected, coverage = prediction(rows)
    (run/'analytical.bin').write_bytes(expected)
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT/'work/build_shader_system_normal_draw.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr)
    need(built.returncode == 0, 'fresh native/owned/regression builds')
    executables = {}
    for tag, name, machine, magic, logname in [
        ('reference','shader_system_normal_reference_x86.exe',0x14c,0x10b,'reference-build.log'),
        ('owned','probe_shader_system_normal_draw_x64.exe',0x8664,0x20b,'probe_shader_system_normal_draw-build.log'),
        ('regression','test_studio_material_draw_x64.exe',0x8664,0x20b,'test_studio_material_draw-build.log')]:
        exe = OUT/name
        raw = exe.read_bytes(); pe = struct.unpack_from('<I', raw, 60)[0]
        need(exe.stat().st_mtime >= started-2 and struct.unpack_from('<H', raw, pe+4)[0] == machine and struct.unpack_from('<H', raw, pe+24)[0] == magic and b'asInvoker' in raw, 'fresh binary architecture/manifest '+tag)
        log = (OUT/logname).read_text(); (run/logname).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', log, re.I), 'clean compile '+tag)
        inspected = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        (run/(tag+'-imports.txt')).write_text(inspected.stdout)
        dlls = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', inspected.stdout, re.I|re.M)))
        need(inspected.returncode == 0 and dlls == ['KERNEL32.dll'], 'system-only binary imports '+tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), machine=hex(machine), imports=dlls)
    runs = []
    for tag in ['reference','owned','reference','owned']:
        trace = run/(tag+'-'+str(len(runs))+'.bin')
        result = subprocess.run([executables[tag]['path'], *([str(MATERIAL)] if tag == 'reference' else []), str(query), str(trace)], cwd=run, capture_output=True, text=True, timeout=90)
        (run/(trace.stem+'.log')).write_text(result.stdout+result.stderr)
        need(result.returncode == 0, tag+' exit='+str(result.returncode)+'; inspect '+trace.stem+'.log')
        actual = trace.read_bytes()
        if actual != expected:
            first = next((i for i,(a,b) in enumerate(zip(actual, expected)) if a != b), min(len(actual),len(expected)))
            raise RuntimeError(tag+' trace mismatch at byte '+str(first))
        need(f'PASS shader_system_normal_cases={coverage["cases"]} events={coverage["events"]} independent_checks={coverage["independent_checks"]}' in result.stdout, 'exact live-state/gate coverage '+tag)
        runs.append(dict(tag=tag, output=str(trace), sha256=sha(trace), stdout=result.stdout.strip()))
    regression = subprocess.run([executables['regression']['path']], cwd=run, capture_output=True, text=True, timeout=60)
    (run/'existing-owned-regression.log').write_text(regression.stdout+regression.stderr)
    need(regression.returncode == 0 and 'material_draw_cases=8192' in regression.stdout, 'fresh8192 prior owned material tests')
    need(all(sha(p) == h for p,h in sources.items()) and all(sha(p) == h for p,h in protected.items()), 'all source/prior artifacts unchanged after')
    need(all(sha(p) == h for p,h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p,h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], '313 editor dependencies/exact binary unchanged')
    outcome = dict(status='VERIFIED normal/missing snapshot route; full shader-system integration INCOMPLETE',
        utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),
        source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
        complete_trace_bytes=len(expected),query_sha256=sha(query),analytical_sha256=sha(run/'analytical.bin'),
        compiler_warnings=0,fresh_regression=regression.stdout,
        import_identities={hex(a):list(resolved[a]) for a in expected_imports},
        static_ranges=[{k:v for k,v in row.items() if k!='instructions'} for row in static],
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        findings=[
          'Whole original ShaderSystem slot18 executes unchanged RX code in a normal-route profile; debug overrides zero, mode argument0. Actual diagnostic cleanup bodies execute with empty external buffer. Shader/API/logging callbacks are observers, not actual shader/GPU execution.',
          'The decoded32bit token is compared with context word+4; a difference sets dirty byte+8 before updating that word. Equal tokens retain the prior dirty byte. Null context is accepted. Semantic token type, cross-thread ownership and actual context implementation remain unknown.',
          'Full32bit selected index OR alpha governs availability; later selection truncates to8bit and snapshot word to16bit. API93 can replace current state, index, snapshot array and API. Shader6 receives live context-pointer-cell and trailing-block addresses; its context replacement/tail writes are preserved.',
          'Logging imports were resolved by PE import descriptors to real tier0 names. No mutex/locking semantics inferred. Cached parameter flags govern API100 even if API6 changes live flags. Missing snapshot exits before flags/logging/final mode reset; normal cleanup clears current pointer but preserves callback-updated index/flag.',
          'New owned helper implements this normal-route protocol only. Debug/fallback dispatcher, nonempty diagnostic storage, actual shader callback consumption/resource binding, material-to-shader-system composition and full GPU fidelity remain unverified. No admission or integration into editor.',
          'No changes to prior modules, original installations/sessions, editor source dependencies or executable; no SFM process control/enumeration, dependency installation or publication.'])
    receipt = run/'results.json'; receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected),editor_rebuilt=False),indent=2),flush=True)

if __name__ == '__main__':
    main()
