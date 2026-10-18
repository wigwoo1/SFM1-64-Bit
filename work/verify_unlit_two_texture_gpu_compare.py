"""Execute both x64 GPU paths and compare actual captures, not only centers."""
from pathlib import Path
import csv
import hashlib
import json
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent.parent
OUTPUT=ROOT/'outputs/unlit-two-texture-gpu-20261014'
SHADERS=ROOT/'outputs/unlit-two-texture-contract-20261014/native-si802g4p'


def require(condition,message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def executable(path):
    raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x8664
            and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'AMD64 PE32+ required')
    return dict(path=str(path),sha256=digest(path),architecture='AMD64 PE32+')


extraction=json.loads((SHADERS/'evidence.json').read_text())
programs={}
for cache in extraction['caches']:
    require(digest(Path(cache['path']))==cache['sha256'],'Installed shader cache changed')
    for entry in cache['programs']:
        programs[Path(entry['disassembly']).with_suffix('.bin').name]=entry['bytecode_sha256']
for name in ['unlittwotexture_vs20-s0-d2.bin','unlittwotexture_ps20-s0-d0.bin']:
    require(digest(SHADERS/name)==programs[name],'Extracted installed program changed')
directory=Path(tempfile.mkdtemp(prefix='comparison-',dir=OUTPUT))
runs={}
for backend,name,shader_directory in [
    ('d3d9','probe_unlit_two_texture_d3d9_compare.exe',SHADERS),
    ('vulkan','probe_unlit_two_texture_vulkan.exe',OUTPUT),
    ('production','probe_unlit_two_texture_vulkan.exe',OUTPUT)]:
    path=OUTPUT/name;record=executable(path)
    arguments=[str(path),str(shader_directory),str(directory/backend)]
    if backend=='production':arguments.append('--production-transport')
    run=subprocess.run(arguments,cwd=ROOT,
                       capture_output=True,text=True,timeout=45)
    (directory/(backend+'.log')).write_text(run.stdout+run.stderr)
    require(run.returncode==0,backend+' failed: '+run.stdout+run.stderr)
    if backend=='production':require('production_fragment_and_shared_indexed_recorder=passed' in run.stdout,'Production path not exercised')
    rows=list(csv.DictReader((directory/backend/'pixels.csv').open()))
    require(len(rows)==14 and len({row['case'] for row in rows})==14,'Missing or duplicate cases')
    record['pixels']={row['case']:[int(row[channel]) for channel in ['r','g','b','a']] for row in rows}
    record['adapter_line']=next(line for line in run.stdout.splitlines() if line.startswith('adapter='))
    require(digest(path)==record['sha256'],'Executable changed during run')
    runs[backend]=record
require(len({run['adapter_line'] for run in runs.values()})==1,'Different physical adapters')
for backend in runs:
    encoded=runs[backend]['pixels']['srgb-layered']
    control=runs[backend]['pixels']['srgb-layered-unorm-control']
    require(max(abs(a-b) for a,b in zip(encoded,control))>20,
            'Output-format negative control failed to distinguish gamma behavior')
expected={
    'linear-product':[64,32,16,255], 'second-uv-only':[32,64,32,255],
    'first-uv-only':[32,32,32,255], 'additive':[80,64,64,255],
    'alpha-write-off':[80,64,64,64], 'occluded':[16,32,48,64],
    'depth-write-off':[32,64,32,255], 'depth-write-on-control':[64,32,16,255],
    'drawn-occluder':[64,32,16,255]}
records=[]
for target in ['vulkan','production']:
 require(set(runs['d3d9']['pixels'])==set(runs[target]['pixels']),'Different case sets')
 for name in runs['d3d9']['pixels']:
    source=(directory/'d3d9'/(name+'.rgba')).read_bytes()
    owned=(directory/target/(name+'.rgba')).read_bytes()
    require(len(source)==len(owned)==64*64*4,'Invalid capture size')
    # Compare all pixels as well as an interior tile, so coverage/edge differences
    # are not hidden. Cases deliberately have constant interpolated UVs.
    errors=[abs(a-b) for a,b in zip(source,owned)]
    tolerance=2 if name.startswith('srgb-') else 1
    require(max(errors)<=tolerance,'Cross-backend pixel mismatch: '+name)
    for backend,raw in [('d3d9',source),(target,owned)]:
        center=list(raw[(32*64+32)*4:(32*64+32)*4+4])
        require(center==runs[backend]['pixels'][name],'CSV mismatch: '+backend)
        if name in expected:
            require(max(abs(a-b) for a,b in zip(center,expected[name]))<=1,'Expected pixel mismatch: '+name)
        for y in range(16,48):
            for x in range(16,48):
                require(list(raw[(y*64+x)*4:(y*64+x)*4+4])==center,'Interior variation: '+name)
    records.append(dict(case=name,backend=target,maximum_channel_error=max(errors),tolerance=tolerance,
                        differing_channels=sum(error!=0 for error in errors),
                        d3d9_sha256=hashlib.sha256(source).hexdigest(),
                        vulkan_sha256=hashlib.sha256(owned).hexdigest()))
report=dict(status='PASS_CONTROLLED_GPU_COMPARISON',runs=runs,cases=records,
            compared_pixels=2*14*64*64,rgba_components_compared=2*14*64*64*4,
            owned_shader_source_sha256=digest(ROOT/'work/unlit_two_texture_gpu_reference.hlsl'),
            production_fragment_source_sha256=digest(ROOT/'work/sfm64-unlit-two-texture-candidate.hlsl'),
            spirv_sha256={name:digest(OUTPUT/name) for name in ['two-texture-reference-vs.spv','two-texture-reference-ps.spv','two-texture-raw-vs.spv','sfm64-unlit-two-texture-candidate.spv']},
            limitations=['Synthetic inputs; not a live SFM pilot-light snapshot.',
                         'D3D9 installed variant pair versus separately reconstructed Vulkan normal contract only.',
                         'Constant sampled UV fixture; no perspective-varying UV, filtering/mips, fog, cloak, proxy or scene parity claim.',
                         'sRGB clear conversion and layered blending measured on this device; native SFM pass scheduling still unverified.',
                         'Editor integration and regression testing remain unfinished.'])
(directory/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: 14 cases in each Vulkan path; 114,688 pixels compared; max channel error='+str(max(record['maximum_channel_error'] for record in records)))
print(directory/'results.json')
