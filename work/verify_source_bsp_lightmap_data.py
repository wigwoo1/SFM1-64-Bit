"""Fresh native CPU tests and independent raw BSP lightmap inventory.

Original installation is read-only. All outputs, including private benchmark
sample inventories, stay in a fresh workspace directory. Never publish inventories.
No renderer, original SFM process, GUI control, or editor promotion participates.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import math
import os
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/bsp-lightmap-contract-20261101'
VS = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
DUMP = VS.parents[2]/'Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe'


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(value, message):
    if not value:
        raise RuntimeError(message)


def put(p, value):
    Path(p).write_text(json.dumps(value, indent=2)+'\n')


def main():
    before = json.loads((OUT/'before.json').read_text())
    parent = before['parent']
    need(sha(parent['receipt']) == parent['receipt_sha256'], 'parent identity')
    prior = json.loads(Path(parent['receipt']).read_text())
    sources, protected = dict(before['source_sha256']), dict(before['protected_sha256'])
    protected[parent['receipt']] = parent['receipt_sha256']
    for group in (sources, protected, before['complete_editor_dependencies_sha256']):
        for p, h in group.items():
            need(sha(p) == h, 'before identity '+p)
    for name in ('source_bsp_lightmap_data_candidate.h','test_source_bsp_lightmap_data.cpp',Path(__file__).name):
        p = ROOT/'work'/name
        need(str(p) not in sources, 'new work files only')
        sources[str(p)] = sha(p)
    # The reused legacy owned test is an explicit source dependency, even though
    # it was not itself in the preceding snapshot's complete-editor closure.
    visited = set()

    def visit(p):
        p = p.resolve()
        if p in visited:
            return
        visited.add(p)
        if str(p) in sources:
            need(sha(p) == sources[str(p)], 'source include identity')
        sources[str(p)] = sha(p)
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', p.read_text(errors='replace'), re.M):
            if (p.parent/name).is_file():
                visit(p.parent/name)
    visit(ROOT/'work/test_source_bsp_lightmap_data.cpp')
    identity = json.loads((OUT/'benchmark-input-identity.json').read_text())
    map_path = Path(identity['path'])
    need(sha(map_path) == identity['sha256'], 'read-only benchmark identity')
    run = Path(tempfile.mkdtemp(prefix='r-', dir=OUT))
    print(str(run), flush=True)
    exe = run/'test_source_bsp_lightmap_data.exe'
    batch = run/'build.cmd'
    batch.write_text('\n'.join([
        '@echo off', f'cd /d "{ROOT}"', f'call "{VS}" >nul', 'if errorlevel 1 exit /b 1',
        f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\test_source_bsp_lightmap_data.cpp /Fo:"{run/"test.obj"}" /Fe:"{exe}" >"{run/"build.log"}" 2>&1',
        'exit /b %errorlevel%', '']))
    clean = {k: v for k, v in os.environ.items() if not k.startswith('SFM64_')}

    def execute(args, label):
        p = subprocess.run(list(map(str, args)), cwd=ROOT, env=clean,
                           capture_output=True, text=True, timeout=300,
                           creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        need(p.returncode == 0, label+' failed: '+(p.stdout+p.stderr)[-2200:])
        return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',batch], 'build-driver')
    b = exe.read_bytes()
    pe = struct.unpack_from('<I', b, 60)[0]
    need((struct.unpack_from('<H',b,pe+4)[0],struct.unpack_from('<H',b,pe+24)[0]) == (0x8664,0x20b), 'AMD64 PE32+ test')
    warnings = re.findall(r'warning (C\d+): ([^\r\n]+)', (run/'build.log').read_text())
    logs = []
    for n in range(2):
        logs.append(execute([exe,run/f't{n}',map_path],f't{n}'))
        print('Fresh CPU test repeat '+str(n)+' passed',flush=True)
    need((run/'t0/faces.bin').read_bytes() == (run/'t1/faces.bin').read_bytes(), 'repeatable face inventory')
    need((run/'t0/lighting.bin').read_bytes() == (run/'t1/lighting.bin').read_bytes(), 'repeatable raw lighting')
    # Independent byte-level oracle, derived from public SDK layout. It compares
    # every retained face/texinfo byte and every style/plane sample, not a count
    # or a second call into the C++ reader.
    source = map_path.read_bytes()

    def lump(i):
        return struct.unpack_from('<4i',source,8+16*i)
    face_offset, face_length, _, _ = lump(58)
    light_offset, light_length, _, _ = lump(53)
    info_offset, _, _, _ = lump(6)
    model_offset, _, _, _ = lump(14)
    first, count = struct.unpack_from('<2i',source,model_offset+40)
    need(first >= 0 and first+count <= face_length//56, 'independent world face span')
    need((run/'t0/lighting.bin').read_bytes() == source[light_offset:light_offset+light_length], 'whole HDR lighting lump byte-exact')
    capture = (run/'t0/faces.bin').read_bytes()
    at = samples = available = unavailable = bumped = displaced = 0
    nonzero_offsets = negative_minima = 0
    for index in range(first, first+count):
        f = face_offset+index*56
        ti, di = struct.unpack_from('<2h',source,f+10)
        info = info_offset+ti*72
        flags, = struct.unpack_from('<I',source,info+64)
        offset, = struct.unpack_from('<i',source,f+20)
        minima = struct.unpack_from('<2i',source,f+28)
        size = struct.unpack_from('<2i',source,f+36)
        styles = source[f+16:f+20]
        styles_count = next((j for j, value in enumerate(styles) if value == 255), 4)
        planes = 4 if flags & 0x800 else 1
        state = 0 if offset < 0 else 1 if styles_count == 0 else 2
        expected = (index, ti, flags, di & 0xffffffff, offset & 0xffffffff,
                    *(v & 0xffffffff for v in minima), *(v & 0xffffffff for v in size),
                    styles_count, planes, state)
        need(struct.unpack_from('<12I',capture,at) == expected, 'independent raw face metadata '+str(index))
        at += 48
        need(capture[at:at+4] == styles, 'authored style bytes')
        at += 4
        need(capture[at:at+56] == source[f:f+56], 'authored face bytes')
        at += 56
        need(capture[at:at+72] == source[info:info+72], 'authored texinfo bytes')
        at += 72
        bumped += planes == 4
        displaced += di >= 0
        negative_minima += any(x < 0 for x in minima)
        if state == 2:
            available += 1
            nonzero_offsets += offset != 0
            n = (size[0]+1)*(size[1]+1)*styles_count*planes*4
            need(offset >= 0 and offset+n <= light_length, 'independent full face span')
            need(capture[at:at+n] == source[light_offset+offset:light_offset+offset+n], 'every raw style/plane sample '+str(index))
            at += n
            samples += n//4
        else:
            unavailable += 1
    need(at == len(capture), 'no missing or extra inventory bytes')
    # All earlier source and editor identities remain unchanged. This reader is
    # not yet included in the editor: do not claim an editor rebuild/test here.
    for group in (sources, protected, before['complete_editor_dependencies_sha256']):
        for p, h in group.items():
            need(sha(p) == h, 'final identity '+p)
    for closure in prior['unaffected_cpu_test_closures'].values():
        for p, h in closure.items():
            need(sha(p) == h, 'previous CPU-test closure unchanged')
    need(sha(map_path) == identity['sha256'], 'original benchmark unchanged')
    execute([DUMP,'/dependents',exe], 'direct-imports')
    imports = re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$',(run/'direct-imports.log').read_text(),re.M|re.I)
    need({i.lower() for i in imports} == {'kernel32.dll'}, 'owned CPU executable direct dependencies')
    metrics = {'faces': count, 'available': available, 'unavailable': unavailable,
               'bumped': bumped, 'displacement_faces': displaced, 'raw_samples': samples,
               'lighting_bytes': light_length, 'nonzero_lit_offsets': nonzero_offsets,
               'faces_with_negative_minima': negative_minima,
               'all_raw_records_and_samples_byte_exact': True}
    result = {'status': 'VERIFIED immutable CPU lightmap records; editor/scene/GPU integration incomplete',
              'time_utc': datetime.now(timezone.utc).isoformat(), 'previous_goal_turn':'PROGRESS',
              'parent':parent, 'source_sha256':sources, 'protected_sha256':protected,
              'executable':{'path':str(exe),'sha256':sha(exe),'architecture':'AMD64 PE32+','direct_imports':imports},
              'compiler_warnings':warnings, 'repetitions':2, 'logs':logs, 'benchmark_read_only':identity,
              'benchmark_raw_inventory':metrics, 'editor_rebuilt':False,'new_editor_gpu_frames':0,
              'complete_editor_preserved':prior['complete_editor_preserved'],
              'complete_editor_dependencies_sha256':before['complete_editor_dependencies_sha256'],
              'unaffected_cpu_test_closures':prior['unaffected_cpu_test_closures'],
              'canonical_preserved_sha256':prior['canonical_preserved_sha256'],
              'limits':['Standalone immutable data reader; geometry emitted-vertex/triangle bindings and retained scene transport are not implemented yet.',
                        'No new material/atlas/color conversion/sampling/lighting shader or lit Source parity claim.',
                        'Private benchmark raw-data inventories are original-asset derivatives; never publish.',
                        'Original sources, legacy preview, complete editor dependencies and prior tests preserved; no editor/default-GPU rerun.'],
              'full_goal':'active/incomplete; independent reconstruction'}
    receipt=run/'results.json'
    put(receipt,result)
    put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)})
    print(json.dumps({'status':result['status'],'executable':result['executable'],
                      'warnings':len(warnings),'benchmark':metrics},indent=2),flush=True)


if __name__=='__main__':
    main()
