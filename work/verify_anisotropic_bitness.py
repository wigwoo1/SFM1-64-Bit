"""Private owned x86/x64 D3D9 discriminator; never launches original SFM."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, math, os, re, struct, subprocess, tempfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/anisotropic-bitness-20261101'
VS = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build')

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b, message):
    if not b: raise RuntimeError(message)
def put(p, value): Path(p).write_text(json.dumps(value, indent=2)+'\n')
def pe(p):
    b = Path(p).read_bytes(); at = struct.unpack_from('<I', b, 60)[0]
    need(b[:2] == b'MZ' and b[at:at+4] == b'PE\0\0', 'PE framing')
    return struct.unpack_from('<H', b, at+4)[0], struct.unpack_from('<H', b, at+24)[0]

def query_rows(path):
    b = path.read_bytes(); count, = struct.unpack_from('<I', b); at = 4; rows = []
    for _ in range(count):
        q = struct.unpack_from('<10I', b, at); at += 40
        strings = []
        for _ in range(2):
            n, = struct.unpack_from('<I', b, at); at += 4
            strings.append(b[at:at+n].decode('ascii')); at += n
        n, = struct.unpack_from('<I', b, at); at += 4
        requests = b[at:at+32*n]; at += 32*n
        need(len(requests) == 32*n, 'complete request inputs')
        rows.append((q, strings[1], n))
    need(at == len(b), 'complete query input')
    return rows

def samples(path, rows):
    b = path.read_bytes(); at = 0; all_values = []
    for serial, (_, _, n) in enumerate(rows):
        need(struct.unpack_from('<2I', b, at) == (serial, n), 'sample framing'); at += 8
        values = struct.unpack_from('<'+'f'*4*n, b, at); at += n*16
        need(all(math.isfinite(v) and -1e-6 <= v <= 1+1e-6 for v in values), 'finite normalized pixels')
        all_values.extend(values)
    need(at == len(b), 'complete readback')
    return all_values

def sampler_states(path, rows, mag_aniso):
    b = path.read_bytes(); at = 0
    for serial, (q, _, _) in enumerate(rows):
        need(struct.unpack_from('<I', b, at)[0] == serial, 'sampler case'); at += 4
        factor, = struct.unpack('<f', struct.pack('<I', q[8]))
        address = lambda x: 4 if x == 3 else 3 if x == 2 else 1
        expected = (address(q[4]), address(q[5]), address(q[4]), 0,
                    3 if factor and mag_aniso else 2 if q[2] else 1,
                    3 if factor else 2 if q[1] else 1,
                    2 if q[3] else 1, 0, q[9], int(factor) if factor else 1,
                    q[0], 0, 0)
        for _ in range(2):
            actual = struct.unpack_from('<13I', b, at); at += 52
            need(actual == expected, f'actual sampler contract case {serial}: {actual} != {expected}')
    need(at == len(b), 'complete sampler capture')

def main():
    before = json.loads((OUT/'before.json').read_text()); parent = before['parent']
    need(sha(parent['receipt']) == parent['receipt_sha256'], 'parent receipt')
    milestone = json.loads(Path(parent['receipt']).read_text())
    transition = before['transition']; source = Path(transition['path'])
    need(sha(transition['backup']) == transition['before_sha256'] == before['source_sha256'][str(source)], 'preserved before source')
    transition['after_sha256'] = sha(source)
    sources = dict(before['source_sha256']); sources[str(source)] = sha(source)
    sources[str(Path(__file__).resolve())] = sha(__file__)
    protected = dict(before['protected_sha256'])
    protected[parent['receipt']] = parent['receipt_sha256']
    protected[transition['backup']] = transition['before_sha256']
    for group in (sources, protected):
        for p, h in group.items(): need(sha(p) == h, 'frozen identity '+p)
    run = Path(tempfile.mkdtemp(prefix='comparison-', dir=OUT)); print(str(run), flush=True)
    gradient = Path(milestone['verifications']['gradient']['receipt']).parent
    lod = Path(milestone['verifications']['lod']['receipt']).parent
    cases = [('gradient-'+str(i), gradient/f'queries-{i}.bin', gradient/f'd3d9-0-{i}'/'samples.bin') for i in range(6)]
    cases.append(('solid-mips', lod/'queries.bin', lod/'d3d9-0'/'samples.bin'))
    inputs = {}
    parsed = {}
    for label, query, prior in cases:
        for p in (query, prior):
            need(str(p) in protected and sha(p) == protected[str(p)], 'checkpointed original input/readback')
            inputs[str(p)] = sha(p)
        parsed[label] = query_rows(query)
        for _, path, _ in parsed[label]:
            need(path in protected and sha(path) == protected[path], 'owned checkpointed fixture')
            inputs[path] = sha(path)
    def execute(args, label):
        result = subprocess.run(list(map(str, args)), cwd=ROOT, capture_output=True, text=True,
                                timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(result.stdout+result.stderr)
        need(result.returncode == 0, label+' failed: '+(result.stdout+result.stderr)[-1600:])
        return result.stdout+result.stderr
    lines = ['@echo off', f'cd /d "{ROOT}"']
    for bits in (64, 32):
        target = run/f'x{bits}'; target.mkdir()
        tool = VS/('vcvars64.bat' if bits == 64 else 'vcvars32.bat')
        define = '' if bits == 64 else '/DSFM64_OWNED_D3D9_X86_RESEARCH=1'
        lines += ['setlocal', f'call "{tool}" >nul', 'if errorlevel 1 exit /b 1',
                  f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 {define} "{source}" /Fo:"{target/"d3d9.obj"}" /Fe:"{target/"d3d9.exe"}" d3d9.lib d3dcompiler.lib user32.lib >"{target/"build.log"}" 2>&1',
                  'if errorlevel 1 exit /b 1', 'endlocal']
    batch = run/'build.cmd'; batch.write_text('\n'.join(lines+['exit /b 0', '']))
    execute(['cmd.exe', '/d', '/c', batch], 'build-driver')
    executables = {}; runtime = {}; traces = {}; shader_hashes = {}; observations = []
    for bits in (64, 32):
        exe = run/f'x{bits}'/'d3d9.exe'
        need(pe(exe) == ((0x8664, 0x20b) if bits == 64 else (0x14c, 0x10b)), 'actual executable bitness')
        log = (exe.parent/'build.log').read_text()
        need(not re.findall(r'warning [A-Z]+\d+', log), 'warning-free owned probe')
        executables[str(bits)] = {'path': str(exe), 'sha256': sha(exe), 'pe_machine_optional_magic': pe(exe)}
        for repeat in range(2):
            for label, query, prior in cases:
                target = run/f'x{bits}-{repeat}-{label}'
                log = execute([exe, query, target, 4318, 11544], f'x{bits}-{repeat}-{label}')
                need(f'pointer_bits={bits}' in log and 'original_runtime=ABSENT' in log, 'explicit probe provenance')
                caps = re.search(r'D3D9 max_aniso=(\d+) min_aniso=(\d+) mag_aniso=(\d+)', log)
                need(caps is not None and tuple(map(int, caps.groups())) == (16, 1, 0), 'observed adapter capabilities')
                meta = dict(line.split('=', 1) for line in (target/'device.txt').read_text().splitlines())
                need(meta['vendor'] == '4318' and meta['device'] == '11544' and meta['pointer_bits'] == str(bits) and meta['window_visible'] == '0', 'same hidden adapter/bitness')
                need(bits not in runtime or runtime[bits]['metadata'] == meta, 'stable runtime/device identity')
                modules = {}
                for name in ('d3d9.dll', 'd3dcompiler_47.dll'):
                    reported = Path(meta[name]); windows = Path(os.environ['WINDIR'])
                    need(reported.is_absolute() and reported.is_relative_to(windows), 'system DLL location')
                    # A WOW64 process may report System32 while its loader resolves
                    # the image through SysWOW64. This observer is native 64-bit.
                    relative = reported.relative_to(windows)
                    p = windows/'SysWOW64'/Path(*relative.parts[1:]) if bits == 32 and relative.parts[0].lower() == 'system32' else reported
                    loaded = int(meta[name+'_loaded_machine']), int(meta[name+'_loaded_optional_magic'])
                    need(loaded == ((0x8664, 0x20b) if bits == 64 else (0x14c, 0x10b)), 'actual loaded system module architecture')
                    need(pe(p) == loaded, 'resolved system DLL matches loaded architecture')
                    modules[name] = {'reported_path': str(reported), 'resolved_file_path': str(p), 'sha256': sha(p), 'pe': pe(p), 'loaded_pe': loaded}
                runtime[bits] = {'metadata': meta, 'modules': modules}
                values = samples(target/'samples.bin', parsed[label])
                sampler_states(target/'sampler-states.bin', parsed[label], False)
                for name in ('vertexMain.bin', 'pixelMain.bin', 'vertexMain.asm', 'pixelMain.asm'):
                    h = sha(target/name)
                    need(name not in shader_hashes or shader_hashes[name] == h, 'identical authored shader across bitness/repeats')
                    shader_hashes[name] = h
                need((target/'pixelMain.asm').read_text().count('texldd') == 2, 'two explicit owned shader gradient operations')
                trace = (target/'samples.bin').read_bytes(); state = (target/'sampler-states.bin').read_bytes()
                key = (bits, label)
                need(key not in traces or traces[key] == (trace, state, values), 'exact per-bitness repeatability')
                traces[key] = trace, state, values
                previous = samples(prior, parsed[label])
                errors = [abs(a-b) for a,b in zip(values, previous)]
                need(len(values) == len(previous), 'complete preceding readback comparison')
                observations.append({'bits': bits, 'repeat': repeat, 'batch': label,
                                     'cases': len(parsed[label]), 'samples': len(values)//4,
                                     'prior_d3d9_byte_exact': trace == prior.read_bytes(),
                                     'prior_max_absolute_error': max(errors),
                                     'prior_different_components': sum(e != 0 for e in errors)})
                print(f'x{bits} repeat{repeat} {label}: {len(values)//4} actual GPU samples', flush=True)
    comparisons = []
    for label, _, _ in cases:
        a, b = traces[(64, label)], traces[(32, label)]
        need(a[1] == b[1], 'same actual sampler states across bitness')
        errors = [abs(x-y) for x,y in zip(a[2], b[2])]
        comparisons.append({'batch': label, 'readback_byte_exact': a[0] == b[0], 'sampler_byte_exact': True,
                            'components': len(errors), 'different_components': sum(e != 0 for e in errors), 'max_absolute_error': max(errors)})
    same_device_keys = ('description', 'vendor', 'device', 'driver_version', 'max_anisotropy', 'filter_caps', 'window_visible')
    need(all(runtime[32]['metadata'][k] == runtime[64]['metadata'][k] for k in same_device_keys), 'same device/driver/cap metadata')
    for group in (sources, protected, inputs):
        for p,h in group.items(): need(sha(p) == h, 'post-run frozen identity '+p)
    exact = all(c['readback_byte_exact'] for c in comparisons)
    result = {'status': 'VERIFIED MEASUREMENT: x86/x64 owned D3D9 readbacks '+('byte-exact' if exact else 'differ'),
              'time_utc': datetime.now(timezone.utc).isoformat(), 'previous_goal_turn': 'PROGRESS',
              'parent': parent, 'source_transition': transition, 'executables': executables,
              'runtime': runtime, 'shader_sha256': shader_hashes, 'comparisons': comparisons,
              'observations': observations, 'repetitions': 2,
              'cases_per_bitness_per_run': sum(len(v) for v in parsed.values()),
              'samples_per_bitness_per_run': sum(sum(n for _,_,n in v) for v in parsed.values()),
              'source_sha256': sources, 'protected_sha256': protected, 'input_sha256': inputs,
              'limits': ['One NVIDIA adapter/driver; explicit-gradient owned shaders/fixtures only, not original SFM runtime or shader fidelity.',
                         'Private x86 research executable is not an editor dependency or deliverable. Editor remains native x64.',
                         'No original installation/process access, driver/profile setting change, renderer integration or tolerance change.',
                         'Vulkan comparisons are preceding protected measurements, not a newly run Vulkan device.']}
    receipt = run/'results.json'; put(receipt, result)
    put(OUT/'latest-verification.json', {'receipt': str(receipt), 'receipt_sha256': sha(receipt)})
    print(json.dumps({k: result[k] for k in ('status', 'cases_per_bitness_per_run', 'samples_per_bitness_per_run', 'comparisons')}, indent=2), flush=True)

if __name__ == '__main__': main()
