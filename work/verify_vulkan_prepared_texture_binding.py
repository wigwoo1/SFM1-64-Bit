"""Sample actual prepared GPU descriptors/frames/mips, retain leases, inject binding failures."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,math,os,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP,INCLUDE
OUT=ROOT/'outputs/texture-binding-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def text(s):
    b=s.encode('ascii');return struct.pack('<I',len(b))+b
def pixel(fmt,frame,mip,x,y):
    if fmt in [13,15]:
        code=[0xf800,0x07e0,0x001f,0xffff][(frame+mip+x//4+2*(y//4))%4];r,g,b=(code>>11)&31,(code>>5)&63,code&31
        return [(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2),255 if fmt==13 else (31+frame*53+mip*37)%256]
    return [(17+frame*61+mip*37+x*23+y*5)%256,(83+frame*11+mip*41+x*3+y*19)%256,(151+frame*17+mip*13+x*11+y*7)%256,255 if fmt==3 else (211+frame*7+mip*3+x+y)%256]
def fixture(fmt):
    b=bytearray(88)
    for at,n in [(0,0x00465456),(4,7),(8,5),(12,88),(52,fmt),(57,0xffffffff),(68,1),(80,0x30),(84,88)]:struct.pack_into('<I',b,at,n)
    for at,n in [(16,8),(18,8),(24,2),(63,1)]:struct.pack_into('<H',b,at,n)
    b[56]=4
    for mip in reversed(range(4)):
        w=max(1,8>>mip)
        for frame in range(2):
            if fmt in [13,15]:
                for by in range((w+3)//4):
                    for bx in range((w+3)//4):
                        color=[0xf800,0x07e0,0x001f,0xffff][(frame+mip+bx+2*by)%4]
                        if fmt==15:b.extend(bytes([(31+frame*53+mip*37)%256,0])+bytes(6))
                        b.extend(struct.pack('<HHI',color,0,0))
            else:
                for y in range(w):
                    for x in range(w):
                        r,g,blue,a=pixel(fmt,frame,mip,x,y);b.extend([r,g,blue,a] if fmt==0 else [blue,g,r] if fmt==3 else [blue,g,r,a])
    return b
def sample(fmt,srgb,linear,mip_linear,clamp,frame,u,v,lod):
    lod=max(0,min(3,lod))
    def texel(level,x,y):
        w=max(1,8>>level);x=max(0,min(w-1,x)) if clamp else x%w;y=max(0,min(w-1,y)) if clamp else y%w;p=[n/255 for n in pixel(fmt,frame,level,x,y)]
        if srgb:
            for k in range(3):p[k]=p[k]/12.92 if p[k]<=.04045 else ((p[k]+.055)/1.055)**2.4
        return p
    def level(level):
        w=max(1,8>>level)
        if not linear:return texel(level,math.floor(u*w),math.floor(v*w))
        x,y=u*w-.5,v*w-.5;ix,iy=math.floor(x),math.floor(y);a,b=x-ix,y-iy;result=[0.]*4
        for dx,dy in itertools.product(range(2),repeat=2):
            p=texel(level,ix+dx,iy+dy);weight=(a if dx else 1-a)*(b if dy else 1-b)
            for k in range(4):result[k]+=weight*p[k]
        return result
    if not mip_linear:return level(math.floor(lod+.5))
    lo=math.floor(lod);weight=lod-lo;a=level(lo);b=level(min(3,lo+1));return [x*(1-weight)+y*weight for x,y in zip(a,b)]

# Bounded image/sampler correctness check, not a Vulkan conformance tolerance
# or original Source shader parity claim. Hardware sRGB conversion need not
# equal the ideal Python EOTF to binary32 precision. Khronos CTS likewise uses
# explicit color thresholds: vktTextureFilteringTests.cpp, LookupPrecision.
# https://raw.githubusercontent.com/KhronosGroup/VK-GL-CTS/main/external/vulkancts/modules/vulkan/texture/vktTextureFilteringTests.cpp
# Keep linear data/alpha strict; allow half an 8-bit linear unit for RGB sRGB
# decoding, plus a separate small filtering budget. Record measured errors.
def tolerances(srgb,filtered):
    base=2e-5 if filtered else 2e-6
    return [base+(0.5/255 if srgb else 0)]*3+[base]

def tolerance_self_test(rows):
    checks=0
    for r in rows:
        q=r['config'];srgb,linear,mip_linear,clamp=bool(q[0]),bool(q[1]),bool(q[3]),bool(q[4]);tol=r['tolerance'];wanted=r['expected'][0];point=r['points'][0]
        wrong_frame=sample(r['format'],srgb,linear,mip_linear,clamp,1-point[0],*point[1:])
        wrong_mip=sample(r['format'],srgb,linear,mip_linear,clamp,*point[:3],3)
        for wrong in [wrong_frame,wrong_mip,wanted[:3]+[wanted[3]+1/255],[float('nan')]+wanted[1:]]:
            need(any(not math.isfinite(a) or abs(a-b)>t for a,b,t in zip(wrong,wanted,tol)),'precision policy must reject wrong frame/mip/alpha/nonfinite output');checks+=1
        if r['format'] in [0,3,12]:
            wrong=sample(r['format'],not srgb,linear,mip_linear,clamp,*point)
            need(any(abs(a-b)>t for a,b,t in zip(wrong,wanted,tol)),'precision policy must reject missing/extra sRGB decoding');checks+=1
    return checks
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-gpu-bridge-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'preceding GPU milestone');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256'];owner=ROOT/'work/vulkan_material_images_candidate.h';backup=OUT/'before-vulkan_material_images_candidate.h';need(sha(backup)==sources[str(owner)],'preserved existing owner');transition=dict(path=str(owner),before=sha(backup),after=sha(owner),reason='Explicit shader consumer stage for upload barrier, retaining fragment default. Prepared binding is a separate composed lifetime owner, not a second cache or raw typed reference.');sources[str(owner)]=sha(owner);protected[str(backup)]=sha(backup)
    publication_transition=json.loads((OUT/'publication-transition.json').read_text());publication_path=str(ROOT/'publication/stage/docs/build-status.json')
    need(publication_transition['path']==publication_path and protected[publication_path]==publication_transition['before'],'explicit preceding publication identity')
    need(sha(publication_transition['before_backup'])==publication_transition['before'] and sha(publication_path)==publication_transition['after'],'publication transition bytes')
    publication_receipt=publication_transition['publication_receipt'];need(sha(publication_receipt)==publication_transition['publication_receipt_sha256'],'verified publication receipt identity');publication=json.loads(Path(publication_receipt).read_text())
    need(publication['commit']=='7406e07c27fb464e8bf3bb7a9c677e0c8f2442e0' and publication['full_tree_verified'] and not publication['canonical_build_replaced'],'verified publication scope')
    protected[publication_path]=publication_transition['after'];protected[publication_transition['before_backup']]=publication_transition['before'];protected[publication_receipt]=publication_transition['publication_receipt_sha256'];protected[str(OUT/'publication-transition.json')]=sha(OUT/'publication-transition.json')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding source/protected identities')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in ['vulkan_prepared_texture_binding_candidate.h','probe_vulkan_prepared_texture_binding.cpp','texture-binding-sample-probe.hlsl',Path(__file__).name]:visit(ROOT/'work'/n)
    fixtures=run/'fixtures';fixtures.mkdir();fixture_hashes={};rows=[];points=[]
    lods=[0.,1.,2.,3.,7.,-2.,.25,1.25,2.75,.75];uvs=[(.1875,.6875),(-.1875,1.3125),(.5,.5),(.03125,.96875)]
    for frame in [0,1,0]:
        for n,lod in enumerate(lods):points.append((frame,*uvs[n%len(uvs)],lod))
    coverage={}
    for fmt in [0,3,12,13,15]:
        path=fixtures/(str(fmt)+'.vtf');path.write_bytes(fixture(fmt));fixture_hashes[str(path)]=sha(path)
        for srgb,policy,conversion in itertools.product(range(2),range(4),range(2)):
            linear=policy>=2;mip_linear=bool(policy&1);clamp=bool(policy&1);flags=0x1000 if (fmt+policy+conversion)%2 else 0;q=[srgb,int(linear),int(linear),int(mip_linear),2 if clamp else 0,2 if clamp else 0,flags,0 if conversion else 0xffffffff];expected=[sample(fmt,srgb,linear,mip_linear,clamp,*point) for point in points];rows.append(dict(config=q,format=fmt,name='fixture/binding/'+str(fmt),source=str(path),points=points,expected=expected,tolerance=tolerances(srgb,linear or mip_linear)));coverage[str(fmt)]=coverage.get(str(fmt),0)+1
    precision_rejection_checks=tolerance_self_test(rows)
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<8I',*r['config'])+text(r['name'])+text(r['source'])+struct.pack('<I',len(points))+b''.join(struct.pack('<I3f',*p) for p in points) for r in rows));(run/'prediction.json').write_text(json.dumps(rows,indent=2))
    dxc=Path(os.environ.get('SFM64_DXC_EXE',str(ROOT/'work/dependencies/dxc-v1.9.2609/bin/x64/dxc.exe')));need(dxc.is_file(),'existing shader compiler');shader=run/'sample.spv';batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"',f'"{dxc}" -spirv -fspv-target-env=vulkan1.0 -T cs_6_0 -E main work\\texture-binding-sample-probe.hlsl -Fo "{shader}" >"{run/"shader-build.log"}" 2>&1','if errorlevel 1 exit /b 1']
    for name,source in [('binding','probe_vulkan_prepared_texture_binding.cpp'),('transfer','probe_vulkan_texture_plan_x64.cpp')]:commands+=['setlocal',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=240,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings={};executables={}
    for name in ['binding','transfer']:
        p=run/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'AMD64 '+name);warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());need(len(warnings[name])==3 and all(code=='C4996' and "'getenv'" in text for code,text in warnings[name]),'only inherited model-header warnings');executables[name]={'path':str(p),'sha256':sha(p)}
    need(not (run/'shader-build.log').read_text().strip(),'shader compile diagnostics');shader_bytes=shader.read_bytes();need(len(shader_bytes)%4==0 and struct.unpack_from('<2I',shader_bytes)==(0x07230203,0x00010000),'owned SPIRV1.0')
    errors=[];logs=[];traces=[]
    for repeat in range(2):
        dest=run/('samples-'+str(repeat)+'.bin');p=execute([run/'binding.exe',shader,query,dest],'sampling-'+str(repeat));logs.append(p.stdout);data=dest.read_bytes();traces.append(data);at=0;max_error=0.;components=0
        for serial,r in enumerate(rows):
            q=r['config'];record=list(struct.unpack_from('<10I',data,at));at+=40;fmt=0 if q[7]!=0xffffffff else r['format'];effective=0x2000 if q[7]!=0xffffffff and r['format']!=0 else 0;expected_record=[serial,2,4,fmt,q[6],0,q[6],effective,int(bool(q[6]&0x3000)),len(points)];need(record==expected_record,'prepared metadata '+str(serial))
            for n,(point,wanted) in enumerate(zip(points,r['expected'])):
                actual=struct.unpack_from('<4f',data,at);at+=16
                for k,(a,b) in enumerate(zip(actual,wanted)):
                    error=abs(a-b);components+=1;max_error=max(max_error,error)
                    if not math.isfinite(a) or error>r['tolerance'][k]:
                        (run/'mismatch.json').write_text(json.dumps({'case':serial,'format':r['format'],'config':q,'point':point,'actual':actual,'expected':wanted,'channel':k,'error':error,'tolerance':r['tolerance']},indent=2));raise RuntimeError('sampled texture mismatch case '+str(serial)+' point '+str(n))
        need(at==len(data),'sampling trace framing');need('descriptor_failures=4' in p.stdout and 'previous_binding_preserved=1' in p.stdout and 'frame_leases=1' in p.stdout,'binding failure/lifetime tests');errors.append(dict(repeat=repeat,components=components,max_absolute_error=max_error))
    need(traces[0]==traces[1],'repeated sampled float trace identity')
    old=Path(prior['gpu_receipt']['receipt']).parent;need(sha(old/'gpu-0.bin')==prior['trace_sha256'],'previous transfer trace identity')
    for repeat in range(2):
        dest=run/('transfer-'+str(repeat)+'.bin');p=execute([run/'transfer.exe',old/'queries.bin',dest],'transfer-'+str(repeat));need(sha(dest)==prior['trace_sha256'] and 'injected_resource_failures=16' in p.stdout,'all112 previous transfer cases/16 faults unchanged')
    imports=execute([DUMP,'/imports',run/'binding.exe'],'imports').stdout;dlls=re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$',imports,re.M|re.I);need(dlls==['KERNEL32.dll'],'owned runtime independence');before=sha(query);p=execute([run/'binding.exe',shader,query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preservation')
    need(all(sha(p)==h for p,h in fixture_hashes.items()),'authored fixtures unchanged')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identities')
    result=dict(status='VERIFIED bounded prepared GPU binding/sampler/frame leases and actual shader sampling; normal editor routing incomplete',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),source_format_coverage=coverage,samples_per_case=len(points),shader_samples_per_run=len(rows)*len(points),repetitions=2,numerical_comparison=errors,repeat_trace_sha256=sha(run/'samples-0.bin'),descriptor_failure_steps_per_run=4,negative_cases=['missing service','linear capability unavailable','invalid sampler policy','out-of-range frame'],previous_transfer_cases=112,previous_transfer_failures=16,executables=executables,shader={'source':str(ROOT/'work/texture-binding-sample-probe.hlsl'),'source_sha256':sha(ROOT/'work/texture-binding-sample-probe.hlsl'),'spirv_sha256':sha(shader),'compiler_sha256':sha(dxc)},compiler_warnings=warnings,imports=dlls,logs=logs,source_transition=transition,source_sha256=sources,protected_sha256=protected,fixture_sha256=fixture_hashes,limits=['Actual compute SampleLevel reads per-frame sampled-image/sampler descriptors, with complete mip views, explicit point/linear min/mag+mip settings, wrap/clamp, source0/3/12/13/15, optional CPU conversion and linear/sRGB storage. CPU metadata retained separately from raw typed reference.','Point linear-storage samples use2e-6 absolute tolerance; point sRGB6e-4; interpolation1.1/255 for allowed hardware filtering/format conversion precision. Compare all components and reject nonfinite values; measured errors recorded. Same-device repeated float traces exact. Not original Source GPU visual/shader parity.','Binding factory does no queue submission/readiness publication. Frame leases retain prepared CPU metadata and actual GPU images/descriptors/sampler after root owners retire; test waits actual fence before final lease destruction. Device/loader/layout must outlive leases.','No global texture cache, native typed lookup success, normal editor routing or document/history change. Native Source sampler-flag policy, full cache/error/inactive state, device loss and submit failure remain unverified.','No original SFM process/DLL imports or original writes. Owned test shader is separate from unchanged editor shader manifest; full editor rebuild/regression required for the upload consumer-stage extension.'])
    result['publication_transition']=publication_transition
    result['precision_rejection_checks']=precision_rejection_checks
    result['precision_policy']={'linear_point_and_alpha':2e-6,'linear_filter_and_alpha':2e-5,'srgb_RGB_additional_absolute_budget':0.5/255,'scope':'Bounded correctness criterion, not original Source GPU parity or a Vulkan conformance certification.'}
    result['limits'][1]='All components/nonfinite values checked independently. Point linear/alpha2e-6; filtered linear/alpha2e-5; sRGB RGB adds0.5/255 absolute. Explicit accuracy budget informed by hardware conversion evidence and CTS threshold-based testing, not a normative conformance bound. Self-tests reject wrong frame/mip/alpha/gamma and nonfinite outputs. Exact same-device repeated traces required.'
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','fixture_sha256','limits']},indent=2),flush=True)
if __name__=='__main__':main()
