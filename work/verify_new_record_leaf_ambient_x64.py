"""Offline new-record composition and actual BSP-cube to owned GPU tests."""
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from verify_source_leaf_ambient_interpolation import reference
from verify_ifm_draw_packet import check_ranges

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/new-record-leaf-ambient-20261024'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
MAP=GAME/'tf_movies/maps/mtt_soldier.bsp'
SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
ENGINE=GAME/'bin/engine.dll'
ENGINE_HASH='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
PRIOR=ROOT/'outputs/new-cache-record-anchor-20261015/verification-u9arua50'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(value,message):
    if not value:raise RuntimeError(message)


def controlled_fixture():
    b=bytearray(1036);struct.pack_into('<II',b,0,0x50534256,20)
    def append(index,version,payload):
        struct.pack_into('<IIII',b,8+16*index,len(b),len(payload),version,0);b.extend(payload)
    leaves=bytearray(64)
    for i in range(2):struct.pack_into('<6h',leaves,i*32+8,-128,-128,-128,128,128,128)
    append(10,1,leaves);index=struct.pack('<4H',1,0,1,1)
    append(51,0,index);append(52,0,index)
    def samples(first):
        payload=bytearray()
        for model in range(2):
            for face in range(6):payload.extend(bytes([first+model*100,20+face,30+face,0]))
            payload.extend(bytes([127,127,127,0]))
        return payload
    append(55,1,samples(10));append(56,1,samples(1))
    return b


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    originals={str(p):sha(p) for p in [ENGINE,MAP,SESSION,GAME/'usermod/gameinfo.txt']}
    require(originals[str(ENGINE)]==ENGINE_HASH,'Pinned engine changed')
    require(originals[str(MAP)]=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Pinned map changed')
    require(originals[str(SESSION)]=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','Pinned benchmark changed')
    source_names=['source_new_record_leaf_ambient_candidate.h','test_new_record_leaf_ambient.cpp',
                  'probe_new_record_leaf_ambient.cpp','test_benchmark_new_record_ambient.cpp',
                  'build_new_record_leaf_ambient_x64.cmd','verify_new_record_leaf_ambient_x64.py',
                  'source_light_cache_new_record_anchor_candidate.h','source_bsp_light_cache_world_candidate.h',
                  'source_leaf_ambient_interpolation_candidate.h','scene_model_ambient_records_candidate.h',
                  'prepared_session_scene_candidate.h','probe_vulkan_win32_surface_x64.cpp']
    sources={p:sha(ROOT/'work'/p) for p in source_names}
    ranges=[('initial_leaf_save',0x101977a8,0x101977f8),
            ('input_leaf_restored_to_ebx',0x1019791b,0x10197929),
            ('sample_point_and_original_leaf_forwarded',0x101979c6,0x101979e3),
            ('ambient_wrapper_arguments',0x101969e4,0x10196a0f),
            ('mode4_false_leaf_interpolation',0x1019323f,0x10193258)]
    _,evidence=check_ranges(ENGINE,'engine-disassembly.txt',ranges)
    rows={row['address']:row['instruction'] for span in evidence for row in span['instructions']}
    for va,text in [(0x101977b5,'call        10145660'),
                    (0x101977f2,'mov         dword ptr [ebp+FFFFFF74h],eax'),
                    (0x10197923,'mov         ebx,dword ptr [ebp+FFFFFF74h]'),
                    (0x101979c6,'lea         esi,[edi+00000198h]'),
                    (0x101979d5,'push        ebx'),(0x101979d6,'push        esi'),
                    (0x101979de,'call        101969A0'),
                    (0x101969fb,'mov         edx,dword ptr [ebp+10h]'),
                    (0x101969ff,'mov         ecx,dword ptr [ebp+0Ch]'),
                    (0x10196a0a,'call        10193170'),(0x1019324a,'call        100E15F0')]:
        require(''.join(rows[va].split()).lower()==''.join(text.split()).lower(),'Static composition binding changed '+hex(va))
    executables=[]
    for name in ['test_new_record_leaf_ambient','probe_new_record_leaf_ambient','test_benchmark_new_record_ambient']:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'AMD64/PE32+ required')
        executables.append(dict(path=str(exe),sha256=sha(exe),freshly_built=True))
    def execute(exe,args,label):
        p=subprocess.run([str(exe),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=180)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr)
        return p.stdout
    fixture=run/'controlled.bsp';fixture.write_bytes(controlled_fixture())
    safety=execute(OUT/'test_new_record_leaf_ambient.exe',[fixture],'safety')
    require('controlled_compositions=11 rejections=20' in safety,'Controlled branch coverage')
    previous=json.loads((PRIOR/'results.json').read_text())
    require(previous['status']=='PASS' and sha(PRIOR/'queries.bin')==previous['query_sha256'],'Prior anchor query provenance')
    oldq=(PRIOR/'queries.bin').read_bytes();oldn=(PRIOR/'native_x86.bin').read_bytes();oldo=(PRIOR/'owned_x64.bin').read_bytes()
    require(previous['cases']==87035 and previous['cache_center_settings']==[0,1,2,-1,-2147483648], 'Prior new-record mode coverage')
    require(len(oldq)==87035*36 and len(oldn)==87035*92 and oldn==oldo,'Prior exact anchor evidence mismatch')
    for exe in previous['executables']:
        require(sha(exe['executable'])==exe['sha256'],'Prior anchor executable changed')
    map_bytes=MAP.read_bytes();selected=sorted({0,1,2,17406,*range(0,17407,137)})
    mode_queries=[];moved=0
    for i in selected:
        # Use mode1 from the later verified five-mode corpus. Earlier anchor
        # executables were rebuilt after their receipt; do not bypass that
        # provenance failure by accepting their new hashes silently.
        source_index=i*5+1
        point=struct.unpack_from('<3f',oldq,source_index*36);leaf=struct.unpack_from('<I',oldn,source_index*92)[0]
        centered=struct.unpack_from('<3f',oldn,source_index*92+80)
        moved+=point!=centered
        for center in [0,1,2,-1,-2147483648]:
            anchor=point if center==0 else centered
            mode_queries.append((point,center,leaf,anchor))
    require(moved>0,'No recentering coverage')
    reference_hashes={}
    for encoding in ['hdr','ldr']:
        rows=[]
        for point,center,leaf,anchor in mode_queries:
            colors=reference(map_bytes,encoding,leaf,anchor)
            rows.append(struct.pack('<3fiI3f18I',*point,center,leaf,*anchor,*colors))
        oracle=run/('composite-'+encoding+'-reference.bin');oracle.write_bytes(b''.join(rows));reference_hashes[encoding]=sha(oracle)
        composite=execute(OUT/'probe_new_record_leaf_ambient.exe',[MAP,encoding,oracle],'composite-'+encoding)
        require('map_compositions='+str(len(rows)) in composite,'Composition coverage mismatch')
    copy=run/'benchmark-input.dmx';copy.write_bytes(SESSION.read_bytes())
    records=run/'bsp-bound-model-records.bin'
    benchmark=execute(OUT/'test_benchmark_new_record_ambient.exe',[copy,GAME/'usermod/gameinfo.txt',MAP,records],'benchmark')
    require('bsp_hdr_cubes=4 bsp_ldr_cubes=4' in benchmark and 'unbound_model_fallbacks=0 geometry_unchanged=1' in benchmark,'Benchmark scope coverage')
    bindings=[]
    for match in re.finditer(r'binding tick=(\d+) model=(\d+) id=(\d+) dag=(\d+) encoding=(hdr|ldr) cache_center=(\d+) input_leaf=(\d+) query=([^ ]+) sample=([^ ]+) cube=([^\n]+)',benchmark):
        tick,model,element,dag=map(int,match.groups()[:4]);encoding=match[5];center,leaf=int(match[6]),int(match[7])
        point=[float(x) for x in match[8].split(',')];anchor=[float(x) for x in match[9].split(',')]
        colors=[float(x) for x in match[10].strip().split(',') if x]
        require(len(colors)==18 and all(math.isfinite(x) for x in colors),'Cube output shape')
        require(struct.pack('<18f',*colors)==struct.pack('<18I',*reference(map_bytes,encoding,leaf,anchor)),'Bound-model cube differs from independent raw BSP reference')
        bindings.append(dict(tick=tick,model=model,element=element,dag=dag,encoding=encoding,cache_center=center,input_leaf=leaf,query=point,sample=anchor,cube=colors))
    require(len(bindings)==8 and {r['tick'] for r in bindings}=={50000,50417} and {r['encoding'] for r in bindings}=={'hdr','ldr'},'DAG/frame/mode/encoding coverage')
    prior_gpu=json.loads((ROOT/'outputs/scene-model-ambient-20261024/verification-9hq7kbj4/results.json').read_text())
    gpu=next(b for b in prior_gpu['executables'] if Path(b['path']).stem=='probe_scene_model_ambient_vulkan')
    require(sha(gpu['path'])==gpu['sha256'],'Verified GPU probe changed')
    shader=ROOT/'outputs/scene-model-ambient-20261024/ambient-readback.spv'
    require(sha(shader)==prior_gpu['shader_sha256'],'Verified shader changed')
    gpu_output=run/'bsp-bound-model-gpu.bin';gpu_log=execute(Path(gpu['path']),[shader,records,gpu_output],'gpu')
    count=records.stat().st_size//112
    require(count>0 and records.stat().st_size==count*112 and gpu_output.stat().st_size==count*16,'GPU data shape')
    require('records='+str(count) in gpu_log,'GPU BSP-cube coverage')
    require(all(sha(p)==h for p,h in originals.items()),'Original input changed')
    require(sha(copy)==originals[str(SESSION)],'Disposable input changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()),'Source changed during tests')
    require(all(sha(b['path'])==b['sha256'] for b in executables),'Test binary changed during tests')
    warnings={}
    for exe in executables:
        log=(OUT/(Path(exe['path']).stem+'-build.log')).read_text()
        require('error C' not in log,'Build error')
        warnings[Path(exe['path']).stem]=[x for x in log.splitlines() if 'warning C' in x]
    require(not warnings['test_new_record_leaf_ambient'] and not warnings['probe_new_record_leaf_ambient'],'New component warnings')
    require(all(('warning C4458' in w and 'graph_transform_hierarchy.h' in w) or ('warning C4996' in w and 'model_render_inputs_candidate.h' in w) for w in warnings['test_benchmark_new_record_ambient']),'Unexpected new benchmark warning')
    lump_info={str(i):dict(zip(['offset','length','version','fourcc'],struct.unpack_from('<4I',map_bytes,8+16*i))) for i in [10,51,52,55,56]}
    receipt=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),controlled_compositions=11,guard_rejections=20,
        original_anchor_points=len(selected),centered_points_moved=moved,compositions=len(mode_queries)*2,exact_cube_channel_comparisons=len(mode_queries)*2*18,
        benchmark_bindings=bindings,actual_bsp_gpu_records=count,gpu_log=gpu_log.strip(),executables=executables,
        reused_gpu_executable=gpu,shader_sha256=sha(shader),sources_sha256=sources,originals_sha256=originals,build_warnings=warnings,
        query_reference_sha256=reference_hashes,native_anchor_input_sha256=sha(PRIOR/'queries.bin'),native_anchor_output_sha256=sha(PRIOR/'native_x86.bin'),
        bsp_ambient_lumps=lump_info,static_instruction_bytes=sum(e['bytes'] for e in evidence),
        static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],
        record_sha256=sha(records),gpu_output_sha256=sha(gpu_output),
        limitations=[
            'Fresh-record leaf mode only: radiosity4, branch boolean false, engine state49/4A not equal1. All settings explicit, not measured live.',
            'Historical byte-exact original/owned anchor corpus reused and revalidated; original code not freshly executed in this run. Cube oracle independently reads raw BSP with float32 arithmetic.',
            'HDR and LDR chosen explicitly; automatic native HDR/LDR selection and fallback remain unverified. Both real-map datasets and distinct controlled datasets exercised.',
            'Only exact in-scene DmeDag illumination binding supplied by owned evaluated graph is used; Pyro missing-attribute fallback, native poses, cache hits/reuse/refresh and other lighting paths remain incomplete.',
            'Actual map-sampled cubes replace synthetic cubes in this GPU test, but no active native-runtime lighting choice or native visual parity is claimed.',
            'Editor not changed/rebuilt/run; no computer use, native process enumeration/probes/launches, Blender or original writes.',
        ])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['status','compositions','exact_cube_channel_comparisons','actual_bsp_gpu_records','static_instruction_bytes']}))
    print('receipt='+str(run/'results.json'))


if __name__=='__main__':main()
