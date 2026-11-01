"""Real native VTF upload requests vs owned x64 retained upload plans."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP
from verify_ifm_draw_packet import check_ranges
from verify_material_texture_converted_image import native_records
from verify_shader_texture_cpu_preparation import queries
OUT=ROOT/'outputs/texture-upload-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def text(s):
    b=s.encode('ascii');return struct.pack('<I',len(b))+b
def binary(b):return struct.pack('<I',len(b))+b
def predict(serial,q,reference):
    record,_,raw,_=reference;frames=record[12];mips=record[11] if q[13]==0xffffffff else q[13];target=record[6] if q[14]==0xffffffff else q[14];format=record[16];blocks={};at=0
    for mip in range(record[19]):
        for frame in range(record[18]):n=struct.unpack_from('<I',raw,at)[0];at+=4;blocks[frame,mip]=raw[at:at+n];at+=n
    need(at==len(raw) and frames==record[18] and mips<=record[11],'reference frame/mip bounds');events=bytearray();uploads=0
    for frame in range(frames):
        handle=0x4000+frame if q[16]==0 or q[16]==2 and frame==0 else 0;events.extend(struct.pack('<3I',1,frame,handle))
        if q[12]:
            for mip in range(mips):
                pixels=blocks[frame,mip];w,h=max(1,record[14]>>mip),max(1,record[15]>>mip);size=((w+3)//4)*((h+3)//4)*(16 if format==15 else 8) if format in [13,15] else w*h*(3 if format==3 else 4);need(size==len(pixels),'reference byte extent')
                events.extend(struct.pack('<11I',2,frame,mip,0,target,0,w,h,format,0,len(pixels))+pixels);uploads+=1
    return struct.pack('<7I',serial,frames,mips,format,target,frames,uploads)+binary(events),uploads,len(events),frames

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-allocation-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted allocation milestone');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding source/protected identities')
    prefix=(ROOT/'work/probe_texture_file_final.cpp').read_text().split('int main(int argc,char**argv){try{',1)[0];need((ROOT/'work/texture_file_final_native_fixture.h').read_text()=='#pragma once\n// Exact owned service fixture prefix from probe_texture_file_final.cpp.\n'+prefix,'exact preceding fixture reuse')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in ['texture_file_final_native_fixture.h','probe_texture_upload.cpp','material_texture_upload_plan_candidate.h','probe_material_texture_upload_plan.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_upload',0x1002d1d0,0x1002d2ea),('core_ordinary_upload_call',0x10030400,0x1003042e)])
    roots=[]
    for name in ['material-texture-converted-image-20261101','texture-block-formats-20261101']:
        i=json.loads((ROOT/'outputs'/name/'latest-milestone.json').read_text());need(sha(i['receipt'])==i['receipt_sha256'],'reference milestone identity');m=json.loads(Path(i['receipt']).read_text());cpu=m['cpu_receipt'];need(sha(cpu['receipt'])==cpu['receipt_sha256'],'CPU reference receipt');base=Path(cpu['receipt']).parent;r=json.loads(Path(cpu['receipt']).read_text());need(sha(base/'native-0.bin')==r['native_trace_sha256'],'CPU native trace');protected[str(base/'native-0.bin')]=sha(base/'native-0.bin');roots.append(base)
    rows=[];fixtures=run/'fixtures';fixtures.mkdir();coverage={};fixture_hashes={}
    def add(q,strings,reference,supported,kind):
        rows.append(dict(config=q,strings=strings,reference=reference,supported=supported));coverage[kind]=coverage.get(kind,0)+1
    samples=[]
    for base in roots:
        qs=queries(base/'queries.bin');refs=native_records((base/'native-0.bin').read_bytes());need(len(qs)==len(refs),'reference query framing')
        for (q,(logical,key,oldsource)),reference in zip(qs,refs):
            p=fixtures/(str(len(fixture_hashes))+'.vtf');shutil.copyfile(oldsource,p);need(sha(p)==sha(oldsource),'fixture copy');fixture_hashes[str(p)]=sha(p);strings=[logical,key,str(p)];b=p.read_bytes();minor=struct.unpack_from('<I',b,8)[0];supported=minor<3 or all(struct.unpack_from('<I',b,80+i*8)[0] in [1,0x30,0x02435243,0x02444f4c] for i in range(struct.unpack_from('<I',b,68)[0]))
            for extra,kind in [([0,1,0xffffffff,0xffffffff,0,0],'normal_full'),([0xffffffff,1,0xffffffff,0xffffffff,0xffffffff,1],'unused_arg_zero_handles_no_status_gate'),([999,0,0xffffffff,12,2,2],'no_faces_selects_only'),([7,1,1,63,0,2],'one_mip_explicit_destination')]:add(q+extra,strings,reference,supported,kind)
            if len(samples)<20 and supported:samples.append((q,strings,reference,supported))
    for q,strings,reference,supported in samples:add(q+[123,1,0,3,0,0],strings,reference,supported,'zero_mips_selects_only')
    need(len(rows)<=8192,'probe case bound');query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<17I',*r['config'])+b''.join(text(s) for s in r['strings']) for r in rows));expected=bytearray();expected_owned=bytearray();uploads=bytes_count=selects=available=0
    for serial,r in enumerate(rows):
        packet,n,size,frames=predict(serial,r['config'],r['reference']);expected.extend(packet);expected_owned.extend(struct.pack('<2I',serial,int(r['supported'])));uploads+=n;bytes_count+=size-frames*12-n*44;selects+=frames
        if r['supported']:expected_owned.extend(packet);available+=1
    (run/'expected-native.bin').write_bytes(expected);(run/'expected-owned.bin').write_bytes(expected_owned)
    batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    for name,vc,source in [('native',VS.parent/'vcvars32.bat','probe_texture_upload.cpp'),('owned',VS,'probe_material_texture_upload_plan.cpp')]:commands+=['setlocal',f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=240);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');executables={};warnings={}
    for name,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        exe=run/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic,'architecture '+name);warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());need(not warnings[name],'warnings '+name);executables[name]={'path':str(exe),'sha256':sha(exe)}
    for repeat in range(2):
        for name,wanted in [('native',expected),('owned',expected_owned)]:
            dest=run/(name+'-'+str(repeat)+'.bin');args=[run/(name+'.exe')]+([LIB] if name=='native' else [])+[query,dest];execute(args,name+'-'+str(repeat));actual=dest.read_bytes()
            if actual!=wanted:
                at=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),min(len(actual),len(wanted)));(run/'mismatch.json').write_text(json.dumps({'probe':name,'repeat':repeat,'byte':at,'actual_near':list(actual[max(0,at-60):at+100]),'expected_near':list(wanted[max(0,at-60):at+100])},indent=2));raise RuntimeError('upload command/pixel mismatch '+name+' byte '+str(at))
    safety=execute([run/'owned.exe','--test'],'owned-safety').stdout;imports=execute([DUMP,'/imports',run/'owned.exe'],'owned-imports').stdout;dlls=re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$',imports,re.M|re.I);need(dlls==['KERNEL32.dll'],'owned runtime independence')
    for name in ['native','owned']:
        before=sha(query);args=[run/(name+'.exe')]+([LIB] if name=='native' else [])+[query,query];p=execute(args,name+'-existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preserved')
    need(all(sha(p)==h for p,h in fixture_hashes.items()),'fixture preservation')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final source/protected identities')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'editor dependencies unchanged');need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'editor unchanged')
    result=dict(status='VERIFIED bounded original ordinary2D upload commands/pixels and owned x64 retained plan; actual GPU submission incomplete',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),available=available,unavailable=len(rows)-available,coverage=coverage,select_commands=selects,upload_commands=uploads,pixel_bytes=bytes_count,repetitions=2,native_trace_sha256=sha(run/'expected-native.bin'),owned_trace_sha256=sha(run/'expected-owned.bin'),executables=executables,compiler_warnings=warnings,owned_safety=safety,owned_imports=dlls,source_sha256=sources,protected_sha256=protected,fixture_sha256=fixture_hashes,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],limits=['Real original CPU reader/VTF conversion precedes whole1002D1D0 upload. Native shader-API select/upload services capture commands/pointer bounds and bytes; original RX code/metadata/handles/all VTF owned storage remain unchanged. No actual GPU/device allocation/submission or native teardown.','First upload parameter is unused; CTexture frame count drives selection. Zero handles/status clear do not suppress commands. Zero face/mip count still selects each frame. Ordinary face0/slice0 only; cubemap/volume traversal remains unverified.','Owned plan retains immutable mip leases and frame indices, not x86 handles. No GPU/cache/material-ready state. Formats0/3/12/13/15 source; destination0..63 is an explicit command field, not measured GPU support/conversion. Unknown custom resources remain unavailable.','No editor integration/source dependency/build/runtime changed. Original installation/assets/sessions read-only; no original SFM process/computer use or feature publication. Actual Vulkan mapping and GPU failure cleanup must precede completed shader lookup.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','fixture_sha256','authenticated_ranges','limits']},indent=2),flush=True)
if __name__=='__main__':main()
