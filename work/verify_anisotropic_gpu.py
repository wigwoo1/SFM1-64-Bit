"""Actual D3D9/Vulkan directional gradient sampling, no presumed API parity."""
from pathlib import Path
from datetime import datetime,timezone
import itertools,json,math,os,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP,INCLUDE
from verify_texture_sampling_cross_api import fixture,string
OUT=ROOT/'outputs/anisotropic-gpu-20261101'
def sha(p):
    import hashlib
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def put(p,j):Path(p).write_text(json.dumps(j,indent=2)+'\n')
def checkpoint():
    before=json.loads((OUT/'before.json').read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[before['prior']['receipt']]=before['prior']['receipt_sha256'];transitions=[]
    for t in before['transitions']:
        need(sha(t['backup'])==t['before']==sources[t['path']],'explicit preserved before source');transitions.append(dict(t,after=sha(t['path'])));sources[t['path']]=sha(t['path']);protected[t['backup']]=t['before']
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for n in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/n).exists():visit(p.parent/n)
    for n in ['probe_vulkan_anisotropic_sampling_x64.cpp','probe_anisotropic_sampling_d3d9_x64.cpp','texture-anisotropic-sample-probe.hlsl',Path(__file__).name]:visit(ROOT/'work'/n)
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    return sources,protected,transitions

