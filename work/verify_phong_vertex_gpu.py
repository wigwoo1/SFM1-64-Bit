"""Actual x64 D3D9 HAL execution of unmodified installed Phong vertex programs.

GPU/CPU values are compared with fixed, predeclared numeric bounds. This is
vertex-output verification on the recorded adapter/driver, not pixel shading,
native material initialization, cross-driver proof or a Vulkan implementation.
"""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,math,struct,subprocess,tempfile
import verify_phong_shader_vertex_skin as phong
from vcs6_shader_cache_candidate import Vcs6ShaderCache

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/phong-vertex-gpu-20261025'
# Fixed before broad GPU execution; never widened to accommodate an outcome.
POSITION_ABS_TOLERANCE=0.005
DIRECTION_ABS_TOLERANCE=0.00002

def require(b,s):
    if not b:raise RuntimeError(s)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def ordered_float(x):
    word=struct.unpack('<I',struct.pack('<f',x))[0]
    return 0x80000000-(word&0x7fffffff) if word&0x80000000 else 0x80000000+word

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    prior_dir=ROOT/'outputs/phong-vertex-variants-20261025/verification-9wa6792k';prior=json.loads((prior_dir/'results.json').read_text())
    query_source=prior_dir/'variant-vertex-token-reference.bin';catalog_file=prior_dir/'catalog.json'
    require(sha(query_source)==prior['vertex_fixture_sha256'] and sha(catalog_file)==prior['catalog_sha256'],'Prior variant evidence')
    for name,pin in prior['sources_sha256'].items():require(sha(ROOT/'work'/name)==pin,'Prior variant source changed '+name)
    catalog=json.loads(catalog_file.read_text());admitted=[p for p in catalog if p['status']=='STREAM_SKIN_ADMITTED'];require(len(admitted)==88,'Admitted variant count')
    originals={};caches={};shader_dir=run/'shaders';shader_dir.mkdir()
    for version,(name,pin) in phong.SOURCES.items():
        path=phong.GAME/'platform/shaders/fxc'/name;originals[str(path)]=sha(path);require(originals[str(path)]==pin,'Installed shader pin');caches[version]=Vcs6ShaderCache(path.read_bytes())
    programs={}
    for p in admitted:
        key=p['version'],p['static'],p['dynamic'];shader=caches[key[0]].shader(key[1],key[2]);digest=hashlib.sha256(shader).hexdigest();require(digest==p['sha256'],'Actual installed shader differs from prior admitted bytes')
        path=shader_dir/f'phong-vs{key[0]}-s{key[1]}-d{key[2]}.bin';path.write_bytes(shader);programs[key]=dict(path=str(path),sha256=digest)
    exe=OUT/'phong_vertex_gpu_x64.exe';build_log=(OUT/'build.log').read_text();require('warning ' not in build_log.lower() and 'error ' not in build_log.lower(),'Build diagnostics')
    b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'AMD64 PE32+ required')
    rows=[];metadata=[];analytical=0
    def add(version,static,dynamic,q,label):
        nonlocal analytical
        require(len(q)==272 and (version,static,dynamic) in programs,'GPU query profile/size')
        i=len(rows)%53;indices=(i,(i+17)%53,(i+35)%53);packed=indices[0]|(indices[1]<<8)|(indices[2]<<16)
        rows.append(struct.pack('<4I',version,static,dynamic,packed)+q);metadata.append(dict(version=version,static=static,dynamic=dynamic,kind=label,indices=indices))
        analytical+=label=='analytical'
    # Every palette slot, each influence and residual selection, negative
    # position output, signed SHORT2 bias, D3DCOLOR and packed frame conversion.
    for version,static in [(20,3),(30,4)]:
        for compressed in [0,1]:
            for index in range(53):
                matrices=[];translations=[]
                for bone in range(3):
                    t=(index+bone*100,-index-bone*20,index*0.5+bone*2);translations.append(t)
                    matrices.extend([1,0,0,t[0],0,1,0,t[1],0,0,1,t[2]])
                for w0,w1 in [(1,0),(0,1),(0,0),(0.25,0.5)]:
                    weights=struct.pack('<2h',int(w0*32768)-1,int(w1*32768)-1)+b'\xa5'*4 if compressed else struct.pack('<2f',w0,w1)
                    normal=bytes([192,192,255,63])+b'\xa5'*8 if compressed else struct.pack('<3f',0,0,1)
                    tangent=b'\xa5'*16 if compressed else struct.pack('<4f',1,0,0,-1)
                    w2=1-w1-w0;position=[(1,2,3)[j]+w0*translations[0][j]+w1*translations[1][j]+w2*translations[2][j] for j in range(3)]
                    expected=[*position,0,0,1,1,0,0,0,-1,0]
                    q=struct.pack('<I36f',(2 if version==30 else 0)+compressed,*matrices)+weights+normal+tangent+struct.pack('<10f',1,2,3,*([0]*7))+struct.pack('<12f',*expected)
                    add(version,static,2+compressed,q,'analytical')
    source=query_source.read_bytes();require(len(source)==prior['expanded_vertex_cases']*272,'Prior query framing')
    per_slot=64+2*prior['benchmark_sample_vertices'];require(per_slot*len(admitted)==prior['expanded_vertex_cases'],'Prior per-slot grouping')
    offset=0
    for p in admitted:
        for i in range(per_slot):
            q=source[offset:offset+272];offset+=272;profile=struct.unpack_from('<I',q)[0]
            require(profile==(2 if p['version']==30 else 0)+(p['dynamic']&1),'Grouped CPU profile identity')
            add(p['version'],p['static'],p['dynamic'],q,'native_synthetic' if i<64 else 'benchmark')
    query=run/'gpu-queries.bin';query.write_bytes(b''.join(rows));(run/'query-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(f'Prepared GPU cases={len(rows)} analytical={analytical} shader_slots={len(programs)}',flush=True)
    gpu_output=run/'gpu'
    result=subprocess.run([str(exe),str(shader_dir),str(query),str(gpu_output)],cwd=ROOT,capture_output=True,text=True,timeout=300)
    (run/'gpu.log').write_text(result.stdout+result.stderr);print(result.stdout,flush=True)
    require(result.returncode==0,'GPU fixture failed '+str(result.returncode)+' '+result.stderr)
    require(f'PASS gpu_vertex_cases={len(rows)} shader_slots=88 float_targets=4 exact_point_pixels=1' in result.stdout and 'PASS resources released' in result.stdout,'GPU execution coverage/cleanup')
    actual_bytes=(gpu_output/'gpu-world-frames.bin').read_bytes();require(len(actual_bytes)==len(rows)*64,'GPU frame extent')
    groups={name:dict(max_abs_error=0.0,max_ulp_distance=0,exact_components=0,components=0,worst_case=None) for name in ['position','normal','tangent','binormal']}
    violations=[];analytical_position_components=0;analytical_position_exact=0
    for i,row in enumerate(rows):
        expected=struct.unpack_from('<12f',row,16+224);gpu=struct.unpack_from('<16f',actual_bytes,i*64)
        for group,name in enumerate(groups):
            require(gpu[group*4+3]==1,'Capture alpha/correspondence');stats=groups[name]
            for c in range(3):
                a=gpu[group*4+c];e=expected[group*3+c];require(math.isfinite(a) and math.isfinite(e),'Nonfinite GPU/reference')
                error=abs(a-e);ulp=abs(ordered_float(a)-ordered_float(e));stats['components']+=1;stats['exact_components']+=a==e;stats['max_ulp_distance']=max(stats['max_ulp_distance'],ulp)
                if error>stats['max_abs_error']:stats['max_abs_error']=error;stats['worst_case']=dict(case=i,component=c,expected=e,actual=a,profile=metadata[i])
                bound=POSITION_ABS_TOLERANCE if group==0 else DIRECTION_ABS_TOLERANCE
                if error>bound:violations.append(dict(case=i,group=name,component=c,error=error,bound=bound))
                if metadata[i]['kind']=='analytical' and group==0:analytical_position_components+=1;analytical_position_exact+=a==e
    comparison=dict(status='PASS' if not violations and analytical_position_exact==analytical_position_components else 'FAIL',position_abs_tolerance=POSITION_ABS_TOLERANCE,direction_abs_tolerance=DIRECTION_ABS_TOLERANCE,groups=groups,violations=violations,analytical_position_components=analytical_position_components,analytical_position_exact=analytical_position_exact)
    (run/'comparison.json').write_text(json.dumps(comparison,indent=2)+'\n')
    require(not violations,'GPU/CPU exceeds fixed bounds; inspect comparison.json, do not widen thresholds')
    require(analytical_position_exact==analytical_position_components,'Analytical palette selection/position must be exact')
    for path,pin in originals.items():require(sha(path)==pin,'Original shader changed')
    for p in programs.values():require(sha(p['path'])==p['sha256'],'Extracted program changed')
    device={}
    for line in (gpu_output/'device.txt').read_text().splitlines():
        key,value=line.split('=',1);device[key]=value
    require(device['window_visible']=='0' and device.get('driver_version') and device.get('driver'),'Hidden GPU window and driver identity')
    previous_receipt=OUT/'verification-4w3ky4ei/results.json';repeat=None
    if previous_receipt.exists():
        previous=json.loads(previous_receipt.read_text());require(previous['status']=='PASS' and previous['gpu_query_sha256']==sha(query),'Repeat input identity')
        repeat=dict(previous_receipt=str(previous_receipt),same_query_bytes=True,same_gpu_result_bytes=previous['gpu_output_sha256']==sha(gpu_output/'gpu-world-frames.bin'))
    sources=['probe_phong_vertex_gpu.cpp','build_phong_vertex_gpu.cmd','verify_phong_vertex_gpu.py']
    receipt=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),gpu_vertex_cases=len(rows),analytical_vertex_cases=analytical,analytical_position_exact_components=analytical_position_exact,expanded_prior_vertex_cases=prior['expanded_vertex_cases'],benchmark_vertex_cases=sum(m['kind']=='benchmark' for m in metadata),shader_slots=len(programs),device=device,
        comparison=comparison,executable=dict(path=str(exe),sha256=sha(exe),machine='AMD64',format='PE32+'),build_log=str(OUT/'build.log'),gpu_log=str(run/'gpu.log'),sources_sha256={name:sha(ROOT/'work'/name) for name in sources},originals_sha256=originals,originals_unchanged=True,programs=list(programs.values()),
        gpu_query_sha256=sha(query),gpu_output_sha256=sha(gpu_output/'gpu-world-frames.bin'),metadata_sha256=sha(run/'query-metadata.json'),capture_pixel_program_sha256={name:sha(gpu_output/name) for name in ['capture-ps20.bin','capture-ps30.bin']},prior_variant_receipt=str(prior_dir/'results.json'),repeat_comparison=repeat,
        computer_use=False,sfm_process_used=False,gpu_used=True,editor_changed=False,
        limitations='Actual HAL GPU output on the recorded device/driver with unmodified installed vertex programs and owned capture-only pixel shaders. Fixed numeric error bounds, not bitwise GPU/CPU equality; ULP distances near zero can be large despite tiny absolute error. Verified declaration builder uses explicit level95/uncompressed flex, streams0/2; this is not proof of active original declaration choice. Three selected matrices are remapped to controlled distinct hardware slots; original strip submission is not exercised. Clip projection is controlled to capture one point per case without altering the world-frame outputs. Four target RGBA32F capture excludes full material/pixel/lighting behavior, motion/timeline fidelity, live effective parameters and active variant. No Vulkan translation or production editor integration. Original SFM process is untouched.')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),gpu_cases=len(rows),max_abs_error={k:v['max_abs_error'] for k,v in groups.items()})))

if __name__=='__main__':main()
