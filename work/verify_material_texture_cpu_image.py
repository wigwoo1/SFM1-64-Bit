"""Fresh native image oracle vs owned x64 immutable selected pixel leases."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib,json,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_texture_file_image import size,text

OUT=ROOT/'outputs/material-texture-cpu-image-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def records(data):
    offset=0;rows=[]
    while offset<len(data):
        record=list(struct.unpack_from('<28I',data,offset));offset+=112;strings=[]
        for _ in range(3):n=struct.unpack_from('<I',data,offset)[0];offset+=4;strings.append(data[offset:offset+n]);offset+=n
        rows.append((record,*strings))
    need(offset==len(data),'native framing');return rows

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-file-image-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted image receipt');prior=json.loads(Path(idx['receipt']).read_text());old=Path(idx['receipt']).parent
    sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for name in ['probe_material_texture_cpu_image.cpp','material_texture_cpu_image_candidate.h',Path(__file__).name]:visit(ROOT/'work'/name)
    data=(old/'queries.bin').read_bytes();offset=4;count=struct.unpack_from('<I',data)[0];rows=[];query=bytearray(struct.pack('<I',count));fixtures=run/'fixtures';fixtures.mkdir()
    for serial in range(count):
        config=struct.unpack_from('<7I',data,offset);offset+=28;strings=[]
        for _ in range(3):n=struct.unpack_from('<I',data,offset)[0];offset+=4;strings.append(data[offset:offset+n].decode('ascii'));offset+=n
        logical,key,source=strings;destination=fixtures/(str(serial)+'.vtf');shutil.copyfile(source,destination);need(sha(source)==sha(destination),'snapshot identity')
        rows.append(dict(serial=serial,config=config,logical=logical,key=key,source=str(destination),sha256=sha(destination)))
        query.extend(struct.pack('<7I',*config)+text(logical)+text(key)+text(destination))
    need(offset==len(data),'query framing');q=run/'queries.bin';q.write_bytes(query)
    batch=run/'build.cmd';lines=['@echo off',f'cd /d "{ROOT}"']
    for arch,vc,source in [('native',VS.parent/'vcvars32.bat','probe_texture_file_image.cpp'),('owned',VS,'probe_material_texture_cpu_image.cpp')]:
        lines += [f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(arch+".obj")}" /Fe:"{run/(arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(lines+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');executables={};warnings={}
    for arch,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        exe=run/(arch+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic,'architecture '+arch)
        warnings[arch]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(arch+'-build.log')).read_text());need(not warnings[arch],'compiler warnings '+arch);executables[arch]={'path':str(exe),'sha256':sha(exe)}
    print('Fresh native/x64 builds complete',flush=True)
    wanted=None;available=0;selected_blocks=0;selected_bytes=0;observations=[]
    for repeat in range(2):
        target=run/('native-'+str(repeat)+'.bin');execute([run/'native.exe',LIB,q,target],'native-'+str(repeat));need(sha(target)==prior['trace_sha256'],'fresh complete native trace regression')
        native=records(target.read_bytes());need(len(native)==count,'native case count')
        expected=bytearray();available=selected_blocks=selected_bytes=0
        for row,(record,filename,trace,pixels) in zip(rows,native):
            b=Path(row['source']).read_bytes();minor=struct.unpack_from('<I',b,8)[0];header=struct.unpack_from('<I',b,12)[0];w,h=struct.unpack_from('<2H',b,16);frames=struct.unpack_from('<H',b,24)[0];flags=struct.unpack_from('<I',b,20)[0];fmt=struct.unpack_from('<I',b,52)[0];mips=b[56]
            declared_size=header+sum(size(max(1,w>>m),max(1,h>>m),fmt)*frames for m in range(mips))
            skip=record[16];levels=record[14];ok=bool(row['config'][6] and b[:4]==b'VTF\0' and len(b)==declared_size and record[1] and skip+levels<=mips)
            expected.extend(struct.pack('<2I',row['serial'],int(ok)))
            if ok:
                effective=flags&0x2e87fbff if minor<=3 else flags
                expected.extend(struct.pack('<3I16I',flags,effective,row['config'][0],*record[5:21]))
                cursor=0;end=0
                for mip in range(record[27]):
                    for frame in range(frames):
                        n=struct.unpack_from('<I',pixels,cursor)[0];cursor+=4+n
                        if mip<levels:selected_blocks+=1;selected_bytes+=n;end=cursor
                need(cursor==len(pixels),'native pixels framing');selected=pixels[:end];expected.extend(struct.pack('<I',len(selected))+selected);available+=1
            if repeat==0:observations.append(dict(serial=row['serial'],available=ok,native_accepted=bool(record[1]),selected_mips=levels,skip=skip,declared_mips=mips,selected_bytes=len(pixels[:end]) if ok else 0))
        if wanted is None:wanted=bytes(expected);(run/'expected-owned.bin').write_bytes(wanted)
        else:need(wanted==expected,'native selected reference repeat')
        dest=run/('owned-'+str(repeat)+'.bin');execute([run/'owned.exe',q,dest],'owned-'+str(repeat));actual=dest.read_bytes()
        if actual!=wanted:
            at=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),min(len(actual),len(wanted)));(run/'mismatch.json').write_text(json.dumps({'byte':at,'actual_bytes':len(actual),'expected_bytes':len(wanted),'actual_near':list(actual[max(0,at-64):at+96]),'expected_near':list(wanted[max(0,at-64):at+96])},indent=2));raise RuntimeError('owned native pixel/plan mismatch at '+str(at))
    safety=execute([run/'owned.exe','--test'],'owned-safety');need('PASS owned_cpu_image_safety_checks=' in safety.stdout,'owned safety output')
    for arch in ['native','owned']:
        before=sha(q);args=[run/(arch+'.exe')]+([LIB] if arch=='native' else [])+[q,q];p=execute(args,arch+'-existing-output',False);need(p.returncode!=0 and sha(q)==before,'existing output preserved '+arch)
    need(all(sha(r['source'])==r['sha256'] for r in rows),'fixture preservation')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identities')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/n)==h for n,h in build['editor_source_dependencies'].items()),'editor dependencies unchanged');editor=ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe';need(sha(editor)==build['executable_sha256'],'editor unchanged')
    (run/'observations.json').write_text(json.dumps(observations,indent=2));(run/'fixtures.json').write_text(json.dumps(rows,indent=2))
    result=dict(status='VERIFIED bounded owned x64 retained CPU image/plan bridge; GPU and production integration UNVERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=count,available=available,unavailable=count-available,selected_frame_mip_blocks=selected_blocks,selected_pixel_bytes=selected_bytes,repetitions=2,native_trace_sha256=prior['trace_sha256'],owned_trace_bytes=len(wanted),owned_trace_sha256=sha(run/'expected-owned.bin'),executables=executables,compiler_warnings=warnings,owned_safety=safety.stdout,source_sha256=sources,protected_sha256=protected,editor_preservation={'dependencies':len(build['editor_source_dependencies']),'changed':[],'path':str(editor),'sha256':sha(editor),'build_this_task':'NOT RUN; no changed dependency','runtime_this_task':'NOT RUN'},limits=['Native original reference is private x86; owned CPU image/plan and its leases run in independent AMD64 with no original DLL/process or GPU dependency.','Original native file/image trace repeats the preceding complete accepted trace; exact selected frame/mip bytes and CTexture plan match. Full native allocated chain remains distinct from the subset exposed as validated owned pixels.','Missing/invalid input and absent selected mips remain unavailable; no error resource is fabricated. Only the documented fresh 2D RGBA/BGRA/DXT1 plan and known explicit hardware/override policy are admitted. LOD resource and depth>1 refused by the new bridge.','This is an isolated opt-in bridge. It does not perform conversion, upload, original cache teardown or active benchmark material publication. Editor/runtime rendering unchanged; no visual fidelity claim.','No original writes, live SFM/computer use or publication. Dedicated workspace fixture copies only.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','limits']},indent=2),flush=True)

if __name__=='__main__':main()
