"""Real GPU readback/fault tests for existing owner extended to full mip/frame plans."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP,INCLUDE
from verify_material_texture_converted_image import native_records
from verify_shader_texture_cpu_preparation import queries
OUT=ROOT/'outputs/texture-gpu-bridge-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def text(s):
    b=s.encode('ascii');return struct.pack('<I',len(b))+b
def predict(serial,q,reference):
    r,_,raw,_=reference;fmt=r[16];frames=r[12];mips=r[11];srgb=q[11];physical={0:(37,43),3:(37,43),12:(44,50),13:(133,134),15:(137,138)}[fmt][srgb];at=0;blocks={}
    for mip in range(r[19]):
        for frame in range(r[18]):n=struct.unpack_from('<I',raw,at)[0];at+=4;blocks[frame,mip]=raw[at:at+n];at+=n
    need(at==len(raw),'native reference framing');out=bytearray(struct.pack('<5I',serial,frames,mips,physical,srgb))
    for frame in range(frames):
        for mip in range(mips):
            w,h=max(1,r[14]>>mip),max(1,r[15]>>mip);pixels=blocks[frame,mip]
            if fmt==3:pixels=bytes(c for i in range(0,len(pixels),3) for c in [pixels[i+2],pixels[i+1],pixels[i],255])
            out.extend(struct.pack('<3I',w,h,len(pixels))+pixels)
    return out,frames,frames*mips
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-upload-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted upload milestone');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256'];owner=ROOT/'work/vulkan_material_images_candidate.h';before=OUT/'before-vulkan_material_images_candidate.h';need(sha(before)==sources[str(owner)],'preserved existing owner identity');transition=dict(path=str(owner),before=sha(before),after=sha(owner),reason='Extend existing Vulkan material owner for complete ordinary mip/frame plans; legacy single-level path retained. Real GPU/failure tests plus full editor rebuild/regression required.');sources[str(owner)]=sha(owner);protected[str(before)]=sha(before)
    canonical_transition=None
    if (OUT/'canonical-transition.json').exists():
        journal=json.loads((OUT/'canonical-transition.json').read_text());need(journal['status']=='VERIFIED canonical promotion and clean startup','verified canonical transition');need(sha(journal['canonical'])==journal['after'] and sha(journal['backup'])==journal['before'],'canonical/backup identities');need(protected[journal['canonical']]==journal['before'],'explicit preceding canonical identity');protected[journal['canonical']]=journal['after'];protected[journal['backup']]=journal['before'];protected[str(OUT/'canonical-transition.json')]=sha(OUT/'canonical-transition.json');canonical_transition=journal
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding source/protected identities')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in ['probe_vulkan_texture_plan_x64.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    grouped={f:{} for f in [0,3,12,13,15]};forced=None
    for folder in ['material-texture-converted-image-20261101','texture-block-formats-20261101']:
        i=json.loads((ROOT/'outputs'/folder/'latest-milestone.json').read_text());need(sha(i['receipt'])==i['receipt_sha256'],'reference milestone');m=json.loads(Path(i['receipt']).read_text());cpu=m['cpu_receipt'];need(sha(cpu['receipt'])==cpu['receipt_sha256'],'CPU receipt');base=Path(cpu['receipt']).parent;info=json.loads(Path(cpu['receipt']).read_text());need(sha(base/'native-0.bin')==info['native_trace_sha256'],'native trace identity');protected[str(base/'native-0.bin')]=sha(base/'native-0.bin')
        for (q,strings),ref in zip(queries(base/'queries.bin'),native_records((base/'native-0.bin').read_bytes())):
            b=Path(strings[2]).read_bytes();minor=struct.unpack_from('<I',b,8)[0]
            if minor>=3 and any(struct.unpack_from('<I',b,80+n*8)[0] not in [1,0x30,0x02435243,0x02444f4c] for n in range(struct.unpack_from('<I',b,68)[0])):continue
            r=ref[0];fmt=r[16];raw_fmt=struct.unpack_from('<I',b,52)[0];key=(r[12],r[11],r[9],r[10],raw_fmt,r[13],r[20]);grouped[fmt].setdefault(key,(q,strings,ref))
            if fmt==0 and r[12]>=2 and forced is None:forced=(q,strings,ref)
    need(forced is not None and all(grouped.values()),'all formats and failure fixture present');selected=[forced]
    for fmt,items in grouped.items():selected.extend(list(items.values())[:12])
    fixtures=run/'fixtures';fixtures.mkdir();rows=[];fixture_hashes={};coverage={}
    for n,(q,strings,ref) in enumerate(selected):
        p=fixtures/(str(n)+'.vtf');shutil.copyfile(strings[2],p);fixture_hashes[str(p)]=sha(p)
        for srgb in [0,1]:rows.append((q+[srgb],strings[1],str(p),ref));fmt=str(ref[0][16]);coverage[fmt]=coverage.get(fmt,0)+1
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<12I',*q)+text(name)+text(p) for q,name,p,ref in rows));expected=bytearray();frames=mips=0
    for serial,(q,name,p,ref) in enumerate(rows):packet,f,n=predict(serial,q,ref);expected.extend(packet);frames+=f;mips+=n
    (run/'expected.bin').write_bytes(expected);batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\probe_vulkan_texture_plan_x64.cpp /Fo:"{run/"gpu.obj"}" /Fe:"{run/"gpu.exe"}" >"{run/"build.log"}" 2>&1','exit /b %errorlevel%','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=240,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2000:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(len(warnings)==3 and all(code=='C4996' and "'getenv'" in text for code,text in warnings),'only three inherited model-header warnings');exe=run/'gpu.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'owned AMD64 PE32+')
    logs=[]
    for repeat in range(2):
        dest=run/('gpu-'+str(repeat)+'.bin');p=execute([exe,query,dest],'gpu-'+str(repeat));actual=dest.read_bytes();logs.append(p.stdout)
        if actual!=expected:
            at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));(run/'mismatch.json').write_text(json.dumps({'byte':at,'actual_near':list(actual[max(0,at-60):at+100]),'expected_near':list(expected[max(0,at-60):at+100])},indent=2));raise RuntimeError('GPU native-byte-reference mismatch '+str(at))
        need('injected_resource_failures=16' in p.stdout and 'cleanup_violations=0' in p.stdout and 'all_owned_resources_released=1' in p.stdout,'failure/lifetime checks')
    before_hash=sha(query);p=execute([exe,query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before_hash,'existing output preserved');imports=execute([DUMP,'/imports',exe],'imports').stdout;dlls=re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$',imports,re.M|re.I);need(dlls==['KERNEL32.dll'],'no original runtime imports');need(all(sha(p)==h for p,h in fixture_hashes.items()),'fixtures unchanged')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final source/protected closure')
    result=dict(status='VERIFIED real GPU mip/frame upload/readback and partial resource failure cleanup; editor regression pending',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),coverage=coverage,frame_images=frames,readback_mips=mips,repetitions=2,trace_sha256=sha(run/'expected.bin'),injected_failures_per_run=16,capability_refusal_per_run=True,existing_owner_preserved=True,committed_gpu_pixels_preserved=True,compiler_warnings=warnings,executable={'path':str(exe),'sha256':sha(exe),'architecture':'AMD64 PE32+'},imports=dlls,logs=logs,transition=transition,source_sha256=sources,protected_sha256=protected,fixture_sha256=fixture_hashes,limits=['Real NVIDIA Vulkan allocation/binding/upload/fence/readback on tested device. Formats0/3/12/13/15, explicit linear andsRGB. BGR888 expands to RGBA8 with alpha255; compressed uploads/readback preserve raw blocks, not a proof of sampled Source shader equivalence.','Existing VulkanMaterialImagesCandidate owner extended: one image per frame, complete validated mip chain. New createUploadPlan requires empty owner/same source-destination format/nonempty chain. Format capabilities queried; caller must explicitly enable BC feature. No second cache or document changes.','Test dispatch injects pre-call allocation/binding/map errors at16 steps, forwards all other calls to actual Vulkan, and tracks resource cleanup/double release/mapped memory. Format refusal and existing-owner rejection tested; prior GPU pixels remain readable. Device loss, queue-submit failure/uncertain completion and full manager commit remain unverified.','No validation layer enabled. No original SFM process/DLL imports or original writes. Complete editor rebuild/regression required separately before accepting source transition; canonical editor preserved.'])
    result['canonical_transition']=canonical_transition
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','fixture_sha256','limits','canonical_transition']},indent=2),flush=True)
if __name__=='__main__':main()
