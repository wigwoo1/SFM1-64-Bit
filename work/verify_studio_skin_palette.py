"""Original studio palette builder plus actual benchmark CPU binding audit."""
from datetime import datetime,timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,SESSION,SESSION_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/studio-skin-palette-20261025'
GAME=IFM.parent.parent.parent
STUDIO=GAME/'bin/studiorender.dll'
STUDIO_SHA='dc7eef7620f2a687c3cdd4182f2f17f0d78890e2748abcbe923b459b5fa340f7'
IDENTITY=[1,0,0,0,0,1,0,0,0,0,1,0]


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(value,message):
    if not value:raise RuntimeError(message)


def fixture(n,mode,mask,rng):
    size=0x600+n*216+n*52
    mdl=bytearray(size)
    def word(at,v):struct.pack_into('<I',mdl,at,v)
    word(0,0x54534449);word(4,49);word(76,size);word(152,0x10 if mode==2 else 0);word(156,n);word(160,0x600)
    worlds=[];initial=[]
    for i in range(max(n,1)):
        worlds.append([rng.uniform(-100,100) for _ in range(12)])
        initial.append([12345.25]*12)
    flags=[];binds=[]
    for i in range(n):
        flags.append(rng.choice([0,0x100,0x400,0x7ff00,0xffffffff]));binds.append([rng.uniform(-4,4) for _ in range(12)])
        word(0x600+i*216+160,flags[i]);struct.pack_into('<12f',mdl,0x600+i*216+96,*binds[i])
    if mode==1:
        word(400,0x300);word(0x310,0x100);word(0x400,n);word(0x404,0x200+n*216);word(0x418,0x200+n*216+n*4)
        # Deliberately different values prove the selected linear metadata,
        # rather than a legacy table which happens to agree on current assets.
        for i in range(n):
            word(0x600+n*216+i*4,flags[n-1-i]^0x100);struct.pack_into('<12f',mdl,0x600+n*216+n*4+i*48,*[v+1 for v in binds[n-1-i]])
    if mode==2:
        word(156,0xffffffff);word(400,0xffffffff) # static branch must bypass these unused fields
    return struct.pack('<3I',len(mdl),len(worlds),mask)+mdl+b''.join(struct.pack('<12f',*m) for m in worlds+initial)


