"""No-app, no-live-probe original bone kernel versus owned x64."""
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import random
import re
import shutil
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/bone-world-palette-20261024'
GAME=IFM.parent.parent.parent


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(v,m):
    if not v:raise RuntimeError(m)


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    originals={str(p):sha(p) for p in [IFM,SESSION,GAME/'usermod/gameinfo.txt']}
    require(originals[str(IFM)]==IFM_SHA and originals[str(SESSION)]==SESSION_SHA,'Original pins')
    sources={p:sha(ROOT/'work'/p) for p in ['ifm_bone_world_palette_candidate.h','session_ifm_bone_inputs_candidate.h',
        'probe_ifm_bone_world_palette.cpp','test_ifm_bone_world_palette.cpp','test_benchmark_ifm_bone_inputs.cpp',
        'build_ifm_bone_world_palette.cmd','verify_ifm_bone_world_palette.py','model_skinning_geometry.h','scene_geometry_preparation_candidate.h','probe_vulkan_win32_surface_x64.cpp']}
    image,evidence=check_ranges(IFM,'ifm-disassembly.txt',[
        ('complete_original_bone_kernel',0x102411b0,0x1024181f),
        ('complete_setup_bones_wrapper',0x1023ce60,0x1023cf1d),
        ('array_bone_input_preparation',0x10241a50,0x10241bf9),
        ('typed_bone_array_index',0x105c3580,0x105c35ca),
        ('saved_bone_count',0x100d7c80,0x100d7c87),
        ('renderable_vtable_registration',0x10243234,0x10243241),
        ('compute_bounds_registration',0x105c2992,0x105c29b9),
        ('complete_attachment_matrix_query',0x1023c3d0,0x1023c516),
        ('attachment_count_normal_virtual',0x105d9870,0x105d9884),
    ])
    require(image.word(0x10adb904+13*4)==0x1023ce60 and image.word(0x10adb9b8+12*4)==0x10241820 and image.word(0x10adb904+34*4)==0x1023c3d0,'Native vtable routes')
    require(image.string(0x10b9465c)=='computeBounds','Native bool field name')
    executables=[]
    for name,machine,magic in [('palette_native_x86',0x14c,0x10b),('palette_owned_x64',0x8664,0x20b),('palette_safety_x64',0x8664,0x20b),('benchmark_inputs_x64',0x8664,0x20b)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic,'Binary architecture '+name)
        executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,cwd=ROOT,timeout=90)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    disposable=run/'benchmark-copy.dmx';shutil.copyfile(SESSION,disposable);copyhash=sha(disposable)
    benchmark=run/'benchmark-queries.bin';bindings=execute('benchmark_inputs_x64',[disposable,GAME/'usermod/gameinfo.txt',benchmark],'benchmark-inputs')
    require('benchmark_cases=4 bones=136' in bindings and 'input_order=session_array' in bindings,'Benchmark coverage')
    require(bindings.count('procedural_flags=0 compute_bounds=0')==4,'Benchmark branch state changed')
    models=[]
    for path in sorted(set(re.findall(r' path=(.+)',bindings))):
        p=Path(path);raw=p.read_bytes();extra=struct.unpack_from('<i',raw,0x190)[0]
        models.append(dict(path=path,sha256=sha(p),illumination_point=struct.unpack_from('<3f',raw,0x5c),
                           illumination_attachment_index=struct.unpack_from('<i',raw,extra+8)[0] if extra else 0,
                           local_attachment_count=struct.unpack_from('<i',raw,0xf0)[0]))
    require(len(models)==2 and models[0]['illumination_attachment_index']==34 and models[0]['local_attachment_count']==10,'Pyro mounted attachment mismatch changed')
    rng=random.Random(2026102425);rows=[];counts=[]
    def bone(parent=-1,flags=0x100,p=(0,0,0),q=(0,0,0,1),scale=1):return struct.pack('<iI8f',parent,flags,*p,*q,scale)
    def append(bones,root=None,mask=0x7ff00):
        if root is None:root=[1,0,0,0,0,1,0,0,0,0,1,0]
        rows.append(struct.pack('<II12f',len(bones),mask,*root)+b''.join(bones));counts.append(len(bones))
    append([]);append([bone(p=(1,2,3))],[1,0,0,10,0,1,0,20,0,0,1,30])
    append([bone(scale=2),bone(0,p=(4,5,6))]);append([bone(flags=0)]);append([bone(q=(.1,.2,.3,.4),scale=-2)])
    # Selected path uses exact unit comparison, supports negative/zero scales,
    # raw nonunit quaternions, index255 and both bone-use bits. Mask-zero and
    # isolated skipped roots prove untouched cache slots are retained.
    for n in [1,2,3,16,65,256]:
        append([bone(-1 if i==0 else i-1,p=(.25,-.5,1)) for i in range(n)])
        append([bone(flags=0,p=(1,2,3)) for _ in range(n)],mask=0)
    for i in range(2048):
        n=rng.randrange(1,33);data=[]
        for j in range(n):
            angle=rng.uniform(-3,3);q=(0,0,math.sin(angle/2),math.cos(angle/2))
            data.append(bone(-1 if j==0 or rng.randrange(4)==0 else rng.randrange(j),rng.choice([0x100,0x200,0x104,0x204]),
                             [rng.uniform(-32,32) for _ in range(3)],q,rng.choice([-1,0,.5,1,1.0000001192092896,2])))
        root=[rng.uniform(-2,2) for _ in range(12)];append(data,root)
    # Additional selective masks with independent roots avoid native undefined
    # reads of a filtered parent. Parent-filter errors are safety refusals.
    for mask in [0,0x100,0x200,0x7ff00,0xffffffff]:append([bone(flags=f) for f in [0,0x100,0x200,0x300]],mask=mask)
    query=run/'queries.bin';controlled=b''.join(rows);query.write_bytes(controlled+benchmark.read_bytes())
    outputs=[]
    for name in ['palette_native_x86','palette_owned_x64']:
        p=run/(name+'.bin');execute(name,[IFM,query,p],name);outputs.append(p.read_bytes())
    expected_bytes=(sum(counts)+136)*48
    require(len(outputs[0])==len(outputs[1])==expected_bytes,'Palette output length')
    require(outputs[0]==outputs[1],'Bone world matrices differ; preserve first failure for focused diagnosis')
    packed=query.read_bytes();at=0;computed=0;parsed_cases=0
    while at<len(packed):
        count,mask=struct.unpack_from('<II',packed,at);computed+=sum(bool(struct.unpack_from('<I',packed,at+56+i*40+4)[0]&mask) for i in range(count));at+=56+count*40;parsed_cases+=1
    require(at==len(packed) and parsed_cases==len(rows)+4,'Counted query coverage')
    require(struct.unpack_from('<12f',outputs[0],0)==(1,0,0,11,0,1,0,22,0,0,1,33),'Independent translated root')
    safety=execute('palette_safety_x64',[],'safety');require('checks=8 rejections=9' in safety,'Safety coverage')
    warnings={}
    for name in ['native','owned','benchmark','safety']:
        warnings[name]=[x for x in (OUT/(name+'-build.log')).read_text().splitlines() if 'warning ' in x.lower()]
        require(all('C4458' in x and 'graph_transform_hierarchy.h' in x for x in warnings[name]),'Unexpected build warning '+name)
    require(all(sha(p)==h for p,h in originals.items()) and sha(disposable)==copyhash,'Original/copy changed')
    require(all(sha(m['path'])==m['sha256'] for m in models),'Original model changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()),'Source changed')
    require(all(sha(e['path'])==e['sha256'] for e in executables),'Binary changed')
    receipt=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),controlled_cases=len(rows),benchmark_cases=4,
        total_cases=len(rows)+4,matrix_slots=sum(counts)+136,computed_matrices=computed,preserved_slots=sum(counts)+136-computed,exact_output_bytes=expected_bytes,differences=0,
        safety=safety.strip(),warnings=warnings,executables=executables,models=models,original_sha256=originals,
        sources_sha256=sources,query_sha256=sha(query),static_instruction_bytes=sum(e['bytes'] for e in evidence),
        static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],bindings=bindings,
        limitations=[
            'Complete original102411B0 executes unpatched in a sealed private image. Cached header, local inputs, root frame, mask, zero precomputed bitset and resolved model state are controlled.',
            'Procedural evaluation disabled, precomputed override-bone traversal disabled by supplied type predicate, timestamp0. Benchmark actual requested procedural flag istrue, but both mounted MDLs have no procedural flag4 bones.',
            'Existing owned graph evaluator supplies benchmark local values and root matrices. Original session loading/evaluation, Euler root-frame selection, virtual-model mapping, synchronization and cache lifecycle remain unverified.',
            'Session inputs are read by saved array index, not name remapping. Missing/untyped supported inputs default to identity; implicit nonfloat scale conversion and external references refuse.',
            'Pyro stored computeBounds=false/index34/local attachments10 disproves assuming the absent-DAG path requires bounds. Full virtual-model attachment population and native fallback point/root-frame binding remain to verify.',
            'No editor/rendering dependency changed or rebuilt/run; no computer use/live probes/SFM launch/control/close/Blender/original write.',
        ])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['status','total_cases','matrix_slots','computed_matrices','preserved_slots','exact_output_bytes','differences','safety']}));print('receipt='+str(run/'results.json'))


if __name__=='__main__':main()
