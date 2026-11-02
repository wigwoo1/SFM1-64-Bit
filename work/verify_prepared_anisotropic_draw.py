"""Opt-in normal producer/cache/indexed draw integration; Vulkan measured references."""
from pathlib import Path
from datetime import datetime,timezone
import itertools,json,math,os,re,shutil,struct,subprocess,tempfile
from verify_anisotropic_gpu import ROOT,OUT,VS,INCLUDE,sha,need,put,checkpoint
from verify_texture_sampling_cross_api import fixture,texel,srgb

def main():
    run=Path(tempfile.mkdtemp(prefix='normal-draw-',dir=OUT));print('Normal directory: '+str(run),flush=True);sources,protected,transitions=checkpoint()
    for n in ['probe_prepared_anisotropic_draw_x64.cpp','prepared-anisotropic-draw-probe.hlsl',Path(__file__).name]:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    idx=json.loads((OUT/'latest-gradient-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'actual gradient reference');ref=Path(idx['receipt']).parent;definitions=json.loads((ref/'cases.json').read_text());rr=definitions['rows'];points=definitions['points'];measured=[]
    for i in range((len(rr)+127)//128):
        b=(ref/f'vulkan-0-{i}.bin').read_bytes();at=0
        for _ in range(min(128,len(rr)-i*128)):
            h=struct.unpack_from('<10I',b,at);at+=40;n=h[-1];measured.append(struct.unpack_from('<'+'f'*(n*4),b,at));at+=16*n
        need(at==len(b),'reference extent')
    lut={(r['format'],r['family'],r['factor'],r['mode'],r['config'][0],r['config'][4],r['config'][9]):i for i,r in enumerate(rr)};point_lut={tuple(p):i for i,p in enumerate(points)};gradients=[p[4:] for p in points[:8]];fixtures=run/'fixtures';fixtures.mkdir();mounts={}
    for fmt in [0,13,15]:
        for flags in [(16,16),(17,16),(16,17),(28,16),(0x20000010,0),(0,16),(0,0)]:
            mount=fixtures/(str(fmt)+'-'+str(flags[0])+'-'+str(flags[1]));d=mount/'materials/fixture';d.mkdir(parents=True)
            for family,name,flag in [(0,'base',flags[0]),(1,'second',flags[1])]:
                b=fixture(fmt,family);struct.pack_into('<I',b,20,flag);(d/(name+'.vtf')).write_bytes(b)
            mask=bytearray(88)
            for at,n in [(0,0x00465456),(4,7),(8,5),(12,88),(20,0x100),(52,0),(57,0xffffffff),(68,1),(80,0x30),(84,88)]:struct.pack_into('<I',mask,at,n)
            for at,n in [(16,2),(18,2),(24,1),(63,1)]:struct.pack_into('<H',mask,at,n)
            mask[56]=1;mask+=bytes([64,64,128,255,192,64,128,255,64,192,128,255,192,192,128,255]);(d/'mask.vtf').write_bytes(mask);(d/'model.vmt').write_text('"WorldVertexTransition" { "$basetexture" "fixture/base" "$basetexture2" "fixture/second" "$selfillum" "1" "$selfillummask" "fixture/mask" }');mounts[(fmt,flags)]=mount
    rows=[]
    for (fmt,flags),mount in mounts.items():
        for gamma,frame,mips,uv,pattern,ch in itertools.product(range(2),[0,1,0],[(0,1),(1,3)],[(.1875,.6875),(.234375,.4609375)],[0,2,4,7],range(4)):
            u,v=uv;values=[]
            for family,flag,base in [(0,flags[0],mips[0]),(1,flags[1],mips[1])]:
                address=3 if flag&0x20000000 else 2 if flag&12 else 0
                if flag&1:
                    w=max(1,8>>base);x,y=math.floor(u*w),math.floor(v*w);x=max(0,min(w-1,x)) if flag&4 else x%w;y=max(0,min(w-1,y)) if flag&8 else y%w;value=[x/255 for x in texel(fmt,family,frame,base,x,y,True)];value=[srgb(x) if gamma and k<3 else x for k,x in enumerate(value)]
                else:
                    factor=4. if flag&16 else 0.;ri=lut[(fmt,family,factor,'linear',gamma,address,base)];pi=point_lut[(frame,u,v,0.,*gradients[pattern])];value=measured[ri][4*pi:4*pi+4]
                values.append(value)
            # Orthogonal integer-code mask ramp retains separate decoded sampler.
            colors=[[64,64,128,255],[192,64,128,255],[64,192,128,255],[192,192,128,255]];x,y=u*2-.5,v*2-.5;ix,iy=math.floor(x),math.floor(y);fx,fy=x-ix,y-iy;mask=[0.]*4
            for dx,dy in itertools.product(range(2),repeat=2):
                c=colors[((iy+dy)%2)*2+(ix+dx)%2];weight=(fx if dx else 1-fx)*(fy if dy else 1-fy);mask=[a+weight*b/255 for a,b in zip(mask,c)]
            expected=values[0] if ch==0 else mask if ch==1 else values[1] if ch==2 else [a*b+.1*c for a,b,c in zip(values[0],values[1],mask)];f=lambda x:struct.unpack('<I',struct.pack('<f',x))[0];q=[fmt,frame,mips[0],5,gamma,ch,f(float(pattern)),mips[1],3,f(u),f(v)];point_reference=(ch==0 and flags[0]&1) or (ch==2 and flags[1]&1) or (ch==3 and (flags[0]|flags[1])&1);rows.append({'config':q,'mount':str(mount),'flags':flags,'expected':expected,'tolerance':[2e-5+(.5/255 if gamma and point_reference else 0)]*3+[2e-5]})
    need(len(rows)==8064,'bounded actual normal grid');put(run/'prediction.json',rows)
    def text(s):
        b=str(s).encode('ascii');return struct.pack('<I',len(b))+b
    def query(path,rs):path.write_bytes(struct.pack('<I',len(rs))+b''.join(struct.pack('<11I',*r['config'])+text(r['mount']) for r in rs))
    queries=run/'queries.bin';query(queries,rows);dxc=Path(os.environ.get('SFM64_DXC_EXE',str(ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe')));lines=['@echo off',f'cd /d "{ROOT}"']
    for name,profile,entry in [('vertex','vs_6_0','vertexMain'),('pixel','ps_6_0','pixelMain')]:lines +=[f'"{dxc}" -spirv -fspv-target-env=vulkan1.0 -T {profile} -E {entry} work\\prepared-anisotropic-draw-probe.hlsl -Fo "{run/(name+".spv")}" >"{run/(name+"-shader.log")}" 2>&1','if errorlevel 1 exit /b 1']
    lines +=[f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\probe_prepared_anisotropic_draw_x64.cpp /Fo:"{run/"draw.obj"}" /Fe:"{run/"draw.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0',''];batch=run/'build.cmd';batch.write_text('\n'.join(lines))
    def execute(args,label,enabled=False):
        env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
        if enabled:env['SFM64_DIAGNOSTIC_ANISOTROPIC_TEXTURE_TEST']='1'
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+(p.stdout+p.stderr)[-1700:]);return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(len(warnings)==3 and all(c=='C4996' and "'getenv'" in m for c,m in warnings),'same inherited warnings');need(all(not (run/(n+'-shader.log')).read_text().strip() for n in ['vertex','pixel']),'no shader diagnostics');traces=[];logs=[];maximum=0.
    for repeat in range(2):
        path=run/f'pixels-{repeat}.bin';logs.append(execute([run/'draw.exe',run/'vertex.spv',run/'pixel.spv',queries,path],'draw-'+str(repeat),True));b=path.read_bytes();at=0
        for serial,row in enumerate(rows):
            need(struct.unpack_from('<I',b,at)[0]==serial,'normal draw record');at+=4
            for pixel in range(4):
                actual=struct.unpack_from('<4f',b,at);at+=16
                for k,(a,e,t) in enumerate(zip(actual,row['expected'],row['tolerance'])):
                    maximum=max(maximum,abs(a-e))
                    if not math.isfinite(a) or abs(a-e)>t:put(run/'mismatch.json',{'case':serial,'row':row,'actual':actual,'channel':k,'error':abs(a-e)});raise RuntimeError('normal draw mismatch '+str(serial))
        need(at==len(b),'full normal output');traces.append(b);print('PASS normal enabled repeat '+str(repeat),flush=True)
    need(traces[0]==traces[1],'exact repeated normal pixels')
    disabled=[r for r in rows if r['flags']==(16,16)];dq=run/'disabled-query.bin';query(dq,disabled);dp=run/'disabled-pixels.bin';logs.append(execute([run/'draw.exe',run/'vertex.spv',run/'pixel.spv',dq,dp],'disabled-normal'))
    oldidx=json.loads((ROOT/'outputs/source-border-20261101/latest-draw-verification.json').read_text());need(sha(oldidx['receipt'])==oldidx['receipt_sha256'],'last exact legacy draw');old=json.loads(Path(oldidx['receipt']).read_text());oldexe=old['executables']['draw'];need(sha(oldexe['path'])==oldexe['sha256'],'old exact executable');oldrun=Path(oldidx['receipt']).parent;oldrows=[]
    for r in disabled:
        q=r['config'].copy();q[3]=0;q[6]=0;oldrows.append(dict(r,config=q))
    oq=run/'legacy-query.bin';query(oq,oldrows);op=run/'legacy-pixels.bin';execute([oldexe['path'],oldrun/'vertex.spv',oldrun/'pixel.spv',oq,op],'legacy-exact-control');need(dp.read_bytes()==op.read_bytes(),'disabled feature/legacy fixed mip policy byte exact')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identities')
    result={'status':'VERIFIED opt-in normal x64 producer/cache/indexed draw integration against measured Vulkan gradient pixels; D3D9 anisotropic fidelity unresolved','time_utc':datetime.now(timezone.utc).isoformat(),'cases':len(rows),'repetitions':2,'disabled_legacy_cases':len(disabled),'disabled_legacy_byte_exact':True,'max_absolute_error':maximum,'logs':logs,'executable':{'path':str(run/'draw.exe'),'sha256':sha(run/'draw.exe')},'compiler_warnings':warnings,'reference_gradient':idx,'source_sha256':sources,'protected_sha256':protected,'transitions':transitions,'fixture_sha256':{str(p):sha(p) for p in fixtures.rglob('*') if p.is_file()},'limits':['Real normal prepared producer, immutable descriptors, independent Source point/clamp/border/aniso flags, CPU frame/view selection, masks, both texture channels and actual fence retirement/failure preservation.','Both channels prepared in new grid. Disabled-policy control matches old binary pixel-for-pixel. General fallback compatibility retains prior separate coverage; no full Source fidelity claim.','Measured Vulkan gradient reference verifies actual integration, NOT equivalence to D3D9. Cross-API rotated differences remain a separate failing fidelity gate.']};p=run/'results.json';put(p,result);put(OUT/'latest-normal-verification.json',{'receipt':str(p),'receipt_sha256':sha(p)});print(json.dumps({k:result[k] for k in ['status','cases','disabled_legacy_cases','max_absolute_error']},indent=2),flush=True)
if __name__=='__main__':main()
