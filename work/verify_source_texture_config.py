"""Read-only original config reader -> force setter -> texture commands -> D3D records."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS,DUMP
from verify_source_texture_sampling import predict,MATERIAL,SHADER
from verify_ifm_draw_packet import check_ranges
OUT=ROOT/'outputs/source-sampler-config-20261101'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
    if not b:raise RuntimeError(s)
def put(p,x):Path(p).write_text(json.dumps(x,indent=2)+'\n')
def signed(x):return x if x<0x80000000 else x-0x100000000

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/material-channel-samplers-20261101/latest-milestone.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'preceding checkpoint');before=json.loads(Path(idx['receipt']).read_text());sources=dict(before['source_sha256']);protected=dict(before['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256']
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'preceding source/protected identities')
    for name in ['probe_source_texture_config_x86.cpp',Path(__file__).name,'pe_image.py','verify_ifm_draw_packet.py','verify_source_texture_sampling.py','probe_source_texture_sampling_x86.cpp','native_private_world_image_fixture.h']:sources[str(ROOT/'work'/name)]=sha(ROOT/'work'/name)
    audit=json.loads((OUT/'native-audit.local.json').read_text());ranges=[(x['name'],int(x['start'],16),int(x['end_exclusive'],16)) for x in audit['ranges']];image,evidence=check_ranges(MATERIAL,'materialsystem-disassembly.txt',ranges);need(sha(MATERIAL)==audit['module_sha256']=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','material module identity')
    need(all(image.string(a)==s for a,s in [(0x10098a8c,'mat_forceaniso'),(0x10098ab0,'mat_filtertextures'),(0x10098ac4,'mat_mipmaptextures'),(0x10098df4,'mat_compressedtextures'),(0x100987c8,'1')]),'native registration names/defaults');need('m_nForceAnisotropicLevel' in image.string(0x10097550),'native debug field identity')
    protected[str(OUT/'native-audit.local.json')]=sha(OUT/'native-audit.local.json');console=ROOT/'outputs/source-sampling-policy-20261101/user-reported-console-20261101.json';protected[str(console)]=sha(console)
    nativeidx=json.loads((ROOT/'outputs/source-sampling-policy-20261101/latest-verification.json').read_text());need(sha(nativeidx['receipt'])==nativeidx['receipt_sha256'],'native texture oracle evidence');native=json.loads(Path(nativeidx['receipt']).read_text());check_ranges(MATERIAL,'materialsystem-disassembly.txt',[(x['name'],int(x['start'],16),int(x['end_exclusive'],16)) for x in native['authenticated_ranges'][:3]]);api,_=check_ranges(SHADER,'shaderapidx9-disassembly.txt',[(x['name'],int(x['start'],16),int(x['end_exclusive'],16)) for x in native['authenticated_ranges'][3:]])
    need([api.word(0x100fcedc+i*4) for i in [79,80,81]]==[0x1001e1e0,0x1001e2d0,0x1001e150],'shader record setter slots');need({'minimum':[api.word(0x1001e2a8+4*i) for i in range(7)],'magnification':[api.word(0x1001e384+4*i) for i in range(7)]}==native['jump_tables'],'native filter tables')
    rows=[];bits=[1,2,4,8,0x10,0x100,0x2000000,0x20000000]
    for mask,force,pattern,cap in itertools.product(range(256),[0x80000000,0xffffffff,0,1,2,4,16,0x7fffffff],range(8),[1,2,16]):
        serial=len(rows);truth=[(pattern>>b)&1 for b in range(3)];values=[(1,2,0xffffffff)[(serial//3+b)%3] if value else 0 for b,value in enumerate(truth)];flags=sum(bit for i,bit in enumerate(bits) if mask&(1<<i));rows.append([flags,force,cap,2,1+(serial%2),0x20 if serial%3 else 0,[0,1,4][serial%3],*values,1])
    for i in range(128):rows.append([sum(bit for b,bit in enumerate(bits) if i&(1<<b)),0xffffffff,2,2,2,0x20,4,1,1,1,0])
    rng=random.Random(2026110127)
    for i in range(1024):rows.append([rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(9),rng.getrandbits(8),rng.choice([0,1,2,4,255]),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),1])
    rows.append([0,1,1,2,1,0x20,4,1,1,1,1]);need(len(rows)<=65536,'bounded query count');query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<11I',*q) for q in rows))
    expected_commands=bytearray(struct.pack('<I',len(rows)));expected_records=bytearray(struct.pack('<I',len(rows)));command_count=record_count=0
    for serial,q in enumerate(rows):
        force=max(1,signed(q[1])) if q[10] else 17;norm=q[:7];norm[1]=force
        if not q[10]:norm[3]=0
        events,records=predict(norm);command_count+=len(events);record_count+=len(records);expected_commands+=struct.pack('<9I',serial,*norm,len(events))+b''.join(struct.pack('<3I',*e) for e in events);expected_records+=struct.pack('<9I',serial,*norm,len(records))+b''.join(struct.pack('<7I',*e) for e in records)
    (run/'expected-commands.bin').write_bytes(expected_commands);(run/'expected-records.bin').write_bytes(expected_records)
    batch=run/'build.cmd';lines=['@echo off',f'cd /d "{ROOT}"']
    for name,source in [('config','probe_source_texture_config_x86.cpp'),('shader','probe_source_texture_sampling_x86.cpp')]:lines+=['setlocal',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run/(name+".obj")}" /Fe:"{run/(name+".exe")}" >"{run/(name+"-build.log")}" 2>&1','if errorlevel 1 exit /b 1','endlocal']
    batch.write_text('\n'.join(lines+['exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build');executables={}
    for name in ['config','shader']:
        exe=run/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x14c and struct.unpack_from('<H',b,pe+24)[0]==0x10b,'PRIVATE x86 PE32, not x64 editor');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(name+'-build.log')).read_text());need(not warnings,'warning-free '+name);executables[name]={'path':str(exe),'sha256':sha(exe),'architecture':'PRIVATE X86 RESEARCH ONLY','compiler_warnings':warnings}
    traces={};reported_case={}
    for repeat in range(2):
        commands=run/f'commands-{repeat}.bin';configs=run/f'config-{repeat}.bin';records=run/f'records-{repeat}.bin';execute([run/'config.exe',MATERIAL,query,commands,configs],f'config-{repeat}');need(commands.read_bytes()==expected_commands,'actual original config -> sampler commands vs independent prediction');execute([run/'shader.exe','shader',SHADER,commands,records],f'shader-{repeat}');need(records.read_bytes()==expected_records,'actual original sampler records vs independent prediction')
        b=configs.read_bytes();need(struct.unpack_from('<I',b)[0]==len(rows),'config trace framing');at=4
        for serial,q in enumerate(rows):
            got=struct.unpack_from('<12I',b,at);at+=48;need(list(got)==[serial,*q],'config identity');config=b[at:at+100];at+=100;calls,last,published=struct.unpack_from('<3I',b,at);at+=12
            if q[10]:
                force=max(1,signed(q[1]));need(struct.unpack_from('<I',config,0x28)[0]==force and config[0x3c]==int(q[7]!=0) and config[0x48]==int(q[8]!=0) and config[0x3a]==int(q[9]!=0),'four actual configuration fields');need((calls,last,published)==(1,80,force),'controlled configuration DX setter/force singleton')
            else:need(config==bytes([0xcd])*100 and (calls,last,published)==(0,0,17),'disabled config leaves all fields unchanged')
            if serial==len(rows)-1:reported_case={'user_reported_values':{'mat_forceaniso':1,'mat_filtertextures':1,'mat_mipmaptextures':1,'mat_compressedtextures':1},'controlled_hardware_capability':1,'result_force_field':published,'non_aniso_texture_commands':predict([0,published,1,2,1,0x20,4])[0],'D3D_record':predict([0,published,1,2,1,0x20,4])[1],'limits':'User cvars reported separately, not live process inspection; this is the original CPU mapping under explicit reference inputs.'}
        need(at==len(b),'complete config trace extent');traces.update({str(p):sha(p) for p in [commands,configs,records]});print('PASS original config -> texture -> shader records repeat '+str(repeat),flush=True)
    need(all((run/(stem+'-0.bin')).read_bytes()==(run/(stem+'-1.bin')).read_bytes() for stem in ['commands','config','records']),'exact repeated original traces')
    before_hash=sha(query);p=execute([run/'config.exe',MATERIAL,query,query,run/'unused.bin'],'existing-output',False);need(p.returncode!=0 and sha(query)==before_hash and not (run/'unused.bin').exists(),'existing output refused without write')
    for group in [sources,protected]:need(all(sha(p)==h for p,h in group.items()),'final source/protected identities')
    editor=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(all(sha(ROOT/k)==before['source_sha256'][str(ROOT/k)] for k in editor['editor_source_dependencies']),'all365 editor sources unchanged during config research')
    result={'status':'VERIFIED original config reader to four sampler configuration fields, force setter, texture commands and ShaderAPI records; runtime integration still incomplete','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':len(rows),'repetitions':2,'material_command_events_per_run':command_count,'D3D_sampler_records_per_run':record_count,'executables':executables,'authenticated_ranges':[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],'trace_sha256':traces,'reported_case':reported_case,'source_sha256':sources,'protected_sha256':protected,'prior_milestone':idx,'editor_dependency_count_unchanged':len(editor['editor_source_dependencies']),'limits':['Entire original0x1000cc80 config reader executes unchanged in private image with39 controlled ConVar parent roots, controlled DX min/max/int setter, queue state and HDR query. Four texture-related fields, outer guards and root mutation checked; unrelated configuration values not claimed fully modeled.','Actual force getter/setter and full CTexture command loop feed separately executed original ShaderAPI setters. Global configuration publication copy authenticated statically; full OverrideConfig update scheduling not executed. No original process, DllMain/import load or GPU.','The config booleans are not read in the CTexture wrap/filter routines; this does not establish their later actual D3D sampler-application or format negotiation effects. No current cvar inspection; supplied values remain user-reported.','No editor/runtime source change, complete-editor rebuild, default policy switch, canonical promotion or publication. No portable/general hardware policy claim. Existing native x64 evidence retained separately.']}
    receipt=run/'results.json';put(receipt,result);put(OUT/'latest-verification.json',{'receipt':str(receipt),'receipt_sha256':sha(receipt)});print(json.dumps({k:result[k] for k in ['status','cases','repetitions','material_command_events_per_run','D3D_sampler_records_per_run','reported_case']},indent=2),flush=True)
if __name__=='__main__':main()