def main():
    run=Path(tempfile.mkdtemp(prefix='measurement-',dir=OUT));print('Measurement directory: '+str(run),flush=True);sources,protected,transitions=checkpoint();rows=[];fixtures={}
    gradients=[(1/64,0,0,1/64),(.125,0,0,.125),(.5,0,0,.03125),(0,.5,.03125,0),(.5,.5,-.03125,.03125),(.25,.125,-.015625,.03125),(1.,0,0,.0625),(.125,.125,-.125,.125)]
    points=[(frame,u,v,0.,*g) for frame in range(2) for u,v in [(.0625,.0625),(.1875,.6875),(.234375,.4609375),(.5,.5),(0.,.3125),(1.,.3125),(-.1875,1.3125),(1.125,-.125)] for g in gradients]
    for fmt in [0,13,15]:
        for family in [0,1]:
            p=run/f'authored-{fmt}-{family}.vtf';p.write_bytes(fixture(fmt,family));fixtures[str(p)]=sha(p)
            for srgb,factor,addr,base in itertools.product(range(2),[0.,2.,4.,8.,16.],[0,2,3],[0,1,3]):rows.append({'format':fmt,'family':family,'factor':factor,'mode':'linear','config':[srgb,1,1,1,addr,addr,0,0xffffffff,struct.unpack('<I',struct.pack('<f',factor))[0],base],'source':str(p)})
            for srgb,addr,base in itertools.product(range(2),[0,2,3],[0,1,3]):rows.append({'format':fmt,'family':family,'factor':0.,'mode':'point','config':[srgb,0,0,0,addr,addr,0,0xffffffff,0,base],'source':str(p)})
    put(run/'cases.json',{'rows':rows,'points':points});batches=[rows[a:a+128] for a in range(0,len(rows),128)];queries=[]
    for i,batch in enumerate(batches):
        p=run/f'queries-{i}.bin';p.write_bytes(struct.pack('<I',len(batch))+b''.join(struct.pack('<10I',*r['config'])+string('fixture/aniso/'+str(r['format'])+'/'+str(r['family']))+string(r['source'])+struct.pack('<I',len(points))+b''.join(struct.pack('<I7f',*q) for q in points) for r in batch));queries.append(p)
    dxc=Path(os.environ.get('SFM64_DXC_EXE',str(ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe')));shader=run/'gradient.spv';batch=run/'build.cmd';lines=['@echo off',f'cd /d "{ROOT}"',f'"{dxc}" -spirv -fspv-target-env=vulkan1.0 -T cs_6_0 -E main work\\texture-anisotropic-sample-probe.hlsl -Fo "{shader}" >"{run/"shader-build.log"}" 2>&1','if errorlevel 1 exit /b 1']
    for label,source,libs in [('vulkan','probe_vulkan_anisotropic_sampling_x64.cpp',''),('d3d9','probe_anisotropic_sampling_d3d9_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib')]:lines+=['setlocal',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\{source} /Fo:"{run/(label+".obj")}" /Fe:"{run/(label+".exe")}" {libs} >"{run/(label+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(lines+['exit /b 0','']))
    def execute(args,label):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1900:]);return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',batch],'build-driver');executables={};warnings={}
    for label in ['vulkan','d3d9']:
        p=run/(label+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'native x64 '+label);warnings[label]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(label+'-build.log')).read_text());executables[label]={'path':str(p),'sha256':sha(p)}
    need(not warnings['d3d9'] and len(warnings['vulkan'])==3 and all(c=='C4996' and "'getenv'" in m for c,m in warnings['vulkan']),'only inherited getenv warnings');need(not (run/'shader-build.log').read_text().strip(),'owned SPIRV diagnostics')
    traces={};adapter=None;logs=[]
    for repeat in range(2):
        values={'vulkan':[],'d3d9':[]}
        for i,(query,rows_batch) in enumerate(zip(queries,batches)):
            vt=run/f'vulkan-{repeat}-{i}.bin';log=execute([run/'vulkan.exe',shader,query,vt],f'vulkan-{repeat}-{i}');logs.append(log);m=re.search(r'vendor=(\d+) device_id=(\d+)',log);need(m is not None and 'aniso_enabled=1' in log,'actual enabled device');pair=tuple(map(int,m.groups()));need(adapter is None or adapter==pair,'same adapter');adapter=pair;dd=run/f'd3d9-{repeat}-{i}';logs.append(execute([run/'d3d9.exe',query,dd,*adapter],f'd3d9-{repeat}-{i}'))
            for label,path,nhead in [('vulkan',vt,10),('d3d9',dd/'samples.bin',2)]:
                b=path.read_bytes();at=0
                for serial,row in enumerate(rows_batch):
                    h=struct.unpack_from('<'+'I'*nhead,b,at);at+=nhead*4;need(h[0]==serial and h[-1]==len(points),'sample framing');floats=struct.unpack_from('<'+'f'*(len(points)*4),b,at);at+=len(points)*16;need(all(math.isfinite(v) and -.000001<=v<=1.000001 for v in floats),'normalized finite samples');values[label].append(floats)
                need(at==len(b),'complete sampled trace')
            print(f'repeat {repeat} batch {i} GPU samples complete',flush=True)
        traces[repeat]=values
    need(traces[0]==traces[1],'exact per-API repeatability');metrics={};mismatches=[];contrast={'vulkan':0,'d3d9':0}
    index={(r['format'],r['family'],r['mode'],tuple(r['config'])):i for i,r in enumerate(rows)}
    for i,row in enumerate(rows):
        group=str((row['mode'],row['factor'],row['config'][0],row['config'][9]));metric=metrics.setdefault(group,{'components':0,'different':0,'over_2e_6':0,'max_absolute_error':0.,'sum_error':0.})
        for component,(a,b) in enumerate(zip(traces[0]['vulkan'][i],traces[0]['d3d9'][i])):
            e=abs(a-b);metric['components']+=1;metric['different']+=int(a!=b);metric['over_2e_6']+=int(e>2e-6);metric['max_absolute_error']=max(metric['max_absolute_error'],e);metric['sum_error']+=e
            if e>2e-6 and len(mismatches)<30:mismatches.append({'case':i,'row':row,'sample':component//4,'point':points[component//4],'channel':component%4,'vulkan':a,'d3d9':b,'error':e})
        if row['factor']:
            q=row['config'].copy();q[8]=0;control=index[(row['format'],row['family'],'linear',tuple(q))]
            for label in contrast:contrast[label]+=sum(abs(a-b)>2e-6 for a,b in zip(traces[0][label][i],traces[0][label][control]))
    for m in metrics.values():m['mean_absolute_error']=m.pop('sum_error')/m['components']
    need(min(contrast.values())>1000,'actual contrasting anisotropic and trilinear pixels')
    for group in [sources,protected,fixtures]:need(all(sha(p)==h for p,h in group.items()),'final source/protected/fixture identity')
    result={'status':'MEASURED actual x64 D3D9/Vulkan gradient pixels; see differences before claiming native equivalence','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':len(rows),'samples_per_case':len(points),'samples_per_api_per_run':len(rows)*len(points),'repetitions':2,'same_gpu_vendor_device':adapter,'exact_cross_api':traces[0]['vulkan']==traces[0]['d3d9'],'metrics':metrics,'mismatch_examples':mismatches,'anisotropy_vs_trilinear_differing_components':contrast,'executables':executables,'compiler_warnings':warnings,'source_sha256':sources,'protected_sha256':protected,'fixture_sha256':fixtures,'transitions':transitions,'logs':logs,'limits':['Owned authored VTF data/shaders and real system D3D9/Vulkan only; no original SFM process, original shader or asset.','One tested adapter/driver; differences are observations, no relaxed tolerance or universal anisotropic conformance claim.','Both per-API repeated traces exact. Actual device feature enablement, descriptor leases/fence cleanup, construction rollback and disabled/overlimit/nonfinite refusal checked; no validation-layer/device-loss certification.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-gradient-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ['status','cases','samples_per_api_per_run','exact_cross_api','anisotropy_vs_trilinear_differing_components','mismatch_examples']},indent=2),flush=True)
if __name__=='__main__':main()
