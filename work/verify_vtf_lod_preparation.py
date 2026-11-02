"""Owned x64 integration against the frozen private LOD CPU reference corpus."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,struct,re,subprocess,tempfile,os
from verify_session_preview_aspect import ROOT,VS,DUMP,INCLUDE
OUT=ROOT/'outputs/vtf-lod-preparation-20261101'
MODIFIED=['vtf_texture_layout.h','texture_image_plan_candidate.h','material_texture_cpu_image_candidate.h','model_prepared_texture_candidate.h','probe_material_texture_cpu_image.cpp']
NEW=['test_vtf_lod_preparation.cpp',Path(__file__).name]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,s):
    if not ok:raise RuntimeError(s)
def put(p,v):Path(p).write_text(json.dumps(v,indent=2)+'\n')
def current():
    b=json.loads((OUT/'before.json').read_text());need(sha(b['parent']['receipt'])==b['parent']['receipt_sha256'],'parent receipt')
    sources=dict(b['source_sha256']);transitions={}
    for n in MODIFIED:
        p=str(ROOT/'work'/n);need(sha(OUT/'baseline'/n)==sources[p],'baseline '+n)
        transitions[p]={'before':sources[p],'after':sha(p)};sources[p]=sha(p)
    for n in NEW:sources[str(ROOT/'work'/n)]=sha(ROOT/'work'/n)
    for group in [sources,b['protected_sha256']]:need(all(sha(p)==h for p,h in group.items()),'current identities')
    return b,sources,transitions
def command(args,label,run,timeout=600):
    env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    p=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW)
    (run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2000:]);return p.stdout+p.stderr
def compile(run,name,source,libs='',vulkan=False):
    exe=run/(name+'.exe');batch=run/(name+'-build.cmd');inc=f'/I"{INCLUDE}"' if vulkan else ''
    batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 {inc} work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{exe}" {libs} >"{run/(name+"-build.log")}" 2>&1','exit /b %errorlevel%','']))
    command(['cmd.exe','/d','/c',batch],name+'-build-driver',run)
    b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'AMD64 PE32+')
    command([DUMP,'/imports',exe],name+'-imports',run)
    warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text())
    print('Fresh '+name+' warnings='+str(len(warnings)),flush=True)
    return exe,warnings
def main():
    before,sources,transitions=current();run=Path(tempfile.mkdtemp(prefix='cpu-',dir=OUT));print(run,flush=True);(OUT/'active-cpu.txt').write_text(str(run))
    idx=json.loads((ROOT/'outputs/vtf-lod-contract-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'frozen native receipt');prior=json.loads(Path(idx['receipt']).read_text());d=Path(prior['discovery'])
    need(sha(d/'native-0.bin')==prior['native_trace_sha256'] and (d/'native-0.bin').read_bytes()==(d/'native-1.bin').read_bytes(),'frozen native bytes')
    rows=json.loads((d/'cases.json').read_text());data=(d/'native-0.bin').read_bytes();p=0;wanted=bytearray();available=blocks=pixel_bytes=0
    for r in rows:
        rec=struct.unpack_from('<28I',data,p);p+=112;strings=[]
        for _ in range(3):n=struct.unpack_from('<I',data,p)[0];p+=4;strings.append(data[p:p+n]);p+=n
        ok=r['lod'] is None or max(r['lod'][:2])<=30
        wanted.extend(struct.pack('<2I',r['serial'],int(ok)))
        if ok:
            wanted.extend(struct.pack('<19I',r['flags'],r['flags']&0x2e87fbff if r['minor']<=3 else r['flags'],r['request'],*rec[5:21]))
            raw=strings[2];q=end=0
            for level in range(rec[27]):
                for frame in range(r['frames']):
                    n=struct.unpack_from('<I',raw,q)[0];q+=4+n
                    if level<rec[14]:end=q;blocks+=1;pixel_bytes+=n
            need(q==len(raw),'reference pixel framing');wanted.extend(struct.pack('<I',end)+raw[:end]);available+=1
    need(p==len(data),'complete reference trace');(run/'expected.bin').write_bytes(wanted)
    exes={};warnings={}
    for name,source in [('cpu-image','probe_material_texture_cpu_image.cpp'),('lod-unit','test_vtf_lod_preparation.cpp'),('legacy','test_vtf_legacy_no_thumbnail.cpp')]:
        exe,w=compile(run,name,source);need(not w,'CPU warnings');exes[name]={'path':str(exe),'sha256':sha(exe)};warnings[name]=w
    logs=[]
    for repeat in range(2):
        dest=run/f'owned-{repeat}.bin';logs.append(command([run/'cpu-image.exe',d/'queries.bin',dest],f'cpu-image-{repeat}',run));need(dest.read_bytes()==wanted,'complete native versus owned selected plan/pixels')
        logs.append(command([run/'cpu-image.exe','--test'],f'cpu-safety-{repeat}',run))
        logs.append(command([run/'lod-unit.exe'],f'lod-unit-{repeat}',run))
        logs.append(command([run/'legacy.exe',run/f'legacy-fixtures-{repeat}'],f'legacy-{repeat}',run))
    need(all(sha(r['source'])==r['source_sha256'] for r in rows),'frozen disposable fixtures preserved')
    current();receipt=run/'results.json';result=dict(status='VERIFIED bounded owned x64 LOD CPU preparation; full editor/GPU pending',time_utc=datetime.now(timezone.utc).isoformat(),source_sha256=sources,source_transitions=transitions,protected_sha256=before['protected_sha256'],native_reference_receipt=idx,cases=len(rows),available=available,refused=len(rows)-available,selected_blocks=blocks,selected_pixel_bytes=pixel_bytes,repetitions=2,executables=exes,compiler_warnings=warnings,logs=logs,expected_sha256=sha(run/'expected.bin'),previous_goal_turn='PROGRESS')
    put(receipt,result);put(OUT/'latest-cpu.json',dict(receipt=str(receipt),receipt_sha256=sha(receipt)));print(json.dumps({k:result[k] for k in ['status','cases','available','refused','selected_blocks','selected_pixel_bytes']},indent=2))
if __name__=='__main__':main()
