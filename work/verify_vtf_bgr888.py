"""Native CPU format3=BGR888 vs owned VTF decoding, with explicit old/new evidence."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,re,struct,subprocess,sys,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/vtf-bgr888-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
HEADER=ROOT/'work/vtf_block_decode.h'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def text(s):
    b=str(s).encode('ascii');return struct.pack('<I',len(b))+b
def main():
    baseline='--baseline' in sys.argv;OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='baseline-' if baseline else 'verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-file-postprocess-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'preceding receipt');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    transition=None
    if not baseline:
        previous=json.loads((OUT/'baseline.json').read_text());need(previous['status']=='VERIFIED BASELINE BUG','baseline required');need(previous['old_header_sha256']!=sha(HEADER),'source correction required');transition={'path':str(HEADER),'before':previous['old_header_sha256'],'after':sha(HEADER)}
    runtime_transition=None
    if not baseline and (OUT/'canonical-transition.json').exists():
        journal=json.loads((OUT/'canonical-transition.json').read_text());need(journal['status']=='VERIFIED canonical content identity and clean startup','verified runtime transition required')
        need(sha(journal['canonical'])==journal['after'] and sha(journal['backup'])==journal['before'],'runtime/backup identities')
        matches=[p for p in protected if Path(p).resolve()==Path(journal['canonical']).resolve()];need(len(matches)==1 and protected[matches[0]]==journal['before'],'historical canonical identity')
        protected[matches[0]]=journal['after'];protected[journal['backup']]=journal['before'];runtime_transition=journal
    for group in [sources,protected]:
        for p,h in group.items():
            if not baseline and Path(p).resolve()==HEADER.resolve():need(h==transition['before'],'historical decoder identity')
            else:need(sha(p)==h,'preceding identity '+p)
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
    for n,h in build['editor_source_dependencies'].items():
        if not baseline and (ROOT/n).resolve()==HEADER.resolve():need(h==transition['before'],'published old decoder identity')
        else:need(sha(ROOT/n)==h,'editor source identity '+n)
    graph=set()
    def visit(p):
        p=p.resolve()
        if p in graph:return
        graph.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for name in ['probe_vtf_bgr888.cpp',Path(__file__).name]:visit(ROOT/'work'/name)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('CPU_conversion',0x10048ec0,0x100496c0)])
    names={i:image.string(image.word(0x10101b84+i*12)) for i in [0,2,3,12]};need(names=={0:'RGBA8888',2:'RGB888',3:'BGR888',12:'BGRA8888'},'native format registry')
    fixtures=run/'fixtures';fixtures.mkdir();rows=[];expected=bytearray();query=bytearray();coverage={}
    for minor,fmt,(w,h),frames in itertools.product(range(1,6),[0,3,12],[(2,2),(4,8),(16,4)],[1,2]):
        mips=max(w,h).bit_length();header=64 if minor==1 else 80 if minor==2 else 88;b=bytearray(header);payload={}
        for o,v in [(0,0x00465456),(4,7),(8,minor),(12,header),(52,fmt),(57,0xffffffff)]:struct.pack_into('<I',b,o,v)
        for o,v in [(16,w),(18,h),(24,frames)]:struct.pack_into('<H',b,o,v)
        if minor>1:struct.pack_into('<H',b,63,1)
        b[56]=mips
        if minor>2:struct.pack_into('<I',b,68,1);struct.pack_into('<2I',b,80,0x30,header)
        for mip in reversed(range(mips)):
            for frame in range(frames):
                mw,mh=max(1,w>>mip),max(1,h>>mip);raw=bytearray();rgba=bytearray()
                for i in range(mw*mh):
                    red,green,blue,alpha=(i*37+mip*53+frame*71+3)%256,(i*19+11)%256,(i*67+97)%256,(i*23+frame*41)%256
                    raw.extend([blue,green,red] if fmt==3 else [blue,green,red,alpha] if fmt==12 else [red,green,blue,alpha]);rgba.extend([red,green,blue,255 if fmt==3 else alpha])
                payload[mip,frame]=(bytes(raw),bytes(rgba));b.extend(raw)
        source=fixtures/(str(len(list(fixtures.iterdir())))+'.vtf');source.write_bytes(b)
        for mip in range(mips):
            for frame in range(frames):
                raw,rgba=payload[mip,frame];mw,mh=max(1,w>>mip),max(1,h>>mip);serial=len(rows);query.extend(struct.pack('<6I',fmt,mw,mh,mip,frame,len(raw))+text(source)+raw);expected.extend(struct.pack('<4I',serial,mw,mh,len(rgba))+rgba);rows.append({'format':fmt,'source':str(source),'sha256':sha(source)});coverage[str(fmt)]=coverage.get(str(fmt),0)+1
    q=run/'queries.bin';q.write_bytes(struct.pack('<I',len(rows))+query);(run/'expected.bin').write_bytes(expected)
    batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    for arch,vc in [('native',VS.parent/'vcvars32.bat'),('owned',VS)]:commands += [f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_vtf_bgr888.cpp /Fo:"{run/(arch+".obj")}" /Fe:"{run/(arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1400:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings={};executables={};mismatches=[]
    for arch,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        exe=run/(arch+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic,'architecture '+arch);warnings[arch]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(arch+'-build.log')).read_text());need(not warnings[arch],'compiler warnings');executables[arch]={'path':str(exe),'sha256':sha(exe)}
        for repeat in range(2):
            dest=run/(arch+'-'+str(repeat)+'.bin');execute([exe,LIB if arch=='native' else run/'absent-original.dll',q,dest],arch+'-'+str(repeat));observed=dest.read_bytes()
            if baseline and arch=='owned':
                offset=0;bad=[]
                for serial,row in enumerate(rows):
                    n=struct.unpack_from('<I',expected,offset+12)[0];length=16+n
                    if observed[offset:offset+length]!=expected[offset:offset+length]:bad.append(serial);need(row['format']==3,'unrelated baseline mismatch')
                    offset+=length
                need(len(bad)==coverage['3'],'all BGR baseline mismatches required');mismatches=bad
            else:need(observed==expected,arch+' native/independent RGBA mismatch')
        need(sha(run/(arch+'-0.bin'))==sha(run/(arch+'-1.bin')),'complete repeat trace '+arch)
        before=sha(q);p=execute([exe,LIB if arch=='native' else run/'absent-original.dll',q,q],arch+'-existing-output',False);need(p.returncode!=0 and sha(q)==before,'existing output preserved')
    need(all(sha(r['source'])==r['sha256'] for r in rows),'fixtures unchanged')
    result=dict(status='VERIFIED BASELINE BUG' if baseline else 'VERIFIED BGR888 decoding correction',time_utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),coverage=coverage,repetitions=2,baseline_wrong_cases=len(mismatches),format_registry=names,executables=executables,compiler_warnings=warnings,trace_bytes=len(expected),expected_sha256=sha(run/'expected.bin'),old_header_sha256=sha(HEADER) if baseline else transition['before'],source_transition=transition,runtime_transition=runtime_transition,source_sha256=sources,protected_sha256=protected,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges])
    for group in [sources,protected]:
        for p,h in group.items():
            if not baseline and Path(p).resolve()==HEADER.resolve() and h==transition['before']:continue
            need(sha(p)==h,'final identity '+p)
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');index={'receipt':str(receipt),'receipt_sha256':sha(receipt)}
    if baseline:(OUT/'baseline.json').write_text(json.dumps(result,indent=2)+'\n')
    else:(OUT/'latest-verification.json').write_text(json.dumps(index,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges']},indent=2),flush=True)
if __name__=='__main__':main()
