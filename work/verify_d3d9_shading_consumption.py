"""Independent barycentric oracle for the opt-in owned x64 shading stream."""
from pathlib import Path
from datetime import datetime,timezone
import json,math,os,re,struct,subprocess,tempfile,shutil
from verify_session_preview_aspect import ROOT,VS,DUMP
from verify_anisotropic_bitness import sha,need,put,pe
from verify_texture_sampling_cross_api import fixture
OUT=ROOT/'outputs/d3d9-shading-consumption-20261101'
TOLERANCE=2e-6  # Declared before measurement; no fitted/tolerance-growth correction.

def main():
    before=json.loads((OUT/'before.json').read_text());parent=before['parent'];need(sha(parent['receipt'])==parent['receipt_sha256'],'parent checkpoint');prior=json.loads(Path(parent['receipt']).read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);t=before['transition'];need(sha(t['backup'])==t['before_sha256']==sources[t['path']],'explicit header before bytes');t=dict(t,after_sha256=sha(t['path']));sources[t['path']]=t['after_sha256'];protected[parent['receipt']]=parent['receipt_sha256'];protected[t['backup']]=t['before_sha256'];seen=set()
    additional=[]
    for item in before.get('additional_transitions',[]):
        need(sha(item['backup'])==item['before_sha256']==sources[item['path']],'additional explicit before bytes');item=dict(item,after_sha256=sha(item['path']));sources[item['path']]=item['after_sha256'];protected[item['backup']]=item['before_sha256'];additional.append(item)
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p)
        if str(p) in sources:need(sources[str(p)]==sha(p),'unchanged/declared source')
        sources[str(p)]=sha(p)
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/name).exists():visit(p.parent/name)
    for n in ['probe_d3d9_shading_consumption_x64.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    for group in (sources,protected):
        for p,h in group.items():need(sha(p)==h,'before identity '+p)
    run=Path(tempfile.mkdtemp(prefix='r-',dir=OUT));print(str(run),flush=True);vtf=run/'owned.vtf';vtf.write_bytes(fixture(0,0));bsp_source=ROOT/'outputs/scene-shading-attributes-20261101/verification-ek3hxudu/cpu-data/maps/shading.bsp';need(sha(bsp_source)==protected[str(bsp_source)],'protected owned BSP fixture');bsp=run/'owned.bsp';shutil.copyfile(bsp_source,bsp)
    rows=[]
    def make(mode,perspective,constant,seed,world=False,aligned=False):
        camera=[1,0,0,0,0,1,0,0,0,0,.5 if perspective else 1,0,0,0,1 if perspective else 0,0 if perspective else 1];models=[]
        # Base-plane/style colors from the independently placed owned BSP fixture.
        colors=[[(16+8)/255,(32+16)/255,(48+24)/255],[(64+8)/255,(96+16)/255,(128+24)/255],[(16+64+32+96)/4/255+8/255,(32+96+40+112)/4/255+16/255,(48+128+48+128)/4/255+24/255]]
        for model in range(3):
            points=[(-1,-1),(0,-1),(-1,1)] if model!=2 else [(1,1),(0,1),(1,-1)];vertices=[]
            for i,(x,y) in enumerate(points):
                j=0 if constant else i
                normal=[-.75+seed/16+j*.25,-.5+model/8+j*.5,1.5-seed/16-j*.25]
                tangent=[1.25+seed/16+j*.125,-.75+model/8+j*.25,.5+j*.5,-1. if model==0 else 1.]
                color=[.875,.25,.125]
                if world:normal=[0.,0.,1.];color=colors[i]
                position=[x,y,2.5+x*.5+y*.25 if perspective else .5]
                if aligned:
                    w=[2.,4.,8.][i];position=[x*w,y*w,w]
                vertices.append({'position':position,'normal':normal,'tangent':tangent,'preview':color,'has_tangent':not world})
            models.append(vertices)
        rows.append({'mode':mode,'world':world,'perspective':perspective,'constant':constant,'seed':seed,'aligned':aligned,'camera':camera,'models':models})
    for perspective in (False,True):
        for constant in (False,True):
            for seed in range(8):
                for mode in (1,2):make(mode,perspective,constant,seed)
    for perspective in (False,True):make(1,perspective,True,0,True)
    # Keep all original arbitrary projections above. Extra dyadic screen-space
    # vertices isolate interpolation from uncertain raster setup quantization.
    for constant in (False,True):
        for seed in range(8):
            for mode in (1,2):make(mode,True,constant,seed,aligned=True)
    put(run/'cases.json',rows);query=run/'q.bin';data=struct.pack('<I',len(rows))
    for r in rows:
        data+=struct.pack('<2I16f',r['mode'],int(r['world']),*r['camera'])
        for model in r['models']:
            for v in model:data+=struct.pack('<13fI',*v['position'],*v['normal'],*v['tangent'],*v['preview'],int(v['has_tangent']))
    query.write_bytes(data);inputs={str(p):sha(p) for p in [vtf,bsp,query]};exe=run/'shading.exe';batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_d3d9_shading_consumption_x64.cpp /Fo:"{run/"shading.obj"}" /Fe:"{exe}" d3d9.lib d3dcompiler.lib user32.lib >"{run/"build.log"}" 2>&1','exit /b %errorlevel%','']))
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=clean,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+(p.stdout+p.stderr)[-1600:]);return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',batch],'build-driver');need(pe(exe)==(0x8664,0x20b),'native proof architecture');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(len(warnings)==4 and sorted(x for x,_ in warnings)==['C4458','C4996','C4996','C4996'],'only four inherited warnings');logs=[]
    for repeat in range(2):logs.append(execute([exe,query,vtf,bsp,run/f'd{repeat}'],f'd{repeat}'));print('Diagnostic GPU repeat'+str(repeat)+' complete',flush=True)
    raw=(run/'d0/pixels.bin').read_bytes();need(raw==(run/'d1/pixels.bin').read_bytes(),'exact diagnostic repeatability');need(all(raw==(run/f'd{n}/reference.bin').read_bytes() for n in range(2)),'independent native reference byte-exact');metrics={'pixels':0,'components':0,'max_error':0.,'over_budget':0,'boundary_pixels_excluded':0,'constant_ortho_pixels_exact':0,'world_normal_pixels':0};strict={'pixels':0,'components':0,'over_budget':0,'max_error':0.};examples=[]
    def projected(vertex,camera):
        p=vertex['position']+[1];v=[sum(camera[4*r+k]*p[k] for k in range(4)) for r in range(4)];return v[0]/v[3],v[1]/v[3],v[3]
    at=0
    for serial,r in enumerate(rows):
        need(struct.unpack_from('<2I',raw,at)==(serial,r['mode']),'diagnostic framing');at+=8;pixels=struct.unpack_from('<4096f',raw,at);at+=16384
        for y in range(32):
            for x in range(32):
                sx,sy=2*(x+.5)/32-1,2*(y+.5)/32-1;expected=[0.,0.,0.,0.];inside=False;boundary=False;uniform=None
                for model in (0,2):
                    vertices=r['models'][model];v=[projected(p,r['camera']) for p in vertices];(ax,ay,_),(bx,by,_),(cx,cy,_)=v;den=(by-cy)*(ax-cx)+(cx-bx)*(ay-cy);l0=((by-cy)*(sx-cx)+(cx-bx)*(sy-cy))/den;l1=((cy-ay)*(sx-cx)+(ax-cx)*(sy-cy))/den;weights=[l0,l1,1-l0-l1]
                    if min(weights)<-1e-7:continue
                    if min(abs(w) for w in weights)<1e-7:boundary=True;break
                    inside=True;weights=[w/p[2] for w,p in zip(weights,v)];total=sum(weights);weights=[w/total for w in weights];attrs=[p['normal']+[1.] if r['mode']==1 else p['tangent'] for p in vertices];expected=[sum(w*a[k] for w,a in zip(weights,attrs)) for k in range(4)];uniform=attrs[0];break
                if boundary:metrics['boundary_pixels_excluded']+=1;continue
                actual=pixels[4*(y*32+x):4*(y*32+x)+4];need(all(math.isfinite(a) for a in actual),'finite raw attribute output');metrics['pixels']+=1
                strict_case=not r['perspective'] or r['aligned'] or r['constant']
                if strict_case:strict['pixels']+=1
                for channel,(a,b) in enumerate(zip(actual,expected)):
                    error=abs(a-b);metrics['components']+=1;metrics['max_error']=max(metrics['max_error'],error);metrics['over_budget']+=int(error>TOLERANCE)
                    if strict_case:strict['components']+=1;strict['max_error']=max(strict['max_error'],error);strict['over_budget']+=int(error>TOLERANCE)
                    if error>TOLERANCE and len(examples)<12:examples.append({'case':serial,'pixel':[x,y],'channel':channel,'actual':a,'expected':b,'error':error})
                if inside and r['constant'] and not r['perspective']:need(tuple(actual)==tuple(uniform),'exact raw constant attribute (no clamp/gamma/normalization)');metrics['constant_ortho_pixels_exact']+=1
                if inside and r['world']:metrics['world_normal_pixels']+=1;need(max(abs(a-b) for a,b in zip(actual,[0,0,1,1]))<TOLERANCE,'true world normal differs from preview color')
    need(at==len(raw),'complete diagnostic capture')
    diag={'status':'PARTIALLY VERIFIED: native reference exact and aligned oracle passes; original ideal arbitrary-perspective discrepancy remains' if metrics['over_budget'] else 'VERIFIED GPU shading input','time_utc':datetime.now(timezone.utc).isoformat(),'parent':parent,'transition':t,'additional_transitions':additional,'executable':{'path':str(exe),'sha256':sha(exe)},'cases':len(rows),'repetitions':2,'tolerance':TOLERANCE,'metrics':metrics,'strict_ortho_constant_aligned_oracle':strict,'independent_interleaved_native_reference_byte_exact':True,'mismatch_examples':examples,'logs':logs,'compiler_warnings':warnings,'source_sha256':sources,'protected_sha256':protected,'input_sha256':inputs};diag_receipt=run/'diagnostic.json';put(diag_receipt,diag);put(OUT/'latest-diagnostic.json',{'receipt':str(diag_receipt),'receipt_sha256':sha(diag_receipt)});need(not strict['over_budget'],'aligned/raw attribute oracle mismatch; preserve and escalate rather than fit')
    # Rerun the unchanged730-case default path with the declared current header.
    import verify_d3d9_prepared_scene as default
    prev_result=json.loads(Path(prior['verification']['receipt']).read_text());previous_gpu=json.loads(Path(prev_result['indexed_gpu_receipt']['receipt']).read_text());directory=run/'g';directory.mkdir();put(directory/'before.json',{'parent':previous_gpu['parent'],'source_sha256':sources,'protected_sha256':protected,'actual_parent':parent,'explicit_transition':t});default.OUT=directory;default.main();default_ref=json.loads((directory/'latest-verification.json').read_text());need(sha(default_ref['receipt'])==default_ref['receipt_sha256'],'default proof receipt');gd=json.loads(Path(default_ref['receipt']).read_text());need(gd['cases']==730 and all(m['over_2e_6']==0 for m in gd['metrics'].values()),'default730 reference gate')
    # No edited header is in the actual complete editor dependency set, nor the
    # seven CPU-test quote-include closures. Preserve prior binaries/evidence.
    editor_deps=prior['complete_editor_dependencies_sha256'];need(t['path'] not in editor_deps and all(sha(p)==h for p,h in editor_deps.items()),'all366 complete editor inputs unchanged')
    cpu=json.loads(Path(prev_result['cpu_receipt']['receipt']).read_text());cpu_closures={}
    for name in cpu['executables']:
        closure={}
        def scan(p):
            p=p.resolve()
            if str(p) in closure:return
            closure[str(p)]=sha(p)
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
                if (p.parent/inc).exists():scan(p.parent/inc)
        scan(ROOT/'work'/(name+'.cpp'));need(t['path'] not in closure and all(h==before['source_sha256'].get(p) for p,h in closure.items()),'unaffected CPU test closure');cpu_closures[name]=closure
    for group in (sources,protected,inputs):
        for p,h in group.items():need(sha(p)==h,'final frozen identity '+p)
    result={'status':'PARTIALLY VERIFIED GPU transport: exact independent native reference and aligned oracle; arbitrary ideal-perspective discrepancy unresolved','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','parent':parent,'transition':t,'additional_transitions':additional,'source_sha256':sources,'protected_sha256':protected,'diagnostic_receipt':{'receipt':str(diag_receipt),'receipt_sha256':sha(diag_receipt)},'default_gpu_receipt':default_ref,'editor_preserved':prior['complete_editor'],'complete_editor_dependencies_sha256':editor_deps,'unchanged_cpu_test_closures':cpu_closures,'canonical_preserved_sha256':prior['canonical_preserved_sha256'],'new_editor_build':False,'new_editor_gpu_frames':0,'limits':['One measured NVIDIA adapter/driver, float32 offscreen owned shaders/data only.','Original ideal-perspective errors retained and not waived; possible raster setup precision not yet proven. No tolerance growth.','No original Source shader/material equivalence, new lighting feature or benchmark image.','Legacy36-byte vertices and default texture rendering unchanged; diagnostic stream28bytes.','CPU tests and95editor frames are prior evidence; closures checked unchanged, not newly run.']};receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({'status':result['status'],'diagnostic':diag['executable'],'cases':len(rows),'metrics':metrics,'strict':strict},indent=2),flush=True)

if __name__=='__main__':main()
