"""Native allocation commands/state vs owned x64 request plan; no real GPU."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP
from verify_ifm_draw_packet import check_ranges
from verify_shader_texture_cpu_preparation import records as cpu_records
OUT=ROOT/'outputs/texture-allocation-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def text(s):
    b=s.encode('ascii');return struct.pack('<I',len(b))+b

def predict(serial,q,name,group):
    flags,status,fmt,w,h,d,mips,frames,depth_mode,device,mode=q
    active=bool(device&255);handles=[0]*frames;steps=['D '+str(device)]
    if active:
        api=int(bool(flags&0x4000))
        if flags&0x80000 and fmt not in [24,27,28,29]:api|=0x20000
        api|=2 if flags&0x8000 else 4
        if flags&1:api|=0x1000
        if flags&0x4000000:api|=0x40
        count=1 if flags&0x8000 and flags&0x10000 and frames==2 else frames
        copies=6 if flags&0x800 and not flags&0x40000 else 1
        depth=1 if flags&0x10000 and depth_mode==4 else d
        args=[count,w,h,depth,fmt,mips,copies,api]
        steps+=['CREATE '+' '.join(map(str,args)),'NAME '+name,'GROUP '+group]
        for i in range(count):
            if mode==1 or mode==2 and i==0:handles[i]=0x4000+i
        if flags&0x10000:
            steps+=['DEPTH '+' '.join(map(str,[fmt,w,h,int(depth_mode==4)])),'NAME '+name+'_ZBuffer'];handles[1]=0x5000 if mode==1 else 0
    words=[serial,int(active),status|(2 if active else 0),int(active),int(active and bool(flags&0x10000)),*handles]
    trace=''.join(s+'\n' for s in steps)
    return struct.pack('<'+'I'*len(words),*words)+text(trace),dict(active=active,zero_handles=not any(handles),partial_handles=any(handles) and not all(handles),depth=active and bool(flags&0x10000))

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-manager-cpu-prefix-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted preceding milestone');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding source/protected identities')
    seen=set()
    def visit(p):
        p=p.resolve()
        if p in seen:return
        seen.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in ['probe_texture_allocation.cpp','texture_allocation_request_candidate.h','probe_texture_allocation_request.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_allocation',0x1002db40,0x1002dd12),('procedural_query',0x1002ce20,0x1002ce2a),('factory',0x10030460,0x100304a6)])
    need(image.string(0x1009abd8)=='%s_ZBuffer','depth name template');need(image.word(0x1009ad14+0x5c)==0x1002ce20,'real CTexture procedural virtual')
    rows=[];coverage={}
    def add(q,name='fixture/allocation',group='Allocation group',kind='controlled'):
        need(len(rows)<16384,'native batch bound');rows.append((q,name,group));coverage[kind]=coverage.get(kind,0)+1
    bits=[1,0x800,0x4000,0x8000,0x10000,0x40000,0x80000,0x4000000]
    for fmt,mask in itertools.product([3,12,24,25,26,27,28,29],range(256)):
        flags=sum(bit for i,bit in enumerate(bits) if mask>>i&1);frames=[1,2,3,8][mask%4]
        if flags&0x10000:frames=max(2,frames)
        add([flags,[0,1,2,0xa5a50080][mask%4],fmt,16,8,4,5,frames,[0,4,0xffffffff][mask%3],1,mask%3],kind='all8_relevant_flag_combinations')
    for fmt,mask in itertools.product(range(64),range(8)):
        flags=sum(bit for i,bit in enumerate([0x80000,0x8000,0x10000]) if mask>>i&1);add([flags,0,fmt,1,65535,2,1,2,4,1,1],kind='all64_formats')
    for low,high in itertools.product(range(256),[0,0xa5ff0000]):add([0xffffffff,0x80,12,32,16,8,6,3,4,high|low,1],kind='device_low_byte')
    for status,mode,flags,frames,depth_mode in itertools.product([0,1,2,3,0x80,0xffffffff],range(3),[0,0x18000,0x10000],[2,3,8],[0,4,0xffffffff]):add([flags,status,25,8,16,4,3,frames,depth_mode,1,mode],kind='status_zero_partial_and_full_handles')
    rng=random.Random(2026110111)
    for i in range(1024):
        flags=rng.getrandbits(32);frames=rng.choice([2,3,8]) if flags&0x10000 else rng.choice([1,2,3,8]);q=[flags,rng.getrandbits(32),rng.randrange(64),rng.randint(1,65535),rng.randint(1,65535),rng.randint(1,65535),rng.randint(1,65535),frames,rng.choice([0,4,0xffffffff]),rng.choice([0,1,0x100,0x101,0xffffffff]),rng.randrange(3)]
        add(q,'fixture/'+('a'*71 if i==0 else 'allocation_'+str(i)),('G'*255 if i==0 else 'Other group'),kind='seeded_descriptors')
    cpu=Path(prior['cpu_receipt']['receipt']);need(sha(cpu)==prior['cpu_receipt']['receipt_sha256'],'preceding CPU receipt identity');cpu_info=json.loads(cpu.read_text());cpu_trace=cpu.parent/'native-0.bin';need(sha(cpu_trace)==cpu_info['native_trace_sha256'],'preceding actual CPU trace identity');protected[str(cpu_trace)]=sha(cpu_trace)
    for record,name,group,trace,pixels in cpu_records(cpu_trace.read_bytes()):
        if record[1]:add([record[6],record[7],record[10],record[13],record[14],1,record[15],record[16],0,1,1],name.decode(),group.decode(),kind='actual_prepared_CPU_descriptors')
    query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<11I',*q)+text(name)+text(group) for q,name,group in rows));expected=bytearray();stats={'active':0,'inactive':0,'success_with_zero_handles':0,'success_with_partial_handles':0,'depth_calls':0}
    for serial,(q,name,group) in enumerate(rows):
        record,observation=predict(serial,q,name,group);expected.extend(record);active=observation['active'];stats['active' if active else 'inactive']+=1;stats['success_with_zero_handles']+=int(active and observation['zero_handles']);stats['success_with_partial_handles']+=int(active and observation['partial_handles']);stats['depth_calls']+=int(observation['depth'])
    (run/'expected.bin').write_bytes(expected)
    batch=run/'build.cmd';commands=['@echo off',f'cd /d "{ROOT}"']
    for name,vc,source in [('native',VS.parent/'vcvars32.bat','probe_texture_allocation.cpp'),('owned',VS,'probe_texture_allocation_request.cpp')]:commands+=['setlocal',f'call "{vc}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(commands+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');executables={};warnings={}
    for name,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        exe=run/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic,'architecture '+name);warnings[name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());need(not warnings[name],'warnings '+name);executables[name]={'path':str(exe),'sha256':sha(exe)}
    for repeat,name in itertools.product(range(2),['native','owned']):
        dest=run/(name+'-'+str(repeat)+'.bin');args=[run/(name+'.exe')]+([LIB] if name=='native' else [])+[query,dest];execute(args,name+'-'+str(repeat));actual=dest.read_bytes()
        if actual!=expected:
            at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));(run/'mismatch.json').write_text(json.dumps({'probe':name,'repeat':repeat,'byte':at,'actual_near':list(actual[max(0,at-60):at+100]),'expected_near':list(expected[max(0,at-60):at+100])},indent=2));raise RuntimeError('allocation command/state mismatch '+name+' byte '+str(at))
    safety=execute([run/'owned.exe','--test'],'owned-safety').stdout
    imports=execute([DUMP,'/imports',run/'owned.exe'],'owned-imports').stdout;dlls=re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$',imports,re.M|re.I);need(dlls==['KERNEL32.dll'],'owned original-runtime independence')
    for name in ['native','owned']:
        before=sha(query);args=[run/(name+'.exe')]+([LIB] if name=='native' else [])+[query,query];p=execute(args,name+'-existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preserved')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final source/protected identities')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'editor dependencies unchanged');need(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'editor unchanged')
    result=dict(status='VERIFIED bounded allocation command/state contract and owned x64 request planner; real GPU allocation/upload incomplete',time_utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='PROGRESS',cases=len(rows),coverage=coverage,observations=stats,repetitions=2,trace_sha256=sha(run/'expected.bin'),executables=executables,compiler_warnings=warnings,owned_safety=safety,owned_imports=dlls,source_sha256=sources,protected_sha256=protected,authenticated_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],limits=['Native original allocation executes to its natural return with unchanged RX code and controlled shader-API device/create/depth services. Supplied IDs are not real GPU handles. Native status bit2/ALtrue do not check zero or partial returned handles.','Owned helper plans commands only, captures descriptor and device service, refuses unsupported descriptors transactionally, and has no handles/status/cache or GPU-ready result. Inactive device plans contain no create/depth commands.','Measured domain: formats0..63, positive16-bit dimensions/mips,1..8 frames (depth flag requires>=2), canonical ASCII names<=79/nonempty groups<=255. No validity/support claim for those formats or combinations on real hardware.','Actual prior prepared CPU descriptors included; planner not connected to editor/cache/Vulkan. Native upload1002D1D0, deallocation/full teardown, GPU failures and commit rollback remain next.','No original installation writes or SFM process/computer use/publication; no editor dependency change, rebuild/runtime not repeated.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['source_sha256','protected_sha256','authenticated_ranges','limits']},indent=2),flush=True)
if __name__=='__main__':main()
