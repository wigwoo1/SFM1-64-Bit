"""Real private native cache insertion/factory integration; file/GPU core stops."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/texture-manager-miss-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,why):
    if not ok:raise RuntimeError(why)
def text(s):
    b=s.encode('ascii');return struct.pack('<I',len(b))+b
def canonical(name):
    s=name.lower()
    if not (len(s)>4 and s.endswith('.hdr')):
        i=len(s)-1
        while i>0 and s[i] not in './\\':i-=1
        if i>0 and s[i]=='.':s=s[:i]
    return s.replace('\\','/')
def predict(sequences):
    data=bytearray();core=misses=hits=0
    for serial,rows in enumerate(sequences):
        cache={}
        for step,(flags,device,load,name,group) in enumerate(rows):
            if name.startswith('\\') or len(name)>1 and name[1]=='\\':name=name[1:]
            key=canonical(name);trace='C 0\n'
            if key not in cache:
                misses+=1;trace+='D '+str(device)+'\n';stored=0
                if device&255:
                    trace+='B\nM '+str(load)+'\n';stored=flags
                    if load&255:core+=1;trace+='L BOUNDARY\n'
                    trace+='E\n'
                cache[key]=(len(cache)+1,stored,group)
            else:hits+=1
            identity,stored,retained_group=cache[key]
            data.extend(struct.pack('<10I',serial,step,identity,len(cache),stored,0,int(bool(stored&0x3000)),0,1,0))
            data.extend(text(key)+text(retained_group)+text(trace))
    return bytes(data),{'core_boundaries':core,'misses':misses,'hits':hits}

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/shader-texture-services-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted service receipt');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256'];need(all(sha(p)==h for p,h in sources.items()),'prior sources');need(all(sha(p)==h for p,h in protected.items()),'prior protected inputs')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for name in ['probe_texture_manager_miss.cpp',Path(__file__).name]:visit(ROOT/'work'/name)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('shader_normal_load',0x10037a30,0x10037b20),('whole_find_or_load',0x10042350,0x100423aa),
        ('whole_create_and_download',0x100419b0,0x10041a60),('texture_factory',0x10030460,0x100304a6),
        ('texture_constructor',0x1002fb00,0x1002fba8),('texture_name_assignment',0x1002cc60,0x1002cd14),
        ('whole_download',0x10030600,0x10030655),('whole_download_inner',0x10030590,0x100305f6),
        ('texture_alpha_query',0x1002cdd0,0x1002cddf),('texture_error_query',0x1002ce50,0x1002ce57),
        ('cache_constructor',0x10041e00,0x10041eea),('cache_insertion',0x10041ef0,0x10041f4f),
        ('GetTextureValue',0x10018130,0x1001821a),('retain',0x10005dc0,0x10005dcd),('release',0x1002dd90,0x1002dd9b),
        ('symbol_init_boundary',0x100738e0,0x10073926),('symbol_text_boundary',0x10073930,0x1007394a)])
    need(image.word(0x1009cc74+28)==0x10042350,'manager binding');need(image.word(0x1009ad14+52)==0x10030600 and image.word(0x1009ad14+24)==0x1002cdd0 and image.word(0x1009ad14+60)==0x1002ce50,'texture virtual bindings')
    need(image.word(0x10098f34)==0x10018130 and image.word(0x10098f34+56)==0x10019710,'material bindings');need(image.string(0x1009ab74)=='.hdr','native suffix')
    batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_texture_manager_miss.cpp /Fo:"{run/"native.obj"}" /Fe:"{run/"native.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1200:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');log=(run/'build.log').read_text();warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',log);need(not warnings,'probe warnings');exe=run/'native.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b,'explicit reference architecture')
    sequences=[];coverage={}
    def add(rows,label):sequences.append(rows);coverage[label]=coverage.get(label,0)+1
    add([(0,0,0,'fixture/shared.vtf','First group'),(0x3000,1,1,'FIXTURE/SHARED.VTF','Later group'),(0xffffffff,1,1,'fixture/shared.png','Third group'),(0x3000,1,1,'fixture/new','First group'),(0xffffffff,1,1,'fixture/new.vtf','Later group')],'analytical_first_request_and_inactive_cache')
    for flags,device,load in itertools.product([0,0x1000,0x2000,0x3000,0x80000,0xffffffff],[0,1,2,255,0x100,0x80000001],[0,1,0x100,0xffffffff]):
        add([(flags,device,load,'fixture/gate.vtf','First group'),(flags^0xffffffff,1,1,'FIXTURE/GATE.PNG','Later group'),(0x3000,0,0,'fixture/gate','Third group')],'download_gate_matrix')
    names=['Normal/Name.vtf','Normal\\Name.PNG','Normal/Name.hdr','Normal/Name.HDR','\\leading\\name.vtf','a\\odd.vtf','/rooted/logical','nested.with.dot/path.ext','x','x'*259]
    for name in names:add([(0x1000,1,0,name,'First group'),(0x2000,1,1,name.upper(),'Later group')],'name_normalization_and_hits')
    rows=[(i&0x3000,1,i&1,'growth/item'+str(i)+'.vtf','Growth group') for i in range(80)];rows += [(0xffffffff,0,0,'GROWTH/ITEM'+str(i)+'.PNG','Ignored group') for i in reversed(range(80))];add(rows,'native_tree_growth_and_reuse')
    rng=random.Random(2026110102)
    for seq in range(64):
        rows=[]
        for _ in range(12):
            key=rng.randrange(6);name='seed'+str(seq)+'/item'+str(key)+rng.choice(['','.vtf','.png','.hdr']);rows.append((rng.getrandbits(32),rng.choice([0,1,0x100,0xffffffff]),rng.choice([0,1,0x100,0xffffffff]),name,rng.choice(['GroupA','GroupB','GroupC'])))
        add(rows,'seeded_cache_sequences')
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(sequences))+b''.join(struct.pack('<I',len(rows))+b''.join(struct.pack('<3I',f,d,l)+text(n)+text(g) for f,d,l,n,g in rows) for rows in sequences));wanted,counts=predict(sequences);(run/'expected.bin').write_bytes(wanted);requests=sum(map(len,sequences));logs=[]
    for repeat in range(2):
        dest=run/('native-'+str(repeat)+'.bin');p=execute([exe,LIB,query,dest],'native-'+str(repeat));logs.append(p.stdout);actual=dest.read_bytes()
        if actual!=wanted:
            at=next((i for i,(x,y) in enumerate(zip(actual,wanted)) if x!=y),min(len(actual),len(wanted)));(run/'mismatch.json').write_text(json.dumps({'byte':at,'actual_length':len(actual),'expected_length':len(wanted),'actual_near':list(actual[max(0,at-100):at+400]),'expected_near':list(wanted[max(0,at-100):at+400])},indent=2));raise RuntimeError('native independent mismatch byte '+str(at))
        need('requests='+str(requests) in p.stdout and 'file_gpu_core_boundaries='+str(counts['core_boundaries']) in p.stdout,'complete execution counts')
    before=sha(query);p=execute([exe,LIB,query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output refused unchanged')
    # This investigation does not alter owned runtime code. Re-exercise the exact
    # accepted owned policy safety executable, without calling it a fresh build.
    owned=prior['executables']['x64'];need(sha(owned['path'])==owned['sha256'],'accepted owned policy executable');p=execute([owned['path'],'--test'],'owned-policy-regression');need(p.stdout==prior['owned_safety'],'owned policy regression')
    need(all(sha(p)==h for p,h in sources.items()),'source identities after');need(all(sha(p)==h for p,h in protected.items()),'protected identities after')
    result={'status':'VERIFIED bounded native integration; actual file/GPU loading UNVERIFIED','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','sequences':len(sequences),'requests':requests,'coverage':coverage,'counts':counts,'repetitions':2,'trace_bytes':len(wanted),'trace_sha256':sha(run/'expected.bin'),'executable':{'path':str(exe),'sha256':sha(exe),'architecture':'x86 private original-code reference'},'compiler_warnings':warnings,'logs':logs,'owned_policy_regression':p.stdout,'reused_owned_executable':owned,'authenticated_ranges':[{k:v for k,v in r.items() if k!='instructions'} for r in ranges],'source_sha256':sources,'protected_sha256':protected,'limits':['Whole native normal shader service, FindOrLoad, create-and-download, CTexture constructor/name handling, real cache insertion/growth/lookup, Download wrappers and CMaterialVar SetTexture/GetTextureValue/Clear execute.','Only symbol intern/text and file/GPU core100301D0 are hardware-observed controlled boundaries on this disposable probe thread; executable bytes remain RX unchanged.','Download device/debug gate results are explicit supplied services, not measured native configuration. File/GPU core returns without reading files or mutating pixels. Native alpha/error queries are meaningful only for these constructed/request-flag states.','Cache has no alias entries or texture overrides. Cache removal/destruction and native symbol-pool lifetime are unverified. Private tracked allocations are guard-checked and discarded by host after each sequence.','No new owned x64 runtime component or editor integration/rebuild/GPU run, live SFM/computer use, original writes or publication. Fresh build is deliberately x86 reference only; accepted owned x64 policy safety executable reused by hash.']}
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','sequences','requests','coverage','counts','trace_bytes','trace_sha256','executable','compiler_warnings','owned_policy_regression']},indent=2),flush=True)

if __name__=='__main__':main()