def query_rows(raw):
    at=0;rows=[]
    while at<len(raw):
        length,n,mask=struct.unpack_from('<3I',raw,at);at+=12
        mdl=raw[at:at+length];at+=length;world=raw[at:at+n*48];at+=n*48;initial=raw[at:at+n*48];at+=n*48
        require(len(initial)==n*48,'Palette query truncated');rows.append(dict(mdl=mdl,world=world,initial=initial,slots=n,mask=mask))
    require(at==len(raw),'Palette query framing');return rows


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    originals={str(p):sha(p) for p in [STUDIO,SESSION,GAME/'usermod/gameinfo.txt']}
    require(originals[str(STUDIO)]==STUDIO_SHA and originals[str(SESSION)]==SESSION_SHA,'Original pins')
    shader=GAME/'platform/shaders/fxc/vertexlit_and_unlit_generic_vs20.vcs'
    shader_receipt=json.loads((ROOT/'outputs/generic-ambient-shader-20261024/extraction.json').read_text())
    originals[str(shader)]=sha(shader)
    require(originals[str(shader)]==shader_receipt['source_sha256']=='0e31c41484207212b069524a079270f5f820059e7d5d4dadc3869bc194d1bd97','Inspected installed shader changed')
    for p in shader_receipt['programs']:
        require(sha(ROOT/'outputs/generic-ambient-shader-20261024'/p['bytecode'])==p['bytecode_sha256'],'Retained inspected shader bytecode changed')
    tracked=['studio_skin_palette_candidate.h','probe_studio_skin_palette.cpp','audit_benchmark_studio_skin.cpp',
        'test_studio_skin_palette.cpp','build_studio_skin_palette.cmd','verify_studio_skin_palette.py',
        'ifm_setup_bones_candidate.h','ifm_bone_world_palette_candidate.h','model_skinning_geometry.h',
        'scene_geometry_preparation_candidate.h','geometry_deformation_worker_pool.h','probe_vulkan_win32_surface_x64.cpp']
    sources={p:sha(ROOT/'work'/p) for p in tracked}
    editor=ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe';editor_sha=sha(editor)
    require(editor_sha=='2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2','Previous editor')
    image,evidence=check_ranges(STUDIO,'studiorender-disassembly.txt',[
        ('complete_skin_palette_selection',0x10001d90,0x10001ea1),
        ('complete_static_matrix_copy',0x10058d70,0x10058db5),
        ('complete_affine_composition',0x10058df0,0x10058ec5),
        ('initial_draw_palette_call_fixed_mask',0x100084fd,0x10008532)])
    require([image.word(0x10071d50+i*4) for i in range(4)]==[0,0,0,0xffffffff],'SIMD translation mask')
    binaries=[];warnings={}
    for tag,name,machine in [('native','skin_native_x86',0x14c),('owned','skin_owned_x64',0x8664),('audit','skin_benchmark_x64',0x8664),('safety','skin_safety_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();warnings[tag]=[x for x in log.splitlines() if 'warning ' in x.lower()]
        require('error C' not in log and all(tag=='audit' and 'C4458' in x and 'graph_transform_hierarchy.h' in x for x in warnings[tag]),'Build diagnostics '+tag)
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Executable architecture')
        binaries.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))

    def execute(name,args,label=None):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/((label or name)+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,name+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout

    safety=execute('skin_safety_x64',[]);require('checks=6 refusals=6' in safety,'Safety coverage')
    rng=random.Random(2026102506);cases=[]
    for n,mode,mask in itertools.product([0,1,2,65,256],[0,1,2],[0,1,0x100,0x400,0x7ff00,0xffffffff]):cases.append(fixture(n,mode,mask,rng))
    for _ in range(1024):cases.append(fixture(rng.randrange(34),rng.randrange(3),rng.getrandbits(32),rng))
    query=run/'queries.bin';query.write_bytes(b''.join(cases));rows=query_rows(query.read_bytes());outputs=[]
    for name in ['skin_native_x86','skin_owned_x64']:
        target=run/(name+'.bin');execute(name,[STUDIO,query,target]);outputs.append(target.read_bytes())
    require(outputs[0]==outputs[1],'Original/owned palette differs; preserve first mismatch')
    at=0;static_cases=0;preserved=0
    for row in rows:
        n=row['slots'];chunk=outputs[0][at:at+n*48];at+=n*48
        if struct.unpack_from('<I',row['mdl'],152)[0]&0x10:
            require(chunk[:48]==row['world'][:48] and chunk[48:]==row['initial'][48:],'Static first-slot copy and unused bypass');static_cases+=1
        elif not row['mask']:
            require(chunk==row['initial'],'Mask0 must preserve output slots');preserved+=n
    require(at==len(outputs[0]),'Output framing')
    source=ROOT/'outputs/benchmark-draw-bounds-20261025/verification-czdinbt0/benchmark-copy.dmx';require(sha(source)==SESSION_SHA,'Disposable session copy pin')
    benchmark=run/'benchmark-queries.bin';geometry=run/'geometry.bin'
    audit=execute('skin_benchmark_x64',[source,GAME/'usermod/gameinfo.txt',benchmark,geometry]);require('PASS studio_skin_benchmark_cases=4 vertices=19618 workers=1,4' in audit,'Benchmark/worker coverage')
    assets=[dict(path=p,sha256=sha(p)) for p in sorted(set(re.findall(r'^asset_path=(.+)$',audit,re.M)))];require(len(assets)==6,'Mounted MDL/VVD/VTX coverage')
    for asset in assets:originals[asset['path']]=asset['sha256']
    previous=ROOT/'outputs/setup-bones-20261025/verification-as7awojd';previous_receipt=json.loads((previous/'results.json').read_text())
    raw_geometry=geometry.read_bytes();pos=0;geometry_models=[];worlds=b'';skins=b'';deltas=[]
    for line in re.findall(r'^binding (.+)$',audit,re.M):deltas.append(dict(re.findall(r'(\w+)=([^ ]+)',line)))
    for i in range(4):
        tick,model,bones,vertices=struct.unpack_from('<4I',raw_geometry,pos);pos+=16
        world=raw_geometry[pos:pos+bones*48];pos+=bones*48;skin=raw_geometry[pos:pos+bones*48];pos+=bones*48
        world_pos_differences=0;normal_differences=0;max_position=0.;max_normal=0.
        for j in range(vertices):
            a=raw_geometry[pos:pos+48];b=raw_geometry[pos+48:pos+96];pos+=96
            av,bv=struct.unpack('<12f',a),struct.unpack('<12f',b)
            world_pos_differences+=a[:12]!=b[:12];normal_differences+=a[12:24]!=b[12:24]
            max_position=max(max_position,*(abs(av[c]-bv[c]) for c in range(3)))
            max_normal=max(max_normal,*(abs(av[c]-bv[c]) for c in range(3,6)))
            require(a[24:32]==b[24:32],'Binding changed UV addressing')
        reported=deltas[i];require(int(reported['tick'])==tick and int(reported['model'])==model and int(reported['changed_positions'])==world_pos_differences and int(reported['changed_normals'])==normal_differences,'Independent comparison counts')
        geometry_models.append(dict(tick=tick,model=model,bones=bones,vertices=vertices,changed_positions=world_pos_differences,changed_normals=normal_differences,max_position_delta=max_position,max_normal_delta=max_normal));worlds+=world;skins+=skin
    require(pos==len(raw_geometry),'Geometry output framing')
    # Previous complete IFM setup was independently compared with this original
    # normal palette baseline. Check current world matrices before studio bind.
    baseline=ROOT/'outputs/precomputed-bone-palette-20261025/verification-50y9kg_7/benchmark-normal_native_x86.bin'
    require(sha(baseline)==previous_receipt['retained_palette_sha256'] and worlds==baseline.read_bytes(),'World matrices differ from original complete setup baseline')
    benchmark_outputs=[]
    for name in ['skin_native_x86','skin_owned_x64']:
        target=run/('benchmark-'+name+'.bin');execute(name,[STUDIO,benchmark,target],'benchmark-'+name);benchmark_outputs.append(target.read_bytes())
    require(benchmark_outputs[0]==benchmark_outputs[1]==skins and len(skins)==6528,'Original studio binding does not match fresh benchmark matrices')
    require(all(sha(p)==h for p,h in originals.items()) and sha(source)==SESSION_SHA,'Original/disposable input changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()) and all(sha(e['path'])==e['sha256'] for e in binaries) and sha(editor)==editor_sha,'Source/binary changed')
    result=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),exact_output_bytes=len(outputs[0]),static_cases=static_cases,zero_mask_preserved_slots=preserved,
        benchmark_cases=4,exact_benchmark_palette_bytes=len(skins),geometry_vertices=19618,worker_counts=[1,4],geometry_models=geometry_models,safety=safety.strip(),warnings=warnings,executables=binaries,
        sources_sha256=sources,original_sha256=originals,assets=assets,query_sha256=sha(query),benchmark_query_sha256=sha(benchmark),geometry_sha256=sha(geometry),
        static_instruction_bytes=sum(e['bytes'] for e in evidence),static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],
        prior_editor=dict(path=str(editor),sha256=editor_sha,rebuilt=False,run=False),limitations=[
            'Complete original studio10001D90, copy and affine composition execute privately without code patches, loader/import initialization or SFM process. Actual mdl bytes/prepared world inputs supplied; no model registry/cache/device lifecycle.',
            'Owned selection preserves static first-slot copy, linear-vs-legacy metadata, masks and untouched slots. Inputs/outputs are separate, finite used matrices; malformed counts/spans/nonfinite used data are owned refusals, not native error semantics.',
            'Fresh two-frame benchmark world matrices equal independently verified original full IFM setup outputs and feed original studio builder. Original shader palette upload/packing is not executed.',
            'Geometry audit uses the SAME existing CPU skinning formula on old/new palettes with original unflexed VVD vertices, without second placement on world-space palette. It quantifies binding/arithmetic differences; it does NOT establish native vertex-shader skinning, packed-weight/strip-bone mapping, flex deformation or rendered pixel parity.',
            'The previously inspected static0/dynamic6 generic VS20 performs weighted matrix blending before dot products and reconstructs residual third weight; installed cache/program hashes are rechecked. Current CPU loop blends transformed points using stored VVD weights. Packed-weight/strip-bone mapping and active program selection remain unverified.',
            'No editor dependency changed or complete editor built/run; no viewport/lighting change, computer use, original process access/launch/control/close, original writes, Blender, GPU job, install or publication.',
        ])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','exact_output_bytes','benchmark_cases','exact_benchmark_palette_bytes','geometry_vertices']},indent=2));print('receipt='+str(run/'results.json'))


if __name__=='__main__':main()
