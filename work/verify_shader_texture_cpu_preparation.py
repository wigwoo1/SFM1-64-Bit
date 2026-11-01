"""Actual native shader/manager CPU prefix vs owned pending lookup preparation."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,re,shutil,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_texture_file_image import text
from verify_material_texture_converted_image import native_records
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/texture-manager-cpu-prefix-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def records(data):
    at=0;rows=[]
    while at<len(data):
        record=list(struct.unpack_from('<32I',data,at));at+=128;strings=[]
        for _ in range(4):n=struct.unpack_from('<I',data,at)[0];at+=4;strings.append(data[at:at+n]);at+=n
        rows.append((record,*strings))
    need(at==len(data),'native record framing');return rows
def queries(p):
    data=Path(p).read_bytes();count=struct.unpack_from('<I',data)[0];at=4;rows=[]
    for serial in range(count):
        q=list(struct.unpack_from('<11I',data,at));at+=44;strings=[]
        for _ in range(3):n=struct.unpack_from('<I',data,at)[0];at+=4;strings.append(data[at:at+n].decode('ascii'));at+=n
        rows.append((q,strings))
    need(at==len(data),'source query framing');return rows
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-block-formats-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted checkpoint');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding source/protected identities')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in ['probe_texture_manager_cpu_prefix.cpp','shader_texture_cpu_preparation_candidate.h','probe_shader_texture_cpu_preparation.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('shader_normal',0x10037a30,0x10037b20),('find_or_load',0x10042350,0x100423aa),('create_and_download',0x100419b0,0x10041a60),('download',0x10030600,0x10030655),('download_inner',0x10030590,0x100305f6),('core_to_CPU_return',0x100301d0,0x100302d9),('first_GPU_allocation_gate',0x1002db40,0x1002db79),('whole_CPU_reader',0x1002fdb0,0x100301cf),('filename_format',0x1006e590,0x1006e5bc)])
    need(image.string(0x10095990)=='materials/%s.vtf','native filename template')
    roots=[]
    for folder,index_name in [('material-texture-converted-image-20261101','latest-milestone.json'),('texture-block-formats-20261101','latest-milestone.json')]:
        index=json.loads((ROOT/'outputs'/folder/index_name).read_text());need(sha(index['receipt'])==index['receipt_sha256'],'accepted reference index');root=json.loads(Path(index['receipt']).read_text());cpu=root['cpu_receipt'];need(sha(cpu['receipt'])==cpu['receipt_sha256'],'CPU receipt identity');base=Path(cpu['receipt']).parent;roots.append(base)
    rows=[];fixtures=run/'fixtures';fixtures.mkdir()
    for base in roots:
        oldrows=queries(base/'queries.bin');oldrecords=native_records((base/'native-0.bin').read_bytes());need(len(oldrows)==len(oldrecords),'reference framing')
        for (q,(logical,key,source)),reference in zip(oldrows,oldrecords):
            p=fixtures/(str(len(rows))+'.vtf');shutil.copyfile(source,p);need(sha(p)==sha(source),'fixture snapshot');rows.append(dict(config=q+[1],logical=logical,name=key,source=str(p),sha256=sha(p),reference=reference))
    for device in [0,0x100,2,0xffffffff]:
        row=dict(rows[0]);q=row['config'].copy();q[0]=0x3000;q[8]=1;q[11]=device;row.update(config=q);rows.append(row)
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<12I',*r['config'])+text(r['logical'])+text(r['name'])+text(r['source']) for r in rows))
    batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"'];jobs=[('native',VS.parent/'vcvars32.bat','probe_texture_manager_cpu_prefix.cpp'),('owned',VS,'probe_shader_texture_cpu_preparation.cpp')]
    for name,vc,source in jobs:commands += ['setlocal',f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');executables={};warnings={}
    for name,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        exe=run/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic,'architecture '+name);warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());need(not warnings[name],'warnings '+name);executables[name]={'path':str(exe),'sha256':sha(exe)}
    expected=None;native=[];observations=[];available=prepared=inactive=blocks=0
    for repeat in range(2):
        dest=run/('native-'+str(repeat)+'.bin');execute([run/'native.exe',LIB,query,dest],'native-'+str(repeat));native.append(dest.read_bytes());actual=records(native[-1]);need(len(actual)==len(rows),'prefix case count');wanted=bytearray();available=prepared=inactive=blocks=0
        for serial,(r,(record,name,group,trace,pixels)) in enumerate(zip(rows,actual)):
            q=r['config'];device=bool(q[11]&255);active=device and bool(q[8]&255);reference,_,oldpixels,_=r['reference'];source=Path(r['source']).read_bytes();minor=struct.unpack_from('<I',source,8)[0]
            if active:
                flags=reference[4]
                # Extra device-gate cases change the requested alpha flags only;
                # the supplied nearest result/source pixels remain identical.
                if serial>=sum(len(queries(p/'queries.bin')) for p in roots):flags=(reference[4]&~rows[0]['config'][0])|q[0]
                expected_record=[serial,1,0,1,0,0,flags,reference[5],int(bool(flags&0x3000)),reference[5]&1,reference[6],reference[7],reference[8],reference[9],reference[10],reference[11],reference[12],reference[13],reference[16],reference[17],reference[14],reference[15],reference[19],reference[18],reference[1],reference[2],reference[3],reference[20],reference[21],reference[22],reference[23],1]
                need(pixels==oldpixels,'actual shader/manager CPU pixels vs standalone full reader');prepared+=1
                steps=trace.decode().splitlines();need(steps[:4]==['C 0','D '+str(q[11]),'B','M '+str(q[8])] and steps[-1]=='STOP BEFORE GPU ALLOCATION' and 'E' not in steps,'pending manager transaction trace')
            else:
                flags=q[0] if device else 0;expected_record=[serial,0,1,3,1,0,flags,0,int(bool(flags&0x3000)),0,0xffffffff]+[0]*20+[1];need(not pixels,'inactive no CPU pixels');inactive+=1
                steps=['C 0','D '+str(q[11])]+(['B','M '+str(q[8]),'E'] if device else []);need(trace.decode().splitlines()==steps,'inactive actual normal-return trace')
            if record!=expected_record:
                (run/'mismatch.json').write_text(json.dumps({'serial':serial,'config':q,'actual':record,'expected':expected_record,'trace':trace.decode()},indent=2));raise RuntimeError('manager CPU prefix state mismatch '+str(serial))
            need(name.decode()==r['name'] and group==b'Prefix group','native first request name/group')
            supported=active
            if minor>=3:supported=supported and all(struct.unpack_from('<I',source,80+i*8)[0] in [1,0x30,0x02435243,0x02444f4c] for i in range(struct.unpack_from('<I',source,68)[0]))
            wanted.extend(struct.pack('<2I',serial,int(supported)))
            if supported:
                plan=[record[6],record[7],record[10],record[11],record[12],1,record[13],record[14],1,record[15],record[16],record[17],*struct.unpack_from('<3I',source,32),1]
                wanted.extend(struct.pack('<I',len(name))+name+struct.pack('<I',len(group))+group+struct.pack('<2I16I2I',q[0],record[8],*plan,record[18],record[19]));at=0;end=0
                for mip in range(record[22]):
                    for frame in range(record[23]):
                        n=struct.unpack_from('<I',pixels,at)[0];at+=4+n
                        if mip<record[15]:end=at;blocks+=1
                need(at==len(pixels),'native full pixel framing');selected=pixels[:end];wanted.extend(struct.pack('<I',len(selected))+selected);available+=1
            if repeat==0:observations.append(dict(serial=serial,native_CPU_prepared=active,owned_pending_available=supported,manager_cache_entries=record[2],material_value_type=record[3],refcount=record[4],zero_GPU_handles=record[31],trace=trace.decode()))
        if expected is None:expected=bytes(wanted);(run/'expected-owned.bin').write_bytes(expected)
        else:need(expected==wanted,'repeat prediction')
        dest=run/('owned-'+str(repeat)+'.bin');p=execute([run/'owned.exe',query,dest],'owned-'+str(repeat));actual=dest.read_bytes()
        if actual!=expected:
            at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));(run/'owned-mismatch.json').write_text(json.dumps({'byte':at,'actual_bytes':len(actual),'expected_bytes':len(expected),'actual_near':list(actual[max(0,at-60):at+100]),'expected_near':list(expected[max(0,at-60):at+100])},indent=2));raise RuntimeError('owned pending preparation mismatch '+str(at))
    need(native[0]==native[1],'repeated native trace')
    for name in ['native','owned']:
        before=sha(query);args=[run/(name+'.exe')]+([LIB] if name=='native' else [])+[query,query];p=execute(args,name+'-existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preserved')
    need(all(sha(r['source'])==r['sha256'] for r in rows),'fixtures unchanged')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identity closure')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'editor source unchanged');need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'editor runtime unchanged')
    (run/'observations.json').write_text(json.dumps(observations,indent=2));result=dict(status='VERIFIED bounded native shader/manager pre-GPU CPU preparation and owned pending adapter; cache/GPU commit incomplete',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),native_CPU_prepared=prepared,inactive_normal_returns=inactive,owned_pending_available=available,owned_unavailable=len(rows)-available,selected_frame_mip_blocks=blocks,repetitions=2,native_trace_sha256=sha(run/'native-0.bin'),owned_trace_sha256=sha(run/'expected-owned.bin'),executables=executables,compiler_warnings=warnings,owned_log=(run/'owned-0.log').read_text(),source_sha256=sources,protected_sha256=protected,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],limits=['Native whole normal shader lookup, manager creation/download prefix and real CPU reader/conversion execute. Active misses stop at100302D9 before GPU allocation and BEFORE manager insertion/material assignment: cache0, material string, refcount0, all GPU handles zero. No fake successful GPU-core return.','Inactive native gates return normally, cache an unloaded texture and assign a typed texture. The pending adapter does not implement that complete inactive-cache path. Its shader find callback deliberately returns unavailable after CPU staging, preserving existing typed state.','Owned adapter is an opt-in cold ordinary normal-miss CPU preparation with known hardware/no overrides and supplied read-only resource identity. Unknown custom resources remain unavailable. No cache publication, error resource or GPU-ready state is fabricated.','Native prefix abort uses a private test-thread hardware boundary; host discards guarded allocations, not proven original teardown. Real renderer device selection/GPU upload, cache commit/eviction/teardown, failed-file/error construction and other shader texture kinds are unverified here.','Original installation/assets/sessions read-only; no live SFM/computer use or publication. No editor dependency/build/runtime changes.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges','limits']},indent=2),flush=True)
if __name__=='__main__':main()
