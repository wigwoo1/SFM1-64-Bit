"""Owned AMD64 D3D9/Vulkan BC sampling measurement, never original SFM control."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,itertools,json,math,os,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP,INCLUDE

OUT=ROOT/'outputs/texture-cross-api-correction-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def put(p,v):Path(p).write_text(json.dumps(v,indent=2)+'\n')
def string(s):
    b=str(s).encode('ascii');return struct.pack('<I',len(b))+b
def srgb(x):return x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4
def unsrgb(x):return x*12.92 if x<=.0031308 else 1.055*x**(1/2.4)-.055
def color565(x):return [((x>>11)<<3)|((x>>11)>>2),(((x>>5)&63)<<2)|(((x>>5)&63)>>4),((x&31)<<3)|((x&31)>>2)]
def nvidia_alpha_palette(a,b):
    # Reviewed test prediction only. GPU BC3 alpha is not native CPU DXT5 or
    # the higher-precision BC4 path. Keep signed arithmetic shifts and multiply
    # delta before the inner correction. Primary empirical source:
    # https://fgiesen.wordpress.com/2021/10/04/gpu-bcn-decoding/
    d=b-a
    def rounded(k,delta):return (k*delta+(delta>>3)+128)>>8
    if a>b:return (a,b,a+rounded(36,d),a+rounded(36,2*d),b+rounded(36,-4*d),a+rounded(36,4*d),b+rounded(36,-2*d),b+rounded(36,-d))
    return (a,b,a+rounded(48,d),a+rounded(48,2*d),b+rounded(48,-2*d),b+rounded(48,-d),0,255)
def nvidia_alpha_rational_palette(a,b):
    # Independent floor algebra: floor(floor(x)/256)==floor(x/256).
    d=b-a
    if a>b:spec=((a,36,d),(a,36,2*d),(b,36,-4*d),(a,36,4*d),(b,36,-2*d),(b,36,-d))
    else:spec=((a,48,d),(a,48,2*d),(b,48,-2*d),(b,48,-d))
    p=(a,b)+tuple(base+((8*k+1)*delta+1024)//2048 for base,k,delta in spec)
    return p if a>b else p+(0,255)
def alpha_reference_self_test():
    for a,b in itertools.product(range(256),repeat=2):
        p=nvidia_alpha_palette(a,b);need(p==nvidia_alpha_rational_palette(a,b),'independent alpha algebra');need(min(p)>=0 and max(p)<=255 and p[:2]==(a,b),'alpha range/endpoints')
        if a<=b:need(p[6:]==(0,255),'alpha special zero/one')
    for a,b,expected in [(13,239,(13,239,55,98,154,197,0,255)),(0,255,(0,255,48,96,159,207,0,255)),(255,0,(255,0,219,183,144,111,72,36)),(87,87,(87,87,87,87,87,87,0,255)),(1,0,(1,0,1,1,1,0,0,0))]:need(nvidia_alpha_palette(a,b)==expected,'reviewed hand-calculated alpha palette')
    sensitivity=sum(((k*d+(d>>3)+128)>>8)!=((k*d+int(d/8)+128)>>8) for k in [36,48] for d in range(-1020,1021));need(sensitivity==7,'signed arithmetic sensitivity')
    return {'endpoint_pairs':65536,'palette_entries':524288,'hand_calculated_palettes':5,'signed_arithmetic_counterexamples':sensitivity,'scope':'CPU algebra/range/reference checks, not exhaustive GPU endpoint coverage'}
ENDPOINTS=[(0xf800,0x07e0,251,17),(0x001f,0xffff,13,239),(0x4a69,0x4a69,87,87),(1,0,1,0),(0x0841,0x0020,0,255),(0x7bef,0x8410,127,128),(0xffff,0,255,0),(0,0xffff,0,0)]
def block_fields(family,frame,mip,bx,by):
    a,b,a0,a1=ENDPOINTS[(family+mip+frame+bx+2*by)%len(ENDPOINTS)]
    phase=(frame+bx+by)%4
    return a,b,a0,a1,[(p+phase)%4 for p in range(16)],[(p//2+frame+by)%8 for p in range(16)]
def texel(fmt,family,frame,mip,x,y,nvidia=False):
    if fmt==0:return [(13+frame*71+mip*19+x*23)%256,(41+frame*31+mip*47+y*29)%256,(59+frame*53+mip*11+x*7+y*13)%256,(101+frame*17+mip*37+x*11+y*19)%256]
    a,b,a0,a1,csel,asel=block_fields(family,frame,mip,x//4,y//4);p=(y%4)*4+x%4
    colors=[color565(a)+[255],color565(b)+[255]]
    four=a>b or fmt==15
    colors.append([(2*colors[0][k]+colors[1][k])//3 if four else (colors[0][k]+colors[1][k])//2 for k in range(3)]+[255])
    colors.append([(colors[0][k]+2*colors[1][k])//3 if four else 0 for k in range(3)]+[255 if four else 0])
    if nvidia:
        # Independent vendor-specific prediction, NOT an editor decoder change.
        # Measured formulas published by the researchers who recovered them:
        # https://fgiesen.wordpress.com/2021/10/04/gpu-bcn-decoding/
        # https://github.com/castano/icbc/blob/master/icbc.h
        # Keep the ideal/truncating prediction above as a separately reported
        # rejected assumption; do not hide that failure with a wider budget.
        for k,shift,mask in [(0,11,31),(2,0,31)]:
            v0=(a>>shift)&mask;v1=(b>>shift)&mask
            colors[2][k]=((2*v0+v1)*22)//8 if four else ((v0+v1)*33)//8
            colors[3][k]=((v0+2*v1)*22)//8 if four else 0
        g0,g1=colors[0][1],colors[1][1];delta=g1-g0
        quarter=abs(delta)//4*(1 if delta>=0 else -1) # C/C++ truncation
        colors[2][1]=(g0*256+quarter+128+delta*(80 if four else 128))//256
        colors[3][1]=(g1*256-quarter+128-delta*80)//256 if four else 0
    out=colors[csel[p]].copy()
    if fmt==15:
        alpha=[a0,a1]+[((8-i)*a0+(i-1)*a1)//7 for i in range(2,8)] if a0>a1 else [a0,a1]+[((6-i)*a0+(i-1)*a1)//5 for i in range(2,6)]+[0,255]
        if nvidia:alpha=nvidia_alpha_palette(a0,a1)
        out[3]=alpha[asel[p]]
    return out
def fixture(fmt,family):
    b=bytearray(88)
    for offset,value in [(0,0x00465456),(4,7),(8,5),(12,88),(52,fmt),(57,0xffffffff),(68,1),(80,0x30),(84,88)]:struct.pack_into('<I',b,offset,value)
    for offset,value in [(16,8),(18,8),(24,2),(63,1)]:struct.pack_into('<H',b,offset,value)
    b[56]=4
    for mip in reversed(range(4)):
        width=max(1,8>>mip)
        for frame in range(2):
            if fmt==0:
                for y in range(width):
                    for x in range(width):b.extend(texel(fmt,family,frame,mip,x,y))
            else:
                for by in range((width+3)//4):
                    for bx in range((width+3)//4):
                        a,c,a0,a1,color,alpha=block_fields(family,frame,mip,bx,by)
                        if fmt==15:b.extend(bytes([a0,a1])+sum(v<<(3*p) for p,v in enumerate(alpha)).to_bytes(6,'little'))
                        b.extend(struct.pack('<HHI',a,c,sum(v<<(2*p) for p,v in enumerate(color))))
    return b
def prediction(row,point,nvidia=False):
    frame,u,v,lod=point;fmt,family=row['format'],row['family'];q=row['config'];lod=max(0,min(3,lod))
    def at(level,x,y):
        w=max(1,8>>level);x=max(0,min(w-1,x)) if q[4] else x%w;y=max(0,min(w-1,y)) if q[5] else y%w
        p=[n/255 for n in texel(fmt,family,frame,level,x,y,nvidia)]
        return [srgb(v) if q[0] and k<3 else v for k,v in enumerate(p)]
    def level(mip):
        w=max(1,8>>mip)
        if not q[1]:return at(mip,math.floor(u*w),math.floor(v*w))
        x,y=u*w-.5,v*w-.5;ix,iy=math.floor(x),math.floor(y);fx,fy=x-ix,y-iy;out=[0.]*4
        for dx,dy in itertools.product(range(2),repeat=2):
            p=at(mip,ix+dx,iy+dy);weight=(fx if dx else 1-fx)*(fy if dy else 1-fy)
            out=[a+weight*b for a,b in zip(out,p)]
        return out
    if not q[3]:return level(math.floor(lod+.5))
    lo=math.floor(lod);f=lod-lo;return [a*(1-f)+b*f for a,b in zip(level(lo),level(min(3,lo+1)))]
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    index=json.loads((ROOT/'outputs/texture-alpha-review-20261101/latest-milestone.json').read_text());need(sha(index['receipt'])==index['receipt_sha256']=='dc10ef4bb018fb18a1221b629079bac516ae9a4a929a17863f99f064bff5f9e8','reviewed alpha evidence');before=json.loads(Path(index['receipt']).read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[index['receipt']]=index['receipt_sha256']
    verifier=Path(__file__).resolve();backup=OUT/'before-verify_texture_sampling_cross_api.py';need(sha(backup)==sources[str(verifier)],'explicit preserved verifier source');source_transition={'path':str(verifier),'before':sha(backup),'after':sha(verifier),'backup':str(backup),'review_receipt':index};sources[str(verifier)]=sha(verifier);protected[str(backup)]=sha(backup)
    receipt=ROOT/'publication/sync-20261101-0841-verified.local.json';publication=json.loads(receipt.read_text());transition=publication['publication_metadata_transition'];p=ROOT/transition['path'];need(publication['full_tree_verified'] and publication['commit']=='80b5a91a0c9792d652c36e35f84e20ff32bd6188' and transition['after']==protected[str(p)]==sha(p),'preceding explicit verified publication transition')
    for group in [sources,protected]:need(all(Path(p).exists() and sha(p)==h for p,h in group.items()),'preceding source/protected identities')
    cpu_alpha_checks=alpha_reference_self_test()
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for name in ['probe_texture_sampling_d3d9_x64.cpp','probe_vulkan_prepared_texture_binding.cpp','texture-binding-sample-probe.hlsl',Path(__file__).name]:visit(ROOT/'work'/name)
    points=[]
    for frame,mip in itertools.product(range(2),range(4)):
        w=max(1,8>>mip)
        points.extend((frame,(x+.5)/w,(y+.5)/w,float(mip)) for y in range(w) for x in range(w))
    points.extend((frame,u,v,lod) for frame in range(2) for u,v in [(-.1875,.3125),(.03125,.59375),(.234375,.4609375),(1.125,1.28125)] for lod in [-.25,.25,1.25,2.75,7.])
    rows=[];fixtures={}
    for fmt in [13,15,0]:
        for family in range(8 if fmt!=0 else 1):
            path=run/f'authored-{fmt}-{family}.vtf';path.write_bytes(fixture(fmt,family));fixtures[str(path)]=sha(path)
            for srgb_read,linear,mip_linear,clamp in itertools.product(range(2),repeat=4):rows.append({'format':fmt,'family':family,'config':[srgb_read,linear,linear,mip_linear,2 if clamp else 0,2 if clamp else 0,0,0xffffffff],'source':str(path),'points':points})
    put(run/'cases.json',rows)
    batches=[rows[:136],rows[136:]];queries=[]
    for i,batch in enumerate(batches):
        query=run/f'queries-{i}.bin';query.write_bytes(struct.pack('<I',len(batch))+b''.join(struct.pack('<8I',*r['config'])+string('fixture/cross-api/'+str(r['format'])+'/'+str(r['family']))+string(r['source'])+struct.pack('<I',len(points))+b''.join(struct.pack('<I3f',*point) for point in points) for r in batch));queries.append(query)
    dxc=Path(os.environ.get('SFM64_DXC_EXE',str(ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe')));shader=run/'sampling.spv';build=run/'build.cmd'
    commands=['@echo off',f'cd /d "{ROOT}"',f'"{dxc}" -spirv -fspv-target-env=vulkan1.0 -T cs_6_0 -E main work\\texture-binding-sample-probe.hlsl -Fo "{shader}" >"{run/"shader-build.log"}" 2>&1','if errorlevel 1 exit /b 1']
    for label,source,libs in [('vulkan','probe_vulkan_prepared_texture_binding.cpp',''),('d3d9','probe_texture_sampling_d3d9_x64.cpp','d3d9.lib d3dcompiler.lib user32.lib')]:commands+=['setlocal',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\{source} /Fo:"{run/(label+".obj")}" /Fe:"{run/(label+".exe")}" {libs} >"{run/(label+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    build.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,timeout=180):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1400:]);return p.stdout+p.stderr
    execute(['cmd.exe','/d','/c',build],'build-driver',300)
    executables={};warnings={};imports={}
    for label in ['d3d9','vulkan']:
        p=run/(label+'.exe');data=p.read_bytes();pe=struct.unpack_from('<I',data,60)[0];need(struct.unpack_from('<H',data,pe+4)[0]==0x8664 and struct.unpack_from('<H',data,pe+24)[0]==0x20b,'AMD64 PE32+ '+label);executables[label]={'path':str(p),'sha256':sha(p)};warnings[label]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(label+'-build.log')).read_text());imports[label]=execute([DUMP,'/dependents',p],label+'-imports');need(not re.search(r'(?i)(?:ifm|studiorender|shaderapi|tier0|vstdlib|engine|materialsystem)\.dll',imports[label]),'no original runtime imports')
    need(not warnings['d3d9'] and len(warnings['vulkan'])==3 and all(code=='C4996' and "'getenv'" in message for code,message in warnings['vulkan']),'only inherited Vulkan warnings')
    need(not (run/'shader-build.log').read_text().strip(),'no Vulkan shader diagnostics');put(run/'build-proof.json',{'executables':executables,'warnings':warnings,'imports':imports})
    traces={};logs=[];adapter=None
    for repeat in range(2):
        values={'d3d9':[],'vulkan':[]}
        for i,(query,batch) in enumerate(zip(queries,batches)):
            trace=run/f'vulkan-{repeat}-{i}.bin';log=execute([run/'vulkan.exe',shader,query,trace],f'vulkan-{repeat}-{i}');logs.append(log);match=re.search(r'vendor=(\d+) device_id=(\d+)',log);need(match is not None,'actual Vulkan adapter identity');current=tuple(map(int,match.groups()));need(adapter is None or adapter==current,'stable same GPU');adapter=current
            output=run/f'd3d9-{repeat}-{i}';log=execute([run/'d3d9.exe',query,output,*adapter],f'd3d9-{repeat}-{i}');logs.append(log)
            need(all(not p.read_bytes().strip(b'\x00\r\n ') for p in output.glob('*-compiler.log')),'owned SM3 shader diagnostics')
            for label,path,header in [('vulkan',trace,10),('d3d9',output/'samples.bin',2)]:
                b=path.read_bytes();at=0
                for serial,row in enumerate(batch):
                    fields=struct.unpack_from('<'+'I'*header,b,at);at+=4*header;need(fields[0]==serial and fields[-1]==len(points),'trace framing '+label)
                    if label=='vulkan':need(fields[1:4]==(2,4,row['format']),'raw authored GPU format retained')
                    floats=struct.unpack_from('<'+'f'*(len(points)*4),b,at);at+=len(points)*16;need(all(math.isfinite(x) and -.000001<=x<=1.000001 for x in floats),'finite normalized output');values[label].append(floats)
                need(at==len(b),'complete trace extent '+label)
            print('repeat '+str(repeat)+' batch '+str(i)+' both GPUs completed',flush=True)
        traces[repeat]=values
    need(traces[0]==traces[1],'exact repeated float traces for both APIs')
    need(traces[0]['d3d9']==traces[0]['vulkan'],'exact cross-API sampled floats on this same GPU; investigate rather than relaxing')
    need(adapter[0]==4318,'independent vendor predictor currently NVIDIA only')
    metrics={};mismatches=[];point_errors={};ideal_errors={};checks=0;alpha_checks=0;max_alpha_error=0.
    for index,row in enumerate(rows):
        q=row['config'];key=f'format{row["format"]}-srgb{q[0]}-linear{q[1]}-miplinear{q[3]}';m=metrics.setdefault(key,{'components':0,'unequal_components':0,'max_absolute_error':0.,'sum_absolute_error':0.})
        for n,point in enumerate(points):
            a=traces[0]['d3d9'][index][n*4:n*4+4];b=traces[0]['vulkan'][index][n*4:n*4+4]
            for k,(x,y) in enumerate(zip(a,b)):
                error=abs(x-y);m['components']+=1;m['unequal_components']+=int(x!=y);m['max_absolute_error']=max(m['max_absolute_error'],error);m['sum_absolute_error']+=error
                if error>2e-6 and len(mismatches)<40:mismatches.append({'case':index,'query':n,'channel':k,'point':point,'d3d9':a,'vulkan':b,'format':row['format'],'config':q})
            # Integer mip + point spatial sampling isolates every raw selector.
            if q[1] or point[3] not in [0.,1.,2.,3.]:continue
            ideal=prediction(row,point);expected=prediction(row,point,nvidia=True)
            for label,actual in [('d3d9',a),('vulkan',b)]:
                p=point_errors.setdefault(label,{'samples':0,'max_encoded_code_error':0.,'max_alpha_code_error':0.});p['samples']+=1
                rejected=ideal_errors.setdefault(label,{'components':0,'components_over_original_1_02_code_budget':0,'max_code_error':0.})
                for k,(x,y) in enumerate(zip(actual,ideal)):
                    x=unsrgb(x) if q[0] and k<3 else x;y=unsrgb(y) if q[0] and k<3 else y;error=abs(x-y)*255;rejected['components']+=1;rejected['components_over_original_1_02_code_budget']+=int(error>1.02);rejected['max_code_error']=max(rejected['max_code_error'],error)
                for k,(x,y) in enumerate(zip(actual,expected)):
                    if k==3:
                        alpha_error=abs(x-y);need(alpha_error<=2e-6,'strict normalized point alpha');max_alpha_error=max(max_alpha_error,alpha_error);alpha_checks+=1
                    x=unsrgb(x) if q[0] and k<3 else x;y=unsrgb(y) if q[0] and k<3 else y;error=abs(x-y)*255
                    field='max_alpha_code_error' if k==3 else 'max_encoded_code_error';p[field]=max(p[field],error)
                    # Fixed before observing results: at most one UNORM8 code
                    # for vendor alpha/palette rounding, plus 0.02 numerical units.
                    # This catches palette/order/selector errors; it does NOT
                    # certify API conformance, Source parity or filtered sRGB.
                    need(error<=1.02,'independent point palette/order/selector '+label+' '+str((index,n,k,error)));checks+=1
    for m in metrics.values():m['mean_absolute_error']=m.pop('sum_absolute_error')/m['components']
    need(all(v['components_over_original_1_02_code_budget']>0 for v in ideal_errors.values()),'historical ideal-reference discrepancy remains visible')
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'sources and protected identities preserved');need(all(sha(p)==h for p,h in fixtures.items()),'fixture immutability')
    result={'status':'VERIFIED bounded owned D3D9/Vulkan sampling measurement; original Source sampler policy remains incomplete','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':len(rows),'samples_per_case':len(points),'samples_per_api_per_run':len(rows)*len(points),'repetitions':2,'same_gpu_vendor_device':adapter,'metrics':metrics,'independent_point_reference':point_errors,'rejected_ideal_truncating_reference':ideal_errors,'independent_component_checks':checks,'point_reference_budget_encoded_codes':1.02,'cross_api_mismatches_over_2e_6_examples':mismatches,'cross_api_comparison':'All compared floats exact across both APIs and both repeats. No tolerance relaxation. Same tested NVIDIA GPU only; no Source parity claim.','coverage':['BC1 four/three-color/equal endpoint cases; all four color selectors','BC3 forward/reverse/equal color endpoints; all four color selectors independent of both alpha endpoint orders/equality and all eight alpha selectors','8x8/4x4/2x2/1x1 mips; two authored frames; clamp/wrap; spatial point/linear; mip point/linear; explicit negative/fractional/beyond-last LOD; sRGB/linear','RGBA8 controls; D3D9 float render target and Vulkan float storage readbacks; owned shaders; System32 Vulkan and Windows D3D9'], 'executables':executables,'compiler_warnings':warnings,'imports':imports,'logs':logs,'source_sha256':sources,'protected_sha256':protected,'fixture_sha256':fixtures,'publication_transition':transition,'reference_correction':'Original ideal/truncating reference failed. Retained its measured failures; separate NVIDIA mathematical predictor from primary empirical research, same original code-unit budget. No runtime decoder changes.','limits':['No original SFM process, original shader bytecode, assets or session consumed by these probes. Same installed GPU/driver only; no general device certification.','The existing native CPU DXT5 conversion uses its separately measured reversed-endpoint palette; that code is unchanged. GPU BC3 reference always uses the four-color rule.','Original material sampler selection/actual texture destination negotiation not yet proven; explicit experimental sampler states are not claimed to be Source states.','No editor runtime source changed or canonical executable promoted. No validation layer/device-loss/submit-failure tests.']}
    result.update(source_transition=source_transition,alpha_review=index,cpu_alpha_reference_checks=cpu_alpha_checks,strict_point_alpha={'component_checks':alpha_checks,'maximum_absolute_error':max_alpha_error,'budget':2e-6},reference_correction='Reviewed test-only NVIDIA BC3 alpha predictor replaces ideal alpha only in the vendor prediction. Original ideal/truncating prediction and its rejection statistics remain. Both prior failed formulations and immutable review evidence preserved; no runtime decoder change or wider budget.')
    put(run/'results.json',result);put(OUT/'latest-verification.json',{'receipt':str(run/'results.json'),'receipt_sha256':sha(run/'results.json')});print(json.dumps({k:result[k] for k in ['status','cases','samples_per_case','same_gpu_vendor_device','independent_point_reference','cross_api_mismatches_over_2e_6_examples']},indent=2),flush=True)
if __name__=='__main__':main()
