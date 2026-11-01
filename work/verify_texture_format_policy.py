"""Installed format request/decision policy with explicit controlled services."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/texture-format-policy-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def predict(q):
    source,flags,current,config,enabled,hdr,selected=q;events=[(0,enabled,0,0)];changed=0;final=current;vtf=source
    if enabled&255:
        if source in [22,23,24,25,26,27,28,29] or config&255 and source in [13,14,15,20,34,35,60,61,62]:request=source
        elif flags&0x3000:request=12
        elif source==5:request=5
        else:request=3
        events += [(3,source,0,0),(1,request,1,selected)]
        if current!=selected:events.append((4,selected,0,0));changed=1;final=selected;vtf=selected
        else:
            events.append((2,hdr,0,0))
            if hdr==1 and selected==24:events += [(4,25,0,0),(4,24,0,0)];changed=1;vtf=24
    return changed,final,vtf,events
def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/vtf-bgr888-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted current milestone');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding identities')
    for n in ['texture_format_policy_candidate.h','probe_texture_format_policy.cpp',Path(__file__).name]:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_format_request',0x1002c800,0x1002c896),('whole_format_conversion_decision',0x1002d070,0x1002d129),('format_metadata_accessor',0x100496d0,0x100496e0)])
    registry=[]
    for f in range(64):
        name=image.string(image.word(0x10101b84+f*12));flag=image.b[image.offset(0x10101b84+f*12)+11]&1;registry.append({'format':f,'name':name,'compressed':bool(flag)})
    need([r['format'] for r in registry if r['compressed']]==[13,14,15,20,34,35,60,61,62],'native compressed metadata');need(registry[3]['name']=='BGR888','native opaque default')
    (run/'format-registry.json').write_text(json.dumps(registry,indent=2));rows=[];coverage={}
    def add(q,label):rows.append(q);coverage[label]=coverage.get(label,0)+1
    for source,flags,config,enabled,current_mode,selected,hdr in itertools.product(range(64),[0,0x1000,0x2000,0x3000,0x80000,0xffffffff],[0,1,0x100,0x101],[0,1,0x100,0x101],range(3),[3,12,24],[0,1,0x100]):
        current=[0,source,selected][current_mode];add([source,flags,current,config,enabled,hdr,selected],'format_flag_config_service_matrix')
    for source,byte,high in itertools.product([0,5,13,24,34],range(256),[0,0xa5b60000]):
        add([source,0,0,high|byte,1,0,3],'compression_low_byte');add([source,0,0,1,high|byte,0,3],'enabled_low_byte')
    rng=random.Random(2026110107)
    for _ in range(2048):add([rng.randrange(64),rng.getrandbits(32),rng.choice([0,24,rng.randrange(64)]),rng.getrandbits(32),rng.choice([0,1,0x100,rng.getrandbits(32)]),rng.choice([0,1,0x100,rng.getrandbits(32)]),rng.randrange(64)],'seeded_valid_formats')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<7I',*q) for q in rows));expected=bytearray();total_events=changed=hdr_roundtrips=disabled=0
    for serial,q in enumerate(rows):
        change,final,vtf,events=predict(q);expected.extend(struct.pack('<5I',serial,change,final,vtf,len(events)));expected.extend(b''.join(struct.pack('<4I',*e) for e in events));total_events+=len(events);changed+=change;hdr_roundtrips+=int(len([e for e in events if e[0]==4])==2);disabled+=int(not(q[4]&255))
    (run/'expected.bin').write_bytes(expected);batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    for arch,vc in [('native',VS.parent/'vcvars32.bat'),('owned',VS)]:commands += [f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_texture_format_policy.cpp /Fo:"{run/(arch+".obj")}" /Fe:"{run/(arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1600:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings={};executables={}
    for arch,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        exe=run/(arch+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic,'architecture '+arch);warnings[arch]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(arch+'-build.log')).read_text());need(not warnings[arch],'compiler warnings '+arch);executables[arch]={'path':str(exe),'sha256':sha(exe)}
        for repeat in range(2):
            dest=run/(arch+'-'+str(repeat)+'.bin');execute([exe,LIB if arch=='native' else run/'absent-original.dll',query,dest],arch+'-'+str(repeat));observed=dest.read_bytes()
            if observed!=expected:
                at=next((i for i,(a,b) in enumerate(zip(observed,expected)) if a!=b),min(len(observed),len(expected)));cursor=0
                for serial,q in enumerate(rows):
                    n=20+len(predict(q)[3])*16
                    if cursor<=at<cursor+n:(run/'mismatch.json').write_text(json.dumps({'serial':serial,'query':q,'relative_byte':at-cursor,'expected':list(expected[cursor:cursor+n]),'observed':list(observed[cursor:cursor+n])},indent=2));break
                    cursor+=n
                raise RuntimeError(arch+' trace mismatch at '+str(at))
        before=sha(query);p=execute([exe,LIB if arch=='native' else run/'absent-original.dll',query,query],arch+'-existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preservation '+arch)
    safety=execute([run/'owned.exe','--test'],'owned-safety');need('owned_format_policy_safety_checks=12' in safety.stdout,'safety count')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final identities')
    result=dict(status='VERIFIED bounded native/owned x64 format policy; pixel conversion and full file tail unverified',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),coverage=coverage,events=total_events,changed=changed,hdr_roundtrips=hdr_roundtrips,disabled=disabled,repetitions=2,trace_bytes=len(expected),trace_sha256=sha(run/'expected.bin'),executables=executables,compiler_warnings=warnings,owned_safety=safety.stdout,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],source_sha256=sources,protected_sha256=protected,limits=['Original CTexture format request and conversion-decision routines execute unchanged in private x86 image. Installed metadata formats0..63 authenticated; hardware nearest-format result, enable/HDR services and VTF conversion are controlled.','Owned result is a transactional decision plan. It does not query actual native hardware, convert pixels, claim successful file/GPU loading or mutate production state.','Conversion callbacks log requested targets and update the supplied fake VTF format only. HDR25/24 request ordering is verified, not its floating-point image results.','No original writes/live SFM/computer use, editor dependency changes/rebuild/runtime or publication. Current BGR-corrected canonical runtime remains protected.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges','limits']},indent=2),flush=True)
if __name__=='__main__':main()
