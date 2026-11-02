"""Enabled-reference normal mip defaults through real retained indexed draws."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,itertools,json,math,os,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP,INCLUDE
from verify_vulkan_prepared_texture_binding import fixture,sample
OUT=ROOT/'outputs/source-mip-defaults-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def text(s):
    b=str(s).encode('ascii');return struct.pack('<I',len(b))+b
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    before=json.loads((OUT/'before.json').read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[before['prior']['receipt']]=before['prior']['receipt_sha256'];transitions=[]
    for t in before['transitions']:
        need(sha(t['backup'])==t['before'],'preserved before bytes');need(str(t['path']) not in sources or sources[t['path']]==t['before'],'prior source transition');transition=dict(t,after=sha(t['path']));transitions.append(transition);sources[t['path']]=transition['after'];protected[t['backup']]=t['before']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in [Path(t['path']).name for t in transitions]+[Path(__file__).name,'test_retained_model_texture_inputs.cpp','test_retained_scene_material_gpu_cache.cpp']:visit(ROOT/'work'/n)
    fixtures=run/'fixtures';fixtures.mkdir();mounts={};fixture_hashes={}
    for fmt in [0,3,12,13,15]:
        mount=fixtures/str(fmt);directory=mount/'materials/fixture';directory.mkdir(parents=True);b=fixture(fmt);second=b[:88];at=88
        for mip in reversed(range(4)):
            w=max(1,8>>mip);size=((w+3)//4)**2*(16 if fmt==15 else 8) if fmt in [13,15] else w*w*(3 if fmt==3 else 4);second+=b[at+size:at+2*size]+b[at:at+size];at+=2*size
        need(at==len(b),'authored animation framing');(directory/'base.vtf').write_bytes(b);(directory/'second.vtf').write_bytes(second)
        mask=bytearray(88)
        for offset,value in [(0,0x00465456),(4,7),(8,5),(12,88),(20,0x100),(52,0),(57,0xffffffff),(68,1),(80,0x30),(84,88)]:struct.pack_into('<I',mask,offset,value)
        for offset,value in [(16,2),(18,2),(24,1),(63,1)]:struct.pack_into('<H',mask,offset,value)
        mask[56]=1;mask+=bytes([64,64,128,255,192,64,128,255,64,192,128,255,192,192,128,255]);(directory/'mask.vtf').write_bytes(mask)
        (directory/'model.vmt').write_text('"WorldVertexTransition" { "$basetexture" "fixture/base" "$basetexture2" "fixture/second" "$selfillum" "1" "$selfillummask" "fixture/mask" }')
        for p in directory.iterdir():fixture_hashes[str(p)]=sha(p)
        mounts[fmt]=mount
    # Independent prediction from authored texels, not descriptor metadata.
    from verify_vulkan_prepared_texture_binding import pixel
    def predicted(fmt,srgb,linear,full,cu,cv,frame,u,v,lod,base):
        level=min(3,base+max(0,lod)) if full else base
        def texel(m,x,y):
            w=max(1,8>>m);x=max(0,min(w-1,x)) if cu else x%w;y=max(0,min(w-1,y)) if cv else y%w
            p=[n/255 for n in pixel(fmt,frame,m,x,y)]
            return [n/12.92 if n<=.04045 else ((n+.055)/1.055)**2.4 for n in p[:3]]+[p[3]] if srgb else p
        def at(m):
            w=max(1,8>>m)
            if not linear:return texel(m,math.floor(u*w),math.floor(v*w))
            x,y=u*w-.5,v*w-.5;ix,iy=math.floor(x),math.floor(y);fx,fy=x-ix,y-iy;out=[0.]*4
            for dx,dy in itertools.product(range(2),repeat=2):
                weight=(fx if dx else 1-fx)*(fy if dy else 1-fy)
                out=[a+weight*b for a,b in zip(out,texel(m,ix+dx,iy+dy))]
            return out
        lo=math.floor(level);f=level-lo
        return [a*(1-f)+b*f for a,b in zip(at(lo),at(min(3,lo+1)))] if full else at(lo)
    def mask_sample(u,v):
        colors=[[64,64,128,255],[192,64,128,255],[64,192,128,255],[192,192,128,255]];x,y=u*2-.5,v*2-.5;ix,iy=math.floor(x),math.floor(y);fx,fy=x-ix,y-iy;out=[0.]*4
        for dx,dy in itertools.product(range(2),repeat=2):
            c=colors[((iy+dy)%2)*2+(ix+dx)%2];w=(fx if dx else 1-fx)*(fy if dy else 1-fy)
            out=[a+w*b/255 for a,b in zip(out,c)]
        return out
    # Orthogonal integer-code mask ramps isolate descriptor routing without
    # predicting arbitrary UNORM filter intermediate rounding. Targeted mixed
    # policy draws use linear data; unchanged1620-case coverage retains sRGB.
    rows=[]
    def add(fmt,frame,bm,sm,full,srgb,ch,lod,presence,u,v,flags,mount):
        vals=[]
        for channel,fl,source_mip,bit in [(0,flags[0],bm,1),(2,flags[1],sm,2)]:
            prepared=bool(presence&bit);base=min(source_mip if prepared else bm,3);chain=bool((3 if full==4 else full)&bit) and prepared and not fl&1
            vals.append(predicted(fmt,bool(srgb),not(fl&1) if prepared else True,chain,bool(fl&4) if prepared else False,bool(fl&8) if prepared else False,frame if channel==0 else 1-frame,u,v,lod,base))
        mask=mask_sample(u,v);expected=vals[0] if ch==0 else mask if ch==1 else vals[1] if ch==2 else [a*b+.1*c for a,b,c in zip(vals[0],vals[1],mask)]
        tol=[2e-5+(0.5/255 if srgb and ch!=1 else 0)]*3+[2e-5]
        rows.append({'config':[fmt,frame,bm,full,srgb,ch,struct.unpack('<I',struct.pack('<f',lod))[0],sm,presence,struct.unpack('<I',struct.pack('<f',u))[0],struct.unpack('<I',struct.pack('<f',v))[0]],'mount':str(mount),'flags':flags,'expected':expected,'tolerance':tol})
    # Preserve the preceding format/frame/mip/gamma/channel coverage.
    for fmt,frame,bm,full,srgb,ch,lod in itertools.product([0,3,12,13,15],[0,1,0],[0,1,99],range(2),range(2),range(3),[0.,1.25,7.]):add(fmt,frame,bm,bm,full*3,srgb,ch,lod,3,.1875,.6875,(0,0),mounts[fmt])
    # Read the actual normal producer's new reference preset, without overwriting it.
    for fmt,frame,bm,srgb,ch,lod in itertools.product([0,3,12,13,15],[0,1,0],[0,1,99],range(2),range(4),[0.,1.25,7.]):add(fmt,frame,bm,bm,4,srgb,ch,lod,3,.1875,.6875,(0,0),mounts[fmt])
    # Contrasting raw VTF flags and channel-specific mip range in a single set.
    for fmt,flags in itertools.product([0,13,15],[(0,5),(9,0),(4,8)]):
        mount=fixtures/(str(fmt)+'-'+str(flags[0])+'-'+str(flags[1]));shutil.copytree(mounts[fmt],mount)
        for name,flag in [('base',flags[0]),('second',flags[1])]:
            path=mount/'materials/fixture'/ (name+'.vtf');b=bytearray(path.read_bytes());struct.pack_into('<I',b,20,flag);path.write_bytes(b)
        for full,presence,ch,frame,uv in itertools.product(range(4),range(4),range(4),[0,1,0],[(-.1875,1.3125),(.234375,.4609375)]):
            bm,sm=(0,2) if frame else (1,0);add(fmt,frame,bm,sm,full,0,ch,1.25,presence,*uv,flags,mount)
    fixture_hashes={str(p):sha(p) for p in fixtures.rglob('*') if p.is_file()}
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<11I',*row['config'])+text(row['mount']) for row in rows));(run/'prediction.json').write_text(json.dumps(rows,indent=2))
    dxc=Path(os.environ.get('SFM64_DXC_EXE',str(ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe')));batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    for name,profile,entry in [('vertex','vs_6_0','vertexMain'),('pixel','ps_6_0','pixelMain')]:commands+=[f'"{dxc}" -spirv -fspv-target-env=vulkan1.0 -T {profile} -E {entry} work\\prepared-material-draw-probe.hlsl -Fo "{run/(name+".spv")}" >"{run/(name+"-shader.log")}" 2>&1','if errorlevel 1 exit /b 1']
    builds=[('draw','probe_prepared_material_draw_x64.cpp'),('retained','test_retained_model_texture_inputs.cpp'),('cache','test_retained_scene_material_gpu_cache.cpp'),('counts','test_vulkan_material_descriptor_counts.cpp')]
    for name,source in builds:commands+=['setlocal',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=300,env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')},creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1600:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');executables={};warnings={}
    for name,_ in builds:
        exe=run/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'AMD64 '+name);warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());executables[name]={'path':str(exe),'sha256':sha(exe)}
        need((not warnings[name] if name=='counts' else len(warnings[name])==3 and all(code=='C4996' and "'getenv'" in message for code,message in warnings[name])),'only inherited input getenv warnings '+name)
    for name in ['vertex','pixel']:need(not (run/(name+'-shader.log')).read_text().strip(),'shader diagnostics '+name)
    execute([run/'counts.exe'],'descriptor-counts');safety=execute([run/'retained.exe'],'retained-safety').stdout;need('frame_selection=PASS input_immutability=PASS gpu_cache_ownership=PASS' in safety,'retained input regression');cache=execute([run/'cache.exe'],'cache-regression').stdout;need('owner_lifetime=1' in cache and 'discarded_recording_retried=1' in cache,'cache lifecycle regression')
    errors=[];traces=[];logs=[]
    for repeat in range(2):
        target=run/f'pixels-{repeat}.bin';p=execute([run/'draw.exe',run/'vertex.spv',run/'pixel.spv',query,target],'draw-'+str(repeat));logs.append(p.stdout);data=target.read_bytes();traces.append(data);at=0;maximum=0.
        for serial,row in enumerate(rows):
            need(struct.unpack_from('<I',data,at)[0]==serial,'draw record');at+=4
            for pixel in range(4):
                actual=struct.unpack_from('<4f',data,at);at+=16
                for k,(a,b,t) in enumerate(zip(actual,row['expected'],row['tolerance'])):
                    error=abs(a-b);maximum=max(maximum,error)
                    if not math.isfinite(a) or error>t:
                        (run/'mismatch.json').write_text(json.dumps({'case':serial,'row':row,'actual':actual,'pixel':pixel,'channel':k,'error':error},indent=2));raise RuntimeError('scene draw mismatch '+str(serial))
        need(at==len(data) and f'prepared_material_scene_draws={len(rows)}' in p.stdout,'actual scene draw framing/count');errors.append(maximum)
    need(traces[0]==traces[1],'repeated scene pixel traces exact');imports=execute([DUMP,'/imports',run/'draw.exe'],'imports').stdout;dlls=re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$',imports,re.M|re.I);need(dlls==['KERNEL32.dll'],'no original runtime imports');before=sha(query);p=execute([run/'draw.exe',run/'vertex.spv',run/'pixel.spv',query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preserved')
    for group in [sources,protected,fixture_hashes]:need(all(sha(p)==h for p,h in group.items()),'final source/protected/fixture identities')
    result={'status':'VERIFIED normal model preparation -> prepared GPU material descriptors -> actual indexed scene draws; reference default mip selection, independent samplers and lifetime/pool failure checks passed; editor regression separately recorded','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':len(rows),'repetitions':2,'pixels_per_draw':4,'max_absolute_error':errors,'trace_sha256':sha(run/'pixels-0.bin'),'executables':executables,'compiler_warnings':warnings,'shader_sha256':{n:sha(run/(n+'.spv')) for n in ['vertex','pixel']},'imports':dlls,'logs':logs,'retained_safety':safety,'cache_regression':cache,'transitions':transitions,'source_sha256':sources,'protected_sha256':protected,'fixture_sha256':fixture_hashes,'limits':['Authored loose-file VMT/VTF data through actual normal model-input function and retained model GPU cache/PreparedSceneGpuBridge/indexed draw recorder. Constant-coordinate full-screen triangle isolates descriptors; it does not prove geometry/animation/lighting fidelity.','Source formats0/3/12/13/15; distinct VTF point/S/T-clamp flags; all four base/second prepared/fallback combinations; combined base*second+mask sample; base/second independent selected frames, mip1 and out-of-range-last clamp, fixed legacy mip and explicit full remaining chain independently; mask keeps its own decoded repeat/linear policy. Native sampler/cache-ready semantics not claimed.','Caller profiles retire before GPU submission; retained cache and bindings survive actual fence wait and then release tracked image/buffer/memory/view resources. No validation layer/device-loss/submit-failure proof.','Original SFM not launched or touched. Complete editor rebuild and benchmark regression recorded separately.']}
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','repetitions','max_absolute_error','retained_safety','cache_regression']},indent=2),flush=True)
if __name__=='__main__':main()
