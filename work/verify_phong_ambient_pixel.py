"""Controlled original D3D9 pixel shader vs owned Vulkan arithmetic and Python.

Private original bytecode/assembly stays in isolated outputs. No editor/SFM use.
Vulkan graphics samples constant textures; compute receives already-sampled inputs.
Nonconstant texture filtering and spatial interpolation equivalence are not claimed.
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

from verify_session_preview_aspect import ROOT, VS, INCLUDE
from vcs6_shader_cache_candidate import Vcs6ShaderCache
from inspect_generic_ambient_shader import disassemble

OUT = ROOT / 'outputs/phong-ambient-pixel-20261030'
TOLERANCE = 2e-5  # Fixed before GPU measurement: abs difference <= tol * max(1,abs(reference)).


def need(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def execute(args, log, success=True):
    p = subprocess.run([str(x) for x in args], cwd=ROOT, capture_output=True, text=True, timeout=150)
    log.write_text(p.stdout + p.stderr, encoding='utf-8')
    if success:
        need(p.returncode == 0, f'{log}: exit {p.returncode}: {(p.stdout+p.stderr)[-1800:]}')
    return p


def reference(row):
    """High precision semantic reference, independently expressed with face selection.

    Not an emulator for D3D instruction rounding, fused MAD or hardware normalize.
    """
    v = [row[i:i+4] for i in range(0, 72, 4)]
    base, sample, tangent, binormal, normal, position, tint, eye, fog, controls, fogcolor, scale = v[:12]
    local = [2*x-1 for x in sample[:3]]
    world = [sum(a[i]*b for a, b in zip((tangent, binormal, normal), local)) for i in range(3)]
    length = math.sqrt(sum(x*x for x in world))
    n = [x/length for x in world]
    ambient = [sum(n[axis]**2*v[12+2*axis+int(n[axis]<0)][c] for axis in range(3)) for c in range(3)]
    weight = min(1, max(0, base[3]+controls[2]))
    distance = math.sqrt(sum((a-b)**2 for a,b in zip(eye[:3], position[:3])))
    fogweight = min(min(1, max(0, distance*fog[3]+fog[0])), fog[2])**2
    lit = [base[c]*ambient[c]*(1+weight*(tint[c]-1))*scale[0] for c in range(3)]
    rgb = [a*(1-fogweight)+b*fogweight for a,b in zip(lit, fogcolor[:3])]
    alpha = tint[3]*(1+(1-controls[0])*controls[2]*(base[3]-1))
    return [*rgb, alpha]


def fixtures():
    rng = random.Random(2026103011)
    cube = [[.15, .3, .55, 1], [.9, .1, .2, 1], [.2, .8, .4, 1],
            [.6, .25, .75, 1], [1.2, .65, .1, 1], [.4, .9, 1.4, 1]]
    bases = [
        ([1,0,0,0], [0,1,0,0], [0,0,1,0]),
        ([0,1,0,0], [-1,0,0,0], [0,0,1,0]),
        ([0,0,1,0], [1,0,0,0], [0,1,0,0]),
        ([.8,.6,0,0], [-.6,.8,0,0], [0,0,-1,0]),
        ([1.4,.1,-.2,0], [.3,.7,.1,0], [-.2,.15,1.1,0]),
    ]
    directions = [(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),
                  (.7,.3,.4),(-.4,.9,-.7),(.01,-.02,-.99),(-.7,-.8,.1)]
    rows, labels = [], []
    def append(v, label):
        flat = [x for f in v for x in f]
        need(len(flat) == 72, 'fixture width')
        rows.append(list(struct.unpack('<72f', struct.pack('<72f', *flat))))
        labels.append(label)
    for bi,(t,b,n) in enumerate(bases):
        for ni,d in enumerate(directions):
            for mode in range(4):
                sample = [*(.5+.5*x for x in d), .37]
                controls = [[0,0,0,0], [.3,0,.7,0], [1,0,1,0], [-.2,0,-.4,0]][mode]
                fog = [[0,0,1,0], [.1,0,.8,.04], [0,0,.65,.3], [-1,0,1,.02]][mode]
                append([[.7,.4,.9,[0,.25,1,.8][mode]],sample,t,b,n,[.2,-.4,.1,0],
                        [.4,1.3,.7,.8],[2,3,5,0],fog,controls,[.1,.2,.35,0],[1.25,7,8,9],*cube],
                       f'basis{bi}-direction{ni}-mode{mode}')
    for i in range(384):
        t,b,n = bases[i%len(bases)]
        sample = [rng.uniform(.03,.97) for _ in range(4)]
        append([[rng.random() for _ in range(4)],sample,t,b,n,[rng.uniform(-3,3) for _ in range(4)],
                [rng.uniform(0,2) for _ in range(4)],[rng.uniform(4,7) for _ in range(4)],
                [rng.uniform(-1,1),0,rng.uniform(0,1),rng.uniform(-.2,.3)],
                [rng.uniform(-.5,1.5),0,rng.uniform(-.5,1.5),0],
                [rng.uniform(0,2) for _ in range(4)],[rng.uniform(.2,2),0,0,0],
                *[[rng.uniform(0,2) for _ in range(4)] for _ in range(6)]], f'seeded-{i}')
    return rows, labels


def compare(name, actual, expected, labels):
    need(len(actual) == len(expected), name+' length')
    max_error = max_scaled = 0.0
    for i,(a,e) in enumerate(zip(actual, expected)):
        need(len(a) == 4 and all(math.isfinite(x) for x in a), name+' finite')
        for c,(x,y) in enumerate(zip(a,e)):
            error = abs(x-y);scaled = error/max(1,abs(y))
            need(scaled <= TOLERANCE, f'{name} {labels[i]} channel{c}: {x} vs {y}, error{error}')
            max_error = max(max_error,error);max_scaled = max(max_scaled,scaled)
    return dict(maximum_absolute_error=max_error, maximum_scaled_error=max_scaled, tolerance=TOLERANCE)


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    (OUT/'active-run.txt').write_text(str(run), encoding='utf-8')
    prior_index = ROOT/'outputs/phong-diffuse-contract-20261030/latest-verification.json'
    index = json.loads(prior_index.read_text(encoding='utf-8'))
    need(sha(index['receipt']) == index['receipt_sha256'], 'prior receipt authentication')
    prior = json.loads(Path(index['receipt']).read_text(encoding='utf-8'))
    for group in ['source_sha256','protected_sha256']:
        need(all(sha(p)==h for p,h in prior[group].items()), 'prior '+group+' preserved')
    sources = dict(prior['source_sha256'])
    for name in ['phong_ambient_pixel_fixture.h','phong_ambient_pixel_candidate.hlsli','phong_ambient_pixel_compute.hlsl',
                 'phong_ambient_pixel_graphics.hlsl','probe_phong_ambient_pixel_graphics.cpp',
                 'probe_phong_ambient_pixel_d3d9.cpp','probe_phong_ambient_pixel_vulkan.cpp',Path(__file__).name]:
        p = ROOT/'work'/name;sources[str(p)] = sha(p)
    sources[str(INCLUDE/'vulkan/vulkan.h')] = sha(INCLUDE/'vulkan/vulkan.h')
    protected = dict(prior['protected_sha256'])
    protected[str(Path(index['receipt']))] = sha(index['receipt'])
    dxc = ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe'
    protected[str(dxc)] = sha(dxc)
    cache_path = Path(prior['shader']['source'])
    data = cache_path.read_bytes();count = struct.unpack_from('<I',data,20)[0]
    entries = [struct.unpack_from('<II',data,28+8*i) for i in range(count)]
    need(entries[0][0] == 0, 'primary zero')
    payload = data[entries[0][1]:entries[1][1]]
    fixture = data[:20]+struct.pack('<II',2,struct.unpack_from('<I',data,24)[0])+struct.pack('<II',0,48)+struct.pack('<II',0xffffffff,48+len(payload))+struct.pack('<I',0)+payload
    shader = Vcs6ShaderCache(fixture).shader(0,0)
    need(hashlib.sha256(shader).hexdigest()==prior['shader']['bytecode_sha256'], 'unchanged native pixel bytecode')
    native_shader = run/'private-original-phong-ps20b-s0-d0.bin';native_shader.write_bytes(shader)
    (run/'private-original-phong-ps20b-s0-d0.asm').write_text(disassemble(shader), encoding='utf-8')
    batch = run/'build.cmd'
    lines = ['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1']
    for api, libs, include in [('d3d9','d3d9.lib d3dcompiler.lib user32.lib',''),('vulkan','',f'/I"{INCLUDE}"'),('graphics','',f'/I"{INCLUDE}"')]:
        lines += [f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 {include} work\\probe_phong_ambient_pixel_{api}.cpp /Fo:"{run/(api+".obj")}" /Fe:"{run/(api+"_x64.exe")}" {libs} >"{run/(api+"-build.log")}" 2>&1', 'if errorlevel 1 exit /b 1']
    for stage, entry, source, output in [('cs','main','compute','ambient'),('vs','vertexMain','graphics','vertex'),('ps','pixelMain','graphics','pixel')]:
        lines += [f'"{dxc}" -spirv -T {stage}_6_0 -E {entry} -Gis -O3 -fvk-use-gl-layout -fspv-target-env=vulkan1.0 work\\phong_ambient_pixel_{source}.hlsl -Fo "{run/("owned-phong-"+output+".spv")}" >"{run/(stage+"-build.log")}" 2>&1',
                  'if errorlevel 1 exit /b 1']
    lines += ['exit /b 0','']
    batch.write_text('\n'.join(lines), encoding='utf-8')
    execute(['cmd.exe','/d','/c',batch],run/'build-driver.log')
    for stage in ['cs','vs','ps']:
        need(not re.search(r'warning', (run/(stage+'-build.log')).read_text(errors='replace'), re.I),stage+' shader warnings')
    binaries = {}
    for api in ['d3d9','vulkan','graphics']:
        p = run/(api+'_x64.exe');raw = p.read_bytes();pe = struct.unpack_from('<I',raw,60)[0]
        need(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b, 'AMD64 PE32+')
        need(not re.search(r'warning C\d+', (run/(api+'-build.log')).read_text(errors='replace')),api+' compiler warnings')
        binaries[api] = dict(path=str(p),sha256=sha(p))
    rows, labels = fixtures();query = run/'queries.bin'
    query.write_bytes(b''.join(struct.pack('<72f',*r) for r in rows))
    (run/'labels.json').write_text(json.dumps(labels,indent=2),encoding='utf-8')
    expected = [reference(row) for row in rows]
    comparisons = {};native_first = owned_first = graphics_first = None
    graphics_validation = []
    for repeat in range(2):
        native_dir = run/('native-'+str(repeat))
        execute([run/'d3d9_x64.exe',native_shader,query,native_dir],run/('native-'+str(repeat)+'.log'))
        pixels = (native_dir/'native-pixels.bin').read_bytes()
        need(len(pixels)==len(rows)*256, 'native 16-pixel framing')
        native = [struct.unpack_from('<4f',pixels,i*256) for i in range(len(rows))]
        owned_path = run/('owned-'+str(repeat)+'.bin')
        execute([run/'vulkan_x64.exe',run/'owned-phong-ambient.spv',query,owned_path],run/('owned-'+str(repeat)+'.log'))
        owned_bytes = owned_path.read_bytes();need(len(owned_bytes)==len(rows)*16, 'Vulkan framing')
        owned = list(struct.iter_unpack('<4f',owned_bytes))
        metadata = dict(line.split('=',1) for line in (native_dir/'device.txt').read_text().splitlines())
        need(metadata['window_visible']=='0', 'native fixture hidden')
        compile_log = native_dir/'vertex-compile.log'
        need(not compile_log.exists() or not compile_log.read_bytes(), 'owned D3D vertex compiler messages')
        graphics_dir = run/('graphics-'+str(repeat))
        execute([run/'graphics_x64.exe',run,query,graphics_dir,metadata['vendor'],metadata['device']],run/('graphics-'+str(repeat)+'.log'))
        graphics_bytes = (graphics_dir/'vulkan-pixels.bin').read_bytes()
        need(len(graphics_bytes)==len(pixels), 'Vulkan graphics framing')
        graphics = [struct.unpack_from('<4f',graphics_bytes,i*256) for i in range(len(rows))]
        graphics_validation.append(dict(line.split('=',1) for line in (graphics_dir/'validation-status.txt').read_text().splitlines()))
        need(graphics_validation[-1]['errors']=='0', 'graphics validation errors')
        comparisons['native-vs-semantic'] = compare('native-vs-semantic',native,expected,labels)
        comparisons['vulkan-vs-semantic'] = compare('vulkan-vs-semantic',owned,expected,labels)
        comparisons['vulkan-vs-native'] = compare('vulkan-vs-native',owned,native,labels)
        comparisons['graphics-vs-semantic'] = compare('graphics-vs-semantic',graphics,expected,labels)
        comparisons['graphics-vs-native'] = compare('graphics-vs-native',graphics,native,labels)
        if repeat:
            need(pixels==native_first and owned_bytes==owned_first and graphics_bytes==graphics_first, 'repeat exact traces')
        else:
            native_first,owned_first,graphics_first = pixels,owned_bytes,graphics_bytes
    refusals = []
    invalids = {}
    for name,sample in {'zero-normal':[.5,.5,.5], 'nan':[float('nan'),.5,1], 'range':[1.2,.5,1]}.items():
        row=rows[0].copy();row[4:7]=sample;invalids[name]=struct.pack('<72f',*row)
    row=rows[0].copy();row[28:31]=row[20:23];invalids['zero-eye-distance']=struct.pack('<72f',*row)
    row=rows[0].copy();row[34]=1.25;invalids['fog-cap']=struct.pack('<72f',*row)
    invalids['truncated']=query.read_bytes()[:-1]
    for name,payload in invalids.items():
        bad=run/(name+'.bin');bad.write_bytes(payload)
        for api in ['d3d9','vulkan','graphics']:
            target=run/(name+'-'+api+('.bin' if api=='vulkan' else ''))
            args=[run/(api+'_x64.exe'), native_shader if api=='d3d9' else run if api=='graphics' else run/'owned-phong-ambient.spv',bad,target]
            if api=='graphics':
                args += [metadata['vendor'],metadata['device']]
            p=execute(args,run/(name+'-'+api+'.log'),False)
            need(p.returncode!=0 and not target.exists(),name+' refusal before output/GPU')
            refusals.append(name+'-'+api)
    for api in ['d3d9','vulkan','graphics']:
        target=run/('existing-'+api)
        if api=='vulkan':
            target.write_bytes(b'disposable-existing-output');before=sha(target)
        else:
            target.mkdir();(target/'marker').write_bytes(b'disposable-existing-directory');before=sha(target/'marker')
        args=[run/(api+'_x64.exe'),native_shader if api=='d3d9' else run if api=='graphics' else run/'owned-phong-ambient.spv',query,target]
        if api=='graphics':
            args += [metadata['vendor'],metadata['device']]
        p=execute(args,run/('existing-'+api+'.log'),False)
        need(p.returncode!=0 and sha(target if api=='vulkan' else target/'marker')==before, 'existing output preserved '+api)
        refusals.append('existing-'+api)
    need(all(sha(p)==h for p,h in sources.items()), 'source identities preserved')
    need(all(sha(p)==h for p,h in protected.items()), 'protected identities preserved')
    result = dict(status='VERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),native_pixels_per_run=len(rows)*16,
                  repetitions=2,comparisons=comparisons,refusals=refusals,executables=binaries,
                  owned_shader_sha256=sha(run/'owned-phong-ambient.spv'),native_shader_sha256=sha(native_shader),
                  graphics_shader_sha256={stage:sha(run/('owned-phong-'+stage+'.spv')) for stage in ['vertex','pixel']},
                  d3d_owned_vertex_sha256=sha(run/'native-0/owned-vertex.bin'),
                  graphics_validation=graphics_validation,graphics_pixels_sha256=hashlib.sha256(graphics_first).hexdigest(),
                  native_pixels_sha256=hashlib.sha256(native_first).hexdigest(),owned_results_sha256=hashlib.sha256(owned_first).hexdigest(),
                  source_sha256=sources,protected_sha256=protected,
                  limits=['Explicit original PS20b static0/dynamic0; active benchmark selection UNVERIFIED.',
                          'D3D9 uses constant interpolants, point/clamp 1x1 float textures, linear sampling and float target.',
                          'Vulkan compute uses sampled inputs; Vulkan graphics uses actual point/clamp 1x1 float textures and constant interpolants.',
                          'Nonconstant texture filtering, spatial interpolation, other permutations and production scene binding UNVERIFIED.',
                          'Compute validation layer not enabled; graphics validation availability/status recorded separately.',
                          'Finite bounded fixtures and nonzero normal/distance only; not bit-exact hardware arithmetic.',
                          'No editor dependency/integration/rebuild, live SFM, computer control, original writes or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:result[k] for k in ['status','cases','native_pixels_per_run','comparisons','executables','refusals']},indent=2),flush=True)


if __name__=='__main__':
    main()
