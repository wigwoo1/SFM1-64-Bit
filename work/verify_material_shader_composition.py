"""Connected original material71 -> system18 vs existing owned x64 helpers."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/material-shader-composition-20261028'
MATERIAL = ROOT / 'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def need(b, s):
    if not b:
        raise RuntimeError(s)

def corpus():
    import itertools
    rng, rows = random.Random(2026102851), []
    for mutation, matmutation, flags, available, forward in itertools.product(
            [0, 1, 14, 16, 32, 64, 128, 255], range(32),
            [0, 0x1000, 4, 0x1004], range(2), range(2)):
        i = len(rows)
        rows.append([i, [0, 1, 2, 254, 255, 256, 257, 510][(i//4)%8],
                     [0, 1, 0x100, 0xffffff00, 0xffffffff][(i//8)%5],
                     available, flags, mutation, (i//32)%4,
                     rng.getrandbits(32), rng.getrandbits(32), matmutation, 1, forward,
                     rng.getrandbits(32), rng.getrandbits(24)<<8])
    for _ in range(512):
        rows.append([len(rows), rng.randrange(512), rng.getrandbits(32), rng.randrange(2),
                     rng.getrandbits(32), rng.randrange(256), rng.randrange(4),
                     rng.getrandbits(32), rng.getrandbits(32), rng.randrange(32),
                     rng.randrange(2), rng.randrange(2), rng.getrandbits(32), rng.getrandbits(24)<<8])
    return rows


def prediction(rows):
    result, total_events, normal, callbacks, missing = bytearray(), 0, 0, 0, 0
    for row in rows:
        serial, selection, alpha, available, flags, mutation, cmode, encoded, compression, matmutation, present, forward, cookie, mode = row
        full = selection | int(bool(alpha & 255))
        target = encoded ^ cookie ^ ((0xf1e2d3c4 ^ 0x12345678) if matmutation & 4 else 0)
        context_keys = [target if cmode & 1 else target ^ 0xabcddcba] * 4
        dirty = [0x77 if cmode & 2 else 0] * 4
        current, index, flag, api, sid, lid = 2, 0xee, 0xa7, 1, int(bool(present)), 1
        array_for_state, block = [0, 1], serial
        pf, values = [flags, flags ^ 0x1000], [0x11110000+serial, 0x22220000+serial]
        events, selected = [], None
        outer = bool(present and not (flags & 4))
        if not present:
            events.append([14]); missing += 1
        if outer:
            events.append([13, 1]); callbacks += 1
            sid = 2 if matmutation & 1 else 1
            lid = 2 if matmutation & 2 else 1
            if matmutation & 4:
                cookie ^= 0xf1e2d3c4; encoded ^= 0x12345678
            if matmutation & 8:
                pf = [p ^ 0x1004 for p in pf]; values = [values[0]+7, values[1]+11]
            if matmutation & 16:
                array_for_state[0] = 1; block ^= 0xabcdef01
            events.append([1, 1])
            api = 2 if mutation & 1 else 1
            events.append([2, api, sid, lid, values[lid-1]])
            if mutation & 2:
                array_for_state[0] = 1
            if available:
                normal += 1
                flags_at_entry = pf[lid-1]
                events += [[3, flags_at_entry], [4, api]]
                if flags_at_entry & 0x1000:
                    events.append([5, api, 0])
                if mutation & 4:
                    pf[lid-1] ^= 0x1000
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
                if cid and context_keys[cid-1] != (encoded ^ cookie):
                    context_keys[cid-1], dirty[cid-1] = encoded ^ cookie, 1
                events.append([10, full, compression, cid, a*0x10000+index,
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
        header = [serial, int(outer and available), pf[lid-1], api, current, index, flag,
                  *[v for pair in zip(context_keys, dirty) for v in pair],
                  len(events), *array_for_state, *digests, int(outer), int(not present),
                  sid, lid, cookie, encoded, block, values[0]]
        result += struct.pack('<28I', *header)
        for event in events:
            result += struct.pack('<8I', *(event+[0]*(8-len(event))))
        total_events += len(events)
    return bytes(result), dict(cases=len(rows), normal_draws=normal, material_callbacks=callbacks,
                              missing_shaders=missing, missing_snapshots=callbacks-normal,
                              material_flag_skips=len(rows)-callbacks-missing,
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
    parent = ROOT / 'outputs/shader-system-normal-draw-20261028/verification-qe9xbp_8/results.json'
    need(sha(parent) == 'fa2817e1cc77d7716c39abc3a243fd4e708080210f217e00ddb3ccfb79f31ba1', 'accepted normal shader-system receipt')
    prior = json.loads(parent.read_text())
    protected = {**prior['protected'], **prior['source'], str(parent): sha(parent)}
    for e in prior['executables'].values():
        protected[e['path']] = e['sha256']
    originals = dict(prior['original_inputs'])
    need(all(sha(p) == h for p, h in protected.items()), 'prior source/artifacts unchanged before')
    need(all(sha(p) == h for p, h in originals.items()), 'original/private inputs unchanged before')
    sources = closure([ROOT/'work/probe_material_shader_composition.cpp',
                       ROOT/'work/build_material_shader_composition.cmd',
                       ROOT/'work/probe_shader_system_normal_draw.cpp',
                       ROOT/'work/test_studio_material_draw.cpp', Path(__file__)])
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and Path(path).resolve().parent == ROOT/'work' and Path(path).suffix == '.py':
            sources[str(Path(path).resolve())] = sha(path)
    image, static = check_ranges(MATERIAL, 'materialsystem-disassembly.txt', [
        ('whole_direct_material_draw',0x10004ae0,0x10004b5a),
        ('whole_forwarding_material_draw',0x1000a910,0x1000a921),
        ('whole_shader_system_draw', 0x10037d40, 0x10037f35),
        ('whole_diagnostic_drain_empty_profile', 0x10076bf0, 0x10076d7f),
        ('whole_buffer_terminate_external_profile', 0x1006eeb0, 0x1006eef2)])
    need(sha(MATERIAL) == 'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8' and image.word(0x1009c9dc+18*4) == 0x10037d40, 'original identity and actual slot18')
    need(image.word(0x10095d74+71*4)==0x10004ae0 and image.word(0x10095a9c+71*4)==0x1000a910,'actual direct/forwarded material slots')
    resolved = imports(image)
    expected_imports = {0x10095144:'LoggingSystem_PopLoggingState', 0x10095148:'LoggingSystem_PushLoggingState', 0x1009514c:'LoggingSystem_RegisterLoggingListener', 0x10095150:'LoggingSystem_SetLoggingResponsePolicy'}
    need(all(resolved[a] == ('tier0.dll', n) for a, n in expected_imports.items()), 'real import identities are logging, not locking')
    (run/'static-ranges.private.json').write_text(json.dumps(static, indent=2)+'\n')
    rows = corpus()
    query = run/'queries.bin'
    query.write_bytes(b''.join(struct.pack('<14I', *row) for row in rows))
    expected, coverage = prediction(rows)
    (run/'analytical.bin').write_bytes(expected)
    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT/'work/build_material_shader_composition.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr)
    need(built.returncode == 0, 'fresh native/owned/regression builds')
    executables = {}
    for tag, name, machine, magic, logname in [
        ('reference','material_shader_composition_reference_x86.exe',0x14c,0x10b,'reference-build.log'),
        ('owned','probe_material_shader_composition_x64.exe',0x8664,0x20b,'probe_material_shader_composition-build.log'),
        ('normal_regression','probe_shader_system_normal_draw_x64.exe',0x8664,0x20b,'probe_shader_system_normal_draw-build.log'),
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
        need(f'PASS material_shader_composition_cases={coverage["cases"]} events={coverage["events"]} independent_checks={coverage["independent_checks"]}' in result.stdout, 'exact live-state/gate coverage '+tag)
        runs.append(dict(tag=tag, output=str(trace), sha256=sha(trace), stdout=result.stdout.strip()))
    regression = subprocess.run([executables['regression']['path']], cwd=run, capture_output=True, text=True, timeout=60)
    (run/'existing-owned-regression.log').write_text(regression.stdout+regression.stderr)
    need(regression.returncode == 0 and 'material_draw_cases=8192' in regression.stdout, 'fresh8192 prior owned material tests')
    normal_trace=run/'fresh-normal-regression.bin'
    normal_regression=subprocess.run([executables['normal_regression']['path'],str(parent.parent/'queries.bin'),str(normal_trace)],cwd=run,capture_output=True,text=True,timeout=90)
    (run/'fresh-normal-regression.log').write_text(normal_regression.stdout+normal_regression.stderr)
    need(normal_regression.returncode==0 and sha(normal_trace)==prior['analytical_sha256'] and 'PASS shader_system_normal_cases=4608' in normal_regression.stdout,'fresh4608 independent normal-route regression')
    need(all(sha(p) == h for p,h in sources.items()) and all(sha(p) == h for p,h in protected.items()), 'all source/prior artifacts unchanged after')
    need(all(sha(p) == h for p,h in originals.items()), 'all original/private inputs unchanged after')
    need(all(sha(p) == h for p,h in prior['editor_dependencies'].items()) and sha(prior['editor']) == prior['editor_sha256'], '313 editor dependencies/exact binary unchanged')
    outcome = dict(status='VERIFIED connected material71 -> shader-system18 normal-route composition; real shader/GPU integration INCOMPLETE',
        utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),
        source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
        complete_trace_bytes=len(expected),query_sha256=sha(query),analytical_sha256=sha(run/'analytical.bin'),
        compiler_warnings=0,fresh_regression=regression.stdout,
        import_identities={hex(a):list(resolved[a]) for a in expected_imports},
        static_ranges=[{k:v for k,v in row.items() if k!='instructions'} for row in static],
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        fresh_normal_regression=normal_regression.stdout,
        findings=[
          'Whole original direct and forwarding material slot71 call actual ShaderSystem slot18 through original vtables/global receiver. Both execute unchanged RX instructions in the same private read-only-source image. There is no shader-system observer stub between these boundaries.',
          'Existing owned helpers compose with a stable material-owned embedded shader-state record. Material callback changes shader/list pointers, parameter flags and values, encoded token/cookie and embedded snapshot-array state before the actual system method consumes them. This proves this controlled fixture composition, not production material ownership.',
          'Material low-byte alpha becomes bool before the shader-system index OR. Full high-byte alpha values therefore do not enter the system selector through material71. Normal-mode lowbyte is0; all high24bits are preserved on the native material call, without triggering fallback.',
          'Missing shader and initial parameter bit4 gates stop before system entry. Setting bit4 during material callback does not gate again. Real system availability checks and normal context-key/dirty updates use the current decoded32bit XOR result. No semantic pointer type or immutable material-value snapshot inferred.',
          'Shader0/7/6, ShaderAPI and logging services are controlled observers. Debug/fallback rendering, nonempty diagnostic storage, actual shader resource binding, full production ownership/threading and GPU image fidelity remain unverified. Empty external diagnostic storage cleanup executes with nonzero sentinels.',
          'New probe/build/verifier only; existing material and normal-draw headers unchanged. Fresh prior normal4608 and material8192 regressions pass. Original files,313 editor dependencies and executable unchanged; no SFM process control/enumeration, dependency changes, editor admission or publication.'])
    receipt = run/'results.json'; receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected),editor_rebuilt=False),indent=2),flush=True)

if __name__ == '__main__':
    main()
