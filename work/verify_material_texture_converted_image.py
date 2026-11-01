"""Fresh actual native file reader vs retained owned x64 converted pixels."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_texture_file_image import text
OUT=ROOT/'outputs/material-texture-converted-image-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def native_records(data):
    at=0;rows=[]
    while at<len(data):
        record=list(struct.unpack_from('<24I',data,at));at+=96;strings=[]
        for _ in range(3):n=struct.unpack_from('<I',data,at)[0];at+=4;strings.append(data[at:at+n]);at+=n
        rows.append((record,*strings))
    need(at==len(data),'native framing');return rows
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-file-final-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted final-reader receipt');prior=json.loads(Path(idx['receipt']).read_text());old=Path(idx['receipt']).parent
    sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    plan=ROOT/'work/texture_image_plan_candidate.h';before=OUT/'before-texture_image_plan_candidate.h'
    oldhash='7d4be75bc2c8824bc7a0e4f5febc13e63ba8ffa1274ca93fe123cbdab4531c0a'
    need(sources[str(plan)]==sha(before)==oldhash,'preserved old image-plan identity')
    need(plan.read_bytes()==before.read_bytes().replace(b'(v.format!=0&&v.format!=12&&v.format!=13)',b'(v.format!=0&&v.format!=3&&v.format!=12&&v.format!=13)'),'only measured BGR admission changed')
    transition=dict(path=str(plan),before=oldhash,after=sha(plan),evidence=idx,reason='Native full-reader matrix verifies60 BGR888 cases; same fresh 2D mip/metadata plan, now admitted by the existing owned bridge.')
    sources[str(plan)]=sha(plan);protected[str(before)]=oldhash
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identity closure')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in ['material_texture_converted_image_candidate.h','probe_material_texture_converted_image.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    fixtures=run/'fixtures';fixtures.mkdir();data=(old/'queries.bin').read_bytes();count=struct.unpack_from('<I',data)[0];at=4;rows=[]
    for serial in range(count):
        q=list(struct.unpack_from('<11I',data,at));at+=44;strings=[]
        for _ in range(3):n=struct.unpack_from('<I',data,at)[0];at+=4;strings.append(data[at:at+n].decode('ascii'));at+=n
        logical,key,source=strings;p=fixtures/(str(serial)+'.vtf');shutil.copyfile(source,p);need(sha(p)==sha(source),'fixture copy identity');rows.append(dict(config=q,logical=logical,key=key,source=str(p),sha256=sha(p),kind='preceding native final-reader matrix'))
    need(at==len(data) and count==prior['cases'],'preceding query framing')
    for fmt in [0,3,12,13]:
        for target in [3,12]:
            base=next(r for r in rows[:count] if struct.unpack_from('<I',Path(r['source']).read_bytes(),52)[0]==fmt and r['config'][7]==target and r['config'][8]==1)
            for mode in range(2):
                row=dict(base);q=base['config'].copy();q[4]=0
                if mode:q[0]|=0x100
                row.update(config=q,kind='selected mip subset');rows.append(row)
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<11I',*r['config'])+text(r['logical'])+text(r['key'])+text(r['source']) for r in rows))
    batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    for name,vc,src in [('native',VS.parent/'vcvars32.bat','probe_texture_file_final.cpp'),('owned',VS,'probe_material_texture_converted_image.cpp'),('plan',VS,'probe_texture_image_plan.cpp')]:
        commands += [f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{src} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2000:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings={};executables={}
    for name,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b),('plan',0x8664,0x20b)]:
        exe=run/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic,'architecture '+name)
        warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());need(not warnings[name],'compiler warnings '+name);executables[name]={'path':str(exe),'sha256':sha(exe)}
    expected=None;observations=[];available=converted=blocks=byte_count=0
    for repeat in range(2):
        target=run/('native-'+str(repeat)+'.bin');execute([run/'native.exe',LIB,query,target],'native-'+str(repeat));native=target.read_bytes();need(hashlib.sha256(native[:prior['trace_bytes']]).hexdigest()==prior['trace_sha256'],'preceding complete native trace prefix')
        records=native_records(native);need(len(records)==len(rows),'native case count');wanted=bytearray();available=converted=blocks=byte_count=0
        for serial,(row,(record,trace,pixels,aux)) in enumerate(zip(rows,records)):
            b=Path(row['source']).read_bytes();minor=struct.unpack_from('<I',b,8)[0];rawflags=struct.unpack_from('<I',b,20)[0];effective=rawflags&0x2e87fbff if minor<=3 else rawflags
            supported=True
            if minor>=3:
                supported=all(struct.unpack_from('<I',b,80+i*8)[0] in [1,0x30,0x02435243,0x02444f4c] for i in range(struct.unpack_from('<I',b,68)[0]))
            wanted.extend(struct.pack('<2I',serial,int(supported)))
            if supported:
                need(record[0]==serial and record[11]<=record[19],'selected native mip count')
                plan=[record[4],record[5],record[6],record[7],record[8],1,record[9],record[10],1,record[11],record[12],record[13],*struct.unpack_from('<3I',b,32),1]
                did_convert=int(record[20]!=0);wanted.extend(struct.pack('<3I16I5I',rawflags,effective,row['config'][0],*plan,record[16],record[17],did_convert,did_convert,did_convert))
                service=''.join(s+'\n' for s in trace.decode().splitlines() if s.startswith(('M ','FORMAT ','HDR '))).encode();at=0;end=0
                for mip in range(record[19]):
                    for frame in range(record[18]):
                        n=struct.unpack_from('<I',pixels,at)[0];at+=4+n
                        if mip<record[11]:end=at;blocks+=1;byte_count+=n
                need(at==len(pixels),'native full allocation framing');selected=pixels[:end];wanted.extend(struct.pack('<I',len(service))+service+struct.pack('<I',len(selected))+selected);available+=1;converted+=did_convert
            if repeat==0:observations.append({'serial':serial,'available':supported,'selected_mips':record[11],'native_allocation_mips':record[19],'kind':row['kind']})
        if expected is None:expected=bytes(wanted);(run/'expected-owned.bin').write_bytes(expected)
        else:need(expected==wanted,'repeated native selected result')
        target=run/('owned-'+str(repeat)+'.bin');execute([run/'owned.exe',query,target],'owned-'+str(repeat));actual=target.read_bytes()
        if actual!=expected:
            at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));(run/'mismatch.json').write_text(json.dumps({'byte':at,'actual_bytes':len(actual),'expected_bytes':len(expected),'actual_near':list(actual[max(0,at-64):at+96]),'expected_near':list(expected[max(0,at-64):at+96])},indent=2));raise RuntimeError('owned converted image mismatch at '+str(at))
    safety=execute([run/'owned.exe','--test'],'owned-safety');need('owned_converted_image_safety_checks=' in safety.stdout and 'owned_cpu_image_safety_checks=37' in safety.stdout,'lifetime/failure safety')
    # Prior image-plan matrix must remain byte-identical after the BGR admission.
    plan_idx=json.loads((ROOT/'outputs/texture-file-image-20261101/latest-verification.json').read_text());old_plan=Path(plan_idx['receipt']).parent
    target=run/'plan-regression.bin';execute([run/'plan.exe',old_plan/'plans-query.bin',target],'plan-regression');need(target.read_bytes()==(old_plan/'plans-0.bin').read_bytes(),'all preceding601 plan cases unchanged')
    for name in ['native','owned']:
        beforehash=sha(query);args=[run/(name+'.exe')]+([LIB] if name=='native' else [])+[query,query];p=execute(args,name+'-existing-output',False);need(p.returncode!=0 and sha(query)==beforehash,'existing output preserved '+name)
    need(all(sha(r['source'])==r['sha256'] for r in rows),'fixture preservation')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identity closure')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'editor dependencies unchanged');need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'current editor unchanged')
    (run/'observations.json').write_text(json.dumps(observations,indent=2));(run/'source-transition.json').write_text(json.dumps(transition,indent=2))
    result=dict(status='VERIFIED bounded owned x64 converted CPU image/lease bridge; GPU and production integration unverified',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),available=available,unavailable=len(rows)-available,converted=converted,selected_frame_mip_blocks=blocks,selected_pixel_bytes=byte_count,repetitions=2,native_trace_sha256=sha(run/'native-0.bin'),owned_trace_bytes=len(expected),owned_trace_sha256=sha(run/'expected-owned.bin'),executables=executables,compiler_warnings=warnings,owned_safety=safety.stdout,preceding_plan_cases=601,source_transition=transition,source_sha256=sources,protected_sha256=protected,editor_preservation={'dependencies':len(build['editor_source_dependencies']),'changed':[],'executable_sha256':build['executable_sha256'],'build_runtime_this_task':'NOT RUN; no changed editor dependency'},limits=['Fresh original full CPU reader and real VTF conversion compared against independent owned AMD64. The original564-case prefix remains exact, with16 added partial-selected-mip cases. No x86/original runtime dependency for the owned probe.','Supported input formats0/3/12/13, converted targets0/3/12 or passthrough. Explicit policy services and known hardware/no overrides. Unknown custom resources remain unavailable under existing immutable resource validation.','Only validated selected mip/frame blocks are published. Native full allocated mip chain may be larger. HDR pixel roundtrip, other formats/debug transforms, actual device queries, GPU creation/upload, original teardown and error-resource construction are not implemented by this stage.','Native conversion-specific DXT1 transparent cyan/alpha0 is isolated here; production generic decoder is unchanged. Raw/source-effective/requested/CTexture flags remain distinct.','No editor integration, original writes/live SFM/computer use or publication. Current owned editor and all335 dependencies unchanged.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','limits']},indent=2),flush=True)
if __name__=='__main__':main()
