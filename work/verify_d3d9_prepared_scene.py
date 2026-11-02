"""Owned prepared CPU packet -> actual x64 D3D9 indexed pixels, no SFM control."""
from pathlib import Path
from datetime import datetime,timezone
import itertools,json,math,re,struct,subprocess,tempfile,shutil
from verify_texture_sampling_cross_api import fixture,texel,string
from verify_session_preview_aspect import ROOT,VS,DUMP,SESSION,CONTENT
from verify_anisotropic_bitness import sha,need,put,pe
OUT=ROOT/'outputs/d3d9-prepared-scene-20261101'
MATRICES=[(1,0,0,1),(16,0,0,16),(16,-1,16,1),(8,-.5,4,1)]
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]

def main():
    before=json.loads((OUT/'before.json').read_text());parent=before['parent'];need(sha(parent['receipt'])==parent['receipt_sha256'],'preceding receipt')
    sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[parent['receipt']]=parent['receipt_sha256'];seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p)
        if str(p) in sources:need(sha(p)==sources[str(p)],'unchanged existing source '+str(p))
        sources[str(p)]=sha(p)
        for n in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/n).exists():visit(p.parent/n)
    for name in ('probe_d3d9_prepared_scene_x64.cpp','d3d9_prepared_scene_candidate.h',Path(__file__).name):visit(ROOT/'work'/name)
    for group in (sources,protected):
        for p,h in group.items():need(sha(p)==h,'preceding identity '+p)
    run=Path(tempfile.mkdtemp(prefix='packet-gpu-',dir=OUT));print(str(run),flush=True)
    rows=[];fixtures={}
    def authored(fmt,flags,family=0):
        key=(fmt,flags,family)
        if key not in fixtures:
            p=run/f'owned-{fmt}-{flags}-{family}.vtf';b=bytearray(fixture(fmt,family));struct.pack_into('<I',b,20,flags);p.write_bytes(b);fixtures[key]=p
        return fixtures[key]
    def row(fmt,flags,frame,srgb,mip,pattern,explicit,geometry=0,alpha=1.,cutoff=0.,paths=None):
        a,b=paths or (authored(fmt,flags),authored(fmt,flags,1))
        q=[frame,mip,srgb,geometry,bits(alpha),bits(cutoff),pattern,explicit]
        rows.append({'format':fmt,'flags':flags,'q':q,'a':str(a),'b':str(b)})
    for fmt,flags,frame,srgb,pattern,explicit in itertools.product((0,13,15),(0,16,17,4,8,0x2000000c,0x110),range(2),range(2),(0,2),range(2)):
        for mip in ((0,) if flags&0x100 else (0,1)):row(fmt,flags,frame,srgb,mip,pattern,explicit)
    for fmt,flags,frame,srgb,explicit in itertools.product((0,13,15),(0,16,17),range(2),range(2),range(2)):row(fmt,flags,frame,srgb,3,2,explicit)
    # Existing strict two-texture admission: ordinary flags only, opaque additive.
    for fmt,frame,srgb,explicit in itertools.product((0,13,15),range(2),range(2),range(2)):row(fmt,0,frame,srgb,0,0,explicit,5)
    def solid(name,rgba):
        b=bytearray(fixture(0,0));struct.pack_into('<I',b,20,1)
        at=88
        for level in reversed(range(4)):
            size=max(1,8>>level)**2*4
            for frame in range(2):b[at:at+size]=bytes(rgba)*(size//4);at+=size
        need(at==len(b),'solid fixture framing');p=run/(name+'.vtf');p.write_bytes(b);return p
    red=solid('solid-red',(255,0,0,255));green=solid('solid-green',(0,255,0,255));half=solid('solid-half-alpha',(255,0,0,128))
    for geometry in (1,2,3,4):row(0,1,0,0,0,0,0,geometry,.5 if geometry==2 else 1.,paths=(red,green))
    row(0,1,0,0,0,0,0,6,cutoff=.75,paths=(half,green));row(0,1,0,0,0,0,0,6,cutoff=.25,paths=(half,green))
    for geometry in (7,8):row(0,1,0,0,0,0,0,geometry,paths=(red,green))
    coordinate=bytearray(fixture(0,0));struct.pack_into('<I',coordinate,20,1);at=88
    for level in reversed(range(4)):
        extent=max(1,8>>level)
        for frame in range(2):
            for y in range(extent):
                for x in range(extent):coordinate[at:at+4]=bytes((32*x,32*y,64,255));at+=4
    need(at==len(coordinate),'coordinate texture framing');uv_texture=run/'owned-coordinate-uv.vtf';uv_texture.write_bytes(coordinate)
    for geometry in (4,9):row(0,1,0,0,0,0,0,geometry,paths=(uv_texture,green))
    put(run/'cases.json',rows);query=run/'packets.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<8I',*r['q'])+string(r['a'])+string(r['b']) for r in rows))
    inputs={str(p):sha(p) for p in run.glob('*.vtf')};inputs[str(query)]=sha(query)
    batch=run/'build.cmd';exe=run/'prepared-scene.exe'
    batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_d3d9_prepared_scene_x64.cpp /Fo:"{run/"probe.obj"}" /Fe:"{exe}" d3d9.lib d3dcompiler.lib user32.lib >"{run/"build.log"}" 2>&1','exit /b %errorlevel%','']))
    def execute(args,label,timeout=300):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+(p.stdout+p.stderr)[-1500:]);return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',batch],'build-driver');need(pe(exe)==(0x8664,0x20b),'native x64 proof executable');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(len(warnings)==4 and sorted(c for c,_ in warnings)==['C4458','C4996','C4996','C4996'],'only four inherited warnings')
    logs=[]
    for repeat in range(2):
        logs.append(execute([exe,query,run/f'actual-{repeat}','owned'],f'actual-{repeat}',600));print(f'Actual indexed run{repeat} complete: {len(rows)} cases',flush=True)
    a=(run/'actual-0'/'pixels.bin').read_bytes();need(a==(run/'actual-1'/'pixels.bin').read_bytes(),'exact GPU repeats');need((run/'actual-0'/'samplers.bin').read_bytes()==(run/'actual-1'/'samplers.bin').read_bytes(),'exact actual sampler repeats')
    pixels=[];at=0;state=(run/'actual-0'/'samplers.bin').read_bytes();st=0
    for serial,r in enumerate(rows):
        n=2 if r['q'][3] in (1,2) else 1;need(struct.unpack_from('<2I',a,at)==(serial,n),'actual indexed framing');at+=8;v=struct.unpack_from('<4096f',a,at);at+=16384;need(all(math.isfinite(x) for x in v),'finite readback');pixels.append(v)
        serial2,materials=struct.unpack_from('<2I',state,st);st+=8;need(serial2==serial and materials==(4 if n==2 else 2),'actual material-slot framing')
        # Two channels per material,13 real sampler inputs each. These are
        # retained candidate state; device GetSamplerState audit is a separate gate.
        for material in range(materials):
            for channel in range(2):
                sampler=struct.unpack_from('<13I',state,st);st+=52
                if channel and r['q'][3]!=5:need(sampler==(0,)*13,'unused channel empty');continue
                flags=r['flags'];point=bool(flags&1);nomip=bool(flags&0x100);aniso=bool(flags&16) and not(point or nomip)
                need(sampler[4]==(1 if point else 2) and sampler[5]==(3 if aniso else 1 if point else 2) and sampler[6]==(1 if point or nomip else 2) and sampler[8]==r['q'][1] and sampler[9]==(4 if aniso else 1) and sampler[10]==r['q'][2],'requested source sampler policy')
    need(at==len(a) and st==len(state),'complete indexed pixels/state')
    # External, preceding owned x64 D3D9 shader submits independent single-pixel
    # requests. For the affine packet cases implicit derivatives have closed-form
    # values; point/noMip deliberately use zero derivative to select the fixed mip.
    prior=json.loads(Path(parent['receipt']).read_text());prev=json.loads(Path(prior['verification']['receipt']).read_text());reference=prev['executables']['64'];need(sha(reference['path'])==reference['sha256'],'independent preceding reference binary')
    refs=[];ref_index=[]
    for i,r in enumerate(rows):
        if r['q'][3] not in (0,5):continue
        frame,mip,srgb,geometry,_,_,pattern,_=r['q'];flags=r['flags'];point=bool(flags&1);nomip=bool(flags&0x100);aniso=bool(flags&16) and not(point or nomip);m=MATRICES[pattern]
        points=[]
        for y in range(32):
            for x in range(32):
                u,v=(x+.5)/32,(y+.5)/32
                gradient=(0.,0.,0.,0.) if point or nomip else (m[0]/32,m[2]/32,m[1]/32,m[3]/32)
                points.append((frame,m[0]*u+m[1]*v+.0625,m[2]*u+m[3]*v+.0625,0.,*gradient))
        address=lambda flag:3 if flags&0x20000000 else 2 if flags&flag else 0
        config=[srgb,int(not point),int(not point),int(not(point or nomip)),address(4),address(8),0,0xffffffff,bits(4.) if aniso else 0,mip]
        for path,flip in [(r['a'],False)]+([(r['b'],True)] if geometry==5 else []):
            request=points if not flip else [(p[0]^1,*p[1:]) for p in points]
            refs.append(struct.pack('<10I',*config)+string('fixture/indexed-reference')+string(path)+struct.pack('<I',len(request))+b''.join(struct.pack('<I7f',*p) for p in request));ref_index.append((i,flip))
    references={}
    for start in range(0,len(refs),128):
        q=run/f'reference-{start}.bin';chunk=refs[start:start+128];q.write_bytes(struct.pack('<I',len(chunk))+b''.join(chunk));target=run/f'reference-{start}'
        execute([reference['path'],q,target,4318,11544],f'reference-{start}');b=(target/'samples.bin').read_bytes();at=0
        for serial in range(len(chunk)):
            need(struct.unpack_from('<2I',b,at)==(serial,1024),'reference framing');at+=8;references[ref_index[start+serial]]=struct.unpack_from('<4096f',b,at);at+=16384
        need(at==len(b),'complete independent samples');print(f'Independent reference batch{start} complete',flush=True)
    metrics={};examples=[]
    for i,r in enumerate(rows):
        if (i,False) not in references:continue
        expected=list(references[(i,False)])
        if r['q'][3]==5:
            second=references[(i,True)]
            for n in range(1024):expected[4*n:4*n+4]=[expected[4*n+k]*second[4*n+k] for k in range(3)]+[1.]
        group=str((r['q'][7],bool(r['flags']&16 and not r['flags']&0x101),r['q'][3]))
        m=metrics.setdefault(group,{'components':0,'over_2e_6':0,'max_error':0.})
        for j,(actual,want) in enumerate(zip(pixels[i],expected)):
            e=abs(actual-want);m['components']+=1;m['over_2e_6']+=int(e>2e-6);m['max_error']=max(m['max_error'],e)
            if e>2e-6 and len(examples)<12:examples.append({'case':i,'row':r,'pixel':j//4,'channel':j%4,'actual':actual,'expected':want,'error':e})
    independent_controls=0
    for i,r in enumerate(rows):
        geometry=r['q'][3]
        if geometry not in (1,2,3,4,6,7,8,9):continue
        for y in range(32):
            for x in range(32):
                if geometry in (3,7,8):want=[0.,0.,0.,0.]
                elif geometry==6:want=[0.,0.,0.,0.] if struct.unpack('<f',struct.pack('<I',r['q'][5]))[0]>.5 else [1.,0.,0.,128/255]
                elif geometry in (4,9):
                    sx,sy=2*(x+.5)/32-1,2*(y+.5)/32-1;wx=3*sx/(1-sx);wz=wx+3;wy=sy*wz
                    if geometry==9:wx=(sy+.125)/.75;wy=(.25-sx)/.5
                    # Exclude exact primitive edges; interior and clear controls
                    # test perspective clipping without inventing tie-break rules.
                    if abs(abs(wx)-1)<1e-6 or abs(abs(wy)-1)<1e-6:continue
                    inside=-1<wx<1 and -1<wy<1;want=[1.,0.,0.,1.] if inside else [0.,0.,0.,0.]
                    if inside and r['a']==str(uv_texture):
                        u,v=(wx+1)/2+.0625,(wy+1)/2+.0625
                        if min(abs(u*8-round(u*8)),abs(v*8-round(v*8)))<1e-6:continue
                        want=[32*(math.floor(u*8)%8)/255,32*(math.floor(v*8)%8)/255,64/255,1.]
                elif 8<=x<24 and 8<=y<24:want=[0.,1.,0.,1.] if geometry==1 else [.5,.5,0.,1.]
                else:want=[1.,0.,0.,1.]
                actual=pixels[i][4*(y*32+x):4*(y*32+x)+4];need(max(abs(a-b) for a,b in zip(actual,want))<2e-6,f'independent camera/depth/alpha control case{i} pixel{x},{y}: {actual} vs {want}');independent_controls+=1
    benchmark_source=ROOT/'outputs/anisotropic-gpu-20261101/editor-runtime-bgctmjki/input-copy.dmx'
    need(str(benchmark_source) in protected and sha(benchmark_source)==protected[str(benchmark_source)],'protected disposable benchmark source')
    benchmark_copy=run/'benchmark-input-copy.dmx'
    with benchmark_source.open('rb') as src,benchmark_copy.open('xb') as dst:shutil.copyfileobj(src,dst)
    benchmark_hash=sha(benchmark_copy);info_hash=sha(CONTENT/'gameinfo.txt')
    bench_args=[str(exe),str(benchmark_copy),str(run/'benchmark'),'benchmark',str(CONTENT),'50000']
    b=subprocess.run(bench_args,cwd=ROOT,capture_output=True,text=True,timeout=240,creationflags=subprocess.CREATE_NO_WINDOW);(run/'benchmark.log').write_text(b.stdout+b.stderr)
    need(sha(benchmark_copy)==benchmark_hash==sha(benchmark_source) and sha(CONTENT/'gameinfo.txt')==info_hash,'benchmark/source/gameinfo preserved')
    benchmark={'args':bench_args,'returncode':b.returncode,'log':b.stdout+b.stderr,'copy_sha256':benchmark_hash,'gameinfo_readonly_sha256':info_hash,'admission':(run/'benchmark/admission.txt').read_text() if (run/'benchmark/admission.txt').exists() else None}
    put(run/'benchmark.json',benchmark)
    for group in (sources,protected,inputs):
        for p,h in group.items():need(sha(p)==h,'post-run identity '+p)
    parity=all(m['over_2e_6']==0 for m in metrics.values())
    result={'status':'VERIFIED owned prepared-scene indexed pixel gates' if parity else 'PARTIALLY VERIFIED: indexed native reference mismatch remains','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','parent':parent,'cases':len(rows),'repetitions':2,'pixels_per_run':len(rows)*1024,'independent_control_pixels':independent_controls,'metrics':metrics,'mismatch_examples':examples,'executable':{'path':str(exe),'sha256':sha(exe)},'reference':reference,'warnings':warnings,'logs':logs,'benchmark':benchmark,'source_sha256':sources,'protected_sha256':protected,'input_sha256':inputs,'limits':['Linear offscreen output, subset admission; no original Source shaders/lighting match.','Independent reference uses explicit gradients on separate single-pixel primitives; implicit-mode differences are measured and must pass the same 2e-6 gate.','Failure hooks cover staged resource preparation; actual driver/device loss/reset not certified.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ('status','cases','pixels_per_run','independent_control_pixels','metrics','mismatch_examples')},indent=2),flush=True)
    need(parity,'native indexed/reference pixel gate failed; measurements preserved')

if __name__=='__main__':main()
