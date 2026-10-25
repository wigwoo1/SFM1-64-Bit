"""Owned CPU/compute-GPU handoff using synthetic cubes, including real geometry.

No native SFM process, UI control, live probing or write to the installation.
"""
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/scene-model-ambient-20261024'
GAME = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
SESSION = GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise RuntimeError(message)


def main():
    run = Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    paths = [SESSION,GAME/'tf_movies/maps/mtt_soldier.bsp',GAME/'usermod/gameinfo.txt']
    prior = json.loads((ROOT/'outputs/vtf-legacy-thumbnail-20261024/verification-bk8vw043/results.json').read_text())
    paths += [Path(p) for p in prior['inputs_before'] if str(p).lower().endswith(('.vmt','.vtf'))]
    originals = {str(p):sha(p) for p in paths}
    require(originals[str(SESSION)]=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee', 'Original benchmark changed')
    source_names = ['scene_model_ambient_records_candidate.h','test_scene_model_ambient_records.cpp',
                    'probe_scene_model_ambient_vulkan.cpp','sfm64-generic-ambient-candidate.hlsli',
                    'sfm64-model-ambient-readback.hlsl','build_scene_model_ambient_x64.cmd',
                    'verify_scene_model_ambient_x64.py','generic_ambient_irradiance_candidate.h',
                    'prepared_session_scene_candidate.h','scene_gpu_vertex_candidate.h',
                    'retained_source_bsp_world_candidate.h','probe_vulkan_win32_surface_x64.cpp',
                    'sfm64-material-alpha-candidate.hlsl']
    sources = {name:sha(ROOT/'work'/name) for name in source_names}
    editor = ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe'
    editor_hash = sha(editor)
    shader = OUT/'ambient-readback.spv'
    shader_hash = sha(shader)
    asm = (OUT/'ambient-readback.spvasm').read_text()
    for text in ['OpMemberDecorate %AmbientRecord 0 Offset 0','OpMemberDecorate %AmbientRecord 1 Offset 16',
                 'OpDecorate %_runtimearr_AmbientRecord ArrayStride 112','OpDecorate %_runtimearr_v4float ArrayStride 16',
                 'OpDecorate %records Binding 0','OpDecorate %irradiance Binding 1']:
        require(text in asm, 'GPU ABI mismatch: '+text)
    require(asm.count('NoContraction')>=6, 'Explicit ambient arithmetic contraction flags missing')
    binaries = []
    for name in ['test_scene_model_ambient_records','probe_scene_model_ambient_vulkan']:
        exe = OUT/(name+'.exe')
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and
                struct.unpack_from('<H',raw,pe+24)[0]==0x20b, 'AMD64/PE32+ required')
        binaries.append(dict(path=str(exe),sha256=sha(exe),machine='AMD64',format='PE32+'))

    def execute(name,args,label):
        p = subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=180)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr)
        return p.stdout

    copy = run/'benchmark-input.dmx'
    copy.write_bytes(SESSION.read_bytes())
    benchmark_records = run/'benchmark-records.bin'
    cpu_log = execute('test_scene_model_ambient_records',[copy,GAME/'usermod/gameinfo.txt',benchmark_records],'cpu-handoff')
    require('ambient_handoff_rejections=66' in cpu_log and 'snapshot_owned=1' in cpu_log and
            'original_geometry_channels_unchanged=1' in cpu_log and 'benchmark_frames=2' in cpu_log, 'CPU coverage missing')
    require(sha(copy)==originals[str(SESSION)], 'Disposable input modified')
    m = re.search(r'benchmark_records=(\d+) animated_normals_changed=(\d+)',cpu_log)
    require(m and int(m[1])>0 and int(m[2])>0,'Animated normal coverage missing')
    benchmark_count,changed = map(int,m.groups())
    raw = benchmark_records.read_bytes()
    require(len(raw)==benchmark_count*112,'Benchmark record count mismatch')

    rng = random.Random(2026102414)
    directions = [(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),(-0.0,1,-0.0)]
    directions += [(x,y,z) for x in (-1,1) for y in (-1,1) for z in (-1,1)]
    synthetic = []

    def append(normal,cube):
        synthetic.append(struct.pack('<28f',*normal,0,*[value for face in cube for value in [*face,0]]))

    for face in range(6):
        for channel in range(3):
            cube = [[0.0]*3 for _ in range(6)]
            cube[face][channel]=1.0
            for normal in directions:
                append(normal,cube)
    for _ in range(4096):
        normal = [rng.uniform(-8,8) for _ in range(3)]
        cube = [[rng.uniform(-2,16) for _ in range(3)] for _ in range(6)]
        append(normal,cube)
    combined = run/'combined-records.bin'
    combined.write_bytes(raw+b''.join(synthetic))
    total = benchmark_count+len(synthetic)
    gpu_output = run/'gpu-results.bin'
    gpu_log = execute('probe_scene_model_ambient_vulkan',[shader,combined,gpu_output],'vulkan-handoff')
    require('PASS adapter=' in gpu_log and 'records='+str(total) in gpu_log,'GPU coverage mismatch')
    output = gpu_output.read_bytes()
    require(len(output)==total*16,'GPU output length')
    for record in struct.iter_unpack('<4f',output):
        require(all(math.isfinite(x) for x in record) and record[3]==1,'GPU incomplete output')
    stats = re.search(r'exact_rgb_records=(\d+) max_abs_error=([\deE.+-]+)',gpu_log)
    require(stats is not None,'GPU precision metrics missing')
    require(all(sha(p)==h for p,h in originals.items()),'Original input modified')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()),'Source changed during tests')
    require(sha(editor)==editor_hash and sha(shader)==shader_hash,'Editor or shader changed during tests')
    require(all(sha(Path(b['path']))==b['sha256'] for b in binaries),'Test executable changed during run')
    warnings = {}
    for name in ['test_scene_model_ambient_records','probe_scene_model_ambient_vulkan']:
        log = (OUT/(name+'-build.log')).read_text()
        require('error C' not in log,'Compiler failure')
        warnings[name] = [line for line in log.splitlines() if 'warning C' in line]
    require(not warnings['probe_scene_model_ambient_vulkan'],'New GPU-probe compiler warnings')
    require(all(('warning C4458' in w and 'graph_transform_hierarchy.h' in w) or
                ('warning C4996' in w and 'model_render_inputs_candidate.h' in w)
                for w in warnings['test_scene_model_ambient_records']), 'Unexpected new benchmark warning')
    require(not (OUT/'shader-build.log').read_text().strip(),'Shader diagnostics require review')
    receipt = dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),
        benchmark_frames=[120,121],benchmark_records=benchmark_count,animated_normals_changed=changed,
        synthetic_records=len(synthetic),gpu_records=total,exact_rgb_records=int(stats[1]),max_absolute_error=float(stats[2]),
        relative_tolerance='2e-6 * max(1, abs(CPU reference channel))',cpu_input_rejections=66,
        cpu_log=cpu_log.strip(),gpu_log=gpu_log.strip(),executables=binaries,build_warnings=warnings,
        shader_sha256=shader_hash,sources_sha256=sources,original_sha256=originals,
        combined_records_sha256=sha(combined),gpu_output_sha256=sha(gpu_output),
        editor=dict(path=str(editor),sha256=editor_hash,rebuilt=False,executed=False),
        limitations=[
            'Cubes are explicit synthetic per-model test inputs, not sampled native map lighting or runtime ConVar state.',
            'Real benchmark normals come from the current owned CPU scene evaluation; native bone/normal precision equivalence remains unverified.',
            'Actual owned Vulkan compute execution is checked against the CPU expression within a stated tolerance. No original GPU shader executes.',
            'No rasterized image, active benchmark material selection, complete native illumination or editor viewport change is claimed.',
            'Separate value-owned records preserve existing geometry and the BSP normal/light-color channel; helper consumes pre-world model geometry only.',
            'No computer use, original process enumeration/probing/launch, Blender or original installation/session writes.',
        ])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['status','benchmark_records','animated_normals_changed','synthetic_records','gpu_records','exact_rgb_records','max_absolute_error']}))
    print('receipt='+str(run/'results.json'))


if __name__=='__main__':
    main()
